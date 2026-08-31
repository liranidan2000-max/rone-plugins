#include "DuckProcessor.h"

void DuckProcessor::prepare (double sampleRate)
{
    sampleRate_ = sampleRate;

    // Fixed fast envelope on the detector itself (control shaping is separate)
    envAttackCoeff_  = 1.0f - std::exp (-1.0f / (0.0005f * (float) sampleRate)); // 0.5ms
    envReleaseCoeff_ = 1.0f - std::exp (-1.0f / (0.0600f * (float) sampleRate)); // 60ms

    smGain_.reset (sampleRate, 0.005);
    smGain_.setCurrentAndTargetValue (1.0f);

    setAttackMs (10.0f);
    setHoldMs (40.0f);
    setReleaseMs (300.0f);
    setSensitivity (0.5f);
    setDetectorFilters (20.0f, 20000.0f);
    reset();
}

void DuckProcessor::reset()
{
    detHpState_ = detLpState_ = 0.0f;
    env_ = 0.0f;
    control_ = 0.0f;
    holdCounter_ = 0;
    currentGrDb_ = 0.0f;
    smGain_.setCurrentAndTargetValue (1.0f);
}

void DuckProcessor::setAmount (float amount01)
{
    amount_ = juce::jlimit (0.0f, 1.0f, amount01);
}

void DuckProcessor::setAttackMs (float ms)
{
    ms = juce::jlimit (0.1f, 200.0f, ms);
    attackCoeff_ = 1.0f - std::exp (-1.0f / (ms * 0.001f * (float) sampleRate_));
}

void DuckProcessor::setHoldMs (float ms)
{
    holdSamples_ = (int) (juce::jlimit (0.0f, 500.0f, ms) * 0.001f * (float) sampleRate_);
}

void DuckProcessor::setReleaseMs (float ms)
{
    ms = juce::jlimit (20.0f, 3000.0f, ms);
    releaseCoeff_ = 1.0f - std::exp (-1.0f / (ms * 0.001f * (float) sampleRate_));
}

void DuckProcessor::setSensitivity (float sens01)
{
    // Higher sensitivity -> lower threshold -> ducks on quieter sources
    thresholdDb_ = -18.0f - 30.0f * juce::jlimit (0.0f, 1.0f, sens01);
}

void DuckProcessor::setDetectorFilters (float hpHz, float lpHz)
{
    hpHz = juce::jlimit (20.0f, 2000.0f, hpHz);
    lpHz = juce::jlimit (1000.0f, 20000.0f, lpHz);
    detHpCoeff_ = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * hpHz / (float) sampleRate_);
    detLpCoeff_ = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * lpHz / (float) sampleRate_);
}

float DuckProcessor::processSample (float dryL, float dryR)
{
    // --- Detector: filtered mono dry ---
    float mono = 0.5f * (dryL + dryR);
    detLpState_ += detLpCoeff_ * (mono - detLpState_);
    detHpState_ += detHpCoeff_ * (detLpState_ - detHpState_);
    float det = std::abs (detLpState_ - detHpState_);

    // Fast envelope
    float envCoeff = det > env_ ? envAttackCoeff_ : envReleaseCoeff_;
    env_ += envCoeff * (det - env_);

    // --- Instantaneous duck target from envelope vs threshold ---
    float envDb = juce::Decibels::gainToDecibels (env_, -80.0f);
    float over  = (envDb - thresholdDb_) / 18.0f;      // 18dB knee
    float target = juce::jlimit (0.0f, 1.0f, over);
    target = target * target * (3.0f - 2.0f * target); // smoothstep

    // --- Attack / hold / release shaping of the control signal ---
    if (target > control_)
    {
        control_ += attackCoeff_ * (target - control_);
        holdCounter_ = holdSamples_;
    }
    else if (holdCounter_ > 0)
    {
        --holdCounter_;
    }
    else
    {
        control_ += releaseCoeff_ * (target - control_);
    }

    float attenDb = kMaxAttenDb * amount_ * control_;
    currentGrDb_ = -attenDb;

    smGain_.setTargetValue (juce::Decibels::decibelsToGain (attenDb));
    return smGain_.getNextValue();
}
