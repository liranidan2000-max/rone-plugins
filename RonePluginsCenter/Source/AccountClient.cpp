#include "AccountClient.h"
#include <cstring>

#if JUCE_WINDOWS
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
#endif

#ifndef RONE_API_BASE
 #define RONE_API_BASE "https://roneaudio.com/api/v1"
#endif

// ============================================================================
// Construction
// ============================================================================

AccountClient::AccountClient() = default;

AccountClient::~AccountClient()
{
    stopTimer();
}

AccountClient::State AccountClient::getState() const
{
    const juce::ScopedLock sl (lock);
    return state;
}

bool AccountClient::hasStoredToken() const
{
    const juce::ScopedLock sl (lock);
    return token.isNotEmpty();
}

// ============================================================================
// Files
// ============================================================================

juce::File AccountClient::getAccountFile() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("RonePlugins")
               .getChildFile ("Account.xml");
}

juce::File AccountClient::getLicenseFile() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("RonePlugins")
               .getChildFile ("BundleLicense.xml");
}

void AccountClient::saveAccountFile()
{
    const juce::ScopedLock sl (lock);

    juce::XmlElement xml ("RoneAccount");
    xml.setAttribute ("token",              token);
    xml.setAttribute ("email",              state.email);
    xml.setAttribute ("name",               state.name);
    xml.setAttribute ("plan",               state.plan);
    xml.setAttribute ("licensed",           state.licensed);
    xml.setAttribute ("expiresAt",          juce::String (state.expiresAt));
    xml.setAttribute ("renewsAt",           juce::String (state.renewsAt));
    xml.setAttribute ("deviceLimit",        state.deviceLimit);
    xml.setAttribute ("lastValidationTime", juce::String (lastValidationTime));

    auto file = getAccountFile();
    file.getParentDirectory().createDirectory();
    xml.writeTo (file, {});
}

bool AccountClient::loadAccountFile()
{
    auto file = getAccountFile();
    if (! file.existsAsFile())
        return false;

    auto xml = juce::parseXML (file);
    if (xml == nullptr || xml->getTagName() != "RoneAccount")
        return false;

    const juce::ScopedLock sl (lock);
    token              = xml->getStringAttribute ("token");
    state.email        = xml->getStringAttribute ("email");
    state.name         = xml->getStringAttribute ("name");
    state.plan         = xml->getStringAttribute ("plan", "none");
    state.licensed     = xml->getBoolAttribute   ("licensed", false);
    state.expiresAt    = xml->getStringAttribute ("expiresAt", "0").getLargeIntValue();
    state.renewsAt     = xml->getStringAttribute ("renewsAt",  "0").getLargeIntValue();
    state.deviceLimit  = xml->getIntAttribute    ("deviceLimit", 2);
    lastValidationTime = xml->getStringAttribute ("lastValidationTime", "0").getLargeIntValue();
    state.signedIn     = token.isNotEmpty();

    return state.signedIn;
}

void AccountClient::clearAccountFile()
{
    getAccountFile().deleteFile();

    const juce::ScopedLock sl (lock);
    token = {};
    state = State{};
}

// ============================================================================
// BundleLicense.xml — the contract with every plugin
// ============================================================================

void AccountClient::writeBundleLicense (bool licensed)
{
    auto file = getLicenseFile();

    if (! licensed)
    {
        // Removing the file is exactly what BundleLicenseChecker treats as
        // "not licensed" — cleaner than leaving a stale licensed="0" behind.
        file.deleteFile();
       #if JUCE_WINDOWS
        HKEY hKey = nullptr;
        if (RegOpenKeyExW (HKEY_CURRENT_USER, L"Software\\RONE\\License", 0, KEY_SET_VALUE, &hKey)
            == ERROR_SUCCESS)
        {
            RegDeleteValueW (hKey, L"BundleStatus");
            RegCloseKey (hKey);
        }
       #endif
        return;
    }

    juce::String email, name;
    {
        const juce::ScopedLock sl (lock);
        email = state.email;
        name  = state.name;
    }

    juce::XmlElement xml ("RoneBundleLicense");
    xml.setAttribute ("licensed",           true);
    xml.setAttribute ("customerName",       name.isNotEmpty() ? name : email);
    xml.setAttribute ("lastValidationTime", juce::String (juce::Time::currentTimeMillis()));
    // The account path has no serial; these keep the file shape identical for
    // any older reader that expects the attributes to exist.
    xml.setAttribute ("licenseKey",         "account:" + email);
    xml.setAttribute ("instanceId",         getMachineId());

    file.getParentDirectory().createDirectory();
    xml.writeTo (file, {});

   #if JUCE_WINDOWS
    HKEY hKey = nullptr;
    DWORD disposition = 0;
    if (RegCreateKeyExW (HKEY_CURRENT_USER, L"Software\\RONE\\License", 0, nullptr,
                         REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, &disposition)
        == ERROR_SUCCESS)
    {
        const wchar_t* value = L"active";
        RegSetValueExW (hKey, L"BundleStatus", 0, REG_SZ,
                        reinterpret_cast<const BYTE*> (value),
                        (DWORD) ((wcslen (value) + 1) * sizeof (wchar_t)));
        RegCloseKey (hKey);
    }
   #endif
}

// ============================================================================
// Machine identity
// ============================================================================

juce::String AccountClient::getMachineId()
{
    auto raw = juce::SystemStats::getUniqueDeviceID();

    if (raw.isEmpty())
    {
        // Fall back to a random id kept beside the account file, so the same
        // computer keeps its device slot across reinstalls.
        auto idFile = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                          .getChildFile ("RonePlugins").getChildFile ("machine-id.txt");
        raw = idFile.existsAsFile() ? idFile.loadFileAsString().trim() : juce::String();

        if (raw.isEmpty())
        {
            raw = juce::Uuid().toDashedString();
            idFile.getParentDirectory().createDirectory();
            idFile.replaceWithText (raw);
        }
    }

    // Hashed: the server never needs the real hardware id, only something
    // stable and unique.
    return juce::SHA256 (("rone-device:" + raw).toUTF8()).toHexString();
}

juce::String AccountClient::getMachineName()
{
    auto name = juce::SystemStats::getComputerName();
    if (name.isEmpty()) name = "Unknown computer";
    return name + " (" + juce::SystemStats::getOperatingSystemName() + ")";
}

// ============================================================================
// HTTP
// ============================================================================

juce::var AccountClient::post (const juce::String& path, const juce::var& body,
                               const juce::String& bearer, int& statusCodeOut)
{
    statusCodeOut = 0;

    juce::URL url (juce::String (RONE_API_BASE) + path);
    url = url.withPOSTData (juce::JSON::toString (body));

    juce::String headers = "Content-Type: application/json\r\nAccept: application/json";
    if (bearer.isNotEmpty())
        headers += "\r\nAuthorization: Bearer " + bearer;

    juce::StringPairArray responseHeaders;
    auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                       .withConnectionTimeoutMs (15000)
                       .withExtraHeaders (headers)
                       .withResponseHeaders (&responseHeaders)
                       .withStatusCode (&statusCodeOut);

    auto stream = url.createInputStream (options);
    if (stream == nullptr)
        return {};                                  // offline / unreachable

    auto parsed = juce::JSON::parse (stream->readEntireStreamAsString());
    return parsed.isObject() ? parsed : juce::var();
}

// ============================================================================
// State from a server response
// ============================================================================

void AccountClient::applyServerState (const juce::var& response)
{
    auto user = response.getProperty ("user", juce::var());
    auto ent  = response.getProperty ("entitlements", juce::var());

    const juce::ScopedLock sl (lock);
    state.signedIn    = true;
    state.email       = user.getProperty ("email", state.email).toString();
    state.name        = user.getProperty ("name", "").toString();
    state.licensed    = (bool) ent.getProperty ("licensed", false);
    state.plan        = ent.getProperty ("plan", "none").toString();
    state.expiresAt   = (juce::int64) (double) ent.getProperty ("expiresAt", 0.0);
    state.renewsAt    = (juce::int64) (double) ent.getProperty ("renewsAt", 0.0);
    state.deviceLimit = (int) ent.getProperty ("deviceLimit", 2);
    lastValidationTime = juce::Time::currentTimeMillis();
}

void AccountClient::notifyChanged()
{
    if (onStateChanged)
        juce::MessageManager::callAsync ([cb = onStateChanged] { cb(); });
}

// ============================================================================
// Sign in / out
// ============================================================================

void AccountClient::signIn (const juce::String& email, const juce::String& password, Callback cb)
{
    if (busy.exchange (true))
    {
        if (cb) cb (false, "A sign-in is already in progress");
        return;
    }

    juce::Thread::launch ([this, email, password, cb]
    {
        auto* payload = new juce::DynamicObject();
        payload->setProperty ("email",       email.trim());
        payload->setProperty ("password",    password);
        payload->setProperty ("machineId",   getMachineId());
        payload->setProperty ("machineName", getMachineName());
        payload->setProperty ("platform",    juce::SystemStats::getOperatingSystemName());

        int status = 0;
        auto response = post ("/app/login", juce::var (payload), {}, status);

        bool success = false;
        juce::String message;

        if (! response.isObject())
        {
            message = "Cannot reach roneaudio.com. Check your internet connection.";
        }
        else if ((bool) response.getProperty ("ok", false))
        {
            {
                const juce::ScopedLock sl (lock);
                token = response.getProperty ("token", "").toString();
            }
            applyServerState (response);
            saveAccountFile();

            const bool licensed = getState().licensed;
            writeBundleLicense (licensed);

            success = true;
            message = licensed ? "Signed in — all plugins unlocked"
                               : "Signed in, but this account has no active pass";
            startTimer (VALIDATION_INTERVAL_MS);
        }
        else
        {
            // The server's own wording is the useful one here: wrong password,
            // no pass on the account, or the device limit with what to do next.
            message = response.getProperty ("message", "Sign-in failed").toString();
        }

        {
            const juce::ScopedLock sl (lock);
            state.message = message;
        }

        busy = false;
        notifyChanged();

        if (cb)
            juce::MessageManager::callAsync ([cb, success, message] { cb (success, message); });
    });
}

// ============================================================================
// Google sign-in: browser hand-off
//
// The Center cannot show Google's page itself (Google blocks sign-in inside
// embedded views), so it listens on a random localhost port, opens the system
// browser on roneaudio.com/api/v1/app/google with that port + a nonce, and
// the website - after Google confirmed the e-mail - sends the browser to
// http://127.0.0.1:<port>/callback?code=...&nonce=... . The code is single
// use and 5 minutes old at most; exchanging it returns the same device token
// a password sign-in returns, so everything after this point is shared.
// ============================================================================

static const char* const kBrowserReply =
    "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\n\r\n"
    "<!doctype html><html><head><meta charset=\"utf-8\"><title>RONE Plugins Center</title></head>"
    "<body style=\"background:#14161A;color:#E8EAED;font-family:Segoe UI,Arial,sans-serif;text-align:center;padding:60px\">"
    "<h2 style=\"font-weight:800\">Signed in to the RONE Plugins Center</h2>"
    "<p style=\"color:#9AA0A8\">You can close this tab and go back to the Center.</p></body></html>";

void AccountClient::cancelGoogleSignIn()
{
    googleCancel = true;
}

void AccountClient::signInWithGoogle (Callback cb)
{
    if (busy.exchange (true))
    {
        if (cb) cb (false, "A sign-in is already in progress");
        return;
    }
    googleCancel = false;

    juce::Thread::launch ([this, cb]
    {
        auto finish = [this, cb] (bool success, juce::String message)
        {
            {
                const juce::ScopedLock sl (lock);
                state.message = message;
            }
            busy = false;
            notifyChanged();
            if (cb)
                juce::MessageManager::callAsync ([cb, success, message] { cb (success, message); });
        };

        // 1. A listener on localhost, on whatever high port is free.
        juce::StreamingSocket server;
        int port = 0;
        juce::Random rng;
        for (int attempt = 0; attempt < 25 && port == 0; ++attempt)
        {
            const int candidate = 49152 + rng.nextInt (16000);
            if (server.createListener (candidate, "127.0.0.1"))
                port = candidate;
        }
        if (port == 0)
        {
            finish (false, "Could not open a local port for the browser hand-off.");
            return;
        }

        // 2. Open the browser on roneaudio.com with our nonce + machine details.
        const juce::String nonce = juce::SHA256 (juce::Uuid().toDashedString().toUTF8()).toHexString().substring (0, 32);
        juce::URL start (juce::String (RONE_API_BASE) + "/app/google");
        start = start.withParameter ("port",        juce::String (port))
                     .withParameter ("nonce",       nonce)
                     .withParameter ("machineId",   getMachineId())
                     .withParameter ("machineName", getMachineName())
                     .withParameter ("platform",    juce::SystemStats::getOperatingSystemName());
        juce::MessageManager::callAsync ([start] { start.launchInDefaultBrowser(); });

        // 3. Wait (up to four minutes) for the browser to come back with the code.
        juce::String code;
        const auto deadline = juce::Time::currentTimeMillis() + 4 * 60 * 1000;

        while (code.isEmpty() && ! googleCancel && juce::Time::currentTimeMillis() < deadline)
        {
            if (server.waitUntilReady (true, 500) != 1)
                continue;

            std::unique_ptr<juce::StreamingSocket> conn (server.waitForNextConnection());
            if (conn == nullptr)
                continue;

            char buf[4096] = {};
            int got = 0;
            if (conn->waitUntilReady (true, 3000) == 1)
                got = conn->read (buf, (int) sizeof (buf) - 1, false);

            const juce::String request   = juce::String::fromUTF8 (buf, juce::jmax (0, got));
            const juce::String firstLine = request.upToFirstOccurrenceOf ("\r\n", false, false);
            const juce::String path      = firstLine.fromFirstOccurrenceOf ("GET ", false, false)
                                                    .upToFirstOccurrenceOf (" ", false, false);

            if (path.startsWith ("/callback?"))
            {
                juce::URL cbUrl ("http://127.0.0.1" + path);
                auto names  = cbUrl.getParameterNames();
                auto values = cbUrl.getParameterValues();
                juce::String gotCode, gotNonce;
                for (int i = 0; i < names.size(); ++i)
                {
                    if (names[i] == "code")  gotCode  = values[i];
                    if (names[i] == "nonce") gotNonce = values[i];
                }
                conn->write (kBrowserReply, (int) std::strlen (kBrowserReply));
                if (gotNonce == nonce && gotCode.isNotEmpty())
                    code = gotCode;
            }
            else
            {
                static const char* const notFound = "HTTP/1.1 404 Not Found\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
                conn->write (notFound, (int) std::strlen (notFound));
            }
        }
        server.close();

        if (code.isEmpty())
        {
            finish (false, googleCancel ? "Google sign-in cancelled."
                                        : "The browser did not come back in time. Please try again.");
            return;
        }

        // 4. Swap the code for a device token - the same response shape as /app/login.
        auto* payload = new juce::DynamicObject();
        payload->setProperty ("code",        code);
        payload->setProperty ("nonce",       nonce);
        payload->setProperty ("machineId",   getMachineId());
        payload->setProperty ("machineName", getMachineName());
        payload->setProperty ("platform",    juce::SystemStats::getOperatingSystemName());

        int status = 0;
        auto response = post ("/app/google/exchange", juce::var (payload), {}, status);

        if (! response.isObject())
        {
            finish (false, "Cannot reach roneaudio.com. Check your internet connection.");
            return;
        }
        if (! (bool) response.getProperty ("ok", false))
        {
            finish (false, response.getProperty ("message", "Sign-in failed").toString());
            return;
        }

        {
            const juce::ScopedLock sl (lock);
            token = response.getProperty ("token", "").toString();
        }
        applyServerState (response);
        saveAccountFile();

        const bool licensed = getState().licensed;
        writeBundleLicense (licensed);
        startTimer (VALIDATION_INTERVAL_MS);

        finish (true, licensed ? "Signed in with Google - all plugins unlocked"
                               : "Signed in, but this account has no active pass");
    });
}

void AccountClient::signOut (Callback cb)
{
    juce::String current;
    {
        const juce::ScopedLock sl (lock);
        current = token;
    }

    // Local state is cleared immediately: signing out must work offline too.
    stopTimer();
    clearAccountFile();
    writeBundleLicense (false);
    notifyChanged();

    if (cb) cb (true, "Signed out");

    if (current.isNotEmpty())
    {
        juce::Thread::launch ([this, current]
        {
            int status = 0;
            post ("/app/logout", juce::var (new juce::DynamicObject()), current, status);
        });
    }
}

// ============================================================================
// Validation
// ============================================================================

void AccountClient::validateAsync (std::function<void (bool)> done)
{
    juce::String current;
    {
        const juce::ScopedLock sl (lock);
        current = token;
    }

    if (current.isEmpty())
    {
        if (done) done (false);
        return;
    }

    juce::Thread::launch ([this, current, done]
    {
        int status = 0;
        auto response = post ("/app/refresh", juce::var (new juce::DynamicObject()), current, status);

        bool licensed = false;

        if (response.isObject() && (bool) response.getProperty ("ok", false))
        {
            applyServerState (response);
            licensed = getState().licensed;
            saveAccountFile();
            writeBundleLicense (licensed);
        }
        else if (status == 401)
        {
            // The server positively says this device is gone (released from the
            // account page, or the account was deleted). Only THIS answer
            // clears the token — a network failure must not.
            clearAccountFile();
            writeBundleLicense (false);
            {
                const juce::ScopedLock sl (lock);
                state.message = "This computer was signed out. Please sign in again.";
            }
        }
        else
        {
            // Offline: keep working until the grace period runs out.
            const auto elapsed = juce::Time::currentTimeMillis() - lastValidationTime;
            licensed = getState().licensed && elapsed < OFFLINE_GRACE_MS;

            if (! licensed && getState().signedIn)
            {
                writeBundleLicense (false);
                const juce::ScopedLock sl (lock);
                state.licensed = false;
                state.message  = "Could not verify your pass. Connect to the internet.";
            }
        }

        notifyChanged();

        if (done)
            juce::MessageManager::callAsync ([done, licensed] { done (licensed); });
    });
}

void AccountClient::timerCallback()
{
    validateAsync();
}

// ============================================================================
// Startup
// ============================================================================

void AccountClient::initialize()
{
    if (! loadAccountFile())
        return;

    const auto elapsed = juce::Time::currentTimeMillis() - lastValidationTime;

    if (elapsed < OFFLINE_GRACE_MS)
    {
        // Trust the cached verdict so the UI is correct instantly, then confirm
        // with the server in the background.
        writeBundleLicense (getState().licensed);
    }
    else
    {
        const juce::ScopedLock sl (lock);
        state.licensed = false;
        state.message  = "Please connect to the internet to verify your pass.";
    }

    validateAsync();
    startTimer (VALIDATION_INTERVAL_MS);
}
