#pragma once

#include <JuceHeader.h>
#include "Parameters.h"
#include "dsp/ReverbEngine.h"
#include "dsp/EchoEngine.h"
#include "dsp/DuckProcessor.h"
#include "dsp/SilkProcessor.h"
#include "../../Shared/BundleLicenseChecker.h"
#include "../../Shared/RemoteLicenseGate.h"
#include "../../Shared/RoneCrashReporter.h"

// Lock-free visualization data (audio -> UI), sync-verb pattern
static constexpr int kVisualizerBufferSize = 256;
static constexpr int kVisualizerPoints     = 64;

class RoneAfterspaceAudioProcessor : public juce::AudioProcessor,
                                    private juce::Timer
{
public:
    RoneAfterspaceAudioProcessor();
    ~RoneAfterspaceAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "RONE AFTERSPACE"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // State schema version — bump + migrate explicitly when the model changes
    static constexpr int kStateSchemaVersion = 1;

    juce::AudioProcessorValueTreeState apvts;

    // === Preset name (message thread only; serialized with state) ===
    juce::String getPresetName() const;
    void setPresetName (const juce::String& name);

    // === Host tempo ===
    double getEffectiveBPM() const;
    std::atomic<double> dawBpm { 0.0 };
    std::atomic<bool>   dawBpmAvailable { false };
    double getCurrentEchoTimeMs() const;

    // === License ===
    std::atomic<bool> licenseValid { false };
    void checkBundleLicense();
    void timerCallback() override { checkBundleLicense(); }

    // === UI size (content area, excl. standalone title bar; spec §24) ===
    std::atomic<int> uiWidth  { 1150 };
    std::atomic<int> uiHeight { 780 };

    // === Visualization (audio thread writes, UI thread reads) ===
    std::array<std::atomic<float>, kVisualizerBufferSize> visualizerRing {};
    std::atomic<int>   visualizerWritePos { 0 };
    std::atomic<float> currentDuckGrDb { 0.0f };
    std::atomic<float> currentSilkGrDb { 0.0f };
    std::atomic<float> currentWetRms { 0.0f };

private:
    // Cached raw parameter pointers (fast processBlock reads)
    std::atomic<float>* pMix        = nullptr;
    std::atomic<float>* pSize       = nullptr;
    std::atomic<float>* pDecay      = nullptr;
    std::atomic<float>* pBloom      = nullptr;
    std::atomic<float>* pSilk       = nullptr;
    std::atomic<float>* pAutoGain   = nullptr;
    std::atomic<float>* pSoloWet    = nullptr;
    std::atomic<float>* pCharacter  = nullptr;
    std::atomic<float>* pBypass     = nullptr;
    std::atomic<float>* pFreeze     = nullptr;
    std::atomic<float>* pDuckAmount = nullptr;
    std::atomic<float>* pDuckMode   = nullptr;
    std::atomic<float>* pDuckAttack = nullptr;
    std::atomic<float>* pDuckHold   = nullptr;
    std::atomic<float>* pDuckRelease = nullptr;
    std::atomic<float>* pDuckSens   = nullptr;
    std::atomic<float>* pEchoAmount = nullptr;
    std::atomic<float>* pEchoTime   = nullptr;
    std::atomic<float>* pEchoFreeMs = nullptr;
    std::atomic<float>* pEchoFeedback = nullptr;
    std::atomic<float>* pEchoRouting  = nullptr;
    std::atomic<float>* pEchoPingPong = nullptr;
    std::atomic<float>* pEchoLowCut   = nullptr;
    std::atomic<float>* pEchoHighCut  = nullptr;
    std::atomic<float>* pPreDelay   = nullptr;
    std::atomic<float>* pTexture    = nullptr;
    std::atomic<float>* pEarlyLate  = nullptr;
    std::atomic<float>* pLowCut     = nullptr;
    std::atomic<float>* pHighCut    = nullptr;
    std::atomic<float>* pLowDamp    = nullptr;
    std::atomic<float>* pHighDamp   = nullptr;
    std::atomic<float>* pModRate    = nullptr;
    std::atomic<float>* pModDepth   = nullptr;
    std::atomic<float>* pWidth      = nullptr;
    std::atomic<float>* pMonoLow    = nullptr;

    // === DSP ===
    ReverbEngine  reverb;
    EchoEngine    echo;
    DuckProcessor duck;
    SilkProcessor silk;

    // Wet tone shaping (spec §14 step 10)
    juce::dsp::StateVariableTPTFilter<float> wetLowCut, wetHighCut;

    // Smoothers
    juce::SmoothedValue<float> smMix;         // 0..1, equal-power applied per sample
    juce::SmoothedValue<float> smEchoSend;
    juce::SmoothedValue<float> smWidth;
    juce::SmoothedValue<float> smBypass;      // 0 = active, 1 = bypassed
    juce::SmoothedValue<float> smWetComp;     // auto-gain factor
    juce::SmoothedValue<float> smMonoLow;     // 0..1 crossfade
    juce::SmoothedValue<float> smSoloWet;     // 0..1 crossfade

    // Mono-low side filter state (one-pole LP on the side signal)
    float sideLowState = 0.0f;
    float sideLowCoeff = 0.02f;

    double currentSampleRate = 48000.0;

    // Preset name (message thread); read by editor timer, written on preset load
    juce::CriticalSection presetNameLock;
    juce::String presetName { "Init - Clean Space" };

    // Visualizer decimation
    float vizAccum = 0.0f;
    int   vizAccumCount = 0;
    int   vizDecimationInterval = 480;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RoneAfterspaceAudioProcessor)
};
