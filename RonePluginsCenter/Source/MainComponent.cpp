#include "MainComponent.h"
#include "BinaryData.h"
#include "../../Shared/RemoteLicenseGate.h"
#include "../../Shared/RoneWebCompat.h"
#include "CrashReportUploader.h"   // also brings in Shared/RoneCrashReporter.h
#include "AutoStart.h"
#include "PluginInUse.h"
#include "OldVersionCleaner.h"
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
        .withNativeFunction ("scanOldVersions", [this] (NativeArgs args, NativeCompletion complete) {
            handleScanOldVersions (args, guarded (std::move (complete)));
        })
        .withNativeFunction ("deleteOldVersions", [this] (NativeArgs args, NativeCompletion complete) {
            handleDeleteOldVersions (args, guarded (std::move (complete)));
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
    batcher.onIdle   = [this] { applyCenterUpdateWhenIdle(); };

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
    };
    accountClient.initialize();
    lastAccountCheckMs = juce::Time::currentTimeMillis();   // initialize() just asked the server

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
    if (pre != prefetched.end() && pre->second.version == info.remoteVersion && pre->second.file.existsAsFile())
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
    juce::ScopedLock sl (pluginDataLock);
    int n = 0;
    for (auto& p : pluginData)
        if (p.status == PluginStatus::UpdateAvailable)
            ++n;
    return n;
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
        || text.contains ("installer code 2)");   // Inno: cancelled by the user before installing
}

void MainComponent::emitStatusMessage (const juce::String& text, const juce::String& type,
                                       const juce::String& code, const juce::var& params)
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
        if (type != "info")
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
    if (type == "error" && ! isEnvironmentalStatus (text))
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

    bool isFree = false, known = false;
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
            if (p.id == pluginId) { isFree = p.free; known = true; break; }
    }

    // A free plugin is unlocked by any signed-in account (the server lists it in
    // `owned` too; this keeps it installable even before that answer arrives).
    if (! isEffectivelyLicensed (licenseHandler, accountClient, pluginId)
        && ! (isFree && accountClient.getState().signedIn))
    {
        error = "License required";
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

    // Already downloaded in the background for this version: install it now
    // (the batch hashes it once more before it runs).
    const auto pre = prefetched.find (pluginId);
    if (pre != prefetched.end() && pre->second.version == version && pre->second.file.existsAsFile())
    {
        handBatcher (pluginId, pre->second.file);
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
    networkManager.downloadInstaller (pluginId, url, sha);
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

void MainComponent::handBatcher (const juce::String& pluginId, const juce::File& installer)
{
    InstallBatcher::Item item;
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
            if (p.id == pluginId)
            {
                item.id            = p.id;
                item.name          = p.name;
                item.registryKey   = p.registryKey;
                item.version       = p.remoteVersion;
                item.vst3Bundle    = p.vst3Bundle;
                item.auBundle      = p.auBundle;
                item.standaloneExe = p.standaloneExe;
               #if JUCE_MAC
                item.sha256        = p.sha256Mac;
               #else
                item.sha256        = p.sha256;
               #endif
                break;
            }
    }

    if (item.id.isEmpty())
        return;

    item.installer = installer;
    prefetched.erase (pluginId);
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
        emitStatusMessage (waitingFor.isEmpty()
                               ? name + " is still open. Close it and the update installs by itself."
                               : name + " is loaded in " + waitingFor + ". Close " + waitingFor
                                      + " and the update installs by itself.",
                           "info", waitingFor.isEmpty() ? "waiting_own" : "waiting_host", juce::var (params));
    }
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

    emitStatusMessage (r.message, r.ok ? "success" : (r.declined ? "info" : "error"), r.code, juce::var (params));

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
            emitStatusMessage (name + ": " + error, "error", error == "License required" ? "license_required" : juce::String(),
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
// Plugins/Manuals); when the local copy is missing (older install, manual
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
    juce::File local = juce::File ("/Users/Shared/RONE Plugins/Manuals").getChildFile (manualPdf);
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
    if (vst3.isNotEmpty())
    {
        candidates.add (VersionChecker::getVst3InstallDir().getChildFile (vst3));
       #if JUCE_WINDOWS
        candidates.add (VersionChecker::getVst3InstallDir().getParentDirectory().getChildFile (vst3)); // top-level VST3 folder
       #endif
    }
   #if JUCE_MAC
    if (au.isNotEmpty())  candidates.add (VersionChecker::getAUInstallDir().getChildFile (au));
    if (exe.isNotEmpty())
    {
        auto appName = exe.replace (".exe", "") + ".app";
        candidates.add (juce::File ("/Applications").getChildFile (appName));
        candidates.add (juce::File ("/Applications/RONE Plugins").getChildFile (appName));
    }
   #else
    if (exe.isNotEmpty()) candidates.add (VersionChecker::getStandaloneInstallDir().getChildFile (exe));
   #endif

    for (auto& f : candidates)
    {
        if (f.exists())
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
void MainComponent::handleOpenInstallFolder (NativeArgs args, NativeCompletion complete)
{
    const auto kind = args.size() > 0 ? args[0].toString() : juce::String ("vst3");

    juce::File dir;
    if (kind == "standalone")
    {
       #if JUCE_MAC
        dir = juce::File ("/Applications/RONE Plugins");
        if (! dir.isDirectory()) dir = juce::File ("/Applications");
       #else
        dir = VersionChecker::getStandaloneInstallDir();
       #endif
    }
    else if (kind == "au")
        dir = VersionChecker::getAUInstallDir();
    else
        dir = VersionChecker::getVst3InstallDir();

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

    bool isFree = false;
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
            if (p.id == pluginId) { isFree = p.free; break; }
    }

    if (! isEffectivelyLicensed (licenseHandler, accountClient, pluginId)
        && ! (isFree && accountClient.getState().signedIn))
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

    struct Want { juce::String id, url, sha, version; };
    juce::Array<Want> wants;
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
        {
            if (p.status != PluginStatus::UpdateAvailable || p.installedVersion == "?")
                continue;
            if (! isEffectivelyLicensed (licenseHandler, accountClient, p.id) && ! (p.free && accountClient.getState().signedIn))
                continue;
           #if JUCE_MAC
            wants.add ({ p.id, p.downloadUrlMac, p.sha256Mac, p.remoteVersion });
           #else
            wants.add ({ p.id, p.downloadUrl, p.sha256, p.remoteVersion });
           #endif
        }
    }

    for (auto& w : wants)
    {
        const auto pre = prefetched.find (w.id);
        if (pre != prefetched.end() && pre->second.version == w.version && pre->second.file.existsAsFile())
            continue;
        if (w.sha.isEmpty() || networkManager.isDownloadQueued (w.id))
            continue;
        networkManager.downloadInstaller (w.id, w.url, w.sha, true);
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
    networkManager.downloadInstaller ("__center__", info.urlMac, info.sha256Mac);
   #else
    networkManager.downloadInstaller ("__center__", info.url, info.sha256);
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

    // Read back after the restart (reportCenterUpdateOutcome): did it land?
    {
        auto* m = new juce::DynamicObject();
        m->setProperty ("version", info.version);
        m->setProperty ("from", juce::String (JUCE_APPLICATION_VERSION_STRING));
        m->setProperty ("at", (double) juce::Time::currentTimeMillis());
        centerUpdateMarker().getParentDirectory().createDirectory();
        centerUpdateMarker().replaceWithText (juce::JSON::toString (juce::var (m)));
    }

    auto failed = [safe = juce::Component::SafePointer<MainComponent> (this)] (const juce::String& text, const juce::String& code)
    {
        juce::MessageManager::callAsync ([safe, text, code]
        {
            if (safe == nullptr) return;
            centerUpdateMarker().deleteFile();
            safe->emitStatusMessage (text, code == "center_update_declined" ? "info" : "error", code);
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

    SHELLEXECUTEINFOW sei {};
    sei.cbSize = sizeof (sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    sei.lpVerb = L"open";
    sei.lpFile = file.toWideCharPointer();
    sei.lpParameters = params.toWideCharPointer();
    sei.nShow = SW_HIDE;

    if (! ShellExecuteExW (&sei) || sei.hProcess == nullptr)
    {
        failed ("Could not start the Center update.", "center_update_failed");
        return;
    }

    emitStatusMessage ("Restarting to finish the Center update...", "info", "center_update_restarting");

    const auto stub = sei.hProcess;
    juce::Thread::launch ([stub, failed]
    {
        // Up to ten minutes for the permission prompt to be answered.
        for (int waited = 0; waited < 600000; waited += 200)
        {
            if (centerSetupIsReady())
            {
                CloseHandle (stub);
                juce::MessageManager::callAsync ([] { juce::JUCEApplication::getInstance()->systemRequestedQuit(); });
                return;
            }

            if (WaitForSingleObject (stub, 200) == WAIT_OBJECT_0)
            {
                DWORD code = 0;
                GetExitCodeProcess (stub, &code);
                CloseHandle (stub);
                // The setup ended without ever running elevated: the prompt was refused.
                failed (code == 0 ? juce::String ("The Center update did not start.")
                                  : juce::String ("Windows did not give permission - the Center was not updated."),
                        code == 0 ? "center_update_failed" : "center_update_declined");
                return;
            }
        }
        CloseHandle (stub);
        failed ("The Center update did not start.", "center_update_failed");
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
                centerUpdateMarker().deleteFile();
                juce::ChildProcess open;
                open.start (juce::StringArray { "/usr/bin/open", dmg.getFullPathName() });
                safe->emitStatusMessage ("macOS did not let the Center replace itself. The update is open in Finder: "
                                         "drag RONE Plugins Center onto Applications, choose Replace, then open it again.",
                                         "error", "center_update_manual");
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

        if (! run (juce::StringArray { "/usr/bin/hdiutil", "attach", "-nobrowse", "-noautoopen", "-quiet",
                                       "-mountpoint", mount.getFullPathName(), dmg.getFullPathName() }, 120000))
        {
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
            juce::String why;
            if (run (juce::StringArray { "/usr/bin/ditto", app.getFullPathName(), staged.getFullPathName() }, 300000, &why)
                && run (juce::StringArray { "/bin/mv", target.getFullPathName(), backup.getFullPathName() }, 30000, &why))
            {
                ok = run (juce::StringArray { "/bin/mv", staged.getFullPathName(), target.getFullPathName() }, 30000, &why);
                if (! ok)
                    run (juce::StringArray { "/bin/mv", backup.getFullPathName(), target.getFullPathName() }, 30000);
            }
            staged.deleteRecursively();

            // "Permission denied" = the folder's permissions: the administrator can.
            // "Operation not permitted" = macOS protecting the app: nobody can, so no
            // password is asked for nothing.
            if (! ok && ! why.containsIgnoreCase ("not permitted"))
            {
                auto q = [] (const juce::File& f) { return "\\\"" + f.getFullPathName() + "\\\""; };
                const juce::String sh = "/usr/bin/ditto " + q (app) + " " + q (staged)
                                      + " && /bin/mv " + q (target) + " " + q (backup)
                                      + " && { /bin/mv " + q (staged) + " " + q (target)
                                      + " || { /bin/mv " + q (backup) + " " + q (target) + "; false; }; }"
                                      + " && /bin/rm -rf " + q (backup) + "; R=$?; /bin/rm -rf " + q (staged) + "; exit $R";
                const auto cmd = "osascript -e 'do shell script \"" + sh + "\" with administrator privileges'";
                const int status = std::system (cmd.toRawUTF8());
                ok = WIFEXITED (status) && WEXITSTATUS (status) == 0 && target.isDirectory();
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

        juce::MessageManager::callAsync ([target]
        {
            juce::ChildProcess open;
            open.start (juce::StringArray { "/usr/bin/open", "-n", target.getFullPathName(), "--args", "--updated" });
            juce::Timer::callAfterDelay (400, [] { juce::JUCEApplication::getInstance()->systemRequestedQuit(); });
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

    auto* params = new juce::DynamicObject();
    params->setProperty ("version", target);

    // The bundle carries the base (1.6.3), the catalog the build (1.6.3.250).
    const bool landed = ! VersionChecker::isNewerVersion (installed, target)
                     || target.startsWith (running + ".") || target == running;

    if (landed)
    {
        emitStatusMessage ("RONE Plugins Center updated to v" + target + ".", "success", "center_updated", juce::var (params));
        return;
    }

    juce::String detail;
    const auto lines = juce::StringArray::fromLines (centerUpdateLog().loadFileAsString());
    for (int i = juce::jmax (0, lines.size() - 6); i < lines.size(); ++i)
        detail << lines[i].trim() << "\n";

    emitStatusMessage ("The Center update to v" + target + " did not finish." + (detail.isNotEmpty() ? "\n" + detail.trim() : juce::String()),
                       "error", "center_update_unfinished", juce::var (params));
}

// ============================================================================
// NetworkManager callbacks → push to JS
// ============================================================================

void MainComponent::onManifestReady (const juce::Array<PluginInfo>& plugins, bool fromCache)
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

    // Pre-downloads made for a version the catalog has since moved past are worthless.
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto it = prefetched.begin(); it != prefetched.end();)
        {
            bool current = false;
            for (auto& p : pluginData)
                if (p.id == it->first && p.remoteVersion == it->second.version)
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

        juce::String url, sha;

        if (retryId == "__center__")
        {
            const auto info = networkManager.getCenterInstallerInfo();
           #if JUCE_MAC
            url = info.urlMac; sha = info.sha256Mac;
           #else
            url = info.url; sha = info.sha256;
           #endif
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
                    p.status = PluginStatus::Queued;
                    p.downloadProgress = 0.0;
                    break;
                }
        }

        if (url.isNotEmpty())
        {
            // From here the id means "this download IS the retry": if it fails
            // the hash again, that is the answer, not another round.
            staleHashRetryInFlight = retryId;
            emitPluginsUpdated();
            networkManager.downloadInstaller (retryId, url, sha);
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
                                         const juce::String& errorMessage)
{
    // What was this download for? The status says: a card that is Queued or
    // Downloading wants an install; anything else was a background pre-download.
    PluginStatus status = PluginStatus::NotInstalled;
    juce::String version, name = pluginId;
    {
        juce::ScopedLock sl (pluginDataLock);
        for (auto& p : pluginData)
            if (p.id == pluginId) { status = p.status; version = p.remoteVersion; name = p.name; break; }
    }
    const bool wantsInstall = status == PluginStatus::Queued || status == PluginStatus::Downloading;

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
            applyCenterUpdate (localFile);
        else
            emitStatusMessage (errorMessage.isNotEmpty() ? errorMessage
                                                         : juce::String ("Center update download failed."),
                               "error");
        return;
    }

    if (! wantsInstall)
    {
        // A background pre-download: keep it for its version, quietly.
        if (success)
        {
            prefetched[pluginId] = { version, localFile };
            emitPluginsUpdated();
        }
        batcher.poke();
        return;
    }

    if (success)
    {
        handBatcher (pluginId, localFile);
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
    }

    // Nothing more downloading may be the moment the batch was waiting for.
    batcher.poke();
}
