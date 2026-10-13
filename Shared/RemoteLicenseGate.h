#pragma once

// juce_core only — so this header also compiles in plugins that include
// JUCE modules directly instead of a generated JuceHeader.h (e.g. Flanger).
#include <juce_core/juce_core.h>
#include <atomic>

// ============================================================================
// RemoteLicenseGate — remote kill-switch for the whole RONE bundle.
//
// Source of truth: the `license_mode` field of versions.json on GitHub:
//
//     "license_mode": "open"       -> everything runs WITHOUT a license
//     "license_mode": "enforced"   -> a real bundle license is required
//
// The Plugins Center and every plugin cache the last fetched mode in ONE
// shared XML file (next to BundleLicense.xml), refresh it from the network
// in the background at most once per kRefreshIntervalHours, and read the
// cache cheaply on the message thread. Missing/unreadable data defaults to
// ENFORCED (fail closed): since the bundle went on sale, an install that has
// never fetched the manifest needs a real licence like any other.
//
// To lock every install that has ever shipped: edit versions.json on the
// monorepo's main branch, set "license_mode": "enforced" (optionally with a
// "license_message"), and push. Every Center / plugin instance with internet
// picks it up on its next refresh (at most a few hours, or immediately on
// next launch). Setting it back to "open" unlocks the same way.
//
// Usage in a plugin (message thread only — never the audio thread):
//     RemoteLicenseGate::refreshFromNetworkAsync();      // throttled, async
//     bool unlocked = RemoteLicenseGate::isOpenMode()
//                  || BundleLicenseChecker::isProductLicensed ("<id from versions.json>");
// (isProductLicensed answers ALL ACCESS and a lifetime licence of that one
// plugin; the ALL-ACCESS-only check would lock every lifetime buyer out.)
//
// NOTE: RONEAnalyzer keeps byte-identical copies of the licence headers in
// RONEAnalyzer/Source/Core/ - copy them over unchanged after every edit here
// (Shared/check-vendored-copies.sh lists them and diffs them). RoneStutter
// includes this file through a one-line shim in its Source/.
// ============================================================================

class RemoteLicenseGate
{
public:
    static constexpr double kRefreshIntervalHours = 6.0;

    // A plugin-side fetch that failed (offline, captive portal) is retried
    // after this - not after the full 6 h (audit R279).
    static constexpr juce::int64 kRetryAfterFailureMs = 10LL * 60 * 1000;

    // No plugin-side fetch in the first 30 s after a module's first licence
    // check (audit R194) - see refreshFromNetworkAsync().
    static constexpr juce::uint32 kQuietAfterLoadMs = 30000;

    // ---- Fast cache read (file only, no network). Message thread. ----------
    // True = the bundle is in open mode and no license is needed right now.
    static bool isOpenMode()
    {
        // LicenseMode.xml is a plain file: mode="open" typed into it unlocked
        // everything. Once signed licences are required, only a signed
        // entitlement unlocks a plugin - the open mode is no longer honoured.
        if (signedLicencesRequired())
            return false;

        auto xml = juce::parseXML (getModeFile());
        if (xml == nullptr || xml->getTagName() != "RoneLicenseMode")
            return false; // no data yet -> enforced (fail closed; the Center writes the cache on its first run)

        return ! xml->getStringAttribute ("mode", "enforced")
                     .equalsIgnoreCase ("enforced");
    }

    // Optional operator message shown when the bundle is locked ("" if none).
    // ---- Signed licences (Center 2.0, Shared/RoneEntitlement.h) ----------
    // From this date every plugin built with this header trusts only the
    // server-signed entitlement in BundleLicense.xml, whatever any local file
    // says. Before it, versions.json can turn the requirement on early
    // ("signed_licences": "required"); nothing local can turn it off. Lemon
    // Squeezy serials carry no signature: their holders need an account by then.
    static constexpr juce::int64 kSignedRequiredFromMs = 1803859200000LL;   // 2027-03-01 00:00 UTC

    static bool signedLicencesRequired()
    {
        if (juce::Time::currentTimeMillis() >= kSignedRequiredFromMs)
            return true;
        auto xml = juce::parseXML (getModeFile());
        return xml != nullptr && xml->getStringAttribute ("signed").equalsIgnoreCase ("required");
    }

    static juce::String getLockMessage()
    {
        auto xml = juce::parseXML (getModeFile());
        return xml != nullptr ? xml->getStringAttribute ("message", juce::String())
                              : juce::String();
    }

    // ---- JUCE's VST3 manifest helper (juce_vst3_helper) links the plugin code
    // into a tiny build-time process that enumerates the classes and exits at
    // once. A background thread or a timer started from the constructor is
    // still running when its statics are torn down -> "Segmentation fault: 11"
    // in CI (AFTERSPACE, run #167). Nothing that outlives the constructor may
    // start in there.
    static bool isRunningInPluginBuildHelper()
    {
        static const bool inHelper = juce::File::getSpecialLocation (juce::File::currentExecutableFile)
                                         .getFileNameWithoutExtension()
                                         .containsIgnoreCase ("vst3_helper");
        return inHelper;
    }

    // ---- Background refresh (throttled fire-and-forget). -------------------
    // Call from constructors / prepareToPlay / editor timers. Never blocks and
    // never writes a file on the caller's thread: the fetch, and the stamp
    // that records it, run on a disposable thread that only rewrites the
    // cache files.
    static void refreshFromNetworkAsync()
    {
        if (isRunningInPluginBuildHelper())
            return;

        // The fetch thread is detached (up to 8 s of HTTPS plus two file
        // writes). A host scanning plugins, or an insert undone at once,
        // creates and destroys an instance - and may unload the module -
        // well inside that, and the thread would then return into unmapped
        // code. So nothing is launched in the first 30 s after this module's
        // first licence check: an instance that lives on fetches on its next
        // 60 s licence tick. Licence reads are untouched - only the cached
        // manifest refresh moves (audit R194). Counted per module: each
        // plugin binary has its own copy of this static.
        static const juce::uint32 firstCallMs = juce::Time::getMillisecondCounter();
        if (juce::Time::getMillisecondCounter() - firstCallMs < kQuietAfterLoadMs)
            return;

        if (! isCacheStale())
            return;

        // One fetch per module at a time. The attempt stamp the thread writes
        // first keeps the other RONE plugins in the same DAW (other modules)
        // from piling on for the next 10 minutes.
        auto& inFlight = fetchInFlight();
        bool idle = false;
        if (! inFlight.compare_exchange_strong (idle, true))
            return;

        const bool launched = juce::Thread::launch ([]
        {
            const juce::ScopeGuard done { [] { fetchInFlight().store (false); } };

            // Record the attempt (fetchedAt kept, attemptedAt = now), so a
            // failure below is retried in 10 minutes rather than 6 hours.
            const auto cached = readModeFile (getModeFile());
            if (! writeModeTo (getModeFile(), cached.mode, cached.message, cached.signedLicences,
                               cached.fetchedAt, juce::Time::currentTimeMillis())
                && ! cached.mode.equalsIgnoreCase ("enforced"))
            {
                // A cache that says "open" and cannot be rewritten never
                // receives "enforced" (or signed="required") again: a frozen
                // file. Observed only - nothing changes for this machine
                // (audit R005; the enforce step is docs/LICENCE-CUTOVER.md).
                observeOnce ("LICENCE_MODE_UNWRITABLE",
                             "LicenseMode.xml says open and cannot be rewritten",
                             "host=" + hostName());
            }

            // raw.githubusercontent caches this for five minutes at the CDN edge,
            // so a fetch can hand back the PREVIOUS manifest. The "?t=" below does
            // NOT get past that cache: the CDN keys on the path alone (X-Cache: HIT
            // with a fresh ?t=, measured 2026-09-11 and 2026-10-12 - see the
            // Center's NetworkManager.cpp). So a flipped kill-switch reaches a
            // plugin up to five minutes late, on top of the 6 h throttle above;
            // the Center, which also writes LicenseMode.xml, is the fast path.
            juce::URL url (juce::String (kManifestUrl)
                             + "?t=" + juce::String (juce::Time::currentTimeMillis()));
            auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                               .withConnectionTimeoutMs (8000)
                               .withNumRedirectsToFollow (5);

            auto stream = url.createInputStream (options);
            if (stream == nullptr)
                return; // offline — keep whatever the cache says

            auto body = stream->readEntireStreamAsString();
            if (body.isEmpty())
                return;

            auto parsed = juce::JSON::parse (body);
            if (! parsed.isObject() || ! parsed.getProperty ("plugins", {}).isArray())
                return; // garbage (captive portal, an API rate-limit message) — keep cache

            writeMode (parsed.getProperty ("license_mode",    "enforced").toString(),
                       parsed.getProperty ("license_message", ""    ).toString(),
                       parsed.getProperty ("signed_licences", ""    ).toString());
            writeLatestVersions (parsed);
        });

        if (! launched)
            inFlight.store (false);
    }

    // ---- Every product's published version, for the update prompt ----------
    // (RoneUpdatePrompt.h). Written from the same fetch as the licence mode, by
    // the plugins and by the Plugins Center, into LatestVersions.xml next to
    // LicenseMode.xml. The manifest's `version` is "<base>.<CI run>"; the prompt
    // compares only the first three numbers with the running binary's version.
    static void writeLatestVersions (const juce::var& manifest)
    {
        auto* plugins = manifest.getProperty ("plugins", juce::var()).getArray();
        if (plugins == nullptr)
            return;

        juce::XmlElement xml ("RoneLatestVersions");
        xml.setAttribute ("fetchedAt", juce::String (juce::Time::currentTimeMillis()));

        for (const auto& plugin : *plugins)
        {
            const auto id      = plugin.getProperty ("id", {}).toString();
            const auto version = plugin.getProperty ("version", {}).toString();
            if (id.isEmpty() || version.isEmpty())
                continue;

            auto* entry = xml.createNewChildElement ("Product");
            entry->setAttribute ("id", id);
            entry->setAttribute ("version", version);
        }

        auto file = getLatestFile();
        file.getParentDirectory().createDirectory();
        xml.writeTo (file, {});
    }

    // "1.4.1.230" for a product id, or "" when nothing is cached yet.
    static juce::String latestVersionFor (const juce::String& productId)
    {
        auto xml = juce::parseXML (getLatestFile());
        if (xml == nullptr || xml->getTagName() != "RoneLatestVersions")
            return {};

        for (auto* entry : xml->getChildWithTagNameIterator ("Product"))
            if (entry->getStringAttribute ("id") == productId)
                return entry->getStringAttribute ("version");

        return {};
    }

    static juce::File getLatestFile()
    {
        return getModeFile().getSiblingFile ("LatestVersions.xml");
    }

    // ---- Cache writer — also called by the Plugins Center, which already ---
    // fetches the manifest on every launch/refresh. A fresh manifest: the
    // file is rewritten with fetchedAt = now. False when the file could not
    // be written.
    static bool writeMode (const juce::String& mode, const juce::String& message,
                           const juce::String& signedLicences = {})
    {
        return writeModeTo (getModeFile(), mode, message, signedLicences,
                            juce::Time::currentTimeMillis(), 0);
    }

    static juce::File getModeFile()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("RonePlugins")
                   .getChildFile ("LicenseMode.xml");
    }

    // ---- Licence-chain observations (measure before enforcing) -------------
    // One "error" report into the queue every RONE product writes and the
    // Plugins Center uploads (<appdata>/RonePlugins/CrashReports/pending, the
    // JSON of Shared/RoneCrashReporter.h writeReport). At most once per 30
    // days per code on this computer; no ids, hashes or account data in it;
    // no network of its own. Not through RoneCrashReporter itself: RoneStutter
    // still carries a private copy of that header, and the two in one
    // translation unit would not compile (audit R117).
    static void observeOnce (const juce::String& code, const juce::String& message,
                             const juce::String& details)
    {
        if (! isRunningInPluginBuildHelper())
            observeOnceIn (getModeFile().getSiblingFile ("CrashReports"), code, message, details);
    }

    // The host process ("FL64", "Ableton Live 12 Suite", ...) - no path.
    static juce::String hostName()
    {
        return juce::File::getSpecialLocation (juce::File::hostApplicationPath).getFileNameWithoutExtension();
    }

private:
    friend struct RemoteLicenseGateTest;   // Shared/tests/licence_sim

    static constexpr const char* kManifestUrl =
        "https://raw.githubusercontent.com/liranidan2000-max/rone-plugins/main/versions.json";

    static std::atomic<bool>& fetchInFlight()
    {
        static std::atomic<bool> inFlight { false };
        return inFlight;
    }

    struct CachedMode
    {
        juce::String mode { "enforced" }, message, signedLicences;
        juce::int64  fetchedAt = 0;      // the last manifest that arrived (ms, 0 = never)
        juce::int64  attemptedAt = 0;    // the last plugin-side try (ms, 0 = none recorded)
    };

    static CachedMode readModeFile (const juce::File& file)
    {
        CachedMode c;
        if (auto xml = juce::parseXML (file); xml != nullptr && xml->getTagName() == "RoneLicenseMode")
        {
            c.mode           = xml->getStringAttribute ("mode", "enforced");
            c.message        = xml->getStringAttribute ("message", juce::String());
            c.signedLicences = xml->getStringAttribute ("signed", juce::String());
            c.fetchedAt      = xml->getStringAttribute ("fetchedAt", "0").getLargeIntValue();
            c.attemptedAt    = xml->getStringAttribute ("attemptedAt", "0").getLargeIntValue();
        }
        return c;
    }

    // attemptedAt is written only by a plugin recording a try; the Center,
    // the Analyzer and older plugins write the file without it (an older
    // build that stamps fetchedAt before fetching just suppresses ours).
    static bool writeModeTo (const juce::File& file, const juce::String& mode, const juce::String& message,
                             const juce::String& signedLicences, juce::int64 fetchedAt, juce::int64 attemptedAt)
    {
        file.getParentDirectory().createDirectory();

        juce::XmlElement xml ("RoneLicenseMode");
        xml.setAttribute ("mode", mode.isNotEmpty() ? mode : "enforced");
        xml.setAttribute ("message", message);
        if (signedLicences.isNotEmpty())
            xml.setAttribute ("signed", signedLicences);
        xml.setAttribute ("fetchedAt", juce::String (fetchedAt));
        if (attemptedAt > 0)
            xml.setAttribute ("attemptedAt", juce::String (attemptedAt));
        return xml.writeTo (file, {});
    }

    static bool isCacheStale()
    {
        if (! getModeFile().existsAsFile())
            return true;

        const auto c = readModeFile (getModeFile());
        return isStale (c.fetchedAt, c.attemptedAt, juce::Time::currentTimeMillis());
    }

    // Due when the last manifest is over 6 h old (or stamped in the future:
    // the clock moved back) - unless a try was recorded in the last 10
    // minutes. A try stamped in the future does not hold anything back.
    static bool isStale (juce::int64 fetchedAt, juce::int64 attemptedAt, juce::int64 now)
    {
        const auto fetchedAge = now - fetchedAt;
        if (fetchedAge >= 0 && fetchedAge <= (juce::int64) (kRefreshIntervalHours * 3600.0 * 1000.0))
            return false;

        const auto triedAge = now - attemptedAt;
        return attemptedAt <= 0 || triedAge < 0 || triedAge > kRetryAfterFailureMs;
    }

    static void observeOnceIn (const juce::File& reportsRoot, const juce::String& code,
                               const juce::String& message, const juce::String& details)
    {
        const auto stamp = reportsRoot.getChildFile ("throttle").getChildFile (code + ".observed");
        if (stamp.existsAsFile()
            && juce::Time::getCurrentTime() - stamp.getLastModificationTime() < juce::RelativeTime::days (30))
            return;
        if (! stamp.getParentDirectory().createDirectory() || ! stamp.replaceWithText ("1"))
            return;   // cannot throttle -> do not report at all

        // The id RoneCrashReporter::getInstallId() keeps; created the same way.
        auto idFile = reportsRoot.getChildFile ("install-id.txt");
        auto installId = idFile.existsAsFile() ? idFile.loadFileAsString().trim() : juce::String();
        if (installId.length() != 36)
        {
            installId = juce::Uuid().toDashedString();
            idFile.replaceWithText (installId);
        }

       #if defined (JucePlugin_Name) && defined (JucePlugin_VersionString)
        const juce::String product (JucePlugin_Name), version (JucePlugin_VersionString);
       #else
        const juce::String product ("RONE"), version;
       #endif

        auto* obj = new juce::DynamicObject();
        obj->setProperty ("type",       "error");
        obj->setProperty ("product",    product);
        obj->setProperty ("version",    version);
        obj->setProperty ("wrapper",    "Plugin");
        obj->setProperty ("code",       code);
        obj->setProperty ("message",    message.substring (0, 2000));
        obj->setProperty ("details",    details.substring (0, 8000));
        obj->setProperty ("stack",      juce::String());
        obj->setProperty ("os",         juce::SystemStats::getOperatingSystemName());
       #if JUCE_ARM
        obj->setProperty ("arch",       "arm64");
       #else
        obj->setProperty ("arch",       juce::SystemStats::isOperatingSystem64Bit() ? "x64" : "x86");
       #endif
        obj->setProperty ("locale",     juce::SystemStats::getUserLanguage() + "-" + juce::SystemStats::getUserRegion());
        obj->setProperty ("install_id", installId);
        obj->setProperty ("time",       juce::Time::getCurrentTime().toISO8601 (true));

        const auto queue = reportsRoot.getChildFile ("pending");
        queue.createDirectory();
        queue.getChildFile (juce::String (juce::Time::currentTimeMillis())
                              + "-" + juce::String (juce::Random::getSystemRandom().nextInt (0x7fffffff)) + ".json")
             .replaceWithText (juce::JSON::toString (juce::var (obj)));
    }

    RemoteLicenseGate() = delete; // static-only
};
