#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>
#include <cmath>

// =============================================================================
// ReverbEngine — AFTERSPACE late-field reverb
//
// Topology (spec §14/§15):
//   pre-delay -> early reflection taps
//             -> input diffusion (4 series allpasses per channel)
//             -> 8-line modulated FDN (Householder feedback)
//                per-line: high damp (one-pole LP), low damp (one-pole HP),
//                          density allpass, LFO-modulated interpolated read
//   output = equal-power blend of early/late per earlyLate balance
//
// Real-time safety: all buffers allocated in prepare(); process() never
// allocates. All gain-critical params ride juce::SmoothedValue ramps.
// Freeze crossfades feedback -> 1.0 and input -> 0 (click-free, spec §10).
// =============================================================================

class ReverbEngine
{
public:
    static constexpr int kNumLines = 8;

    ReverbEngine() = default;

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Block-rate parameter setters (cheap; called from processBlock)
    void setSize (float size01);            // 0..1
    void setDecaySeconds (float seconds);
    void setPreDelayMs (float ms);
    void setDiffusion (float amount01);
    void setDensity (float amount01);
    void setModRate (float hz);
    void setModDepth (float amount01);
    void setHighDamp (float amount01);
    void setLowDamp (float amount01);
    void setEarlyLateBalance (float balance); // -1 early .. +1 late
    void setFreeze (bool shouldFreeze);

    void processSample (float inL, float inR, float& outL, float& outR);

    bool isFrozen() const { return freezeTarget_; }

private:
    void updateFeedbackGains();

    double sampleRate_ = 48000.0;
    float  srRatio_    = 1.0f;   // sampleRate / 48000

    // --- Pre-delay + early reflection source buffer ---
    std::vector<float> preBufL_, preBufR_;
    int preWritePos_ = 0;
    int preBufSize_  = 0;
    juce::SmoothedValue<float> smPreDelaySamples_;

    // Early reflection taps (offsets in samples, scaled by size at block rate)
    static constexpr int kNumErTaps = 8;
    std::array<float, kNumErTaps> erTapSamplesL_ {};
    std::array<float, kNumErTaps> erTapSamplesR_ {};
    std::array<float, kNumErTaps> erGains_ {};
    float erLpStateL_ = 0.0f, erLpStateR_ = 0.0f;

    // --- Input diffusion allpasses (4 per channel) ---
    static constexpr int kNumDiff = 4;
    std::array<std::vector<float>, kNumDiff> diffBufL_, diffBufR_;
    std::array<int, kNumDiff> diffPosL_ {}, diffPosR_ {};
    std::array<int, kNumDiff> diffLen_ {};
    juce::SmoothedValue<float> smDiffCoeff_;

    // --- FDN ---
    std::array<std::vector<float>, kNumLines> fdnBuf_;
    std::array<int, kNumLines> fdnWritePos_ {};
    std::array<int, kNumLines> fdnBufSize_ {};
    std::array<juce::SmoothedValue<float>, kNumLines> smLineLen_;
    std::array<juce::SmoothedValue<float>, kNumLines> smFeedback_;
    std::array<float, kNumLines> baseLenScaled_ {};   // current target lengths

    // Per-line damping state
    std::array<float, kNumLines> hiDampState_ {};
    std::array<float, kNumLines> loDampState_ {};
    juce::SmoothedValue<float> smHiDampCoeff_;  // one-pole LP coeff
    juce::SmoothedValue<float> smLoDampCoeff_;  // one-pole HP tracker coeff

    // Per-line density allpass
    std::array<std::vector<float>, kNumLines> densBuf_;
    std::array<int, kNumLines> densPos_ {};
    std::array<int, kNumLines> densLen_ {};
    juce::SmoothedValue<float> smDensCoeff_;

    // Per-line LFO
    std::array<float, kNumLines> lfoPhase_ {};
    std::array<float, kNumLines> lfoInc_ {};
    juce::SmoothedValue<float> smModDepthSamples_;
    float modRateHz_ = 0.25f;

    // Freeze crossfade
    juce::SmoothedValue<float> smFreeze_;   // 0 = normal, 1 = frozen
    bool freezeTarget_ = false;

    // Early/late equal-power gains
    juce::SmoothedValue<float> smErGain_, smLateGain_;

    // Cached params for feedback-gain recompute
    float decaySeconds_ = 2.2f;
    float size_ = 0.55f;

    // Read helper: linear interpolated read from a circular buffer
    static float readInterp (const std::vector<float>& buf, int bufSize,
                             int writePos, float delaySamples);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbEngine)
};
