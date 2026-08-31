#include "RoneStuckerEngine.h"

void RoneStuckerEngine::prepare (double sampleRate, int /*maxBlockSize*/, int channels)
{
    sampleRateHz = sampleRate;
    numChannels  = juce::jmax (1, channels);

    // 10 seconds of capture — enough for a whole bar at slow tempos,
    // with plenty of headroom before the write head can reach a frozen slice.
    bufferLength = (int) std::ceil (sampleRate * 10.0);
    captureBuffer.setSize (numChannels, bufferLength);

    amountSmoothed.reset (sampleRate, 0.02);
    wetGainSmoothed.reset (sampleRate, 0.012);

    // ~6ms one-pole for loop-length glides (keeps the shrink zipper-free)
    loopLenCoeff = 1.0 - std::exp (-1.0 / (0.006 * sampleRate));

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) 512, (juce::uint32) numChannels };
    lowCutFilter.prepare (spec);
    lowCutFilter.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    lowCutFilter.setResonance (0.707f);

    reset();
}

void RoneStuckerEngine::reset()
{
    captureBuffer.clear();
    writePos = 0;
    engaged  = false;
    phase    = 0.0;
    loopLenSmoothed = 0.0;
    amountSmoothed.setCurrentAndTargetValue (0.0f);
    wetGainSmoothed.setCurrentAndTargetValue (0.0f);
    uiDepth.store (0.0f);
    uiEngaged.store (false);
}

void RoneStuckerEngine::setParameters (float amount, float smooth, float rangeOctaves,
                                       float lowCutHz, float baseLoopLengthSamples,
                                       float syncOffsetSamples) noexcept
{
    amountTarget.store (amount);
    smoothTarget.store (smooth);
    octavesTarget.store (rangeOctaves);
    lowCutTarget.store (lowCutHz);
    baseLenTarget.store (baseLoopLengthSamples);
    syncOffsetTarget.store (syncOffsetSamples);
}

float RoneStuckerEngine::readInterpolated (int channel, double position) const noexcept
{
    // Wrap into [0, bufferLength)
    double wrapped = std::fmod (position, (double) bufferLength);
    if (wrapped < 0.0)
        wrapped += (double) bufferLength;

    const int i0 = (int) wrapped;
    const int i1 = (i0 + 1 < bufferLength) ? i0 + 1 : 0;
    const float frac = (float) (wrapped - (double) i0);

    const float* data = captureBuffer.getReadPointer (channel);
    return data[i0] + frac * (data[i1] - data[i0]);
}

void RoneStuckerEngine::process (juce::AudioBuffer<float>& buffer) noexcept
{
    const int numSamples = buffer.getNumSamples();
    const int chans      = juce::jmin (buffer.getNumChannels(), numChannels);

    const float amount   = amountTarget.load();
    const float smooth   = smoothTarget.load();
    const float octaves  = octavesTarget.load();
    const float lowCutHz = lowCutTarget.load();
    const float baseLen  = baseLenTarget.load();

    amountSmoothed.setTargetValue (amount);
    lowCutFilter.setCutoffFrequency (juce::jlimit (20.0f, 500.0f, lowCutHz));
    const bool lowCutActive = lowCutHz > 25.0f; // fully open = transparent, skip the filter

    // --- Engage / release (block-rate decision, click-free via wetGain ramp) ---
    if (! engaged && amount > engageEpsilon)
    {
        engaged    = true;
        captureLen = juce::jlimit (minLoopSamples, (double) bufferLength / 2.0, (double) baseLen);

        const float syncOffset = syncOffsetTarget.load();
        if (syncOffset >= 0.0f)
        {
            // SYNC: loop the last COMPLETE division block, ending at the most
            // recent grid line — its start sits exactly on the grid (the kick).
            // Starting playback at phase = syncOffset keeps the loop phase-locked
            // to the host timeline, so the repeat lands on the beat.
            const double offset = juce::jlimit (0.0, captureLen - 1.0, (double) syncOffset);
            captureStart = writePos - (int) offset - (int) captureLen;
            phase        = offset;
        }
        else
        {
            // FREE: grab the last captureLen samples ending right now.
            captureStart = writePos - (int) captureLen;
            phase        = 0.0;
        }

        while (captureStart < 0)
            captureStart += bufferLength;

        loopLenSmoothed = captureLen;
        wetGainSmoothed.setTargetValue (1.0f);
        lowCutFilter.reset();
    }
    else if (engaged && amount <= engageEpsilon)
    {
        engaged = false;
        wetGainSmoothed.setTargetValue (0.0f);
    }

    uiEngaged.store (engaged);

    for (int i = 0; i < numSamples; ++i)
    {
        const float amt = amountSmoothed.getNextValue();
        const float wg  = wetGainSmoothed.getNextValue();

        // --- Record the live input while not frozen ---------------------------
        // (While engaged the buffer is frozen so the slice can't be overwritten;
        //  during the short release fade the write head trails far behind the
        //  frozen slice, so resuming immediately is safe.)
        if (! engaged)
        {
            for (int ch = 0; ch < chans; ++ch)
                captureBuffer.setSample (ch, writePos, buffer.getSample (ch, i));
            writePos = (writePos + 1 < bufferLength) ? writePos + 1 : 0;
        }

        if (wg <= 0.0001f)
        {
            uiDepth.store (0.0f);
            continue; // fully dry — buffer already holds the input
        }

        // --- Loop length: exponential shrink, one-pole smoothed ---------------
        const double targetLen = juce::jlimit (minLoopSamples, captureLen,
                                               captureLen * std::exp2 (-(double) amt * (double) octaves));
        loopLenSmoothed += (targetLen - loopLenSmoothed) * loopLenCoeff;

        // --- Advance phase & wrap against the (moving) loop length ------------
        if (phase >= loopLenSmoothed)
            phase = std::fmod (phase, loopLenSmoothed);

        // --- Crossfaded loop read ---------------------------------------------
        // In the last F samples of the loop, equal-power blend the current read
        // position with (position - loopLen), which lands just before the loop
        // start — so the wrap back to phase 0 is continuous.
        const double fadeLen = juce::jmax (4.0, (double) smooth * 0.5 * loopLenSmoothed);
        const double fadeStart = loopLenSmoothed - fadeLen;

        for (int ch = 0; ch < chans; ++ch)
        {
            const double readPos = (double) captureStart + phase;
            float wet;

            if (phase > fadeStart)
            {
                const float t  = (float) ((phase - fadeStart) / fadeLen);
                const float g1 = std::cos (t * juce::MathConstants<float>::halfPi);
                const float g2 = std::sin (t * juce::MathConstants<float>::halfPi);
                wet = g1 * readInterpolated (ch, readPos)
                    + g2 * readInterpolated (ch, readPos - loopLenSmoothed);
            }
            else
            {
                wet = readInterpolated (ch, readPos);
            }

            if (lowCutActive)
                wet = lowCutFilter.processSample (ch, wet);

            const float dry = buffer.getSample (ch, i);
            buffer.setSample (ch, i, dry * (1.0f - wg) + wet * wg);
        }

        phase += 1.0;

        // UI: 0..1 how deep into the shrink we are (log-scaled feels right)
        if (captureLen > minLoopSamples)
            uiDepth.store ((float) (std::log2 (captureLen / loopLenSmoothed)
                                    / std::log2 (captureLen / minLoopSamples)));
        uiLoopMs.store ((float) (loopLenSmoothed / sampleRateHz * 1000.0));
    }
}
