#pragma once

#include <JuceHeader.h>
#include <set>
#include "ControlCore.h"
#include "HandLink.h"

// ============================================================================
// The link: where the hands come from. ONE per DAW process, shared by every RONE Control instance
// (juce::SharedResourcePointer). A small off-screen WebView runs link.html, which does both:
//   PHONE  - the pairing (the lab Worker relays the handshake) and the WebRTC DataChannel the phone
//            streams its hands into;
//   CAMERA - the computer's own camera, with the hand model the plugin carries inside (Windows).
// Which one feeds the slots is chosen in the plugin and remembered per computer. It lives with the
// plugin, not with a window, so the hands keep coming when the plugin window is closed.
//
// ONE link per computer, too: the phone's room takes one engine (a new one replaces the old) and a camera serves
// one program. With RONE Control in two processes (FL and Live open together, Bitwig running each plug-in in its
// own process) the first to start the link owns it (an inter-process lock) and sends every hand frame to the
// others on localhost (kPeerPort); they take their hands from it ("peer") and take over when it goes away.
// ============================================================================
namespace RoneCtl
{
inline const juce::String kLabOrigin { "https://rone-control-lab.ronemusics.workers.dev" };

class LinkHub : private juce::Timer
{
public:
    struct Client
    {
        virtual ~Client() = default;
        virtual void hubHands (const HandFrame& f) = 0;       // message thread, as each frame lands
        virtual juce::var hubFeedback() = 0;                   // [{l, name, out, held}] for the phone's screen
        virtual int  handsNeeded() const = 0;                  // 1, or 2 when a slot needs both hands
        virtual double lastActive() const = 0;                 // when one of its slots was last held
    };

    enum class Source { phone, camera };

    LinkHub();
    ~LinkHub() override;

    void addClient (Client* c);
    void removeClient (Client* c);

    void start();                                              // idempotent: pairing + the link view (or a peer)
    bool isStarted() const noexcept { return window != nullptr || peer != nullptr; }
    bool isPeer() const noexcept { return peer != nullptr; }   // another process owns the link: hands come from it
    void newCode();                                            // a fresh room + key: the old phone is cut off
    juce::String phoneUrl() const;                             // what the QR opens

    void setSource (Source s);                                 // remembered per computer
    Source source() const noexcept { return src; }
    void setCameraDevice (const juce::String& deviceId);       // "" = the default camera; also a retry
    static bool cameraSupported();                             // Windows (a macOS plug-in cannot own the camera)
    static bool shouldAutoStart();                             // a phone was paired here, or CAMERA is the source

    struct PhoneStatus { juce::String state { "off" }, via; float fps = 0.0f, rtt = 0.0f; };   // via: "direct" | "relay"
    struct CamStatus   { juce::String state { "off" }, label, device, tip, error; float fps = 0.0f, ms = 0.0f; juce::var devices; bool still = false; };
    PhoneStatus phoneStatus() const noexcept { return phone; }
    CamStatus camStatus() const noexcept { return cam; }
    juce::String cameraThumb (int& seqOut) const { seqOut = thumbSeq; return thumb; }   // a JPEG data URL

    // VIDEO view: the picture of the player - the computer's camera, or the phone's camera streamed straight
    // to this computer (WebRTC, never through a server) - only while an open editor shows it.
    void wantVideo (const void* editor, bool on);
    void setView (const juce::String& v);                      // "video" | "hand", remembered per computer
    juce::String getView() const { return view; }
    juce::String linkError() const;

private:
    class LinkWindow;
    friend class LinkWindow;

    void timerCallback() override;                             // 10 Hz: feedback, ping, how many hands
    void linkEvent (const juce::String& name, const juce::var& v);
    void loadPairing (bool fresh);
    void loadSettings();
    void saveSettings() const;
    void sendPair();
    void sendCamera();
    void dispatch (const juce::var& payload, int source);
    void logLine (const juce::String& text);                   // link.log: the last sessions' handshakes, for support
    void sendVideo();

    bool becomeOwner();                                        // the inter-process lock (false: another process has it)
    void startPeer();
    void writeStatus() const;                                  // owner, 1 Hz: link-status.json = the real phone / camera state
    void readOwnerStatus();                                    // peer, 1 Hz: mirror it ("peer" only while the owner really has hands)

    std::unique_ptr<LinkWindow> window;
    std::unique_ptr<juce::InterProcessLock> ownerLock;
    juce::String claimToken;                                   // this hub's claim on the link inside the process
    std::unique_ptr<HandLinkServer> peerServer;                // owner: the hands go out to the other processes
    std::unique_ptr<HandLinkClient> peer;                      // peer: the hands come in from the owner
    juce::Array<Client*> clients;
    juce::String room, key, camDevice;
    Source src = Source::phone;
    PhoneStatus phone;
    CamStatus cam;
    juce::String thumb;
    int thumbSeq = 0;
    std::set<const void*> videoWanters;
    juce::String view { "video" };
    int lastHandsNeeded = 0, ticks = 0;
    // JUCE 8.0.4 builds WebView2s one at a time, and a view sent a script before it exists can be left
    // behind for good when another view is being built at that moment. So the link page gets nothing
    // until it says linkReady (the editor does the same with its page).
    bool linkPageReady = false;
    juce::uint32 seq = 0;

    JUCE_DECLARE_NON_COPYABLE (LinkHub)
};
} // namespace RoneCtl
