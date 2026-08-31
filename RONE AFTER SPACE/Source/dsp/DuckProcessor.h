#pragma once

#include <JuceHeader.h>
#include <cmath>

// =============================================================================
// DuckProcessor — AFTERSPACE internal-sidechain ducking (spec §7)
//
// MVP: high-quality broadband duck with a filtered detector. The parameter
// model (amount / attack / hold / release / sensitivity / detector filters)
// already matches the future spectral duck so sessions won't break when
// SMART becomes truly frequency-aware.
// TODO(V1): SpectralDuckProcessor — per-band attenuation weighted by source
// energy (spec §7 SMART DUCK BEHAVIOR).
//
// Control flow: detector env (filtered dry) -> instantaneous duck target ->
// attack/hold/release shaping -> dB attenuation scaled by amount.
// =============================================================================

class DuckProcessor
{
public:
    DuckProcessor() = default;

    void prepare (double sampleRate);
    void reset();

    void setAmount (float amount01);         // 0..1 -> 0..-24 dB max attenuation
    void setAttackMs (float ms);
    void setHoldMs (float ms);
    void setReleaseMs (float ms);
    void setSensitivity (float sens01);      // higher = ducks earlier
    void setDetectorFilters (float hpHz, float lpHz);

    // Returns the linear gain to apply to the wet path for this sample.
    float processSample (float dryL, float dryR);

    // Current gain reduction in dB (>= 0 = none), for UI metering
    float getGainReductionDb() const { return currentGrDb_; }

private:
    double sampleRate_ = 48000.0;

    // Detector filters (one-pole)
    float detHpCoeff_ = 0.0f, detLpCoeff_ = 1.0f;
    float detHpState_ = 0.0f, detLpState_ = 0.0f;

    // Envelope follower on detector (fast, fixed)
    float env_ = 0.0f;
    float envAttackCoeff_ = 0.0f, envReleaseCoeff_ = 0.0f;

    // Duck control signal shaping
    float control_ = 0.0f;          // 0..1 duck depth
    float attackCoeff_ = 0.0f, releaseCoeff_ = 0.0f;
    int   holdSamples_ = 0, holdCounter_ = 0;

    float amount_ = 0.2f;
    float thresholdDb_ = -33.0f;

    juce::SmoothedValue<float> smGain_;
    float currentGrDb_ = 0.0f;

    static constexpr float kMaxAttenDb = -24.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DuckProcessor)
};
