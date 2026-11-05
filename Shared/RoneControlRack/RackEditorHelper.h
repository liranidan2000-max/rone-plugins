#pragma once

// ============================================================================
// RoneControlRack - EditorHelper: what a RONE editor needs to carry the rack.
//
//   RoneRack::EditorHelper rackUi;                       // declared BEFORE the WebView
//   rackUi (*this, p.rack, kBaseW, kBaseH)
//   webView (rackUi.addTo (updatePrompt.addTo (Options{} ...)))   // the + handle's script + its event
//   rackUi.attach (webView);                              // constructor body
//   rackUi.onToggle = [this] { applyResizeLimits(); };    // limits follow the current ratio
//   resized():        rackUi.layout (getLocalBounds());   // plugin view left, rack right
//   requestResize:    h = rackUi.heightForWidth (w);      // keeps (W + 280) : H while the rack is open
//
// The + is drawn INSIDE the plugin's page by a user script (a JUCE component cannot sit over a native
// WebView2 window): a 3 px hairline on the right edge, hover widens it, click opens. No page changes.
//
// Opening: the editor asks the host for a wider window (setSize). If the host does not give it (the
// width is unchanged a moment later) the rack falls back to SHRINK: the plugin's view narrows and the
// rack takes at most 38 % of the window.
// ============================================================================

#include <JuceHeader.h>
#include "RackEngine.h"
#include "RackView.h"

namespace RoneRack
{
class EditorHelper : private juce::Timer
{
public:
    static constexpr int kRackW = 280;                     // design px, at the plugin's design height

    EditorHelper (juce::AudioProcessorEditor& editor, Engine& engine, int baseW, int baseH);
    ~EditorHelper() override;

    juce::WebBrowserComponent::Options addTo (juce::WebBrowserComponent::Options options);
    void attach (juce::WebBrowserComponent& pluginView);

    void layout (juce::Rectangle<int> bounds);
    int  heightForWidth (int contentWidth) const;          // content height, no title bar
    double widthFactor() const;                            // 1, or (W + 280) / W while the rack grows the window
    bool isOpen() const noexcept { return open; }
    bool grows() const noexcept { return open && ! shrink; }

    void setOpen (bool shouldOpen);
    std::function<void()> onToggle;                        // the window's min/max follow: called BEFORE the window widens, AFTER it narrows
    void setTitleBarHeight (int px) { tb = px; }           // standalone (Windows): the page's own title bar is not plugin height

private:
    void timerCallback() override;
    void tellPage();
    int rackPx (int height) const { return juce::roundToInt (kRackW * (double) juce::jmax (1, height - tb) / (double) H); }

    juce::AudioProcessorEditor& ed;
    Engine& eng;
    juce::WebBrowserComponent* plugin = nullptr;
    std::unique_ptr<View> view;
    const int W, H;
    int tb = 0;
    bool open = false, shrink = false;
    bool pendingReopen = false;                            // the window reopened with the rack open last time
    int expectW = 0;
    int waitTicks = 0;                                     // 120 ms checks while the host answers the wider window
    bool lateGrantQueued = false;

    JUCE_DECLARE_NON_COPYABLE (EditorHelper)
};
} // namespace RoneRack
