#pragma once

#include <JuceHeader.h>
#include "DSP/RoneStuckerEngine.h"

namespace RoneParamIDs
{
    inline constexpr const char* AMOUNT = "amount"; // the one knob
    inline constexpr const char* BYPASS = "bypass"; // top-right power button
    inline constexpr const char* SYNC   = "sync";   // top-left toggle — grid-locked capture
    inline constexpr const char* SMOOTH = "smooth"; // advanced — loop crossfade
    inline constexpr const char* LENGTH = "length"; // advanced — capture division
    inline constexpr const char* RANGE  = "range";  // advanced — shrink depth (octaves)
    inline constexpr const char* LOWCUT = "lowcut"; // advanced — wet high-pass, 20-500 Hz
}

class RoneStuckerAudioProcessor : public juce::AudioProcessor
{
public:
    RoneStuckerAudioProcessor();
    ~RoneStuckerAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Rone Stucker"; }
    bool acceptsMidi() const override           { return false; }
    bool producesMidi() const override          { return false; }
    bool isMidiEffect() const override          { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override                { return 1; }
    int getCurrentProgram() override             { return 0; }
    void setCurrentProgram (int) override        {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    RoneStuckerEngine engine;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::atomic<float>* amountParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* syncParam   = nullptr;
    std::atomic<float>* smoothParam = nullptr;
    std::atomic<float>* lengthParam = nullptr;
    std::atomic<float>* rangeParam  = nullptr;
    std::atomic<float>* lowcutParam = nullptr;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RoneStuckerAudioProcessor)
};
