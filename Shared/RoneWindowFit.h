#pragma once

// ============================================================================
// RoneWindowFit - the WebView fills the plug-in window in every host, on every
// screen. Rides along with RoneUpdatePrompt (addTo / attachBrowser), so every
// RONE WebView plug-in has it and no editor or page changes.
//
// JUCE puts the WebView2 at (its bounds in the window) x the peer's scale, and
// only when the WebView itself moves or resizes. Ways a host leaves it the
// wrong size:
//  - Ableton Live's "Auto-Scale Plug-In Window" (on by default, Windows; seen
//    2026-10-23 in Live 12.4.5 on a 125 % screen, RONE Control and RONE Throw):
//    the plug-in window is DPI-unaware and Windows stretches it x1.25 - the
//    plug-in sees 860 x 580, the screen shows 1075 x 725. The WebView2 inside
//    drew 860 x 580 real pixels: a white band right and bottom. Given the real
//    size (1075 x 725), the WebView2's own window is stretched the same way
//    (1344 x 906) and only the page's top-left (viewport / 1.25) is seen.
//    So: the stretch is measured (the client area seen by a DPI-aware caller
//    over what the plug-in sees), the WebView gets the real pixels, and the
//    page is told ("roneViewFit" { k }): the script below shrinks the body to
//    what is seen (its transform makes it the frame of every fixed element -
//    grip, About, update bar) and makes innerWidth / innerHeight say the same,
//    so each page's own fit (Math.min(innerWidth / 1240, ...)) fits it.
//  - A monitor move (RONE 1176, 2026-10-21): the peer's scale changes but the
//    window keeps its physical size - the WebView is sized from the window's
//    real client area.
//  - A host that scales the editor: JUCE scales it with a transform, the
//    WebView does not move or resize, so it is never told - refitted (a pixel
//    off and back) on each scale change, the peer's or the editor's.
// The editor keeps setting the WebView's bounds as before (resized()); this
// listens to the WebView and turns those bounds into the ones that fill the
// window. Twice a second it looks again: a window can be stretched or moved
// without the editor hearing of it.
//
// Found and checked first in RONE Control (tests/winrects.py: the window tree
// in real pixels; tests/wingrab.py: a real-pixel grab).
// ============================================================================

#include <cmath>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_gui_extra/juce_gui_extra.h>

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
#endif

class RoneWindowFit : private juce::ComponentListener,
                      private juce::ComponentPeer::ScaleFactorListener,
                      private juce::AsyncUpdater,
                      private juce::Timer
{
public:
    RoneWindowFit() = default;
    ~RoneWindowFit() override { detach(); }

    // The page side, and the page's "tell me" when it loads.
    juce::WebBrowserComponent::Options addTo (juce::WebBrowserComponent::Options options)
    {
        return options
            .withUserScript (kScript)
            .withEventListener ("roneViewFitHello", [this] (const juce::var&) { sentStretch = -1.0; check(); });
    }

    void attach (juce::WebBrowserComponent& b)
    {
        detach();
        browser = &b;
        base = b.getBounds();
        lastSet = {};
        b.addComponentListener (this);
        trackPeer();
        startTimerHz (2);
    }

private:
    // ---- the editor set the WebView's bounds: make them the window-filling ones
    void componentMovedOrResized (juce::Component& c, bool, bool) override
    {
        if (applying || c.getBounds() == lastSet)
            return;
        base = c.getBounds();
        apply (false);
    }

    void componentParentHierarchyChanged (juce::Component&) override { trackPeer(); triggerAsyncUpdate(); }
    void componentBeingDeleted (juce::Component&) override          { detach(); }
    void nativeScaleFactorChanged (double) override                  { triggerAsyncUpdate(); }

    // after a scale change: JUCE hands the WebView2 its size only on a bounds change
    void handleAsyncUpdate() override
    {
        trackPeer();
        apply (true);
        check();
    }

    void timerCallback() override { check(); }

    void check()
    {
        if (browser == nullptr)
            return;
        trackPeer();
        // a host that scales the editor does it with a transform: the WebView's bounds stay, so it is not told
        const double scale = juce::Component::getApproximateScaleFactorForComponent (browser);
        apply (std::abs (scale - seenScale) > 0.001);
        seenScale = scale;
        const double k = stretch();
        if (std::abs (k - sentStretch) > 0.005)
        {
            sentStretch = k;
            auto* o = new juce::DynamicObject();
            o->setProperty ("k", k);
            browser->emitEventIfBrowserIsVisible ("roneViewFit", juce::var (o));
        }
    }

    void apply (bool nudge)
    {
        if (browser == nullptr || applying)
            return;
        const auto target = targetBounds();
        if (target.isEmpty() || (! nudge && target == browser->getBounds()))
            return;
        const juce::ScopedValueSetter<bool> busy (applying, true);
        lastSet = target;
        if (nudge)
            browser->setBounds (target.withTrimmedRight (1));
        browser->setBounds (target);
    }

    juce::Rectangle<int> targetBounds() const
    {
        auto b = base;
       #if JUCE_WINDOWS
        if (const double k = stretch(); k > 1.0)
            return { juce::roundToInt (b.getX() * k),     juce::roundToInt (b.getY() * k),
                     juce::roundToInt (b.getWidth() * k), juce::roundToInt (b.getHeight() * k) };

        // a monitor move: the window's real client area, when the WebView fills an editor that fills the plug-in's
        // window (a standalone's own window has a frame, and JUCE scales it itself)
        auto* parent = browser->getParentComponent();
        if (! juce::JUCEApplicationBase::isStandaloneApp()
            && peer != nullptr && parent != nullptr && b == parent->getLocalBounds()
            && parent->getBounds().getWidth() == peer->getComponent().getWidth()
            && parent->getBounds().getHeight() == peer->getComponent().getHeight())
        {
            const auto hwnd = (HWND) peer->getNativeHandle();
            RECT rc {};
            const double s = juce::Component::getApproximateScaleFactorForComponent (parent);
            if (s > 0.0 && clientRect (hwnd, Win32::windowContext (hwnd), rc))
            {
                const int lw = juce::roundToInt ((rc.right - rc.left) / s), lh = juce::roundToInt ((rc.bottom - rc.top) / s);
                if (lw > 0 && lh > 0 && (std::abs (lw - b.getWidth()) > 2 || std::abs (lh - b.getHeight()) > 2))
                    b.setSize (lw, lh);
            }
        }
       #endif
        return b;
    }

    // How much Windows stretches this (DPI-unaware) window on screen: its client area seen by a DPI-aware caller
    // over the one the window itself has. 1 when the window is DPI-aware (JUCE scales it then, through the peer's
    // scale). Measured in Live on a 125 % screen: logical 860, physical 1075 -> 1.25.
    // Both are read under a stated DPI context: GetClientRect answers in the calling THREAD's context, and that is
    // the host's (per-monitor aware in a test host) unless Windows switched it to the window's for a message.
    double stretch() const
    {
       #if JUCE_WINDOWS
        if (peer != nullptr)
        {
            const auto hwnd = (HWND) peer->getNativeHandle();
            const auto own = Win32::windowContext (hwnd);
            RECT logical {}, physical {};
            if (own != nullptr && Win32::isUnaware (own)
                && clientRect (hwnd, own, logical) && logical.right > 0
                && clientRect (hwnd, (void*) (intptr_t) -4, physical))   // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
            {
                const double k = (double) physical.right / (double) logical.right;
                if (k > 1.02 && k < 4.0)
                    return k;
            }
        }
       #endif
        return 1.0;
    }

   #if JUCE_WINDOWS
    // user32's DPI calls by name (Windows 10 1607+), so nothing depends on the SDK's _WIN32_WINNT
    struct Win32
    {
        using SetCtx   = void* (WINAPI*) (void*);
        using WinCtx   = void* (WINAPI*) (HWND);
        using CtxToAw  = int   (WINAPI*) (void*);
        static FARPROC get (const char* name) { return GetProcAddress (GetModuleHandleW (L"user32.dll"), name); }
        static SetCtx setThread()  { static const auto f = (SetCtx)  (void*) get ("SetThreadDpiAwarenessContext");     return f; }
        static void* windowContext (HWND h)
        {
            static const auto f = (WinCtx) (void*) get ("GetWindowDpiAwarenessContext");
            return f != nullptr ? f (h) : nullptr;
        }
        static bool isUnaware (void* ctx)
        {
            static const auto f = (CtxToAw) (void*) get ("GetAwarenessFromDpiAwarenessContext");
            return f != nullptr && f (ctx) == 0;   // DPI_AWARENESS_UNAWARE
        }
    };

    // GetClientRect as seen in a given DPI context
    static bool clientRect (HWND hwnd, void* ctx, RECT& r)
    {
        const auto setCtx = Win32::setThread();
        if (setCtx == nullptr || ctx == nullptr)
            return GetClientRect (hwnd, &r) != 0;
        const auto old = setCtx (ctx);
        const bool ok = GetClientRect (hwnd, &r) != 0;
        if (old != nullptr) setCtx (old);
        return ok;
    }
   #endif

    void trackPeer()
    {
        auto* p = browser != nullptr ? browser->getPeer() : nullptr;
        if (p == peer)
            return;
        if (peer != nullptr && juce::ComponentPeer::isValidPeer (peer))
            peer->removeScaleFactorListener (this);
        peer = p;
        if (peer != nullptr)
            peer->addScaleFactorListener (this);
    }

    void detach()
    {
        stopTimer();
        cancelPendingUpdate();
        if (peer != nullptr && juce::ComponentPeer::isValidPeer (peer))
            peer->removeScaleFactorListener (this);
        peer = nullptr;
        if (browser != nullptr)
            browser->removeComponentListener (this);
        browser = nullptr;
    }

    juce::WebBrowserComponent* browser = nullptr;
    juce::ComponentPeer* peer = nullptr;
    juce::Rectangle<int> base, lastSet;   // the bounds the editor gave the WebView / the ones this gave it
    bool applying = false;
    double sentStretch = -1.0;            // the stretch the page was last told
    double seenScale = 1.0;               // the WebView's whole scale (peer x the editor's transform) at the last look

    // The page side. Plain ES5, one IIFE, nothing happens unless a stretch is reported.
    static constexpr const char* kScript = R"RONEJS(
(function () {
  if (window.__roneViewFit) return;
  window.__roneViewFit = true;
  var k = 1, hooked = false;
  var dw = Object.getOwnPropertyDescriptor(window, 'innerWidth');
  var dh = Object.getOwnPropertyDescriptor(window, 'innerHeight');
  function realW() { return dw && dw.get ? dw.get.call(window) : document.documentElement.clientWidth; }
  function realH() { return dh && dh.get ? dh.get.call(window) : document.documentElement.clientHeight; }
  // the page's own fit reads innerWidth / innerHeight: they say what is seen
  function hook() {
    if (hooked || !dw || !dh || !dw.get || !dh.get) return;
    try {
      Object.defineProperty(window, 'innerWidth',  { configurable: true, enumerable: true, get: function () { return Math.round(realW() / k); }, set: function () {} });
      Object.defineProperty(window, 'innerHeight', { configurable: true, enumerable: true, get: function () { return Math.round(realH() / k); }, set: function () {} });
      hooked = true;
    } catch (e) {}
  }
  function size() {
    var b = document.body && document.body.style;
    if (!b) return;
    if (k > 1) { b.width = (realW() / k) + 'px'; b.height = (realH() / k) + 'px'; b.transform = 'translateZ(0)'; b.overflow = 'hidden'; }
    else { b.width = b.height = b.transform = b.overflow = ''; }
  }
  function set(nk) {
    nk = nk > 1.02 && nk < 4 ? nk : 1;
    if (Math.abs(nk - k) < 0.001) return;
    k = nk;
    if (k > 1) hook();
    size();
    try { window.dispatchEvent(new Event('resize')); } catch (e) {}
  }
  // added at document start, so it runs before the page's own resize handlers
  window.addEventListener('resize', function () { if (k > 1) size(); });
  function backend() { return window.__JUCE__ && window.__JUCE__.backend; }
  function start() {
    var b = backend();
    if (!b) { setTimeout(start, 150); return; }
    b.addEventListener('roneViewFit', function (o) { set(o && +o.k); });
    var hello = function () { size(); b.emitEvent('roneViewFitHello', {}); };
    if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', hello, { once: true });
    else hello();
  }
  start();
})();
)RONEJS";

    JUCE_DECLARE_NON_COPYABLE (RoneWindowFit)
};
