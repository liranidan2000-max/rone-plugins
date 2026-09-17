#pragma once
#include <JuceHeader.h>
#include <cmath>

// ============================================================================
// RoneBarClock - where the song is in the bar, and when the next bar starts.
//
// Every RONE plugin syncs its TIMES to the host tempo; this is the part that
// knows the PHRASE. A throw that has to end on the downbeat, a freeze that
// releases on the bar, a riser that starts at the top of the phrase and lands
// on the drop: all of them ask this clock two questions - "where are we" and
// "when is the next bar" - and ride a curve between the two.
//
// Fed once per block from the audio thread with the host's play head. When the
// host gives no position (a standalone, or a host that only reports tempo) the
// clock free-runs at the last known tempo from wherever it was, so a bar is
// still a bar, only not the song's.
//
// Curves are the ones a riser is drawn with by hand: LIN (a straight line - the
// one that "sounds like seasickness"), EXP (nearly silent for the first half,
// accelerating hard at the end - the shape every transition guide asks for),
// S (exponential with an extra push in the last stretch - the "gear shift").
// ============================================================================
struct RoneBarClock
{
    double ppq = 0.0;           // quarter notes since the song start (or free-running)
    double bpm = 120.0;
    double beatsPerBar = 4.0;   // from the host's time signature, 4 if unknown
    bool   playing = false;     // host transport rolling
    bool   hostPpq = false;     // ppq came from the host this block
    bool   wasPlaying = false;

    void update (juce::AudioPlayHead* ph, double sampleRate, int numSamples) noexcept
    {
        hostPpq = false;
        playing = false;

        if (ph != nullptr)
        {
            if (auto pos = ph->getPosition())
            {
                if (auto b = pos->getBpm()) if (*b > 0.0) bpm = *b;
                if (auto ts = pos->getTimeSignature())
                    if (ts->numerator > 0 && ts->denominator > 0)
                        beatsPerBar = 4.0 * (double) ts->numerator / (double) ts->denominator;
                playing = pos->getIsPlaying();
                if (auto q = pos->getPpqPosition())
                {
                    // Rolling: follow the song. Just stopped: take the parked
                    // position once (it tells us the bar), then free-run from
                    // it so a ride or a freeze armed while stopped still lands.
                    if (playing || wasPlaying) { ppq = *q; hostPpq = true; }
                }
            }
        }
        wasPlaying = playing;

        if (! (hostPpq && playing))
            ppq += (double) numSamples / sampleRate * bpm / 60.0;
    }

    double barStart() const noexcept { return std::floor (ppq / beatsPerBar) * beatsPerBar; }
    double nextBar()  const noexcept { return barStart() + beatsPerBar; }
    double phaseInBar() const noexcept { return (ppq - barStart()) / beatsPerBar; }   // 0..1

    // 0 = LIN, 1 = EXP, 2 = S. x in 0..1 -> 0..1, monotonic, ends exactly at 1.
    static float shape (float x, int curve) noexcept
    {
        x = juce::jlimit (0.0f, 1.0f, x);
        switch (curve)
        {
            case 1:  return std::pow (x, 2.6f);                                    // quiet, then everything at the end
            case 2:  { const float e = std::pow (x, 2.2f);                         // exp with a last-stretch push
                       const float push = x > 0.75f ? std::pow ((x - 0.75f) / 0.25f, 2.0f) : 0.0f;
                       return juce::jlimit (0.0f, 1.0f, e * 0.8f + push * 0.2f); }   // both reach 1 at x = 1
            default: return x;
        }
    }
};
