#pragma once

// ============================================================================
// RoneControlRack - Engine: RONE Control's hands driving THIS plugin's own parameters.
//
// One per plugin instance, owned by the processor (so the hands keep working while the editor is closed).
// The hands come from RONE Control's LinkHub (one per DAW process; one link per computer: the phone or the
// computer's camera, or a peer of the process that owns it). Each slot binds a hand move to one parameter
// of the APVTS through RONE Control's own SlotMapper, and writes it like a mouse drag: inside a change
// gesture, setValueNotifyingHost. No MIDI, no linking, nothing DAW-specific.
//
// The shipped map comes from the plugin's rack.json ("rack": [...]); the user's edits are kept per plugin
// on this computer (%APPDATA%\RonePlugins\<id>\rack-slots.json), never in the project.
// ARMED: with several RONE plugins open the hands drive the one armed last. Armed state is a small file
// in RONE Control's data folder, so it is one truth across every plugin binary and every DAW process.
// LITE (Liran, 26.10.2026, Rack-first): slot A is free in every plugin; slots B.. run only with RONE ALL ACCESS
// (BundleLicenseChecker::isBundleLicensed, or the remote open mode). Locked slots are shown, never stepped.
// ============================================================================

#include <JuceHeader.h>
#include <vector>
#include <atomic>
#include "ControlCore.h"
#include "LinkHub.h"
#include "RackTransport.h"

namespace RoneRack
{
class Engine : private juce::Timer,
               private RoneCtl::LinkHub::Client,
               private juce::ValueTree::Listener      // a project load (replaceState) brings this instance's map back
{
public:
    Engine (juce::AudioProcessorValueTreeState& apvts, const juce::String& pluginId, const juce::String& manifestJson);
    ~Engine() override;

    const juce::String& pluginId() const noexcept { return id; }
    const juce::String& accentColour() const noexcept { return accent; }   // the plugin's own neon (rack.json "accent")

    // The rack was opened in an editor: the link starts (lazily, once) and the slots run.
    void setOpen (bool shouldOpen);
    bool isOpen() const noexcept { return open; }
    bool wasOpenLastTime() const noexcept { return rememberedOpen; }
    // The plugin window closed with the rack open: the rack closes (a closed window never moves a knob) and
    // remembers to come back when the window opens again (per instance, this session).
    void editorClosed();
    bool takeReopen() noexcept { const bool r = reopen; reopen = false; return r; }
    void arm();
    void disarm();                                                        // nobody: an open rack takes over
    bool isArmed() const noexcept { return armed; }
    void attachTransport (Transport& t) noexcept { transport = &t; }     // the REC button

    juce::var uiState (bool withHands);                                   // 30 Hz to the rack page
    void pageEvent (const juce::String& name, const juce::var& payload);  // the rack page's events

private:
    struct Slot
    {
        juce::String param, why;
        RoneCtl::SlotConfig cfg;        // cfg.enabled = the ON switch
        RoneCtl::SlotState st;
        bool activeWas = false, gestureOpen = false;
        int lastBool = -1;
        float lastSent = -1.0f;
    };

    void timerCallback() override;      // 60 Hz: step, freshness, the armed file
    void stepAll (double t);
    void closeGesture (Slot& s);
    void releaseAll();

    void hubHands (const RoneCtl::HandFrame& f) override;
    juce::var hubFeedback() override;
    int  handsNeeded() const override;
    double lastActive() const override { return lastHeldAt; }

    void loadManifest();
    void loadSaved();
    void save() const;
    juce::File savedFile() const;
    static juce::File armedFile();
    void checkArmed();
    void refreshLite();                                                   // every ~2 s: ALL ACCESS or not
    bool locked (int slotIndex) const noexcept { return lite && slotIndex > 0; }

    juce::RangedAudioParameter* paramFor (const Slot& s) const;
    Slot slotFromVar (const juce::var& v) const;
    juce::var slotToVar (const Slot& s, int i, bool forPage) const;
    juce::var paramList() const;
    // One move per knob (Liran, beta round 8.11): two ON slots never share HOLD WITH + MOVE, whatever the hand -
    // one hand in the picture is read as left or right by where it is, so "same move, other hand" moved the wrong knob.
    static bool sameMove (const RoneCtl::SlotConfig& a, const RoneCtl::SlotConfig& b) noexcept;
    int clashOf (int slotIndex) const noexcept;            // another ON slot with the same move, or -1
    void settleClashes();                                   // a later ON slot that repeats a move is switched off
    static constexpr int kMapVersion = 2;                   // 2 = beta round 8.11 maps (one move per knob, any hand)

    // MAP PRESETS (beta round 8.11, the presets mockup AdA8MXTWHTtJDa8PWfLFNp): the user's own maps per plugin on this
    // computer (%APPDATA%/RonePlugins/<id>/rack-maps/<name>.json, shared by every DAW), RONE DEFAULT (= rack.json) always
    // first and never overwritten, a star = the map every NEW instance opens with. The instance's own map rides in the
    // project (a property of the APVTS state), so a saved project reopens with the map it was saved with.
    juce::File mapsDir() const;
    juce::StringArray mapNames() const;                     // the user's maps, sorted (RONE DEFAULT is not in it)
    juce::String defaultMapName() const;                    // "" = RONE DEFAULT
    juce::File mapFile (const juce::String& mapName) const;
    bool loadMap (const juce::String& mapName);             // "" = RONE DEFAULT; false = no such map (nothing changes)
    void slotsFromArray (const juce::var& arr);
    juce::var slotsToArray() const;
    void writeProjectMap();                                 // the instance's map -> apvts.state (saved with the project)
    bool readProjectMap();                                  // apvts.state -> the instance's map; false = the project has none
    void edited() { mapDirty = true; }
    void valueTreeRedirected (juce::ValueTree&) override { stateSwapped = true; }   // any thread: the timer takes it
    juce::String mapName;                                   // "" = RONE DEFAULT
    bool mapDirty = false;
    std::atomic<bool> stateSwapped { false };
    juce::StringArray mapNamesCache;                        // read from disk every 2 s / after a map event, not 30 times a second
    juce::String mapDefCache;
    void refreshMapCache();

    juce::AudioProcessorValueTreeState& apvts;
    juce::String id, name, accent;
    juce::var manifest;
    juce::SharedResourcePointer<RoneCtl::LinkHub> hub;
    std::vector<Slot> slots;

    RoneCtl::HandFrame hubFrame, simFrame;
    double hubAt = -1.0, simAt = -1.0, lastT = 0.0, lastHeldAt = 0.0;
    bool useSim = false, live = true, open = false, armed = false;
    bool lite = false;                                     // true = only slot A runs (no ALL ACCESS here)
    bool rememberedOpen = false;                          // the saved open state, remembered only (see DESIGN.md open questions)
    bool reopen = false;
    Transport* transport = nullptr;
    juce::String myId;
    int ticks = 0;

    JUCE_DECLARE_NON_COPYABLE (Engine)
};
} // namespace RoneRack
