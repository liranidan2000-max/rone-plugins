#pragma once
#include <JuceHeader.h>
#include "VersionChecker.h"
#include "Styles.h"

// ============================================================================
// PluginCard — one visual card representing a single plugin
// ============================================================================
class PluginCard : public juce::Component,
                   public juce::Button::Listener
{
public:
    std::function<void (const juce::String& pluginId)> onActionClicked;
    std::function<void (const juce::String& pluginId)> onOpenClicked;
    std::function<void (const juce::String& pluginId)> onInfoClicked;

    PluginCard();

    void setPluginInfo (const PluginInfo& info);
    void setDownloadProgress (double progress);
    void setLicensed (bool isLicensed);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void buttonClicked (juce::Button* button) override;

private:
    PluginInfo pluginInfo;
    bool licensed = false;
    juce::Image pluginLogo;

    juce::Label       nameLabel;
    juce::Label       descLabel;
    juce::Label       versionLabel;
    juce::TextButton  actionButton  { "INSTALL" };
    juce::TextButton  openButton    { "OPEN" };
    juce::TextButton  infoButton    { "i" };
    juce::ProgressBar progressBar   { pluginInfo.downloadProgress };

    void updateButtonState();
    void loadPluginLogo();
    void drawFormatBadges (juce::Graphics& g, juce::Rectangle<int> area);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginCard)
};
