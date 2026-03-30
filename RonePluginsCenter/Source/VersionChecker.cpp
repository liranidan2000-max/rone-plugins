#include "VersionChecker.h"

#if JUCE_WINDOWS
 #include <windows.h>
#endif

// ============================================================================
// Registry helpers (Windows only — stubs on other platforms)
// ============================================================================

juce::String VersionChecker::getInstalledVersion (const juce::String& registryKey)
{
#if JUCE_WINDOWS
    juce::String path = RONE_REGISTRY_PATH;
    path += "\\" + registryKey;

    HKEY hKey = nullptr;
    auto pathWide = path.toWideCharPointer();

    if (RegOpenKeyExW (HKEY_CURRENT_USER, pathWide, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        wchar_t buf[256] = {};
        DWORD bufSize = sizeof (buf);
        DWORD type    = REG_SZ;

        if (RegQueryValueExW (hKey, L"InstalledVersion", nullptr, &type,
                              reinterpret_cast<LPBYTE> (buf), &bufSize) == ERROR_SUCCESS)
        {
            RegCloseKey (hKey);
            return juce::String (buf);
        }
        RegCloseKey (hKey);
    }
#else
    juce::ignoreUnused (registryKey);
#endif

    return {};
}

void VersionChecker::setInstalledVersion (const juce::String& registryKey,
                                           const juce::String& version)
{
#if JUCE_WINDOWS
    juce::String path = RONE_REGISTRY_PATH;
    path += "\\" + registryKey;

    HKEY hKey = nullptr;
    DWORD disposition = 0;
    auto pathWide = path.toWideCharPointer();

    if (RegCreateKeyExW (HKEY_CURRENT_USER, pathWide, 0, nullptr,
                         REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr,
                         &hKey, &disposition) == ERROR_SUCCESS)
    {
        auto versionWide = version.toWideCharPointer();
        RegSetValueExW (hKey, L"InstalledVersion", 0, REG_SZ,
                        reinterpret_cast<const BYTE*> (versionWide),
                        (DWORD) ((version.length() + 1) * sizeof (wchar_t)));
        RegCloseKey (hKey);
    }
#else
    juce::ignoreUnused (registryKey, version);
#endif
}

// ============================================================================
// Version comparison
// ============================================================================

juce::Array<int> VersionChecker::parseVersion (const juce::String& v)
{
    juce::Array<int> parts;
    auto tokens = juce::StringArray::fromTokens (v, ".", {});

    for (auto& t : tokens)
        parts.add (t.getIntValue());

    // Ensure at least 3 components
    while (parts.size() < 3)
        parts.add (0);

    return parts;
}

bool VersionChecker::isNewerVersion (const juce::String& installed,
                                      const juce::String& remote)
{
    if (installed.isEmpty())
        return true; // not installed → remote is "newer"

    auto a = parseVersion (installed);
    auto b = parseVersion (remote);

    for (int i = 0; i < juce::jmin (a.size(), b.size()); ++i)
    {
        if (b[i] > a[i]) return true;
        if (b[i] < a[i]) return false;
    }
    return false;
}

PluginStatus VersionChecker::determineStatus (const juce::String& installed,
                                               const juce::String& remote)
{
    if (installed.isEmpty())
        return PluginStatus::NotInstalled;

    if (isNewerVersion (installed, remote))
        return PluginStatus::UpdateAvailable;

    return PluginStatus::UpToDate;
}

// ============================================================================
// Filesystem checks — scan well-known install paths
// ============================================================================

juce::File VersionChecker::getStandaloneInstallDir()
{
#if JUCE_WINDOWS
    auto progFiles = juce::File::getSpecialLocation (
        juce::File::globalApplicationsDirectory);
    return progFiles.getChildFile (RONE_INSTALL_DIR);
#else
    return juce::File::getSpecialLocation (
        juce::File::userApplicationDataDirectory).getChildFile ("RONE Plugins");
#endif
}

juce::File VersionChecker::getVst3InstallDir()
{
#if JUCE_WINDOWS
    // C:\Program Files\Common Files\VST3\RONE
    auto common = juce::File::getSpecialLocation (
        juce::File::globalApplicationsDirectoryX86)  // gives Common Files parent
        .getParentDirectory()
        .getChildFile ("Common Files")
        .getChildFile ("VST3")
        .getChildFile (RONE_VST3_SUBDIR);
    return common;
#elif JUCE_MAC
    return juce::File ("/Library/Audio/Plug-Ins/VST3/RONE");
#else
    return juce::File ("~/.vst3/RONE");
#endif
}

bool VersionChecker::isStandaloneInstalled (const juce::String& exeName)
{
    if (exeName.isEmpty()) return false;
    return getStandaloneInstallDir().getChildFile (exeName).existsAsFile();
}

bool VersionChecker::isVst3Installed (const juce::String& bundleName)
{
    if (bundleName.isEmpty()) return false;
    auto vst3 = getVst3InstallDir().getChildFile (bundleName);
    return vst3.exists(); // could be file or directory (bundle)
}
