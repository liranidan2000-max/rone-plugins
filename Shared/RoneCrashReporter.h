#pragma once

// ============================================================================
// RoneCrashReporter — bundle-wide crash & error capture (queue side).
//
// Every RONE product writes crash/error reports as small JSON files into a
// shared local queue:
//     <userAppData>/RonePlugins/CrashReports/pending/*.json
//
// Nothing here talks to the network and nothing here holds any secret — the
// RONE Plugins Center is the single uploader (see the Center's
// CrashReportUploader.h): whenever it runs it drains this queue into the
// private GitHub issue tracker (rone-crash-reports). Plugins and standalones
// only ever WRITE files, so a report queued while offline (or from a machine
// that never opened the Center yet) is simply picked up later.
//
// Usage:
//   - Standalone apps / the Center, once at startup:
//         RoneCrashReporter::installCrashHandler ("RONE AFTERSPACE",
//                                                 JucePlugin_VersionString,
//                                                 "Standalone");
//     (Never install the crash handler from a plugin running inside a DAW —
//      the process and its crash handling belong to the host.)
//   - Anywhere, for non-fatal problems worth telling the devs about:
//         RoneCrashReporter::reportError ("RONE Plugins Center", version,
//                                         "Center", "INSTALL_FAILED", msg);
// ============================================================================

#include <juce_core/juce_core.h>

#if JUCE_MAC && JUCE_INTEL
 // For runningArch(). Declared here rather than through <sys/sysctl.h>, which
 // would bring <sys/param.h> (MIN, MAX, isset, roundup... macros) into every
 // plugin processor that includes this header.
 extern "C" int sysctlbyname (const char*, void*, size_t*, void*, size_t);
#endif

namespace RoneCrashReporter
{

// The code that is running, not the OS: "x64" (every Windows report has
// always said so), "arm64" (Apple Silicon, native slice), or "x64-rosetta"
// (the Intel slice of the universal build translated on Apple Silicon, e.g.
// inside an Intel-only DAW). The site only prints it (audit R293).
inline juce::String runningArch()
{
   #if JUCE_ARM && JUCE_64BIT
    return "arm64";
   #elif JUCE_ARM
    return "arm";
   #else
    juce::String arch (juce::SystemStats::isOperatingSystem64Bit() ? "x64" : "x86");
    #if JUCE_MAC
     int translated = 0;
     size_t size = sizeof (translated);
     if (sysctlbyname ("sysctl.proc_translated", &translated, &size, nullptr, 0) == 0 && translated == 1)
         arch << "-rosetta";
    #endif
    return arch;
   #endif
}

inline juce::File getReportsRoot()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("RonePlugins")
        .getChildFile ("CrashReports");
}

inline juce::File getQueueDir()
{
    auto dir = getReportsRoot().getChildFile ("pending");
    dir.createDirectory();
    return dir;
}

// Anonymous, per-machine random id so repeated reports can be grouped
// ("same crash on 3 machines vs 3 times on one") — carries no personal data.
inline juce::String getInstallId()
{
    auto f = getReportsRoot().getChildFile ("install-id.txt");
    auto id = f.existsAsFile() ? f.loadFileAsString().trim() : juce::String();
    if (id.length() != 36)
    {
        id = juce::Uuid().toDashedString();
        f.getParentDirectory().createDirectory();
        f.replaceWithText (id);
    }
    return id;
}

inline void writeReport (const juce::String& type,     // "crash" | "error"
                         const juce::String& product,
                         const juce::String& version,
                         const juce::String& wrapper,  // "Standalone" | "Center" | "VST3"
                         const juce::String& code,
                         const juce::String& message,
                         const juce::String& details,
                         const juce::String& stack)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("type",       type);
    obj->setProperty ("product",    product);
    obj->setProperty ("version",    version);
    obj->setProperty ("wrapper",    wrapper);
    obj->setProperty ("code",       code);
    obj->setProperty ("message",    message.substring (0, 2000));
    obj->setProperty ("details",    details.substring (0, 8000));
    obj->setProperty ("stack",      stack.substring (0, 16000));
    obj->setProperty ("os",         juce::SystemStats::getOperatingSystemName());
    obj->setProperty ("arch",       runningArch());
    obj->setProperty ("locale",     juce::SystemStats::getUserLanguage() + "-" + juce::SystemStats::getUserRegion());
    obj->setProperty ("install_id", getInstallId());
    obj->setProperty ("time",       juce::Time::getCurrentTime().toISO8601 (true));

    auto name = juce::String (juce::Time::currentTimeMillis())
                + "-" + juce::String (juce::Random::getSystemRandom().nextInt (0x7fffffff))
                + ".json";
    getQueueDir().getChildFile (name)
        .replaceWithText (juce::JSON::toString (juce::var (obj)));
}

// Throttle repeated identical errors (an offline machine retrying every
// launch must not queue hundreds of copies): same code+message at most once
// per 12 hours.
inline void reportError (const juce::String& product,
                         const juce::String& version,
                         const juce::String& wrapper,
                         const juce::String& code,
                         const juce::String& message,
                         const juce::String& details = {})
{
    auto key   = juce::String::toHexString ((code + "|" + message).hashCode64());
    auto stamp = getReportsRoot().getChildFile ("throttle").getChildFile (key + ".t");
    if (stamp.existsAsFile()
        && juce::Time::getCurrentTime() - stamp.getLastModificationTime() < juce::RelativeTime::hours (12))
        return;
    stamp.getParentDirectory().createDirectory();
    stamp.replaceWithText ("1");

    writeReport ("error", product, version, wrapper, code, message, details, {});
}

namespace detail
{
    // Captured at install time so the crash handler itself does as little as
    // possible beyond grabbing the stack and writing the file.
    inline juce::String& crashProduct() { static juce::String s; return s; }
    inline juce::String& crashVersion() { static juce::String s; return s; }
    inline juce::String& crashWrapper() { static juce::String s; return s; }

    inline void crashHandler (void*)
    {
        static bool alreadyReported = false;
        if (alreadyReported) return;
        alreadyReported = true;

        writeReport ("crash", crashProduct(), crashVersion(), crashWrapper(),
                     "CRASH", "The application crashed", {},
                     juce::SystemStats::getStackBacktrace());
    }
}

// Call ONLY from processes we own (standalone apps and the Center) — never
// from a plugin loaded inside a DAW.
inline void installCrashHandler (const juce::String& product,
                                 const juce::String& version,
                                 const juce::String& wrapper)
{
    static bool installed = false;
    if (installed) return;
    installed = true;

    detail::crashProduct() = product;
    detail::crashVersion() = version;
    detail::crashWrapper() = wrapper;
    getQueueDir();     // make sure the directory exists before any crash
    getInstallId();

    juce::SystemStats::setApplicationCrashHandler (detail::crashHandler);
}

} // namespace RoneCrashReporter
