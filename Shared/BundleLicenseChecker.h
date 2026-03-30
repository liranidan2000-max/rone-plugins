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
        auto file = getLicenseFile();

        DBG ("BundleLicenseChecker: checking file -> " + file.getFullPathName());

        if (! file.existsAsFile())
        {
            DBG ("BundleLicenseChecker: file does NOT exist");
            return false;
        }

        DBG ("BundleLicenseChecker: file EXISTS, size = " + juce::String (file.getSize()) + " bytes");

        // Read raw content for debug
        auto rawContent = file.loadFileAsString();
        DBG ("BundleLicenseChecker: raw content = " + rawContent.substring (0, 500));

        auto xml = juce::parseXML (file);
        if (xml == nullptr)
        {
            DBG ("BundleLicenseChecker: XML parse returned nullptr — treating existing file as LICENSED (fallback)");
            return true;  // Fallback: don't block user if file exists but can't parse
        }

        DBG ("BundleLicenseChecker: XML tag = " + xml->getTagName());

        if (xml->getTagName() != "RoneBundleLicense")
        {
            DBG ("BundleLicenseChecker: unexpected tag name, but file exists — treating as LICENSED (fallback)");
            return true;  // Fallback: don't block user
        }

        // Check licensed attribute — accept "1", "true", "yes"
        bool licensed = xml->getBoolAttribute ("licensed", false);
        DBG ("BundleLicenseChecker: licensed attribute = " + juce::String (licensed ? "true" : "false"));

        if (! licensed)
        {
            // Double-check: maybe the attribute value is "1" stored as string
            auto licensedStr = xml->getStringAttribute ("licensed", "");
            DBG ("BundleLicenseChecker: licensed raw string = '" + licensedStr + "'");
            if (licensedStr == "1" || licensedStr.equalsIgnoreCase ("true") || licensedStr.equalsIgnoreCase ("yes"))
                licensed = true;
        }

        if (! licensed)
        {
            DBG ("BundleLicenseChecker: licensed = false in XML");
            return false;
        }

        auto lastValStr = xml->getStringAttribute ("lastValidationTime", "0");
        auto lastVal = lastValStr.getLargeIntValue();
        auto now = juce::Time::currentTimeMillis();
        auto elapsed = now - lastVal;

        DBG ("BundleLicenseChecker: lastValidationTime = " + lastValStr
             + ", now = " + juce::String (now)
             + ", elapsed = " + juce::String (elapsed) + " ms");

        // Accept if validated within 8 days (7-day grace + 1-day buffer)
        static constexpr int64_t GRACE_LIMIT_MS = 8LL * 24 * 60 * 60 * 1000;

        if (elapsed >= GRACE_LIMIT_MS)
        {
            DBG ("BundleLicenseChecker: EXPIRED — elapsed " + juce::String (elapsed / (1000 * 60 * 60 * 24)) + " days > 8 day limit");
            return false;
        }

        DBG ("BundleLicenseChecker: LICENSED — all checks passed!");
        return true;
    }

    // Helper: get the license file path (shared with Center app)
    static juce::File getLicenseFile()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("RonePlugins")
                   .getChildFile ("BundleLicense.xml");
    }

    // ---- Quick registry check (Windows only) -------------------------------
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
