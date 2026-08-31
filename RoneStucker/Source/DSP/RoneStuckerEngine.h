#pragma once

#include <JuceHeader.h>

// ============================================================================
// RoneStuckerEngine — one-knob buffer-retrigger roll-up ("stutter riser").
//
// Signal model:
//   * The input is continuously recorded into a circular capture buffer.
//   * When AMOUNT leaves zero, the last tempo-synced slice is frozen and
//     looped in place of the dry signal.
//   * As AMOUNT increases the loop length shrinks exponentially; below ~50ms
//     the retrigger rate crosses into audio rate and the slice behaves like
//     a wavetable oscillator whose pitch rises as the loop keeps shrinking.
//   * Returning AMOUNT to zero releases back to the live input.
//
// All buffers are allocated in prepare(); process() is allocation-free.
// ============================================================================

class RoneStuckerEngine
{
public:
    RoneStuckerEngine() = default;

    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();

    // Called once per block from the audio thread — cheap atomics only.
    // syncOffsetSamples: samples elapsed since the last grid line of the
    // capture division (>= 0 locks the captured slice to the host grid);
    // pass a negative value for free (unsynced) capture.
    void setParameters (float amount, float smooth, float rangeOctaves,
                        float lowCutHz, float baseLoopLengthSamples,
                        float syncOffsetSamples) noexcept;

    void process (juce::AudioBuffer<float>& buffer) noexcept;

    // 0..1 — how far into the shrink the engine currently is (for UI).
    float getCurrentDepth() const noexcept  { return uiDepth.load(); }
    bool  isEngaged() const noexcept        { return uiEngaged.load(); }
    float getCurrentLoopMs() const noexcept { return uiLoopMs.load(); }

private:
    float readInterpolated (int channel, double position) const noexcept;

    juce::AudioBuffer<float> captureBuffer;
    int bufferLength = 0;
    int numChannels  = 2;
    int writePos     = 0;

    double sampleRateHz = 44100.0;

    // Block-rate parameter targets
    std::atomic<float> amountTarget  { 0.0f };
    std::atomic<float> smoothTarget  { 0.35f };
    std::atomic<float> octavesTarget { 8.0f };
    std::atomic<float> lowCutTarget  { 20.0f };
    std::atomic<float> baseLenTarget { 22050.0f };
    std::atomic<float> syncOffsetTarget { -1.0f };

    // Low cut on the wet (looped) signal only
    juce::dsp::StateVariableTPTFilter<float> lowCutFilter;

    // Sample-rate smoothed values
    juce::SmoothedValue<float> amountSmoothed;  // knob position
    juce::SmoothedValue<float> wetGainSmoothed; // engage/release crossfade
    double loopLenSmoothed = 0.0;               // one-pole smoothed loop length
    double loopLenCoeff    = 0.0;

    // Loop state
    bool   engaged      = false;
    int    captureStart = 0;
    double captureLen   = 0.0;
    double phase        = 0.0;

    // UI feedback
    std::atomic<float> uiDepth   { 0.0f };
    std::atomic<bool>  uiEngaged { false };
    std::atomic<float> uiLoopMs  { 0.0f };

    static constexpr double minLoopSamples = 32.0; // audio-rate ceiling (~1.4kHz @44.1k)
    static constexpr float  engageEpsilon  = 0.001f;
};
