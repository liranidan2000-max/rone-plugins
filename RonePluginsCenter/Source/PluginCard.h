#pragma once
#include <JuceHeader.h>
#include "VersionChecker.h"
#include "Styles.h"

// ============================================================================
// PluginCard — one visual card in the grid representing a single plugin
// ============================================================================
class PluginCard : public juce::Component,
                   public juce::Button::Listener
{
public:
    // Callback when the user clicks the action button
    std::function<void (const juce::String& pluginId)> onActionClicked;
    // Callback when the user clicks "Open" standalone
    std::function<void (const juce::String& pluginId)> onOpenClicked;
    // Callback to show changelog
    std::function<void (const juce::String& pluginId)> onInfoClicked;

    PluginCard();

    void setPluginInfo (const PluginInfo& info);
    void setDownloadProgress (double progress);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void buttonClicked (juce::Button* button) override;

private:
    PluginInfo pluginInfo;

    juce::Label       nameLabel;
    juce::Label       descLabel;
    juce::Label       versionLabel;
    juce::Label       formatsLabel;
    juce::TextButton  actionButton  { "INSTALL" };
    juce::TextButton  openButton    { "OPEN" };
    juce::TextButton  infoButton    { "i" };
    juce::ProgressBar progressBar   { pluginInfo.downloadProgress };

    void updateButtonState();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginCard)
};
