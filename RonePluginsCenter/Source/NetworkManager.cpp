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

            bool success = tempFile.existsAsFile() && tempFile.getSize() > 0;
            auto pid  = activePluginId;
            auto file = tempFile;

            juce::MessageManager::callAsync ([this, pid, file, success]
            {
                listeners.call (&Listener::onDownloadComplete,
                                pid, file, success,
                                success ? juce::String()
                                        : juce::String ("Download failed — file is empty."));
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
