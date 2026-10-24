# minihost - a plug-in window the way Ableton Live opens it

Ableton Live's "Auto-Scale Plug-In Window" (on by default, Windows) opens the plug-in window DPI-unaware inside a
per-monitor aware process, and Windows stretches it (x1.25 on a 125 % screen). `minihost.exe` does the same with any
VST3 - no audio, no MIDI, so it works while the Windows MIDI service is wedged (see the memory note on it) and needs
no DAW. It hosts the component and controller, connects them, and attaches the editor to a raw Win32 window.

    build.bat "<a plug-in build>\_deps\juce-src"
    minihost.exe "<...>.vst3\Contents\x86_64-win\<name>.vst3" "MH Throw"          (Live's stretched window)
    minihost.exe "<...>.vst3\Contents\x86_64-win\<name>.vst3" "MH Throw" aware    (FL / REAPER / Live without it)

Pass the DLL inside the bundle. Then look with RoneControl's `tests/winrects.py "MH Throw"` (the window tree in real
pixels: `Chrome_WidgetWin_1` must be the JUCE window x the stretch, and the page's innerWidth the editor's width) and
`tests/wingrab.py "MH Throw" out.png` (a real-pixel grab). For the page itself, start it with
`WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS=--remote-debugging-port=9333` and read it over CDP.
A monitor move: open it `aware` and move the window to a screen with another scale (SetWindowPos from a per-monitor
aware process) - minihost does not answer WM_DPICHANGED, so the window keeps its physical size, as hosts do.

Found the RoneWindowFit bugs on 2026-10-24: before it, every RONE WebView plug-in drew in the top-left 80 % with a
white band; a measurement that trusted the thread's DPI context read a stretch of 1 here.
