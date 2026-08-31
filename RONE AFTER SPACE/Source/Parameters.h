#pragma once

#include <JuceHeader.h>

// =============================================================================
// RONE AFTERSPACE — Parameter model
//
// Parameter IDs follow the spec pattern "group.name" and are a STABLE public
// API from the first release. Never rename an ID; add new ones instead.
// State schema versioning lives in PluginProcessor (kStateSchemaVersion).
// =============================================================================

namespace ParamIDs
{
    // MAIN
    inline constexpr const char* mix        = "main.mix";
    inline constexpr const char* size       = "main.size";
    inline constexpr const char* decay      = "main.decay";
    inline constexpr const char* bloom      = "main.bloom";
    inline constexpr const char* character  = "main.character";
    inline constexpr const char* bypass     = "main.bypass";
    inline constexpr const char* silk       = "silk.amount";     // built-in tail de-esser
    inline constexpr const char* autoGain   = "main.autogain";   // wet loudness compensation
    inline constexpr const char* soloWet    = "main.solowet";    // audition the space alone

    // FREEZE
    inline constexpr const char* freeze     = "freeze.active";

    // DUCK
    inline constexpr const char* duckAmount      = "duck.amount";
    inline constexpr const char* duckMode        = "duck.mode";
    inline constexpr const char* duckAttack      = "duck.attack";
    inline constexpr const char* duckHold        = "duck.hold";
    inline constexpr const char* duckRelease     = "duck.release";
    inline constexpr const char* duckSensitivity = "duck.sensitivity";

    // ECHO
    inline constexpr const char* echoAmount   = "echo.amount";
    inline constexpr const char* echoTime     = "echo.time";
    inline constexpr const char* echoFreeMs   = "echo.freeMs";
    inline constexpr const char* echoFeedback = "echo.feedback";
    inline constexpr const char* echoRouting  = "echo.routing";
    inline constexpr const char* echoPingPong = "echo.pingpong";
    inline constexpr const char* echoLowCut   = "echo.lowcut";
    inline constexpr const char* echoHighCut  = "echo.highcut";

    // SPACE (advanced)
    inline constexpr const char* preDelay   = "space.predelay";
    inline constexpr const char* texture    = "space.texture";    // drives density+diffusion
    inline constexpr const char* earlyLate  = "space.earlylate";  // kept for automation; not in UI

    // TONE (advanced)
    inline constexpr const char* lowCut     = "tone.lowcut";
    inline constexpr const char* highCut    = "tone.highcut";
    inline constexpr const char* lowDamp    = "tone.lowdamp";
    inline constexpr const char* highDamp   = "tone.highdamp";

    // MOD (advanced)
    inline constexpr const char* modRate    = "mod.rate";   // kept for automation; not in UI
    inline constexpr const char* modDepth   = "mod.depth";  // shown as "Motion"

    // STEREO (advanced)
    inline constexpr const char* width      = "stereo.width";
    inline constexpr const char* monoLow    = "stereo.monolow"; // wet mono below ~150Hz
}

namespace DuckModes
{
    enum Mode { Smart = 0, Full, Vocal, Transient, Custom };
    inline const juce::StringArray choices { "Smart", "Full", "Vocal", "Transient", "Custom" };
}

namespace EchoRouting
{
    enum Routing { Before = 0, Inside, After };
    inline const juce::StringArray choices { "Before", "Inside", "After" };
}

namespace EchoTimes
{
    // Index order is a stable API — append only.
    inline const juce::StringArray choices {
        "1/32", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D",
        "1/4T", "1/4", "1/4D", "1/2T", "1/2", "1/2D", "1 Bar", "2 Bars", "Free"
    };

    inline constexpr int kFreeIndex = 15;
    inline constexpr int kDefaultIndex = 9; // 1/4D

    // Multiplier of a quarter note for each synced choice (Free excluded)
    inline constexpr float beatMultipliers[15] = {
        0.125f,          // 1/32
        1.0f / 6.0f,     // 1/16T
        0.25f,           // 1/16
        0.375f,          // 1/16D
        1.0f / 3.0f,     // 1/8T
        0.5f,            // 1/8
        0.75f,           // 1/8D
        2.0f / 3.0f,     // 1/4T
        1.0f,            // 1/4
        1.5f,            // 1/4D
        4.0f / 3.0f,     // 1/2T
        2.0f,            // 1/2
        3.0f,            // 1/2D
        4.0f,            // 1 Bar
        8.0f             // 2 Bars
    };

    inline double timeMs (int index, double bpm, float freeMs)
    {
        if (index == kFreeIndex)
            return (double) freeMs;
        if (bpm <= 0.0) bpm = 120.0;
        index = juce::jlimit (0, 14, index);
        return (60000.0 / bpm) * (double) beatMultipliers[index];
    }
}

// =============================================================================
// APVTS layout — all defaults follow spec §16 / §35 (INIT — CLEAN SPACE)
// =============================================================================
inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using P  = juce::AudioParameterFloat;
    using PB = juce::AudioParameterBool;
    using PC = juce::AudioParameterChoice;
    using Range = juce::NormalisableRange<float>;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto pct = Range (0.0f, 1.0f, 0.0001f);

    // --- MAIN ---
    params.push_back (std::make_unique<P> (juce::ParameterID (ParamIDs::mix, 1),   "Mix",   pct, 0.25f));
    params.push_back (std::make_unique<P> (juce::ParameterID (ParamIDs::size, 1),  "Size",  pct, 0.55f));

    Range decayRange (0.20f, 30.0f, 0.001f);
    decayRange.setSkewForCentre (2.8f); // musically curated nonlinear range
    params.push_back (std::make_unique<P> (juce::ParameterID (ParamIDs::decay, 1), "Decay", decayRange, 2.2f));

    params.push_back (std::make_unique<P> (juce::ParameterID (ParamIDs::bloom, 1),     "Bloom",     pct, 0.10f));
    params.push_back (std::make_unique<P> (juce::ParameterID (ParamIDs::silk, 1),      "Silk",      pct, 0.35f));
    params.push_back (std::make_unique<P> (juce::ParameterID (ParamIDs::character, 1), "Character", pct, 0.25f));
    params.push_back (std::make_unique<PB> (juce::ParameterID (ParamIDs::bypass, 1),   "Bypass",    false));
    params.push_back (std::make_unique<PB> (juce::ParameterID (ParamIDs::autoGain, 1), "Auto Gain", true));
    params.push_back (std::make_unique<PB> (juce::ParameterID (ParamIDs::soloWet, 1),  "Solo Wet",  false));

    // --- FREEZE ---
    params.push_back (std::make_unique<PB> (juce::ParameterID (ParamIDs::freeze, 1), "Freeze", false));

    // --- DUCK ---
    params.push_back (std::make_unique<P>  (juce::ParameterID (ParamIDs::duckAmount, 1), "Duck", pct, 0.20f));
    params.push_back (std::make_unique<PC> (juce::ParameterID (ParamIDs::duckMode, 1),   "Duck Mode", DuckModes::choices, DuckModes::Smart));

    Range atkRange (0.1f, 200.0f, 0.01f);  atkRange.setSkewForCentre (15.0f);
    params.push_back (std::make_unique<P> (juce::ParameterID (ParamIDs::duckAttack, 1), "Duck Attack", atkRange, 10.0f));
    params.push_back (std::make_unique<P> (juce::ParameterID (ParamIDs::duckHold, 1),   "Duck Hold",
                                           Range (0.0f, 500.0f, 1.0f), 40.0f));
    Range relRange (20.0f, 3000.0f, 1.0f); relRange.setSkewForCentre (350.0f);
    params.push_back (std::make_unique<P> (juce::ParameterID (ParamIDs::duckRelease, 1), "Duck Release", relRange, 300.0f));
    params.push_back (std::make_unique<P> (juce::ParameterID (ParamIDs::duckSensitivity, 1), "Duck Sensitivity", pct, 0.5f));

    // --- ECHO ---
    params.push_back (std::make_unique<P>  (juce::ParameterID (ParamIDs::echoAmount, 1), "Echo", pct, 0.0f));
    params.push_back (std::make_unique<PC> (juce::ParameterID (ParamIDs::echoTime, 1),   "Echo Time", EchoTimes::choices, EchoTimes::kDefaultIndex));
    Range freeRange (10.0f, 2000.0f, 1.0f); freeRange.setSkewForCentre (350.0f);
    params.push_back (std::make_unique<P>  (juce::ParameterID (ParamIDs::echoFreeMs, 1), "Echo Free Time", freeRange, 350.0f));
    params.push_back (std::make_unique<P>  (juce::ParameterID (ParamIDs::echoFeedback, 1), "Echo Feedback",
                                            Range (0.0f, 0.95f, 0.001f), 0.25f));
    params.push_back (std::make_unique<PC> (juce::ParameterID (ParamIDs::echoRouting, 1), "Echo Routing", EchoRouting::choices, EchoRouting::Before));
    params.push_back (std::make_unique<PB> (juce::ParameterID (ParamIDs::echoPingPong, 1), "Echo Ping Pong", false));
    Range echoLcRange (20.0f, 2000.0f, 1.0f);   echoLcRange.setSkewForCentre (200.0f);
    params.push_back (std::make_unique<P>  (juce::ParameterID (ParamIDs::echoLowCut, 1), "Echo Low Cut", echoLcRange, 150.0f));
    Range echoHcRange (1000.0f, 20000.0f, 1.0f); echoHcRange.setSkewForCentre (6000.0f);
    params.push_back (std::make_unique<P>  (juce::ParameterID (ParamIDs::echoHighCut, 1), "Echo High Cut", echoHcRange, 10000.0f));

    // --- SPACE ---
    params.push_back (std::make_unique<P> (juce::ParameterID (ParamIDs::preDelay, 1), "Pre-Delay",
                                           Range (0.0f, 250.0f, 0.1f), 20.0f));
    params.push_back (std::make_unique<P> (juce::ParameterID (ParamIDs::texture, 1),   "Texture",   pct, 0.70f));
    params.push_back (std::make_unique<P> (juce::ParameterID (ParamIDs::earlyLate, 1), "Early/Late",
                                           Range (-1.0f, 1.0f, 0.01f), 0.30f));

    // --- TONE ---
    Range lcRange (20.0f, 1000.0f, 1.0f);   lcRange.setSkewForCentre (150.0f);
    params.push_back (std::make_unique<P> (juce::ParameterID (ParamIDs::lowCut, 1),  "Low Cut",  lcRange, 120.0f));
    Range hcRange (2000.0f, 20000.0f, 1.0f); hcRange.setSkewForCentre (8000.0f);
    params.push_back (std::make_unique<P> (juce::ParameterID (ParamIDs::highCut, 1), "High Cut", hcRange, 12000.0f));
    params.push_back (std::make_unique<P> (juce::ParameterID (ParamIDs::lowDamp, 1),  "Low Damp",  pct, 0.20f));
    params.push_back (std::make_unique<P> (juce::ParameterID (ParamIDs::highDamp, 1), "High Damp", pct, 0.40f));

    // --- MOD ---
    Range rateRange (0.01f, 5.0f, 0.001f); rateRange.setSkewForCentre (0.5f);
    params.push_back (std::make_unique<P> (juce::ParameterID (ParamIDs::modRate, 1),  "Mod Rate",  rateRange, 0.25f));
    params.push_back (std::make_unique<P> (juce::ParameterID (ParamIDs::modDepth, 1), "Mod Depth", pct, 0.15f));

    // --- STEREO ---
    params.push_back (std::make_unique<P> (juce::ParameterID (ParamIDs::width, 1), "Width",
                                           Range (0.0f, 2.0f, 0.01f), 1.15f));
    params.push_back (std::make_unique<PB> (juce::ParameterID (ParamIDs::monoLow, 1), "Mono Low", true));

    return { params.begin(), params.end() };
}
