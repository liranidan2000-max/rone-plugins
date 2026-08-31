#include <JuceHeader.h>
#include <cstdio>
#include <cmath>

#include "../dsp/ReverbEngine.h"
#include "../dsp/EchoEngine.h"
#include "../dsp/DuckProcessor.h"
#include "../dsp/SilkProcessor.h"

// =============================================================================
// AFTERSPACE DSP smoke tests (spec §26 /tests, §31 acceptance criteria)
// Console app; returns non-zero exit code on failure.
// =============================================================================

static int failures = 0;

static void check (bool condition, const char* name)
{
    std::printf ("[%s] %s\n", condition ? "PASS" : "FAIL", name);
    if (! condition) ++failures;
}

// -----------------------------------------------------------------------------
static void testReverbImpulse (double sr)
{
    ReverbEngine rv;
    rv.prepare (sr, 512);
    rv.setDecaySeconds (0.8f);
    rv.setSize (0.6f);
    rv.setPreDelayMs (0.0f);

    const int total = (int) (sr * 4.0);
    float outL = 0, outR = 0;
    double earlyEnergy = 0, lateEnergy = 0;
    bool finite = true;

    for (int n = 0; n < total; ++n)
    {
        float in = (n == 0) ? 1.0f : 0.0f;
        rv.processSample (in, in, outL, outR);
        if (! std::isfinite (outL) || ! std::isfinite (outR)) finite = false;

        double e = (double) (outL * outL + outR * outR);
        if (n < (int) sr)            earlyEnergy += e;   // first second
        else if (n > (int) (sr * 3)) lateEnergy  += e;   // 3s..4s
    }

    char name[128];
    std::snprintf (name, sizeof (name), "Reverb impulse finite @ %.0f Hz", sr);
    check (finite, name);
    std::snprintf (name, sizeof (name), "Reverb produces a tail @ %.0f Hz", sr);
    check (earlyEnergy > 1.0e-6, name);
    std::snprintf (name, sizeof (name), "Reverb tail decays (0.8s T60) @ %.0f Hz", sr);
    check (lateEnergy < earlyEnergy * 0.05, name);
}

// -----------------------------------------------------------------------------
static void testReverbFreeze()
{
    const double sr = 48000.0;
    ReverbEngine rv;
    rv.prepare (sr, 512);
    rv.setDecaySeconds (2.0f);

    float outL = 0, outR = 0;

    // Excite with 100ms of noise
    juce::Random rng (42);
    for (int n = 0; n < (int) (sr * 0.1); ++n)
    {
        float in = rng.nextFloat() * 2.0f - 1.0f;
        rv.processSample (in, in, outL, outR);
    }

    rv.setFreeze (true);

    // Run frozen for 3 minutes of audio; tail must neither die nor blow up
    double energyWindow = 0;
    bool finite = true;
    float peak = 0.0f;
    const int total = (int) (sr * 180.0);
    for (int n = 0; n < total; ++n)
    {
        rv.processSample (0.0f, 0.0f, outL, outR);
        if (! std::isfinite (outL) || ! std::isfinite (outR)) finite = false;
        peak = juce::jmax (peak, std::abs (outL), std::abs (outR));
        if (n > total - (int) sr)
            energyWindow += (double) (outL * outL + outR * outR);
    }

    check (finite, "Freeze stays finite for 3 minutes");
    check (energyWindow > 1.0e-8, "Freeze sustains the tail (no silence after 3 min)");
    check (peak < 4.0f, "Freeze does not blow up (peak < +12dB)");
}

// -----------------------------------------------------------------------------
static void testEchoFeedbackStability()
{
    const double sr = 48000.0;
    EchoEngine echo;
    echo.prepare (sr, 512);
    echo.setTimeMs (80.0f);
    echo.setFeedback (0.95f);   // maximum allowed
    echo.setLowCutHz (20.0f);
    echo.setHighCutHz (20000.0f);

    float outL = 0, outR = 0, peak = 0;
    bool finite = true;

    for (int n = 0; n < (int) (sr * 10.0); ++n)
    {
        float in = (n < (int) sr) ? 0.9f * std::sin (0.05f * (float) n) : 0.0f;
        echo.processSample (in, in, outL, outR);
        if (! std::isfinite (outL) || ! std::isfinite (outR)) finite = false;
        peak = juce::jmax (peak, std::abs (outL), std::abs (outR));
    }

    check (finite, "Echo max-feedback output finite");
    check (peak < 2.0f, "Echo feedback soft-clip keeps level bounded");
}

// -----------------------------------------------------------------------------
static void testDuck()
{
    const double sr = 48000.0;
    DuckProcessor duck;
    duck.prepare (sr);
    duck.setAmount (1.0f);
    duck.setAttackMs (5.0f);
    duck.setHoldMs (20.0f);
    duck.setReleaseMs (100.0f);
    duck.setSensitivity (0.5f);

    // Loud 1kHz input for 0.5s -> gain must drop well below 1
    // (the detector high-passes, so a DC test signal would be filtered out)
    float gain = 1.0f;
    const float w = juce::MathConstants<float>::twoPi * 1000.0f / (float) sr;
    for (int n = 0; n < (int) (sr * 0.5); ++n)
    {
        float s = 0.8f * std::sin (w * (float) n);
        gain = duck.processSample (s, s);
    }
    check (gain < 0.5f, "Duck attenuates during loud input");

    // Silence for 2s -> gain must recover
    for (int n = 0; n < (int) (sr * 2.0); ++n)
        gain = duck.processSample (0.0f, 0.0f);
    check (gain > 0.95f, "Duck releases back to unity in silence");

    check (std::isfinite (gain), "Duck gain finite");
}

// -----------------------------------------------------------------------------
static void testSilk()
{
    const double sr = 48000.0;
    const float w8k = juce::MathConstants<float>::twoPi * 8000.0f / (float) sr;
    const float w500 = juce::MathConstants<float>::twoPi * 500.0f / (float) sr;

    auto rmsOut = [&] (float amount, float toneW)
    {
        SilkProcessor s;
        s.prepare (sr);
        s.setAmount (amount);
        double acc = 0; int count = 0;
        for (int n = 0; n < (int) sr; ++n)
        {
            float l = 0.5f * std::sin (toneW * (float) n);
            float r = l;
            s.processSample (l, r);
            if (n > (int) sr / 2) { acc += (double) l * l; ++count; }
        }
        return std::sqrt (acc / count);
    };

    double sib0 = rmsOut (0.0f, w8k);
    double sib1 = rmsOut (1.0f, w8k);
    double low1 = rmsOut (1.0f, w500);

    check (sib1 < sib0 * 0.5, "Silk attenuates sibilance band (>6dB at full amount)");
    check (sib0 > 0.30, "Silk at 0% leaves the signal intact");
    check (low1 > 0.30, "Silk leaves the body of the sound alone (500Hz untouched)");
}

// -----------------------------------------------------------------------------
static void testReverbBlockSizes()
{
    // Engines are per-sample, but verify prepare() at odd sizes doesn't break
    for (int block : { 16, 33, 256, 4096 })
    {
        ReverbEngine rv;
        rv.prepare (44100.0, block);
        float l = 0, r = 0;
        for (int n = 0; n < 1000; ++n)
            rv.processSample (n == 0 ? 1.0f : 0.0f, 0.0f, l, r);
        char name[64];
        std::snprintf (name, sizeof (name), "Reverb ok with maxBlockSize %d", block);
        check (std::isfinite (l) && std::isfinite (r), name);
    }
}

// -----------------------------------------------------------------------------
int main()
{
    std::printf ("=== RONE AFTERSPACE DSP smoke tests ===\n");

    for (double sr : { 44100.0, 48000.0, 96000.0 })
        testReverbImpulse (sr);

    testReverbFreeze();
    testEchoFeedbackStability();
    testDuck();
    testSilk();
    testReverbBlockSizes();

    std::printf ("=== %s (%d failure%s) ===\n",
                 failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
