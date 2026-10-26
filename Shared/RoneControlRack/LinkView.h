#pragma once

#include <JuceHeader.h>

#if JUCE_WINDOWS
namespace RoneCtl
{
// ============================================================================
// The link's own WebView2 (Windows) - built directly, not through JUCE's WebBrowserComponent, because
// the link needs what JUCE cannot give it:
//   - its own browser arguments: a page nobody looks at (an off-screen window) must not be throttled
//     or treated as occluded, or the camera and the phone link would run at 1 frame a second;
//   - the camera for OUR page only, answered in code (a permission prompt in an off-screen window
//     could never be clicked), everything else refused;
//   - its files from a folder the plugin writes (link.html and the hand model it carries inside).
// Its own user data folder, so its browser process and arguments never meet the editor's.
// Message thread only. Page -> C++: chrome.webview.postMessage({ n, p }); C++ -> page: post (n, p).
// ============================================================================
class LinkView
{
public:
    using Handler = std::function<void (const juce::String& name, const juce::var& payload)>;

    static constexpr const char* kOrigin = "https://rone-control.example";

    LinkView (void* parentHwnd, const juce::File& folder, Handler onMessage);
    ~LinkView();

    void post (const juce::String& name, const juce::var& payload);
    void setSize (int width, int height);
    juce::String error() const;                     // why the view could not be built ("" = fine)

private:
    struct Impl;
    std::shared_ptr<Impl> impl;

    JUCE_DECLARE_NON_COPYABLE (LinkView)
};

// Writes link.html and the hand model (carried inside RONE Control as resources) into `folder`.
// Files already there with the right size are left alone. A module without the resources (a plugin's rack)
// accepts the files RONE Control wrote; when they are absent it returns kNoModelError (the camera cannot start,
// the phone still can - the rack page shows the "install RONE Control" card on this text). "" = all good.
inline const char* const kNoModelError = "no hand model here - install RONE Control from the Center";
juce::String prepareLinkFolder (const juce::File& folder, const juce::String& linkHtml);
} // namespace RoneCtl
#endif
