#include "CrashTest.h"
#include <thread>

namespace CrashTest
{
std::atomic<bool> audioAsked { false };

// One function each, because the guard throttles by address: with one shared crash site, the second
// zone would report nothing for 12 hours. Writing a different number keeps the linker from folding them.
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

RestZone::RestZone (juce::String t, std::function<void()> a)
    : text (std::move (t)), action (std::move (a))
{
    setMouseClickGrabsKeyboardFocus (false);
}

void RestZone::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xffa32020));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
    g.setColour (juce::Colours::white);
    g.setFont (13.0f);
    g.drawText (text, getLocalBounds(), juce::Justification::centred);
}

void RestZone::mouseEnter (const juce::MouseEvent&) { startTimer (700); }

// In the proof the pointer is not really there, so Windows says it left straight away.
void RestZone::mouseExit (const juce::MouseEvent&)
{
    if (juce::SystemStats::getEnvironmentVariable ("RONE_CRASH_TEST", {}) != "hover")
        stopTimer();
}

void RestZone::timerCallback()
{
    stopTimer();
    action();
}

Strip::Strip()
    : ui ("Rest: UI", [] { crashOnMessageThread(); }),
      audio ("Rest: Audio", [] { audioAsked = true; })
{
    addAndMakeVisible (ui);
    addAndMakeVisible (audio);

    // Does nothing itself: pressing it is the crash, inside JUCE, on its way to taking keyboard focus.
    press.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff6a1b9a));
    addAndMakeVisible (press);

    // In a host with no screen the zones fire by themselves, so the crashes can be proven before a person
    // trusts them in a DAW. The crash on our own thread has no zone: it kills any host, a DAW that dies
    // holding MIDI ports can wedge the Windows MIDI service, and it teaches nothing the proof does not.
    const auto asked = juce::SystemStats::getEnvironmentVariable ("RONE_CRASH_TEST", {});

    if (asked == "thread")
        juce::Timer::callAfterDelay (1500, [] { std::thread (crashOnOwnThread).detach(); });
    else if (asked == "ui" || asked == "audio")
        juce::Timer::callAfterDelay (1500, [safe = juce::Component::SafePointer<RestZone> (asked == "ui" ? &ui : &audio)]
        {
            if (safe != nullptr)
                safe->fire();
        });
}

void Strip::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff14161a));
}

void Strip::resized()
{
    auto area = getLocalBounds().reduced (6);
    const auto third = area.getWidth() / 3;

    ui.setBounds (area.removeFromLeft (third).reduced (2, 0));
    audio.setBounds (area.removeFromLeft (third).reduced (2, 0));
    press.setBounds (area.reduced (2, 0));
}
}
