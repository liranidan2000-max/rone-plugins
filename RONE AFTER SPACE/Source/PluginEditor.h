#pragma once

#include "PluginProcessor.h"
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
    juce::WebBrowserComponent webView;
    RoneAboutOverlay aboutOverlay { "AFTERSPACE", JucePlugin_VersionString,
                                    juce::Colour (0xffFF8A3D) };
    std::unique_ptr<CustomTitleBar> customTitleBar;
    bool isStandalone = false;

    // License check throttle (~5s at 30Hz)
    static constexpr int kLicenseCheckInterval = 150;
    int licenseCheckCounter = kLicenseCheckInterval;

    void sendLicenseStatusToJS();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RoneAfterspaceAudioProcessorEditor)
};
