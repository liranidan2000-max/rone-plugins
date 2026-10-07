#include "VersionChecker.h"
#include <map>

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

#if JUCE_WINDOWS
// The installers are Inno Setup, and Inno records what it actually finished
// installing under the machine's Uninstall key (DisplayVersion). That entry
// is written only when an install completes and removed only by the
// uninstaller, so it is the truth about what is on disk - unlike our own
// HKCU stamp, which a silent install that ROLLED BACK (a plugin file open in
// the DAW, exit code 5) leaves at whatever it said before. On 2026-09-14
// every stamp on Liran's machine was one to three releases behind the
// uninstall entries, and the Center offered the same Analyzer update forever.
static const std::map<juce::String, juce::String>& innoAppIds()
{
    static const std::map<juce::String, juce::String> ids {
        { "ReverseReverb",  "{B2C3D4E5-F6A7-8901-BCDE-F12345678901}" },
        { "RoneStutter",    "{D4E5F6A7-B8C9-0123-DEFA-234567890123}" },
        { "RoneStucker",    "{DEDF18E3-6C8E-4090-B461-655FE6048BA6}" },
        { "RoneThrow",      "{547A9CAA-B46F-414C-BFEE-7699EB212906}" },
        { "RoneClipper",    "{E02BE7CD-F957-43CC-81A5-950AC7AE62B0}" },   // installer/RoneClipper.iss
        { "RoneRise",       "{E4F73C6B-63EB-4D37-9E02-0C6E91F7E9CE}" },   // installer/RoneRise.iss
        { "RoneIron",       "{DAFDABED-2E84-4166-86BB-EC4810E01F1F}" },   // installer/RoneIron.iss
        { "RoneFlanger",    "{E5F6A7B8-C9D0-1234-EFAB-345678901234}" },
        { "RoneAfterspace", "{A1B2C3D4-E5F6-7890-ABCD-AFTERSPACE01}" },
        { "RONEAnalyzer",   "{E7F8A9B0-C1D2-3456-EF01-6789ABCDEF01}" },
        { "RoneStemsFixer", "{C3D4E5F6-A7B8-9012-CDEF-123456789012}" },
        { "RoneSyncVerb",   "{A1B2C3D4-E5F6-7890-ABCD-SYNCVERB0001}" },
        { "__center__",     "{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}" },
    };
    return ids;
}

static juce::String readRegString (HKEY root, const juce::String& subKey, const wchar_t* valueName, REGSAM extra = 0)
{
    HKEY hKey = nullptr;
    if (RegOpenKeyExW (root, subKey.toWideCharPointer(), 0, KEY_READ | extra, &hKey) != ERROR_SUCCESS)
        return {};

    juce::String result;
    DWORD type = REG_SZ, bufSize = 0;

    // Query required buffer size first to avoid overflow
    if (RegQueryValueExW (hKey, valueName, nullptr, &type, nullptr, &bufSize) == ERROR_SUCCESS && bufSize > 0)
    {
        std::vector<wchar_t> buf (bufSize / sizeof (wchar_t) + 1, 0);
        if (RegQueryValueExW (hKey, valueName, nullptr, &type,
                              reinterpret_cast<LPBYTE> (buf.data()), &bufSize) == ERROR_SUCCESS)
            result = juce::String (buf.data());
    }

    RegCloseKey (hKey);
    return result;
}

// AppIds the manifest carries (inno_app_id). A plugin released after this
// Center build is found through these, without a Center release of its own.
static juce::CriticalSection& manifestIdsLock() { static juce::CriticalSection l; return l; }
static std::map<juce::String, juce::String>& manifestIds() { static std::map<juce::String, juce::String> m; return m; }

static juce::String appIdFor (const juce::String& registryKey)
{
    {
        const juce::ScopedLock sl (manifestIdsLock());
        const auto it = manifestIds().find (registryKey);
        if (it != manifestIds().end())
            return it->second;
    }
    const auto& ids = innoAppIds();
    const auto it = ids.find (registryKey);
    return it == ids.end() ? juce::String() : it->second;
}

static juce::String innoUninstallValue (const juce::String& registryKey, const wchar_t* valueName)
{
    const auto appId = appIdFor (registryKey);
    if (appId.isEmpty())
        return {};

    // The installers run in 64-bit mode, so the entry lives in the native
    // view; ask for it explicitly in case the Center is ever built 32-bit.
    return readRegString (HKEY_LOCAL_MACHINE,
                          "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + appId + "_is1",
                          valueName, KEY_WOW64_64KEY);
}

static juce::String innoInstalledVersion (const juce::String& registryKey)
{
    return innoUninstallValue (registryKey, L"DisplayVersion");
}
#endif

juce::String VersionChecker::getUninstallCommand (const juce::String& registryKey)
{
#if JUCE_WINDOWS
    return innoUninstallValue (registryKey, L"UninstallString");
#else
    juce::ignoreUnused (registryKey);
    return {};
#endif
}

void VersionChecker::clearInstalledVersion (const juce::String& registryKey)
{
    if (registryKey.isEmpty())
        return;

#if JUCE_WINDOWS
    // The installer's own [Registry] entry (uninsdeletekey) is this same key.
    const auto path = juce::String (RONE_REGISTRY_PATH) + "\\" + registryKey;
    RegDeleteKeyValueW (HKEY_CURRENT_USER, path.toWideCharPointer(), L"InstalledVersion");
#else
    auto file = getVersionsXmlFile();
    if (! file.existsAsFile())
        return;

    auto xml = juce::parseXML (file);
    if (xml == nullptr)
        return;

    for (auto* child : xml->getChildIterator())
        if (child->getStringAttribute ("id") == registryKey)
        {
            xml->removeChildElement (child, true);
            xml->writeTo (file, {});
            return;
        }
#endif
}

void VersionChecker::registerInnoAppId (const juce::String& registryKey, const juce::String& appId)
{
   #if JUCE_WINDOWS
    const auto id = appId.trim();
    // "{GUID}" only: the value ends up inside a registry path.
    if (registryKey.isEmpty() || ! id.startsWithChar ('{') || ! id.endsWithChar ('}')
        || ! id.containsOnly ("{}-0123456789ABCDEFabcdefGHIJKLMNOPQRSTUVWXYZghijklmnopqrstuvwxyz"))
        return;
    const juce::ScopedLock sl (manifestIdsLock());
    manifestIds()[registryKey] = id;
   #else
    juce::ignoreUnused (registryKey, appId);
   #endif
}

juce::String VersionChecker::readBundleVersion (const juce::File& bundle)
{
   #if JUCE_MAC
    const auto plist = bundle.getChildFile ("Contents").getChildFile ("Info.plist");
    if (! plist.existsAsFile())
        return {};

    // Info.plist is XML in every bundle JUCE builds: <key>CFBundleShortVersionString</key><string>1.1.10</string>
    if (auto xml = juce::parseXML (plist))
        if (auto* dict = xml->getChildByName ("dict"))
            for (auto* e = dict->getFirstChildElement(); e != nullptr; e = e->getNextElement())
                if (e->hasTagName ("key") && e->getAllSubText() == "CFBundleShortVersionString")
                    if (auto* value = e->getNextElement())
                        return value->getAllSubText().trim();
   #else
    juce::ignoreUnused (bundle);
   #endif
    return {};
}

#if JUCE_MAC
// "1.1.10.249" -> "1.1.10": the bundles carry the CMake version, the manifest adds the build number.
static juce::String baseVersion (const juce::String& v)
{
    auto parts = juce::StringArray::fromTokens (v, ".", {});
    while (parts.size() > 3) parts.remove (parts.size() - 1);
    return parts.joinIntoString (".");
}
#endif

void VersionChecker::refreshInstallState (PluginInfo& info)
{
    info.installedVersion = getInstalledVersion (info.registryKey);
    info.status = determineStatus (info.installedVersion, info.remoteVersion);

    // Fallback: if registry says not installed but the files exist on disk,
    // treat as installed (handles manual installs / first run after existing install)
    if (info.status == PluginStatus::NotInstalled)
    {
        bool found = isStandaloneInstalled (info.standaloneExe)
                  || isVst3Installed (info.vst3Bundle)
                  || isAUInstalled (info.auBundle);
        if (found)
        {
            info.installedVersion = "?";
            info.status = PluginStatus::UpdateAvailable;  // can't compare -> prompt update

           #if JUCE_MAC
            // A .pkg from the website never told the Center its version, and "?"
            // offered the same update forever. The bundle itself knows.
            // System-wide or in the user's folders (Center 2.1), wherever it is.
            juce::String bundleVersion;
            for (const auto& bundle : { findVst3 (info.vst3Bundle), findAU (info.auBundle), findApp (info.standaloneExe) })
                if (bundleVersion.isEmpty() && bundle != juce::File())
                    bundleVersion = readBundleVersion (bundle);

            if (bundleVersion.isNotEmpty())
            {
                const bool current = baseVersion (bundleVersion) == baseVersion (info.remoteVersion);
                info.installedVersion = current ? info.remoteVersion : bundleVersion;
                info.status = current ? PluginStatus::UpToDate : PluginStatus::UpdateAvailable;
                if (current)
                    setInstalledVersion (info.registryKey, info.remoteVersion);
            }
           #endif
        }
    }
}

juce::String VersionChecker::getInstalledVersion (const juce::String& registryKey)
{
#if JUCE_WINDOWS
    const auto stamped = readRegString (HKEY_CURRENT_USER,
                                        juce::String (RONE_REGISTRY_PATH) + "\\" + registryKey,
                                        L"InstalledVersion");

    const auto actual = innoInstalledVersion (registryKey);

    if (actual.isNotEmpty())
    {
        if (actual != stamped)
            setInstalledVersion (registryKey, actual);   // heal the stamp in passing
        return actual;
    }

    return stamped;

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

    // The catalog is the truth: anything that is not exactly the published
    // version gets offered the published version. The old "only if remote is
    // strictly newer" rule met registry stamps written in a three-part scheme
    // ("1.0.92") against the four-part manifest ("1.0.0.98"); compared
    // component-wise the *installed* side looked newer, and every machine in
    // that state was told it was up to date, forever. Converging on the
    // manifest self-heals those machines on their next sync.
    auto a = parseVersion (installed);
    auto b = parseVersion (remote);

    const int maxLen = juce::jmax (a.size(), b.size());

    for (int i = 0; i < maxLen; ++i)
    {
        const int ai = i < a.size() ? a[i] : 0;
        const int bi = i < b.size() ? b[i] : 0;

        if (ai != bi)
            return PluginStatus::UpdateAvailable;
    }

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
    // C:\Program Files\Common Files\VST3\RONE, where every installer puts it
    // ({commoncf}\VST3\RONE). The old code took the parent of Program Files (x86),
    // which is C:\, so the VST3 check and "Open Folder" never matched.
    auto common = juce::File::getSpecialLocation (
        juce::File::globalApplicationsDirectory)
        .getChildFile ("Common Files")
        .getChildFile ("VST3")
        .getChildFile (RONE_VST3_SUBDIR);
    return common;
#elif JUCE_MAC
    return juce::File ("/Library/Audio/Plug-Ins/VST3");
#else
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory)
               .getChildFile (".vst3/RONE");
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

#if JUCE_MAC
juce::File VersionChecker::getUserVst3Dir()
{
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("Library/Audio/Plug-Ins/VST3");
}

juce::File VersionChecker::getUserAUDir()
{
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("Library/Audio/Plug-Ins/Components");
}

juce::File VersionChecker::getUserAppsDir()
{
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("Applications");
}

juce::File VersionChecker::getUserManualsDir()
{
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory)
               .getChildFile ("Library/Application Support/RONE Plugins/Manuals");
}

juce::File VersionChecker::findVst3 (const juce::String& bundleName)
{
    if (bundleName.isEmpty()) return {};
    for (const auto& dir : { getVst3InstallDir(), getUserVst3Dir() })
        if (dir.getChildFile (bundleName).exists())
            return dir.getChildFile (bundleName);
    return {};
}

juce::File VersionChecker::findAU (const juce::String& bundleName)
{
    if (bundleName.isEmpty()) return {};
    for (const auto& dir : { getAUInstallDir(), getUserAUDir() })
        if (dir.getChildFile (bundleName).exists())
            return dir.getChildFile (bundleName);
    return {};
}

juce::File VersionChecker::findApp (const juce::String& exeName)
{
    if (exeName.isEmpty()) return {};
    const auto appName = exeName.replace (".exe", "") + ".app";
    // /Applications (the .pkg), /Applications/RONE Plugins (older installers),
    // ~/Applications (Center 2.1), ~/Library/RONE Plugins (older still)
    for (const auto& dir : { juce::File ("/Applications"), juce::File ("/Applications/RONE Plugins"),
                             getUserAppsDir(), getStandaloneInstallDir() })
        if (dir.getChildFile (appName).exists())
            return dir.getChildFile (appName);
    return {};
}
#endif

bool VersionChecker::isStandaloneInstalled (const juce::String& exeName)
{
    if (exeName.isEmpty()) return false;

#if JUCE_MAC
    return findApp (exeName) != juce::File();
#else
    return getStandaloneInstallDir().getChildFile (exeName).existsAsFile();
#endif
}

bool VersionChecker::isVst3Installed (const juce::String& bundleName)
{
    if (bundleName.isEmpty()) return false;
#if JUCE_MAC
    return findVst3 (bundleName) != juce::File();
#else
    auto vst3 = getVst3InstallDir().getChildFile (bundleName);
    return vst3.exists(); // could be file or directory (bundle)
#endif
}

bool VersionChecker::isAUInstalled (const juce::String& bundleName)
{
    if (bundleName.isEmpty()) return false;

#if JUCE_MAC
    return findAU (bundleName) != juce::File();
#else
    juce::ignoreUnused (bundleName);
    return false;
#endif
}
