#include "EchoEngine.h"

void EchoEngine::prepare (double sampleRate, int /*maxBlockSize*/)
{
    sampleRate_ = sampleRate;

    // Max 4.2s of delay (2 bars at slow tempos / 2000ms free + headroom)
    bufSize_ = juce::nextPowerOfTwo ((int) (sampleRate * 4.2) + 8);
    bufL_.assign ((size_t) bufSize_, 0.0f);
    bufR_.assign ((size_t) bufSize_, 0.0f);
    writePos_ = 0;

    smDelaySamples_.reset (sampleRate, 0.15);  // tape-style glide on time changes
    smDelaySamples_.setCurrentAndTargetValue (0.350f * (float) sampleRate);
    smFeedback_.reset (sampleRate, 0.02);
    smFeedback_.setCurrentAndTargetValue (0.25f);
    smPingPong_.reset (sampleRate, 0.05);
    smPingPong_.setCurrentAndTargetValue (0.0f);

    setLowCutHz (150.0f);
    setHighCutHz (10000.0f);
    reset();
}

void EchoEngine::reset()
{
    std::fill (bufL_.begin(), bufL_.end(), 0.0f);
    std::fill (bufR_.begin(), bufR_.end(), 0.0f);
    lpStateL_ = lpStateR_ = hpStateL_ = hpStateR_ = 0.0f;
}

void EchoEngine::setTimeMs (float ms)
{
    float samples = juce::jlimit (1.0f, (float) bufSize_ - 4.0f,
                                  ms * (float) sampleRate_ / 1000.0f);
    smDelaySamples_.setTargetValue (samples);
}

void EchoEngine::setFeedback (float fb01)
{
    smFeedback_.setTargetValue (juce::jlimit (0.0f, 0.95f, fb01));
}

void EchoEngine::setPingPong (bool enabled)
{
    smPingPong_.setTargetValue (enabled ? 1.0f : 0.0f);
}

void EchoEngine::setLowCutHz (float hz)
{
    hz = juce::jlimit (20.0f, 2000.0f, hz);
    hpCoeff_ = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * hz / (float) sampleRate_);
}

void EchoEngine::setHighCutHz (float hz)
{
    hz = juce::jlimit (1000.0f, 20000.0f, hz);
    lpCoeff_ = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * hz / (float) sampleRate_);
}

float EchoEngine::readInterp (const std::vector<float>& buf, int bufSize,
                              int writePos, float delaySamples)
{
    float readPos = (float) writePos - delaySamples;
    int mask = bufSize - 1;
    int i0 = (int) std::floor (readPos);
    float frac = readPos - (float) i0;
    float s0 = buf[(size_t) (i0 & mask)];
    float s1 = buf[(size_t) ((i0 + 1) & mask)];
    return s0 + frac * (s1 - s0);
}

void EchoEngine::processSample (float inL, float inR, float& outL, float& outR)
{
    float delay = smDelaySamples_.getNextValue();
    float fb    = smFeedback_.getNextValue();
    float pp    = smPingPong_.getNextValue();

    float tapL = readInterp (bufL_, bufSize_, writePos_, delay);
    float tapR = readInterp (bufR_, bufSize_, writePos_, delay);

    // Feedback-path tone shaping
    lpStateL_ += lpCoeff_ * (tapL - lpStateL_);
    lpStateR_ += lpCoeff_ * (tapR - lpStateR_);
    hpStateL_ += hpCoeff_ * (lpStateL_ - hpStateL_);
    hpStateR_ += hpCoeff_ * (lpStateR_ - hpStateR_);
    float fbL = lpStateL_ - hpStateL_;
    float fbR = lpStateR_ - hpStateR_;

    // Stereo feedback vs ping-pong cross-feedback (crossfaded, click-free)
    float wetFbL = fbL * (1.0f - pp) + fbR * pp;
    float wetFbR = fbR * (1.0f - pp) + fbL * pp;

    // Ping-pong feeds input to the left line only (classic bounce)
    float monoIn = 0.5f * (inL + inR);
    float writeInL = inL * (1.0f - pp) + monoIn * pp;
    float writeInR = inR * (1.0f - pp);

    // Soft clip inside the loop — feedback can never run away
    float wL = std::tanh (writeInL + fb * wetFbL);
    float wR = std::tanh (writeInR + fb * wetFbR);

    bufL_[(size_t) writePos_] = wL;
    bufR_[(size_t) writePos_] = wR;
    writePos_ = (writePos_ + 1) & (bufSize_ - 1);

    outL = tapL;
    outR = tapR;
}
