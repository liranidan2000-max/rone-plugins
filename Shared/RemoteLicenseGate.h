#pragma once

// juce_core only — so this header also compiles in plugins that include
// JUCE modules directly instead of a generated JuceHeader.h (e.g. Flanger).
#include <juce_core/juce_core.h>

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
//                  || BundleLicenseChecker::isBundleLicensed();
//
// NOTE: RoneStutter carries its own copy (RoneStutter/Source/
// RemoteLicenseGate.h) because its repo builds without Shared/ —
// keep the two files in sync when editing this one.
// ============================================================================

class RemoteLicenseGate
{
public:
    static constexpr double kRefreshIntervalHours = 6.0;

    // ---- Fast cache read (file only, no network). Message thread. ----------
    // True = the bundle is in open mode and no license is needed right now.
    static bool isOpenMode()
    {
        auto xml = juce::parseXML (getModeFile());
        if (xml == nullptr || xml->getTagName() != "RoneLicenseMode")
            return false; // no data yet -> enforced (fail closed; the Center writes the cache on its first run)

        return ! xml->getStringAttribute ("mode", "enforced")
                     .equalsIgnoreCase ("enforced");
    }

    // Optional operator message shown when the bundle is locked ("" if none).
    static juce::String getLockMessage()
    {
        auto xml = juce::parseXML (getModeFile());
        return xml != nullptr ? xml->getStringAttribute ("message", juce::String())
                              : juce::String();
    }

    // ---- Background refresh (throttled fire-and-forget). -------------------
    // Call from constructors / prepareToPlay / editor timers. Never blocks:
    // the fetch runs on a disposable thread and only rewrites the cache file.
    static void refreshFromNetworkAsync()
    {
        if (! isCacheStale())
            return;

        // Stamp first so a wall of plugin instances loading together doesn't
        // fire a burst of identical requests.
        touchFetchStamp();

        juce::Thread::launch ([]
        {
            juce::URL url (kManifestUrl);
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
            if (! parsed.isObject())
                return; // garbage (captive portal etc.) — keep cache

            writeMode (parsed.getProperty ("license_mode",    "enforced").toString(),
                       parsed.getProperty ("license_message", ""    ).toString());
        });
    }

    // ---- Cache writer — also called by the Plugins Center, which already ---
    // fetches the manifest on every launch/refresh.
    static void writeMode (const juce::String& mode, const juce::String& message)
    {
        auto file = getModeFile();
        file.getParentDirectory().createDirectory();

        juce::XmlElement xml ("RoneLicenseMode");
        xml.setAttribute ("mode", mode.isNotEmpty() ? mode : "enforced");
        xml.setAttribute ("message", message);
        xml.setAttribute ("fetchedAt", juce::String (juce::Time::currentTimeMillis()));
        xml.writeTo (file, {});
    }

    static juce::File getModeFile()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("RonePlugins")
                   .getChildFile ("LicenseMode.xml");
    }

private:
    static constexpr const char* kManifestUrl =
        "https://raw.githubusercontent.com/liranidan2000-max/rone-plugins/main/versions.json";

    static bool isCacheStale()
    {
        auto xml = juce::parseXML (getModeFile());
        if (xml == nullptr)
            return true;

        auto last  = xml->getStringAttribute ("fetchedAt", "0").getLargeIntValue();
        auto ageMs = juce::Time::currentTimeMillis() - last;
        return ageMs > (juce::int64) (kRefreshIntervalHours * 3600.0 * 1000.0)
            || ageMs < 0; // clock moved backwards — refetch
    }

    // Rewrite the cache with the same mode but a fresh timestamp.
    static void touchFetchStamp()
    {
        juce::String mode = "enforced", msg;
        if (auto xml = juce::parseXML (getModeFile());
            xml != nullptr && xml->getTagName() == "RoneLicenseMode")
        {
            mode = xml->getStringAttribute ("mode", "enforced");
            msg  = xml->getStringAttribute ("message", juce::String());
        }
        writeMode (mode, msg);
    }

    RemoteLicenseGate() = delete; // static-only
};
