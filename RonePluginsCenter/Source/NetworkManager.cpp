#include "NetworkManager.h"

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
                                         const juce::String& url)
{
    cancelDownload();
    currentTask    = DownloadFile;
    targetUrl      = url;
    activePluginId = pluginId;
    startThread();
}

void NetworkManager::cancelDownload()
{
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
            // --- Download an installer to a temp file --------------------
            auto tempDir  = juce::File::getSpecialLocation (juce::File::tempDirectory)
                               .getChildFile ("RONE_Downloads");
            tempDir.createDirectory();

            auto tempFile = tempDir.getChildFile (activePluginId + "_Installer.exe");
            if (tempFile.existsAsFile())
                tempFile.deleteFile();

            juce::URL url (targetUrl);
            auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                               .withConnectionTimeoutMs (15000)
                               .withNumRedirectsToFollow (5);

            auto stream = url.createInputStream (options);

            if (stream == nullptr || threadShouldExit())
            {
                auto pid = activePluginId;
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
                auto pid = activePluginId;
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

            while (! threadShouldExit())
            {
                auto bytesRead = stream->read (buffer, bufferSize);
                if (bytesRead <= 0)
                    break;

                output.write (buffer, (size_t) bytesRead);
                downloaded += bytesRead;

                if (totalBytes > 0)
                {
                    double progress = (double) downloaded / (double) totalBytes;
                    auto pid = activePluginId;
                    juce::MessageManager::callAsync ([this, pid, progress]
                    {
                        listeners.call (&Listener::onDownloadProgress, pid, progress);
                    });
                }
            }

            output.flush();

            if (threadShouldExit())
            {
                tempFile.deleteFile();
                return;
            }

            // Verify the downloaded file is a real installer, not an HTML error page.
            // GitHub returns small HTML 404 pages for missing/private release assets.
            static constexpr juce::int64 MIN_INSTALLER_SIZE = 1024 * 1024; // 1 MB

            bool fileExists = tempFile.existsAsFile();
            auto fileSize   = fileExists ? tempFile.getSize() : 0;
            bool success    = fileExists && fileSize >= MIN_INSTALLER_SIZE;

            auto pid  = activePluginId;
            auto file = tempFile;

            juce::String errorMsg;
            if (! fileExists)
                errorMsg = "Download failed — file was not saved.";
            else if (fileSize < MIN_INSTALLER_SIZE)
                errorMsg = "Download failed — corrupt file (" + juce::String (fileSize / 1024)
                         + " KB). The download link may be invalid.";

            juce::MessageManager::callAsync ([this, pid, file, success, errorMsg]
            {
                listeners.call (&Listener::onDownloadComplete,
                                pid, file, success, errorMsg);
            });
            break;
        }

        default:
            break;
    }
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
        info.downloadUrl   = entry.getProperty ("download_url", {}).toString();
        info.sha256        = entry.getProperty ("sha256",       {}).toString();
        info.standaloneExe = entry.getProperty ("standalone_exe", {}).toString();
        info.vst3Bundle    = entry.getProperty ("vst3_bundle",  {}).toString();
        info.registryKey   = entry.getProperty ("registry_key", {}).toString();
        info.type          = entry.getProperty ("type",         {}).toString();

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
                      || VersionChecker::isVst3Installed (info.vst3Bundle);
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
      "name": "ReverseReverb",
      "version": "1.0.0",
      "type": "plugin",
      "formats": ["VST3", "Standalone"],
      "description": "Real-time reverse reverb effect with WebView2 UI",
      "whats_new": "Initial release",
      "standalone_exe": "ReverseReverb.exe",
      "vst3_bundle": "ReverseReverb.vst3",
      "download_url": "https://github.com/liranidan2000-max/rone-plugins/releases/latest/download/ReverseReverb_Installer.exe",
      "sha256": "",
      "registry_key": "ReverseReverb"
    },
    {
      "id": "RoneStemsFixer",
      "name": "RONE Stems Fixer",
      "version": "1.0.0",
      "type": "standalone",
      "formats": ["Standalone"],
      "description": "Audio stem analysis and repair tool",
      "whats_new": "Initial release",
      "standalone_exe": "RONE Stems Fixer.exe",
      "vst3_bundle": "",
      "download_url": "https://github.com/liranidan2000-max/rone-plugins/releases/latest/download/RoneStemsFixer_Installer.exe",
      "sha256": "",
      "registry_key": "RoneStemsFixer"
    },
    {
      "id": "RoneStutter",
      "name": "Rone Stutter",
      "version": "1.0.0",
      "type": "plugin",
      "formats": ["VST3", "Standalone"],
      "description": "Glitch and stutter effect with WebView2 UI",
      "whats_new": "Initial release",
      "standalone_exe": "Rone Stutter.exe",
      "vst3_bundle": "Rone Stutter.vst3",
      "download_url": "https://github.com/liranidan2000-max/rone-plugins/releases/latest/download/RoneStutter_Installer.exe",
      "sha256": "",
      "registry_key": "RoneStutter"
    },
    {
      "id": "RoneFlanger",
      "name": "Rone Flanger",
      "version": "1.0.0",
      "type": "plugin",
      "formats": ["VST3", "Standalone"],
      "description": "Manual flanger with custom visualizer",
      "whats_new": "Initial release",
      "standalone_exe": "Rone Flanger.exe",
      "vst3_bundle": "Rone Flanger.vst3",
      "download_url": "https://github.com/liranidan2000-max/rone-plugins/releases/latest/download/RoneFlanger_Installer.exe",
      "sha256": "",
      "registry_key": "RoneFlanger"
    }
  ]
})";

    return parseManifest (json);
}
