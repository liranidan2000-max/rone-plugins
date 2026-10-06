#pragma once
#include <JuceHeader.h>
#include <map>
#include "NetworkManager.h"
#include "VersionChecker.h"
#include "LicenseHandler.h"
#include "AccountClient.h"
#include "CustomTitleBar.h"
#include "InstallBatcher.h"
#include "CenterLinks.h"

// ============================================================================
// MainComponent — WebView host for the React UI
// ============================================================================
class MainComponent : public juce::Component,
                      public NetworkManager::Listener,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void resized() override;
    void paint (juce::Graphics&) override;
    void parentHierarchyChanged() override;

    // The page lives only while the window can be seen (MainWindow calls this on
    // show / hide / minimise). Hidden in the tray the Center keeps its backend -
    // manifest, downloads, installs, licence and account checks - but no
    // WebView2: releasing it lets its six msedgewebview2 processes (~240 MB
    // private on 2026-09-23) exit. Showing the window builds a fresh page.
    void setUiVisible (bool shouldBeLive);

    // Behind another app (typically the DAW) the page pauses its endless CSS
    // animations - the sidebar equaliser, pulsing dots, shimmer - so a window
    // left open costs no GPU frames. MainWindow reports the foreground state.
    void setWindowActive (bool isActive);

    // A plugin's UPDATE button launched us with "--update <id>" (or forwarded
    // it to the running Center): update that one plugin, with a fresh manifest
    // first unless it already knows the update.
    void requestUpdate (const juce::String& productId);

    // A ronecenter:// link (CenterLinks.h): install / open a plugin's page / Updates.
    void handleLink (const CenterLinks::Link& link);

    // Downloads, installs or uninstalls in progress - the tray's Quit asks first.
    bool isWorking();

    // The tray icon shows how many updates wait (Main.cpp wires this up).
    std::function<void (int updates)> onUpdateCountChanged;

    // NetworkManager::Listener
    void onManifestReady  (const juce::Array<PluginInfo>& plugins, bool fromCache) override;
    void onManifestError  (const juce::String& errorMessage) override;
    void onDownloadProgress (const juce::String& pluginId, double progress) override;
    void onDownloadStarted (const juce::String& pluginId) override;
    void onDownloadComplete (const juce::String& pluginId,
                             const juce::File& localFile,
                             bool success,
                             bool cancelled,
                             const juce::String& errorMessage) override;

private:
    // ---- Resource provider ----
    std::optional<juce::WebBrowserComponent::Resource>
        getResource (const juce::String& url);
    static juce::String getMimeForExtension (const juce::String& ext);

    // ---- Serialisation helpers ----
    juce::var pluginInfoToVar (const PluginInfo& info);
    juce::var allPluginsToVar();
    juce::String statusToString (PluginStatus s);

    // ---- Native function handlers (JS → C++) ----
    using NativeArgs       = const juce::Array<juce::var>&;
    using NativeCompletion = juce::WebBrowserComponent::NativeFunctionCompletion;

    juce::WebBrowserComponent::Options makeWebOptions();

    // A completion that settles only the page that asked. JUCE's own completion
    // points into the WebBrowserComponent that made the call; once the window is
    // hidden that component is gone, and a late answer (a Google sign-in that
    // finishes after the window closed) would otherwise write into freed memory.
    NativeCompletion guarded (NativeCompletion complete);

    // Every event to the page goes through here: dropped while there is no page
    // (the page asks for fresh state when it loads), marshalled to the message
    // thread when a background thread sends it.
    void emitToPage (const juce::Identifier& eventId, const juce::var& payload);

    // Starts the download (then the install) of one plugin; false + error text otherwise.
    bool startInstall (const juce::String& pluginId, juce::String& error);
    void servePendingUpdate();
    bool anyPluginBusy();

    void handleGetPlugins      (NativeArgs args, NativeCompletion complete);
    void handleInstallPlugin   (NativeArgs args, NativeCompletion complete);
    void handleOpenPlugin      (NativeArgs args, NativeCompletion complete);
    void handleOpenManual      (NativeArgs args, NativeCompletion complete);
    void handleOpenFolder      (NativeArgs args, NativeCompletion complete);
    void handleOpenInstallFolder (NativeArgs args, NativeCompletion complete);
    void handleRefreshPlugins  (NativeArgs args, NativeCompletion complete);
    void handleActivateLicense (NativeArgs args, NativeCompletion complete);
    void handleDeactivateLicense (NativeArgs args, NativeCompletion complete);
    void handleGetLicenseStatus(NativeArgs args, NativeCompletion complete);

    // Account sign-in (roneaudio.com); the license-key handlers above stay for
    // customers who bought before accounts existed.
    void handleAccountSignIn (NativeArgs args, NativeCompletion complete);
    void handleAccountSignOut (NativeArgs args, NativeCompletion complete);
    void handleAccountGoogleSignIn (NativeArgs args, NativeCompletion complete);
    void handleAccountGoogleCancel (NativeArgs args, NativeCompletion complete);
    void handleGetAccountStatus (NativeArgs args, NativeCompletion complete);
    juce::var accountStatusVar() const;
    void handleGetAppVersion   (NativeArgs args, NativeCompletion complete);

    // The site's popups (roneaudio.com/api/v1/popup): a new plugin, a free one, a
    // deal - the same list the website shows, so one place (the admin console)
    // runs both. Fetched here because the endpoint sends no CORS headers.
    void handleGetAnnouncements (NativeArgs args, NativeCompletion complete);
    void handleGetAutoStart    (NativeArgs args, NativeCompletion complete);

    // Settings > DELETE OLD VERSIONS (OldVersionCleaner.h): list, then delete.
    void handleScanOldVersions   (NativeArgs args, NativeCompletion complete);
    void handleDeleteOldVersions (NativeArgs args, NativeCompletion complete);

    // UNINSTALL in a card's menu (PluginUninstaller.h)
    void handleUninstallPlugin   (NativeArgs args, NativeCompletion complete);
    void handleSetAutoStart    (NativeArgs args, NativeCompletion complete);

    // Center 2.0
    void handleCancelInstall     (NativeArgs args, NativeCompletion complete);   // one plugin, or all with no id
    void handleGetDaws           (NativeArgs args, NativeCompletion complete);
    void handleGetCenterSettings (NativeArgs args, NativeCompletion complete);
    void handleSetCenterSetting  (NativeArgs args, NativeCompletion complete);

    // ---- Window controls ----
    bool beginNativeWindowDrag();

    // ---- Center self-update ----
public:
    // Reads the version the installer stamped for the Center itself; empty on
    // installs older than the self-update feature. Public: the version-compare
    // helper lives at file scope in the .cpp.
    static juce::String readInstalledCenterVersion();

private:
    void checkForCenterUpdate();                       // call after each manifest fetch
    void handleApplyCenterUpdate (NativeArgs args, NativeCompletion complete);
    void applyCenterUpdate (const juce::File& installerFile);
    void applyCenterUpdateWhenIdle();                  // never in the middle of an install
    void reportCenterUpdateOutcome();                  // after a restart: did the update land?

    // ---- Installs ----
    void handBatcher (const juce::String& pluginId, const juce::File& installer);
    void onBatchStatus (const juce::String& id, PluginStatus status, const juce::String& waitingFor);
    void onBatchResult (const InstallBatcher::Result& result);
    void setStatus (const juce::String& id, PluginStatus status, const juce::String& waitingFor = {});
    void restoreIdleState (const juce::String& id);    // after a cancel: whatever is on disk now
    void startBackgroundDownloads();                   // pre-download the updates the user owns
    int  countUpdates();
    void publishUpdateCount();

    // ---- Periodic checks (juce::Timer, once a minute) ----
    void timerCallback() override;
    void fetchManifestNow();

    // ---- Emit helper ----
    void emitPluginsUpdated();
    // `code` + `params` let the page say it in the user's language; `text` is the
    // English it falls back to (and what an error report carries).
    void emitStatusMessage (const juce::String& text, const juce::String& type,
                            const juce::String& code = {}, const juce::var& params = {});
    juce::var manifestStateVar() const;

    // ---- Members ----
    CustomTitleBar            titleBar;
    std::unique_ptr<juce::WebBrowserComponent> webView;   // only while the window is shown
    juce::uint32              webGeneration = 0;           // bumped each time a page is made or released
    NetworkManager            networkManager;
    LicenseHandler            licenseHandler;
    AccountClient             accountClient;
    juce::int64               lastAccountCheckMs = 0;   // setWindowActive's own throttle on /app/refresh
    juce::Array<PluginInfo>   pluginData;
    juce::CriticalSection     pluginDataLock;  // guards pluginData access across threads
    juce::String              pendingUpdateId;  // from --update, served once the manifest is in

    // The plugin whose download failed its hash check and is waiting on a fresh
    // manifest to try once more. See onDownloadComplete: the installers live
    // behind moving "-latest" tags, so a Center left open across a release ends
    // up checking a NEW file against the hash it read hours ago. Empty when no
    // retry is pending. Once the retry download is running its id moves to
    // staleHashRetryInFlight, and a second failure of THAT download is final -
    // the first version cleared the id before the retry ran, so every failure
    // looked like a first one and the Analyzer update looped on 2026-09-11.
    juce::String              staleHashRetryId;
    juce::String              staleHashRetryInFlight;

    // ---- Center 2.0 state (message thread only) ----
    InstallBatcher            batcher;

    // Background pre-downloads that finished: the verified installer, kept for
    // the version it was fetched for, so Update goes straight to installing.
    struct Prefetched { juce::String version; juce::File file; };
    std::map<juce::String, Prefetched> prefetched;

    bool                      manifestFromCache = false;   // offline: showing the last manifest that arrived
    bool                      manifestEverLoaded = false;
    juce::int64               lastManifestAttemptMs = 0;
    int                       offlineRetries = 0;
    int                       lastPublishedUpdates = -1;
    bool                      centerUpdateWanted = false;  // asked for while something was installing
    CenterLinks::Link         pendingLink;                 // waits for the manifest / the account check
    juce::String              pendingNavigation;           // "plugin:RoneIron" / "updates" for the page
    bool                      pageReady = false;           // the live page has pulled getPlugins
    bool                      offlineSeen = false;         // "offline" was said; "back online" is owed
    juce::Array<juce::var>    pendingMessages;             // said before the page was up (start-up)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
