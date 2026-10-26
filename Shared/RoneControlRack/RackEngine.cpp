#include "RackEngine.h"
#include "RackArmed.h"
#include "../RoneUpdatePrompt.h"      // RoneUpdates::launchCenterFor - the "install RONE Control" card's button

namespace RoneRack
{
using namespace RoneCtl;

namespace
{
    double nowSeconds() { return juce::Time::getMillisecondCounterHiRes() * 0.001; }
    constexpr int kMaxSlots = 4;                 // two per hand at most: more is not playable
    constexpr double kFresh = 0.5;               // a frame older than this = no hands (a camera that stopped lets go)

    juce::String letter (int i) { return juce::String::charToString ((juce::juce_wchar) ('A' + i)); }

    // What the card shows: the parameter's own text when it has one (a label, a choice); a plain 0..1 knob as a percent
    juce::String valueText (const juce::RangedAudioParameter& p, float norm)
    {
        if (dynamic_cast<const juce::AudioParameterBool*> (&p) != nullptr) return norm > 0.5f ? "ON" : "OFF";
        const auto& r = p.getNormalisableRange();
        const float real = r.convertFrom0to1 (norm);
        if (p.getLabel().isEmpty() && r.start >= -1.0f && r.end <= 1.5f && r.end - r.start >= 0.5f)
            return (r.start < 0.0f && real > 0.0f ? "+" : "") + juce::String (juce::roundToInt (real * 100.0f)) + "%";
        auto t = p.getText (norm, 16).trim();
        if (t.containsOnly ("0123456789.-") && t.contains (".")) t = juce::String (real, std::abs (real) >= 100.0f ? 0 : 1);
        return p.getLabel().isNotEmpty() ? t + " " + p.getLabel() : t;
    }
}

Engine::Engine (juce::AudioProcessorValueTreeState& s, const juce::String& pluginId, const juce::String& manifestJson)
    : apvts (s), id (pluginId)
{
    manifest = juce::JSON::parse (manifestJson);
    name = manifest["name"].toString();
    accent = manifest["accent"].toString();
    if (accent.isEmpty()) accent = "#D8E4EC";
    myId = juce::Uuid().toString();
    loadSaved();
    if (slots.empty()) loadManifest();
    hub->addClient (this);
    startTimerHz (60);
}

Engine::~Engine()
{
    stopTimer();
    releaseAll();
    hub->removeClient (this);
}

// ---------------------------------------------------------------- the map
juce::RangedAudioParameter* Engine::paramFor (const Slot& s) const
{
    return apvts.getParameter (s.param);
}

Engine::Slot Engine::slotFromVar (const juce::var& v) const
{
    Slot s;
    s.param = v["param"].toString();
    s.why = v["why"].toString();
    s.cfg.enabled = ! v.hasProperty ("on") || (bool) v["on"];
    s.cfg.hand = HandSel::any;                              // the manifest says which hand; "any" is a choice too
    auto c = v.hasProperty ("cfg") ? v["cfg"] : v;          // a saved slot carries its cfg object; a manifest row is flat
    s.cfg.applyVar (c);
    if (v.hasProperty ("on")) s.cfg.enabled = (bool) v["on"];
    if ((s.cfg.src == Src::pinch || s.cfg.src == Src::grip) && ! c.hasProperty ("eng")) s.cfg.eng = Engage::always;
    if (auto* p = paramFor (s)) s.cfg.name = p->getName (14).toUpperCase();
    return s;
}

juce::var Engine::slotToVar (const Slot& s, int i, bool forPage) const
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("param", s.param);
    o->setProperty ("on", s.cfg.enabled);
    o->setProperty ("why", s.why);
    o->setProperty ("cfg", s.cfg.toVar());
    if (forPage)
    {
        o->setProperty ("i", i);
        o->setProperty ("l", letter (i));
        auto* p = paramFor (s);
        o->setProperty ("name", p != nullptr ? p->getName (14).toUpperCase() : s.param.toUpperCase());
        const float n = p != nullptr ? p->getValue() : 0.0f;
        o->setProperty ("norm", n);
        o->setProperty ("text", p != nullptr ? valueText (*p, n) : juce::String());
        const bool isBool = dynamic_cast<juce::AudioParameterBool*> (p) != nullptr;
        o->setProperty ("bool", isBool);
        // where 0 sits for a knob that goes both ways (FEEDBACK -1..1, PITCH -200..200): the bar grows from there
        float zero = 0.0f;
        if (! isBool && p != nullptr) { const auto& r = p->getNormalisableRange(); if (r.start < 0.0f && r.end > 0.0f) zero = r.convertTo0to1 (0.0f); }
        o->setProperty ("zero", zero);
        o->setProperty ("held", s.st.held && s.activeWas);
        o->setProperty ("active", s.activeWas);
        o->setProperty ("driver", juce::String::charToString ((juce::juce_wchar) (s.st.driver != 0 ? s.st.driver : ' ')).trim());
        o->setProperty ("missing", p == nullptr);
    }
    return juce::var (o);
}

void Engine::loadManifest()
{
    slots.clear();
    if (auto* arr = manifest["rack"].getArray())
        for (const auto& v : *arr)
        {
            if ((int) slots.size() >= kMaxSlots) break;
            auto s = slotFromVar (v);
            if (paramFor (s) != nullptr) slots.push_back (std::move (s));
        }
}

juce::File Engine::savedFile() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("RonePlugins").getChildFile (id).getChildFile ("rack-slots.json");
}

void Engine::loadSaved()
{
    const auto j = juce::JSON::parse (savedFile().loadFileAsString());
    if (! j.isObject()) return;
    rememberedOpen = (bool) j["open"];                      // remembered only: the editor opens the rack, and that open must start the link
    live = ! j.hasProperty ("live") || (bool) j["live"];
    useSim = j["source"].toString() == "sim";
    slots.clear();
    if (auto* arr = j["slots"].getArray())
        for (const auto& v : *arr)
        {
            if ((int) slots.size() >= kMaxSlots) break;
            auto s = slotFromVar (v);
            if (paramFor (s) != nullptr) slots.push_back (std::move (s));
        }
}

void Engine::save() const
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("open", open);
    o->setProperty ("live", live);
    o->setProperty ("source", useSim ? "sim" : "hub");
    juce::Array<juce::var> a;
    for (int i = 0; i < (int) slots.size(); ++i) a.add (slotToVar (slots[(size_t) i], i, false));
    o->setProperty ("slots", a);
    const auto f = savedFile();
    f.getParentDirectory().createDirectory();
    f.replaceWithText (juce::JSON::toString (juce::var (o)));
}

// ---------------------------------------------------------------- armed
// The file and its rules live in RackArmed.h - RONE Control itself reads and writes the same file.
juce::File Engine::armedFile() { return Armed::file(); }

void Engine::arm()
{
    Armed::take (myId, id);
    armed = true;
}

void Engine::disarm()
{
    if (! armed) return;
    Armed::releaseAll();                                                // nobody
    armed = false;
    releaseAll();
}

void Engine::checkArmed()
{
    const auto owner = Armed::read().id;
    if (owner.isEmpty() && open)                                        // the armed rack (or RONE Control's window) closed: an open one takes over
    {
        arm();
        return;
    }
    const bool now = owner == myId;
    if (now != armed)
    {
        armed = now;
        if (! armed) releaseAll();
    }
}

// ---------------------------------------------------------------- open / hub
void Engine::setOpen (bool shouldOpen)
{
    if (open == shouldOpen) return;
    open = shouldOpen;
    if (open && ! useSim) hub->start();                     // lazily: the first open brings the link up
    if (! open) { releaseAll(); disarm(); }                 // closed = never moves a knob; the hands go to another open rack
    save();
}

void Engine::editorClosed()
{
    reopen = open;
    setOpen (false);
}

void Engine::hubHands (const HandFrame& f)
{
    hubFrame = f;
    hubAt = nowSeconds();
}

juce::var Engine::hubFeedback()
{
    juce::Array<juce::var> a;
    for (int i = 0; i < (int) slots.size(); ++i)
    {
        const auto& s = slots[(size_t) i];
        if (! s.cfg.enabled) continue;
        auto* o = new juce::DynamicObject();
        o->setProperty ("l", letter (i));
        o->setProperty ("name", s.cfg.name);
        o->setProperty ("tgt", name);
        auto* p = paramFor (s);
        o->setProperty ("out", p != nullptr ? std::round (p->getValue() * 1000.0f) / 1000.0f : 0.0f);
        o->setProperty ("held", s.st.held || s.activeWas);
        a.add (juce::var (o));
    }
    return a;
}

int Engine::handsNeeded() const
{
    bool left = false, right = false;
    for (const auto& s : slots)
    {
        if (! s.cfg.enabled) continue;
        if (s.cfg.src == Src::spread) return 2;
        left  = left  || s.cfg.hand == HandSel::left;
        right = right || s.cfg.hand == HandSel::right;
    }
    return (left && right) ? 2 : 1;
}

// ---------------------------------------------------------------- the step
void Engine::closeGesture (Slot& s)
{
    if (s.gestureOpen)
        if (auto* p = paramFor (s)) p->endChangeGesture();
    s.gestureOpen = false;
}

void Engine::releaseAll()
{
    for (auto& s : slots)
    {
        closeGesture (s);
        s.activeWas = false;
        s.st = SlotState {};
        s.lastBool = -1;
    }
}

void Engine::timerCallback()
{
    const double t = nowSeconds();
    if (++ticks % 10 == 0) checkArmed();
    if (! open) { lastT = t; return; }
    stepAll (t);
}

void Engine::stepAll (double t)
{
    const double dt = lastT > 0.0 ? juce::jlimit (0.001, 0.05, t - lastT) : 1.0 / 60.0;
    lastT = t;

    // the freshest frame of the chosen source; stale = nobody there
    HandFrame f;
    const double at = useSim ? simAt : hubAt;
    if (at > 0.0 && t - at < kFresh) f = useSim ? simFrame : hubFrame;
    const bool run = live && armed;

    for (auto& s : slots)
    {
        auto* p = paramFor (s);
        if (p == nullptr || ! s.cfg.enabled)
        {
            if (s.gestureOpen) closeGesture (s);
            s.activeWas = false;
            continue;
        }
        // Idle and the knob moved (the mouse, automation, a preset): the next grab starts from there.
        const float cur = p->getValue();
        if (! s.activeWas && std::abs (cur - s.lastSent) > 1.0e-4f)
        {
            s.st.pos = SlotMapper::posFromOutput (s.cfg, cur);
            s.lastSent = cur;
        }
        const bool active = SlotMapper::step (s.cfg, s.st, f, run, t, dt);
        if (active)
        {
            if (s.st.held) lastHeldAt = t;
            if (! s.gestureOpen) { p->beginChangeGesture(); s.gestureOpen = true; }
            const float out = SlotMapper::output (s.cfg, s.st.pos);
            if (dynamic_cast<juce::AudioParameterBool*> (p) != nullptr)
            {
                const int on = out > 0.5f ? 1 : 0;          // a switch: edges only (KILL, GO are momentary)
                if (on != s.lastBool) { s.lastBool = on; p->setValueNotifyingHost ((float) on); s.lastSent = (float) on; }
            }
            else if (std::abs (out - s.lastSent) > 1.0e-5f)
            {
                p->setValueNotifyingHost (out);
                s.lastSent = out;
            }
        }
        else if (s.gestureOpen)
        {
            closeGesture (s);
            s.lastBool = -1;
        }
        s.activeWas = active;
    }
}

// ---------------------------------------------------------------- the page
juce::var Engine::paramList() const
{
    juce::Array<juce::var> a;
    for (auto* prm : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
        {
            if (dynamic_cast<juce::AudioParameterChoice*> (rp) != nullptr) continue;   // a list is not a knob
            auto* o = new juce::DynamicObject();
            o->setProperty ("id", rp->paramID);
            o->setProperty ("name", rp->getName (14).toUpperCase());
            o->setProperty ("bool", dynamic_cast<juce::AudioParameterBool*> (rp) != nullptr);
            a.add (juce::var (o));
        }
    return a;
}

juce::var Engine::uiState (bool withHands)
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("open", open);
    o->setProperty ("armed", armed);
    o->setProperty ("live", live);
    o->setProperty ("name", name);
    o->setProperty ("accent", accent);
    o->setProperty ("canCamera", LinkHub::cameraSupported());
    o->setProperty ("source", useSim ? "sim" : (hub->source() == LinkHub::Source::camera ? "camera" : "phone"));
    o->setProperty ("linkStarted", hub->isStarted());
    o->setProperty ("linkError", hub->linkError());
    o->setProperty ("peer", hub->isPeer());

    const auto ps = hub->phoneStatus();
    auto* ph = new juce::DynamicObject();
    ph->setProperty ("state", hub->isStarted() ? ps.state : juce::String ("off"));
    ph->setProperty ("via", ps.via);
    ph->setProperty ("fps", ps.fps);
    ph->setProperty ("url", hub->phoneUrl());
    o->setProperty ("phone", juce::var (ph));

    auto* rec = new juce::DynamicObject();                              // the REC button
    rec->setProperty ("can", transport != nullptr && transport->canRecord());
    rec->setProperty ("on", transport != nullptr && transport->hostRecording());
    rec->setProperty ("how", transport != nullptr ? transport->how() : juce::String());
    rec->setProperty ("why", transport != nullptr ? transport->why() : juce::String ("No transport"));
    rec->setProperty ("host", transport != nullptr ? transport->hostName() : juce::String());
    o->setProperty ("rec", juce::var (rec));

    const auto cs = hub->camStatus();
    auto* cm = new juce::DynamicObject();
    cm->setProperty ("state", cs.state);
    cm->setProperty ("fps", cs.fps);
    cm->setProperty ("label", cs.label);
    cm->setProperty ("error", cs.error);
    cm->setProperty ("tip", cs.tip);
    cm->setProperty ("still", cs.still);
    o->setProperty ("cam", juce::var (cm));

    if (withHands)
    {
        const double t = nowSeconds(), at = useSim ? simAt : hubAt;
        const HandFrame* drawn = (at > 0.0 && t - at < kFresh) ? (useSim ? &simFrame : &hubFrame) : nullptr;
        juce::Array<juce::var> hands;
        if (drawn != nullptr)
            for (int k = 0; k < 2; ++k)
            {
                const auto& h = drawn->hands[k];
                if (! h.visible) continue;
                auto* ho = new juce::DynamicObject();
                ho->setProperty ("h", k == 0 ? "L" : "R");
                ho->setProperty ("p", h.pinched);
                ho->setProperty ("f", h.fisted);
                ho->setProperty ("pcx", h.pcx);
                ho->setProperty ("pcy", h.pcy);
                juce::Array<juce::var> lm;
                for (int j = 0; j < 21; ++j) { lm.add (h.lm[j][0]); lm.add (h.lm[j][1]); }
                ho->setProperty ("lm", lm);
                hands.add (juce::var (ho));
            }
        o->setProperty ("hands", hands);
        o->setProperty ("aspect", drawn != nullptr ? drawn->aspect : 4.0f / 3.0f);
        int seqOut = 0;
        const auto thumb = hub->cameraThumb (seqOut);
        if (thumb.isNotEmpty() && ! useSim) { o->setProperty ("thumb", thumb); o->setProperty ("thumbSeq", seqOut); }
    }

    juce::Array<juce::var> sl;
    for (int i = 0; i < (int) slots.size(); ++i) sl.add (slotToVar (slots[(size_t) i], i, true));
    o->setProperty ("slots", sl);
    o->setProperty ("params", paramList());
    o->setProperty ("maxSlots", kMaxSlots);
    return juce::var (o);
}

void Engine::pageEvent (const juce::String& ev, const juce::var& v)
{
    auto slotAt = [this] (const juce::var& x) -> Slot* { const int i = (int) x["i"]; return (i >= 0 && i < (int) slots.size()) ? &slots[(size_t) i] : nullptr; };

    if (ev == "hands")
    {
        if (! useSim) return;
        simFrame = HandFrame::fromVar (v);
        simFrame.source = 2;
        simAt = nowSeconds();
    }
    else if (ev == "slotConfig")
    {
        if (auto* s = slotAt (v))
        {
            closeGesture (*s); s->activeWas = false; s->st = SlotState {}; s->lastBool = -1;
            s->cfg.applyVar (v["cfg"]);
            s->why.clear();
            save();
        }
    }
    else if (ev == "slotOn")
    {
        if (auto* s = slotAt (v)) { s->cfg.enabled = (bool) v["on"]; if (! s->cfg.enabled) { closeGesture (*s); s->activeWas = false; s->st = SlotState {}; } save(); }
    }
    else if (ev == "slotParam")
    {
        if (auto* s = slotAt (v))
        {
            closeGesture (*s); s->activeWas = false; s->st = SlotState {}; s->lastBool = -1; s->lastSent = -1.0f;
            s->param = v["param"].toString();
            if (auto* p = paramFor (*s)) s->cfg.name = p->getName (14).toUpperCase();
            s->why.clear();
            save();
        }
    }
    else if (ev == "addSlot")
    {
        if ((int) slots.size() >= kMaxSlots) return;
        int lefts = 0, rights = 0;
        for (const auto& s : slots) { if (s.cfg.hand == HandSel::left) ++lefts; else if (s.cfg.hand == HandSel::right) ++rights; }
        Slot s;
        for (auto* prm : apvts.processor.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
            {
                if (dynamic_cast<juce::AudioParameterChoice*> (rp) != nullptr || dynamic_cast<juce::AudioParameterBool*> (rp) != nullptr) continue;
                bool used = false; for (const auto& x : slots) used = used || x.param == rp->paramID;
                if (! used) { s.param = rp->paramID; s.cfg.name = rp->getName (14).toUpperCase(); break; }
            }
        if (s.param.isEmpty()) return;
        s.cfg.enabled = true;
        s.cfg.hand = rights <= lefts ? HandSel::right : HandSel::left;
        slots.push_back (std::move (s));
        save();
    }
    else if (ev == "removeSlot")
    {
        const int i = (int) v["i"];
        if (i >= 0 && i < (int) slots.size()) { closeGesture (slots[(size_t) i]); slots.erase (slots.begin() + i); save(); }
    }
    else if (ev == "reset")
    {
        releaseAll();
        loadManifest();
        save();
    }
    else if (ev == "setLive")
    {
        live = (bool) v["on"];
        if (! live) releaseAll();
        save();
    }
    else if (ev == "arm")
    {
        arm();
    }
    else if (ev == "setSource")
    {
        const auto s = v["v"].toString();
        useSim = s == "sim";
        if (! useSim)
        {
            hub->setSource (s == "camera" ? LinkHub::Source::camera : LinkHub::Source::phone);
            hub->start();
        }
        releaseAll();
        save();
    }
    else if (ev == "rec")
    {
        if (transport != nullptr) transport->record (! transport->hostRecording());
    }
    else if (ev == "newCode")
    {
        hub->start();
        hub->newCode();
    }
    else if (ev == "camera")
    {
        hub->setCameraDevice (v["device"].toString());
    }
    else if (ev == "view")
    {
        hub->setView (v["v"].toString());
    }
    else if (ev == "openCenter")                 // the model lives once: RONE Control is not installed here
    {
        if (! RoneUpdates::launchCenterFor ("RoneControl"))
            juce::URL ("https://roneaudio.com/#center").launchInDefaultBrowser();
    }
}
} // namespace RoneRack
