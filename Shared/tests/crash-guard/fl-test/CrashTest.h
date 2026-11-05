// Ways to crash on purpose, for the crash guard's test inside a real DAW. Test builds only:
// build.ps1 copies this into a throwaway copy of a plugin, and no plugin in the monorepo includes it.
#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <functional>

namespace CrashTest
{
extern std::atomic<bool> audioAsked;

void crashOnAudioThread();

// the first thing processBlock does
inline void checkInAudio()
{
    if (audioAsked.load (std::memory_order_relaxed))
        crashOnAudioThread();
}

// Fires when the mouse rests on it, so nothing is pressed. A press is what the first FL test tripped on:
// JUCE 8.0.4 takes keyboard focus on mouse down, which FL Studio's message thread does not survive, so
// the crash that was meant to be tested never ran. See README.md.
class RestZone : public juce::Component,
                 private juce::Timer
{
public:
    RestZone (juce::String text, std::function<void()> action);

    void fire() { action(); }

    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    juce::String text;
    std::function<void()> action;
};

class Strip : public juce::Component
{
public:
    static constexpr int height = 40;

    Strip();
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    RestZone ui, audio;
    juce::TextButton press { "Press" };
};
}
