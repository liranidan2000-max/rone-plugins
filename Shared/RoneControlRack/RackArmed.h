#pragma once

// ============================================================================
// RoneControlRack - ARMED: who gets the hands right now.
//
// One camera (or phone), several RONE plugins open: the hands drive ONE of them - the one armed last (the rack
// opened last, its ARMED pill clicked, or RONE Control's own window opened). The truth is a small file in RONE
// Control's data folder, so it is one across plugin binaries and DAW processes; everyone polls it at ~6 Hz.
//   { "id": "<uuid of the instance>", "plugin": "RoneThrow" | "RoneControl" | ..., "at": ms }
//   id "" = nobody: an open rack takes over; RONE Control works as it always did.
//
// RONE Control itself (Liran, 26.10.2026: "only what is open in front of me gets the hands"): it yields while a
// rack of ANOTHER plugin is armed, and arms itself when its window opens. Two RONE Control instances never yield
// to each other (a hidden instance driving DAW knobs through MIDI is RONE Control's everyday use).
// ============================================================================

#include <JuceHeader.h>
#include "ControlCore.h"

namespace RoneRack::Armed
{
    struct Owner { juce::String id, plugin; };

    inline juce::File file() { return RoneCtl::controlDataFolder().getChildFile ("rack-armed.json"); }

    inline Owner read()
    {
        Owner o;
        const auto j = juce::JSON::parse (file().loadFileAsString());
        if (j.isObject()) { o.id = j["id"].toString(); o.plugin = j["plugin"].toString(); }
        return o;
    }

    inline void write (const juce::String& id, const juce::String& plugin)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("id", id);
        o->setProperty ("plugin", plugin);
        o->setProperty ("at", juce::Time::currentTimeMillis());
        const auto f = file();
        f.getParentDirectory().createDirectory();
        f.replaceWithText (juce::JSON::toString (juce::var (o)));
    }

    inline void take (const juce::String& id, const juce::String& plugin) { write (id, plugin); }
    inline void releaseAll()                                              { write ({}, {}); }          // nobody
    inline void release (const juce::String& id)                          { if (read().id == id) releaseAll(); }   // only what is ours
}
