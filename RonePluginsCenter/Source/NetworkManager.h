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
    void fetchManifest();

    // Download a file from `url` to a temp folder (async — progress via listener).
    void downloadInstaller (const juce::String& pluginId,
                            const juce::String& url);

    // Cancel any ongoing download.
    void cancelDownload();

private:
    void run() override;

    // Parse the raw JSON body into PluginInfo structs.
    juce::Array<PluginInfo> parseManifest (const juce::String& jsonBody);

    // ---- State -------------------------------------------------------------
    enum Task { None, FetchManifest, DownloadFile };

    Task               currentTask  { None };
    juce::String       targetUrl;
    juce::String       activePluginId;
    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NetworkManager)
};
