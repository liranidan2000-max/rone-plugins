#pragma once

// ============================================================================
// RoneControlRack - Transport: the rack's REC button. Liran (25.10): "open the plugin, open its control, press
// REC and start lifting the hand" - the fastest workflow there is.
//
// What can start the DAW recording from inside a plugin, host by host:
//   REAPER      directly: REAPER hands every VST3 its API (IReaperHostApplication) - Main_OnCommand 1013
//               (Transport: Record), GetPlayState tells whether it records. Nothing to install.
//   FL Studio, Ableton Live, Bitwig   through RONE Control's own script in that DAW (the LINK channel). The rack
//               writes a command file {seq, op: "record", on, at}; the script acts once per (seq, at) while it is
//               fresh (5 s) and writes a state file {recording, playing, host, at, ack} about every second (at once
//               when something changed), which lights the button and proves the script is there (fresh = 3 s).
//               Live and Bitwig: %APPDATA%\RonePlugins\Control\transport\cmd.json / state.json, the script deletes
//               the command. FL: FL's Python sandbox lets a script write only in its own folder and never delete,
//               so both files sit in FL's Hardware\RONE Control folder (transport_cmd.json / transport_state.json)
//               and the rack deletes the command once its "at" comes back as ack, or when it is 5 s old.
//               Until the script is running the button is dimmed with the reason.
//   Standalone, Cubase, Studio One, Logic   no path yet: the button is dimmed with the reason.
// JUCE's AudioPlayHead::transportRecord exists but no JUCE wrapper (VST3 / AU / Standalone) implements it.
//
// The processor owns one, forwards VST3ClientExtensions::setIHostApplication to attachHost() and, in processBlock,
// the play head's isRecording to setHostRecording(). Everything else is message thread.
// ============================================================================

#include <JuceHeader.h>

namespace Steinberg { class FUnknown; }

namespace RoneRack
{
class Transport
{
public:
    Transport() = default;
    ~Transport();

    void attachHost (Steinberg::FUnknown* hostApplication);        // VST3 only; a no-op outside REAPER
    void setStandalone (bool b) noexcept { standalone = b; }
    void setHostRecording (bool b) noexcept { recording.store (b, std::memory_order_relaxed); }   // audio thread
    bool hostRecording() const;

    bool canRecord() const;                                         // a path exists in this host, and it answers
    juce::String how() const;                                       // "reaper" | "script" | ""
    juce::String why() const;                                       // when it cannot: the reason, for the button
    juce::String hostName() const;
    bool record (bool on);                                          // message thread; true = asked the host

    juce::File commandFile() const;                                 // the script channel, for this host
    juce::File stateFile() const;

private:
    struct Api
    {
        bool ok = false;
        void (*Main_OnCommand) (int, int) = nullptr;
        int  (*GetPlayState)() = nullptr;
    };
    struct ScriptState                                              // the last state file read
    {
        double readAt = 0.0;                                        // ms counter; read at most every 250 ms
        bool fresh = false, recording = false, playing = false;
        juce::int64 ack = 0;
    };
    void poll() const;                                              // reads the state file, deletes a done command
    static juce::File flScriptFolder();                             // FL's Hardware\RONE Control (as RONE Control finds it)

    void* host = nullptr;
    Api api;
    bool standalone = false;
    std::atomic<bool> recording { false };
    int seq = 0;
    mutable juce::int64 cmdAt = 0;                                  // the command waiting for the script, 0 = none
    mutable ScriptState st;

    JUCE_DECLARE_NON_COPYABLE (Transport)
};
} // namespace RoneRack
