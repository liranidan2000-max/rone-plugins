#include "LinkHub.h"
#include "LinkView.h"
#include "LinkUI.h"
#include "Version.h"
#include <random>

namespace RoneCtl
{
namespace
{
    juce::File pairingFile()  { return controlDataFolder().getChildFile ("pairing.json"); }
    juce::File settingsFile() { return controlDataFolder().getChildFile ("settings.json"); }
    juce::File statusFile()   { return controlDataFolder().getChildFile ("link-status.json"); }   // the owner's real state, for the peers

    // 16 bytes from the OS generator (std::random_device is the system CSPRNG on MSVC and macOS), base64url
    juce::String randomToken()
    {
        std::random_device rd;
        juce::MemoryBlock mb (16);
        for (size_t i = 0; i < 16; i += 4)
        {
            const auto v = (juce::uint32) rd();
            std::memcpy (static_cast<char*> (mb.getData()) + i, &v, 4);
        }
        return juce::Base64::toBase64 (mb.getData(), mb.getSize())
                   .replaceCharacter ('+', '-').replaceCharacter ('/', '_').trimCharactersAtEnd ("=");
    }

    bool validToken (const juce::String& s)
    {
        return s.length() >= 16 && s.length() <= 64 && s.containsOnly ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_");
    }

    double nowSeconds() { return juce::Time::getMillisecondCounterHiRes() * 0.001; }
}

// ============================================================================
// The off-screen window the link view lives in: a tool window (no taskbar button) far off-screen.
// ============================================================================
#if JUCE_WINDOWS
class LinkHub::LinkWindow : public juce::Component
{
public:
    explicit LinkWindow (LinkHub& h) : hub (h)
    {
        setOpaque (true);
        setSize (64, 48);
        addToDesktop (juce::ComponentPeer::windowIsTemporary
                      | juce::ComponentPeer::windowIgnoresKeyPresses
                      | juce::ComponentPeer::windowIgnoresMouseClicks);
        setTopLeftPosition (-20000, -20000);
        setVisible (true);

        const auto folder = controlDataFolder().getChildFile ("link");
        prepError = prepareLinkFolder (folder, juce::String (LinkUI::getLinkHTML()));
        if (auto* peer = getPeer())
            view = std::make_unique<LinkView> (peer->getNativeHandle(), folder,
                                               [this] (const juce::String& n, const juce::var& p) { hub.linkEvent (n, p); });
    }

    void send (const juce::String& event, const juce::var& v) { if (view != nullptr) view->post (event, v); }
    juce::String error() const
    {
        if (prepError.isNotEmpty()) return prepError;
        return view != nullptr ? view->error() : juce::String ("no window for the link");
    }
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colour (0xff14161a)); }
    void resized() override { if (view != nullptr) view->setSize (getWidth(), getHeight()); }

private:
    LinkHub& hub;
    juce::String prepError;
    std::unique_ptr<LinkView> view;
};
#else
// macOS: JUCE's WebView (phone only - the page is event-driven, so a background page's throttled timers
// do not matter; the computer's camera is not offered, a plug-in cannot own it there).
class LinkHub::LinkWindow : public juce::Component
{
public:
    explicit LinkWindow (LinkHub& h)
        : hub (h),
          web (juce::WebBrowserComponent::Options{}
              .withNativeIntegrationEnabled()
              .withResourceProvider ([] (const juce::String& url) -> std::optional<juce::WebBrowserComponent::Resource>
              {
                  if (url == "/" || url.isEmpty() || url == "/index.html")
                  {
                      const juce::String html (LinkUI::getLinkHTML());
                      auto utf8 = html.toUTF8();
                      auto* raw = reinterpret_cast<const std::byte*> (utf8.getAddress());
                      return juce::WebBrowserComponent::Resource { std::vector<std::byte> (raw, raw + utf8.sizeInBytes()), juce::String ("text/html") };
                  }
                  return std::nullopt;
              })
              .withEventListener ("linkReady",  [this] (const juce::var& v) { hub.linkEvent ("linkReady", v); })
              .withEventListener ("hands",      [this] (const juce::var& v) { hub.linkEvent ("hands", v); })
              .withEventListener ("linkStatus", [this] (const juce::var& v) { hub.linkEvent ("linkStatus", v); })
              .withEventListener ("linkLog",    [this] (const juce::var& v) { hub.linkEvent ("linkLog", v); }))
    {
        setOpaque (true);
        addAndMakeVisible (web);
        setSize (48, 48);
        addToDesktop (juce::ComponentPeer::windowIsTemporary
                      | juce::ComponentPeer::windowIgnoresKeyPresses
                      | juce::ComponentPeer::windowIgnoresMouseClicks);
        setTopLeftPosition (-20000, -20000);
        setVisible (true);
        web.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
    }

    void send (const juce::String& event, const juce::var& v) { web.emitEventIfBrowserIsVisible (event, v); }
    juce::String error() const { return {}; }
    void resized() override { web.setBounds (getLocalBounds()); }
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colour (0xff14161a)); }

private:
    LinkHub& hub;
    juce::WebBrowserComponent web;
};
#endif

// ============================================================================
LinkHub::LinkHub()
{
    loadSettings();
}

LinkHub::~LinkHub()
{
    stopTimer();
    window = nullptr;
    peer = nullptr;
    peerServer = nullptr;
    ownerLock = nullptr;
}

bool LinkHub::becomeOwner()
{
    if (ownerLock == nullptr)
        ownerLock = std::make_unique<juce::InterProcessLock> ("RONE_Control_Link_"
                        + juce::String::toHexString (controlDataFolder().getFullPathName().toLowerCase().hashCode64()));
    return ownerLock->enter (0);
}

void LinkHub::startPeer()
{
    if (peer != nullptr) return;
    peer = std::make_unique<HandLinkClient> (kPeerPort);
    peer->onFrame = [this] (const HandFrame& f) { for (auto* c : clients) c->hubHands (f); };
    phone = PhoneStatus {};
    cam = CamStatus {};
    readOwnerStatus();                                   // "peer" only if the owner really has a phone / camera
    logLine ("link owned by another program here - hands come from it");
    startTimerHz (10);
}

// The owner writes what it really has, once a second; a peer mirrors it. Before this a peer always said
// "peer" (= connected), so with a RONE plug-in's rack or a second DAW holding the link and no phone in, the
// plug-in opened on the stage, the PHONE panel closed itself after a second, and nothing ever came (Liran, 25.10).
void LinkHub::writeStatus() const
{
    auto* o = new juce::DynamicObject();
    auto* p = new juce::DynamicObject();
    p->setProperty ("state", phone.state); p->setProperty ("via", phone.via);
    p->setProperty ("fps", phone.fps);     p->setProperty ("rtt", phone.rtt);
    auto* k = new juce::DynamicObject();
    k->setProperty ("state", cam.state);   k->setProperty ("label", cam.label);
    k->setProperty ("still", cam.still);   k->setProperty ("fps", cam.fps);
    o->setProperty ("phone", juce::var (p));
    o->setProperty ("cam", juce::var (k));
    o->setProperty ("source", src == Source::camera ? "camera" : "phone");
    o->setProperty ("at", juce::Time::currentTimeMillis());
    o->setProperty ("core", kCoreProtocol);                    // which Shared/RoneControlRack wrote this (peers of another build)
    statusFile().replaceWithText (juce::JSON::toString (juce::var (o), true));
}

void LinkHub::readOwnerStatus()
{
    // No file (an older owner - a plug-in's rack built before this - never writes one) or a file nobody has
    // touched for 10 s (a stuck owner; the takeover handles a dead one): assume the owner has the hands, as before.
    const auto v = juce::JSON::parse (statusFile());
    const auto age = juce::Time::currentTimeMillis() - (juce::int64) v.getProperty ("at", 0);
    const bool known = v.isObject() && age >= 0 && age < 10000;
    const auto p = v["phone"], k = v["cam"];
    const auto ps = known ? p["state"].toString() : juce::String ("connected");
    const auto ks = known ? k["state"].toString() : juce::String ("running");
    if (ps == "connected") { phone.state = "peer"; phone.via = p["via"].toString(); phone.fps = (float) p["fps"]; phone.rtt = (float) p["rtt"]; }
    else                   { phone.state = ps.isEmpty() ? "waiting" : ps; phone.via = {}; phone.fps = 0.0f; phone.rtt = 0.0f; }
    if (ks == "running" && ! (bool) k["still"]) { cam.state = "peer"; cam.label = k["label"].toString(); cam.fps = (float) k["fps"]; cam.still = false; }
    else                                        { cam.state = ks.isEmpty() ? "starting" : ks; cam.still = (bool) k["still"]; }
}

void LinkHub::addClient (Client* c)     { clients.addIfNotAlreadyThere (c); }
void LinkHub::removeClient (Client* c)  { clients.removeFirstMatchingValue (c); }

bool LinkHub::cameraSupported()
{
   #if JUCE_WINDOWS
    return true;
   #else
    return false;
   #endif
}

bool LinkHub::shouldAutoStart()
{
    if (pairingFile().existsAsFile()) return true;
    const auto j = juce::JSON::parse (settingsFile().loadFileAsString());
    return cameraSupported() && j.isObject() && j["source"].toString() == "camera";
}

void LinkHub::loadSettings()
{
    const auto j = juce::JSON::parse (settingsFile().loadFileAsString());
    if (! j.isObject()) return;
    src = (j["source"].toString() == "camera" && cameraSupported()) ? Source::camera : Source::phone;
    camDevice = j["camera"].toString();
    view = j["view"].toString() == "hand" ? "hand" : "video";
}

void LinkHub::saveSettings() const
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("source", src == Source::camera ? "camera" : "phone");
    o->setProperty ("camera", camDevice);
    o->setProperty ("view", view);
    const auto f = settingsFile();
    f.getParentDirectory().createDirectory();
    f.replaceWithText (juce::JSON::toString (juce::var (o)));
}

void LinkHub::loadPairing (bool fresh)
{
    const auto f = pairingFile();
    if (! fresh)
        if (auto j = juce::JSON::parse (f.loadFileAsString()); j.isObject())
        {
            room = j["room"].toString();
            key  = j["key"].toString();
            if (validToken (room) && validToken (key)) return;
        }
    room = randomToken();
    key  = randomToken();
    auto* o = new juce::DynamicObject();
    o->setProperty ("room", room);
    o->setProperty ("key", key);
    f.getParentDirectory().createDirectory();
    f.replaceWithText (juce::JSON::toString (juce::var (o)));
}

juce::String LinkHub::phoneUrl() const
{
    return room.isEmpty() ? juce::String() : kLabOrigin + "/p/#" + room + "." + key;
}

juce::String LinkHub::linkError() const
{
    return window != nullptr ? window->error() : juce::String();
}

void LinkHub::start()
{
    if (window != nullptr || peer != nullptr) return;
    if (room.isEmpty()) loadPairing (false);
    if (! becomeOwner()) { startPeer(); return; }
    peerServer = std::make_unique<HandLinkServer>();
    peerServer->start (kPeerPort);
    phone.state = "waiting";
    if (src == Source::camera) cam.state = "starting";
    linkPageReady = false;
    window = std::make_unique<LinkWindow> (*this);
    logLine (juce::String ("link started - source ") + (src == Source::camera ? "camera" : "phone")
             + (window->error().isNotEmpty() ? " - " + window->error() : juce::String()));
    writeStatus();
    startTimerHz (10);
}

void LinkHub::newCode()
{
    if (peer != nullptr) return;                     // the owner's room: a new code is made where the link runs
    loadPairing (true);
    phone = PhoneStatus {};
    phone.state = window != nullptr ? "waiting" : "off";
    sendPair();
}

void LinkHub::setSource (Source s)
{
    if (s == Source::camera && ! cameraSupported()) return;
    if (s == src && (s != Source::camera || window != nullptr)) return;
    src = s;
    saveSettings();
    if (src == Source::camera)
    {
        cam = CamStatus {};
        cam.state = "starting";
        start();                                   // the camera lives in the link view
    }
    sendCamera();
}

void LinkHub::setCameraDevice (const juce::String& deviceId)
{
    camDevice = deviceId;
    saveSettings();
    if (src != Source::camera) return;
    cam.state = "starting";
    cam.error.clear();
    sendCamera();                                  // also the TRY AGAIN of a busy or missing camera
}

void LinkHub::wantVideo (const void* editor, bool on)
{
    const auto before = ! videoWanters.empty();
    if (on) videoWanters.insert (editor); else videoWanters.erase (editor);
    if (before != ! videoWanters.empty()) sendVideo();
}

void LinkHub::setView (const juce::String& v)
{
    view = v == "hand" ? "hand" : "video";
    saveSettings();
}

void LinkHub::sendVideo()
{
    if (window == nullptr || ! linkPageReady) return;          // linkReady sends it
    auto* o = new juce::DynamicObject();
    o->setProperty ("on", ! videoWanters.empty());
    window->send ("video", juce::var (o));
}

// The handshakes of the last sessions, a line each ("my phone will not connect" -> this file says why).
// Kinds of candidates and timings only: never an address, a key or a hand. Small on purpose.
void LinkHub::logLine (const juce::String& text)
{
    const auto f = controlDataFolder().getChildFile ("link.log");
    if (f.getSize() > 200 * 1024) f.deleteFile();
    f.getParentDirectory().createDirectory();
    f.appendText (juce::Time::getCurrentTime().formatted ("%Y-%m-%d %H:%M:%S  ") + text.substring (0, 400) + "\n", false, false, "\n");
}

void LinkHub::sendPair()
{
    if (window == nullptr || ! linkPageReady) return;          // linkReady sends it
    auto* o = new juce::DynamicObject();
    o->setProperty ("room", room);
    o->setProperty ("key", key);
    o->setProperty ("lab", kLabOrigin);
    window->send ("pair", juce::var (o));
}

void LinkHub::sendCamera()
{
    if (src != Source::camera)
        cam = CamStatus {};
    thumb.clear();                                  // the other source's picture is not this one's
    if (window == nullptr || ! linkPageReady) return;          // linkReady sends it
    auto* o = new juce::DynamicObject();
    o->setProperty ("on", src == Source::camera && cameraSupported());
    o->setProperty ("device", camDevice);
    o->setProperty ("hands", juce::jmax (1, lastHandsNeeded));
    window->send ("camera", juce::var (o));
}

void LinkHub::dispatch (const juce::var& payload, int source)
{
    auto f = HandFrame::fromVar (payload);
    f.source = source;
    f.seq = ++seq;
    f.engineTime = nowSeconds();
    for (auto* c : clients) c->hubHands (f);
    if (peerServer != nullptr) peerServer->broadcast (f);       // RONE Control in the other DAWs
}

void LinkHub::linkEvent (const juce::String& name, const juce::var& v)
{
    if (name == "linkReady")
    {
        linkPageReady = true;
        lastHandsNeeded = 0;                            // re-send the config to the next phone
        sendPair();
        sendCamera();
        sendVideo();
    }
    else if (name == "hands")
    {
        if (src == Source::phone) dispatch (v, 3);
    }
    else if (name == "camHands")
    {
        if (src == Source::camera) dispatch (v, 1);
    }
    else if (name == "linkStatus")
    {
        phone.state = v["state"].toString();
        phone.via = v["via"].toString();
        phone.fps = (float) (double) v["fps"];
        phone.rtt = (float) (double) v["rtt"];
    }
    else if (name == "linkLog")
    {
        logLine (v.toString());
    }
    else if (name == "camStatus")
    {
        if (src != Source::camera) return;
        cam.state   = v["state"].toString();
        cam.fps     = (float) (double) v["fps"];
        cam.ms      = (float) (double) v["ms"];
        cam.label   = v["label"].toString();
        cam.device  = v["device"].toString();
        cam.tip     = v["tip"].toString();
        cam.error   = v["error"].toString();
        cam.devices = v["devices"];
        cam.still   = (bool) v["still"];                // a camera sending the same frame (a virtual one, no app)
    }
    else if (name == "camThumb")
    {
        // { src: "camera" | "phone", data }: only the chosen source's picture is kept
        const auto from = v.isObject() ? v["src"].toString() : juce::String ("camera");
        if (from != (src == Source::camera ? "camera" : "phone")) return;
        const auto s = v.isObject() ? v["data"].toString() : v.toString();
        if (s.startsWith ("data:image/jpeg;base64,") && s.length() < 400000)
        {
            thumb = s;
            ++thumbSeq;
        }
    }
}

void LinkHub::timerCallback()
{
    if (peer != nullptr)
    {
        // the owner went away (its DAW closed): this process takes the link over
        if (++ticks % 20 == 0 && becomeOwner())
        {
            peer = nullptr;
            ownerLock->exit();                              // start() takes it again
            phone = PhoneStatus {};
            cam = CamStatus {};
            start();
            return;
        }
        if (ticks % 10 == 0) readOwnerStatus();
        return;
    }
    if (window == nullptr) return;
    ++ticks;
    if (ticks % 10 == 0) writeStatus();
    if (! linkPageReady) return;

    // The phone's screen follows the instance that was played last.
    Client* best = nullptr;
    for (auto* c : clients)
        if (best == nullptr || c->lastActive() > best->lastActive()) best = c;
    auto* fb = new juce::DynamicObject();
    fb->setProperty ("type", "fb");
    fb->setProperty ("slots", best != nullptr ? best->hubFeedback() : juce::var (juce::Array<juce::var>()));
    window->send ("fb", juce::var (fb));

    // One hand unless a slot needs two: fewer false second hands, and a faster model.
    int need = 1;
    for (auto* c : clients) need = juce::jmax (need, c->handsNeeded());
    if (need != lastHandsNeeded || ticks % 50 == 0)
    {
        lastHandsNeeded = need;
        auto* cfg = new juce::DynamicObject();
        cfg->setProperty ("type", "cfg");
        cfg->setProperty ("hands", need);
        window->send ("cfg", juce::var (cfg));
    }
    if (ticks % 10 == 0)
        window->send ("ping", juce::var());
}
} // namespace RoneCtl
