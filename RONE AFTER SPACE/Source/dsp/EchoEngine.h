#pragma once

#include <JuceHeader.h>
#include <vector>
#include <cmath>

// =============================================================================
// EchoEngine — tempo-synced stereo/ping-pong delay for AFTERSPACE
//
// - Interpolated reads with a slow smoothed delay time (tape-style glide on
//   tempo/time changes; spec §18 allows this as an intentional transition).
// - Feedback loop: low cut + high cut one-pole filters + tanh soft clip.
//   Feedback is clamped to 0.95 — runaway is impossible (spec §8 SAFETY).
// - Routing (Before/Inside/After) is wired at the processor level.
// =============================================================================

class EchoEngine
{
public:
    EchoEngine() = default;

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    void setTimeMs (float ms);            // 10..2000+
    void setFeedback (float fb01);        // 0..0.95
    void setPingPong (bool enabled);
    void setLowCutHz (float hz);
    void setHighCutHz (float hz);

    void processSample (float inL, float inR, float& outL, float& outR);

private:
    double sampleRate_ = 48000.0;

    std::vector<float> bufL_, bufR_;
    int bufSize_ = 0;
    int writePos_ = 0;

    juce::SmoothedValue<float> smDelaySamples_;
    juce::SmoothedValue<float> smFeedback_;
    juce::SmoothedValue<float> smPingPong_;   // 0 = stereo, 1 = ping-pong (crossfaded)

    // Feedback-path filters (one-pole)
    float lpCoeff_ = 1.0f, hpCoeff_ = 0.0f;
    float lpStateL_ = 0.0f, lpStateR_ = 0.0f;
    float hpStateL_ = 0.0f, hpStateR_ = 0.0f;

    static float readInterp (const std::vector<float>& buf, int bufSize,
                             int writePos, float delaySamples);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EchoEngine)
};
