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
// ============================================================================

#include <JuceHeader.h>
#include <vector>
#include "ControlCore.h"
#include "LinkHub.h"
#include "RackTransport.h"

namespace RoneRack
{
class Engine : private juce::Timer,
               private RoneCtl::LinkHub::Client
{
public:
    Engine (juce::AudioProcessorValueTreeState& apvts, const juce::String& pluginId, const juce::String& manifestJson);
    ~Engine() override;

    const juce::String& pluginId() const noexcept { return id; }

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

    juce::RangedAudioParameter* paramFor (const Slot& s) const;
    Slot slotFromVar (const juce::var& v) const;
    juce::var slotToVar (const Slot& s, int i, bool forPage) const;
    juce::var paramList() const;

    juce::AudioProcessorValueTreeState& apvts;
    juce::String id, name, accent;
    juce::var manifest;
    juce::SharedResourcePointer<RoneCtl::LinkHub> hub;
    std::vector<Slot> slots;

    RoneCtl::HandFrame hubFrame, simFrame;
    double hubAt = -1.0, simAt = -1.0, lastT = 0.0, lastHeldAt = 0.0;
    bool useSim = false, live = true, open = false, armed = false;
    bool rememberedOpen = false;                           // the saved open state, remembered only (see DESIGN.md open questions)
    bool reopen = false;
    Transport* transport = nullptr;
    juce::String myId;
    int ticks = 0;

    JUCE_DECLARE_NON_COPYABLE (Engine)
};
} // namespace RoneRack
