#include "MainComponent.h"
#include "BinaryData.h"
#include "../../Shared/RemoteLicenseGate.h"
#include "CrashReportUploader.h"   // also brings in Shared/RoneCrashReporter.h

// Open mode (remote kill-switch OFF) counts as licensed everywhere:
// the C++ side and the web UI both key off this one predicate.
static bool isEffectivelyLicensed (const LicenseHandler& handler)
{
    return RemoteLicenseGate::isOpenMode() || handler.isLicensed();
}

#if JUCE_WINDOWS
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #include <windows.h>
#endif

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
        { "logos/RoneFlanger.png",    { BinaryData::RoneFlanger_icon_png,   BinaryData::RoneFlanger_icon_pngSize } },
        { "logos/RONEAnalyzer.png",   { BinaryData::RONEAnalyzer_icon_png,  BinaryData::RONEAnalyzer_icon_pngSize } },
        { "logos/RoneStucker.png",    { BinaryData::RoneStucker_icon_png,   BinaryData::RoneStucker_icon_pngSize } },
        { "logos/RoneAfterspace.png", { BinaryData::RoneAfterspace_icon_png, BinaryData::RoneAfterspace_icon_pngSize } },
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
                // AppData, not TEMP: anything the UI keeps in browser storage
                // survives disk cleanups, like every other RONE plugin's data.
                .withUserDataFolder (
                    juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                        .getChildFile ("RonePluginsCenter")
                        .getChildFile ("WebView2")))
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
        .withNativeFunction ("applyCenterUpdate", [this] (NativeArgs args, NativeCompletion complete) {
            handleApplyCenterUpdate (args, std::move (complete));
        })
        .withNativeFunction ("openExternalUrl", [] (NativeArgs args, NativeCompletion complete) {
            if (args.size() > 0)
                juce::URL (args[0].toString()).launchInDefaultBrowser();
            complete (juce::var ("ok"));
        })

        // ---- Resource provider ----
        // The localhost origin is a dev-server convenience only; release builds
        // must not extend the native bridge to anything that can bind that port.
        .withResourceProvider (
            [this] (const auto& url) { return getResource (url); }
           #if JUCE_DEBUG
            , juce::URL { "http://localhost:3000/" }.getOrigin()
           #endif
            ))
{
    // Crash & error reporting: the Center owns its process, so it installs the
    // crash handler, and it is the bundle's single uploader — drain whatever
    // the plugins/standalones queued since the last run.
    RoneCrashReporter::installCrashHandler ("RONE Plugins Center",
                                            JUCE_APPLICATION_VERSION_STRING,
                                            "Center");
    CrashReportUploader::uploadPendingAsync();

    addAndMakeVisible (webView);

#if JUCE_WINDOWS
    titleBar.startNativeDrag = [this] { return beginNativeWindowDrag(); };
    addAndMakeVisible (titleBar);
#endif

    setSize (920, 640);

    networkManager.addListener (this);

    // License handler
    licenseHandler.onLicenseStateChanged = [this] (bool isLicensed)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("licensed",     isLicensed || RemoteLicenseGate::isOpenMode());
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

void MainComponent::paint (juce::Graphics& g)
{
    // Shows through the grab strip below - same vertical ramp as the web UI's
    // body, so the strip reads as the window edge rather than a border.
    g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xff17191E), 0.0f,
                                                       juce::Colour (0xff14161A), (float) getHeight()));
    g.fillAll();
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    if (titleBar.isVisible())
        titleBar.setBounds (area.removeFromTop (CustomTitleBar::kHeight));

#if JUCE_WINDOWS
    // The WebView is a real child window, so anything it covers can never reach
    // the window's resize frame. Keep a thin strip clear along the edges the
    // title bar doesn't already leave open, so the window stays resizable.
    constexpr int grabStrip = 5;
    area = area.withTrimmedLeft   (grabStrip)
               .withTrimmedRight  (grabStrip)
               .withTrimmedBottom (grabStrip);
#endif

    webView.setBounds (area);
}

void MainComponent::parentHierarchyChanged()
{
    titleBar.setWindowToDrag (getTopLevelComponent());
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

    // Every user-visible error is also a queued report (throttled per message,
    // see RoneCrashReporter) — this is the "why didn't it install/open" feed
    // the devs see without the tester having to describe anything.
    if (type == "error")
    {
        RoneCrashReporter::reportError ("RONE Plugins Center",
                                        JUCE_APPLICATION_VERSION_STRING,
                                        "Center", "CENTER_ERROR", text);
        CrashReportUploader::uploadPendingAsync();
    }
}

// ============================================================================
// Native function handlers
// ============================================================================

static juce::String pendingCenterUpdateVersion (const NetworkManager::CenterInstallerInfo& info);

void MainComponent::handleGetPlugins (NativeArgs, NativeCompletion complete)
{
    auto result = allPluginsToVar();

    // Ride the update flag along with the catalog: at startup the UI pulls
    // this before any events can reach it, so a pending Center update must be
    // in the pulled payload too.
    const auto centerVersion = pendingCenterUpdateVersion (networkManager.getCenterInstallerInfo());
    if (centerVersion.isNotEmpty())
    {
        if (auto* obj = result.getDynamicObject())
        {
            auto* upd = new juce::DynamicObject();
            upd->setProperty ("version", centerVersion);
            obj->setProperty ("centerUpdate", juce::var (upd));
        }
    }

    complete (juce::JSON::toString (result));
}

void MainComponent::handleInstallPlugin (NativeArgs args, NativeCompletion complete)
{
    if (args.isEmpty())
    {
        complete ("{\"started\":false,\"error\":\"Missing plugin ID\"}");
        return;
    }

    if (! isEffectivelyLicensed (licenseHandler))
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
                    // `sha256_mac` carries the .pkg hash (empty only for a
                    // platform that wasn't rebuilt this run = "not verified").
                    networkManager.downloadInstaller (pluginId, p.downloadUrlMac, p.sha256Mac);
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

    if (! isEffectivelyLicensed (licenseHandler))
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
                emitStatusMessage ("License activated - all plugins unlocked!", "success");
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
    const bool effective = isEffectivelyLicensed (licenseHandler);

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("licensed",     effective);
    obj->setProperty ("customerName", licenseHandler.getCustomerName());
    obj->setProperty ("licenseKey",   licenseHandler.getLicenseKey());

    // When the remote kill-switch is engaged, surface its message (if any)
    // over the generic local one.
    auto lockMsg = (! effective) ? RemoteLicenseGate::getLockMessage() : juce::String();
    obj->setProperty ("message", lockMsg.isNotEmpty() ? lockMsg
                                                      : licenseHandler.getStatusMessage());
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
// Window controls — the title bar is a native component (CustomTitleBar), so
// the drag can be handed straight to the OS and keep Aero-snap behaviour.
// ============================================================================

bool MainComponent::beginNativeWindowDrag()
{
#if JUCE_WINDOWS
    if (auto* peer = getPeer())
    {
        if (auto hwnd = (HWND) peer->getNativeHandle())
        {
            POINT screenPos {};
            GetCursorPos (&screenPos);

            // Hand the drag to the OS so Aero Snap and multi-monitor work.
            //
            // WM_NCLBUTTONDOWN is not usable here: JUCE swallows it and only
            // defers to DefWindowProc once it sees a *non-client* mouse move,
            // which never arrives for a borderless window whose title bar
            // lives in the client area. SC_MOVE is passed straight through.
            ReleaseCapture();
            PostMessage (hwnd, WM_SYSCOMMAND, (WPARAM) (SC_MOVE | HTCAPTION),
                         MAKELPARAM (screenPos.x, screenPos.y));
            return true;
        }
    }
#endif
    return false;
}

// ============================================================================
// Center self-update — the Center treats itself like any catalog product:
// compare the version its installer stamped against center_installer in the
// manifest, download + SHA256-verify through the same pipeline as plugins,
// then hand the swap to a detached script (an exe cannot replace itself).
// ============================================================================

juce::String MainComponent::readInstalledCenterVersion()
{
#if JUCE_WINDOWS
    return juce::WindowsRegistry::getValue (
        "HKEY_CURRENT_USER\\Software\\RONE\\Plugins\\__center__\\InstalledVersion", {});
#else
    return {};
#endif
}

// Empty string = up to date (or unknowable); otherwise the catalog version.
static juce::String pendingCenterUpdateVersion (const NetworkManager::CenterInstallerInfo& info)
{
    if (! info.isValid())
        return {};

    const auto installed = MainComponent::readInstalledCenterVersion();

    if (installed.isNotEmpty())
        return installed == info.version ? juce::String() : info.version;   // catalog is truth

    // Installs older than this feature never stamped their version; only the
    // CMake base is known. A base change (which any release carrying this
    // feature makes) is detectable - same-base rebuilds are not.
    const juce::String base (JUCE_APPLICATION_VERSION_STRING);
    const bool sameBase = info.version == base || info.version.startsWith (base + ".");
    return sameBase ? juce::String() : info.version;
}

void MainComponent::checkForCenterUpdate()
{
    const auto version = pendingCenterUpdateVersion (networkManager.getCenterInstallerInfo());
    if (version.isEmpty())
        return;

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("version", version);
    webView.emitEventIfBrowserIsVisible ("centerUpdateAvailable", juce::var (obj));
}

void MainComponent::handleApplyCenterUpdate (NativeArgs, NativeCompletion complete)
{
#if ! JUCE_WINDOWS
    // Self-update is Windows-only for now; the manifest's center url/sha256
    // point at the Windows installer, so don't download it elsewhere.
    complete ("{\"started\":false,\"error\":\"Center self-update is Windows-only for now\"}");
    return;
#else
    const auto info = networkManager.getCenterInstallerInfo();

    if (! info.isValid())
    {
        complete ("{\"started\":false,\"error\":\"No update information yet - refresh first\"}");
        return;
    }

    emitStatusMessage ("Downloading Center update v" + info.version + "...", "info");
    networkManager.downloadInstaller ("__center__", info.url, info.sha256);
    complete ("{\"started\":true}");
#endif
}

void MainComponent::applyCenterUpdate (const juce::File& installerFile)
{
#if JUCE_WINDOWS
    const auto exePath = juce::File::getSpecialLocation (juce::File::currentExecutableFile)
                             .getFullPathName();

    // The script outlives this process: waits for our exit, runs the installer
    // elevated (one UAC prompt), then relaunches the exe - the old build if the
    // user declined UAC, the new one after a successful swap.
    juce::String script;
    script << "Wait-Process -Id " << (int) GetCurrentProcessId() << " -ErrorAction SilentlyContinue; "
           << "try { Start-Process -FilePath '" << installerFile.getFullPathName().replace ("'", "''")
           << "' -ArgumentList '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART' -Verb RunAs -Wait } catch { }; "
           << "Start-Process -FilePath '" << exePath.replace ("'", "''") << "'";

    juce::ChildProcess updater;
    const juce::StringArray cmd { "powershell.exe", "-NoProfile", "-WindowStyle", "Hidden",
                                  "-ExecutionPolicy", "Bypass", "-Command", script };

    if (updater.start (cmd, 0))   // 0 = capture nothing; fully detached
    {
        emitStatusMessage ("Restarting to finish the Center update...", "info");
        juce::Timer::callAfterDelay (600, []
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        });
    }
    else
    {
        emitStatusMessage ("Could not start the Center updater.", "error");
    }
#else
    juce::ignoreUnused (installerFile);
    emitStatusMessage ("Center self-update is Windows-only for now.", "info");
#endif
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
        emitStatusMessage ("Could not load plugins - check your connection", "error");
        return;
    }

    emitPluginsUpdated();
    checkForCenterUpdate();

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
    emitStatusMessage ("Offline - " + errorMessage, "error");
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
    if (pluginId == "__center__")
    {
        if (success)
            applyCenterUpdate (localFile);
        else
            emitStatusMessage (errorMessage.isNotEmpty() ? errorMessage
                                                         : juce::String ("Center update download failed."),
                               "error");
        return;
    }

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
        // NetworkManager already prefixes errors with "Download failed - " where
        // appropriate; just surface whatever it sent.
        emitStatusMessage (errorMessage.isNotEmpty() ? errorMessage
                                                     : juce::String ("Download failed."),
                           "error");
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
                    emitStatusMessage ("Install verification failed - components not found.", "error");
            }

            // Push full updated state
            emitPluginsUpdated();
        });
    });
}
