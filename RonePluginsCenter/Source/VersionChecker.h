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
    Downloading,
    Installing,
    Error
};

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
    juce::String sha256;
    juce::String standaloneExe;
    juce::String vst3Bundle;
    juce::String registryKey;
    juce::String type;                // "plugin" or "standalone"
    juce::StringArray formats;
    PluginStatus status = PluginStatus::NotInstalled;
    double       downloadProgress = 0.0;
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

    // Determine the PluginStatus from the two version strings.
    static PluginStatus determineStatus (const juce::String& installed,
                                          const juce::String& remote);

    // Check whether the standalone exe or VST3 bundle exists on disk.
    static bool isStandaloneInstalled (const juce::String& exeName);
    static bool isVst3Installed       (const juce::String& bundleName);

    // Get the install directory for standalones.
    static juce::File getStandaloneInstallDir();

    // Get the VST3 install directory.
    static juce::File getVst3InstallDir();

private:
    // Split "1.2.3" into {1, 2, 3}.
    static juce::Array<int> parseVersion (const juce::String& v);
};
