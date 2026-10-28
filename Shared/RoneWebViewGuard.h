#pragma once

// ============================================================================
// RoneWebViewGuard - a plug-in's WebView2 never takes clicks meant for the DAW.
// Rides along with RoneUpdatePrompt (addTo / attachBrowser), so every RONE
// WebView plug-in has it and no editor or page changes.
//
// Found 2026-10-28 (Liran in Stucker, Lutra in Iron, FL Studio): with a plug-in
// window minimised, closed to the background or behind the mixer, the area
// where its page sits still takes the mouse - the cursor acts as if over the
// plug-in and FL's own controls there cannot be clicked.
//
// Why: current WebView2 runtimes put an invisible top-level window of their
// own (msedgewebview2.exe, Chrome_WidgetWin_1, layered, alpha 0, NOT
// WS_EX_TRANSPARENT, so it still hit-tests) over the page's area. It is not a
// child of the plug-in window, so it neither hides with it nor sits behind
// what covers it. It goes away only when the controller is told IsVisible =
// false, and JUCE 8.0.4 says that only when the WebBrowserComponent itself is
// hidden - never when the host hides, minimises or covers the window around
// it. Same window, same cause: MicrosoftEdge/WebView2Feedback #5668,
// egoist/mygo #830 (fixed there by hiding the WebView with its window).
//
// So: 30 times a second this looks at the plug-in's real window and hides the
// WebView (JUCE setVisible -> put_IsVisible(false), the page stays loaded) when
//   - the window is hidden, minimised or entirely off every screen, or
//   - the mouse is over the plug-in's area but another window is on top there
//     (the mixer, the browser, another program): that click is not the plug-in's.
// and shows it again as soon as neither holds. Nothing changes while a mouse
// button is held (a knob drag that slides over the mixer keeps going), and
// nothing happens on macOS (WKWebView is a real child view there).
// ============================================================================

#include <map>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_gui_extra/juce_gui_extra.h>

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
#endif

class RoneWebViewGuard : private juce::Timer
{
public:
    RoneWebViewGuard() = default;
    ~RoneWebViewGuard() override { detach(); }

    // Hiding the WebView must not unload the page (JUCE's default sends a hidden
    // browser to about:blank and the page would reload on every show).
    static juce::WebBrowserComponent::Options addTo (juce::WebBrowserComponent::Options options)
    {
        return options.withKeepPageLoadedWhenBrowserIsHidden();
    }

    void attach (juce::WebBrowserComponent& b)
    {
        detach();
        browser = &b;
       #if JUCE_WINDOWS
        startTimerHz (30);
       #endif
    }

private:
    void detach()
    {
        stopTimer();
        if (browser != nullptr && hiding)
            browser->setVisible (true);
        hiding = false;
        browser = nullptr;
    }

    void timerCallback() override
    {
       #if JUCE_WINDOWS
        if (browser == nullptr)
            return;

        auto* peer = browser->getPeer();
        if (peer == nullptr)
            return;

        // a held button: a drag in progress - the plug-in's or the host's - is left alone
        if ((GetAsyncKeyState (VK_LBUTTON) | GetAsyncKeyState (VK_RBUTTON) | GetAsyncKeyState (VK_MBUTTON)) & 0x8000)
            return;

        const auto hwnd = (HWND) peer->getNativeHandle();
        const bool hide = ! reallyShown (hwnd) || coveredUnderMouse (hwnd);

        if (hide != hiding)
        {
            hiding = hide;
            browser->setVisible (! hide);
        }
       #endif
    }

   #if JUCE_WINDOWS
    // Visible (with every ancestor), not minimised, and on some screen.
    static bool reallyShown (HWND hwnd)
    {
        if (hwnd == nullptr || ! IsWindow (hwnd) || ! IsWindowVisible (hwnd))
            return false;

        if (IsIconic (GetAncestor (hwnd, GA_ROOT)))
            return false;

        RECT r {};
        if (! GetWindowRect (hwnd, &r) || r.right <= r.left || r.bottom <= r.top)
            return false;

        return MonitorFromRect (&r, MONITOR_DEFAULTTONULL) != nullptr;
    }

    // The mouse is inside the plug-in window, but a window above it in z-order
    // (inside the host, or any other program's) holds that point.
    bool coveredUnderMouse (HWND hwnd)
    {
        POINT pt {};
        RECT mine {};
        if (! GetCursorPos (&pt) || ! GetWindowRect (hwnd, &mine) || ! PtInRect (&mine, pt))
            return false;

        for (HWND level = hwnd; level != nullptr; )
        {
            for (HWND above = GetWindow (level, GW_HWNDPREV); above != nullptr; above = GetWindow (above, GW_HWNDPREV))
                if (holdsPoint (above, pt))
                    return true;

            if ((GetWindowLongPtrW (level, GWL_STYLE) & WS_CHILD) == 0)
                break;                              // the top level's siblings were the last to check

            level = GetAncestor (level, GA_PARENT);
        }

        return false;
    }

    // A window that really is on screen at pt and would take a click there.
    bool holdsPoint (HWND w, POINT pt)
    {
        if (! IsWindowVisible (w))
            return false;

        RECT r {};
        if (! GetWindowRect (w, &r) || ! PtInRect (&r, pt))
            return false;

        const auto ex = GetWindowLongPtrW (w, GWL_EXSTYLE);
        if ((ex & WS_EX_LAYERED) != 0 && (ex & WS_EX_TRANSPARENT) != 0)
            return false;                           // click-through overlays

        if ((GetWindowLongPtrW (w, GWL_STYLE) & WS_CHILD) == 0)
        {
            if (IsIconic (w) || isCloaked (w))
                return false;                       // minimised, or on another virtual desktop

            if ((ex & WS_EX_TOPMOST) != 0)
                return false;                       // menus, tooltips, the taskbar: above the overlay, they take their own clicks

            if (isWebView2Window (w))
                return false;                       // the invisible overlays themselves (ours or another plug-in's)

            if (isDropOverlay (w))
                return false;                       // the plug-in's own file-drop layer, part of the page

            if ((ex & WS_EX_LAYERED) != 0 && ! layeredAndSeen (w))
                return false;                       // a screen-wide overlay (NVIDIA, Discord, Steam): clicks go through it
        }

        // a window with a region (rounded / shaped) only holds what its region holds
        HRGN rgn = CreateRectRgn (0, 0, 0, 0);
        const int kind = GetWindowRgn (w, rgn);
        bool inside = true;
        if (kind == SIMPLEREGION || kind == COMPLEXREGION)
            inside = PtInRegion (rgn, pt.x - r.left, pt.y - r.top) != 0;
        DeleteObject (rgn);
        return inside;
    }

    // The sample plug-ins (Iron, Stutter, Reverse Reverb) lay a window of their
    // own over the page to take file drops and the drag-out: a top-level popup
    // in this process, class "<Plugin>DropOverlay", alpha 1, passing every click
    // on to the WebView. It is the plug-in, not something covering it - counting
    // it hid the page the moment the mouse came over it, and the window went
    // black (Alex 2026-10-28, Live 12 and Studio One, 1.0.12 / 1.7.2 / 1.2.8).
    static bool isDropOverlay (HWND w)
    {
        DWORD pid = 0;
        GetWindowThreadProcessId (w, &pid);
        if (pid != GetCurrentProcessId())
            return false;

        wchar_t cls[128] = {};
        if (GetClassNameW (w, cls, 128) <= 0)
            return false;

        return juce::String (cls).endsWith ("DropOverlay");
    }

    // A layered window of another program only covers us when it is drawn with
    // one visible opacity (SetLayeredWindowAttributes, LWA_ALPHA above 0). The
    // screen-wide overlays of the GPU driver, Discord, Steam or the game bar draw
    // per pixel (UpdateLayeredWindow): Windows hands the mouse through wherever
    // they are clear, which is almost everywhere. Counting them blacked out the
    // page on any machine running one (found beside the DropOverlay case, NVIDIA
    // Overlay over the whole desktop). A layered window can't be asked for its
    // pixel under the mouse, so per-pixel ones are left out; a real app window
    // that draws per pixel is rare, and missing it only means the old FL catch.
    static bool layeredAndSeen (HWND w)
    {
        BYTE alpha = 0;
        DWORD flags = 0;
        if (! GetLayeredWindowAttributes (w, nullptr, &alpha, &flags))
            return false;                           // per pixel (UpdateLayeredWindow)

        return (flags & LWA_ALPHA) == 0 || alpha > 0;
    }

    static bool isCloaked (HWND w)
    {
        using Fn = HRESULT (WINAPI*) (HWND, DWORD, PVOID, DWORD);
        static const auto fn = []
        {
            const auto dwm = LoadLibraryW (L"dwmapi.dll");
            return dwm != nullptr ? (Fn) (void*) GetProcAddress (dwm, "DwmGetWindowAttribute") : (Fn) nullptr;
        }();

        DWORD cloaked = 0;
        return fn != nullptr && SUCCEEDED (fn (w, 14 /* DWMWA_CLOAKED */, &cloaked, sizeof (cloaked))) && cloaked != 0;
    }

    // Belongs to a WebView2 browser process (msedgewebview2.exe). Cached per process id.
    bool isWebView2Window (HWND w)
    {
        DWORD pid = 0;
        GetWindowThreadProcessId (w, &pid);
        if (pid == 0 || pid == GetCurrentProcessId())
            return false;

        if (processCache.size() > 256)
            processCache.clear();

        if (const auto it = processCache.find (pid); it != processCache.end())
            return it->second;

        bool isWebView = false;
        if (const auto proc = OpenProcess (PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid))
        {
            wchar_t path[MAX_PATH] = {};
            DWORD len = MAX_PATH;
            if (QueryFullProcessImageNameW (proc, 0, path, &len))
                isWebView = juce::String (path).endsWithIgnoreCase ("msedgewebview2.exe");
            CloseHandle (proc);
        }

        processCache[pid] = isWebView;
        return isWebView;
    }

    std::map<DWORD, bool> processCache;
   #endif

    juce::WebBrowserComponent* browser = nullptr;
    bool hiding = false;

    JUCE_DECLARE_NON_COPYABLE (RoneWebViewGuard)
};
