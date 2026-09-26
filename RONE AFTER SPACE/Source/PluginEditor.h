#pragma once

#include "PluginProcessor.h"
#include "../../Shared/RoneUpdatePrompt.h"
#include "CustomTitleBar.h"
#include "../../Shared/RoneAboutOverlay.h"

class RoneAfterspaceAudioProcessorEditor : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    static constexpr int kWidth  = 1150;
    static constexpr int kHeight = 780;

    // Resize range: 0.7x .. 1.6x of the base size, aspect locked
    static constexpr int kMinWidth  = 805;
    static constexpr int kMinHeight = 546;
    static constexpr int kMaxWidth  = 1840;
    static constexpr int kMaxHeight = 1248;

    explicit RoneAfterspaceAudioProcessorEditor (RoneAfterspaceAudioProcessor&);
    ~RoneAfterspaceAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

private:
    void timerCallback() override;

    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);
    void sendAllParametersToJS();

    RoneAfterspaceAudioProcessor& processorRef;
    RoneUpdatePrompt updatePrompt;            // before the WebView: its options carry the prompt's script
    juce::WebBrowserComponent webView;
    RoneAboutOverlay aboutOverlay { "AFTERSPACE", JucePlugin_VersionString,
                                    juce::Colour (0xffFF8A3D) };
    std::unique_ptr<CustomTitleBar> customTitleBar;
    bool isStandalone = false;

    // Nothing goes to the page until it says it is up ("uiReady", sent once its bridge
    // listeners are in). JUCE 8.0.4 builds WebView2s one at a time, and a script sent to
    // a view that does not exist yet can leave it blank for good when another window of
    // this plugin is being built at that moment (two windows reopened together).
    bool pageReady = false;

    // Content width when a corner-grip drag started (see "beginResize")
    int resizeBaseW = kWidth;

    // License check throttle (~5s at 30Hz)
    static constexpr int kLicenseCheckInterval = 150;
    int licenseCheckCounter = kLicenseCheckInterval;

    void sendLicenseStatusToJS();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RoneAfterspaceAudioProcessorEditor)
};
