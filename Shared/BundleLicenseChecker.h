#pragma once

#include <JuceHeader.h>

#if JUCE_WINDOWS
 #include <windows.h>
#endif

// ============================================================================
// BundleLicenseChecker — lightweight, read-only license check for plugins
//
// Each RONE plugin includes this header to check whether the user has an
// active RONE Full Bundle license.  The Center writes the license state to
// a shared XML file; plugins just read it.
//
// Usage:
//   if (BundleLicenseChecker::isBundleLicensed())
//       enableProFeatures();
// ============================================================================
class BundleLicenseChecker
{
public:
    // ---- Primary check: read the shared XML file ---------------------------
    static bool isBundleLicensed()
    {
        auto file = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                        .getChildFile ("RonePlugins")
                        .getChildFile ("BundleLicense.xml");

        if (! file.existsAsFile())
            return false;

        auto xml = juce::parseXML (file);
        if (xml == nullptr || xml->getTagName() != "RoneBundleLicense")
            return false;

        bool licensed = xml->getBoolAttribute ("licensed", false);
        if (! licensed)
            return false;

        auto lastVal = xml->getStringAttribute ("lastValidationTime", "0").getLargeIntValue();
        auto elapsed = juce::Time::currentTimeMillis() - lastVal;

        // Accept if validated within 8 days (7-day grace + 1-day buffer)
        static constexpr int64_t GRACE_LIMIT_MS = 8LL * 24 * 60 * 60 * 1000;
        return elapsed < GRACE_LIMIT_MS;
    }

    // ---- Quick registry check (Windows only) -------------------------------
    // Returns "active", "expired", or "" (empty = no entry).
    // Plugins that already link advapi32 can use this for a faster check
    // before falling back to the XML file.
    static juce::String getRegistryStatus()
    {
    #if JUCE_WINDOWS
        HKEY hKey = nullptr;

        if (RegOpenKeyExW (HKEY_CURRENT_USER,
                           L"Software\\RONE\\License",
                           0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            wchar_t buf[64] = {};
            DWORD bufSize = sizeof (buf);
            DWORD type    = REG_SZ;

            if (RegQueryValueExW (hKey, L"BundleStatus", nullptr, &type,
                                  reinterpret_cast<LPBYTE> (buf), &bufSize) == ERROR_SUCCESS)
            {
                RegCloseKey (hKey);
                return juce::String (buf);
            }

            RegCloseKey (hKey);
        }
    #endif

        return {};
    }

private:
    BundleLicenseChecker() = delete; // static-only class
};
