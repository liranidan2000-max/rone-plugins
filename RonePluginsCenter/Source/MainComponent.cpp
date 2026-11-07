#include "MainComponent.h"
#include "BinaryData.h"
#include "../../Shared/RemoteLicenseGate.h"
#include "../../Shared/RoneWebCompat.h"
#include "CrashReportUploader.h"   // also brings in Shared/RoneCrashReporter.h
#include "OsCrashScanner.h"
#include "AutoStart.h"
#include "PluginInUse.h"
#include "OldVersionCleaner.h"
#include "UserFolderMove.h"
#include "PluginUninstaller.h"
#include "DawDetector.h"

// Open mode (remote kill-switch OFF) counts as licensed everywhere:
// the C++ side and the web UI both key off this one predicate.
//
// Three ways to be unlocked, in the order they arrived historically: the
// remote kill-switch being open, a Lemon Squeezy serial (pre-account
// customers), or being signed in to a roneaudio.com account with an active
// pass — which is what new customers use.
static bool isEffectivelyLicensed (const LicenseHandler& handler)
{
    return RemoteLicenseGate::isOpenMode() || handler.isLicensed();
}

static bool isEffectivelyLicensed (const LicenseHandler& handler, const AccountClient& account)
{
    return isEffectivelyLicensed (handler) || account.getState().licensed;
}

// The same question for one plugin. Everything above unlocks the whole
// catalog; a LIFETIME licence unlocks exactly the plugin it was bought for,
// so anything that acts on a single card has to ask this one instead — a
// customer who paid for RONE Stucker must be able to install and open it.
//
// THE OWNERSHIP RULE, stated once for everyone who has to implement it: a
// product is owned when some WHOLE comma-separated token of the owned list,
// trimmed, equals the wanted id, trimmed, compared case-insensitively.
//
// BundleLicenseChecker::ownsProduct() and the web UI's PluginCard both decide
// it exactly that way. Nothing here may become a substring test: owned
// "RoneStutter" must not unlock "RoneStut", and a plugin called "RoneStuckerX"
// must not ride in on "RoneStucker".
static bool isEffectivelyLicensed (const LicenseHandler& handler,
                                   const AccountClient& account,
                                   const juce::String& pluginId)
{
    if (isEffectivelyLicensed (handler, account))
        return true;

    const auto wanted = pluginId.trim();

    if (wanted.isEmpty())
        return false;

    // The state is copied out first — iterating a temporary's member would
    // walk a StringArray that has already been destroyed.
    const auto s = account.getState();

    auto owned = s.ownedProducts;
    owned.trim();                 // tolerate "a, b , c" whoever wrote the list
    owned.removeEmptyStrings();

    return owned.contains (wanted, true);   // whole token, ignoring case
}

#if JUCE_WINDOWS
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #include <windows.h>
#endif

#if JUCE_MAC
 #include <unistd.h>
 #include <sys/mount.h>
#endif

// The home folder as "~" in what goes into an error report.
static juce::String withoutHome (const juce::String& text)
{
    return text.replace (juce::File::getSpecialLocation (juce::File::userHomeDirectory).getFullPathName(), "~");
}

// The installer hash of this platform: sha256_mac carries the .pkg's (empty for a
// leg that was not built), sha256 the Windows installer's.
static juce::String platformSha (const PluginInfo& p)
{
   #if JUCE_MAC
    return p.sha256Mac;
   #else
    return p.sha256;
   #endif
}

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
    if (ext == "webp") return "image/webp";
    if (ext == "svg")  return "image/svg+xml";
    if (ext == "json") return "application/json";
    if (ext == "ico")  return "image/x-icon";
    if (ext == "woff2") return "font/woff2";
    if (ext == "woff") return "font/woff";
    return "application/octet-stream";
}

// The page's files, its fonts, every plugin icon and every 3D unit come out of
// BinaryData by their original file names, so adding a plugin's artwork is a
// file in Resources/ and nothing here (it used to be a hand-kept table that
// every new plugin had to extend).
static const char* findEmbedded (const juce::String& fileName, int& size)
{
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        if (fileName == BinaryData::originalFilenames[i])
            return BinaryData::getNamedResource (BinaryData::namedResourceList[i], size);
    size = 0;
    return nullptr;
}

std::optional<juce::WebBrowserComponent::Resource>
MainComponent::getResource (const juce::String& url)
{
    auto path = url == "/" ? juce::String ("index.html")
                           : url.fromFirstOccurrenceOf ("/", false, false)
                                .upToFirstOccurrenceOf ("?", false, false);

    // /logos/<id>.png -> <id>_icon.png ; /units/<id>.webp -> unit_<id>.webp ; the rest by name.
    juce::String file;
    if (path.startsWith ("logos/") && path.endsWith (".png"))
        file = path.fromFirstOccurrenceOf ("logos/", false, false).dropLastCharacters (4) + "_icon.png";
    else if (path.startsWith ("units/") && path.endsWith (".webp"))
        file = "unit_" + path.fromFirstOccurrenceOf ("units/", false, false);
    else if (! path.containsChar ('/'))
        file = path;

    if (file.isEmpty() || file.contains (".."))
        return std::nullopt;

    int size = 0;
    const char* data = findEmbedded (file, size);
    if (data == nullptr || size <= 0)
        return std::nullopt;

    std::vector<std::byte> bytes ((size_t) size);
    std::memcpy (bytes.data(), data, (size_t) size);

    return juce::WebBrowserComponent::Resource {
        std::move (bytes),
        getMimeForExtension (file.fromLastOccurrenceOf (".", false, false).toLowerCase())
    };
}

// ============================================================================
// Construction
// ============================================================================

juce::WebBrowserComponent::Options MainComponent::makeWebOptions()
{
    return juce::WebBrowserComponent::Options{}
        .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options (
            juce::WebBrowserComponent::Options::WinWebView2{}
                // The page is rebuilt every time the window opens: paint the
                // ground graphite so the reload never flashes white.
                .withBackgroundColour (juce::Colour (0xff14161A))
                // AppData, not TEMP: anything the UI keeps in browser storage
                // survives disk cleanups, like every other RONE plugin's data.
                .withUserDataFolder (
                    juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                        .getChildFile ("RonePluginsCenter")
                        .getChildFile ("WebView2")))
        // On the Mac the page runs in the system WebKit; on an older macOS that
        // is an older Safari with no flex `gap` (Zanon, 2026-10-06). The same
        // document-start polyfill every plugin carries; a modern engine skips it.
        .withUserScript (RoneWebCompat::script())
        .withNativeIntegrationEnabled()

        // ---- JS → C++ native functions ----
        .withNativeFunction ("getPlugins", [this] (NativeArgs args, NativeCompletion complete) {
            handleGetPlugins (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("installPlugin", [this] (NativeArgs args, NativeCompletion complete) {
            handleInstallPlugin (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("openPlugin", [this] (NativeArgs args, NativeCompletion complete) {
            handleOpenPlugin (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("openManual", [this] (NativeArgs args, NativeCompletion complete) {
            handleOpenManual (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("openFolder", [this] (NativeArgs args, NativeCompletion complete) {
            handleOpenFolder (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("openInstallFolder", [this] (NativeArgs args, NativeCompletion complete) {
            handleOpenInstallFolder (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("refreshPlugins", [this] (NativeArgs args, NativeCompletion complete) {
            handleRefreshPlugins (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("activateLicense", [this] (NativeArgs args, NativeCompletion complete) {
            handleActivateLicense (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("deactivateLicense", [this] (NativeArgs args, NativeCompletion complete) {
            handleDeactivateLicense (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("getLicenseStatus", [this] (NativeArgs args, NativeCompletion complete) {
            handleGetLicenseStatus (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("accountSignIn", [this] (NativeArgs args, NativeCompletion complete) {
            handleAccountSignIn (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("accountSignOut", [this] (NativeArgs args, NativeCompletion complete) {
            handleAccountSignOut (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("accountGoogleSignIn", [this] (NativeArgs args, NativeCompletion complete) {
            handleAccountGoogleSignIn (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("accountGoogleCancel", [this] (NativeArgs args, NativeCompletion complete) {
            handleAccountGoogleCancel (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("getAccountStatus", [this] (NativeArgs args, NativeCompletion complete) {
            handleGetAccountStatus (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("getAppVersion", [this] (NativeArgs args, NativeCompletion complete) {
            handleGetAppVersion (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("getAutoStart", [this] (NativeArgs args, NativeCompletion complete) {
            handleGetAutoStart (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("setAutoStart", [this] (NativeArgs args, NativeCompletion complete) {
            handleSetAutoStart (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("applyCenterUpdate", [this] (NativeArgs args, NativeCompletion complete) {
            handleApplyCenterUpdate (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("getAnnouncements", [this] (NativeArgs args, NativeCompletion complete) {
            handleGetAnnouncements (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("getUpsell", [this] (NativeArgs args, NativeCompletion complete) {
            handleGetUpsell (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("getOptin", [this] (NativeArgs args, NativeCompletion complete) {
            handleOptin (args, guarded (std::move (complete)), false);
        })
        .withNativeFunction ("setOptin", [this] (NativeArgs args, NativeCompletion complete) {
            handleOptin (args, guarded (std::move (complete)), true);
        })
        .withNativeFunction ("scanOldVersions", [this] (NativeArgs args, NativeCompletion complete) {
            handleScanOldVersions (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("deleteOldVersions", [this] (NativeArgs args, NativeCompletion complete) {
            handleDeleteOldVersions (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("scanUserFolderMove", [this] (NativeArgs args, NativeCompletion complete) {
            handleScanUserFolderMove (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("moveToUserFolders", [this] (NativeArgs args, NativeCompletion complete) {
            handleMoveToUserFolders (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("uninstallPlugin", [this] (NativeArgs args, NativeCompletion complete) {
            handleUninstallPlugin (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("cancelInstall", [this] (NativeArgs args, NativeCompletion complete) {
            handleCancelInstall (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("getDaws", [this] (NativeArgs args, NativeCompletion complete) {
            handleGetDaws (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("getCenterSettings", [this] (NativeArgs args, NativeCompletion complete) {
            handleGetCenterSettings (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("setCenterSetting", [this] (NativeArgs args, NativeCompletion complete) {
            handleSetCenterSetting (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("openExternalUrl", [] (NativeArgs args, NativeCompletion complete) {
            if (args.size() > 0)
            {
                // Only http(s) reaches the OS launcher. Every call from the page passes
                // https://roneaudio.com; anything else - a file:// path, a shell scheme -
                // would otherwise be handed straight to the system handler.
                const auto roneUrl = args[0].toString();
                if (roneUrl.startsWithIgnoreCase ("https://") || roneUrl.startsWithIgnoreCase ("http://"))
                    juce::URL (roneUrl).launchInDefaultBrowser();
            }
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
            );
}

MainComponent::MainComponent()
{
    // Crash & error reporting: the Center owns its process, so it installs the
    // crash handler, and it is the bundle's single uploader — drain whatever
    // the plugins/standalones queued since the last run.
    RoneCrashReporter::installCrashHandler ("RONE Plugins Center",
                                            JUCE_APPLICATION_VERSION_STRING,
                                            "Center");
    OsCrashScanner::scanAsync();
    CrashReportUploader::uploadPendingAsync();

#if JUCE_WINDOWS
    titleBar.startNativeDrag = [this] { return beginNativeWindowDrag(); };
    addAndMakeVisible (titleBar);
#endif

    setSize (1200, 800);   // three plugin cards per row from the first launch

    networkManager.addListener (this);

    // Installs: everything downloaded goes in one batch behind one permission prompt.
    batcher.downloadsPending = [this] { return networkManager.hasPendingDownloads(); };
    batcher.onStatus = [this] (const juce::String& id, PluginStatus s, const juce::String& waitingFor)
    {
        onBatchStatus (id, s, waitingFor);
    };
    batcher.onResult = [this] (const InstallBatcher::Result& r) { onBatchResult (r); };
    batcher.onIdle   = [this] { flushTrayNotice(); applyCenterUpdateWhenIdle(); };

    // License handler
    licenseHandler.onLicenseStateChanged = [this] (bool isLicensed)
    {
        auto* obj = new juce::DynamicObject();
        juce::ignoreUnused (isLicensed);
        obj->setProperty ("licensed",     isEffectivelyLicensed (licenseHandler, accountClient));
        obj->setProperty ("customerName", licenseHandler.getCustomerName());
        obj->setProperty ("message",      licenseHandler.getStatusMessage());
        emitToPage ("licenseChanged", juce::var (obj));

        // Also push updated plugin data (license affects card state)
        emitPluginsUpdated();
    };
    licenseHandler.initialize();

    // Account sign-in shares the same UI surface as the serial: whichever one
    // unlocks the bundle, the cards and the account panel react the same way.
    accountClient.onStateChanged = [this]
    {
        const auto s = accountClient.getState();

        auto* obj = new juce::DynamicObject();
        obj->setProperty ("licensed",     isEffectivelyLicensed (licenseHandler, accountClient));
        obj->setProperty ("customerName", s.name.isNotEmpty() ? s.name : s.email);
        obj->setProperty ("message",      s.message);
        emitToPage ("licenseChanged", juce::var (obj));

        emitToPage ("accountChanged", accountStatusVar());
        emitPluginsUpdated();
        refreshDevChannel();      // signed in as Liran: the plug-ins in development appear; signed out: they go
    };
    accountClient.initialize();
    lastAccountCheckMs = juce::Time::currentTimeMillis();   // initialize() just asked the server

    // Audit R285 stage 1: an account file that claims a licence without the
    // server's signed copy is reported (and, in this release, still trusted).
    // Audit R125 stage 1: so is a sealed sign-in this Windows account cannot open.
    if (const auto note = accountClient.takeStartupNote(); note.isNotEmpty())
        RoneCrashReporter::reportError ("RONE Plugins Center", JUCE_APPLICATION_VERSION_STRING, "Center", "CENTER_NOTE", note);

    // Fetch manifest after a short delay to let the WebView initialize
    juce::Timer::callAfterDelay (500, [safe = juce::Component::SafePointer<MainComponent> (this)]
    {
        if (safe != nullptr)
            safe->fetchManifestNow();
    });

    // Did the Center update that was applied before this start land?
    reportCenterUpdateOutcome();

    // Once a minute: is a manifest check due (every 6 hours, sooner when offline)?
    startTimer (60 * 1000);
}

MainComponent::~MainComponent()
{
    stopTimer();
    networkManager.removeListener (this);
}

// ============================================================================
// Periodic checks
//
// Before 2.0 the manifest was read at start-up and on Refresh only, and the
// Center starts with Windows and lives in the tray - so it showed the versions
// of the morning it booted for days. Now: every six hours, when the window
// comes to the front after 15 minutes, and - offline - again after 1, 2, 5,
// 10 and then every 30 minutes until the network is back.
// ============================================================================
void MainComponent::fetchManifestNow()
{
    lastManifestAttemptMs = juce::Time::currentTimeMillis();
    networkManager.fetchManifest();
}

void MainComponent::timerCallback()
{
    // Audit R283: the crash queue went out only at start-up or after the Center's
    // own error, so a report a plugin queued while the Center sat in the tray for
    // days waited for days. Every 10 minutes (a pass on an empty queue costs a
    // directory listing, and the uploader limits itself to one pass a minute).
    if (++timerTicks % 10 == 0)
    {
        OsCrashScanner::scanAsync();
        CrashReportUploader::uploadPendingAsync();
    }

    // Audit R026 (review): a Center update parked behind busy plugins is woken by
    // the batch going idle or a failed download - not by an uninstall finishing.
    // Once a minute, a parked update that nothing woke goes ahead as soon as
    // nothing is working any more (applyCenterUpdateWhenIdle checks that).
    if (centerUpdateWanted)
        applyCenterUpdateWhenIdle();

    const auto now = juce::Time::currentTimeMillis();
    const auto sinceAttempt = now - lastManifestAttemptMs;

    static constexpr int offlineSteps[] = { 1, 2, 5, 10, 30 };   // minutes
    const bool offline = manifestFromCache || ! manifestEverLoaded;
    const juce::int64 due = offline ? (juce::int64) offlineSteps[juce::jmin (offlineRetries, 4)] * 60 * 1000
                                    : 6LL * 3600 * 1000;

    if (sinceAttempt >= due)
    {
        if (offline)
            ++offlineRetries;
        fetchManifestNow();
    }
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

    if (webView != nullptr)
        webView->setBounds (area);
}

void MainComponent::parentHierarchyChanged()
{
    titleBar.setWindowToDrag (getTopLevelComponent());
}

void MainComponent::setUiVisible (bool shouldBeLive)
{
    JUCE_ASSERT_MESSAGE_THREAD
    if (shouldBeLive == (webView != nullptr))
        return;

    ++webGeneration;
    pageReady = false;

    if (shouldBeLive)
    {
        // Navigate to the resource provider root (JUCE's internal scheme, not HTTP).
        webView = std::make_unique<juce::WebBrowserComponent> (makeWebOptions());
        addAndMakeVisible (*webView);
        resized();
        webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
    }
    else
    {
        removeChildComponent (webView.get());
        webView.reset();
    }
}

void MainComponent::setWindowActive (bool isActive)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("active", isActive);
    emitToPage ("windowActive", juce::var (obj));

    // Coming back to the Center - from the browser where a plugin was just
    // bought, from the tray or the taskbar - asks the server again what this
    // account owns. Without it a purchase made while the Center runs stayed
    // invisible until the once-a-day check, and closing the window only hides
    // it in the tray, so "close it and open it again" changed nothing (an Iron
    // buyer on 2026-10-05). At most once a minute.
    if (isActive)
    {
        const auto now = juce::Time::currentTimeMillis();
        if (now - lastAccountCheckMs >= 60 * 1000)
        {
            lastAccountCheckMs = now;
            accountClient.validateAsync();
        }

        // ...and the catalog, when the last look is more than 15 minutes old: a
        // fetch no longer disturbs a download, so there is nothing to wait for.
        if (now - lastManifestAttemptMs >= 15 * 60 * 1000)
            fetchManifestNow();
    }
}

MainComponent::NativeCompletion MainComponent::guarded (NativeCompletion complete)
{
    return [safeThis = juce::Component::SafePointer<MainComponent> (this),
            generation = webGeneration,
            complete = std::move (complete)] (juce::var result) mutable
    {
        auto settle = [safeThis, generation, complete, result]() mutable
        {
            if (safeThis != nullptr && safeThis->webView != nullptr && safeThis->webGeneration == generation)
                complete (result);
        };

        if (juce::MessageManager::getInstance()->isThisTheMessageThread())
            settle();
        else
            juce::MessageManager::callAsync (std::move (settle));
    };
}

void MainComponent::emitToPage (const juce::Identifier& eventId, const juce::var& payload)
{
    if (! juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        juce::MessageManager::callAsync ([safeThis = juce::Component::SafePointer<MainComponent> (this), eventId, payload]
        {
            if (safeThis != nullptr)
                safeThis->emitToPage (eventId, payload);
        });
        return;
    }

    if (webView != nullptr)
        webView->emitEventIfBrowserIsVisible (eventId, payload);
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
        case PluginStatus::Queued:          return "queued";
        case PluginStatus::Downloading:     return "downloading";
        case PluginStatus::ReadyToInstall:  return "ready";
        case PluginStatus::Installing:      return "installing";
        case PluginStatus::WaitingForHost:  return "waiting";
        case PluginStatus::Uninstalling:    return "uninstalling";
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
    obj->setProperty ("waitingFor",       info.waitingFor);
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
    obj->setProperty ("hasManual",           info.manualPdf.isNotEmpty());

    // The YouTube guide, for the plugins that have one. Absent stays absent:
    // the card's Manual chooser then leaves its video side switched off.
    if (info.videoUrl.startsWithIgnoreCase ("https://"))
        obj->setProperty ("videoUrl", info.videoUrl);

    // Individual LIFETIME price + where to buy it, under the manifest's own
    // key names. Set only when the manifest actually carries them: a plugin
    // that isn't sold on its own must arrive with no price at all, so the card
    // can stay silent instead of offering it for $0.
    if (! info.price.isVoid())       obj->setProperty ("price",        info.price);
    if (! info.launchPrice.isVoid()) obj->setProperty ("launch_price", info.launchPrice);
    if (info.storeUrl.isNotEmpty())  obj->setProperty ("store_url",    info.storeUrl);

    // ---- Center 2.0 ----
    // Free with any RONE account: before 2.0 the page never heard of it, and a
    // signed-out visitor saw RONE Clipper as LOCKED with no way in.
    if (info.free) obj->setProperty ("free", true);

    // A RONE app (RONE VST MANAGER): the page gives it its own gift card, never a
    // library card, and shows it only when this account is a customer.
    if (info.app)
    {
        obj->setProperty ("kind", "app");
        obj->setProperty ("unlocked", isRoneCustomer());
        // Only where this platform has an installer: RONE VST MANAGER went out on Windows first.
       #if JUCE_MAC
        obj->setProperty ("installable", info.downloadUrlMac.isNotEmpty());
       #else
        obj->setProperty ("installable", info.downloadUrl.isNotEmpty());
       #endif
    }
    if (info.dev)  obj->setProperty ("dev", true);    // a DEV CHANNEL card (Liran's own Center only)

    auto list = [] (const juce::StringArray& s)
    {
        juce::Array<juce::var> a;
        for (auto& x : s) a.add (x);
        return juce::var (a);
    };
    obj->setProperty ("categories", list (info.categories));
    obj->setProperty ("tags",       list (info.tags));
    if (info.accent.isNotEmpty())    obj->setProperty ("accent",   info.accent);
    if (info.released.isNotEmpty())  obj->setProperty ("released", info.released);
    if (info.sizeBytes > 0)          obj->setProperty ("sizeBytes", (double) info.sizeBytes);
    if (info.i18n.isObject())        obj->setProperty ("i18n", info.i18n);

    // Audio previews, https only (the page plays them straight from roneaudio.com)
    if (info.previewDry.startsWithIgnoreCase ("https://") && info.previewWet.startsWithIgnoreCase ("https://"))
    {
        auto* pv = new juce::DynamicObject();
        pv->setProperty ("dry", info.previewDry);
        pv->setProperty ("wet", info.previewWet);
        obj->setProperty ("preview", juce::var (pv));
    }

    // The 3D hardware unit, when this build carries it (Resources/units)
    int unitSize = 0;
    if (findEmbedded ("unit_" + info.id + ".webp", unitSize) != nullptr)
        obj->setProperty ("unitUrl", "/units/" + info.id + ".webp");

    // Downloaded in the background already: Update goes straight to installing.
    const auto pre = prefetched.find (info.id);
    if (pre != prefetched.end() && pre->second.sha256.equalsIgnoreCase (platformSha (info)) && pre->second.file.existsAsFile())
        obj->setProperty ("downloaded", true);

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
    // The catalog is built on the message thread: it reads the pre-download table.
    if (! juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<MainComponent> (this)]
        {
            if (safe != nullptr)
                safe->emitPluginsUpdated();
        });
        return;
    }

    emitToPage ("pluginsUpdated", allPluginsToVar());
    publishUpdateCount();

    // Who runs which version, for the admin console: the installed plug-ins ride along with /app/refresh.
    if (manifestEverLoaded)
    {
        auto* installed = new juce::DynamicObject();
        {
            juce::ScopedLock sl (pluginDataLock);
            for (auto& p : pluginData)
                if (p.installedVersion.isNotEmpty())
                    installed->setProperty (p.id, p.installedVersion);
        }
        accountClient.setInstalledVersions (juce::var (installed), JUCE_APPLICATION_VERSION_STRING);
    }
}

juce::var MainComponent::manifestStateVar() const
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("offline",  manifestFromCache);
    o->setProperty ("loaded",   manifestEverLoaded);
    o->setProperty ("syncedAt", (double) networkManager.getLastManifestSuccessMs());
    return juce::var (o);
}

// Plugins the user can act on that have a newer version. Not the ones that were
// never installed: before 2.0 "N updates" counted every plugin on sale.
int MainComponent::countUpdates()
{
    // Audit R119: only updates this account can install - the same gate as
    // startInstall and the page's own count. The tray said "3 updates ready" to a
    // signed-out or lapsed account while the page said everything was up to date.
    juce::ScopedLock sl (pluginDataLock);
    int n = 0;
    for (auto& p : pluginData)
        if (p.status == PluginStatus::UpdateAvailable && mayUse (p))
            ++n;
    return n;
}

// Who gets the gift apps is the server's call, signed into the licence: "vstm"
// (30.10.2026: a purchase, an ALL ACCESS Liran gave, or a story that tagged
// RONE), or the older "paid" (29.10: a purchase). RONE VST MANAGER unlocks on
// the same claims. No claim yet (a server before it, a
// serial without an account) = no card, never a card for an app that then
// stays in its trial.
bool MainComponent::isRoneCustomer()
{
    return accountClient.hasPaidLicence();
}

bool MainComponent::mayUse (const PluginInfo& p)
{
    if (p.app)
        return isRoneCustomer();

    // A free plugin is unlocked by any signed-in account (the server lists it in
    // `owned` too; this keeps it installable even before that answer arrives).
    return isEffectivelyLicensed (licenseHandler, accountClient, p.id)
        || (p.free && accountClient.getState().signedIn);
}

void MainComponent::publishUpdateCount()
{
    const int n = countUpdates();
    if (n == lastPublishedUpdates)
        return;
    lastPublishedUpdates = n;
    if (onUpdateCountChanged)
        onUpdateCountChanged (n);
}

// A status the Center shows that is nobody's bug: nothing to file in the
// crash tracker. Matched on the fixed prefixes the Center itself emits.
static bool isEnvironmentalStatus (const juce::String& text)
{
    return text.startsWith ("Offline - ")
        || text.startsWith ("Install cancelled")
        || text.contains ("a file was in use")
        || text.contains ("Enter your password when prompted")
        || text.contains ("installer code 2)")                            // Inno: cancelled by the user before installing
        || text.startsWith ("Could not reach the download server")         // R101, R123: GitHub's assets blocked, not a bug
        || text.startsWith ("Download interrupted")                        // R123: the connection dropped
        || text.startsWith ("Download failed - could not write")           // R284: a full disk
        || text.contains ("the download server is busy")                   // 2.1.26: GitHub's edge said 5xx (issue #82)
        || text.contains ("Windows needs a restart before")                 // R195: Inno exit 8
        || text.contains ("did not let the Center replace itself")        // R201: the designed Finder fallback
        || text.endsWith (": License required")                           // R246: the account's state, not a bug
        || text.endsWith (": Sign in required");
}

void MainComponent::emitStatusMessage (const juce::String& text, const juce::String& type,
                                       const juce::String& code, const juce::var& params,
                                       const juce::String& details)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("text", text);
    obj->setProperty ("type", type);
    if (code.isNotEmpty())
    {
        obj->setProperty ("code", code);
        obj->setProperty ("params", params.isObject() ? params : juce::var (new juce::DynamicObject()));
    }

    // No page yet (start-up, or the window is in the tray): kept for when it opens,
    // so "installed" or "the Center update did not finish" is not lost.
    if (! pageReady || webView == nullptr)
    {
        if (code == "installed")
        {
            // Audit R120: four installs in the tray came back as four toasts at once.
            pendingInstalled.add (juce::var (obj));
            while (pendingInstalled.size() > 50)
                pendingInstalled.remove (0);
        }
        else if (type != "info")
        {
            pendingMessages.add (juce::var (obj));
            while (pendingMessages.size() > 6)
                pendingMessages.remove (0);
        }
    }
    else
        emitToPage ("statusMessage", juce::var (obj));

    // Every user-visible error is also a queued report (throttled per message,
    // see RoneCrashReporter) — this is the "why didn't it install/open" feed
    // the devs see without the tester having to describe anything.
    //
    // Except the ones that are the tester's own situation, not ours: a machine
    // without a network at launch, an installer the user cancelled or that
    // could not replace a file a DAW still had open, the macOS password prompt
    // dismissed. 33 of the first 49 reports in the tracker were "Offline" -
    // that is noise that buries a real crash (1.3.8).
    //
    // The report carries the home folder as "~" (review of R195/R202): 2.1.4's
    // messages name the download folder and quote the installer's own log, and a
    // Windows user name has no place in the tracker. The page shows the full text.
    //
    // 2.1.26: `details` rides along (the reporter stores 8000 characters, the
    // tracker shows them) - the installer's exit code and log, the self-update's
    // last step, the machine's state - so a report can be read without writing
    // to the customer. The code stays CENTER_ERROR: the Support Center groups by
    // code + message, and a new code would split every open issue in two.
    if (type == "error" && ! isEnvironmentalStatus (text))
    {
        RoneCrashReporter::reportError ("RONE Plugins Center",
                                        JUCE_APPLICATION_VERSION_STRING,
                                        "Center", "CENTER_ERROR", withoutHome (text), withoutHome (details));
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

    if (auto* obj = result.getDynamicObject())
    {
        // Ride the update flag along with the catalog: at startup the UI pulls
        // this before any events can reach it, so a pending Center update must be
        // in the pulled payload too.
        const auto centerVersion = pendingCenterUpdateVersion (networkManager.getCenterInstallerInfo());
        if (centerVersion.isNotEmpty())
        {
            auto* upd = new juce::DynamicObject();
            upd->setProperty ("version", centerVersion);
            obj->setProperty ("centerUpdate", juce::var (upd));
        }

        // Offline / last sync, the tips of the week, and where a link asked to go.
        pageReady = true;
        if (onPageReady != nullptr)
        {
            // A moment for React to paint the library before the intro lets go.
            juce::Component::SafePointer<MainComponent> safe (this);
            juce::Timer::callAfterDelay (120, [safe] { if (safe != nullptr && safe->onPageReady) safe->onPageReady(); });
        }
        obj->setProperty ("manifest", manifestStateVar());
        obj->setProperty ("tips", networkManager.getManifestExtras().getProperty ("tips", juce::var()));
        if (pendingNavigation.isNotEmpty())
        {
            obj->setProperty ("navigate", pendingNavigation);
            pendingNavigation.clear();
        }
    }

    complete (juce::JSON::toString (result));

    // What happened while there was no page, shown once it has its catalog.
    // More than two installs read as one line (audit R120).
    if (pendingInstalled.size() > 2)
    {
        juce::StringArray names;
        for (auto& m : pendingInstalled)
        {
            const auto params = m.getProperty ("params", {});
            names.add (params.getProperty ("name", params.getProperty ("id", "").toString()).toString());
        }
        auto* params = new juce::DynamicObject();
        params->setProperty ("count", names.size());
        params->setProperty ("names", names.joinIntoString (", "));
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("text", juce::String (names.size()) + " plugins installed: " + names.joinIntoString (", ") + ".");
        obj->setProperty ("type", "success");
        obj->setProperty ("code", "installed_many");
        obj->setProperty ("params", juce::var (params));
        pendingMessages.insert (0, juce::var (obj));
    }
    else
        for (int i = pendingInstalled.size(); --i >= 0;)
            pendingMessages.insert (0, pendingInstalled.getReference (i));
    pendingInstalled.clear();

    if (! pendingMessages.isEmpty())
    {
        auto messages = pendingMessages;
        pendingMessages.clear();
        juce::Timer::callAfterDelay (900, [safe = juce::Component::SafePointer<MainComponent> (this), messages]
        {
            if (safe != nullptr)
                for (auto& m : messages)
                    safe->emitToPage ("statusMessage", m);
        });
    }
}

void MainComponent::handleInstallPlugin (NativeArgs args, NativeCompletion complete)
{
    if (args.isEmpty())
    {
        complete ("{\"started\":false,\"error\":\"Missing plugin ID\"}");
        return;
    }

    juce::String error;
    if (startInstall (args[0].toString(), error))
        complete ("{\"started\":true}");
    else
        complete ("{\"started\":false,\"error\":\"" + error + "\"}");
}

bool MainComponent::startInstall (const juce::String& pluginId, juce::String& error)
{
    JUCE_ASSERT_MESSAGE_THREAD

    bool known = false, isDev = false, allowed = false;
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
            if (p.id == pluginId) { isDev = p.dev; known = true; allowed = mayUse (p); break; }
    }

    if (known && ! allowed)
    {
        // Audit R246: a signed-out Center (a forced sign-out, a new computer) is
        // asked to sign in - not told to wait a minute for a purchase.
        error = accountClient.getState().signedIn ? "License required" : "Sign in required";
        return false;
    }

    if (! known)
    {
        error = "Plugin not found";
        return false;
    }

    juce::String url, sha, version;
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
        {
            if (p.id != pluginId)
                continue;

            if (isBusyStatus (p.status))
            {
                error = "Already on its way";
                return false;
            }

           #if JUCE_MAC
            // `sha256_mac` carries the .pkg hash (empty only for a platform
            // that wasn't rebuilt this run = "not verified").
            url = p.downloadUrlMac; sha = p.sha256Mac;
           #else
            url = p.downloadUrl;    sha = p.sha256;
           #endif
            version = p.remoteVersion;
            p.downloadProgress = 0.0;
            p.waitingFor = {};
            break;
        }
    }

    // Already downloaded in the background for this very file (same hash): install
    // it now (the batch hashes it once more before it runs). A pre-download of
    // another build - the card moved on since - is not it, and a fresh download
    // follows (applyCatalog drops it anyway).
    const auto pre = prefetched.find (pluginId);
    if (pre != prefetched.end() && pre->second.sha256.equalsIgnoreCase (sha) && sha.isNotEmpty()
        && pre->second.file.existsAsFile())
    {
        handBatcher (pluginId, pre->second.file, pre->second.sha256, version);
        return true;
    }

    // A background download of it is running: it simply installs when done.
    if (networkManager.isDownloadQueued (pluginId))
    {
        networkManager.promoteToInstall (pluginId);
        setStatus (pluginId, PluginStatus::Queued);
        return true;
    }

    setStatus (pluginId, PluginStatus::Queued);
    networkManager.downloadInstaller (pluginId, url, sha, version, false, isDev);
    return true;
}

bool MainComponent::anyPluginBusy()
{
    if (batcher.isBusy())
        return true;

    juce::ScopedLock sl (pluginDataLock);
    for (auto& p : pluginData)
        if (isBusyStatus (p.status))
            return true;
    return false;
}

bool MainComponent::isWorking()
{
    return anyPluginBusy() || networkManager.hasPendingDownloads();
}

void MainComponent::setStatus (const juce::String& id, PluginStatus status, const juce::String& waitingFor)
{
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
            if (p.id == id)
            {
                p.status = status;
                p.waitingFor = waitingFor;
                if (status != PluginStatus::Downloading)
                    p.downloadProgress = status == PluginStatus::Queued ? 0.0 : p.downloadProgress;
                break;
            }
    }
    emitPluginsUpdated();
}

// After a cancel or a failure: what is on disk decides the card again.
void MainComponent::restoreIdleState (const juce::String& id)
{
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
            if (p.id == id)
            {
                VersionChecker::refreshInstallState (p);
                p.waitingFor = {};
                p.downloadProgress = 0.0;
                break;
            }
    }
    emitPluginsUpdated();
}

void MainComponent::handBatcher (const juce::String& pluginId, const juce::File& installer,
                                 const juce::String& verifiedSha, const juce::String& version)
{
    // The item carries the identity the file was VERIFIED with (the job's hash and
    // version), not the card's live values. applyCatalog replaces the card on every
    // manifest refresh - including the one a release triggers minutes after the user
    // clicked Update - and a file installed under the card's NEW hash failed the
    // batch's second check as "the installer changed after it was checked" (issue
    // #5: 6 reports, 4 machines, Windows and Mac). The Center's own update has done
    // it this way since R025 (centerUpdateExpectedSha).
    InstallBatcher::Item item;
    juce::String liveSha, liveUrl, liveVersion;
    bool isDev = false;
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
            if (p.id == pluginId)
            {
                item.id            = p.id;
                item.name          = p.name;
                item.registryKey   = p.registryKey;
                item.version       = version.isNotEmpty() ? version : p.remoteVersion;
                item.sha256        = verifiedSha;
                item.vst3Bundle    = p.vst3Bundle;
                item.auBundle      = p.auBundle;
                item.standaloneExe = p.standaloneExe;
                liveSha            = platformSha (p);
               #if JUCE_MAC
                liveUrl            = p.downloadUrlMac;
               #else
                liveUrl            = p.downloadUrl;
               #endif
                liveVersion        = p.remoteVersion;
                isDev              = p.dev;
                break;
            }
    }

    if (item.id.isEmpty())
        return;

    prefetched.erase (pluginId);

    // The card moved to another build while this one downloaded. The verified
    // file would install fine, but the user would get the older build with the
    // newer one offered right after - so, once, fetch the newer one instead (the
    // same choice the stale-hash retry makes). No hash for this platform (a dev
    // entry whose leg was not built, the Windows-first gap) or already re-queued
    // once (a flapping manifest): back to idle, with a word.
    if (! liveSha.equalsIgnoreCase (verifiedSha))
    {
        auto* params = new juce::DynamicObject();
        params->setProperty ("name", item.name);
        if (liveSha.isEmpty() || liveUrl.isEmpty() || handoffRequeued.contains (pluginId))
        {
            handoffRequeued.removeString (pluginId);
            restoreIdleState (pluginId);
            emitStatusMessage ("A newer build of " + item.name + " is on its way - check again in a minute.",
                               "info", "newer_build_pending", juce::var (params));
            return;
        }
        handoffRequeued.add (pluginId);
        setStatus (pluginId, PluginStatus::Queued);
        emitStatusMessage ("A newer build of " + item.name + " just came out - downloading it.",
                           "info", "newer_build_downloading", juce::var (params));
        networkManager.downloadInstaller (pluginId, liveUrl, liveSha, liveVersion, false, isDev);
        return;
    }
    handoffRequeued.removeString (pluginId);

    item.installer = installer;
    setStatus (pluginId, PluginStatus::ReadyToInstall);
    batcher.add (item);
    batcher.poke();
}

void MainComponent::onBatchStatus (const juce::String& id, PluginStatus status, const juce::String& waitingFor)
{
    juce::String name = id;
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
            if (p.id == id) { name = p.name; break; }
    }

    setStatus (id, status, waitingFor);

    if (status == PluginStatus::WaitingForHost)
    {
        auto* params = new juce::DynamicObject();
        params->setProperty ("name", name);
        params->setProperty ("host", waitingFor);
        // "Close FL Studio" - a DAW keeps the plugin loaded until it quits, so
        // the program is named, not the plugin window.
        const auto text = waitingFor.isEmpty()
                              ? name + " is still open. Close it and the update installs by itself."
                              : name + " is loaded in " + waitingFor + ". Close " + waitingFor
                                     + " and the update installs by itself.";
        emitStatusMessage (text, "info", waitingFor.isEmpty() ? "waiting_own" : "waiting_host", juce::var (params));

        // Audit R120: in the tray the page is not there to say it - an info line
        // was dropped, and the update just waited without a word.
        if (! isShowing() && onTrayNotice)
            onTrayNotice (text);
    }
}

// What a batch did while the window was hidden, in one tray notification once it is done (R120).
void MainComponent::flushTrayNotice()
{
    if (trayInstalled.isEmpty() && trayFailed.isEmpty())
        return;

    juce::String text;
    if (trayInstalled.size() == 1)     text << trayInstalled[0] << " is installed.";
    else if (trayInstalled.size() > 1) text << trayInstalled.size() << " plugins installed.";
    if (trayFailed.size() > 0)
        text << (text.isNotEmpty() ? " " : "") << trayFailed.joinIntoString (", ")
             << " could not be installed - open the Center to see why.";

    trayInstalled.clear();
    trayFailed.clear();
    if (! isShowing() && onTrayNotice)
        onTrayNotice (text);
}

void MainComponent::onBatchResult (const InstallBatcher::Result& r)
{
    juce::String name = r.id, version;
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
            if (p.id == r.id)
            {
                name = p.name;
                version = p.remoteVersion;
                if (r.ok)
                {
                    p.installedVersion = VersionChecker::getInstalledVersion (p.registryKey);
                    if (p.installedVersion.isEmpty())
                        p.installedVersion = p.remoteVersion;
                    p.status = PluginStatus::UpToDate;
                }
                else
                    p.status = PluginStatus::Error;
                p.waitingFor = {};
                break;
            }
    }

    // A refused prompt is the user's choice, not a failure: back to what is on disk.
    if (r.declined)
        restoreIdleState (r.id);
    else
        emitPluginsUpdated();

    auto* params = new juce::DynamicObject();
    params->setProperty ("name", name);
    params->setProperty ("id", r.id);
    params->setProperty ("version", version);
    params->setProperty ("exitCode", r.exitCode);
    // 2.1.26: the program that holds the files and the file Inno could not
    // replace, for the page's in_use_host / install_locked lines.
    if (r.heldBy.isNotEmpty())
        params->setProperty ("host", r.heldBy);
    if (r.lockedFile.isNotEmpty())
        params->setProperty ("file", juce::File (r.lockedFile).getFileName());

    // What the installer itself said (Inno's log, the Mac installer's stderr -
    // audit R195, R281): on the page beside the translated line, and in the report.
    auto message = r.message;
    if (r.reason.isNotEmpty())
    {
        params->setProperty ("reason", r.reason);
        message << " (" << r.reason << ")";
    }

    // The report's details (2.1.26, issue #9): everything the page gets plus the
    // machine's state the elevated copy saw - diagnosable without a reply from
    // the customer. Only for a failure; never shown on the page.
    juce::String details;
    if (! r.ok && ! r.declined)
    {
        auto* d = new juce::DynamicObject();
        d->setProperty ("id", r.id);
        d->setProperty ("version", version);
        d->setProperty ("code", r.code);
        d->setProperty ("exitCode", r.exitCode);
        if (r.reason.isNotEmpty())        d->setProperty ("reason", r.reason);
        if (r.lockedFile.isNotEmpty())    d->setProperty ("lockedFile", r.lockedFile);
        if (r.heldBy.isNotEmpty())        d->setProperty ("heldBy", r.heldBy);
        if (r.diag.isNotEmpty())          d->setProperty ("machine", r.diag);
        if (r.integrityNote.isNotEmpty()) d->setProperty ("integrity", r.integrityNote);
        details = juce::JSON::toString (juce::var (d), true);
    }

    emitStatusMessage (message, r.ok ? "success" : (r.declined ? "info" : "error"), r.code, juce::var (params), details);

    // Audit R024 / R048 stage 1: an install job that did not match its hash, or a
    // staging folder that is not what it should be - reported, nothing refused.
    if (r.integrityNote.isNotEmpty())
        RoneCrashReporter::reportError ("RONE Plugins Center", JUCE_APPLICATION_VERSION_STRING, "Center", "CENTER_INTEGRITY",
                                        r.integrityNote);

    if (! isShowing() && ! r.declined)
        (r.ok ? trayInstalled : trayFailed).addIfNotAlreadyThere (name);

    // 2.0.6: nothing opens by itself after an install any more. On the Mac every
    // standalone that 1.x/2.0 launched here asked for the microphone the first
    // time it ran, so installing a few plugins became a string of permission
    // prompts nobody asked for (Liran on his Mac, 2026-10-07). A standalone now
    // opens - and asks - only when the user opens it (the card's Open).
}

void MainComponent::requestUpdate (const juce::String& productId)
{
    JUCE_ASSERT_MESSAGE_THREAD
    if (productId.isEmpty())
        return;

    pendingUpdateId = productId;

    bool knowsUpdate = false, haveData = false;
    {
        juce::ScopedLock sl (pluginDataLock);
        haveData = ! pluginData.isEmpty();
        for (auto& p : pluginData)
            if (p.id == productId)
                knowsUpdate = p.status == PluginStatus::UpdateAvailable || isBusyStatus (p.status);
    }

    // The Center may have sat in the tray since before the release the plugin
    // just saw: read the manifest again first (a fetch no longer disturbs a
    // download). With no data yet the start-up fetch is already on its way and
    // serves the request when it lands.
    if (knowsUpdate)
        servePendingUpdate();
    else if (haveData)
        fetchManifestNow();
}

void MainComponent::servePendingUpdate()
{
    if (pendingUpdateId.isEmpty())
        return;

    const auto id = pendingUpdateId;
    pendingUpdateId.clear();

    PluginStatus status = PluginStatus::NotInstalled;
    juce::String name;
    bool found = false;
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
            if (p.id == id) { status = p.status; name = p.name; found = true; break; }
    }
    if (! found)
        return;

    auto* params = new juce::DynamicObject();
    params->setProperty ("name", name);
    params->setProperty ("id", id);

    if (status == PluginStatus::UpdateAvailable || status == PluginStatus::Error || status == PluginStatus::NotInstalled)
    {
        juce::String error;
        if (startInstall (id, error))
            emitStatusMessage ("Updating " + name + "...", "info", "updating", juce::var (params));
        else
            emitStatusMessage (name + ": " + error, "error", error == "License required" ? "license_required"
                                                           : error == "Sign in required" ? "sign_in_required"
                                                                                         : juce::String(),
                               juce::var (params));
    }
    else if (status == PluginStatus::UpToDate)
    {
        // The new version is on disk while the DAW still runs the old one (macOS
        // replaces loaded bundles); a DAW loads a plugin's code once per session.
        emitStatusMessage (name + " is already up to date - restart your DAW to load the new version.", "success",
                           "already_current", juce::var (params));
    }
    // Downloading / installing / waiting: it is already on its way.
}

// ============================================================================
// ronecenter:// links (CenterLinks.h)
// ============================================================================
void MainComponent::handleLink (const CenterLinks::Link& link)
{
    JUCE_ASSERT_MESSAGE_THREAD
    if (! link.isValid())
        return;

    if (link.action == "plugin" || link.action == "install")
        pendingNavigation = "plugin:" + link.productId;
    else if (link.action == "updates")
        pendingNavigation = "updates";

    // A page that is up takes it now; one still loading pulls it with getPlugins.
    if (pendingNavigation.isNotEmpty() && pageReady && webView != nullptr)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("to", pendingNavigation);
        emitToPage ("navigate", juce::var (o));
        pendingNavigation.clear();
    }

    if (link.action != "install")
        return;

    // The purchase is seconds old: ask the server what this account owns first,
    // then install - or, if the payment is still on its way, say so on the page.
    const auto id = link.productId;
    lastAccountCheckMs = juce::Time::currentTimeMillis();
    accountClient.validateAsync ([safe = juce::Component::SafePointer<MainComponent> (this), id] (bool)
    {
        if (safe == nullptr)
            return;

        bool known = false;
        {
            juce::ScopedLock sl (safe->pluginDataLock);
            for (auto& p : safe->pluginData)
                if (p.id == id) { known = true; break; }
        }

        if (! known)
        {
            // The manifest is not in yet (a cold start from the link): its arrival serves it.
            safe->pendingUpdateId = id;
            return;
        }

        safe->pendingUpdateId = id;
        safe->servePendingUpdate();
    });
}

// ============================================================================
// Open the plugin's user manual. The PDF ships inside the installer
// (Windows: <Program Files>\RONE Plugins\Manuals, macOS: /Users/Shared/RONE
// Plugins/Manuals, or Application Support for a plugin the Center 2.1 put in the
// user's folders); when the local copy is missing (older install, manual
// deleted) the same file is served from the public monorepo.
// ============================================================================
void MainComponent::handleOpenManual (NativeArgs args, NativeCompletion complete)
{
    if (args.size() < 1)
    {
        complete ("{\"success\":false,\"error\":\"Missing plugin id\"}");
        return;
    }

    auto pluginId = args[0].toString();
    juce::String manualPdf;
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
            if (p.id == pluginId) { manualPdf = p.manualPdf; break; }
    }

    if (manualPdf.isEmpty())
    {
        complete ("{\"success\":false,\"error\":\"No manual is available for this plugin yet\"}");
        return;
    }

   #if JUCE_MAC
    juce::File local = VersionChecker::getUserManualsDir().getChildFile (manualPdf);   // the newer one, when both
    if (! local.existsAsFile())
        local = juce::File ("/Users/Shared/RONE Plugins/Manuals").getChildFile (manualPdf);
   #else
    juce::File local = VersionChecker::getStandaloneInstallDir().getChildFile ("Manuals").getChildFile (manualPdf);
   #endif

    if (local.existsAsFile())
    {
        local.startAsProcess();
        complete ("{\"success\":true,\"source\":\"local\"}");
        return;
    }

    juce::URL online ("https://github.com/liranidan2000-max/rone-plugins/raw/main/docs/manuals/"
                      + juce::URL::addEscapeChars (manualPdf, false));
    online.launchInDefaultBrowser();
    complete ("{\"success\":true,\"source\":\"online\"}");
}

// ============================================================================
// "Open Folder": reveal where the plugin actually lives - the VST3 bundle
// (or the AU on macOS), falling back to the standalone - in Explorer/Finder.
// ============================================================================
void MainComponent::handleOpenFolder (NativeArgs args, NativeCompletion complete)
{
    if (args.size() < 1)
    {
        complete ("{\"success\":false,\"error\":\"Missing plugin id\"}");
        return;
    }

    auto pluginId = args[0].toString();
    juce::String vst3, au, exe;
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
            if (p.id == pluginId) { vst3 = p.vst3Bundle; au = p.auBundle; exe = p.standaloneExe; break; }
    }

    juce::Array<juce::File> candidates;
   #if JUCE_MAC
    // System-wide or in the user's folders (Center 2.1)
    candidates.add (VersionChecker::findVst3 (vst3));
    candidates.add (VersionChecker::findAU (au));
    candidates.add (VersionChecker::findApp (exe));
   #else
    if (vst3.isNotEmpty())
    {
        candidates.add (VersionChecker::getVst3InstallDir().getChildFile (vst3));
       #if JUCE_WINDOWS
        candidates.add (VersionChecker::getVst3InstallDir().getParentDirectory().getChildFile (vst3)); // top-level VST3 folder
       #endif
    }
    if (exe.isNotEmpty()) candidates.add (VersionChecker::getStandaloneInstallDir().getChildFile (exe));
   #endif

    for (auto& f : candidates)
    {
        if (f != juce::File() && f.exists())
        {
            f.revealToUser();
            complete ("{\"success\":true}");
            return;
        }
    }

    complete ("{\"success\":false,\"error\":\"Nothing installed on disk for this plugin yet\"}");
}

// ============================================================================
// Settings > OPEN FOLDER: the folders every RONE installer writes to - the
// VST3 plugins, the standalone apps (with the manuals beside them), and on
// macOS the Audio Units. Opens the folder itself, not its parent.
// ============================================================================
#if JUCE_MAC
static bool holdsRone (const juce::File& dir)
{
    if (! dir.isDirectory()) return false;
    for (const auto& e : juce::RangedDirectoryIterator (dir, false, "*", juce::File::findFilesAndDirectories))
        if (e.getFile().getFileName().startsWithIgnoreCase ("RONE "))
            return true;
    return false;
}
#endif

void MainComponent::handleOpenInstallFolder (NativeArgs args, NativeCompletion complete)
{
    const auto kind = args.size() > 0 ? args[0].toString() : juce::String ("vst3");

    juce::File dir;
    if (kind == "standalone")
    {
       #if JUCE_MAC
        dir = juce::File ("/Applications/RONE Plugins");
        if (! dir.isDirectory()) dir = juce::File ("/Applications");
        if (! holdsRone (dir) && holdsRone (VersionChecker::getUserAppsDir()))
            dir = VersionChecker::getUserAppsDir();
       #else
        dir = VersionChecker::getStandaloneInstallDir();
       #endif
    }
    else if (kind == "au")
        dir = VersionChecker::getAUInstallDir();
    else
        dir = VersionChecker::getVst3InstallDir();

   #if JUCE_MAC
    // Center 2.1 installs into the user's folders what was not system-wide:
    // open the one that holds the RONE plugins.
    if (kind == "au" && ! holdsRone (dir) && holdsRone (VersionChecker::getUserAUDir()))
        dir = VersionChecker::getUserAUDir();
    if (kind != "au" && kind != "standalone" && ! holdsRone (dir) && holdsRone (VersionChecker::getUserVst3Dir()))
        dir = VersionChecker::getUserVst3Dir();
   #endif

    if (dir == juce::File() || ! dir.isDirectory())
    {
        complete ("{\"success\":false,\"error\":\"That folder does not exist yet - install a plugin first\"}");
        return;
    }

    dir.startAsProcess();
    complete ("{\"success\":true}");
}

void MainComponent::handleOpenPlugin (NativeArgs args, NativeCompletion complete)
{
    if (args.isEmpty())
    {
        complete ("{\"success\":false,\"error\":\"Missing plugin ID\"}");
        return;
    }

    auto pluginId = args[0].toString();

    bool allowed = false;
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
            if (p.id == pluginId) { allowed = mayUse (p); break; }
    }

    if (! allowed)
    {
        complete ("{\"success\":false,\"error\":\"License required\"}");
        return;
    }

    juce::ScopedLock sl (pluginDataLock);
    for (auto& p : pluginData)
    {
        if (p.id == pluginId)
        {
        #if JUCE_MAC
            if (p.standaloneExe.isNotEmpty())
            {
                const auto app = VersionChecker::findApp (p.standaloneExe);   // /Applications or ~/Applications

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
    // Safe at any moment now: a fetch never touches a running download.
    offlineRetries = 0;
    fetchManifestNow();
    complete ("{\"success\":true}");
}

// ============================================================================
// Center 2.0 handlers
// ============================================================================

// Cancel one plugin (queued, downloading, ready or waiting for its DAW) - or,
// with no id, everything that has not started installing.
void MainComponent::handleCancelInstall (NativeArgs args, NativeCompletion complete)
{
    juce::StringArray ids;
    if (args.size() > 0 && args[0].toString().isNotEmpty())
        ids.add (args[0].toString());
    else
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
            if (p.status == PluginStatus::Queued || p.status == PluginStatus::Downloading
                || p.status == PluginStatus::ReadyToInstall || p.status == PluginStatus::WaitingForHost)
                ids.add (p.id);
    }

    int cancelled = 0;
    for (auto& id : ids)
    {
        PluginStatus s = PluginStatus::NotInstalled;
        {
            juce::ScopedLock sl (pluginDataLock);
            for (auto& p : pluginData)
                if (p.id == id) { s = p.status; break; }
        }

        if (s == PluginStatus::Queued || s == PluginStatus::Downloading)
        {
            networkManager.cancelDownload (id);
            restoreIdleState (id);
            ++cancelled;
        }
        else if ((s == PluginStatus::ReadyToInstall || s == PluginStatus::WaitingForHost) && batcher.cancel (id))
        {
            restoreIdleState (id);
            ++cancelled;
        }
    }

    batcher.poke();

    auto* o = new juce::DynamicObject();
    o->setProperty ("cancelled", cancelled);
    complete (juce::var (o));
}

void MainComponent::handleGetDaws (NativeArgs, NativeCompletion complete)
{
    complete (DawDetector::toVar (DawDetector::find()));
}

void MainComponent::handleGetCenterSettings (NativeArgs, NativeCompletion complete)
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("backgroundDownload", CenterSettings::getBool (CenterSettings::backgroundDownload, true));
    o->setProperty ("autoStartSupported", AutoStart::isSupported());
    o->setProperty ("autoStart",          AutoStart::isEnabled());
    complete (juce::var (o));
}

void MainComponent::handleSetCenterSetting (NativeArgs args, NativeCompletion complete)
{
    const auto key = args.size() > 0 ? args[0].toString() : juce::String();
    const bool value = args.size() > 1 && (bool) args[1];

    if (key == CenterSettings::backgroundDownload.toString())
    {
        CenterSettings::setBool (CenterSettings::backgroundDownload, value);
        if (value)
            startBackgroundDownloads();
        else
        {
            // Stop what is fetching on its own; what the user asked for keeps going.
            juce::StringArray stop;
            {
                juce::ScopedLock sl (pluginDataLock);
                for (auto& p : pluginData)
                    if (networkManager.isPrefetch (p.id))
                        stop.add (p.id);
            }
            for (auto& id : stop)
                networkManager.cancelDownload (id);
        }
    }
    else if (key == "autoStart")
        AutoStart::setEnabled (value);

    handleGetCenterSettings ({}, std::move (complete));
}

// Updates of plugins this account can install are fetched quietly, one at a
// time, behind anything the user asked for; the card then reads "downloaded"
// and Update goes straight to installing. Never a plugin that is not installed,
// never anything locked, and never an install - that always waits for the user.
void MainComponent::startBackgroundDownloads()
{
    if (! CenterSettings::getBool (CenterSettings::backgroundDownload, true) || manifestFromCache)
        return;

    struct Want { juce::String id, url, sha, version; bool dev; };
    juce::Array<Want> wants;
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
        {
            if (p.status != PluginStatus::UpdateAvailable || p.installedVersion == "?")
                continue;
            if (! mayUse (p))
                continue;
           #if JUCE_MAC
            wants.add ({ p.id, p.downloadUrlMac, p.sha256Mac, p.remoteVersion, p.dev });
           #else
            wants.add ({ p.id, p.downloadUrl, p.sha256, p.remoteVersion, p.dev });
           #endif
        }
    }

    for (auto& w : wants)
    {
        const auto pre = prefetched.find (w.id);
        if (pre != prefetched.end() && pre->second.sha256.equalsIgnoreCase (w.sha) && pre->second.file.existsAsFile())
            continue;
        if (w.sha.isEmpty() || networkManager.isDownloadQueued (w.id))
            continue;
        networkManager.downloadInstaller (w.id, w.url, w.sha, w.version, true, w.dev);
    }
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

            emitToPage ("licenseActivationResult", juce::var (obj));

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
            emitToPage ("licenseDeactivationResult", juce::var (obj));
        });
    });
}

void MainComponent::handleGetLicenseStatus (NativeArgs, NativeCompletion complete)
{
    const bool effective = isEffectivelyLicensed (licenseHandler, accountClient);
    const auto account = accountClient.getState();

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("licensed",     effective);
    // A signed-in account owns the identity shown in the UI; the serial path
    // only supplies a name when no account is in play.
    obj->setProperty ("customerName", account.signedIn && account.name.isNotEmpty()
                                          ? account.name
                                          : (account.signedIn ? account.email
                                                              : licenseHandler.getCustomerName()));
    obj->setProperty ("licenseKey",   licenseHandler.getLicenseKey());

    // When the remote kill-switch is engaged, surface its message (if any)
    // over the generic local one.
    auto lockMsg = (! effective) ? RemoteLicenseGate::getLockMessage() : juce::String();
    obj->setProperty ("message", lockMsg.isNotEmpty()
                                     ? lockMsg
                                     : (account.signedIn ? account.message
                                                         : licenseHandler.getStatusMessage()));
    complete (juce::JSON::toString (juce::var (obj)));
}

// ============================================================================
// Account sign-in (roneaudio.com)
// ============================================================================

juce::var MainComponent::accountStatusVar() const
{
    const auto s = accountClient.getState();

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("signedIn",    s.signedIn);
    obj->setProperty ("licensed",    s.licensed);
    obj->setProperty ("email",       s.email);
    obj->setProperty ("name",        s.name);
    obj->setProperty ("plan",        s.plan);
    obj->setProperty ("deviceLimit", s.deviceLimit);
    obj->setProperty ("message",     s.message);

    // The plugins bought outright, as canonical ids. `licensed` stays
    // ALL-ACCESS-only, so this list is the only way the UI can tell a LIFETIME
    // card from one that is still for sale.
    juce::Array<juce::var> owned;
    for (const auto& id : s.ownedProducts)
        owned.add (id);
    obj->setProperty ("owned", owned);

    // Who bills the live pass: "comp" is a gift Liran gave by hand, and its
    // holder is never shown a deal popup (ui/src/announcements.js). Empty =
    // no live pass, a giveaway trial, or not heard from the server yet.
    obj->setProperty ("passSource",  s.passSource);

    // Dates cross the bridge as milliseconds; 0 means "not applicable".
    obj->setProperty ("expiresAt",   (double) s.expiresAt);
    obj->setProperty ("renewsAt",    (double) s.renewsAt);
    return juce::var (obj);
}

void MainComponent::handleAccountSignIn (NativeArgs args, NativeCompletion complete)
{
    if (args.size() < 2)
    {
        complete ("{\"ok\":false,\"message\":\"Enter your email and password\"}");
        return;
    }

    const auto email    = args[0].toString();
    const auto password = args[1].toString();

    auto shared = std::make_shared<NativeCompletion> (std::move (complete));

    accountClient.signIn (email, password, [this, shared] (bool success, juce::String message)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("ok",      success);
        obj->setProperty ("message", message);
        obj->setProperty ("account", accountStatusVar());
        (*shared) (juce::JSON::toString (juce::var (obj)));
    });
}

void MainComponent::handleAccountGoogleSignIn (NativeArgs, NativeCompletion complete)
{
    auto shared = std::make_shared<NativeCompletion> (std::move (complete));

    accountClient.signInWithGoogle ([this, shared] (bool success, juce::String message)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("ok",      success);
        obj->setProperty ("message", message);
        obj->setProperty ("account", accountStatusVar());
        (*shared) (juce::JSON::toString (juce::var (obj)));
    });
}

void MainComponent::handleAccountGoogleCancel (NativeArgs, NativeCompletion complete)
{
    accountClient.cancelGoogleSignIn();
    complete (juce::var ("ok"));
}

void MainComponent::handleAccountSignOut (NativeArgs, NativeCompletion complete)
{
    accountClient.signOut ([complete = std::move (complete)] (bool, juce::String message) mutable
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("ok",      true);
        obj->setProperty ("message", message);
        complete (juce::JSON::toString (juce::var (obj)));
    });
}

void MainComponent::handleGetAccountStatus (NativeArgs, NativeCompletion complete)
{
    complete (juce::JSON::toString (accountStatusVar()));
}

// ============================================================================
// Start with Windows / Open at login (AutoStart.h). The OS entry is the
// source of truth, so the toggle always shows what will really happen.
// ============================================================================
void MainComponent::handleGetAutoStart (NativeArgs, NativeCompletion complete)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("supported", AutoStart::isSupported());
    obj->setProperty ("enabled",   AutoStart::isEnabled());
    complete (juce::JSON::toString (juce::var (obj)));
}

void MainComponent::handleSetAutoStart (NativeArgs args, NativeCompletion complete)
{
    const bool wanted = args.size() > 0 && (bool) args[0];
    const bool ok = AutoStart::setEnabled (wanted);
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("success",   ok);
    obj->setProperty ("enabled",   AutoStart::isEnabled());
    obj->setProperty ("supported", AutoStart::isSupported());
    if (! ok) obj->setProperty ("error", "Could not change the login entry");
    complete (juce::JSON::toString (juce::var (obj)));
}

void MainComponent::handleGetAnnouncements (NativeArgs, NativeCompletion complete)
{
    // Off the message thread; `complete` is guarded(), so it lands on the message
    // thread and is dropped if the window closed meanwhile. Offline or a bad
    // answer = {"ok": false} and the page simply shows nothing.
    juce::Thread::launch ([complete = std::move (complete)]() mutable
    {
        juce::URL url (juce::String (RONE_API_BASE) + "/popup?surface=center");
        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                           .withConnectionTimeoutMs (8000)
                           .withExtraHeaders ("Accept: application/json");

        juce::var result;
        if (auto stream = url.createInputStream (options))
            result = juce::JSON::parse (stream->readEntireStreamAsString());

        if (! result.isObject())
        {
            auto* none = new juce::DynamicObject();
            none->setProperty ("ok", false);
            result = juce::var (none);
        }
        complete (result);
    });
}

void MainComponent::handleGetUpsell (NativeArgs, NativeCompletion complete)
{
    // This account's own prices (roneaudio.com/api/v1/upsell, worker/_lib/upsell.js on the site): the 24-hour
    // "one more for $19" after a purchase, the owner price on every plugin it lacks, and a signed /owner link for
    // each. Asked with the device token, off the message thread. Signed out, offline or a bad answer =
    // {"ok": false}: the page shows the ordinary prices.
    juce::Thread::launch ([job = accountClient.authorizedGet ("/upsell"), complete = std::move (complete)]() mutable
    {
        auto result = job();
        if (! result.isObject())
        {
            auto* none = new juce::DynamicObject();
            none->setProperty ("ok", false);
            result = juce::var (none);
        }
        complete (result);
    });
}

void MainComponent::handleOptin (NativeArgs args, NativeCompletion complete, bool write)
{
    // The deals list (roneaudio.com/api/v1/optin, Liran's sales plan 14.10: Black Friday early access): read it, or
    // join / leave with this machine's device token - the switch in Settings and the one-time banner ask.
    // Signed out, offline or a bad answer = {"ok": false}.
    std::function<juce::var()> job;
    if (write)
    {
        auto* body = new juce::DynamicObject();
        body->setProperty ("on", args.size() > 0 && (bool) args[0]);
        body->setProperty ("source", "center");
        job = accountClient.authorizedPost ("/optin", juce::var (body));
    }
    else
    {
        job = accountClient.authorizedGet ("/optin");
    }
    juce::Thread::launch ([job = std::move (job), complete = std::move (complete)]() mutable
    {
        auto result = job();
        if (! result.isObject())
        {
            auto* none = new juce::DynamicObject();
            none->setProperty ("ok", false);
            result = juce::var (none);
        }
        complete (result);
    });
}

// ============================================================================
// Settings > DELETE OLD VERSIONS - see OldVersionCleaner.h for what counts.
// The scan is a few directory listings (message thread); the delete may wait
// for an elevated copy of the Center, so it runs off the message thread.
// ============================================================================
void MainComponent::handleScanOldVersions (NativeArgs, NativeCompletion complete)
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("success", true);
    o->setProperty ("items", OldVersionCleaner::toVar (OldVersionCleaner::scan()));
    complete (juce::var (o));
}

void MainComponent::handleDeleteOldVersions (NativeArgs, NativeCompletion complete)
{
    juce::Thread::launch ([complete = std::move (complete)]() mutable
    {
        complete (OldVersionCleaner::cleanNow());
    });
}

// ============================================================================
// Settings > MOVE PLUGINS TO MY FOLDER (macOS) - see UserFolderMove.h. After
// it, every update installs with no password. The scan is a few directory
// listings; the move waits for the password, so it runs off the message thread.
// ============================================================================
void MainComponent::handleScanUserFolderMove (NativeArgs, NativeCompletion complete)
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("success", true);
    o->setProperty ("supported", UserFolderMove::isSupported());
    o->setProperty ("items", UserFolderMove::toVar (UserFolderMove::scan()));
    complete (juce::var (o));
}

void MainComponent::handleMoveToUserFolders (NativeArgs, NativeCompletion complete)
{
    juce::Thread::launch ([safe = juce::Component::SafePointer<MainComponent> (this), complete = std::move (complete)]() mutable
    {
        const auto result = UserFolderMove::moveNow();
        juce::MessageManager::callAsync ([safe, result, complete = std::move (complete)]() mutable
        {
            if (safe != nullptr)
                safe->forgetVersionsOfRemovedCopies (result);
            complete (result);
        });
    });
}

// Audit R196 (a): when a plugin was in both places, the move kept the copy in the
// user's folders and removed the system one - which may have been the NEWER one,
// since every update went to /Library before. The version the Center recorded is
// then the removed copy's. Forgotten here, it is read again from the bundle that
// stayed (its Info.plist), and an older copy is offered its update. The worst case
// is one update offer too many.
void MainComponent::forgetVersionsOfRemovedCopies (const juce::var& moveResult)
{
    auto* items = moveResult.getProperty ("items", {}).getArray();
    if (items == nullptr)
        return;

    bool changed = false;
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& it : *items)
        {
            if (it.getProperty ("status", "").toString() != "removed")
                continue;
            const auto name = it.getProperty ("name", "").toString();
            for (auto& p : pluginData)
            {
                const auto app = p.standaloneExe.isNotEmpty() ? p.standaloneExe.replace (".exe", "") + ".app" : juce::String();
                if (name.isNotEmpty() && (name == p.vst3Bundle || name == p.auBundle || name == app) && ! isBusyStatus (p.status))
                {
                    VersionChecker::clearInstalledVersion (p.registryKey);
                    VersionChecker::refreshInstallState (p);
                    changed = true;
                }
            }
        }
    }
    if (changed)
        emitPluginsUpdated();
}

// ============================================================================
// UNINSTALL in a card's menu (PluginUninstaller.h). Answers once it is done;
// the card shows "uninstalling" meanwhile. REINSTALL needs nothing of its own:
// installPlugin already takes a plugin that is up to date.
// ============================================================================
void MainComponent::handleUninstallPlugin (NativeArgs args, NativeCompletion complete)
{
    auto answer = [] (bool ok, const juce::String& error)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("ok", ok);
        o->setProperty ("error", error);
        return juce::JSON::toString (juce::var (o));
    };

    if (args.isEmpty())
    {
        complete (answer (false, "Missing plugin ID"));
        return;
    }

    const auto pluginId = args[0].toString();
    PluginInfo target;
    bool found = false, busy = false;
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
        {
            if (p.id == pluginId)
            {
                found = true;
                busy = isBusyStatus (p.status);
                if (! busy)
                {
                    target = p;
                    p.status = PluginStatus::Uninstalling;
                }
                break;
            }
        }
    }

    if (! found || busy)
    {
        complete (answer (false, found ? "It is busy - try again when it has finished" : "Unknown plugin"));
        return;
    }
    emitPluginsUpdated();

    juce::Thread::launch ([safe = juce::Component::SafePointer<MainComponent> (this), target, complete = std::move (complete)]() mutable
    {
        const auto r = PluginUninstaller::uninstall (target);

        juce::MessageManager::callAsync ([safe, target, r, complete = std::move (complete)]() mutable
        {
            if (safe == nullptr)
                return;
            {
                juce::ScopedLock sl (safe->pluginDataLock);
                for (auto& p : safe->pluginData)
                    if (p.id == target.id)
                    {
                        VersionChecker::refreshInstallState (p);
                        p.waitingFor = {};
                        break;
                    }
            }
            safe->prefetched.erase (target.id);
            safe->emitPluginsUpdated();

            // Audit R171: a removal that failed for a reason that is not the user's
            // choice or an open DAW is a bug report too (the toast alone reached nobody).
            if (! r.ok && ! r.declined && r.hosts.isEmpty() && r.error.isNotEmpty())
                RoneCrashReporter::reportError ("RONE Plugins Center", JUCE_APPLICATION_VERSION_STRING, "Center", "CENTER_ERROR",
                                                "Uninstall " + target.id + ": " + withoutHome (r.error));

            auto* o = new juce::DynamicObject();
            o->setProperty ("ok", r.ok);
            o->setProperty ("error", r.error);
            o->setProperty ("declined", r.declined);
            o->setProperty ("hosts", r.hosts.joinIntoString (", "));
            juce::Array<juce::var> left;
            for (const auto& l : r.leftovers)
                left.add (l);
            o->setProperty ("leftovers", left);
            complete (juce::JSON::toString (juce::var (o)));
        });
    });
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
    // Same read as the plugins: the bundle installer's Uninstall entry is the
    // truth, the HKCU stamp the fallback (and it is healed to match).
    return VersionChecker::getInstalledVersion ("__center__");
#elif JUCE_MAC
    // Audit R122: the build an update that landed stamped (reportCenterUpdateOutcome),
    // so a rebuild of the same version (2.1.4.270 -> .271) reaches Macs too. Only
    // while it belongs to the running app: after any other install it is ignored,
    // exactly as before the stamp existed.
    const auto stamped = VersionChecker::getInstalledVersion ("__center__");
    const juce::String base (JUCE_APPLICATION_VERSION_STRING);
    return (stamped.startsWith (base + ".") || stamped == base) ? stamped : juce::String();
#else
    return {};
#endif
}


// Empty string = up to date (or unknowable); otherwise the catalog version.
// Only ever a NEWER version: the old rule ("anything but the catalog's") would
// have offered a downgrade the day a manifest went backwards.
static juce::String pendingCenterUpdateVersion (const NetworkManager::CenterInstallerInfo& info)
{
    if (! info.isValid())
        return {};

    const auto installed = MainComponent::readInstalledCenterVersion();

    if (installed.isNotEmpty())
        return VersionChecker::isNewerVersion (installed, info.version) ? info.version : juce::String();

    // Installs older than this feature never stamped their version; only the
    // CMake base is known. A base change (which any release carrying this
    // feature makes) is detectable - same-base rebuilds are not.
    const juce::String base (JUCE_APPLICATION_VERSION_STRING);
    const bool sameBase = info.version == base || info.version.startsWith (base + ".");
    return (sameBase || ! VersionChecker::isNewerVersion (base, info.version)) ? juce::String() : info.version;
}

void MainComponent::checkForCenterUpdate()
{
    const auto version = pendingCenterUpdateVersion (networkManager.getCenterInstallerInfo());
    if (version.isEmpty())
        return;

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("version", version);
    emitToPage ("centerUpdateAvailable", juce::var (obj));
}

static juce::File centerUpdateMarker()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("RonePluginsCenter").getChildFile ("center-update.json");
}

static juce::File centerUpdateLog()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("RonePluginsCenter").getChildFile ("center-update.log");
}

// 2.1.26: how far the self-update got, kept in the marker itself ("step",
// "stepAt", "stepDetail"). Thirteen "did not finish" reports (issue #4, 12
// machines, mostly Mac) said nothing else, because the marker held only the
// target version and the Mac path wrote no log at all. Not in center-update.log:
// on Windows Inno overwrites that file when setup starts and holds it open.
// A marker already settled (deleted by `failed`, manual() or the read-back) is
// never brought back - that would report a finished attempt as unfinished.
static void centerUpdateStep (const juce::String& step, const juce::String& detail = {})
{
    const auto marker = centerUpdateMarker();
    if (! marker.existsAsFile())
        return;
    auto m = juce::JSON::parse (marker.loadFileAsString());
    if (auto* o = m.getDynamicObject())
    {
        o->setProperty ("step", step);
        o->setProperty ("stepAt", juce::Time::getCurrentTime().toISO8601 (true));
        if (detail.isNotEmpty())
            o->setProperty ("stepDetail", detail);
        marker.replaceWithText (juce::JSON::toString (m, true));
    }
}

// The marker's step line for a report ("step: setup_started at ..."), empty when there is none.
static juce::String centerUpdateStepLine()
{
    const auto m = juce::JSON::parse (centerUpdateMarker().loadFileAsString());
    const auto step = m.getProperty ("step", "").toString();
    if (step.isEmpty())
        return {};
    auto line = "step: " + step + " at " + m.getProperty ("stepAt", "").toString();
    const auto detail = m.getProperty ("stepDetail", "").toString();
    if (detail.isNotEmpty())
        line << " (" << detail << ")";
    return line;
}

// The last `lines` lines of Inno's log of the self-update (Windows), or why there
// are none. Inno creates the log before InitializeSetup, so a missing log means
// setup never started logging at all - not that it stopped on the way.
static juce::String centerUpdateLogTail (int lines)
{
    const auto log = centerUpdateLog();
    if (! log.existsAsFile())
        return "inno log: missing - setup never started (quit during the permission prompt?)";
    const auto all = juce::StringArray::fromLines (log.loadFileAsString());
    juce::String tail ("inno log, last lines:");
    for (int i = juce::jmax (0, all.size() - lines); i < all.size(); ++i)
        if (all[i].trim().isNotEmpty())
            tail << "\n" << all[i].trim();
    return tail;
}

void MainComponent::handleApplyCenterUpdate (NativeArgs, NativeCompletion complete)
{
    const auto info = networkManager.getCenterInstallerInfo();

    if (! info.isValid())
    {
        complete ("{\"started\":false,\"error\":\"No update information yet - refresh first\"}");
        return;
    }

    // Never in the middle of an install: the Center has to quit to be replaced,
    // and quitting abandoned whatever was downloading or waiting for a DAW.
    if (isWorking())
    {
        centerUpdateWanted = true;
        complete ("{\"started\":false,\"waiting\":true}");
        auto* params = new juce::DynamicObject();
        params->setProperty ("version", info.version);
        emitStatusMessage ("The Center updates itself as soon as the installs finish.", "info",
                           "center_update_waiting", juce::var (params));
        return;
    }

    centerUpdateWanted = false;
    auto* params = new juce::DynamicObject();
    params->setProperty ("version", info.version);
    emitStatusMessage ("Downloading Center update v" + info.version + "...", "info",
                       "center_update_downloading", juce::var (params));
   #if JUCE_MAC
    centerUpdateExpectedSha = info.sha256Mac;
    networkManager.downloadInstaller ("__center__", info.urlMac, info.sha256Mac, info.version);
   #else
    centerUpdateExpectedSha = info.sha256;
    networkManager.downloadInstaller ("__center__", info.url, info.sha256, info.version);
   #endif
    complete ("{\"started\":true}");
}

void MainComponent::applyCenterUpdateWhenIdle()
{
    if (! centerUpdateWanted || isWorking())
        return;

    handleApplyCenterUpdate ({}, [] (juce::var) {});
}

#if JUCE_WINDOWS
static bool isReparsePointPath (const juce::File& f)
{
    const auto attrs = GetFileAttributesW (f.getFullPathName().toWideCharPointer());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

// The Center's own installer (installer/RONE_Plugins.iss), started with
// /RELAUNCH=1, creates this mutex once it runs elevated - then waits for the
// Center to exit before it replaces the exe, and starts the new one as the user.
static bool centerSetupIsReady()
{
    if (auto h = OpenMutexW (SYNCHRONIZE, FALSE, L"RoneCenterUpdateGo"))
    {
        CloseHandle (h);
        return true;
    }
    // Made by an elevated process: we may not open it, but it is there.
    return GetLastError() == ERROR_ACCESS_DENIED;
}
#endif

void MainComponent::applyCenterUpdate (const juce::File& installerFile)
{
    const auto info = networkManager.getCenterInstallerInfo();

    // Read back after the restart (reportCenterUpdateOutcome): did it land? The
    // marker also records each step reached (centerUpdateStep), so a Center quit
    // mid-update says at the next start where it stopped.
    {
        auto* m = new juce::DynamicObject();
        m->setProperty ("version", info.version);
        m->setProperty ("from", juce::String (JUCE_APPLICATION_VERSION_STRING));
        m->setProperty ("at", (double) juce::Time::currentTimeMillis());
        m->setProperty ("step", "requested");
        m->setProperty ("stepAt", juce::Time::getCurrentTime().toISO8601 (true));
        centerUpdateMarker().getParentDirectory().createDirectory();
        centerUpdateMarker().replaceWithText (juce::JSON::toString (juce::var (m), true));
    }

    // The attempt is over, here: say so, and file it (unless the code is one of the
    // user's own situations - emitStatusMessage decides by the text). `details`
    // (the installer's log tail, the exit code) goes into the report only, with the
    // marker's last step; the marker itself is settled and removed.
    auto failed = [safe = juce::Component::SafePointer<MainComponent> (this)] (const juce::String& text, const juce::String& code,
                                                                                const juce::String& details = {},
                                                                                const juce::var& params = {})
    {
        juce::MessageManager::callAsync ([safe, text, code, details, params]
        {
            if (safe == nullptr) return;
            const auto step = centerUpdateStepLine();
            centerUpdateMarker().deleteFile();
            safe->emitStatusMessage (text, code == "center_update_declined" ? "info" : "error", code, params,
                                     (step.isNotEmpty() ? step + "\n" : juce::String()) + details);
        });
    };

#if JUCE_WINDOWS
    // No PowerShell any more (a hidden "-ExecutionPolicy Bypass" script started by
    // an unsigned exe is exactly what antivirus flags). The installer is opened
    // the normal way: it asks Windows for permission itself, and the elevated
    // setup tells us (centerSetupIsReady) when to step aside.
    const auto log = centerUpdateLog();
    log.deleteFile();
    const auto params = juce::String ("/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP- /RELAUNCH=1 /LOG=\"")
                      + log.getFullPathName() + "\"";
    const auto file = installerFile.getFullPathName();

    // Audit R025, stage 1 (observe only - the self-update is the one way every
    // Center in the field gets its fixes, so nothing here may stop it): the file
    // that is about to run is hashed again against the hash it was downloaded for
    // - it sat in the user's %TEMP% since - and a mismatch or a reparse point is
    // reported. It still runs in this release. Stage 2 holds a deny-write handle
    // on it until setup is elevated, and refuses a mismatch (see the audit notes).
    {
        juce::String problem;
        if (isReparsePointPath (installerFile.getParentDirectory()) || isReparsePointPath (installerFile))
            problem = "the download folder or the Center installer is a reparse point";
        else if (centerUpdateExpectedSha.length() == 64)
        {
            juce::FileInputStream in (installerFile);
            if (! in.openedOk())
                problem = "the Center installer could not be read back before it ran";
            else if (! juce::SHA256 (in).toHexString().equalsIgnoreCase (centerUpdateExpectedSha))
                problem = "center installer changed after download";
        }
        if (problem.isNotEmpty())
        {
            RoneCrashReporter::reportError ("RONE Plugins Center", JUCE_APPLICATION_VERSION_STRING, "Center",
                                            "CENTER_INTEGRITY", problem + " (self-update, logged only)");
            CrashReportUploader::uploadPendingAsync();
        }
        centerUpdateStep ("downloaded", problem.isNotEmpty() ? problem : "hash ok");
    }

    SHELLEXECUTEINFOW sei {};
    sei.cbSize = sizeof (sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    sei.lpVerb = L"open";
    sei.lpFile = file.toWideCharPointer();
    sei.lpParameters = params.toWideCharPointer();
    sei.nShow = SW_HIDE;

    if (! ShellExecuteExW (&sei) || sei.hProcess == nullptr)
    {
        // The stub itself could not start (antivirus, SmartScreen, the file gone).
        // Not a refused prompt: the installer is asInvoker and asks for permission
        // only once it runs, so a refusal comes back as its exit code below.
        failed ("Could not start the Center update.", "center_update_failed",
                "ShellExecuteEx error " + juce::String ((int) GetLastError()));
        return;
    }

    centerUpdateStep ("setup_started");
    emitStatusMessage ("Restarting to finish the Center update...", "info", "center_update_restarting");

    const auto stub = sei.hProcess;
    juce::Thread::launch ([stub, failed]
    {
        // The stub's exit is the end of the attempt, so poll as long as it lives: a
        // permission prompt answered late still works (before 2.1.26 a ten-minute
        // cap then said "did not start" while setup could still run - and when it
        // did, nobody quit for it). Windows itself dismisses an unanswered prompt
        // after about two minutes; the cap below is for a stub that hangs, so the
        // page is never stuck on "Restarting..." for good.
        constexpr int capMs = 30 * 60 * 1000;
        for (int waited = 0; waited < capMs; waited += 200)
        {
            if (centerSetupIsReady())
            {
                CloseHandle (stub);
                centerUpdateStep ("go_seen");
                juce::MessageManager::callAsync ([] { juce::JUCEApplication::getInstance()->systemRequestedQuit(); });
                return;
            }

            if (WaitForSingleObject (stub, 200) == WAIT_OBJECT_0)
            {
                DWORD code = 0;
                GetExitCodeProcess (stub, &code);
                CloseHandle (stub);
                centerUpdateStep ("setup_exited", "code " + juce::String ((int) code));

                // Setup ended while this Center is still here. Its code, mapped the way
                // InstallBatcher::describe maps a plugin install - with the phrases
                // isEnvironmentalStatus knows, so a file in use or a restart is nobody's
                // bug - instead of "any non-zero = declined" (2.1.26, issue #6):
                //   0     it says it finished, yet never created RoneCenterUpdateGo: it ran
                //         against another folder or inside an antivirus sandbox (reported);
                //   1, 2  it never reached its install phase: a refused permission prompt
                //         (the stub is asInvoker and respawns elevated; the refusal comes
                //         back as 1 "failed to initialize" or 2 "cancelled"). NOT measured
                //         yet - a real init failure hides here until someone runs the
                //         installer with /RELAUNCH=1, declines, and reads the code;
                //   5     a file could not be replaced: this exe still running when the
                //         installer's wait ran out, or held by something else;
                //   8     Windows needs a restart first;
                //   other failed (reported, with the log's last lines).
                const auto tail = centerUpdateLogTail (6);
                if (code == 0)
                    failed ("The Center installer finished but never asked the Center to quit - it probably ran in an"
                            " antivirus sandbox or against another install folder. The Center was not updated.",
                            "center_update_not_asked", tail);
                else if (code == 1 || code == 2)
                    failed ("Windows did not give permission - the Center was not updated.", "center_update_declined");
                else if (code == 5)
                    failed ("Center update cancelled - a file was in use (installer code 5): the Center did not close"
                            " in time. Close every RONE window, then update again.",
                            "center_update_file_in_use", tail);
                else if (code == 8)
                    failed ("Windows needs a restart before the Center can be updated. Restart, then update again.",
                            "center_update_restart_needed", tail);
                else
                {
                    auto* params = new juce::DynamicObject();
                    params->setProperty ("code", (int) code);
                    failed ("The Center update failed (installer code " + juce::String ((int) code) + ").",
                            "center_update_error", tail, juce::var (params));
                }
                return;
            }
        }
        CloseHandle (stub);
        failed ("The Center update is still waiting for its installer after 30 minutes - the Center was not updated."
                " Close the Center and open it again, then update.", "center_update_timeout", "stub alive at the cap");
    });

#elif JUCE_MAC
    // macOS: the .dmg holds the app. Mount it, put the new app where this one
    // is (asking for the administrator password only when that folder needs
    // it), unmount, start the new app, quit.
    //
    // Since macOS 13, App Management can refuse an unsigned app the right to
    // touch an app in /Applications - root included ("Operation not permitted").
    // Marvin, macOS 26, 2026-10-06: the password, then "not installed"; the retry
    // then failed to attach the image a first try had left mounted. 2.0.0 also
    // deleted the running app BEFORE copying, so a refusal could leave it broken.
    // Now: copy beside it and swap by renaming (the old app comes back on any
    // failure), ask for the password only when plain permissions are the reason,
    // and when macOS says no, open the .dmg in Finder for one drag onto
    // Applications - Finder is always allowed.
    juce::ignoreUnused (failed);
    const auto target = juce::File::getSpecialLocation (juce::File::currentApplicationFile);
    const auto dmg = installerFile;

    // Audit R199: a Center started from its mounted disk image (or from the
    // quarantine copy macOS runs it from) can never replace itself - it asked for
    // a password for nothing, failed and filed an error. Say how instead, and open
    // the new image for the one drag onto Applications.
    // The test is a READ-ONLY volume, not the place: a Center kept on a writable
    // drive under /Volumes/ updates itself as before (a mounted .dmg and the
    // AppTranslocation mount are both read-only). Only under /Volumes/, where a
    // .dmg mounts: the boot disk's system part is read-only too, and a Center in
    // /Applications must never be turned away here.
    {
        const auto path = target.getFullPathName();
        struct statfs fs {};
        const bool readOnlyVolume = path.startsWith ("/Volumes/")
                                    && ::statfs (target.getParentDirectory().getFullPathName().toRawUTF8(), &fs) == 0
                                    && (fs.f_flags & MNT_RDONLY) != 0;
        if (readOnlyVolume || path.contains ("/AppTranslocation/"))
        {
            centerUpdateMarker().deleteFile();
            juce::ChildProcess open;
            open.start (juce::StringArray { "/usr/bin/open", dmg.getFullPathName() });
            emitStatusMessage ("The Center is running from its disk image. Drag RONE Plugins Center onto Applications, "
                               "open it from there, and it updates itself.", "info", "center_update_from_dmg");
            return;
        }
    }

    emitStatusMessage ("Restarting to finish the Center update...", "info", "center_update_restarting");

    juce::Thread::launch ([target, dmg, safe = juce::Component::SafePointer<MainComponent> (this)]
    {
        auto run = [] (const juce::StringArray& args, int timeoutMs, juce::String* output = nullptr)
        {
            juce::ChildProcess p;
            if (! p.start (args, juce::ChildProcess::wantStdOut | juce::ChildProcess::wantStdErr))
                return false;
            // Wait first (with its limit), then read: these tools say a line or two.
            const bool finished = p.waitForProcessToFinish (timeoutMs);
            if (! finished)
                p.kill();
            else if (output != nullptr)
                *output = p.readAllProcessOutput();
            return finished && p.getExitCode() == 0;
        };

        // The finish line when the Center may not do it itself.
        auto manual = [dmg, safe]
        {
            juce::MessageManager::callAsync ([dmg, safe]
            {
                if (safe == nullptr)
                    return;
                const auto step = centerUpdateStepLine();
                centerUpdateMarker().deleteFile();
                juce::ChildProcess open;
                open.start (juce::StringArray { "/usr/bin/open", dmg.getFullPathName() });
                // Audit R201: the running Center has to be quit for Finder's Replace to
                // work (the X only hides it). The designed fallback is not a bug report.
                safe->emitStatusMessage ("macOS did not let the Center replace itself. Quit the Center from its menu-bar icon, "
                                         "then in the Finder window that opened drag RONE Plugins Center onto Applications, "
                                         "choose Replace, and open it again.",
                                         "error", "center_update_manual", {}, step);
            });
        };

        // A mount left by an earlier try keeps the image busy: hdiutil then
        // refuses to attach it again.
        const auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory);
        for (const auto& old : tmp.findChildFiles (juce::File::findDirectories, false, "RONE_center_update_*"))
        {
            run (juce::StringArray { "/usr/bin/hdiutil", "detach", old.getFullPathName(), "-force", "-quiet" }, 60000);
            old.deleteRecursively();
        }

        const auto mount = tmp.getChildFile ("RONE_center_update_" + juce::Uuid().toString().substring (0, 8));
        mount.createDirectory();

        // Each stage into the marker (2.1.26): the Mac path writes no log, and nine
        // of the thirteen "did not finish" reports of issue #4 came from here.
        centerUpdateStep ("attach");
        juce::String attachOut;
        if (! run (juce::StringArray { "/usr/bin/hdiutil", "attach", "-nobrowse", "-noautoopen", "-quiet",
                                       "-mountpoint", mount.getFullPathName(), dmg.getFullPathName() }, 120000, &attachOut))
        {
            centerUpdateStep ("attach_failed", attachOut.trim());
            mount.deleteRecursively();
            manual();
            return;
        }

        juce::File app;
        for (const auto& f : mount.findChildFiles (juce::File::findDirectories, false, "*.app"))
            app = f;

        // Fresh names every time: a leftover directory under a fixed name would
        // turn "mv old new" into "mv old INTO new".
        const auto tag    = juce::Uuid().toString().substring (0, 8);
        const auto name   = target.getFileNameWithoutExtension();
        const auto staged = target.getSiblingFile (name + ".updating-" + tag + ".app");
        const auto backup = target.getSiblingFile (name + ".previous-" + tag + ".app");
        bool ok = false;

        if (app.isDirectory() && target.getFileName().endsWith (".app")
            && ! (target.getFullPathName() + app.getFullPathName()).containsAnyOf ("'\"\\$`"))
        {
            // As this user: copy beside, then two renames.
            centerUpdateStep ("swap");
            juce::String why;
            if (run (juce::StringArray { "/usr/bin/ditto", app.getFullPathName(), staged.getFullPathName() }, 300000, &why)
                && run (juce::StringArray { "/bin/mv", target.getFullPathName(), backup.getFullPathName() }, 30000, &why))
            {
                ok = run (juce::StringArray { "/bin/mv", staged.getFullPathName(), target.getFullPathName() }, 30000, &why);
                if (! ok)
                    run (juce::StringArray { "/bin/mv", backup.getFullPathName(), target.getFullPathName() }, 30000);
            }
            staged.deleteRecursively();
            centerUpdateStep (ok ? "swap_ok" : "swap_failed", why.trim());

            // "Permission denied" = the folder's permissions: the administrator can.
            // "Operation not permitted" = macOS protecting the app: nobody can, so no
            // password is asked for nothing.
            // "Read-only file system" (a disk image) - nobody can either (R199).
            if (! ok && ! why.containsIgnoreCase ("not permitted") && ! why.containsIgnoreCase ("read-only file system"))
            {
                auto q = [] (const juce::File& f) { return "\\\"" + f.getFullPathName() + "\\\""; };
                const juce::String sh = "/usr/bin/ditto " + q (app) + " " + q (staged)
                                      + " && /bin/mv " + q (target) + " " + q (backup)
                                      + " && { /bin/mv " + q (staged) + " " + q (target)
                                      + " || { /bin/mv " + q (backup) + " " + q (target) + "; false; }; }"
                                      + " && /bin/rm -rf " + q (backup) + "; R=$?; /bin/rm -rf " + q (staged) + "; exit $R";
                centerUpdateStep ("swap_admin");
                const auto cmd = "osascript -e 'do shell script \"" + sh + "\" with administrator privileges'";
                const int status = std::system (cmd.toRawUTF8());
                ok = WIFEXITED (status) && WEXITSTATUS (status) == 0 && target.isDirectory();
                centerUpdateStep (ok ? "swap_admin_ok" : "swap_admin_failed", "status " + juce::String (status));
            }

            if (ok)
                backup.deleteRecursively();
        }

        run (juce::StringArray { "/usr/bin/hdiutil", "detach", mount.getFullPathName(), "-quiet" }, 60000);
        mount.deleteRecursively();

        if (! ok)
        {
            manual();
            return;
        }

        centerUpdateStep ("relaunch");
        juce::MessageManager::callAsync ([target]
        {
            // Audit R198: "open -n" 400 ms before quitting raced the single-instance
            // lock - a new Center that starts while this one still holds it quits at
            // once, and the Center was simply gone after its update. A small waiter
            // starts the new one only once this process has exited (20 s at most).
            // The path travels as $0, so it needs no quoting.
            const juce::String sh = "i=0; while /bin/kill -0 " + juce::String ((int) getpid())
                                  + " 2>/dev/null && [ $i -lt 100 ]; do /bin/sleep 0.2; i=$((i+1)); done; "
                                    "exec /usr/bin/open -n \"$0\" --args --updated";
            juce::ChildProcess relaunch;
            relaunch.start (juce::StringArray { "/bin/sh", "-c", sh, target.getFullPathName() }, 0);
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        });
    });
#else
    juce::ignoreUnused (installerFile);
    failed ("Center self-update is not supported on this system.", "center_update_failed");
#endif
}

// After a restart: the marker names the version the update was for. Landed =
// say so; not landed = say that, with what the installer's log says, and file it.
void MainComponent::reportCenterUpdateOutcome()
{
    const auto marker = centerUpdateMarker();
    if (! marker.existsAsFile())
        return;

    const auto m = juce::JSON::parse (marker.loadFileAsString());
    marker.deleteFile();

    const auto target = m.getProperty ("version", "").toString();
    if (target.isEmpty())
        return;

    const juce::String running (JUCE_APPLICATION_VERSION_STRING);
    auto installed = readInstalledCenterVersion();
    if (installed.isEmpty())
        installed = running;

    const auto step = m.getProperty ("step", "").toString();
    auto* params = new juce::DynamicObject();
    params->setProperty ("version", target);
    if (step.isNotEmpty())
        params->setProperty ("step", step);

    // The bundle carries the base (1.6.3), the catalog the build (1.6.3.250).
    const bool landed = ! VersionChecker::isNewerVersion (installed, target)
                     || target.startsWith (running + ".") || target == running;

    if (landed)
    {
       #if JUCE_MAC
        VersionChecker::setInstalledVersion ("__center__", target);   // R122: see readInstalledCenterVersion
       #endif
        emitStatusMessage ("RONE Plugins Center updated to v" + target + ".", "success", "center_updated", juce::var (params));
        return;
    }

    // Not landed. The page gets the short sentence (plus the step, so the next
    // report of issue #4 names its branch); the report's details get everything
    // there is: the marker (version, from, when, last step and its detail), what is
    // installed and running, and Inno's log - or that there is none (2.1.26). A
    // marker older than a week is an attempt long past: still an error for the
    // user (the Center IS old), said as such in the details.
    const auto at = (juce::int64) (double) m.getProperty ("at", 0.0);
    const bool stale = at > 0 && juce::Time::currentTimeMillis() - at > 7LL * 24 * 3600 * 1000;

    juce::String details;
    if (stale)
        details << "stale attempt from " << juce::Time (at).toISO8601 (true) << "\n";
    details << "marker: " << juce::JSON::toString (m, true) << "\n"
            << "installed: " << installed << "\nrunning: " << running << "\n";
   #if JUCE_WINDOWS
    details << centerUpdateLogTail (20);
   #else
    details << "no installer log on macOS - see the marker's step";
   #endif

    emitStatusMessage ("The Center update to v" + target + " did not finish."
                           + (step.isNotEmpty() ? " Stopped after: " + step + "." : juce::String()),
                       "error", "center_update_unfinished", juce::var (params), details);
}

// ============================================================================
// NetworkManager callbacks → push to JS
// ============================================================================

void MainComponent::onManifestReady (const juce::Array<PluginInfo>& plugins, bool fromCache)
{
    lastPublicPlugins = plugins;
    lastPublicFromCache = fromCache;
    applyCatalog (withDevChannel (plugins), fromCache);
    refreshDevChannel();
}

juce::Array<PluginInfo> MainComponent::withDevChannel (const juce::Array<PluginInfo>& publicPlugins) const
{
    auto all = publicPlugins;
    for (const auto& d : devPlugins)
    {
        int released = -1;
        for (int i = 0; i < all.size(); ++i)
            if (all[i].id == d.id) { released = i; break; }

        if (released < 0)
            all.add (d);                                   // a plug-in before its release: its own card
        else if (all[released].app)
        {
            // A dev build of a RONE app (RONE VST MANAGER for Mac, 30.10.2026) keeps the gift card. The dev
            // channel answers in "plugins"; the public entry knows it is an app. Taken whenever this platform
            // has no public installer (the Mac app before its release), otherwise only when it is newer.
           #if JUCE_MAC
            const bool publicHere = all[released].downloadUrlMac.isNotEmpty();
           #else
            const bool publicHere = all[released].downloadUrl.isNotEmpty();
           #endif
            if (! publicHere || VersionChecker::isNewerVersion (all[released].remoteVersion, d.remoteVersion))
            {
                auto appBuild = d;
                appBuild.app = true;
                all.set (released, appBuild);
            }
        }
        // A released plug-in whose NEXT version is in development (rack-first, 26.10.2026: Throw 1.9.0 while 1.8.6 is
        // public): the newer dev build takes the card, so this Center offers it as the update - and never offers the
        // public version back over it (the status rule is "anything but the published version gets the published
        // version", which turned a locally installed 1.9.0 into an endless UPDATE to 1.8.6). The public entry wins
        // again once it catches up. Only Liran's accounts ever receive dev entries.
        else if (VersionChecker::isNewerVersion (all[released].remoteVersion, d.remoteVersion))
            all.set (released, d);
    }
    return all;
}

void MainComponent::refreshDevChannel()
{
    if (devFetchInFlight) return;
    devFetchInFlight = true;
    accountClient.fetchDevChannelAsync ([safe = juce::Component::SafePointer<MainComponent> (this)] (juce::String body)
    {
        if (safe == nullptr) return;
        safe->devFetchInFlight = false;
        auto fresh = safe->networkManager.parseDevChannel (body);
        // nothing changed (the usual answer, and every customer's): no second pass over the catalog
        bool same = fresh.size() == safe->devPlugins.size();
        for (int i = 0; same && i < fresh.size(); ++i)
            same = fresh[i].id == safe->devPlugins[i].id && fresh[i].remoteVersion == safe->devPlugins[i].remoteVersion
                && fresh[i].installedVersion == safe->devPlugins[i].installedVersion;
        if (same) return;
        safe->devPlugins = fresh;
        if (safe->manifestEverLoaded)
            safe->applyCatalog (safe->withDevChannel (safe->lastPublicPlugins), safe->lastPublicFromCache);
    });
}

void MainComponent::applyCatalog (const juce::Array<PluginInfo>& plugins, bool fromCache)
{
    // A refresh must not reset a card that is downloading, installing, waiting
    // for its DAW or uninstalling: before 2.0 it put them all back to "Update",
    // and a second click started a second elevated installer next to the first.
    {
        juce::ScopedLock sl (pluginDataLock);
        juce::Array<PluginInfo> merged = plugins;
        for (auto& fresh : merged)
            for (auto& old : pluginData)
                if (old.id == fresh.id && isBusyStatus (old.status))
                {
                    fresh.status           = old.status;
                    fresh.downloadProgress = old.downloadProgress;
                    fresh.waitingFor       = old.waitingFor;
                    break;
                }
        pluginData = merged;
    }

    const bool wasCached = manifestFromCache;
    manifestFromCache = fromCache;
    manifestEverLoaded = true;
    if (! fromCache)
        offlineRetries = 0;

    // Pre-downloads of a file the catalog no longer names (by hash, 2.1.26) are worthless.
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto it = prefetched.begin(); it != prefetched.end();)
        {
            bool current = false;
            for (auto& p : pluginData)
                if (p.id == it->first && platformSha (p).equalsIgnoreCase (it->second.sha256) && it->second.sha256.isNotEmpty())
                    current = true;
            it = current ? std::next (it) : prefetched.erase (it);
        }
    }

    emitPluginsUpdated();
    emitToPage ("manifestState", manifestStateVar());

    if (fromCache && ! wasCached)
    {
        offlineSeen = true;
        emitStatusMessage ("Offline - showing the plugins as they were at the last check.", "info", "offline_cached");
    }
    else if (! fromCache && offlineSeen)
    {
        offlineSeen = false;
        emitStatusMessage ("Back online.", "success", "back_online");
    }

    if (fromCache)
        return;   // nothing below may act on an old catalog

    // A plugin's UPDATE button or a ronecenter:// link asked for this manifest.
    servePendingUpdate();

    // A download that failed its hash check asked for this manifest. Now that
    // the hash is current, try that one plugin again - see onDownloadComplete.
    if (staleHashRetryId.isNotEmpty())
    {
        const auto retryId = staleHashRetryId;
        staleHashRetryId.clear();

        juce::String url, sha, version;
        bool isDev = false;

        if (retryId == "__center__")
        {
            const auto info = networkManager.getCenterInstallerInfo();
           #if JUCE_MAC
            url = info.urlMac; sha = info.sha256Mac;
           #else
            url = info.url; sha = info.sha256;
           #endif
            version = info.version;
            centerUpdateExpectedSha = sha;
        }
        else
        {
            juce::ScopedLock sl (pluginDataLock);
            for (auto& p : pluginData)
                if (p.id == retryId)
                {
                   #if JUCE_MAC
                    url = p.downloadUrlMac; sha = p.sha256Mac;
                   #else
                    url = p.downloadUrl;    sha = p.sha256;
                   #endif
                    version = p.remoteVersion;
                    p.status = PluginStatus::Queued;
                    p.downloadProgress = 0.0;
                    isDev = p.dev;
                    break;
                }
        }

        if (url.isNotEmpty())
        {
            // From here the id means "this download IS the retry": if it fails
            // the hash again, that is the answer, not another round.
            staleHashRetryInFlight = retryId;
            emitPluginsUpdated();
            networkManager.downloadInstaller (retryId, url, sha, version, false, isDev);
        }
    }

    checkForCenterUpdate();
    startBackgroundDownloads();
}

void MainComponent::onManifestError (const juce::String& errorMessage)
{
    // The fresh manifest a failed download was waiting on did not arrive, so
    // that download stays failed - and visibly so, not "Downloading" forever.
    if (staleHashRetryId.isNotEmpty())
    {
        const auto id = staleHashRetryId;
        staleHashRetryId.clear();
        setStatus (id, PluginStatus::Error);
        emitStatusMessage ("Download could not be verified - please try again in a few minutes.", "error", "verify_retry_failed");
        return;
    }

    emitToPage ("manifestState", manifestStateVar());

    // Said once, not on every retry; "Offline - " keeps it out of the error reports.
    if (! offlineSeen)
    {
        offlineSeen = true;
        emitStatusMessage ("Offline - " + errorMessage, "error", "offline");
    }
}

void MainComponent::onDownloadStarted (const juce::String& pluginId)
{
    if (pluginId == "__center__" || networkManager.isPrefetch (pluginId))
        return;
    setStatus (pluginId, PluginStatus::Downloading);
}

void MainComponent::onDownloadProgress (const juce::String& pluginId, double progress)
{
    if (networkManager.isPrefetch (pluginId))
        return;   // a background download stays out of sight

    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
            if (p.id == pluginId)
            {
                p.downloadProgress = progress;
                p.status = PluginStatus::Downloading;
                break;
            }
    }

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("pluginId", pluginId);
    obj->setProperty ("progress", progress);
    emitToPage ("downloadProgress", juce::var (obj));
}

void MainComponent::onDownloadComplete (const juce::String& pluginId,
                                         const juce::File& localFile,
                                         bool success,
                                         bool cancelled,
                                         const juce::String& errorMessage,
                                         const juce::String& sha256,
                                         const juce::String& version)
{
    // What was this download for? The status says: a card that is Queued or
    // Downloading wants an install; anything else was a background pre-download.
    // sha256 / version are the JOB's - the identity the file was verified with -
    // not the card's, which a refresh may have moved on meanwhile (2.1.26).
    PluginStatus status = PluginStatus::NotInstalled;
    juce::String name = pluginId;
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
            if (p.id == pluginId) { status = p.status; name = p.name; break; }
    }
    const bool wantsInstall = status == PluginStatus::Queued || status == PluginStatus::Downloading;

    if (success)
        busyRetried.removeString (pluginId);   // the next busy server gets its own retry

    if (cancelled)
    {
        if (wantsInstall)
            restoreIdleState (pluginId);
        batcher.poke();
        applyCenterUpdateWhenIdle();
        return;
    }

    // A hash mismatch usually means our manifest is OLD, not that the file is
    // bad - see below. Decide that first, for the Center's own installer too:
    // it sits behind the same moving tag and the same cached manifest.
    const bool isRetry = (staleHashRetryInFlight == pluginId);

    if (isRetry)
        staleHashRetryInFlight.clear();

    if (! success && errorMessage.contains ("SHA256") && ! isRetry && staleHashRetryId.isEmpty()
        && (wantsInstall || pluginId == "__center__"))
    {
        staleHashRetryId = pluginId;
        emitStatusMessage ("Checking for a newer version...", "info", "checking_newer");
        lastManifestAttemptMs = juce::Time::currentTimeMillis();
        networkManager.fetchManifest (true);
        return;
    }

    if (pluginId == "__center__")
    {
        if (success)
        {
            // Audit R026: the check at request time is not enough - the download
            // takes a while, and an install the user started meanwhile was cut off
            // when the Center quit to update. anyPluginBusy, not isWorking: the
            // download that just finished may still count as running.
            if (anyPluginBusy())
            {
                centerUpdateWanted = true;
                auto* params = new juce::DynamicObject();
                params->setProperty ("version", networkManager.getCenterInstallerInfo().version);
                emitStatusMessage ("The Center updates itself as soon as the installs finish.", "info",
                                   "center_update_waiting", juce::var (params));
                return;
            }
            applyCenterUpdate (localFile);
        }
        else
            emitStatusMessage (errorMessage.isNotEmpty() ? errorMessage
                                                         : juce::String ("Center update download failed."),
                               "error");
        return;
    }

    if (! wantsInstall)
    {
        // A background pre-download: keep it under the hash and version it was
        // fetched for, quietly. (Before 2.1.26 the card's version at COMPLETION was
        // stored, so a card swapped mid-prefetch labelled the old file with the new
        // version - and it passed the version check into the batch.)
        if (success)
        {
            prefetched[pluginId] = { version, sha256, localFile };
            emitPluginsUpdated();
        }
        batcher.poke();
        return;
    }

    if (success)
    {
        handBatcher (pluginId, localFile, sha256, version);
    }
    else if (errorMessage.contains ("the download server is busy") && ! busyRetried.contains (pluginId))
    {
        // 2.1.26 (issue #82): the download server answered 5xx through all the
        // worker's attempts - a blip at GitHub's edge, not this computer and not
        // the file. Once per plugin, try again by itself two minutes later; the
        // card shows Error meanwhile (a click starts it sooner), and a second
        // failure stands as the error below.
        busyRetried.add (pluginId);
        setStatus (pluginId, PluginStatus::Error);
        auto* params = new juce::DynamicObject();
        params->setProperty ("name", name);
        emitStatusMessage ("The download server is busy. The Center tries " + name + " again in two minutes.",
                           "info", "download_retry_later", juce::var (params));
        juce::Timer::callAfterDelay (120000, [safe = juce::Component::SafePointer<MainComponent> (this), pluginId]
        {
            if (safe == nullptr) return;
            juce::String ignored;
            safe->startInstall (pluginId, ignored);   // "Already on its way" if the user clicked meanwhile
        });
    }
    else
    {
        // The installers live behind moving "-latest" release tags, so the file
        // behind a URL changes the moment CI publishes. A Center left open
        // across a release still holds the hash it read before, and every
        // retry re-downloads the NEW file and re-compares it against the OLD
        // hash - failing forever, which is exactly what a user hit on
        // 2026-09-11 with Stutter showing v1.1.3.187 after .194 had shipped.
        //
        // So the first mismatch (handled above) fetches the manifest again -
        // from origin, past the CDN's five-minute copy - and tries once. A
        // mismatch on that retry lands here: the file really is wrong, and the
        // error stands.
        setStatus (pluginId, PluginStatus::Error);

        auto* params = new juce::DynamicObject();
        params->setProperty ("name", name);
        // NetworkManager already prefixes errors with "Download failed - " where
        // appropriate; just surface whatever it sent.
        emitStatusMessage (errorMessage.isNotEmpty() ? errorMessage : juce::String ("Download failed."),
                           "error", "download_failed", juce::var (params));

        // A Center update parked behind downloads that all failed still runs (R026):
        // no batch goes idle to wake it. Its file is already downloaded and verified.
        juce::Timer::callAfterDelay (500, [safe = juce::Component::SafePointer<MainComponent> (this)]
        {
            if (safe != nullptr)
                safe->applyCenterUpdateWhenIdle();
        });
    }

    // Nothing more downloading may be the moment the batch was waiting for.
    batcher.poke();
}
