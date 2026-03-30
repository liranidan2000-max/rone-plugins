#include "VersionChecker.h"

#if JUCE_WINDOWS
 #include <windows.h>
#endif

// ============================================================================
// Version persistence helpers
// ============================================================================

#if JUCE_MAC || JUCE_LINUX
// On macOS/Linux we store installed versions in a shared XML file
// at ~/Library/Application Support/RonePlugins/versions.xml
static juce::File getVersionsXmlFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("RonePlugins")
               .getChildFile ("versions.xml");
}
#endif

// ============================================================================
// Registry / XML helpers — read & write installed version per plugin
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

    return {};

#else
    // macOS / Linux: read from shared XML file
    auto file = getVersionsXmlFile();
    if (! file.existsAsFile())
        return {};

    auto xml = juce::parseXML (file);
    if (xml == nullptr)
        return {};

    for (auto* child : xml->getChildIterator())
    {
        if (child->getStringAttribute ("id") == registryKey)
            return child->getStringAttribute ("version");
    }

    return {};
#endif
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
    // macOS / Linux: write to shared XML file
    auto file = getVersionsXmlFile();
    file.getParentDirectory().createDirectory();

    std::unique_ptr<juce::XmlElement> xml;
    if (file.existsAsFile())
        xml = juce::parseXML (file);
    if (xml == nullptr)
        xml = std::make_unique<juce::XmlElement> ("RoneVersions");

    // Find or create the plugin entry
    bool found = false;
    for (auto* child : xml->getChildIterator())
    {
        if (child->getStringAttribute ("id") == registryKey)
        {
            child->setAttribute ("version", version);
            found = true;
            break;
        }
    }

    if (! found)
    {
        auto* entry = xml->createNewChildElement ("Plugin");
        entry->setAttribute ("id", registryKey);
        entry->setAttribute ("version", version);
    }

    xml->writeTo (file, {});
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

    int maxLen = juce::jmax (a.size(), b.size());

    for (int i = 0; i < maxLen; ++i)
    {
        int ai = i < a.size() ? a[i] : 0;
        int bi = i < b.size() ? b[i] : 0;

        if (bi > ai) return true;
        if (bi < ai) return false;
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
    return juce::File ("/Library/Audio/Plug-Ins/VST3");
#else
    return juce::File ("~/.vst3/RONE");
#endif
}

juce::File VersionChecker::getAUInstallDir()
{
#if JUCE_MAC
    return juce::File ("/Library/Audio/Plug-Ins/Components");
#else
    return juce::File(); // AU is macOS-only
#endif
}

bool VersionChecker::isStandaloneInstalled (const juce::String& exeName)
{
    if (exeName.isEmpty()) return false;

#if JUCE_MAC
    // On Mac, standalone apps are .app bundles — check multiple locations
    auto appName = exeName.replace (".exe", "") + ".app";

    // 1. /Applications/  (direct install)
    if (juce::File ("/Applications").getChildFile (appName).exists())
        return true;

    // 2. /Applications/RONE Plugins/  (standardized subfolder)
    if (juce::File ("/Applications/RONE Plugins").getChildFile (appName).exists())
        return true;

    // 3. ~/Library/Application Support/RONE Plugins/
    if (getStandaloneInstallDir().getChildFile (appName).exists())
        return true;

    return false;
#else
    return getStandaloneInstallDir().getChildFile (exeName).existsAsFile();
#endif
}

bool VersionChecker::isVst3Installed (const juce::String& bundleName)
{
    if (bundleName.isEmpty()) return false;
    auto vst3 = getVst3InstallDir().getChildFile (bundleName);
    return vst3.exists(); // could be file or directory (bundle)
}

bool VersionChecker::isAUInstalled (const juce::String& bundleName)
{
    if (bundleName.isEmpty()) return false;

#if JUCE_MAC
    // Check system-wide and user AU directories
    if (getAUInstallDir().getChildFile (bundleName).exists())
        return true;

    auto userAU = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                      .getChildFile ("Library/Audio/Plug-Ins/Components")
                      .getChildFile (bundleName);
    return userAU.exists();
#else
    juce::ignoreUnused (bundleName);
    return false;
#endif
}
