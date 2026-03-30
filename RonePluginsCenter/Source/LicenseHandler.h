#pragma once

#include <JuceHeader.h>

#if JUCE_WINDOWS
 #include <windows.h>
#endif

// ============================================================================
// LicenseHandler — unified Lemon Squeezy bundle license for RONE Full Bundle
//
// The Center activates a single serial key and writes the license state to a
// shared XML file (~/.RonePlugins/BundleLicense.xml).  On Windows it also
// mirrors the status to HKCU\Software\RONE\License\BundleStatus so that
// plugins can do a quick registry read.
// ============================================================================
class LicenseHandler : public juce::Timer
{
public:
    LicenseHandler();
    ~LicenseHandler() override;

    // --- State queries (thread-safe) ----------------------------------------
    bool isLicensed() const { return licensed.load(); }
    juce::String getLicenseKey() const;
    juce::String getInstanceId() const;
    juce::String getCustomerName() const;
    juce::String getStatusMessage() const;

    // --- Actions (async — callbacks on message thread) ----------------------
    void activateLicense (const juce::String& key,
                          std::function<void (bool success, juce::String message)> callback);
    void deactivateLicense (std::function<void (bool success, juce::String message)> callback);
    void validateLicenseAsync (std::function<void (bool valid)> callback = nullptr);

    // Called once at startup to load persisted state + start validation timer.
    void initialize();

    // Callback for the UI to react to license state changes.
    std::function<void (bool licensed)> onLicenseStateChanged;

private:
    // =========================================================================
    // Lemon Squeezy constants — set these from the LS dashboard
    // =========================================================================
    static constexpr int EXPECTED_STORE_ID   = 285646;
    static constexpr int EXPECTED_PRODUCT_ID = 827526;

    // Re-validation every 24 h; 7-day offline grace period
    static constexpr int64_t VALIDATION_INTERVAL_MS = 24LL * 60 * 60 * 1000;
    static constexpr int64_t OFFLINE_GRACE_MS       = 7LL * 24 * 60 * 60 * 1000;

    // --- License state ------------------------------------------------------
    std::atomic<bool> licensed { false };
    juce::String licenseKey;
    juce::String instanceId;
    juce::String customerName;
    juce::String statusMessage;
    int64_t lastValidationTime = 0;

    juce::CriticalSection lock;

    // --- Persistence --------------------------------------------------------
    juce::File getLicenseFile() const;
    void saveLicenseFile();
    bool loadLicenseFile();
    void clearLicenseFile();

    // Mirror status to Windows Registry (no-op on other platforms)
    void updateRegistryStatus (const juce::String& status);

    // --- HTTP helper --------------------------------------------------------
    juce::var postToLemonSqueezy (const juce::String& endpoint,
                                  const juce::StringPairArray& params);

    // --- Machine identity ---------------------------------------------------
    juce::String getMachineInstanceName() const;

    // --- Periodic re-validation (Timer) ------------------------------------
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LicenseHandler)
};
