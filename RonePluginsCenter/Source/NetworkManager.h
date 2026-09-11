#pragma once
#include <JuceHeader.h>
#include "VersionChecker.h"

// ============================================================================
// NetworkManager — async JSON fetch + file downloads with progress
// ============================================================================
class NetworkManager : public juce::Thread
{
public:
    // ---- Listener interface ------------------------------------------------
    struct Listener
    {
        virtual ~Listener() = default;

        // Called on the message thread when versions.json is parsed.
        virtual void onManifestReady (const juce::Array<PluginInfo>& plugins) = 0;

        // Called on the message thread when a manifest fetch fails.
        virtual void onManifestError (const juce::String& errorMessage) = 0;

        // Called periodically on the message thread during a download.
        virtual void onDownloadProgress (const juce::String& pluginId,
                                          double progress) = 0;

        // Called on the message thread when a download finishes.
        virtual void onDownloadComplete (const juce::String& pluginId,
                                          const juce::File& localFile,
                                          bool success,
                                          const juce::String& errorMessage) = 0;
    };

    // ---- API ---------------------------------------------------------------
    NetworkManager();
    ~NetworkManager() override;

    void addListener    (Listener* l)  { listeners.add (l); }
    void removeListener (Listener* l)  { listeners.remove (l); }

    // Fetch versions.json from the remote URL (async — results via listener).
    // freshFromOrigin skips raw.githubusercontent's five-minute CDN copy and
    // reads the file from GitHub's API instead - only for the retry after a
    // hash mismatch, see MainComponent::onDownloadComplete.
    void fetchManifest (bool freshFromOrigin = false);

    // Download a file from `url` to a temp folder (async — progress via listener).
    // If `sha256` is non-empty, the downloaded file is verified against it.
    void downloadInstaller (const juce::String& pluginId,
                            const juce::String& url,
                            const juce::String& sha256 = {});

    // Cancel any ongoing download.
    void cancelDownload();

private:
    void run() override;

    // Parse the raw JSON body into PluginInfo structs.
    juce::Array<PluginInfo> parseManifest (const juce::String& jsonBody);

    // Hardcoded fallback manifest — used when remote fetch returns 0 plugins
    // (e.g. private repo 404, no internet, JSON parse failure).
    juce::Array<PluginInfo> getFallbackManifest();

    // ---- Center self-update ------------------------------------------------
public:
    struct CenterInstallerInfo
    {
        juce::String version, url, sha256;
        bool isValid() const { return version.isNotEmpty() && url.isNotEmpty(); }
    };

    // Snapshot of the manifest's center_installer block, captured by the last
    // successful fetch (empty until then). Safe to call from the message thread.
    CenterInstallerInfo getCenterInstallerInfo() const
    {
        const juce::ScopedLock sl (centerInfoLock);
        return centerInfo;
    }

private:
    mutable juce::CriticalSection centerInfoLock;
    CenterInstallerInfo           centerInfo;

    // ---- State -------------------------------------------------------------
    enum Task { None, FetchManifest, DownloadFile };

    struct DownloadJob
    {
        juce::String pluginId;
        juce::String url;
        juce::String sha256;
    };

    // Download a single job (runs on the network thread). Fires listener callbacks.
    // Retries a few times when the failure looks like a dropped connection.
    void runDownloadJob (const DownloadJob& job);

    // One transfer attempt: fetch, check the HTTP status, verify that every
    // promised byte arrived, then check size and SHA256. Returns true when
    // tempFile holds a verified installer; otherwise fills errorMessage and
    // sets retryable when another attempt could plausibly succeed.
    bool attemptDownload (const DownloadJob& job,
                          const juce::File& tempFile,
                          juce::String& errorMessage,
                          bool& retryable);

    Task               currentTask  { None };
    juce::String       targetUrl;            // used by FetchManifest only
    bool               manifestFromOrigin { false };

    juce::CriticalSection      queueLock;
    juce::Array<DownloadJob>   downloadQueue; // pending downloads (FIFO)

    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NetworkManager)
};
