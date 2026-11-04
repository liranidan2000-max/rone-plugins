// Ways to crash on purpose, for the crash guard's test inside a real DAW. Test builds only:
// build.ps1 copies this into a throwaway copy of a plugin, and no plugin in the monorepo includes it.
#pragma once

#include <JuceHeader.h>
#include <atomic>

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

class Strip : public juce::Component
{
public:
    static constexpr int height = 40;

    Strip();
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::TextButton ui { "UI crash" }, audio { "Audio crash" };
};
}
