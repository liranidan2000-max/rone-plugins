#pragma once
#include <JuceHeader.h>
#include "VersionChecker.h"

// ============================================================================
// NetworkManager — the manifest and the installer downloads, each on its own
// long-lived thread.
//
// Before Center 2.0 one thread did both, so every manifest fetch (the Refresh
// button, the retry after a hash mismatch) cancelled whatever was downloading
// and emptied the queue - in the middle of "Update All" plugins silently fell
// out of it - and a download queued while the manifest was loading was never
// picked up at all (the card sat at "Downloading 0%"). Now the two never touch
// each other: a fetch never cancels a download, and the download worker sleeps
// on an event, so nothing queued can be missed.
//
// The last good manifest is kept on disk. Offline, the Center shows the
// catalog it last saw (and says it is offline) instead of an empty page; the
// hardcoded fallback catalog is gone - it listed six plugins at 1.0.0 with no
// hashes, so behind a captive portal every installed plugin "had an update"
// that could only fail.
// ============================================================================
class NetworkManager
{
public:
    // ---- Listener interface ------------------------------------------------
    struct Listener
    {
        virtual ~Listener() = default;

        // Message thread. fromCache = the network failed and this is the last
        // manifest that arrived, read back from disk.
        virtual void onManifestReady (const juce::Array<PluginInfo>& plugins, bool fromCache) = 0;

        // Message thread: no manifest at all (offline and nothing cached), or a
        // fresh-from-origin read for a hash retry failed.
        virtual void onManifestError (const juce::String& errorMessage) = 0;

        // Message thread, about ten times a second during a download.
        virtual void onDownloadProgress (const juce::String& pluginId, double progress) = 0;

        // Message thread. The download left the queue and is running now.
        virtual void onDownloadStarted (const juce::String& pluginId) = 0;

        // Message thread, when a download finishes. cancelled = cancelDownload() asked for it.
        virtual void onDownloadComplete (const juce::String& pluginId,
                                         const juce::File& localFile,
                                         bool success,
                                         bool cancelled,
                                         const juce::String& errorMessage) = 0;
    };

    // ---- API ---------------------------------------------------------------
    NetworkManager();
    ~NetworkManager();

    void addListener    (Listener* l)  { listeners.add (l); }
    void removeListener (Listener* l)  { listeners.remove (l); }

    // Fetch versions.json (async — results via listener). Never touches the
    // downloads. Requests that arrive while one is running are folded into one
    // more fetch after it. freshFromOrigin skips raw.githubusercontent's
    // five-minute CDN copy and reads the file from GitHub's API instead - only
    // for the retry after a hash mismatch (MainComponent::onDownloadComplete).
    void fetchManifest (bool freshFromOrigin = false);

    // Queue a download (async — progress via listener). The file is verified
    // against sha256, which must be present. A prefetch is a background
    // pre-download: the caller keeps the file for the moment the user says Update.
    void downloadInstaller (const juce::String& pluginId,
                            const juce::String& url,
                            const juce::String& sha256,
                            bool isPrefetch = false);

    // Cancel one plugin's download, queued or running.
    void cancelDownload (const juce::String& pluginId);
    void cancelAllDownloads();

    bool isDownloadQueued (const juce::String& pluginId) const;   // queued or running
    bool isPrefetch (const juce::String& pluginId) const;         // ...and only as a background pre-download
    void promoteToInstall (const juce::String& pluginId);         // a pre-download the user now wants installed
    bool hasPendingDownloads() const;                             // anything queued or running

    juce::int64 getLastManifestSuccessMs() const noexcept { return lastSuccessMs.load(); }

    // Where downloads land, and where the background pre-downloads wait.
    static juce::File getDownloadDir();
    static juce::File installerFileFor (const juce::String& pluginId);

    // ---- Center self-update ------------------------------------------------
    struct CenterInstallerInfo
    {
        juce::String version, url, sha256, urlMac, sha256Mac;
        bool isValid() const
        {
           #if JUCE_MAC
            return version.isNotEmpty() && urlMac.isNotEmpty() && sha256Mac.isNotEmpty();
           #else
            return version.isNotEmpty() && url.isNotEmpty();
           #endif
        }
    };

    // Snapshot of the manifest's center_installer block, captured by the last
    // successful fetch (empty until then). Safe to call from any thread.
    CenterInstallerInfo getCenterInstallerInfo() const
    {
        const juce::ScopedLock sl (centerInfoLock);
        return centerInfo;
    }

    // Manifest-wide extras for the page (tips of the week), as parsed JSON.
    juce::var getManifestExtras() const
    {
        const juce::ScopedLock sl (centerInfoLock);
        return extras;
    }

private:
    struct DownloadJob
    {
        juce::String pluginId;
        juce::String url;
        juce::String sha256;
        bool prefetch = false;
    };

    // ---- the two workers -----------------------------------------------------
    struct ManifestWorker : public juce::Thread
    {
        explicit ManifestWorker (NetworkManager& o) : juce::Thread ("RONE-Manifest"), owner (o) {}
        void run() override;
        NetworkManager& owner;
        juce::WaitableEvent wake;
    };

    struct DownloadWorker : public juce::Thread
    {
        explicit DownloadWorker (NetworkManager& o) : juce::Thread ("RONE-Download"), owner (o) {}
        void run() override;
        NetworkManager& owner;
        juce::WaitableEvent wake;
    };

    void fetchOnce (bool fromOrigin);                    // manifest thread
    void runDownloadJob (const DownloadJob& job);        // download thread
    bool attemptDownload (const DownloadJob& job, const juce::File& tempFile,
                          juce::String& errorMessage, bool& retryable);
    bool shouldAbortDownload() const;

    // Parse the raw JSON body into PluginInfo structs.
    juce::Array<PluginInfo> parseManifest (const juce::String& jsonBody);
    void captureManifestWide (const juce::var& root);

    static juce::File getManifestCacheFile();

    // Runs fn on the message thread unless this manager has been destroyed by then.
    void post (std::function<void()> fn);

    ManifestWorker manifestWorker { *this };
    DownloadWorker downloadWorker { *this };

    juce::CriticalSection   requestLock;            // the manifest request flags
    bool                    manifestRequested = false;
    bool                    manifestFromOrigin = false;

    mutable juce::CriticalSection queueLock;        // the download queue + the job in flight
    juce::Array<DownloadJob>      downloadQueue;    // FIFO
    juce::String                  currentJobId;     // running now, empty when idle
    bool                          currentIsPrefetch = false;
    std::atomic<bool>             cancelCurrent { false };

    std::atomic<juce::int64>      lastSuccessMs { 0 };

    mutable juce::CriticalSection centerInfoLock;
    CenterInstallerInfo           centerInfo;
    juce::var                     extras;

    std::shared_ptr<std::atomic<bool>> alive = std::make_shared<std::atomic<bool>> (true);
    juce::ListenerList<Listener>  listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NetworkManager)
};
