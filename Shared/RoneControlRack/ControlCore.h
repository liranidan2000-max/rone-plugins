#pragma once

#include <JuceHeader.h>

// ============================================================================
// RONE Control core: the hand frame the engine broadcasts, the slot config,
// and the slot mapper (GRAB / FOLLOW, clutch settle, One Euro, curve, range).
// Pure logic, runs on the message thread. The page's JS does the per-hand work
// (features + clutch hysteresis) so the phone, the engine and the plugin share it.
// ============================================================================
namespace RoneCtl
{
    constexpr int kNumSlots = 8;

    // Where the pairing, the chosen source and camera, the MIDI port choice and the link's files live.
    // Tests point RONE_CONTROL_DATA_DIR at a scratch folder, so a user's own choices are never touched.
    inline juce::File controlDataFolder()
    {
        const auto test = juce::SystemStats::getEnvironmentVariable ("RONE_CONTROL_DATA_DIR", {});
        if (test.isNotEmpty()) return juce::File (test);
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("RonePlugins").getChildFile ("Control");
    }

    // A WebView2 profile folder. In tests it moves under RONE_CONTROL_DATA_DIR too: a DAW that has RONE Control
    // open holds the real folder, and a second process opening it with other browser arguments cannot start.
    inline juce::File webViewFolder (const juce::String& name)
    {
        const auto test = juce::SystemStats::getEnvironmentVariable ("RONE_CONTROL_DATA_DIR", {});
        if (test.isNotEmpty()) return juce::File (test).getChildFile (name);
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("RonePlugins").getChildFile (name);
    }
    constexpr int kPort     = 47931;           // engine <-> plugins, localhost only (the tests' scripted hands)
    constexpr int kPeerPort = 47932;           // the link's owner -> RONE Control in the other DAWs, localhost only
    constexpr juce::uint32 kFrameMagic = 0x3146564C;  // 'LVF1'

    // ---------------------------------------------------------------- the hand
    struct Hand
    {
        bool  visible = false, pinched = false, fisted = false;
        float height = 0.0f, side = 0.0f, depth = 0.0f;
        float twist  = 0.0f;                   // degrees, unwrapped (turning past 180 keeps counting)
        float pinch  = 0.0f, grip = 0.0f;      // 0..1
        float pcx = 0.0f, pcy = 0.0f;          // palm centre, mirrored + normalised
        float lm[21][2] {};                    // mirrored, normalised landmarks (drawing only)
    };

    struct HandFrame
    {
        juce::uint32 seq = 0;
        double engineTime = 0.0;
        float  aspect = 4.0f / 3.0f;
        juce::uint8 source = 0;                // 0 none, 1 camera, 2 sim, 3 phone
        Hand   hands[2];                       // [0] left, [1] right

        bool anyVisible() const noexcept { return hands[0].visible || hands[1].visible; }
        juce::MemoryBlock encode() const;
        static bool decode (const juce::MemoryBlock&, HandFrame& out);
        static HandFrame fromVar (const juce::var& v);   // the page's "hands" event
    };

    // ---------------------------------------------------------------- slot config
    enum class Src     { height, side, depth, twist, pinch, grip, spread };
    enum class HandSel { left, right, any };
    enum class Engage  { pinch, fist, always };
    enum class Mode    { grab, follow };
    enum class Release { stay, ret };
    enum class Curve   { lin, soft, exp, s };

    struct SlotConfig
    {
        bool enabled = false;
        juce::String name { "SLOT" };
        Src src = Src::height;
        HandSel hand = HandSel::any;
        Engage eng = Engage::pinch;
        Mode mode = Mode::grab;
        Release rel = Release::stay;
        Curve curve = Curve::lin;
        float smooth = 0.35f, reach = 0.5f, lo = 0.0f, hi = 1.0f, home = 0.0f;
        int cc = 20, ch = 1;                           // its CC and MIDI channel (1..16) on the port and the MIDI out
        juce::String target, targetOf;                 // FL: what the slot is linked to ("Cutoff", "Serum") - display only
        juce::String ref;                              // REAPER: which knob ("trackGUID|fxGUID|param|plink or learn")

        juce::var toVar() const;
        void applyVar (const juce::var& v);            // partial updates from the page
        juce::ValueTree toTree() const;
        static SlotConfig fromTree (const juce::ValueTree& t);
    };

    // ---------------------------------------------------------------- One Euro (Casiez et al. 2012)
    class OneEuro
    {
    public:
        void setup (float minCutoff, float beta, float dCutoff = 1.0f) noexcept { mc = minCutoff; b = beta; dc = dCutoff; primed = false; }
        float filter (float x, double t) noexcept;
    private:
        static float alpha (float cutoff, double dt) noexcept;
        float mc = 1.0f, b = 0.0f, dc = 1.0f, x = 0.0f, dx = 0.0f;
        double last = 0.0;
        bool primed = false;
    };

    // ---------------------------------------------------------------- slot runtime
    struct SlotState
    {
        float  pos = 0.0f;           // knob position 0..1 before curve and range
        bool   held = false, returning = false, started = false;
        double settleUntil = 0.0;
        float  startRaw = 0.0f, startPos = 0.0f, undo = -1.0f;
        char   driver = 0;           // 'L', 'R', 'B' (both) while held
        OneEuro filt;
    };

    struct SlotMapper
    {
        // One step. Returns true while the slot is "active" (held, gliding home or test-moving):
        // the processor keeps a host change-gesture open for exactly that long.
        static bool step (const SlotConfig& c, SlotState& s, const HandFrame& f, bool live, double t, double dt);

        static float shape (Curve c, float x) noexcept;
        static float unshape (Curve c, float y) noexcept;
        static float output (const SlotConfig& c, float pos) noexcept;       // the host parameter value
        static float posFromOutput (const SlotConfig& c, float out) noexcept;
        static const Hand* pickHand (const SlotConfig& c, const HandFrame& f) noexcept;
        static bool  engaged (const SlotConfig& c, const Hand& h) noexcept;
        static float raw (const SlotConfig& c, const Hand& h, const HandFrame& f) noexcept;
    };

    // Gesture setups (the bottom preset row). Index 0 is what a fresh instance opens with.
    struct Preset { const char* name; const char* sub; };
    int numPresets() noexcept;
    Preset presetInfo (int i) noexcept;
    void loadPreset (int i, SlotConfig (&cfg)[kNumSlots], float (&pos)[kNumSlots]);

    juce::String srcName (Src s);
}
