#include "NetworkManager.h"
#include "../../Shared/RemoteLicenseGate.h"

NetworkManager::NetworkManager()
    : Thread ("RONE-Network")
{
}

NetworkManager::~NetworkManager()
{
    cancelDownload();
    stopThread (5000);
}

// ============================================================================
// Public API
// ============================================================================

void NetworkManager::fetchManifest()
{
    cancelDownload();
    currentTask = FetchManifest;
    targetUrl   = VERSIONS_JSON_URL;
    startThread();
}

void NetworkManager::downloadInstaller (const juce::String& pluginId,
                                         const juce::String& url,
                                         const juce::String& sha256)
{
    // Queue the job. The network thread processes downloads one at a time, so
    // "Update All" (which fires this rapidly for every plugin) no longer has each
    // call cancel the previous one and leave it stuck at 0%.
    {
        const juce::ScopedLock sl (queueLock);

        // Skip duplicates already queued for this plugin.
        for (auto& j : downloadQueue)
            if (j.pluginId == pluginId)
                return;

        downloadQueue.add ({ pluginId, url, sha256 });
    }

    // If the thread is busy fetching the manifest, let it finish; otherwise start
    // the download worker.
    if (! isThreadRunning())
    {
        currentTask = DownloadFile;
        startThread();
    }
}

void NetworkManager::cancelDownload()
{
    {
        const juce::ScopedLock sl (queueLock);
        downloadQueue.clear();
    }

    if (isThreadRunning())
    {
        signalThreadShouldExit();
        stopThread (3000);
    }
    currentTask = None;
}

// ============================================================================
// Background thread
// ============================================================================

void NetworkManager::run()
{
    switch (currentTask)
    {
        case FetchManifest:
        {
            // --- Fetch the JSON manifest ---------------------------------
            juce::URL url (targetUrl);
            auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                               .withConnectionTimeoutMs (10000)
                               .withNumRedirectsToFollow (5);

            auto stream = url.createInputStream (options);

            if (stream == nullptr || threadShouldExit())
            {
                juce::MessageManager::callAsync ([this]
                {
                    listeners.call (&Listener::onManifestError,
                                    juce::String ("Network error: could not reach the update server."));
                });
                return;
            }

            auto body = stream->readEntireStreamAsString();

            if (body.isEmpty() || threadShouldExit())
            {
                juce::MessageManager::callAsync ([this]
                {
                    listeners.call (&Listener::onManifestError,
                                    juce::String ("Empty response from update server."));
                });
                return;
            }

            auto plugins = parseManifest (body);

            // Capture the manifest's own-update block for the Center
            {
                auto root = juce::JSON::parse (body);

                // Propagate the remote kill-switch (license_mode) to the
                // shared cache file that every RONE plugin reads.
                if (root.isObject())
                    RemoteLicenseGate::writeMode (
                        root.getProperty ("license_mode",    "enforced").toString(),
                        root.getProperty ("license_message", ""    ).toString());

                auto ci   = root.getProperty ("center_installer", {});
                const juce::ScopedLock sl (centerInfoLock);
                centerInfo.version = ci.getProperty ("version", {}).toString();
                centerInfo.url     = ci.getProperty ("url",     {}).toString();
                centerInfo.sha256  = ci.getProperty ("sha256",  {}).toString();
            }

            // If remote JSON was unparseable (e.g. 404 HTML from private repo),
            // fall back to a hardcoded catalog so the UI always shows plugins.
            if (plugins.isEmpty())
                plugins = getFallbackManifest();

            juce::MessageManager::callAsync ([this, plugins]
            {
                listeners.call (&Listener::onManifestReady, plugins);
            });
            break;
        }

        case DownloadFile:
        {
            // Drain the download queue one job at a time.
            for (;;)
            {
                if (threadShouldExit())
                    break;

                DownloadJob job;
                {
                    const juce::ScopedLock sl (queueLock);
                    if (downloadQueue.isEmpty())
                        break;
                    job = downloadQueue.removeAndReturn (0);
                }

                runDownloadJob (job);
            }
            break;
        }

        default:
            break;
    }
}

// ============================================================================
// Single download job (serial worker called from run())
// ============================================================================

void NetworkManager::runDownloadJob (const DownloadJob& job)
{
    // --- Download an installer to a temp file --------------------
    auto tempDir  = juce::File::getSpecialLocation (juce::File::tempDirectory)
                       .getChildFile ("RONE_Downloads");
    tempDir.createDirectory();

    // pluginId comes from the remote manifest — never let it shape a path
    const auto safeId = juce::File::createLegalFileName (job.pluginId);

#if JUCE_MAC
    auto tempFile = tempDir.getChildFile (safeId + "_Installer.pkg");
#else
    auto tempFile = tempDir.getChildFile (safeId + "_Installer.exe");
#endif
            if (tempFile.existsAsFile())
                tempFile.deleteFile();

        #if JUCE_MAC
            // On macOS, juce::URL with JUCE_USE_CURL=0 (CFNetwork) does not reliably
            // follow GitHub's cross-domain 302 redirect from github.com to the signed
            // release-assets.githubusercontent.com URL. Shell out to /usr/bin/curl
            // (preinstalled on every macOS) which handles redirects correctly.
            // HEAD request to learn the final Content-Length for progress reporting.
            juce::int64 totalBytes = 0;
            {
                juce::ChildProcess head;
                juce::StringArray headArgs { "/usr/bin/curl", "-sIL", job.url };
                if (head.start (headArgs))
                {
                    auto headers = head.readAllProcessOutput();
                    head.waitForProcessToFinish (5000);
                    for (auto& line : juce::StringArray::fromLines (headers))
                    {
                        auto trimmed = line.trim();
                        if (trimmed.startsWithIgnoreCase ("content-length:"))
                            totalBytes = trimmed.fromFirstOccurrenceOf (":", false, true)
                                                 .trim().getLargeIntValue();
                    }
                }
            }

            juce::StringArray dlArgs { "/usr/bin/curl",
                                       "-L",                  // follow redirects
                                       "-f",                  // fail on HTTP error
                                       "--silent",
                                       "--show-error",
                                       "-o", tempFile.getFullPathName(),
                                       job.url };

            juce::ChildProcess curl;
            if (! curl.start (dlArgs, juce::ChildProcess::wantStdErr))
            {
                auto pid = job.pluginId;
                juce::MessageManager::callAsync ([this, pid]
                {
                    listeners.call (&Listener::onDownloadComplete,
                                    pid, juce::File(), false,
                                    juce::String ("Failed to start download process."));
                });
                return;
            }

            auto lastProgressTime = juce::Time::getMillisecondCounterHiRes();
            while (curl.isRunning())
            {
                if (threadShouldExit())
                {
                    curl.kill();
                    tempFile.deleteFile();
                    return;
                }

                if (totalBytes > 0 && tempFile.existsAsFile())
                {
                    auto now = juce::Time::getMillisecondCounterHiRes();
                    if (now - lastProgressTime >= 100.0)
                    {
                        lastProgressTime = now;
                        double progress = juce::jlimit (0.0, 1.0,
                                            (double) tempFile.getSize() / (double) totalBytes);
                        auto pid = job.pluginId;
                        juce::MessageManager::callAsync ([this, pid, progress]
                        {
                            listeners.call (&Listener::onDownloadProgress, pid, progress);
                        });
                    }
                }

                juce::Thread::sleep (50);
            }

            auto curlStderr = curl.readAllProcessOutput();
            auto exitCode   = curl.getExitCode();

            if (exitCode != 0)
            {
                tempFile.deleteFile();
                auto pid = job.pluginId;
                auto err = curlStderr.trim().isNotEmpty()
                              ? juce::String ("Download failed - ") + curlStderr.trim()
                              : juce::String ("Download failed (curl exit ") + juce::String (exitCode) + ").";
                juce::MessageManager::callAsync ([this, pid, err]
                {
                    listeners.call (&Listener::onDownloadComplete,
                                    pid, juce::File(), false, err);
                });
                return;
            }
        #else
            juce::URL url (job.url);
            auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                               .withConnectionTimeoutMs (15000)
                               .withNumRedirectsToFollow (5);

            auto stream = url.createInputStream (options);

            if (stream == nullptr || threadShouldExit())
            {
                auto pid = job.pluginId;
                juce::MessageManager::callAsync ([this, pid]
                {
                    listeners.call (&Listener::onDownloadComplete,
                                    pid, juce::File(), false,
                                    juce::String ("Failed to connect to download server."));
                });
                return;
            }

            // Try to get content length for progress reporting
            auto totalBytes = stream->getTotalLength();
            juce::int64 downloaded = 0;

            juce::FileOutputStream output (tempFile);
            if (! output.openedOk())
            {
                auto pid = job.pluginId;
                juce::MessageManager::callAsync ([this, pid]
                {
                    listeners.call (&Listener::onDownloadComplete,
                                    pid, juce::File(), false,
                                    juce::String ("Could not create temp file for download."));
                });
                return;
            }

            constexpr int bufferSize = 32768;
            juce::HeapBlock<char> buffer (bufferSize);
            auto lastProgressTime = juce::Time::getMillisecondCounterHiRes();

            while (! threadShouldExit())
            {
                auto bytesRead = stream->read (buffer, bufferSize);
                if (bytesRead <= 0)
                    break;

                output.write (buffer, (size_t) bytesRead);
                downloaded += bytesRead;

                if (totalBytes > 0)
                {
                    auto now = juce::Time::getMillisecondCounterHiRes();
                    // Throttle progress events to ~10/sec to avoid flooding the message queue
                    if (now - lastProgressTime >= 100.0 || downloaded >= totalBytes)
                    {
                        lastProgressTime = now;
                        double progress = (double) downloaded / (double) totalBytes;
                        auto pid = job.pluginId;
                        juce::MessageManager::callAsync ([this, pid, progress]
                        {
                            listeners.call (&Listener::onDownloadProgress, pid, progress);
                        });
                    }
                }
            }

            output.flush();

            if (threadShouldExit())
            {
                tempFile.deleteFile();
                return;
            }
        #endif

            // Verify the downloaded file is a real installer, not an HTML error page.
            // GitHub returns small HTML 404 pages for missing/private release assets.
            static constexpr juce::int64 MIN_INSTALLER_SIZE = 1024 * 1024; // 1 MB

            bool fileExists = tempFile.existsAsFile();
            auto fileSize   = fileExists ? tempFile.getSize() : 0;
            bool success    = fileExists && fileSize >= MIN_INSTALLER_SIZE;

            // SHA256 verification — compare downloaded file hash against manifest
            if (success && job.sha256.isNotEmpty())
            {
                juce::FileInputStream fis (tempFile);
                if (fis.openedOk())
                {
                    juce::SHA256 hash (fis);
                    auto computed = hash.toHexString();

                    if (computed.compareIgnoreCase (job.sha256) != 0)
                    {
                        DBG ("[Download] SHA256 mismatch! Expected: " + job.sha256
                             + " Got: " + computed);
                        success = false;
                    }
                    else
                    {
                        DBG ("[Download] SHA256 verified OK: " + computed);
                    }
                }
            }

            auto pid  = job.pluginId;
            auto file = tempFile;

            juce::String errorMsg;
            if (! fileExists)
                errorMsg = "Download failed - file was not saved.";
            else if (fileSize < MIN_INSTALLER_SIZE)
                errorMsg = "Download failed - corrupt file (" + juce::String (fileSize / 1024)
                         + " KB). The download link may be invalid.";
            else if (! success)
                errorMsg = "Download failed - file integrity check failed (SHA256 mismatch).";

            juce::MessageManager::callAsync ([this, pid, file, success, errorMsg]
            {
                listeners.call (&Listener::onDownloadComplete,
                                pid, file, success, errorMsg);
            });
}

// ============================================================================
// JSON parsing
// ============================================================================

juce::Array<PluginInfo> NetworkManager::parseManifest (const juce::String& jsonBody)
{
    juce::Array<PluginInfo> result;

    auto root = juce::JSON::parse (jsonBody);
    if (! root.isObject())
        return result;

    auto* pluginsArray = root.getProperty ("plugins", {}).getArray();
    if (pluginsArray == nullptr)
        return result;

    for (auto& entry : *pluginsArray)
    {
        PluginInfo info;
        info.id            = entry.getProperty ("id",           {}).toString();
        info.name          = entry.getProperty ("name",         {}).toString();
        info.remoteVersion = entry.getProperty ("version",      {}).toString();
        info.description   = entry.getProperty ("description",  {}).toString();
        info.whatsNew      = entry.getProperty ("whats_new",    {}).toString();
        info.downloadUrl    = entry.getProperty ("download_url",     {}).toString();
        info.downloadUrlMac = entry.getProperty ("download_url_mac", {}).toString();
        info.sha256         = entry.getProperty ("sha256",           {}).toString();
        info.sha256Mac      = entry.getProperty ("sha256_mac",       {}).toString();
        info.standaloneExe  = entry.getProperty ("standalone_exe",   {}).toString();
        info.vst3Bundle     = entry.getProperty ("vst3_bundle",      {}).toString();
        info.auBundle       = entry.getProperty ("au_bundle",        {}).toString();
        info.manualPdf      = entry.getProperty ("manual",           {}).toString();
        info.registryKey    = entry.getProperty ("registry_key",     {}).toString();
        info.type          = entry.getProperty ("type",         {}).toString();

        // Only the plugins sold on their own carry these. Left as raw vars so a
        // manifest without them (an ALL-ACCESS-only plugin, or a cached older
        // catalog) stays distinguishable from one priced at nothing.
        info.price       = entry.getProperty ("price",        {});
        info.launchPrice = entry.getProperty ("launch_price", {});
        info.storeUrl    = entry.getProperty ("store_url",    {}).toString();

        auto* fmts = entry.getProperty ("formats", {}).getArray();
        if (fmts != nullptr)
            for (auto& f : *fmts)
                info.formats.add (f.toString());

        // Determine installed status
        info.installedVersion = VersionChecker::getInstalledVersion (info.registryKey);
        info.status = VersionChecker::determineStatus (info.installedVersion,
                                                        info.remoteVersion);

        // Fallback: if registry says not installed but the files exist on disk,
        // treat as installed (handles manual installs / first run after existing install)
        if (info.status == PluginStatus::NotInstalled)
        {
            bool found = VersionChecker::isStandaloneInstalled (info.standaloneExe)
                      || VersionChecker::isVst3Installed (info.vst3Bundle)
                      || VersionChecker::isAUInstalled (info.auBundle);
            if (found)
            {
                info.installedVersion = "?";
                info.status = PluginStatus::UpdateAvailable;  // can't compare → prompt update
            }
        }

        result.add (std::move (info));
    }

    return result;
}

// ============================================================================
// Hardcoded fallback — always shows the plugin catalog even if the remote
// versions.json is unreachable (private repo, no internet, etc.)
// ============================================================================

juce::Array<PluginInfo> NetworkManager::getFallbackManifest()
{
    juce::String json = R"({
  "plugins": [
    {
      "id": "ReverseReverb",
      "name": "RONE Reverse Reverb",
      "version": "1.0.0",
      "type": "plugin",
      "formats": ["VST3", "AU", "Standalone"],
      "description": "Tempo-synced reverse reverb for risers, vocal swells and seamless transitions - tail lengths from 1/32 to 8 bars, shaped with fades, filters and stereo width",
      "whats_new": "First public release",
      "standalone_exe": "RONE Reverse Reverb.exe",
      "vst3_bundle": "RONE Reverse Reverb.vst3",
      "au_bundle": "RONE Reverse Reverb.component",
      "download_url": "https://github.com/liranidan2000-max/rone-plugins/releases/download/ReverseReverb-latest/ReverseReverb_Installer.exe",
      "download_url_mac": "https://github.com/liranidan2000-max/rone-plugins/releases/download/ReverseReverb-latest/ReverseReverb_Installer.pkg",
      "sha256": "",
      "registry_key": "ReverseReverb"
    },
    {
      "id": "RoneStemsFixer",
      "name": "RONE Stems Fixer",
      "version": "1.0.0",
      "type": "standalone",
      "formats": ["Standalone"],
      "description": "Drop in a folder of stems and it listens: identifies kick, bass, vocals, drums, atmos and FX, then renames the whole batch into a clean, consistent session",
      "whats_new": "First public release",
      "standalone_exe": "RONE Stems Fixer.exe",
      "vst3_bundle": "",
      "au_bundle": "",
      "download_url": "https://github.com/liranidan2000-max/rone-plugins/releases/download/RoneStemsFixer-latest/RoneStemsFixer_Installer.exe",
      "download_url_mac": "https://github.com/liranidan2000-max/rone-plugins/releases/download/RoneStemsFixer-latest/RoneStemsFixer_Installer.pkg",
      "sha256": "",
      "registry_key": "RoneStemsFixer"
    },
    {
      "id": "RoneStutter",
      "name": "RONE Stutter",
      "version": "1.0.0",
      "type": "plugin",
      "formats": ["VST3", "AU", "Standalone"],
      "description": "Tempo-locked stutter engine - beat divisions, sculpted fade curves and stereo movement for instant fills, edits and glitch builds",
      "whats_new": "First public release",
      "standalone_exe": "RONE Stutter.exe",
      "vst3_bundle": "RONE Stutter.vst3",
      "au_bundle": "RONE Stutter.component",
      "download_url": "https://github.com/liranidan2000-max/rone-plugins/releases/download/RoneStutter-latest/RoneStutter_Installer.exe",
      "download_url_mac": "https://github.com/liranidan2000-max/rone-plugins/releases/download/RoneStutter-latest/RoneStutter_Installer.pkg",
      "sha256": "",
      "registry_key": "RoneStutter"
    },
    {
      "id": "RoneFlanger",
      "name": "RONE Flanger",
      "version": "1.0.0",
      "type": "plugin",
      "formats": ["VST3", "Standalone"],
      "description": "A hands-on flanger you play like an instrument - manual delay control, stereo offset and a rhythmic gate, from subtle metallic motion to full jet sweeps",
      "whats_new": "First public release",
      "standalone_exe": "RONE Flanger.exe",
      "vst3_bundle": "RONE Flanger.vst3",
      "au_bundle": "",
      "download_url": "https://github.com/liranidan2000-max/rone-plugins/releases/download/RoneFlanger-latest/RoneFlanger_Installer.exe",
      "download_url_mac": "https://github.com/liranidan2000-max/rone-plugins/releases/download/RoneFlanger-latest/RoneFlanger_Installer.pkg",
      "sha256": "",
      "registry_key": "RoneFlanger"
    },
    {
      "id": "RoneSyncVerb",
      "name": "RONE Sync Verb",
      "version": "1.0.0",
      "type": "plugin",
      "formats": ["VST3", "AU", "Standalone"],
      "description": "Reverb that lives on the grid - tail lengths in note values that cut exactly on time, with built-in ducking so the space never swallows the mix",
      "whats_new": "First public release",
      "standalone_exe": "RONE Sync Verb.exe",
      "vst3_bundle": "RONE Sync Verb.vst3",
      "au_bundle": "RONE Sync Verb.component",
      "download_url": "https://github.com/liranidan2000-max/rone-plugins/releases/download/RoneSyncVerb-latest/RoneSyncVerb_Installer.exe",
      "download_url_mac": "https://github.com/liranidan2000-max/rone-plugins/releases/download/RoneSyncVerb-latest/RoneSyncVerb_Installer.pkg",
      "sha256": "",
      "registry_key": "RoneSyncVerb"
    },
    {
      "id": "RONEAnalyzer",
      "name": "RONE Analyzer",
      "version": "1.0.0",
      "type": "standalone",
      "formats": ["Standalone"],
      "description": "Professional measurement suite: 30-band spectrum with reference-track comparison, vectorscope, level meters, EBU R128 loudness and bit statistics",
      "whats_new": "First public release",
      "standalone_exe": "RONE Analyzer.exe",
      "vst3_bundle": "",
      "au_bundle": "",
      "download_url": "https://github.com/liranidan2000-max/rone-plugins/releases/download/RONEAnalyzer-latest/RONEAnalyzer_Installer.exe",
      "download_url_mac": "https://github.com/liranidan2000-max/rone-plugins/releases/download/RONEAnalyzer-latest/RONEAnalyzer_Installer.pkg",
      "sha256": "",
      "registry_key": "RONEAnalyzer"
    }
  ]
})";

    return parseManifest (json);
}
