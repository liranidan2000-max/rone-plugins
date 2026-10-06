#include "NetworkManager.h"
#include "../../Shared/RemoteLicenseGate.h"

NetworkManager::NetworkManager()
{
    manifestWorker.startThread();
    downloadWorker.startThread();
}

NetworkManager::~NetworkManager()
{
    alive->store (false);

    cancelCurrent = true;
    manifestWorker.signalThreadShouldExit();
    downloadWorker.signalThreadShouldExit();
    manifestWorker.wake.signal();
    downloadWorker.wake.signal();
    manifestWorker.stopThread (5000);
    downloadWorker.stopThread (5000);
}

void NetworkManager::post (std::function<void()> fn)
{
    juce::MessageManager::callAsync ([flag = alive, fn = std::move (fn)]
    {
        if (flag->load())
            fn();
    });
}

juce::File NetworkManager::getDownloadDir()
{
    return juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("RONE_Downloads");
}

juce::File NetworkManager::installerFileFor (const juce::String& pluginId)
{
    // pluginId comes from the remote manifest — never let it shape a path
    const auto safeId = juce::File::createLegalFileName (pluginId);
   #if JUCE_MAC
    return getDownloadDir().getChildFile (safeId + (pluginId == "__center__" ? "_Installer.dmg" : "_Installer.pkg"));
   #else
    return getDownloadDir().getChildFile (safeId + "_Installer.exe");
   #endif
}

juce::File NetworkManager::getManifestCacheFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("RonePluginsCenter").getChildFile ("manifest-cache.json");
}

// ============================================================================
// Manifest
// ============================================================================

void NetworkManager::fetchManifest (bool freshFromOrigin)
{
    {
        const juce::ScopedLock sl (requestLock);
        manifestRequested = true;
        manifestFromOrigin = manifestFromOrigin || freshFromOrigin;
    }
    manifestWorker.wake.signal();
}

void NetworkManager::ManifestWorker::run()
{
    while (! threadShouldExit())
    {
        bool origin = false, requested = false;
        {
            const juce::ScopedLock sl (owner.requestLock);
            requested = owner.manifestRequested;
            origin = owner.manifestFromOrigin;
            owner.manifestRequested = false;
            owner.manifestFromOrigin = false;
        }

        if (requested)
            owner.fetchOnce (origin);
        else
            wake.wait (-1);
    }
}

void NetworkManager::fetchOnce (bool fromOrigin)
{
    // raw.githubusercontent serves this with Cache-Control: max-age=300 through a
    // CDN, so for up to five minutes after a release a POP can still hand out the
    // PREVIOUS manifest. The installers, meanwhile, live behind moving "-latest"
    // release tags and change the instant CI publishes. Stale hash + fresh file =
    // "file integrity check failed (SHA256 mismatch)" for anyone who presses
    // Update in that window.
    //
    // A "?t=<now>" query string does NOT get past that cache: the CDN keys on
    // the path alone, and a never-seen query string still came back X-Cache:
    // HIT (measured 2026-09-11, when the Analyzer update looped on it). The
    // cached copy is fine for browsing; when a hash has just failed, the file
    // is read from origin through the API instead, which is never cached but
    // is rate-limited - so only then.
    juce::URL url (fromOrigin ? juce::String (VERSIONS_JSON_ORIGIN_URL) : juce::String (VERSIONS_JSON_URL));

    // The API hands back the file itself only when asked for it this way; its
    // default is a JSON envelope with the content in base64.
    const auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                             .withConnectionTimeoutMs (10000)
                             .withNumRedirectsToFollow (5)
                             .withExtraHeaders (fromOrigin ? "Accept: application/vnd.github.raw\r\n"
                                                             "X-GitHub-Api-Version: 2022-11-28\r\n"
                                                           : "");

    juce::String body;
    if (auto stream = url.createInputStream (options))
        body = stream->readEntireStreamAsString();

    if (manifestWorker.threadShouldExit())
        return;

    // A real manifest has a "plugins" array. A captive portal's HTML page, an
    // API rate-limit message (valid JSON!) and an empty body all do not - and
    // none of them may reach the licence mode or the cache.
    const auto root = juce::JSON::parse (body);
    const bool isManifest = root.isObject() && root.getProperty ("plugins", {}).isArray();
    auto plugins = isManifest ? parseManifest (body) : juce::Array<PluginInfo>();

    if (! plugins.isEmpty())
    {
        // Propagate the remote kill-switch (license_mode) to the shared cache
        // file every RONE plugin reads, and every product's version for the
        // plugins' own "version X is available" bar (Shared/RoneUpdatePrompt.h).
        RemoteLicenseGate::writeMode (root.getProperty ("license_mode", "enforced").toString(),
                                      root.getProperty ("license_message", "").toString(),
                                      root.getProperty ("signed_licences", "").toString());
        RemoteLicenseGate::writeLatestVersions (root);
        captureManifestWide (root);

        auto cache = getManifestCacheFile();
        cache.getParentDirectory().createDirectory();
        cache.replaceWithText (body);

        lastSuccessMs = juce::Time::currentTimeMillis();
        post ([this, plugins] { listeners.call (&Listener::onManifestReady, plugins, false); });
        return;
    }

    // The origin read exists to verify a download: a cached catalog proves nothing there.
    if (fromOrigin)
    {
        post ([this] { listeners.call (&Listener::onManifestError,
                                       juce::String ("Could not read a fresh manifest from the update server.")); });
        return;
    }

    // Offline (or a portal in the way): the last manifest that did arrive.
    const auto cache = getManifestCacheFile();
    if (cache.existsAsFile())
    {
        const auto cachedBody = cache.loadFileAsString();
        auto cached = parseManifest (cachedBody);
        if (! cached.isEmpty())
        {
            captureManifestWide (juce::JSON::parse (cachedBody));
            post ([this, cached] { listeners.call (&Listener::onManifestReady, cached, true); });
            return;
        }
    }

    post ([this] { listeners.call (&Listener::onManifestError,
                                   juce::String ("Could not reach the update server.")); });
}

void NetworkManager::captureManifestWide (const juce::var& root)
{
    const auto ci = root.getProperty ("center_installer", {});

    auto* x = new juce::DynamicObject();
    x->setProperty ("tips", root.getProperty ("tips", juce::var (juce::Array<juce::var>())));

    const juce::ScopedLock sl (centerInfoLock);
    centerInfo.version   = ci.getProperty ("version",    {}).toString();
    centerInfo.url       = ci.getProperty ("url",        {}).toString();
    centerInfo.sha256    = ci.getProperty ("sha256",     {}).toString();
    centerInfo.urlMac    = ci.getProperty ("url_mac",    {}).toString();
    centerInfo.sha256Mac = ci.getProperty ("sha256_mac", {}).toString();
    extras = juce::var (x);
}

// ============================================================================
// Downloads
// ============================================================================

void NetworkManager::downloadInstaller (const juce::String& pluginId,
                                         const juce::String& url,
                                         const juce::String& sha256,
                                         bool prefetch)
{
    {
        const juce::ScopedLock sl (queueLock);

        if (currentJobId == pluginId)
        {
            if (! prefetch) currentIsPrefetch = false;   // the user wants it now: it installs when done
            return;
        }

        for (auto& j : downloadQueue)
            if (j.pluginId == pluginId)
            {
                if (! prefetch) j.prefetch = false;
                return;
            }

        DownloadJob job { pluginId, url, sha256, prefetch };

        // What the user asked for goes before what the Center fetches on its own.
        if (prefetch)
            downloadQueue.add (job);
        else
        {
            int at = 0;
            while (at < downloadQueue.size() && ! downloadQueue.getReference (at).prefetch)
                ++at;
            downloadQueue.insert (at, job);
        }
    }

    downloadWorker.wake.signal();
}

void NetworkManager::cancelDownload (const juce::String& pluginId)
{
    const juce::ScopedLock sl (queueLock);
    for (int i = downloadQueue.size(); --i >= 0;)
        if (downloadQueue.getReference (i).pluginId == pluginId)
            downloadQueue.remove (i);

    if (currentJobId == pluginId)
        cancelCurrent = true;
}

void NetworkManager::cancelAllDownloads()
{
    const juce::ScopedLock sl (queueLock);
    downloadQueue.clear();
    if (currentJobId.isNotEmpty())
        cancelCurrent = true;
}

bool NetworkManager::isDownloadQueued (const juce::String& pluginId) const
{
    const juce::ScopedLock sl (queueLock);
    if (currentJobId == pluginId)
        return true;
    for (auto& j : downloadQueue)
        if (j.pluginId == pluginId)
            return true;
    return false;
}

bool NetworkManager::isPrefetch (const juce::String& pluginId) const
{
    const juce::ScopedLock sl (queueLock);
    if (currentJobId == pluginId)
        return currentIsPrefetch;
    for (auto& j : downloadQueue)
        if (j.pluginId == pluginId)
            return j.prefetch;
    return false;
}

void NetworkManager::promoteToInstall (const juce::String& pluginId)
{
    const juce::ScopedLock sl (queueLock);
    if (currentJobId == pluginId)
        currentIsPrefetch = false;
    for (auto& j : downloadQueue)
        if (j.pluginId == pluginId)
            j.prefetch = false;
}

bool NetworkManager::hasPendingDownloads() const
{
    const juce::ScopedLock sl (queueLock);
    // A background pre-download alone does not hold a batch back.
    if (currentJobId.isNotEmpty() && ! currentIsPrefetch)
        return true;
    for (auto& j : downloadQueue)
        if (! j.prefetch)
            return true;
    return false;
}

bool NetworkManager::shouldAbortDownload() const
{
    return downloadWorker.threadShouldExit() || cancelCurrent.load();
}

void NetworkManager::DownloadWorker::run()
{
    while (! threadShouldExit())
    {
        DownloadJob job;
        bool haveJob = false;
        {
            const juce::ScopedLock sl (owner.queueLock);
            if (! owner.downloadQueue.isEmpty())
            {
                job = owner.downloadQueue.removeAndReturn (0);
                owner.currentJobId = job.pluginId;
                owner.currentIsPrefetch = job.prefetch;
                owner.cancelCurrent = false;
                haveJob = true;
            }
        }

        if (! haveJob)
        {
            wake.wait (-1);
            continue;
        }

        owner.runDownloadJob (job);

        const juce::ScopedLock sl (owner.queueLock);
        owner.currentJobId.clear();
        owner.currentIsPrefetch = false;
    }
}

// ============================================================================
// Single download job (the download thread)
// ============================================================================

namespace
{
    // A real installer is megabytes. Anything under this is an HTML error page
    // (GitHub serves small 404 pages for missing/private release assets).
    constexpr juce::int64 MIN_INSTALLER_SIZE = 1024 * 1024; // 1 MB

    // Most failures reported from the field are a dropped connection, not a bad
    // link, so any attempt that could plausibly succeed next time is retried.
    constexpr int downloadAttempts = 3;
    constexpr int retryBackoffMs   = 1500;

    juce::String describeSize (juce::int64 bytes)
    {
        if (bytes < 1024 * 1024)
            return juce::String (bytes / 1024) + " KB";

        return juce::String (bytes / (1024.0 * 1024.0), 1) + " MB";
    }

    // A verified file is kept for its version (background pre-downloads): its
    // hash is remembered beside it so the next run knows whether it is still good.
    juce::File hashStampFor (const juce::File& f) { return f.withFileExtension (f.getFileExtension() + ".sha256"); }
}

void NetworkManager::runDownloadJob (const DownloadJob& job)
{
    getDownloadDir().createDirectory();
    const auto tempFile = installerFileFor (job.pluginId);
    const auto partFile = tempFile.withFileExtension (tempFile.getFileExtension() + ".part");

    post ([this, id = job.pluginId] { listeners.call (&Listener::onDownloadStarted, id); });

    bool         success = false;
    bool         cancelled = false;
    juce::String errorMsg;

    // Already here and already verified (a background pre-download, or the same
    // file a cancelled install left): no second transfer.
    if (tempFile.existsAsFile() && hashStampFor (tempFile).loadFileAsString().trim().equalsIgnoreCase (job.sha256)
        && job.sha256.isNotEmpty())
    {
        juce::FileInputStream fis (tempFile);
        success = fis.openedOk() && juce::SHA256 (fis).toHexString().equalsIgnoreCase (job.sha256);
    }

    for (int attempt = 1; ! success && attempt <= downloadAttempts; ++attempt)
    {
        if (shouldAbortDownload())
            break;

        bool retryable = false;
        success = attemptDownload (job, partFile, errorMsg, retryable);

        if (success || ! retryable || attempt == downloadAttempts)
            break;

        // Back off before trying again, in short slices so a cancel still lands quickly.
        for (int waited = 0; waited < retryBackoffMs * attempt && ! shouldAbortDownload(); waited += 100)
            juce::Thread::sleep (100);
    }

    if (! success && shouldAbortDownload())
    {
        cancelled = true;
        errorMsg = "Download cancelled.";
    }

    if (success && partFile.existsAsFile())
    {
        tempFile.deleteFile();
        hashStampFor (tempFile).deleteFile();
        if (partFile.moveFileTo (tempFile))
            hashStampFor (tempFile).replaceWithText (job.sha256.toLowerCase());
        else
        {
            success = false;
            errorMsg = "Download failed - the file could not be saved. Check that antivirus is not blocking RONE installers.";
        }
    }

    partFile.deleteFile();
    if (! success)
    {
        tempFile.deleteFile();
        hashStampFor (tempFile).deleteFile();
    }

    if (downloadWorker.threadShouldExit())
        return;

    post ([this, pid = job.pluginId, file = success ? tempFile : juce::File(), success, cancelled, err = errorMsg]
    {
        listeners.call (&Listener::onDownloadComplete, pid, file, success, cancelled, err);
    });
}

//==============================================================================
/** One transfer attempt. Returns true when tempFile holds a verified installer.
    On failure `errorMessage` explains what went wrong and `retryable` says
    whether trying again could plausibly help (a dropped or throttled connection)
    as opposed to a genuinely wrong link or a bad asset.
*/
bool NetworkManager::attemptDownload (const DownloadJob& job,
                                      const juce::File& tempFile,
                                      juce::String& errorMessage,
                                      bool& retryable)
{
    retryable = false;
    errorMessage.clear();

    if (tempFile.existsAsFile())
        tempFile.deleteFile();

    // Only https: the manifest is fetched over TLS, and so is everything it points at.
    if (! job.url.startsWithIgnoreCase ("https://"))
    {
        errorMessage = "Download refused - the download link is not a secure (https) address.";
        return false;
    }

    juce::int64 totalBytes = 0;   // Content-Length; 0 when the server will not say
    juce::int64 downloaded = 0;

#if JUCE_MAC
    // On macOS, juce::URL with JUCE_USE_CURL=0 (CFNetwork) does not reliably
    // follow GitHub's cross-domain 302 redirect from github.com to the signed
    // release-assets.githubusercontent.com URL. Shell out to /usr/bin/curl
    // (preinstalled on every macOS) which handles redirects correctly.
    // HEAD request to learn the final Content-Length for progress reporting.
    {
        juce::ChildProcess head;
        juce::StringArray headArgs { "/usr/bin/curl", "-sIL", "--proto", "=https", job.url };
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
                               "--proto", "=https",   // never anything but https, redirects included
                               "--silent",
                               "--show-error",
                               "-o", tempFile.getFullPathName(),
                               job.url };

    juce::ChildProcess curl;
    if (! curl.start (dlArgs, juce::ChildProcess::wantStdErr))
    {
        errorMessage = "Failed to start download process.";
        return false;
    }

    auto lastProgressTime = juce::Time::getMillisecondCounterHiRes();
    while (curl.isRunning())
    {
        if (shouldAbortDownload())
        {
            curl.kill();
            tempFile.deleteFile();
            errorMessage = "Download cancelled.";
            return false;
        }

        if (totalBytes > 0 && tempFile.existsAsFile())
        {
            auto now = juce::Time::getMillisecondCounterHiRes();
            if (now - lastProgressTime >= 100.0)
            {
                lastProgressTime = now;
                double progress = juce::jlimit (0.0, 1.0,
                                    (double) tempFile.getSize() / (double) totalBytes);
                post ([this, pid = job.pluginId, progress]
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

        // curl exit 22 is "-f" firing on an HTTP >= 400: the link itself is
        // wrong or the asset is gone, and retrying will not change that.
        // Everything else here is a transport failure worth another go.
        retryable = (exitCode != 22);

        auto detail = curlStderr.trim();
        errorMessage = retryable
                         ? juce::String ("Download interrupted - ")
                             + (detail.isNotEmpty() ? detail
                                                    : juce::String ("the connection dropped."))
                             + " Check your internet connection and try again."
                         : juce::String ("Download failed - the server rejected the request. "
                                         "This installer may have been moved or removed.");
        return false;
    }

    downloaded = tempFile.existsAsFile() ? tempFile.getSize() : 0;
#else
    int statusCode = 0;

    juce::URL url (job.url);
    auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                       .withConnectionTimeoutMs (15000)
                       .withNumRedirectsToFollow (5)
                       .withStatusCode (&statusCode);

    auto stream = url.createInputStream (options);

    if (shouldAbortDownload())
    {
        errorMessage = "Download cancelled.";
        return false;
    }

    if (stream == nullptr)
    {
        // No stream at all: either we never reached the server, or the backend
        // refused to hand us the body of an error response.
        if (statusCode >= 400)
        {
            errorMessage = "Download failed - the server returned HTTP "
                         + juce::String (statusCode)
                         + ". This installer may have been moved or removed.";
            retryable = (statusCode >= 500);
        }
        else
        {
            errorMessage = "Could not reach the download server. "
                           "Check your internet connection and try again.";
            retryable = true;
        }

        return false;
    }

    // An HTTP error still carries a body (GitHub's 404 page), and writing that
    // to disk is exactly how a "corrupt 0 KB installer" used to be born.
    if (statusCode != 0 && (statusCode < 200 || statusCode >= 300))
    {
        errorMessage = "Download failed - the server returned HTTP "
                     + juce::String (statusCode)
                     + ". This installer may have been moved or removed.";
        retryable = (statusCode >= 500);
        return false;
    }

    totalBytes = juce::jmax ((juce::int64) 0, stream->getTotalLength());

    {
        juce::FileOutputStream output (tempFile);
        if (! output.openedOk())
        {
            errorMessage = "Could not create temp file for download.";
            return false;
        }

        constexpr int bufferSize = 32768;
        juce::HeapBlock<char> buffer (bufferSize);
        auto lastProgressTime = juce::Time::getMillisecondCounterHiRes();

        while (! shouldAbortDownload())
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
                    double progress = juce::jlimit (0.0, 1.0, (double) downloaded / (double) totalBytes);
                    post ([this, pid = job.pluginId, progress]
                    {
                        listeners.call (&Listener::onDownloadProgress, pid, progress);
                    });
                }
            }
        }

        output.flush();
    }

    if (shouldAbortDownload())
    {
        tempFile.deleteFile();
        errorMessage = "Download cancelled.";
        return false;
    }
#endif

    // --- Did we actually receive everything the server promised? ------------
    // read() returning 0 means "no more bytes", which a dropped connection
    // looks exactly like. Without this the truncated file sails on to the size
    // and SHA checks and gets reported as a corrupt or tampered installer.
    if (totalBytes > 0 && downloaded < totalBytes)
    {
        tempFile.deleteFile();
        retryable    = true;
        errorMessage = "Download interrupted - only " + describeSize (downloaded)
                     + " of " + describeSize (totalBytes)
                     + " arrived. Check your internet connection and try again.";
        return false;
    }

    if (! tempFile.existsAsFile())
    {
        // The bytes went somewhere and the file is gone: on Windows this is
        // usually antivirus quarantining an unsigned installer mid-write.
        retryable    = true;
        errorMessage = "Download failed - the file was not saved. "
                       "Check that antivirus is not blocking RONE installers.";
        return false;
    }

    const auto fileSize = tempFile.getSize();

    if (fileSize < MIN_INSTALLER_SIZE)
    {
        tempFile.deleteFile();

        // A complete-but-tiny response is an error page, not a transfer fault;
        // only retry when the server never told us how big the file should be.
        retryable    = (totalBytes <= 0);
        errorMessage = "Download failed - the server sent " + describeSize (fileSize)
                     + " instead of an installer. The download link may be invalid.";
        return false;
    }

    // SHA256 verification.
    //
    // This is the gate that decides whether a file downloaded off the internet
    // is about to be EXECUTED on a customer's machine, so every way past it has
    // to be a deliberate pass. An entry without a hash is exactly the case the
    // manifest writes when a build leg did not run, which is when an unverified
    // binary is MOST likely to be the wrong one. Refusing costs a customer one
    // retry; the other way costs them whatever the file turns out to be. (The
    // file is hashed once more right before it runs - InstallBatcher.)
    if (job.sha256.isEmpty())
    {
        tempFile.deleteFile();
        retryable    = false;      // nothing about retrying produces a hash
        errorMessage = "Download refused - this release has no integrity hash to check it against. "
                       "Try again once the release finishes publishing.";
        return false;
    }

    juce::FileInputStream fis (tempFile);
    if (! fis.openedOk())
    {
        tempFile.deleteFile();
        // Usually anti-virus holding the fresh file open; the next attempt
        // often gets it.
        retryable    = true;
        errorMessage = "Download failed - the file could not be read back to verify it.";
        return false;
    }

    juce::SHA256 hash (fis);
    const auto computed = hash.toHexString();

    if (computed.compareIgnoreCase (job.sha256) != 0)
    {
        DBG ("[Download] SHA256 mismatch! Expected: " + job.sha256 + " Got: " + computed);

        tempFile.deleteFile();
        // A silently corrupted transfer hashes differently every time, so one
        // more attempt is worth it before blaming the release.
        retryable    = true;
        errorMessage = "Download failed - file integrity check failed (SHA256 mismatch).";
        return false;
    }

    DBG ("[Download] SHA256 verified OK: " + computed);
    return true;
}

// ============================================================================
// JSON parsing
// ============================================================================

static juce::StringArray stringList (const juce::var& v)
{
    juce::StringArray out;
    if (auto* arr = v.getArray())
        for (auto& x : *arr)
            if (x.toString().trim().isNotEmpty())
                out.add (x.toString().trim());
    return out;
}

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
        info.videoUrl       = entry.getProperty ("video",            {}).toString();
        info.registryKey    = entry.getProperty ("registry_key",     {}).toString();
        info.type          = entry.getProperty ("type",         {}).toString();

        if (info.id.isEmpty())
            continue;

        // Only the plugins sold on their own carry these. Left as raw vars so a
        // manifest without them (an ALL-ACCESS-only plugin, or a cached older
        // catalog) stays distinguishable from one priced at nothing.
        info.price       = entry.getProperty ("price",        {});
        info.launchPrice = entry.getProperty ("launch_price", {});
        info.storeUrl    = entry.getProperty ("store_url",    {}).toString();

        // Center 2.0 catalog fields
        info.free       = (bool) entry.getProperty ("free", false);
        info.categories = stringList (entry.getProperty ("category", {}));
        info.tags       = stringList (entry.getProperty ("tags", {}));
        info.accent     = entry.getProperty ("accent",   {}).toString().trim();
        info.released   = entry.getProperty ("released", {}).toString().trim();
        const auto preview = entry.getProperty ("preview", {});
        info.previewDry = preview.getProperty ("dry", {}).toString().trim();
        info.previewWet = preview.getProperty ("wet", {}).toString().trim();
        info.innoAppId  = entry.getProperty ("inno_app_id", {}).toString().trim();
        info.i18n       = entry.getProperty ("i18n", {});
       #if JUCE_MAC
        info.sizeBytes  = (juce::int64) (double) entry.getProperty ("size_mac", 0.0);
       #else
        info.sizeBytes  = (juce::int64) (double) entry.getProperty ("size", 0.0);
       #endif

        if (info.innoAppId.isNotEmpty())
            VersionChecker::registerInnoAppId (info.registryKey, info.innoAppId);

        auto* fmts = entry.getProperty ("formats", {}).getArray();
        if (fmts != nullptr)
            for (auto& f : *fmts)
                info.formats.add (f.toString());

        // Determine installed status
        VersionChecker::refreshInstallState (info);

        result.add (std::move (info));
    }

    return result;
}
