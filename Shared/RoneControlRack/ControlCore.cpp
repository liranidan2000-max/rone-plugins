#include "ControlCore.h"

namespace RoneCtl
{
// ============================================================================
// Hand frame <-> bytes (engine -> plugins) and <- the page's JSON
// ============================================================================
juce::MemoryBlock HandFrame::encode() const
{
    juce::MemoryOutputStream mo (512);
    mo.writeInt ((int) kFrameMagic);
    mo.writeInt ((int) seq);
    mo.writeDouble (engineTime);
    mo.writeFloat (aspect);
    mo.writeByte ((char) source);
    for (const auto& h : hands)
    {
        mo.writeByte ((char) ((h.visible ? 1 : 0) | (h.pinched ? 2 : 0) | (h.fisted ? 4 : 0)));
        for (float v : { h.height, h.side, h.depth, h.twist, h.pinch, h.grip, h.pcx, h.pcy })
            mo.writeFloat (v);
        for (int i = 0; i < 21; ++i) { mo.writeFloat (h.lm[i][0]); mo.writeFloat (h.lm[i][1]); }
    }
    return mo.getMemoryBlock();
}

bool HandFrame::decode (const juce::MemoryBlock& mb, HandFrame& out)
{
    constexpr size_t perHand = 1 + 8 * 4 + 42 * 4;
    constexpr size_t need = 4 + 4 + 8 + 4 + 1 + 2 * perHand;
    if (mb.getSize() < need) return false;
    juce::MemoryInputStream mi (mb, false);
    if ((juce::uint32) mi.readInt() != kFrameMagic) return false;
    HandFrame f;
    f.seq = (juce::uint32) mi.readInt();
    f.engineTime = mi.readDouble();
    f.aspect = mi.readFloat();
    f.source = (juce::uint8) mi.readByte();
    for (auto& h : f.hands)
    {
        const auto flags = (juce::uint8) mi.readByte();
        h.visible = (flags & 1) != 0; h.pinched = (flags & 2) != 0; h.fisted = (flags & 4) != 0;
        h.height = mi.readFloat(); h.side = mi.readFloat(); h.depth = mi.readFloat(); h.twist = mi.readFloat();
        h.pinch  = mi.readFloat(); h.grip = mi.readFloat(); h.pcx   = mi.readFloat(); h.pcy   = mi.readFloat();
        for (int i = 0; i < 21; ++i) { h.lm[i][0] = mi.readFloat(); h.lm[i][1] = mi.readFloat(); }
    }
    out = f;
    return true;
}

HandFrame HandFrame::fromVar (const juce::var& v)
{
    // Page and phone data is untrusted: a NaN reaching a slot would poison its filter and then the
    // DAW parameter, so anything that is not a finite number is treated as "no hand".
    auto num = [] (const juce::var& x, float fallback)
    {
        if (! (x.isDouble() || x.isInt() || x.isInt64())) return fallback;      // JSON null (a NaN in JS) or missing
        const auto d = (double) x;
        return std::isfinite (d) ? (float) d : fallback;
    };
    HandFrame f;
    f.seq = (juce::uint32) (int) v["seq"];
    f.engineTime = (double) v["t"];
    f.aspect = num (v["aspect"], 4.0f / 3.0f);
    if (f.aspect < 0.2f || f.aspect > 5.0f) f.aspect = 4.0f / 3.0f;
    const auto src = v["source"].toString();
    f.source = src == "cam" ? 1 : src == "sim" ? 2 : src == "phone" ? 3 : 0;
    if (auto* arr = v["hands"].getArray())
        for (const auto& hv : *arr)
        {
            const int idx = hv["h"].toString() == "L" ? 0 : 1;
            const float nan = std::numeric_limits<float>::quiet_NaN();
            const float vals[] = { num (hv["height"], nan), num (hv["sideX"], nan), num (hv["depth"], nan), num (hv["twist"], nan),
                                   num (hv["pinch"], nan), num (hv["grip"], nan), num (hv["pcx"], nan), num (hv["pcy"], nan) };
            bool finite = true;
            for (float x : vals) finite = finite && std::isfinite (x);
            if (! finite) continue;
            auto& h = f.hands[idx];
            h.visible = true;
            h.pinched = (bool) hv["p"];
            h.fisted  = (bool) hv["f"];
            h.height = vals[0]; h.side = vals[1]; h.depth = vals[2]; h.twist = vals[3];
            h.pinch  = vals[4]; h.grip = vals[5]; h.pcx = vals[6]; h.pcy = vals[7];
            if (auto* lm = hv["lm"].getArray())
                for (int i = 0; i < 21 && i * 2 + 1 < lm->size(); ++i)
                {
                    h.lm[i][0] = num ((*lm)[i * 2], 0.0f);
                    h.lm[i][1] = num ((*lm)[i * 2 + 1], 0.0f);
                }
        }
    return f;
}

// ============================================================================
// Slot config <-> page / state
// ============================================================================
namespace
{
    const char* const srcKeys[]   = { "height", "side", "depth", "twist", "pinch", "grip", "spread" };
    const char* const handKeys[]  = { "L", "R", "ANY" };
    const char* const engKeys[]   = { "pinch", "fist", "always" };
    const char* const modeKeys[]  = { "grab", "follow" };
    const char* const relKeys[]   = { "stay", "return" };
    const char* const curveKeys[] = { "lin", "soft", "exp", "s" };

    template <typename E, size_t N>
    E fromKey (const juce::String& k, const char* const (&keys)[N], E fallback)
    {
        for (size_t i = 0; i < N; ++i)
            if (k == keys[i]) return (E) i;
        return fallback;
    }

    struct Range { float abs0, abs1, reach0, reach1; };
    const Range ranges[] = {
        { 0.22f, 0.82f, 0.22f, 0.70f },   // height (fraction of the frame)
        { 0.20f, 0.80f, 0.22f, 0.70f },   // side
        { 0.15f, 0.85f, 0.25f, 0.80f },   // depth
        { -70.f, 70.f, 60.f, 200.f },     // twist (degrees)
        { 0.06f, 0.85f, 0.30f, 0.90f },   // pinch opening
        { 0.05f, 0.90f, 0.30f, 0.90f },   // grip
        { 0.12f, 0.75f, 0.20f, 0.70f },   // two-hand spread
    };

    template <typename T> T lerp (T a, T b, T t) { return a + (b - a) * t; }
}

juce::String srcName (Src s) { return juce::String (srcKeys[(int) s]).toUpperCase(); }

juce::var SlotConfig::toVar() const
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("enabled", enabled);
    o->setProperty ("name", name);
    o->setProperty ("src", srcKeys[(int) src]);
    o->setProperty ("hand", handKeys[(int) hand]);
    o->setProperty ("eng", engKeys[(int) eng]);
    o->setProperty ("mode", modeKeys[(int) mode]);
    o->setProperty ("rel", relKeys[(int) rel]);
    o->setProperty ("curve", curveKeys[(int) curve]);
    o->setProperty ("smooth", smooth); o->setProperty ("reach", reach);
    o->setProperty ("lo", lo); o->setProperty ("hi", hi); o->setProperty ("home", home);
    o->setProperty ("cc", cc);
    o->setProperty ("ch", ch);
    o->setProperty ("target", target);
    o->setProperty ("targetOf", targetOf);
    o->setProperty ("ref", ref);
    return juce::var (o);
}

void SlotConfig::applyVar (const juce::var& v)
{
    if (v.hasProperty ("enabled")) enabled = (bool) v["enabled"];
    if (v.hasProperty ("name"))    name = v["name"].toString().toUpperCase().substring (0, 14);
    if (v.hasProperty ("src"))     src   = fromKey (v["src"].toString(),   srcKeys,   src);
    if (v.hasProperty ("hand"))    hand  = fromKey (v["hand"].toString(),  handKeys,  hand);
    if (v.hasProperty ("eng"))     eng   = fromKey (v["eng"].toString(),   engKeys,   eng);
    if (v.hasProperty ("mode"))    mode  = fromKey (v["mode"].toString(),  modeKeys,  mode);
    if (v.hasProperty ("rel"))     rel   = fromKey (v["rel"].toString(),   relKeys,   rel);
    if (v.hasProperty ("curve"))   curve = fromKey (v["curve"].toString(), curveKeys, curve);
    if (v.hasProperty ("smooth"))  smooth = juce::jlimit (0.0f, 1.0f, (float) v["smooth"]);
    if (v.hasProperty ("reach"))   reach  = juce::jlimit (0.0f, 1.0f, (float) v["reach"]);
    if (v.hasProperty ("lo"))      lo     = juce::jlimit (0.0f, 1.0f, (float) v["lo"]);
    if (v.hasProperty ("hi"))      hi     = juce::jlimit (0.0f, 1.0f, (float) v["hi"]);
    if (v.hasProperty ("home"))    home   = juce::jlimit (0.0f, 1.0f, (float) v["home"]);
    if (v.hasProperty ("cc"))      cc     = juce::jlimit (0, 127, (int) v["cc"]);
    if (v.hasProperty ("ch"))      ch     = juce::jlimit (1, 16, (int) v["ch"]);
    if (v.hasProperty ("target"))   target   = v["target"].toString().substring (0, 40);
    if (v.hasProperty ("targetOf")) targetOf = v["targetOf"].toString().substring (0, 40);
    if (v.hasProperty ("ref"))      ref      = v["ref"].toString().substring (0, 200);
    if (lo > hi) std::swap (lo, hi);
    // a move made of the fingers cannot also be held by the fingers
    if ((src == Src::pinch || src == Src::grip) && eng != Engage::always) eng = Engage::always;
    if (name.isEmpty()) name = "SLOT";
}

juce::ValueTree SlotConfig::toTree() const
{
    juce::ValueTree t ("SLOT");
    auto v = toVar();
    if (auto* o = v.getDynamicObject())
        for (const auto& p : o->getProperties())
            t.setProperty (p.name, p.value, nullptr);
    return t;
}

SlotConfig SlotConfig::fromTree (const juce::ValueTree& t)
{
    auto* o = new juce::DynamicObject();
    for (int i = 0; i < t.getNumProperties(); ++i)
        o->setProperty (t.getPropertyName (i), t.getProperty (t.getPropertyName (i)));
    SlotConfig c;
    c.applyVar (juce::var (o));
    return c;
}

// ============================================================================
// One Euro
// ============================================================================
float OneEuro::alpha (float cutoff, double dt) noexcept
{
    const double tau = 1.0 / (juce::MathConstants<double>::twoPi * (double) cutoff);
    return (float) (1.0 / (1.0 + tau / dt));
}

float OneEuro::filter (float v, double t) noexcept
{
    if (! primed) { x = v; dx = 0.0f; last = t; primed = true; return v; }
    const double dt = juce::jmax (1.0e-3, t - last);
    last = t;
    dx += alpha (dc, dt) * ((float) ((v - x) / dt) - dx);
    x  += alpha (mc + b * std::abs (dx), dt) * (v - x);
    return x;
}

// ============================================================================
// Slot mapper
// ============================================================================
float SlotMapper::shape (Curve c, float x) noexcept
{
    x = juce::jlimit (0.0f, 1.0f, x);
    switch (c)
    {
        case Curve::soft: return std::pow (x, 1.35f);
        case Curve::exp:  return std::pow (x, 2.4f);
        case Curve::s:    return x < 0.5f ? 2.0f * x * x : 1.0f - 2.0f * (1.0f - x) * (1.0f - x);
        case Curve::lin:  default: return x;
    }
}

float SlotMapper::unshape (Curve c, float y) noexcept
{
    y = juce::jlimit (0.0f, 1.0f, y);
    switch (c)
    {
        case Curve::soft: return std::pow (y, 1.0f / 1.35f);
        case Curve::exp:  return std::pow (y, 1.0f / 2.4f);
        case Curve::s:    return y < 0.5f ? std::sqrt (y * 0.5f) : 1.0f - std::sqrt ((1.0f - y) * 0.5f);
        case Curve::lin:  default: return y;
    }
}

float SlotMapper::output (const SlotConfig& c, float pos) noexcept
{
    return juce::jlimit (0.0f, 1.0f, c.lo + (c.hi - c.lo) * shape (c.curve, pos));
}

float SlotMapper::posFromOutput (const SlotConfig& c, float out) noexcept
{
    const float span = c.hi - c.lo;
    if (span < 1.0e-5f) return 0.0f;
    return unshape (c.curve, (out - c.lo) / span);
}

bool SlotMapper::engaged (const SlotConfig& c, const Hand& h) noexcept
{
    switch (c.eng)
    {
        case Engage::always: return true;
        case Engage::fist:   return h.fisted;
        case Engage::pinch:  default: return h.pinched;
    }
}

const Hand* SlotMapper::pickHand (const SlotConfig& c, const HandFrame& f) noexcept
{
    const auto& L = f.hands[0];
    const auto& R = f.hands[1];
    if (c.src == Src::spread)
        return (L.visible && R.visible && engaged (c, L) && engaged (c, R)) ? &R : nullptr;
    if (c.hand == HandSel::left)  return L.visible ? &L : nullptr;
    if (c.hand == HandSel::right) return R.visible ? &R : nullptr;
    if (R.visible && engaged (c, R)) return &R;
    if (L.visible && engaged (c, L)) return &L;
    return R.visible ? &R : (L.visible ? &L : nullptr);
}

float SlotMapper::raw (const SlotConfig& c, const Hand& h, const HandFrame& f) noexcept
{
    switch (c.src)
    {
        case Src::height: return h.height;
        case Src::side:   return h.side;
        case Src::depth:  return h.depth;
        case Src::twist:  return h.twist;
        case Src::pinch:  return h.pinch;
        case Src::grip:   return h.grip;
        case Src::spread: return std::abs (f.hands[0].pcx - f.hands[1].pcx);
    }
    return 0.0f;
}

bool SlotMapper::step (const SlotConfig& c, SlotState& s, const HandFrame& f, bool live, double t, double dt)
{
    const Hand* h = live ? pickHand (c, f) : nullptr;
    const bool want = h != nullptr && (c.src == Src::spread || engaged (c, *h));
    const float r = h != nullptr ? raw (c, *h, f) : 0.0f;
    s.driver = want ? (c.src == Src::spread ? 'B' : (h == &f.hands[0] ? 'L' : 'R')) : 0;

    const auto& rg = ranges[(int) c.src];
    if (want && ! s.held)
    {
        s.held = true; s.settleUntil = t + 0.06; s.undo = s.pos; s.started = false; s.returning = false;
        // One Euro tuned for a knob: steady when the hand rests, near-zero lag once it moves
        // (at SMOOTH 35 %: 3.1 Hz at rest, ~7 Hz at a slow sweep of 0.3 frame/s).
        const float span = c.src == Src::twist ? 60.0f : 1.0f;     // twist lives in degrees
        s.filt.setup (lerp (4.5f, 0.6f, c.smooth), lerp (20.0f, 3.0f, c.smooth) / span, 1.0f);
        s.filt.filter (r, t);
    }
    else if (! want && s.held)
    {
        s.held = false;                                               // the knob stays where you let go ...
        if (c.rel == Release::ret) s.returning = true;                // ... or glides home
    }

    if (s.held)
    {
        const float fr = s.filt.filter (r, t);
        if (t >= s.settleUntil)                                       // the pinch has closed: the palm has stopped shifting
        {
            if (c.mode == Mode::follow)
                s.pos = juce::jlimit (0.0f, 1.0f, (fr - rg.abs0) / (rg.abs1 - rg.abs0));
            else
            {
                if (! s.started) { s.startRaw = fr; s.startPos = s.pos; s.started = true; }
                const float span = lerp (rg.reach0, rg.reach1, c.reach);
                const float p = s.startPos + (fr - s.startRaw) / span;
                s.pos = juce::jlimit (0.0f, 1.0f, p);
                if (p != s.pos) { s.startPos = s.pos; s.startRaw = fr; }   // past the end: the way back answers at once
            }
        }
    }
    else if (s.returning)
    {
        s.pos += (c.home - s.pos) * (1.0f - (float) std::exp (-dt / 0.09));
        if (std::abs (s.pos - c.home) < 0.002f) { s.pos = c.home; s.returning = false; }
    }
    return s.held || s.returning;
}

// ============================================================================
// Presets (gesture setups)
// ============================================================================
namespace
{
    const Preset presets[] = { { "FILTER RIDE", "PINCH + MOVE" }, { "FREE HAND", "NO GRAB" }, { "SPRING BACK", "RETURNS HOME" } };

    SlotConfig spare (int i)
    {
        static const Src order[] = { Src::height, Src::side, Src::twist, Src::depth, Src::pinch, Src::grip, Src::spread, Src::height };
        static const char* names[] = { "SEND", "WIDTH", "DRIVE", "SPACE", "MIX", "CRUSH", "SPREAD", "LEVEL" };
        SlotConfig c;
        c.enabled = false; c.src = order[i]; c.name = names[i]; c.cc = 20 + i;
        if (c.src == Src::pinch || c.src == Src::grip) c.eng = Engage::always;
        return c;
    }
}

int numPresets() noexcept { return (int) std::size (presets); }
Preset presetInfo (int i) noexcept { return presets[juce::jlimit (0, numPresets() - 1, i)]; }

void loadPreset (int i, SlotConfig (&cfg)[kNumSlots], float (&pos)[kNumSlots])
{
    for (int k = 0; k < kNumSlots; ++k) { cfg[k] = spare (k); pos[k] = 0.0f; }
    auto set = [&] (int k, const char* name, Src src, int cc, float p) -> SlotConfig&
    {
        auto& c = cfg[k];
        c.enabled = true; c.name = name; c.src = src; c.cc = cc; pos[k] = p;
        return c;
    };
    switch (juce::jlimit (0, numPresets() - 1, i))
    {
        case 1:   // FREE HAND - no grab, the knob sits where the hand is
            { auto& a = set (0, "CUTOFF", Src::height, 74, 0.5f); a.eng = Engage::always; a.mode = Mode::follow; a.curve = Curve::soft; }
            { auto& b = set (1, "PAN", Src::side, 10, 0.5f);      b.eng = Engage::always; b.mode = Mode::follow; b.home = 0.5f; }
            break;
        case 2:   // SPRING BACK - grab, sweep, let go: it glides home
            { auto& a = set (0, "CUTOFF", Src::height, 74, 0.3f); a.rel = Release::ret; a.home = 0.3f; a.curve = Curve::soft; }
            { auto& b = set (1, "RESO", Src::twist, 71, 0.15f);   b.rel = Release::ret; b.home = 0.15f; b.hi = 0.85f; }
            break;
        case 0:
        default:  // FILTER RIDE - pinch and move: height = cutoff, side = pan, twist = resonance
            { auto& a = set (0, "CUTOFF", Src::height, 74, 0.25f); a.curve = Curve::soft; }
            { auto& b = set (1, "PAN", Src::side, 10, 0.5f);       b.home = 0.5f; }
            { auto& c = set (2, "RESO", Src::twist, 71, 0.28f);    c.hi = 0.85f; }
            break;
    }
}
} // namespace RoneCtl
