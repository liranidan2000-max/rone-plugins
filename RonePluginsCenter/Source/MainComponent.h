#pragma once
#include <JuceHeader.h>
#include "NetworkManager.h"
#include "VersionChecker.h"
#include "LicenseHandler.h"
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

    void handleGetPlugins      (NativeArgs args, NativeCompletion complete);
    void handleInstallPlugin   (NativeArgs args, NativeCompletion complete);
    void handleOpenPlugin      (NativeArgs args, NativeCompletion complete);
    void handleRefreshPlugins  (NativeArgs args, NativeCompletion complete);
    void handleActivateLicense (NativeArgs args, NativeCompletion complete);
    void handleDeactivateLicense (NativeArgs args, NativeCompletion complete);
    void handleGetLicenseStatus(NativeArgs args, NativeCompletion complete);
    void handleGetAppVersion   (NativeArgs args, NativeCompletion complete);

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
    juce::WebBrowserComponent webView;
    NetworkManager            networkManager;
    LicenseHandler            licenseHandler;
    juce::Array<PluginInfo>   pluginData;
    juce::CriticalSection     pluginDataLock;  // guards pluginData access across threads

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
