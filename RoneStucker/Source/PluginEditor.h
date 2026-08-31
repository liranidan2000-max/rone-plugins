#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class RoneStuckerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit RoneStuckerAudioProcessorEditor (RoneStuckerAudioProcessor&);
    ~RoneStuckerAudioProcessorEditor() override;

    void resized() override;

private:
    void timerCallback() override;
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);
    void pushParamsToJS();

    RoneStuckerAudioProcessor& processorRef;
    juce::WebBrowserComponent webView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RoneStuckerAudioProcessorEditor)
};
