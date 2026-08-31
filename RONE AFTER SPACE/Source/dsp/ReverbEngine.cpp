#include "ReverbEngine.h"

namespace
{
    // Base FDN delay lengths in samples @48kHz (mutually prime, ~35–78ms)
    constexpr int kBaseDelays48k[ReverbEngine::kNumLines] = {
        1687, 1931, 2213, 2503, 2801, 3109, 3413, 3719
    };

    // Input diffuser delays in samples @48kHz (~5.3 / 7.9 / 11.2 / 14.8 ms),
    // right channel slightly detuned for decorrelation
    constexpr int kDiffDelaysL48k[4] = { 254, 379, 538, 710 };
    constexpr int kDiffDelaysR48k[4] = { 269, 397, 557, 733 };

    // Density allpass delays per FDN line @48kHz (small primes)
    constexpr int kDensDelays48k[ReverbEngine::kNumLines] = {
        113, 149, 181, 211, 239, 271, 307, 337
    };

    // Early reflection tap offsets in ms (asymmetric L/R)
    constexpr float kErTapsMsL[8] = { 7.1f, 11.3f, 17.9f, 23.6f, 31.7f, 41.3f, 53.9f, 67.7f };
    constexpr float kErTapsMsR[8] = { 8.3f, 13.1f, 19.7f, 26.3f, 34.9f, 43.7f, 56.3f, 71.9f };

    // Output tap sign patterns (orthogonal rows -> decorrelated L/R)
    constexpr float kOutSignL[8] = { +1, +1, -1, -1, +1, +1, -1, -1 };
    constexpr float kOutSignR[8] = { +1, -1, -1, +1, +1, -1, -1, +1 };

    constexpr float kInputGain  = 0.353553391f; // 1/sqrt(8)
    constexpr float kOutputGain = 0.42f;

    // Cheap sine approximation for LFOs (phase in 0..1)
    inline float fastSin01 (float phase) noexcept
    {
        float x = phase - 0.5f;               // -0.5 .. 0.5
        float y = x * (8.0f - 16.0f * std::abs (x)); // parabola approx of sin(2*pi*x)
        return y * (0.775f + 0.225f * std::abs (y)); // refine
    }

    // Max size scale used for buffer allocation headroom
    constexpr float kMaxSizeScale = 1.15f;
    constexpr float kModMarginSamples48k = 24.0f;
}

// =============================================================================
void ReverbEngine::prepare (double sampleRate, int /*maxBlockSize*/)
{
    sampleRate_ = sampleRate;
    srRatio_    = (float) (sampleRate / 48000.0);

    // --- Pre-delay buffer: 250ms pre-delay + 80ms ER taps + margin ---
    preBufSize_ = juce::nextPowerOfTwo ((int) (sampleRate * 0.350) + 8);
    preBufL_.assign ((size_t) preBufSize_, 0.0f);
    preBufR_.assign ((size_t) preBufSize_, 0.0f);
    preWritePos_ = 0;

    smPreDelaySamples_.reset (sampleRate, 0.05);
    smPreDelaySamples_.setCurrentAndTargetValue (0.020f * (float) sampleRate);

    // ER gains: decaying, normalized-ish
    for (int t = 0; t < kNumErTaps; ++t)
        erGains_[(size_t) t] = 0.32f * std::pow (0.80f, (float) t);

    // --- Input diffusers ---
    for (int i = 0; i < kNumDiff; ++i)
    {
        diffLen_[(size_t) i] = juce::jmax (4, (int) ((float) kDiffDelaysL48k[i] * srRatio_));
        int lenR             = juce::jmax (4, (int) ((float) kDiffDelaysR48k[i] * srRatio_));
        diffBufL_[(size_t) i].assign ((size_t) diffLen_[(size_t) i], 0.0f);
        diffBufR_[(size_t) i].assign ((size_t) lenR, 0.0f);
        diffPosL_[(size_t) i] = 0;
        diffPosR_[(size_t) i] = 0;
    }
    smDiffCoeff_.reset (sampleRate, 0.05);
    smDiffCoeff_.setCurrentAndTargetValue (0.25f + 0.5f * 0.70f);

    // --- FDN lines ---
    for (int i = 0; i < kNumLines; ++i)
    {
        float maxLen = (float) kBaseDelays48k[i] * srRatio_ * kMaxSizeScale
                       + kModMarginSamples48k * srRatio_ + 8.0f;
        fdnBufSize_[(size_t) i] = juce::nextPowerOfTwo ((int) maxLen);
        fdnBuf_[(size_t) i].assign ((size_t) fdnBufSize_[(size_t) i], 0.0f);
        fdnWritePos_[(size_t) i] = 0;

        smLineLen_[(size_t) i].reset (sampleRate, 0.15); // slow, click-free size sweeps
        smFeedback_[(size_t) i].reset (sampleRate, 0.05);

        densLen_[(size_t) i] = juce::jmax (4, (int) ((float) kDensDelays48k[i] * srRatio_));
        densBuf_[(size_t) i].assign ((size_t) densLen_[(size_t) i], 0.0f);
        densPos_[(size_t) i] = 0;

        // LFO phases spread by golden-ratio steps
        lfoPhase_[(size_t) i] = std::fmod (0.618034f * (float) i, 1.0f);
    }

    smHiDampCoeff_.reset (sampleRate, 0.05);
    smLoDampCoeff_.reset (sampleRate, 0.05);
    smDensCoeff_.reset (sampleRate, 0.05);
    smModDepthSamples_.reset (sampleRate, 0.10);
    smFreeze_.reset (sampleRate, 0.10);     // 100ms freeze crossfade
    smErGain_.reset (sampleRate, 0.05);
    smLateGain_.reset (sampleRate, 0.05);

    setSize (size_);
    setDecaySeconds (decaySeconds_);
    setModRate (modRateHz_);
    setModDepth (0.15f);
    setHighDamp (0.4f);
    setLowDamp (0.2f);
    setDensity (0.7f);
    setEarlyLateBalance (0.3f);

    // Snap smoothers to their targets so prepare() doesn't ramp from garbage
    for (int i = 0; i < kNumLines; ++i)
    {
        smLineLen_[(size_t) i].setCurrentAndTargetValue (smLineLen_[(size_t) i].getTargetValue());
        smFeedback_[(size_t) i].setCurrentAndTargetValue (smFeedback_[(size_t) i].getTargetValue());
    }
    smFreeze_.setCurrentAndTargetValue (0.0f);

    reset();
}

void ReverbEngine::reset()
{
    for (auto& b : { &preBufL_, &preBufR_ })
        std::fill (b->begin(), b->end(), 0.0f);

    for (int i = 0; i < kNumDiff; ++i)
    {
        std::fill (diffBufL_[(size_t) i].begin(), diffBufL_[(size_t) i].end(), 0.0f);
        std::fill (diffBufR_[(size_t) i].begin(), diffBufR_[(size_t) i].end(), 0.0f);
    }

    for (int i = 0; i < kNumLines; ++i)
    {
        std::fill (fdnBuf_[(size_t) i].begin(), fdnBuf_[(size_t) i].end(), 0.0f);
        std::fill (densBuf_[(size_t) i].begin(), densBuf_[(size_t) i].end(), 0.0f);
        hiDampState_[(size_t) i] = 0.0f;
        loDampState_[(size_t) i] = 0.0f;
    }

    erLpStateL_ = erLpStateR_ = 0.0f;
}

// =============================================================================
// Parameter setters
// =============================================================================
void ReverbEngine::setSize (float size01)
{
    size_ = juce::jlimit (0.0f, 1.0f, size01);

    // Perceived size: scales FDN line lengths and ER tap spread
    float sizeScale = 0.30f + 0.85f * size_;

    for (int i = 0; i < kNumLines; ++i)
    {
        baseLenScaled_[(size_t) i] = (float) kBaseDelays48k[i] * srRatio_ * sizeScale;
        smLineLen_[(size_t) i].setTargetValue (baseLenScaled_[(size_t) i]);
    }

    float erScale = (0.45f + 0.75f * size_) * (float) sampleRate_ / 1000.0f;
    for (int t = 0; t < kNumErTaps; ++t)
    {
        erTapSamplesL_[(size_t) t] = kErTapsMsL[t] * erScale;
        erTapSamplesR_[(size_t) t] = kErTapsMsR[t] * erScale;
    }

    updateFeedbackGains();
}

void ReverbEngine::setDecaySeconds (float seconds)
{
    decaySeconds_ = juce::jlimit (0.2f, 30.0f, seconds);
    updateFeedbackGains();
}

void ReverbEngine::updateFeedbackGains()
{
    // Per-line T60 gain: g = 10^(-3 * L / (T60 * sr))
    float t60Samples = decaySeconds_ * (float) sampleRate_;
    for (int i = 0; i < kNumLines; ++i)
    {
        float g = std::pow (10.0f, -3.0f * baseLenScaled_[(size_t) i] / t60Samples);
        smFeedback_[(size_t) i].setTargetValue (juce::jlimit (0.0f, 0.9995f, g));
    }
}

void ReverbEngine::setPreDelayMs (float ms)
{
    float samples = juce::jlimit (0.0f, 250.0f, ms) * (float) sampleRate_ / 1000.0f;
    smPreDelaySamples_.setTargetValue (samples);
}

void ReverbEngine::setDiffusion (float amount01)
{
    smDiffCoeff_.setTargetValue (0.25f + 0.5f * juce::jlimit (0.0f, 1.0f, amount01));
}

void ReverbEngine::setDensity (float amount01)
{
    smDensCoeff_.setTargetValue (0.20f + 0.45f * juce::jlimit (0.0f, 1.0f, amount01));
}

void ReverbEngine::setModRate (float hz)
{
    modRateHz_ = juce::jlimit (0.01f, 5.0f, hz);
    for (int i = 0; i < kNumLines; ++i)
    {
        // Slightly detuned rates per line avoid coherent pitch wobble
        float rate = modRateHz_ * (0.82f + 0.05f * (float) i);
        lfoInc_[(size_t) i] = rate / (float) sampleRate_;
    }
}

void ReverbEngine::setModDepth (float amount01)
{
    // Max ~9 samples @48k: audible resonance smoothing without obvious chorus
    smModDepthSamples_.setTargetValue (juce::jlimit (0.0f, 1.5f, amount01) * 9.0f * srRatio_);
}

void ReverbEngine::setHighDamp (float amount01)
{
    // 0 -> ~20kHz (transparent), 1 -> ~1.2kHz
    float amt = juce::jlimit (0.0f, 1.0f, amount01);
    float cutoff = 20000.0f * std::pow (1200.0f / 20000.0f, amt);
    float c = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * cutoff / (float) sampleRate_);
    smHiDampCoeff_.setTargetValue (juce::jlimit (0.0f, 1.0f, c));
}

void ReverbEngine::setLowDamp (float amount01)
{
    // One-pole low tracker subtracted from feedback: 0 -> ~15Hz, 1 -> ~350Hz
    float amt = juce::jlimit (0.0f, 1.0f, amount01);
    float cutoff = 15.0f * std::pow (350.0f / 15.0f, amt);
    float c = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * cutoff / (float) sampleRate_);
    smLoDampCoeff_.setTargetValue (juce::jlimit (0.0f, 1.0f, c));
}

void ReverbEngine::setEarlyLateBalance (float balance)
{
    // -1 all early .. +1 all late, equal-power
    float x = (juce::jlimit (-1.0f, 1.0f, balance) + 1.0f) * 0.5f; // 0..1
    smErGain_.setTargetValue (std::cos (x * juce::MathConstants<float>::halfPi));
    smLateGain_.setTargetValue (std::sin (x * juce::MathConstants<float>::halfPi));
}

void ReverbEngine::setFreeze (bool shouldFreeze)
{
    freezeTarget_ = shouldFreeze;
    smFreeze_.setTargetValue (shouldFreeze ? 1.0f : 0.0f);
}

// =============================================================================
// Processing
// =============================================================================
float ReverbEngine::readInterp (const std::vector<float>& buf, int bufSize,
                                int writePos, float delaySamples)
{
    float readPos = (float) writePos - delaySamples;
    int mask = bufSize - 1; // bufSize is a power of two
    int i0 = (int) std::floor (readPos);
    float frac = readPos - (float) i0;
    float s0 = buf[(size_t) (i0 & mask)];
    float s1 = buf[(size_t) ((i0 + 1) & mask)];
    return s0 + frac * (s1 - s0);
}

void ReverbEngine::processSample (float inL, float inR, float& outL, float& outR)
{
    int preMask = preBufSize_ - 1;
    float freeze = smFreeze_.getNextValue();

    // --- Write dry into pre-delay buffer (input frozen out during freeze) ---
    float inGain = 1.0f - freeze;
    preBufL_[(size_t) preWritePos_] = inL * inGain;
    preBufR_[(size_t) preWritePos_] = inR * inGain;

    float preDelay = smPreDelaySamples_.getNextValue();

    // Pre-delayed signal feeding diffusion + FDN
    float pdL = readInterp (preBufL_, preBufSize_, preWritePos_, preDelay + 1.0f);
    float pdR = readInterp (preBufR_, preBufSize_, preWritePos_, preDelay + 1.0f);

    // --- Early reflections: taps beyond pre-delay ---
    float erL = 0.0f, erR = 0.0f;
    for (int t = 0; t < kNumErTaps; ++t)
    {
        erL += erGains_[(size_t) t] * readInterp (preBufL_, preBufSize_, preWritePos_,
                                                  preDelay + erTapSamplesL_[(size_t) t]);
        erR += erGains_[(size_t) t] * readInterp (preBufR_, preBufSize_, preWritePos_,
                                                  preDelay + erTapSamplesR_[(size_t) t]);
    }
    // Gentle LP on ER (~9kHz) so early taps don't sound brittle
    float erLpC = 0.55f;
    erLpStateL_ += erLpC * (erL - erLpStateL_);
    erLpStateR_ += erLpC * (erR - erLpStateR_);
    erL = erLpStateL_;
    erR = erLpStateR_;

    preWritePos_ = (preWritePos_ + 1) & preMask;

    // --- Input diffusion (series allpasses) ---
    float diffCoeff = smDiffCoeff_.getNextValue();
    float dL = pdL, dR = pdR;
    for (int i = 0; i < kNumDiff; ++i)
    {
        auto& bL = diffBufL_[(size_t) i];
        int   pL = diffPosL_[(size_t) i];
        float delayed = bL[(size_t) pL];
        float w = dL - diffCoeff * delayed;
        bL[(size_t) pL] = w;
        dL = delayed + diffCoeff * w;
        diffPosL_[(size_t) i] = (pL + 1 < (int) bL.size()) ? pL + 1 : 0;

        auto& bR = diffBufR_[(size_t) i];
        int   pR = diffPosR_[(size_t) i];
        float delayedR = bR[(size_t) pR];
        float wR = dR - diffCoeff * delayedR;
        bR[(size_t) pR] = wR;
        dR = delayedR + diffCoeff * wR;
        diffPosR_[(size_t) i] = (pR + 1 < (int) bR.size()) ? pR + 1 : 0;
    }

    // --- FDN read + damping ---
    float modDepth  = smModDepthSamples_.getNextValue();
    float hiDampC   = smHiDampCoeff_.getNextValue();
    float loDampC   = smLoDampCoeff_.getNextValue();
    float densCoeff = smDensCoeff_.getNextValue();

    // During freeze: open damping so the held tail doesn't decay spectrally
    float hiC = hiDampC + freeze * (1.0f - hiDampC) * 0.97f;
    float loC = loDampC * (1.0f - freeze);

    std::array<float, kNumLines> lineOut;
    for (int i = 0; i < kNumLines; ++i)
    {
        // LFO-modulated read position
        float lfo = fastSin01 (lfoPhase_[(size_t) i]);
        lfoPhase_[(size_t) i] += lfoInc_[(size_t) i];
        if (lfoPhase_[(size_t) i] >= 1.0f) lfoPhase_[(size_t) i] -= 1.0f;

        float len = smLineLen_[(size_t) i].getNextValue() + lfo * modDepth;
        float x = readInterp (fdnBuf_[(size_t) i], fdnBufSize_[(size_t) i],
                              fdnWritePos_[(size_t) i], len);

        // High damp: one-pole LP in the loop
        auto& hs = hiDampState_[(size_t) i];
        hs += hiC * (x - hs);
        x = hs;

        // Low damp: subtract tracked lows (faster low-frequency decay)
        auto& ls = loDampState_[(size_t) i];
        ls += loC * (x - ls);
        x -= ls * 0.85f * (loC > 0.0f ? 1.0f : 0.0f);

        lineOut[(size_t) i] = x;
    }

    // --- Householder feedback matrix: y_i = x_i - (2/N) * sum ---
    float sum = 0.0f;
    for (int i = 0; i < kNumLines; ++i) sum += lineOut[(size_t) i];
    float k = sum * (2.0f / (float) kNumLines);

    // --- Write back: input injection + feedback, through density allpass ---
    for (int i = 0; i < kNumLines; ++i)
    {
        float fb = smFeedback_[(size_t) i].getNextValue();
        float fbEff = fb + freeze * (0.9995f - fb); // freeze -> unity-ish loop

        float inj = ((i & 1) == 0 ? dL : dR) * kInputGain * inGain;
        // Alternate injection polarity for extra decorrelation
        if ((i & 3) >= 2) inj = -inj;

        float v = inj + fbEff * (lineOut[(size_t) i] - k);

        // Density allpass in the loop
        auto& db = densBuf_[(size_t) i];
        int   dp = densPos_[(size_t) i];
        float delayed = db[(size_t) dp];
        float w = v - densCoeff * delayed;
        db[(size_t) dp] = w;
        v = delayed + densCoeff * w;
        densPos_[(size_t) i] = (dp + 1 < (int) db.size()) ? dp + 1 : 0;

        auto& buf = fdnBuf_[(size_t) i];
        buf[(size_t) fdnWritePos_[(size_t) i]] = v;
        fdnWritePos_[(size_t) i] = (fdnWritePos_[(size_t) i] + 1) & (fdnBufSize_[(size_t) i] - 1);
    }

    // --- Output taps (orthogonal sign patterns for stereo decorrelation) ---
    float lateL = 0.0f, lateR = 0.0f;
    for (int i = 0; i < kNumLines; ++i)
    {
        lateL += kOutSignL[i] * lineOut[(size_t) i];
        lateR += kOutSignR[i] * lineOut[(size_t) i];
    }
    lateL *= kOutputGain * kInputGain * 2.0f;
    lateR *= kOutputGain * kInputGain * 2.0f;

    float erGain   = smErGain_.getNextValue() * (1.0f - freeze); // ER fades in freeze
    float lateGain = smLateGain_.getNextValue();

    outL = erL * erGain + lateL * lateGain;
    outR = erR * erGain + lateR * lateGain;
}
