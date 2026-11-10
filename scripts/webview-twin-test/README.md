# webview-twin-test - two windows of one plugin, opened together

Checks a RONE WebView plugin for the JUCE 8.0.4 WebView2 serial-creation trap. JUCE 8.0.4
builds WebView2s one at a time, and a script sent to a view that does not exist yet
(`emitEventIfBrowserIsVisible` from a timer started in the editor's constructor) can leave
that window blank for good when another window of the SAME plugin DLL is being built at that
moment - a DAW reopening a project with two of its windows open. The fix in every editor is
`pageReady`: set by the page's first message, and the timer pushes nothing before it.
JUCE 8.0.12 (Flanger, Analyzer, Stems, Center) no longer has the trap.

`TwinEditorHost` loads one VST3, makes N instances of it, creates all N editors and their
windows in one message-loop turn and hosts them. `cdp_check.mjs` then asks the WebView2 debug
port whether every editor owns a `https://juce.backend/` page, and whether each page still
receives the event its timer pushes. It all runs on a hidden desktop (`HiddenDesktopRun`) with
its own WebView2 profile, so a DAW that is open on the machine never loses focus or its profile.

```
# once: the host, against a JUCE 8.0.4 tree from any plugin build
cmake -S scripts/webview-twin-test -B $env:TEMP\wv2t\host -G "Visual Studio 17 2022" -A x64 -DJUCE_DIR=<juce-src>
cmake --build $env:TEMP\wv2t\host --config Release
# a plugin, out of tree, below-normal priority, never installed
scripts\webview-twin-test\build_plugin.ps1 -Src RoneStucker -Short stk -Target RoneStucker_VST3 -Bundle stucker
# the test: -Event is something the editor's timer pushes (paramState, playbackState, bpmState...)
scripts\webview-twin-test\run_twin.ps1 -Vst3 "$env:TEMP\wv2t\bundles\stucker\RONE Stucker.vst3" -Event paramState
```

Results 2026-09-27: RONE Stucker 1.2.1 as released - 1 of 2 pages (twice), 2 of 3 with three
windows. With `pageReady` - 2 of 2 (three runs) and 3 of 3; Throw, Rise, Clipper, Reverse Reverb,
Stutter, Sync Verb and AFTERSPACE 2 of 2, every page receiving its pushes.

## reopen - one window closed and opened again at once (2026-11-10)

Liran in FL: DMT, then Iron - closed the window, opened it again from the Channel rack, white for
46 s to minutes. The closed window's msedgewebview2 browser lingers 20-40 s while it shuts down
(under load); a new window on the same profile folder gets E_ABORT from
CreateCoreWebView2Controller after 10 s and JUCE (8.0.4 and 8.0.12) never retries. The fix is
`Shared/RoneWebViewFolder.h` (through `RoneUpdatePrompt::addTo`): each window picks a folder whose
browser is gone.

`ReopenEditorHost` opens one window, hosts it, deletes it, opens it again; it sees the page from
inside (a `Chrome_RenderWidgetHostHWND` under the window), so no debug port decides the result.
`-LingerMs` freezes each closed window's browser that long - the slow shutdown made certain.

```
cmake --build $env:TEMP\wv2t\host --config Release --target ReopenEditorHost
scripts\webview-twin-test\reopen.ps1 -Vst3 "<bundle>" -Cycles 6 -Hold 10 -GapMs 300 -LingerMs 20000
```

A build with the fix gets `RONE_WEBVIEW_TEST_ROOT` (its folders under the test root);
one without gets the old single `WEBVIEW2_USER_DATA_FOLDER`. Results 2026-11-10, Iron:
installed 1.1.0 - 4 of 6 opens white; with the fix - 8 of 8 in under 1 s (folders rotate
`RoneIron_WebView2`, `-2`, `-3`). The Test Bench runs the same case nightly (check `reopen`).
