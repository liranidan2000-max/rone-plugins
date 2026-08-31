#pragma once

#include <JuceHeader.h>
#include <cmath>

// =============================================================================
// SilkProcessor — built-in de-esser on the wet path
//
// The #1 problem of vocal reverb: the tail spits sibilance. The classic fix is
// a de-esser after the send — SILK builds it in (AFTERSPACE workflow motto:
// the chain is inside).
//
// Design: complementary one-pole split at ~4.6kHz; the high band is compressed
// by a fast envelope (0.5ms attack / 60ms release). Amount scales both the
// threshold and the maximum reduction (0..-15dB). Stereo-linked detector.
// =============================================================================

class SilkProcessor
{
public:
    SilkProcessor() = default;

    void prepare (double sampleRate)
    {
        sampleRate_ = sampleRate;
        xoverCoeff_ = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 4600.0f / (float) sampleRate);
        envAttackCoeff_  = 1.0f - std::exp (-1.0f / (0.0005f * (float) sampleRate)); // 0.5ms
        envReleaseCoeff_ = 1.0f - std::exp (-1.0f / (0.0600f * (float) sampleRate)); // 60ms
        smGain_.reset (sampleRate, 0.003);
        smGain_.setCurrentAndTargetValue (1.0f);
        reset();
    }

    void reset()
    {
        lp1L_ = lp1R_ = lp2L_ = lp2R_ = 0.0f;
        env_ = 0.0f;
        currentGrDb_ = 0.0f;
        smGain_.setCurrentAndTargetValue (1.0f);
    }

    void setAmount (float amount01)
    {
        amount01 = juce::jlimit (0.0f, 1.0f, amount01);
        maxGrDb_   = 15.0f * amount01;
        threshDb_  = -26.0f - 12.0f * amount01; // more amount -> catches quieter esses
    }

    void processSample (float& wetL, float& wetR)
    {
        // Complementary split, two cascaded one-poles (12dB/oct low band);
        // low + high still sum back exactly to the input
        lp1L_ += xoverCoeff_ * (wetL - lp1L_);
        lp2L_ += xoverCoeff_ * (lp1L_ - lp2L_);
        lp1R_ += xoverCoeff_ * (wetR - lp1R_);
        lp2R_ += xoverCoeff_ * (lp1R_ - lp2R_);
        float lowL  = lp2L_,        lowR  = lp2R_;
        float highL = wetL - lowL;
        float highR = wetR - lowR;

        // Stereo-linked fast envelope on the sibilance band
        float det = juce::jmax (std::abs (highL), std::abs (highR));
        env_ += (det > env_ ? envAttackCoeff_ : envReleaseCoeff_) * (det - env_);

        float envDb = juce::Decibels::gainToDecibels (env_, -80.0f);
        float grDb  = juce::jlimit (0.0f, maxGrDb_, (envDb - threshDb_) * 1.4f);
        currentGrDb_ = grDb;

        smGain_.setTargetValue (juce::Decibels::decibelsToGain (-grDb));
        float g = smGain_.getNextValue();

        wetL = lowL + highL * g;
        wetR = lowR + highR * g;
    }

    float getGainReductionDb() const { return currentGrDb_; }

private:
    double sampleRate_ = 48000.0;
    float xoverCoeff_ = 0.5f;
    float lp1L_ = 0.0f, lp1R_ = 0.0f, lp2L_ = 0.0f, lp2R_ = 0.0f;

    float env_ = 0.0f;
    float envAttackCoeff_ = 0.0f, envReleaseCoeff_ = 0.0f;

    float maxGrDb_ = 5.25f;
    float threshDb_ = -30.2f;

    juce::SmoothedValue<float> smGain_;
    float currentGrDb_ = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SilkProcessor)
};
