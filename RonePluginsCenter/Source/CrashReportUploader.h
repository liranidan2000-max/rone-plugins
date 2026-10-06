#pragma once

// ============================================================================
// CrashReportUploader — the Center is the bundle's single crash-report
// uploader. It drains the shared queue that every RONE product writes into
// (see Shared/RoneCrashReporter.h) and hands each report to roneaudio.com,
// which files it as an issue in the private tracker
// github.com/liranidan2000-max/rone-crash-reports.
//
// Until Center 2.0 the GitHub token itself was compiled into the exe (CMake
// RONE_CRASH_TOKEN), and anything compiled in can be pulled out: whoever did
// could read every report - machine ids, file paths, stack traces - and flood
// the tracker. Now the token lives only on the server (the website Worker's
// CRASH_REPORT_TOKEN secret, worker/api/v1/crash.js), which also checks and
// rate-limits what it files. Reports wait on disk until the server takes them.
// ============================================================================

#include <juce_core/juce_core.h>
#include "../../Shared/RoneCrashReporter.h"

#ifndef RONE_API_BASE
 #define RONE_API_BASE "https://roneaudio.com/api/v1"
#endif

namespace CrashReportUploader
{

enum class Outcome { sent, drop, stop };

inline Outcome uploadOne (const juce::File& reportFile)
{
    auto parsed = juce::JSON::parse (reportFile.loadFileAsString());
    if (! parsed.isObject())
        return Outcome::drop;    // unreadable/corrupt file — drop it

    juce::URL url { juce::String (RONE_API_BASE) + "/crash" };
    juce::WebInputStream stream (url.withPOSTData (juce::JSON::toString (parsed)), true);
    stream.withExtraHeaders ("Content-Type: application/json\r\nAccept: application/json\r\nUser-Agent: RonePluginsCenter");
    stream.withConnectionTimeout (10000);

    if (! stream.connect (nullptr))
        return Outcome::stop;                  // offline — keep the file, retry later

    const auto status = stream.getStatusCode();
    if (status == 200 || status == 201)
        return Outcome::sent;                  // filed — delete the file
    if (status == 400 || status == 413 || status == 422)
        return Outcome::drop;                  // the server will never take this one
    return Outcome::stop;                      // 429 / 5xx / not deployed yet: keep everything, try next pass
}

// Drains the queue on a background thread. Throttled: at most one pass per
// minute, at most 20 reports per pass (the rest go next pass).
inline void uploadPendingAsync()
{
    static std::atomic<bool> running { false };
    static std::atomic<juce::int64> lastRunMs { 0 };

    auto now = juce::Time::currentTimeMillis();
    if (running.exchange (true))
        return;
    if (now - lastRunMs.load() < 60'000 && lastRunMs.load() != 0)
    {
        running = false;
        return;
    }
    lastRunMs = now;

    juce::Thread::launch ([]
    {
        auto files = RoneCrashReporter::getQueueDir()
                         .findChildFiles (juce::File::findFiles, false, "*.json");
        int sent = 0;
        for (auto& f : files)
        {
            if (sent >= 20)
                break;
            const auto outcome = uploadOne (f);
            if (outcome == Outcome::stop)
                break;                          // network/server trouble — stop the pass
            f.deleteFile();
            if (outcome == Outcome::sent)
                ++sent;
        }
        running = false;
    });
}

} // namespace CrashReportUploader
