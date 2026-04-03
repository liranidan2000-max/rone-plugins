#include "MainComponent.h"
#include "BinaryData.h"

// ============================================================================
// Resource provider — serves embedded HTML/CSS/JS + logos
// ============================================================================

juce::String MainComponent::getMimeForExtension (const juce::String& ext)
{
    if (ext == "html") return "text/html";
    if (ext == "css")  return "text/css";
    if (ext == "js")   return "application/javascript";
    if (ext == "png")  return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "svg")  return "image/svg+xml";
    if (ext == "json") return "application/json";
    if (ext == "ico")  return "image/x-icon";
    return "application/octet-stream";
}

std::optional<juce::WebBrowserComponent::Resource>
MainComponent::getResource (const juce::String& url)
{
    auto path = url == "/" ? juce::String ("index.html")
                           : url.fromFirstOccurrenceOf ("/", false, false);

    // Map URL paths to BinaryData entries
    struct Entry { const char* data; int size; };
    const std::map<juce::String, Entry> resources = {
        { "index.html",              { BinaryData::index_html,              BinaryData::index_htmlSize } },
        { "bundle.js",               { BinaryData::bundle_js,              BinaryData::bundle_jsSize } },
        { "styles.css",              { BinaryData::styles_css,             BinaryData::styles_cssSize } },
        { "logos/ReverseReverb.png",  { BinaryData::ReverseReverb_icon_png, BinaryData::ReverseReverb_icon_pngSize } },
        { "logos/RoneStutter.png",    { BinaryData::RoneStutter_icon_png,   BinaryData::RoneStutter_icon_pngSize } },
        { "logos/RoneStemsFixer.png",  { BinaryData::RoneStemsFixer_icon_png, BinaryData::RoneStemsFixer_icon_pngSize } },
        { "logos/RoneFlanger.png",    { BinaryData::RoneFlanger_icon_png,   BinaryData::RoneFlanger_icon_pngSize } },
    };

    auto it = resources.find (path);
    if (it == resources.end())
        return std::nullopt;

    auto& entry = it->second;
    std::vector<std::byte> bytes ((size_t) entry.size);
    std::memcpy (bytes.data(), entry.data, (size_t) entry.size);

    auto ext = path.fromLastOccurrenceOf (".", false, false).toLowerCase();

    return juce::WebBrowserComponent::Resource {
        std::move (bytes),
        getMimeForExtension (ext)
    };
}

// ============================================================================
// Construction
// ============================================================================

MainComponent::MainComponent()
    : webView (juce::WebBrowserComponent::Options{}
        .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options (
            juce::WebBrowserComponent::Options::WinWebView2{}
                .withUserDataFolder (
                    juce::File::getSpecialLocation (juce::File::tempDirectory)
                        .getChildFile ("RonePluginsCenter")))
        .withNativeIntegrationEnabled()

        // ---- JS → C++ native functions ----
        .withNativeFunction ("getPlugins", [this] (NativeArgs args, NativeCompletion complete) {
            handleGetPlugins (args, std::move (complete));
        })
        .withNativeFunction ("installPlugin", [this] (NativeArgs args, NativeCompletion complete) {
            handleInstallPlugin (args, std::move (complete));
        })
        .withNativeFunction ("openPlugin", [this] (NativeArgs args, NativeCompletion complete) {
            handleOpenPlugin (args, std::move (complete));
        })
        .withNativeFunction ("refreshPlugins", [this] (NativeArgs args, NativeCompletion complete) {
            handleRefreshPlugins (args, std::move (complete));
        })
        .withNativeFunction ("activateLicense", [this] (NativeArgs args, NativeCompletion complete) {
            handleActivateLicense (args, std::move (complete));
        })
        .withNativeFunction ("deactivateLicense", [this] (NativeArgs args, NativeCompletion complete) {
            handleDeactivateLicense (args, std::move (complete));
        })
        .withNativeFunction ("getLicenseStatus", [this] (NativeArgs args, NativeCompletion complete) {
            handleGetLicenseStatus (args, std::move (complete));
        })
        .withNativeFunction ("getAppVersion", [this] (NativeArgs args, NativeCompletion complete) {
            handleGetAppVersion (args, std::move (complete));
        })

        // ---- Resource provider ----
        .withResourceProvider (
            [this] (const auto& url) { return getResource (url); },
            juce::URL { "http://localhost:3000/" }.getOrigin()))
{
    addAndMakeVisible (webView);
    setSize (920, 640);

    networkManager.addListener (this);

    // License handler
    licenseHandler.onLicenseStateChanged = [this] (bool isLicensed)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("licensed",     isLicensed);
        obj->setProperty ("customerName", licenseHandler.getCustomerName());
        obj->setProperty ("message",      licenseHandler.getStatusMessage());
        webView.emitEventIfBrowserIsVisible ("licenseChanged", juce::var (obj));

        // Also push updated plugin data (license affects card state)
        emitPluginsUpdated();
    };
    licenseHandler.initialize();

    // Navigate to resource provider root (uses JUCE's internal scheme, not actual HTTP)
    webView.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    // Fetch manifest after a short delay to let the WebView initialize
    juce::Timer::callAfterDelay (500, [this] { networkManager.fetchManifest(); });
}

MainComponent::~MainComponent()
{
    networkManager.removeListener (this);
}

void MainComponent::resized()
{
    webView.setBounds (getLocalBounds());
}

// ============================================================================
// Serialisation helpers
// ============================================================================

juce::String MainComponent::statusToString (PluginStatus s)
{
    switch (s)
    {
        case PluginStatus::NotInstalled:    return "not_installed";
        case PluginStatus::UpToDate:        return "up_to_date";
        case PluginStatus::UpdateAvailable: return "update_available";
        case PluginStatus::Downloading:     return "downloading";
        case PluginStatus::Installing:      return "installing";
        case PluginStatus::Error:           return "error";
    }
    return "unknown";
}

juce::var MainComponent::pluginInfoToVar (const PluginInfo& info)
{
    auto* obj = new juce::DynamicObject();

    obj->setProperty ("id",               info.id);
    obj->setProperty ("name",             info.name);
    obj->setProperty ("description",      info.description);
    obj->setProperty ("remoteVersion",    info.remoteVersion);
    obj->setProperty ("installedVersion", info.installedVersion);
    obj->setProperty ("whatsNew",         info.whatsNew);
    obj->setProperty ("status",           statusToString (info.status));
    obj->setProperty ("downloadProgress", info.downloadProgress);
    obj->setProperty ("type",             info.type);

    // Formats array
    juce::Array<juce::var> fmts;
    for (auto& f : info.formats)
        fmts.add (f);
    obj->setProperty ("formats", fmts);

    // Logo URL (served by resource provider)
    obj->setProperty ("logoUrl", "/logos/" + info.id + ".png");

    // Standalone availability
    bool hasStandalone = info.standaloneExe.isNotEmpty();
    bool standaloneInstalled = hasStandalone
                             && VersionChecker::isStandaloneInstalled (info.standaloneExe);
    obj->setProperty ("hasStandalone",       hasStandalone);
    obj->setProperty ("standaloneInstalled", standaloneInstalled);

    return juce::var (obj);
}

juce::var MainComponent::allPluginsToVar()
{
    juce::ScopedLock sl (pluginDataLock);
    juce::Array<juce::var> arr;
    for (auto& p : pluginData)
        arr.add (pluginInfoToVar (p));

    auto* result = new juce::DynamicObject();
    result->setProperty ("plugins", arr);
    return juce::var (result);
}

// ============================================================================
// Emit helpers
// ============================================================================

void MainComponent::emitPluginsUpdated()
{
    webView.emitEventIfBrowserIsVisible ("pluginsUpdated", allPluginsToVar());
}

void MainComponent::emitStatusMessage (const juce::String& text, const juce::String& type)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("text", text);
    obj->setProperty ("type", type);
    webView.emitEventIfBrowserIsVisible ("statusMessage", juce::var (obj));
}

// ============================================================================
// Native function handlers
// ============================================================================

void MainComponent::handleGetPlugins (NativeArgs, NativeCompletion complete)
{
    auto result = allPluginsToVar();
    complete (juce::JSON::toString (result));
}

void MainComponent::handleInstallPlugin (NativeArgs args, NativeCompletion complete)
{
    if (args.isEmpty())
    {
        complete ("{\"started\":false,\"error\":\"Missing plugin ID\"}");
        return;
    }

    if (! licenseHandler.isLicensed())
    {
        complete ("{\"started\":false,\"error\":\"License required\"}");
        return;
    }

    auto pluginId = args[0].toString();

    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
        {
            if (p.id == pluginId)
            {
                if (p.status == PluginStatus::NotInstalled
                 || p.status == PluginStatus::UpdateAvailable
                 || p.status == PluginStatus::Error
                 || p.status == PluginStatus::UpToDate)
                {
                    p.status = PluginStatus::Downloading;
                    p.downloadProgress = 0.0;

                #if JUCE_MAC
                    networkManager.downloadInstaller (pluginId, p.downloadUrlMac, p.sha256);
                #else
                    networkManager.downloadInstaller (pluginId, p.downloadUrl, p.sha256);
                #endif

                    emitPluginsUpdated();
                    complete ("{\"started\":true}");
                    return;
                }
                break;
            }
        }
    }

    complete ("{\"started\":false,\"error\":\"Plugin not in installable state\"}");
}

void MainComponent::handleOpenPlugin (NativeArgs args, NativeCompletion complete)
{
    if (args.isEmpty())
    {
        complete ("{\"success\":false,\"error\":\"Missing plugin ID\"}");
        return;
    }

    if (! licenseHandler.isLicensed())
    {
        complete ("{\"success\":false,\"error\":\"License required\"}");
        return;
    }

    auto pluginId = args[0].toString();

    juce::ScopedLock sl (pluginDataLock);
    for (auto& p : pluginData)
    {
        if (p.id == pluginId)
        {
        #if JUCE_MAC
            if (p.standaloneExe.isNotEmpty())
            {
                auto appName = p.standaloneExe.replace (".exe", "") + ".app";
                juce::File app;

                for (auto& dir : { juce::File ("/Applications"),
                                    juce::File ("/Applications/RONE Plugins"),
                                    VersionChecker::getStandaloneInstallDir() })
                {
                    auto candidate = dir.getChildFile (appName);
                    if (candidate.exists()) { app = candidate; break; }
                }

                if (app.exists())
                {
                    app.startAsProcess();
                    complete ("{\"success\":true}");
                    return;
                }
            }

            // Standalone not found — tell the user to install
            complete ("{\"success\":false,\"error\":\"Standalone not installed. Click INSTALL to download it.\"}");
            return;
        #else
            if (p.standaloneExe.isNotEmpty())
            {
                auto exe = VersionChecker::getStandaloneInstallDir()
                               .getChildFile (p.standaloneExe);
                if (exe.existsAsFile())
                {
                    exe.startAsProcess();
                    complete ("{\"success\":true}");
                    return;
                }
            }

            complete ("{\"success\":false,\"error\":\"Standalone not found on disk\"}");
        #endif
            return;
        }
    }

    complete ("{\"success\":false,\"error\":\"Plugin not found\"}");
}

void MainComponent::handleRefreshPlugins (NativeArgs, NativeCompletion complete)
{
    networkManager.fetchManifest();
    complete ("{\"success\":true}");
}

void MainComponent::handleActivateLicense (NativeArgs args, NativeCompletion complete)
{
    if (args.isEmpty())
    {
        auto* err = new juce::DynamicObject();
        err->setProperty ("success", false);
        err->setProperty ("message", "No license key provided");
        complete (juce::JSON::toString (juce::var (err)));
        return;
    }

    auto key = args[0].toString().trim();
    if (key.isEmpty())
    {
        auto* err = new juce::DynamicObject();
        err->setProperty ("success", false);
        err->setProperty ("message", "Empty license key");
        complete (juce::JSON::toString (juce::var (err)));
        return;
    }

    // Respond immediately — result comes via licenseActivationResult event
    auto* startObj = new juce::DynamicObject();
    startObj->setProperty ("started", true);
    complete (juce::JSON::toString (juce::var (startObj)));

    licenseHandler.activateLicense (key, [this] (bool success, juce::String msg)
    {
        juce::MessageManager::callAsync ([this, success, msg]()
        {
            auto* obj = new juce::DynamicObject();
            obj->setProperty ("success", success);
            obj->setProperty ("message", msg);
            if (success)
                obj->setProperty ("customerName", licenseHandler.getCustomerName());

            webView.emitEventIfBrowserIsVisible ("licenseActivationResult", juce::var (obj));

            if (success)
                emitStatusMessage ("License activated — all plugins unlocked!", "success");
        });
    });
}

void MainComponent::handleDeactivateLicense (NativeArgs, NativeCompletion complete)
{
    auto* startObj = new juce::DynamicObject();
    startObj->setProperty ("started", true);
    complete (juce::JSON::toString (juce::var (startObj)));

    licenseHandler.deactivateLicense ([this] (bool success, juce::String msg)
    {
        juce::MessageManager::callAsync ([this, success, msg]()
        {
            auto* obj = new juce::DynamicObject();
            obj->setProperty ("success", success);
            obj->setProperty ("message", msg);
            webView.emitEventIfBrowserIsVisible ("licenseDeactivationResult", juce::var (obj));
        });
    });
}

void MainComponent::handleGetLicenseStatus (NativeArgs, NativeCompletion complete)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("licensed",     licenseHandler.isLicensed());
    obj->setProperty ("customerName", licenseHandler.getCustomerName());
    obj->setProperty ("licenseKey",   licenseHandler.getLicenseKey());
    obj->setProperty ("message",      licenseHandler.getStatusMessage());
    complete (juce::JSON::toString (juce::var (obj)));
}

void MainComponent::handleGetAppVersion (NativeArgs, NativeCompletion complete)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("version",  juce::String (JUCE_APPLICATION_VERSION_STRING));
#if JUCE_MAC
    obj->setProperty ("platform", "mac");
#elif JUCE_WINDOWS
    obj->setProperty ("platform", "windows");
#else
    obj->setProperty ("platform", "linux");
#endif
    complete (juce::JSON::toString (juce::var (obj)));
}

// ============================================================================
// NetworkManager callbacks → push to JS
// ============================================================================

void MainComponent::onManifestReady (const juce::Array<PluginInfo>& plugins)
{
    {
        juce::ScopedLock sl (pluginDataLock);
        pluginData = plugins;
    }

    if (plugins.isEmpty())
    {
        emitStatusMessage ("Could not load plugins — check your connection", "error");
        return;
    }

    emitPluginsUpdated();

    int updates = 0;
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
            if (p.status == PluginStatus::UpdateAvailable || p.status == PluginStatus::NotInstalled)
                ++updates;
    }

    if (updates > 0)
        emitStatusMessage (juce::String (updates) + " update(s) available", "info");
    else
        emitStatusMessage ("All plugins up to date", "success");
}

void MainComponent::onManifestError (const juce::String& errorMessage)
{
    emitStatusMessage ("Offline — " + errorMessage, "error");
}

void MainComponent::onDownloadProgress (const juce::String& pluginId, double progress)
{
    // Update local state
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
        {
            if (p.id == pluginId)
            {
                p.downloadProgress = progress;
                break;
            }
        }
    }

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("pluginId", pluginId);
    obj->setProperty ("progress", progress);
    webView.emitEventIfBrowserIsVisible ("downloadProgress", juce::var (obj));
}

void MainComponent::onDownloadComplete (const juce::String& pluginId,
                                         const juce::File& localFile,
                                         bool success,
                                         const juce::String& errorMessage)
{
    if (success)
    {
        launchSilentInstaller (localFile, pluginId);
    }
    else
    {
        {
            juce::ScopedLock sl (pluginDataLock);
            for (auto& p : pluginData)
            {
                if (p.id == pluginId)
                {
                    p.status = PluginStatus::Error;
                    break;
                }
            }
        }

        emitPluginsUpdated();
        emitStatusMessage ("Download failed — " + errorMessage, "error");
    }
}

// ============================================================================
// Silent installer — runs in background thread
// ============================================================================

void MainComponent::launchSilentInstaller (const juce::File& installerFile,
                                            const juce::String& pluginId)
{
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
        {
            if (p.id == pluginId)
            {
                p.status = PluginStatus::Installing;
                break;
            }
        }
    }
    emitPluginsUpdated();
    emitStatusMessage ("Installing...", "info");

    auto filePath     = installerFile.getFullPathName();
    auto pid          = pluginId;
    juce::String regKey, remoteVer, vst3Bundle, auBundle, standaloneExe;

    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
        {
            if (p.id == pluginId)
            {
                regKey        = p.registryKey;
                remoteVer     = p.remoteVersion;
                vst3Bundle    = p.vst3Bundle;
                auBundle      = p.auBundle;
                standaloneExe = p.standaloneExe;
                break;
            }
        }
    }

    juce::Thread::launch ([this, filePath, pid, regKey, remoteVer,
                           vst3Bundle, auBundle, standaloneExe]
    {
        bool started = false;

    #if JUCE_MAC
        // Record what was already on disk BEFORE installation
        bool hadVst3Before       = VersionChecker::isVst3Installed (vst3Bundle);
        bool hadAUBefore         = VersionChecker::isAUInstalled (auBundle);
        bool hadStandaloneBefore = VersionChecker::isStandaloneInstalled (standaloneExe);

        juce::String cmd = juce::String ("osascript -e 'do shell script \"installer -pkg ")
                         + "\\\"" + filePath + "\\\""
                         + " -target /\" with administrator privileges'";

        DBG ("[Installer] Running: " + cmd);

        // Use system() instead of ChildProcess — ChildProcess can't show
        // the macOS admin password dialog from a background thread
        int exitCode = std::system (cmd.toRawUTF8());
        bool processFinished = true;
        started = true;
    #else
        juce::ChildProcess process;
        juce::String cmd = "\"" + filePath + "\" /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP-";
        started = process.start (cmd);

        if (! started)
        {
            DBG ("[Installer] Failed to start process");
            juce::MessageManager::callAsync ([this, pid] {
                for (auto& p : pluginData)
                    if (p.id == pid) { p.status = PluginStatus::Error; break; }
                emitPluginsUpdated();
                emitStatusMessage ("Failed to launch installer.", "error");
            });
            return;
        }

        bool processFinished = process.waitForProcessToFinish (120000);
        auto exitCode = process.getExitCode();
    #endif
        DBG ("[Installer] Process finished=" + juce::String (processFinished ? "YES" : "NO")
             + " exitCode=" + juce::String (exitCode));

        bool verified = false;

    #if JUCE_MAC
        if (processFinished && exitCode == 0)
        {
            // Check that something NEW was installed (not just pre-existing files)
            bool hasVst3Now       = VersionChecker::isVst3Installed (vst3Bundle);
            bool hasAUNow         = VersionChecker::isAUInstalled (auBundle);
            bool hasStandaloneNow = VersionChecker::isStandaloneInstalled (standaloneExe);

            bool somethingNew = (hasVst3Now && ! hadVst3Before)
                             || (hasAUNow && ! hadAUBefore)
                             || (hasStandaloneNow && ! hadStandaloneBefore);

            // If nothing new but all targets exist, still consider it verified
            // (re-installing over existing files)
            bool allTargetsPresent = (vst3Bundle.isEmpty()    || hasVst3Now)
                                  && (auBundle.isEmpty()      || hasAUNow)
                                  && (standaloneExe.isEmpty() || hasStandaloneNow);

            verified = somethingNew || allTargetsPresent;

            DBG ("[Installer] Verification: somethingNew=" + juce::String (somethingNew ? "YES" : "NO")
                 + " allTargetsPresent=" + juce::String (allTargetsPresent ? "YES" : "NO")
                 + " verified=" + juce::String (verified ? "YES" : "NO"));

            if (verified)
                VersionChecker::setInstalledVersion (regKey, remoteVer);
        }
        else
        {
            DBG ("[Installer] Installation failed or was cancelled (exitCode=" + juce::String (exitCode) + ")");
        }
    #else
        if (processFinished)
        {
            auto installedVer = VersionChecker::getInstalledVersion (regKey);
            verified = installedVer.isNotEmpty();
        }
    #endif

        juce::MessageManager::callAsync ([this, pid, verified, remoteVer, exitCode, processFinished]
        {
            juce::String pluginName;

            {
                juce::ScopedLock sl (pluginDataLock);
                for (auto& p : pluginData)
                {
                    if (p.id == pid)
                    {
                        if (verified)
                        {
                            p.installedVersion = VersionChecker::getInstalledVersion (p.registryKey);
                            if (p.installedVersion.isEmpty())
                                p.installedVersion = remoteVer;
                            p.status = PluginStatus::UpToDate;
                            pluginName = p.name;
                        }
                        else
                        {
                            p.status = PluginStatus::Error;
                        }

                        break;
                    }
                }
            }

            if (verified)
            {
                emitStatusMessage (pluginName + " installed successfully!", "success");

                // Auto-open standalone after install
            #if JUCE_MAC
                juce::String standaloneExeLocal;
                {
                    juce::ScopedLock sl (pluginDataLock);
                    for (auto& p : pluginData)
                        if (p.id == pid) { standaloneExeLocal = p.standaloneExe; break; }
                }
                if (standaloneExeLocal.isNotEmpty())
                {
                    auto appName = standaloneExeLocal.replace (".exe", "") + ".app";
                    juce::File app;
                    for (auto& dir : { juce::File ("/Applications"),
                                        juce::File ("/Applications/RONE Plugins"),
                                        VersionChecker::getStandaloneInstallDir() })
                    {
                        auto candidate = dir.getChildFile (appName);
                        if (candidate.exists()) { app = candidate; break; }
                    }
                    if (app.exists())
                        app.startAsProcess();
                }
            #endif
            }
            else
            {
                if (! processFinished)
                    emitStatusMessage ("Install timed out.", "error");
                else if (exitCode != 0)
                    emitStatusMessage ("Install cancelled or failed (code " + juce::String (exitCode) + "). Enter your password when prompted.", "error");
                else
                    emitStatusMessage ("Install verification failed — components not found.", "error");
            }

            // Push full updated state
            emitPluginsUpdated();
        });
    });
}
