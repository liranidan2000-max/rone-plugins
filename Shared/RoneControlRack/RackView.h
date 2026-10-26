#pragma once

// ============================================================================
// RoneControlRack - RackView: the rack beside the plugin. A second WebBrowserComponent showing the rack
// page (ui/rack.html, in the plugin's BinaryData), talking to the Engine. Built only when the rack is
// first opened (JUCE 8.0.4 builds WebView2s one at a time: never in the same tick as the plugin's own
// view), and nothing is sent to it before the page says rackReady.
// ============================================================================

#include <JuceHeader.h>
#include "RackEngine.h"

namespace RoneRack
{
class View : public juce::Component,
             private juce::Timer
{
public:
    View (Engine& engine, const juce::String& pluginId, std::function<void()> onCollapse);
    ~View() override;

    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override;
    static std::optional<juce::WebBrowserComponent::Resource> resource (const juce::String& url);

    Engine& engine;
    std::function<void()> collapse;
    bool pageReady = false;
    juce::WebBrowserComponent web;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (View)
};
} // namespace RoneRack
