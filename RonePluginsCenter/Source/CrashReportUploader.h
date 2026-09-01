#pragma once

// ============================================================================
// CrashReportUploader — the Center is the bundle's single crash-report
// uploader. It drains the shared queue that every RONE product writes into
// (see Shared/RoneCrashReporter.h) and files each report as an issue in the
// private tracker  github.com/liranidan2000-max/rone-crash-reports.
//
// The upload token comes in at build time (CMake option RONE_CRASH_TOKEN,
// injected by CI from the CRASH_REPORT_TOKEN secret). It is a fine-grained
// token that can ONLY read/write issues on that one private repo — if it is
// empty (local builds, secret not configured) uploading is silently disabled
// and reports simply stay queued on disk until a token-carrying Center runs.
// ============================================================================

#include <juce_core/juce_core.h>
#include "../../Shared/RoneCrashReporter.h"

#ifndef RONE_CRASH_TOKEN
 #define RONE_CRASH_TOKEN ""
#endif

namespace CrashReportUploader
{

inline constexpr const char* kIssuesApiUrl =
    "https://api.github.com/repos/liranidan2000-max/rone-crash-reports/issues";

inline bool uploadOne (const juce::File& reportFile)
{
    auto parsed = juce::JSON::parse (reportFile.loadFileAsString());
    if (! parsed.isObject())
        return true;    // unreadable/corrupt file — drop it

    auto s = [&parsed] (const char* k) { return parsed.getProperty (k, "?").toString(); };

    auto title = "[" + s ("product") + " " + s ("version") + "] "
                 + s ("code") + " - " + s ("message").substring (0, 80);

    juce::String body;
    body << "**" << s ("type") << "** in **" << s ("product") << " " << s ("version")
         << "** (" << s ("wrapper") << ")\n\n"
         << "- OS: " << s ("os") << " (" << s ("arch") << ", " << s ("locale") << ")\n"
         << "- Machine: `" << s ("install_id") << "`\n"
         << "- Time: " << s ("time") << "\n\n"
         << "**Message:** " << s ("message") << "\n\n";
    if (s ("details").isNotEmpty() && s ("details") != "?")
        body << "**Details:**\n```\n" << s ("details") << "\n```\n\n";
    if (s ("stack").isNotEmpty() && s ("stack") != "?")
        body << "**Stack:**\n```\n" << s ("stack") << "\n```\n";

    auto* payload = new juce::DynamicObject();
    payload->setProperty ("title", title);
    payload->setProperty ("body",  body);

    juce::URL url { kIssuesApiUrl };
    juce::WebInputStream stream (url.withPOSTData (juce::JSON::toString (juce::var (payload))), true);
    stream.withExtraHeaders ("Authorization: Bearer " + juce::String (RONE_CRASH_TOKEN)
                             + "\r\nAccept: application/vnd.github+json"
                             + "\r\nContent-Type: application/json"
                             + "\r\nUser-Agent: RonePluginsCenter");
    stream.withConnectionTimeout (10000);

    if (! stream.connect (nullptr))
        return false;                          // offline — keep the file, retry later

    auto status = stream.getStatusCode();
    if (status == 201)
        return true;                           // filed — delete the file
    if (status == 401 || status == 403 || status == 404 || status == 410 || status == 422)
    {
        // Token revoked/misconfigured or repo missing — don't burn the queue,
        // but don't retry this session either.
        return false;
    }
    return false;
}

// Drains the queue on a background thread. Throttled: at most one pass per
// minute, at most 20 reports per pass (the rest go next pass).
inline void uploadPendingAsync()
{
    if (juce::String (RONE_CRASH_TOKEN).isEmpty())
        return;

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
            if (uploadOne (f))
            {
                f.deleteFile();
                ++sent;
            }
            else
                break;                          // network/token trouble — stop the pass
        }
        running = false;
    });
}

} // namespace CrashReportUploader
