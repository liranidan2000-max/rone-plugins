#pragma once
#include <JuceHeader.h>
#include "NetworkManager.h"
#include "VersionChecker.h"
#include "LicenseHandler.h"
#include "AccountClient.h"
#include "CustomTitleBar.h"

// ============================================================================
// MainComponent — WebView host for the React UI
// ============================================================================
class MainComponent : public juce::Component,
                      public NetworkManager::Listener
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

    // NetworkManager::Listener
    void onManifestReady  (const juce::Array<PluginInfo>& plugins) override;
    void onManifestError  (const juce::String& errorMessage) override;
    void onDownloadProgress (const juce::String& pluginId, double progress) override;
    void onDownloadComplete (const juce::String& pluginId,
                             const juce::File& localFile,
                             bool success,
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
    void handleGetAutoStart    (NativeArgs args, NativeCompletion complete);
    void handleSetAutoStart    (NativeArgs args, NativeCompletion complete);

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

    // ---- Backend logic (carried over) ----
    void launchSilentInstaller (const juce::File& installer,
                                 const juce::String& pluginId);

    // ---- Emit helper ----
    void emitPluginsUpdated();
    void emitStatusMessage (const juce::String& text, const juce::String& type);

    // ---- Members ----
    CustomTitleBar            titleBar;
    std::unique_ptr<juce::WebBrowserComponent> webView;   // only while the window is shown
    juce::uint32              webGeneration = 0;           // bumped each time a page is made or released
    NetworkManager            networkManager;
    LicenseHandler            licenseHandler;
    AccountClient             accountClient;
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
