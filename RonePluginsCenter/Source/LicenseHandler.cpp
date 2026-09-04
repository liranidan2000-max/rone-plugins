#include "LicenseHandler.h"
#include "BundleLicenseFile.h"

// ============================================================================
// Construction / Destruction
// ============================================================================

LicenseHandler::LicenseHandler()  {}

LicenseHandler::~LicenseHandler()
{
    stopTimer();
}

// ============================================================================
// Thread-safe getters
// ============================================================================

juce::String LicenseHandler::getLicenseKey() const
{
    juce::ScopedLock sl (lock);
    return licenseKey;
}

juce::String LicenseHandler::getInstanceId() const
{
    juce::ScopedLock sl (lock);
    return instanceId;
}

juce::String LicenseHandler::getCustomerName() const
{
    juce::ScopedLock sl (lock);
    return customerName;
}

juce::String LicenseHandler::getStatusMessage() const
{
    juce::ScopedLock sl (lock);
    return statusMessage;
}

// ============================================================================
// The shared XML file — written through BundleLicenseFile so that the account
// path's claim on the same file survives everything this class does to it.
// ============================================================================

void LicenseHandler::saveLicenseFile()
{
    bool isLicensed = licensed.load();
    juce::String key, instId, custName;
    juce::int64 validatedAt = 0;

    {
        juce::ScopedLock sl (lock);
        key         = licenseKey;
        instId      = instanceId;
        custName    = customerName;
        validatedAt = lastValidationTime;
    }

    // Only the serial's own attributes go out. Composing the whole document
    // here is what used to erase products= — the plugins the same customer had
    // bought outright — every time this serial re-validated.
    BundleLicenseFile::writeSerial (isLicensed, key, instId, custName, validatedAt);
}

// ============================================================================
// Load license state from shared XML
// ============================================================================

bool LicenseHandler::loadLicenseFile()
{
    const auto file = BundleLicenseFile::load();

    // serialLicensed is this class's own claim, and it is the only one worth
    // reading here. The merged `licensed` can be true because the ACCOUNT's
    // pass is active, and adopting that would send a non-key to Lemon Squeezy
    // and let the failure tear down a perfectly good file.
    if (! file.serialLicensed)
        return false;

    // A file written by an older Center could still carry the account path's
    // "account:<email>" where a serial belongs. Not ours to re-validate.
    if (file.licenseKey.startsWith ("account:"))
        return false;

    if (file.licenseKey.isEmpty() || file.instanceId.isEmpty())
        return false;

    // Committed only once the file is known to be ours, so a rejected one
    // never leaves half its contents in the fields the UI reads.
    juce::ScopedLock sl (lock);
    licenseKey         = file.licenseKey;
    instanceId         = file.instanceId;
    customerName       = file.customerName;
    lastValidationTime = file.lastValidationTime;

    return true;
}

// ============================================================================
// Drop this class's claim on the shared file
// ============================================================================

void LicenseHandler::clearLicenseFile()
{
    // Drops the serial only. A revoked or deactivated Lemon Squeezy key says
    // nothing about a plugin the same person bought outright, and deleting the
    // file here used to revoke it anyway. BundleLicenseFile removes the file
    // when — and only when — nothing is left to claim.
    BundleLicenseFile::clearSerial();
}

// ============================================================================
// Machine identifier for Lemon Squeezy instance_name
// ============================================================================

juce::String LicenseHandler::getMachineInstanceName() const
{
    auto deviceId = juce::SystemStats::getUniqueDeviceID();
    if (deviceId.isNotEmpty())
        return deviceId;

    return juce::SystemStats::getComputerName();
}

// ============================================================================
// HTTP POST helper for Lemon Squeezy License API
// ============================================================================

juce::var LicenseHandler::postToLemonSqueezy (const juce::String& endpoint,
                                               const juce::StringPairArray& params)
{
    juce::URL url ("https://api.lemonsqueezy.com/v1/licenses/" + endpoint);

    for (auto& key : params.getAllKeys())
        url = url.withParameter (key, params.getValue (key, ""));

    auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                       .withConnectionTimeoutMs (15000)
                       .withExtraHeaders ("Accept: application/json");

    auto stream = url.createInputStream (options);

    if (stream == nullptr)
        return juce::var();

    auto responseBody = stream->readEntireStreamAsString();
    auto parsed = juce::JSON::parse (responseBody);

    if (! parsed.isObject())
    {
        DBG ("[LicenseHandler] Malformed response from Lemon Squeezy: " + responseBody.substring (0, 200));
        return juce::var();
    }

    return parsed;
}

// ============================================================================
// Initialize — called once at startup
// ============================================================================

void LicenseHandler::initialize()
{
    if (loadLicenseFile())
    {
        auto now     = juce::Time::currentTimeMillis();
        auto elapsed = now - lastValidationTime;

        if (elapsed < OFFLINE_GRACE_MS)
        {
            licensed.store (true);
            {
                juce::ScopedLock sl (lock);
                statusMessage = "Licensed — RONE Full Bundle";
            }

            // Try to re-validate in background
            validateLicenseAsync();

            // Start periodic re-validation
            startTimer ((int) VALIDATION_INTERVAL_MS);
        }
        else
        {
            // Grace period expired — must validate online
            licensed.store (false);
            {
                juce::ScopedLock sl (lock);
                statusMessage = "License validation required. Please connect to the internet.";
            }

            validateLicenseAsync ([this] (bool valid)
            {
                if (! valid)
                {
                    juce::ScopedLock sl (lock);
                    statusMessage = "License expired or invalid. Please re-activate.";
                }
            });
        }
    }
    else
    {
        licensed.store (false);
        {
            juce::ScopedLock sl (lock);
            statusMessage = "No license found. Enter your RONE Full Bundle key.";
        }
    }
}

// ============================================================================
// Activate a license key
// ============================================================================

void LicenseHandler::activateLicense (const juce::String& key,
                                       std::function<void (bool, juce::String)> callback)
{
    auto instanceName = getMachineInstanceName();

    juce::Thread::launch ([this, key, instanceName, callback]()
    {
        juce::StringPairArray params;
        params.set ("license_key",   key);
        params.set ("instance_name", instanceName);

        auto json = postToLemonSqueezy ("activate", params);

        juce::MessageManager::callAsync ([this, json, key, callback]()
        {
            if (json.isVoid())
            {
                callback (false, "Network error. Check your internet connection.");
                return;
            }

            bool activated = (bool) json.getProperty ("activated", false);

            if (! activated)
            {
                auto error = json.getProperty ("error", "Activation failed.").toString();
                callback (false, error);
                return;
            }

            // Verify store and product IDs
            auto meta = json.getProperty ("meta", juce::var());
            if (meta.isObject())
            {
                int storeId   = (int) meta.getProperty ("store_id",   0);
                int productId = (int) meta.getProperty ("product_id", 0);

                if (EXPECTED_STORE_ID != 0 && storeId != EXPECTED_STORE_ID)
                {
                    callback (false, "This license key is not valid for RONE Full Bundle.");
                    return;
                }
                if (EXPECTED_PRODUCT_ID != 0 && productId != EXPECTED_PRODUCT_ID)
                {
                    callback (false, "This license key is not valid for RONE Full Bundle.");
                    return;
                }
            }

            // Extract instance ID
            auto instance = json.getProperty ("instance", juce::var());
            auto instId   = instance.isObject()
                              ? instance.getProperty ("id", "").toString()
                              : juce::String();

            // Extract customer name from meta
            auto custName = meta.isObject()
                              ? meta.getProperty ("customer_name", "").toString()
                              : juce::String();

            {
                juce::ScopedLock sl (lock);
                licenseKey         = key;
                instanceId         = instId;
                customerName       = custName;
                lastValidationTime = juce::Time::currentTimeMillis();
                statusMessage      = "Licensed — RONE Full Bundle";
            }
            licensed.store (true);

            saveLicenseFile();
            startTimer ((int) VALIDATION_INTERVAL_MS);

            if (onLicenseStateChanged)
                onLicenseStateChanged (true);

            callback (true, "License activated successfully!");
        });
    });
}

// ============================================================================
// Validate an existing license key + instance
// ============================================================================

void LicenseHandler::validateLicenseAsync (std::function<void (bool)> callback)
{
    juce::String key;
    juce::String instId;

    {
        juce::ScopedLock sl (lock);
        key    = licenseKey;
        instId = instanceId;
    }

    if (key.isEmpty() || instId.isEmpty())
    {
        if (callback) callback (false);
        return;
    }

    juce::Thread::launch ([this, key, instId, callback]()
    {
        juce::StringPairArray params;
        params.set ("license_key", key);
        params.set ("instance_id", instId);

        auto json = postToLemonSqueezy ("validate", params);

        juce::MessageManager::callAsync ([this, json, callback]()
        {
            if (json.isVoid())
            {
                // Network error — keep cached state, don't revoke
                if (callback) callback (licensed.load());
                return;
            }

            bool valid = (bool) json.getProperty ("valid", false);

            if (valid)
            {
                // Verify store/product IDs
                auto meta = json.getProperty ("meta", juce::var());
                if (meta.isObject())
                {
                    int storeId   = (int) meta.getProperty ("store_id",   0);
                    int productId = (int) meta.getProperty ("product_id", 0);

                    if ((EXPECTED_STORE_ID != 0 && storeId != EXPECTED_STORE_ID) ||
                        (EXPECTED_PRODUCT_ID != 0 && productId != EXPECTED_PRODUCT_ID))
                    {
                        valid = false;
                    }
                }
            }

            if (valid)
            {
                {
                    juce::ScopedLock sl (lock);
                    lastValidationTime = juce::Time::currentTimeMillis();
                    statusMessage = "Licensed — RONE Full Bundle";
                }
                licensed.store (true);
                saveLicenseFile();
            }
            else
            {
                // License revoked, expired, or invalid
                {
                    juce::ScopedLock sl (lock);
                    statusMessage = "License is no longer valid.";
                }
                licensed.store (false);
                // Only this serial goes. The registry mirror follows from what
                // is left in the file, so an account pass on the same machine
                // keeps its "active".
                clearLicenseFile();

                if (onLicenseStateChanged)
                    onLicenseStateChanged (false);
            }

            if (callback) callback (valid);
        });
    });
}

// ============================================================================
// Deactivate the current license
// ============================================================================

void LicenseHandler::deactivateLicense (std::function<void (bool, juce::String)> callback)
{
    juce::String key;
    juce::String instId;

    {
        juce::ScopedLock sl (lock);
        key    = licenseKey;
        instId = instanceId;
    }

    if (key.isEmpty() || instId.isEmpty())
    {
        callback (false, "No active license to deactivate.");
        return;
    }

    juce::Thread::launch ([this, key, instId, callback]()
    {
        juce::StringPairArray params;
        params.set ("license_key", key);
        params.set ("instance_id", instId);

        auto json = postToLemonSqueezy ("deactivate", params);

        juce::MessageManager::callAsync ([this, json, callback]()
        {
            if (json.isVoid())
            {
                callback (false, "Network error. Check your internet connection.");
                return;
            }

            bool deactivated = (bool) json.getProperty ("deactivated", false);

            if (deactivated)
            {
                {
                    juce::ScopedLock sl (lock);
                    licenseKey.clear();
                    instanceId.clear();
                    customerName.clear();
                    lastValidationTime = 0;
                    statusMessage = "License deactivated.";
                }
                licensed.store (false);
                clearLicenseFile();
                stopTimer();

                if (onLicenseStateChanged)
                    onLicenseStateChanged (false);

                callback (true, "License deactivated successfully.");
            }
            else
            {
                auto error = json.getProperty ("error", "Deactivation failed.").toString();
                callback (false, error);
            }
        });
    });
}

// ============================================================================
// Periodic re-validation (Timer callback)
// ============================================================================

void LicenseHandler::timerCallback()
{
    if (licensed.load())
        validateLicenseAsync();
}
