#pragma once
#include <JuceHeader.h>
#include "Styles.h"
#include "PluginCard.h"
#include "NetworkManager.h"
#include "VersionChecker.h"
#include "LicenseHandler.h"

// ============================================================================
// MainComponent — root UI with 2-column grid + header + license bar
// ============================================================================
class MainComponent : public juce::Component,
                      public NetworkManager::Listener,
                      public juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    // NetworkManager::Listener
    void onManifestReady  (const juce::Array<PluginInfo>& plugins) override;
    void onManifestError  (const juce::String& errorMessage) override;
    void onDownloadProgress (const juce::String& pluginId, double progress) override;
    void onDownloadComplete (const juce::String& pluginId,
                             const juce::File& localFile,
                             bool success,
                             const juce::String& errorMessage) override;

    // Timer — auto-refresh + fade-in animation
    void timerCallback() override;

private:
    void refreshPlugins();
    void handleAction (const juce::String& pluginId);
    void handleOpen   (const juce::String& pluginId);
    void handleInfo   (const juce::String& pluginId);
    void launchSilentInstaller (const juce::File& installerExe,
                                 const juce::String& pluginId);

    void handleActivate();
    void handleDeactivate();
    void updateLicenseUI();
    void refreshCardLicenseState();

    PluginCard* findCard (const juce::String& pluginId);

    // Data
    juce::Array<PluginInfo>          pluginData;
    juce::OwnedArray<PluginCard>     cards;
    NetworkManager                   networkManager;
    LicenseHandler                   licenseHandler;

    // UI
    RoneLookAndFeel  roneLnf;
    juce::Label      titleLabel;
    juce::Label      statusLabel;
    juce::TextButton refreshButton { "Refresh" };
    juce::Component  cardContainer;
    juce::Viewport   viewport;

    // License UI
    juce::TextEditor licenseKeyInput;
    juce::TextButton activateButton   { "ACTIVATE" };
    juce::TextButton deactivateButton { "DEACTIVATE" };
    juce::Label      licenseStatusLabel;
    juce::Label      proBadge;

    // Fade-in animation
    float fadeAlpha = 0.0f;
    bool  fadeComplete = false;
    int   fadeTimerId = 1;
    int   refreshTimerId = 2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
