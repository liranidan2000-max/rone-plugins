#pragma once

#include <JuceHeader.h>

#if JUCE_WINDOWS
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
#endif

// ============================================================================
// BundleLicenseFile — the ONE read-modify-write path to BundleLicense.xml
//
// Two things inside the Center claim that file. LicenseHandler puts a Lemon
// Squeezy bundle serial in it — that is how everyone who bought before
// accounts existed is still unlocked. AccountClient puts a roneaudio.com
// account in it: the ALL ACCESS pass, and the plugins bought outright as
// one-off LIFETIME licences.
//
// Both used to compose the whole document out of their own fields, so
// whichever wrote last erased the other. A pre-accounts customer who later
// bought one lifetime plugin lost his serial or his plugin on every
// re-validation, and a failed Lemon Squeezy check deleted the file outright —
// revoking a perpetual licence that had nothing to do with it and cannot
// expire. Those were lockouts of people who had paid.
//
// So neither of them writes a document any more. Each hands over ONLY the
// attributes it owns, and everything else is carried across from what is
// already on disk:
//
//     LicenseHandler   serialLicensed, licenseKey, instanceId, customerName
//     AccountClient    accountLicensed, products
//
// `licensed` and `lastValidationTime` belong to NEITHER of them. They are
// derived here, because they are what every already-shipped plugin build
// reads and neither writer knows the whole truth on its own:
//
//     licensed           = serialLicensed OR accountLicensed
//     lastValidationTime = the newest stamp either side has ever earned
//
// `licensed` therefore keeps meaning "ALL ACCESS is active, or a genuine
// legacy serial is". Owning a plugin outright must NEVER flip it: the builds
// already sitting in people's DAWs read that one attribute and nothing else,
// so a lifetime-only customer who saw licensed="1" would be handed the whole
// catalogue. That is why `products` is a separate attribute and why nothing
// in here ever folds it into `licensed`.
//
// One stamp, two clocks — a deliberate trade. Whichever side validated most
// recently sets `lastValidationTime` for both, so a live account can carry a
// serial's 8-day window (and vice versa) a little further than that side
// earned on its own. The alternative — letting the older stamp win — locks a
// paying customer out the moment one of the two servers is unreachable, which
// is the failure mode this file exists to end. The stamp only ever moves
// forward for that reason.
// ============================================================================
namespace BundleLicenseFile
{

/** Everything the file holds, both groups, already migrated out of the older
    single-writer shapes. */
struct Contents
{
    // ---- LicenseHandler's group: a Lemon Squeezy bundle serial -------------
    bool         serialLicensed = false;
    juce::String licenseKey;
    juce::String instanceId;
    juce::String customerName;

    // ---- AccountClient's group: the roneaudio.com account ------------------
    bool         accountLicensed = false;   // the ALL ACCESS pass is active
    juce::String products;                  // comma-separated canonical ids

    // ---- Derived, owned by neither ----------------------------------------
    // The newest moment either server said yes. Every plugin's offline grace
    // is measured from it (8 days for the pass, 90 for a plugin bought
    // outright), so it may only ever move forward.
    juce::int64  lastValidationTime = 0;

    /** What every shipped plugin build reads. True for an active pass or a
        genuine serial — never for a plugin owned outright. */
    bool isBundleLicensed() const noexcept   { return serialLicensed || accountLicensed; }

    /** False when the file would claim nothing at all. That is the only case
        in which it may be deleted: its absence is what the plugins read as
        "not licensed", and deleting it while a claim survives is how a paid
        licence used to disappear. */
    bool holdsSomething() const noexcept
    {
        return isBundleLicensed() || products.isNotEmpty();
    }
};

inline juce::File getFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("RonePlugins")
               .getChildFile ("BundleLicense.xml");
}

namespace detail
{
    /** The two writers answer on different threads — LicenseHandler on the
        message thread, AccountClient on its network thread — and a
        read-modify-write is only safe if they take turns. */
    inline juce::CriticalSection& fileLock()
    {
        static juce::CriticalSection cs;
        return cs;
    }

    /** The registry value mirrors the PASS and nothing else: anything reading
        it reads it as "the bundle is active", so a lifetime-only account must
        never leave an "active" behind. */
    inline void mirrorToRegistry (bool bundleActive)
    {
       #if JUCE_WINDOWS
        HKEY hKey = nullptr;

        if (! bundleActive)
        {
            if (RegOpenKeyExW (HKEY_CURRENT_USER, L"Software\\RONE\\License", 0, KEY_SET_VALUE, &hKey)
                == ERROR_SUCCESS)
            {
                RegDeleteValueW (hKey, L"BundleStatus");
                RegCloseKey (hKey);
            }
            return;
        }

        DWORD disposition = 0;
        if (RegCreateKeyExW (HKEY_CURRENT_USER, L"Software\\RONE\\License", 0, nullptr,
                             REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, &disposition)
            == ERROR_SUCCESS)
        {
            // sizeof, not wcslen: the array carries its own terminator and this
            // header must not depend on <cwchar> reaching it.
            static const wchar_t value[] = L"active";
            RegSetValueExW (hKey, L"BundleStatus", 0, REG_SZ,
                            reinterpret_cast<const BYTE*> (value),
                            (DWORD) sizeof (value));
            RegCloseKey (hKey);
        }
       #else
        juce::ignoreUnused (bundleActive);
       #endif
    }

    /** Caller holds fileLock(). Fails closed: anything unparseable or not ours
        reads as an empty claim, exactly like BundleLicenseChecker. */
    inline Contents readLocked()
    {
        Contents c;

        auto file = getFile();
        if (! file.existsAsFile())
            return c;

        auto xml = juce::parseXML (file);
        if (xml == nullptr || xml->getTagName() != "RoneBundleLicense")
            return c;

        c.licenseKey         = xml->getStringAttribute ("licenseKey",   "");
        c.instanceId         = xml->getStringAttribute ("instanceId",   "");
        c.customerName       = xml->getStringAttribute ("customerName", "");
        c.products           = xml->getStringAttribute ("products",     "");
        c.lastValidationTime = xml->getStringAttribute ("lastValidationTime", "0").getLargeIntValue();

        if (xml->hasAttribute ("serialLicensed") || xml->hasAttribute ("accountLicensed"))
        {
            c.serialLicensed  = xml->getBoolAttribute ("serialLicensed",  false);
            c.accountLicensed = xml->getBoolAttribute ("accountLicensed", false);
            return c;
        }

        // A file from a Center that still had a single writer. It carries one
        // merged `licensed` and no idea whose it was, so read it off the shape
        // the writer left behind: the account path stamped "account:<email>"
        // where a serial belongs, and only ever wrote the file when it held
        // something. Guessing wrong here would sign the customer out, so both
        // branches keep the claim rather than dropping it.
        const bool legacyLicensed = xml->getBoolAttribute ("licensed", false);
        const bool looksLikeSerial = c.licenseKey.isNotEmpty()
                                       && ! c.licenseKey.startsWith ("account:");

        if (looksLikeSerial)
        {
            c.serialLicensed = legacyLicensed;
        }
        else
        {
            c.accountLicensed = legacyLicensed;

            // The account never owned these; they were shape-filler. Leaving
            // "account:<email>" in licenseKey would only confuse the serial
            // side, which now reads its own group.
            c.licenseKey.clear();
            c.instanceId.clear();
        }

        return c;
    }

    /** Caller holds fileLock(). Writes the merged document, or removes it when
        nothing is left to claim. */
    inline void commitLocked (const Contents& c)
    {
        auto file = getFile();

        if (! c.holdsSomething())
        {
            file.deleteFile();
            mirrorToRegistry (false);
            return;
        }

        const bool bundle = c.isBundleLicensed();

        juce::XmlElement xml ("RoneBundleLicense");
        xml.setAttribute ("licensed",           bundle ? 1 : 0);
        xml.setAttribute ("products",           c.products);
        xml.setAttribute ("customerName",       c.customerName);
        xml.setAttribute ("lastValidationTime", juce::String (c.lastValidationTime));
        xml.setAttribute ("licenseKey",         c.licenseKey);
        xml.setAttribute ("instanceId",         c.instanceId);

        // Whose claim `licensed` came from. Plugins ignore these two; they
        // exist so that the next write by either side can leave the other's
        // claim exactly as it found it.
        xml.setAttribute ("serialLicensed",     c.serialLicensed  ? 1 : 0);
        xml.setAttribute ("accountLicensed",    c.accountLicensed ? 1 : 0);

        file.getParentDirectory().createDirectory();
        xml.writeTo (file, {});

        mirrorToRegistry (bundle);
    }
}

/** The whole file, for a caller that wants to read back its own group. */
inline Contents load()
{
    const juce::ScopedLock sl (detail::fileLock());
    return detail::readLocked();
}

/** LicenseHandler's group. `validatedAt` is the last time Lemon Squeezy said
    yes — never the moment of writing. */
inline void writeSerial (bool licensed,
                         const juce::String& licenseKey,
                         const juce::String& instanceId,
                         const juce::String& customerName,
                         juce::int64 validatedAt)
{
    const juce::ScopedLock sl (detail::fileLock());

    auto c = detail::readLocked();
    c.serialLicensed     = licensed;
    c.licenseKey         = licenseKey;
    c.instanceId         = instanceId;
    c.customerName       = customerName;
    c.lastValidationTime = juce::jmax (c.lastValidationTime, validatedAt);

    detail::commitLocked (c);
}

/** Drops the serial and everything that describes it, and NOTHING else. A
    revoked or deactivated serial says nothing about a plugin the same person
    bought outright. */
inline void clearSerial()
{
    const juce::ScopedLock sl (detail::fileLock());

    auto c = detail::readLocked();
    c.serialLicensed = false;
    c.licenseKey     = {};
    c.instanceId     = {};
    c.customerName   = {};

    detail::commitLocked (c);
}

/** AccountClient's group. `validatedAt` is the last time roneaudio.com
    answered — never the moment of writing, or an offline launch would restart
    every plugin's grace clock and a refund could never be enforced. */
inline void writeAccount (bool licensed,
                          const juce::String& products,
                          juce::int64 validatedAt)
{
    const juce::ScopedLock sl (detail::fileLock());

    auto c = detail::readLocked();
    c.accountLicensed    = licensed;
    c.products           = products;
    c.lastValidationTime = juce::jmax (c.lastValidationTime, validatedAt);

    detail::commitLocked (c);
}

/** Signing out drops the account and its plugins, and NOTHING else. A legacy
    serial on the same machine was never part of the account. */
inline void clearAccount()
{
    const juce::ScopedLock sl (detail::fileLock());

    auto c = detail::readLocked();
    c.accountLicensed = false;
    c.products        = {};

    detail::commitLocked (c);
}

}
