#include "CrashTest.h"
#include <thread>

namespace CrashTest
{
std::atomic<bool> audioAsked { false };

// One function each, because the guard throttles by address: with one shared crash site, the second
// button would report nothing for 12 hours. Writing a different number keeps the linker from folding them.
__declspec(noinline) static void crashOnMessageThread()
{
    volatile int* nowhere = nullptr;
    *nowhere = 1;
}

__declspec(noinline) void crashOnAudioThread()
{
    volatile int* nowhere = nullptr;
    *nowhere = 2;
}

__declspec(noinline) static void crashOnOwnThread()
{
    volatile int* nowhere = nullptr;
    *nowhere = 3;
}

Strip::Strip()
{
    for (auto* b : { &ui, &audio })
    {
        b->setColour (juce::TextButton::buttonColourId, juce::Colour (0xffa32020));
        addAndMakeVisible (*b);
    }

    ui.onClick    = [] { crashOnMessageThread(); };
    audio.onClick = [] { audioAsked = true; };

    // In a host with no screen a button presses itself, so the crashes can be proven before a person
    // trusts them in a DAW. The crash on our own thread has no button: it kills any host, a DAW that dies
    // holding MIDI ports can wedge the Windows MIDI service, and it teaches nothing the proof does not.
    const auto asked = juce::SystemStats::getEnvironmentVariable ("RONE_CRASH_TEST", {});

    if (asked == "thread")
        juce::Timer::callAfterDelay (1500, [] { std::thread (crashOnOwnThread).detach(); });
    else if (asked == "ui" || asked == "audio")
        juce::Timer::callAfterDelay (1500, [safe = juce::Component::SafePointer<juce::Button> (asked == "ui" ? &ui : &audio)]
        {
            if (safe != nullptr)
                safe->triggerClick();
        });
}

void Strip::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff14161a));
}

void Strip::resized()
{
    auto area = getLocalBounds().reduced (6);

    ui.setBounds (area.removeFromLeft (area.getWidth() / 2).reduced (2, 0));
    audio.setBounds (area.reduced (2, 0));
}
}
