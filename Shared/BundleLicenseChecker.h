#pragma once

// juce_core only — so this header also compiles in plugins that include
// JUCE modules directly instead of a generated JuceHeader.h (e.g. Flanger).
#include <juce_core/juce_core.h>

#if JUCE_WINDOWS
 // windows.h must not leak min/max macros into JUCE headers included after
 // this one (std::numeric_limits<>::max() turns into a syntax error).
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
#endif

// ============================================================================
// BundleLicenseChecker — lightweight, read-only license check for plugins
//
// Each RONE plugin includes this header to ask what the user is entitled to.
// The Center writes the license state to a shared XML file; plugins just
// read it.
//
// Usage:
//   if (BundleLicenseChecker::isProductLicensed ("RoneStutter"))
//       enableProFeatures();
//
// ----------------------------------------------------------------------------
// TWO WAYS TO OWN A PLUGIN
// ----------------------------------------------------------------------------
// RONE ALL ACCESS ($20/mo) covers every plugin — that is `licensed="1"` in
// the file, and isBundleLicensed() answers it. A per-plugin LIFETIME licence
// covers exactly one product and is listed in the `products` attribute.
//
// isBundleLicensed() deliberately stays ALL-ACCESS-only. A lifetime-only
// customer gets `licensed="0"`, which is precisely what keeps every plugin
// build shipped before lifetime licences existed locked: those builds only
// know how to read `licensed`. New builds call isProductLicensed() instead.
//
// ----------------------------------------------------------------------------
// OPEN / LOCKED STATE
// ----------------------------------------------------------------------------
// The old compile-time RONE_BETA_MODE flag is gone. "Everything runs free"
// is now controlled REMOTELY via RemoteLicenseGate (the `license_mode` field
// of versions.json). Plugins combine the two:
//
//     unlocked = RemoteLicenseGate::isOpenMode() || isProductLicensed ("<id>")
//
// so flipping license_mode to "enforced" on GitHub locks every install that
// has this build, without recompiling anything.
// ============================================================================

class BundleLicenseChecker
{
public:
    // ---- ALL ACCESS check: read the shared XML file ------------------------
    // True only while an ALL ACCESS subscription is active. Owning a plugin
    // outright does NOT make this true — see the note above.
    static bool isBundleLicensed()
    {
        const auto state = readLicenseState();

        if (! state.valid)
            return false;

        if (! state.bundle)
        {
            DBG ("BundleLicenseChecker: licensed = false in XML");
            return false;
        }

        // 8 days = the Center's 7-day offline grace plus a day of buffer.
        return withinGrace (state, BUNDLE_GRACE_MS);
    }

    // ---- Per-plugin check: ALL ACCESS, or this one product owned outright --
    // `productId` is the canonical id from versions.json ("RoneStutter",
    // "RoneStucker", ...) — the same token the Center writes into `products`.
    static bool isProductLicensed (const juce::String& productId)
    {
        const auto state = readLicenseState();

        if (! state.valid)
            return false;

        // ALL ACCESS covers everything, but on its own 8-day window: a
        // subscriber must not inherit the perpetual grace below.
        if (state.bundle && withinGrace (state, BUNDLE_GRACE_MS))
            return true;

        if (! ownsProduct (state.products, productId))
            return false;

        // 90 days for a perpetual licence. It has nothing to expire; the only
        // reason to re-check at all is a refund, and the Center re-validates
        // every 24h anyway. Locking a paying owner out of a plugin he bought
        // after 8 offline days would be wrong.
        return withinGrace (state, PERPETUAL_GRACE_MS);
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
    // Accept if validated within 8 days (7-day grace + 1-day buffer)
    static constexpr juce::int64 BUNDLE_GRACE_MS    =  8LL * 24 * 60 * 60 * 1000;
    static constexpr juce::int64 PERPETUAL_GRACE_MS = 90LL * 24 * 60 * 60 * 1000;

    // A validation stamp a few minutes ahead of the clock is an NTP
    // correction, not a forgery — see withinGrace().
    static constexpr juce::int64 CLOCK_SKEW_SLACK_MS = 5LL * 60 * 1000;

    // Everything both public checks need out of ONE parse. Two copies of this
    // parse would eventually drift, and a drifted copy is a free licence.
    struct LicenseState
    {
        bool         valid          = false;  // file exists, parsed, is ours
        bool         bundle         = false;  // licensed="1" — ALL ACCESS
        juce::String products;                // comma-separated canonical ids
        juce::int64  lastValidation = 0;      // ms since epoch, 0 when absent
    };

    static LicenseState readLicenseState()
    {
        LicenseState state;

        auto file = getLicenseFile();

        DBG ("BundleLicenseChecker: checking file -> " + file.getFullPathName());

        if (! file.existsAsFile())
        {
            DBG ("BundleLicenseChecker: file does NOT exist");
            return state;
        }

        DBG ("BundleLicenseChecker: file EXISTS, size = " + juce::String (file.getSize()) + " bytes");

        auto xml = juce::parseXML (file);
        if (xml == nullptr)
        {
            // Fail closed: an unparseable file is not proof of a license. A
            // corrupted file self-heals on the next Center validation, which
            // rewrites it; treating garbage as LICENSED means any hand-made
            // file unlocks the bundle.
            DBG ("BundleLicenseChecker: XML parse returned nullptr — NOT licensed");
            return state;
        }

        DBG ("BundleLicenseChecker: XML tag = " + xml->getTagName());

        if (xml->getTagName() != "RoneBundleLicense")
        {
            DBG ("BundleLicenseChecker: unexpected tag name — NOT licensed");
            return state;
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

        state.valid          = true;
        state.bundle         = licensed;
        state.products       = xml->getStringAttribute ("products");
        state.lastValidation = xml->getStringAttribute ("lastValidationTime", "0").getLargeIntValue();

        DBG ("BundleLicenseChecker: products = '" + state.products + "'");

        return state;
    }

    // Whole-token, case-insensitive compare. A substring match here would be
    // a licence give-away: products="RoneStutter" must not unlock "RoneStut",
    // and a plugin called "RoneStuckerX" must not ride in on "RoneStucker".
    static bool ownsProduct (const juce::String& products, const juce::String& productId)
    {
        const auto wanted = productId.trim();

        if (wanted.isEmpty() || products.isEmpty())
            return false;

        juce::StringArray owned;
        owned.addTokens (products, ",", "");
        owned.trim();                 // tolerate "a, b , c"
        owned.removeEmptyStrings();

        const bool found = owned.contains (wanted, true);

        DBG ("BundleLicenseChecker: product '" + wanted + "' in '" + products + "' -> "
             + juce::String (found ? "OWNED" : "not owned"));

        return found;
    }

    // The one grace test, so the two windows cannot drift apart.
    static bool withinGrace (const LicenseState& state, juce::int64 graceMs)
    {
        const auto now = juce::Time::currentTimeMillis();

        // Absent, zero or negative means no validation ever happened. A stamp
        // in the future means the clock moved — or a hand-written file is
        // buying itself an endless grace. Neither is evidence of a licence,
        // so both fail closed. The few minutes of slack keep an NTP
        // correction landing just after a validation from locking an owner
        // out. (Checked before the subtraction so a nonsense stamp cannot
        // overflow it.)
        if (state.lastValidation <= 0 || state.lastValidation > now + CLOCK_SKEW_SLACK_MS)
        {
            DBG ("BundleLicenseChecker: lastValidationTime = " + juce::String (state.lastValidation)
                 + " is not a past validation — NOT licensed");
            return false;
        }

        const auto elapsed = now - state.lastValidation;

        DBG ("BundleLicenseChecker: lastValidationTime = " + juce::String (state.lastValidation)
             + ", now = " + juce::String (now)
             + ", elapsed = " + juce::String (elapsed) + " ms"
             + ", grace = " + juce::String (graceMs / (1000 * 60 * 60 * 24)) + " days");

        if (elapsed >= graceMs)
        {
            DBG ("BundleLicenseChecker: EXPIRED — elapsed "
                 + juce::String (elapsed / (1000 * 60 * 60 * 24)) + " days");
            return false;
        }

        DBG ("BundleLicenseChecker: LICENSED — all checks passed!");
        return true;
    }

    BundleLicenseChecker() = delete; // static-only class
};
