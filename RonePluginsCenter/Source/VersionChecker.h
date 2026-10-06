#pragma once
#include <JuceHeader.h>

// ============================================================================
// Plugin status — determined by comparing remote version vs local registry
// ============================================================================
enum class PluginStatus
{
    NotInstalled,
    UpToDate,
    UpdateAvailable,
    Queued,           // waiting in the download queue behind another plugin
    Downloading,
    ReadyToInstall,   // downloaded and verified; installs with the rest of the batch (one permission prompt)
    Installing,
    WaitingForHost,   // downloaded; a DAW or the standalone still holds the files (Windows)
    Uninstalling,     // the plugin's own uninstaller is running (the card's UNINSTALL)
    Error
};

// Downloading, installing, uninstalling... anything a refresh must not reset and
// a second click must not start again.
inline bool isBusyStatus (PluginStatus s) noexcept
{
    return s == PluginStatus::Queued || s == PluginStatus::Downloading || s == PluginStatus::ReadyToInstall
        || s == PluginStatus::Installing || s == PluginStatus::WaitingForHost || s == PluginStatus::Uninstalling;
}

// ============================================================================
// Lightweight struct holding everything the UI needs per plugin
// ============================================================================
struct PluginInfo
{
    juce::String id;
    juce::String name;
    juce::String description;
    juce::String remoteVersion;
    juce::String installedVersion;    // empty if not installed
    juce::String whatsNew;
    juce::String downloadUrl;
    juce::String downloadUrlMac;      // .pkg URL for macOS
    juce::String sha256;
    juce::String sha256Mac;           // hash of the macOS .pkg (may be empty)
    juce::String standaloneExe;
    juce::String vst3Bundle;
    juce::String auBundle;            // e.g. "RONE Reverse Reverb.component"
    juce::String manualPdf;           // "RONE Stutter - User Manual.pdf" (ships in the install folder)
    juce::String videoUrl;            // the YouTube guide; empty until the plugin has one
    juce::String registryKey;
    juce::String type;                // "plugin" or "standalone"
    juce::StringArray formats;
    PluginStatus status = PluginStatus::NotInstalled;
    double       downloadProgress = 0.0;
    juce::String waitingFor;          // WaitingForHost: the program to close ("FL Studio"); empty = its own window

    // Individual LIFETIME pricing, carried straight from the manifest so the
    // Center never keeps a second price list. Held as vars because only the
    // plugins sold on their own have them, and absent has to stay absent all
    // the way to the UI — a defaulted 0 would read as "free".
    juce::var    price;               // regular USD
    juce::var    launchPrice;         // what is actually charged during the sale
    juce::String storeUrl;            // product page to send a locked card to

    // Center 2.0 catalog fields. All optional: an older manifest leaves them
    // empty and the page falls back to what it can say without them.
    bool              free = false;   // unlocked by any signed-in RONE account
    juce::StringArray categories;     // "transitions", "space", "rhythm", "vocal", "mix"
    juce::StringArray tags;           // what a producer types: "riser", "vocal chop"
    juce::String      accent;         // the plugin's own neon, "#2BD9FF"
    juce::String      released;       // "2026-10-04": a plugin younger than a month reads as NEW
    juce::String      previewDry;     // https://roneaudio.com/media/previews/<id>-dry.mp3
    juce::String      previewWet;
    juce::String      innoAppId;      // "{GUID}": the installer's AppId, so a new plugin needs no Center release
    juce::int64       sizeBytes = 0;  // this platform's installer
    juce::var         i18n;           // {"pt": {"description": ...}, "es": {...}}: the page picks the user's language
};

// ============================================================================
// VersionChecker — reads/writes Windows Registry, compares version strings
// ============================================================================
class VersionChecker
{
public:
    // Read the installed version from the Windows Registry.
    // Returns empty string if not installed.
    static juce::String getInstalledVersion (const juce::String& registryKey);

    // Write (or create) the installed version after a successful install.
    static void setInstalledVersion (const juce::String& registryKey,
                                     const juce::String& version);

    // True if remoteVersion is newer than installedVersion.
    // Uses simple semantic-version comparison (major.minor.patch).
    static bool isNewerVersion (const juce::String& installed,
                                const juce::String& remote);

    // installedVersion + status from what is on this machine now: the installer's
    // record first, then the files themselves (a manual install shows as "?").
    static void refreshInstallState (PluginInfo& info);

    // Windows: the command line the plugin's Inno uninstaller registered
    // (UninstallString); empty when there is none, and on macOS.
    static juce::String getUninstallCommand (const juce::String& registryKey);

    // Forget the version the Center stamped for this plugin (after an uninstall).
    static void clearInstalledVersion (const juce::String& registryKey);

    // The installer AppId the manifest names for a plugin (Windows). The built-in
    // table below stays the fallback for manifests written before the field.
    static void registerInnoAppId (const juce::String& registryKey, const juce::String& appId);

    // macOS: the version inside an installed bundle's Info.plist ("1.1.10"), for
    // a plugin installed from the website's .pkg - the Center never stamped it.
    static juce::String readBundleVersion (const juce::File& bundle);

    // Determine the PluginStatus from the two version strings.
    static PluginStatus determineStatus (const juce::String& installed,
                                          const juce::String& remote);

    // Check whether the standalone exe or VST3/AU bundle exists on disk.
    static bool isStandaloneInstalled (const juce::String& exeName);
    static bool isVst3Installed       (const juce::String& bundleName);
    static bool isAUInstalled         (const juce::String& bundleName);

    // Get the install directories.
    static juce::File getStandaloneInstallDir();
    static juce::File getVst3InstallDir();
    static juce::File getAUInstallDir();

private:
    // Split "1.2.3" into {1, 2, 3}.
    static juce::Array<int> parseVersion (const juce::String& v);
};
