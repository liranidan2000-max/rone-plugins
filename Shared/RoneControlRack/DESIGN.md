# RONE Control Rack - design and the production shape

Liran, 2026-10-24: every RONE plugin gets a hidden `+` on its right edge; click it and RONE Control opens
beside the plugin, already mapped to its knobs. Visual control of every RONE plugin with zero linking.
His condition: it must never be something we regret - stability first, every point thought through.

## 1. Why it is smaller than it sounds

RONE Control's hard part is talking to OTHER people's plugins: FL "Link to controller", Live control
surfaces, REAPER plink, a virtual MIDI port, Cubase still open. Inside a RONE plugin none of that exists:
the hand writes to the plugin's own `AudioProcessorValueTreeState` with a change gesture, exactly like a
mouse drag. The host records automation, every DAW behaves the same, Windows and Mac alike.

`LinkHub` is already a `juce::SharedResourcePointer`: one camera / one phone link per DAW process. Ten RONE
plugins with their racks open share one hub. The architecture for the rack exists; it has to be moved,
not invented.

## 2. The production shape

```
Shared/RoneControlRack/          <- RoneControl/Source/Core moved here (ControlCore, HandLink, LinkHub, LinkView)
   RackHub.h                     one per process: hub + which editor is ARMED + the shared model files
   RackView.h                    the side panel: a second WebBrowserComponent showing the rack page
   RackEditorHelper.h            glue for an editor: + handle, layout, aspect switch, manifest
RoneControl/                     becomes the first HOST of that library (its own slots, LINK, MIDI, phone)
<Plugin>/Source/rack.json        the manifest: parameters + shipped map (2 per hand max)
```

Per plugin the change is small and mechanical, through the Shared helper (the same way RoneUpdatePrompt and
RoneWindowFit reached every plugin):

```cpp
RackEditorHelper rack { *this, processorRef.apvts, RackManifest::fromResource ("rack.json") };
// resized(): rack.layout (webView, getLocalBounds());  -> plugin bounds + optional rack bounds
// requestResize: rack.heightForWidth (w)              -> keeps the right ratio for the current state
```

Two WebViews side by side (plugin page + rack page) rather than injecting the rack into each plugin page:
the eleven pages all lay out with their own `fitScale` on `#app-viewport`, and a script that wraps or
narrows that viewport is a fragile cross-page change. A second WebView is zero change to any page.
Trap to honour: [webview2-serial-creation] - the second WebView is created after the first is up, never
in the same tick, and its user data folder is its own.

### The `+`
A 14 px hover strip on the editor's right edge, drawn by the plugin's WebView? No - by a tiny JUCE
component over the WebView (so no page changes): a 3 px laser-green hairline, hover widens it to 46 px with
the `+`, click opens. Vertically centred, 84 px tall, so the corner grip at the bottom right stays free.
Hidden while the rack is open; the rack's own `›` collapses it. State (open / closed) is remembered per
plugin in `settings.json` of the hub.

### Window and aspect ratio (the point Liran named)
Every RONE editor keeps a fixed ratio `W : H` (`kBaseW : kBaseH`) and resizes from its corner grip. With the
rack open the editor's ratio becomes `(W + 280·k) : H` where `k = H_now / kBaseH` - i.e. design ratio
`(kBaseW + 280) : kBaseH`. The helper updates the constrainer / `setResizeLimits` on every toggle and
`heightForWidth` uses the ratio of the current state. The grip keeps working.

Hosts: FL Studio, Live, Logic, REAPER, Bitwig, Cubase and Studio One all honour `setSize` from the
editor (the plugin asks, the host resizes). If a host refuses (the editor's `getWidth()` does not change
after `setSize`), the helper falls back: the plugin's WebView narrows and the rack takes at most 38 % of the
window, the rack scaling itself (`HOST: REFUSES` in the mockup shows exactly that). For a 440-wide plugin
that fallback is poor - acceptable only because it should never be needed; it must be tested per host
before release and the hosts where it triggers listed in the manual.

Ableton Live's auto-scale (RoneWindowFit) measures the real client area and refits; it must run on the rack
WebView too - both views get `RoneWindowFit::attach`.

### Loading only when asked (the stability point)
Nothing loads until the `+` is clicked: no second WebView, no MediaPipe, no camera permission prompt, no
model, no phone link. A customer who never clicks the `+` runs the plugin exactly as today - same memory,
same startup, same DAW scan. The `+` itself is a few hundred bytes of JUCE.

### The model lives once
RONE Control's DLL carries the model (~26 MB). The rack never embeds it: the hub reads
`%APPDATA%\RonePlugins\Control\link` (already the pattern - link.html + MediaPipe extracted there). If RONE
Control is not installed, the `+` opens a one-card page: "RONE Control gives this plugin hands - free, from
the Center", a button to launch the Center. So the rack SELLS RONE Control instead of duplicating it, and
every installer stays its current size. Mac: the hub is phone-only until a macOS camera path exists -
same gap as RONE Control today, the rack does not widen it.

### Audio thread
The hub and the mapper run on the message thread (a 60 Hz timer, as in RONE Control). The mapper calls
`setValueNotifyingHost` inside `beginChangeGesture` / `endChangeGesture` for exactly as long as a slot is
held or gliding home. The DSP sees parameter changes like any other; no lock, no allocation, no new thread.

### Focus: who gets the hands (the point users will fumble)
One camera, several open RONE plugins: the hand drives ONE plugin. Rule: the last rack you ARMED (opened,
or clicked its ARMED pill) owns the hands; every other rack shows its hands dimmed and "ARMED ELSEWHERE ·
click to take". Armed state is hub-wide, so it is consistent across all editors. Closing the armed
plugin's editor hands the arm to the most recently armed open rack, else nobody. Shown, never silent.

### The manifest (one file per plugin)
```json
{ "window": { "w": 440, "h": 560 },
  "params": [ { "id": "throw", "name": "THROW", "type": "float", "min": 0, "max": 1, "def": 0, "unit": "%" }, ... ],
  "rack": [ { "param": "throw", "hand": "R", "src": "height", "eng": "pinch", "mode": "grab", "rel": "return", "home": 0,
              "why": "Pinch and lift: the delay opens. Let go and it falls back." }, ... ] }
```
At most two knobs per hand; a slot per hero knob, in the plugin's accent, with ONE sentence that says the
gesture. The user may change knob / hand / move / hold / let-go per slot (the same words as RONE Control's
slot editor) and reset to the plugin map. Bool parameters (KILL, GO, RIDE GO) take `grip` or `pinch`:
output above 0.5 = on, edge-triggered. Choice parameters are not offered.

### Licensing
As RONE Control: LITE = slot A free in every plugin (the hero knob by hand, for everyone - the demo that
sells), the other slots need a RONE Control licence or ALL ACCESS. The lock is the rack page's, like today.

### Kill switch
The remote kill-switch ([security-pass-2026-09-01]) gets a `rack` flag per plugin and a global one: off
means the `+` is not drawn. A misbehaving host can be darkened in minutes without a release.

## 2b. What was built (2026-10-24, evening) - the DEV rack in RONE Throw

`RoneControlRack/Shared/RoneControlRack/` (to move into the monorepo's Shared/ when it ships):
* `RackEngine` - per plugin instance, owned by the processor. Slots bind a hand move to an APVTS parameter
  through RONE Control's own `SlotMapper` (compiled in from RoneControl/Source/Core, untouched); writes
  inside begin/end change gesture; switches (KILL, GO) are edge-triggered; a frame older than 0.5 s = no
  hands (a camera that stops lets go). Hands come from RONE Control's `LinkHub` (SharedResourcePointer; the
  SAME pairing / settings files as RONE Control, so a phone paired there feeds the rack; peer mode when
  another process owns the link). ARMED = `%APPDATA%\RonePlugins\Control
ack-armed.json` - one truth
  across plugin binaries and DAW processes, polled at 6 Hz. The user's map is saved per plugin on this
  computer (`%APPDATA%\RonePlugins\<id>
ack-slots.json`), never in the project.
* `RackView` - a second `WebBrowserComponent` with the rack page (`ui/rack.html|css|js` as BinaryData); built
  on the first open, nothing sent before `rackReady`; `rackState` at 30 Hz.
* `RackEditorHelper` - the `+` is a USER SCRIPT in the plugin's page (a JUCE component cannot sit over a
  native WebView2 window): 3 px laser hairline, hover widens, click emits `roneRack {open}`; hidden while
  open. `layout()` splits the editor between the plugin view and the rack; `heightForWidth()` keeps
  `(W + 280) : H` while open; after `setSize` it checks 180 ms later whether the host obeyed, else SHRINK
  (rack <= 38 %). The editor's min/max follow (`applyResizeLimits`).
* RONE Throw, branch `rack-dev` (tag `pre-rack` on main, `rack-dev-0` first build): rack.json + CMake block +
  5 lines in the processor header + ~25 in the editor. Built Release VST3 + Standalone with zero errors.
  DEV only: the hand model rides as RCDATA (27 MB) exactly as RONE Control's does; the shipped rack will read
  the files RONE Control installed instead.
* Not verified at run time yet: the Windows MIDI service (MidiSrv) was wedged on Liran's PC that evening -
  every JUCE standalone (the installed RONE Throw too) blocked in its MIDI device scan (LpcReply). A reboot
  is needed before any DAW / standalone test ([windows-midi-service-hang]).

## 2c. Liran's test of the standalone (2026-10-25) - what changed
* The phone QR lives in the rack (PHONE tab, until the phone is in; NEW CODE). Same room + key as RONE Control
  (`qrcode-generator` compiled in, drawn offline). Bug found: the saved `open: true` made `setOpen (true)` return
  early, so the link never started on the next open - the saved open state is now remembered only.
* The `+`: a GREY hairline at rest, laser green only under the mouse ("more aesthetic").
* A closed rack or a closed plugin window NEVER moves a knob: `EditorHelper`'s destructor closes the engine's
  rack (`editorClosed()`), which releases every gesture and DISARMS (the armed file says nobody); any other open
  rack takes the hands over at its next 6 Hz check. The window remembers it was open and reopens the rack 600 ms
  after it comes back (per instance, this session). One pairing feeds every plugin: `LinkHub` is per process and
  reads the same pairing files, so a phone paired once works in every RONE plugin's rack (peer mode across DAW
  processes).
* REC (`RackTransport`): "open the plugin, open its control, press REC, lift the hand". REAPER: directly through
  `IReaperHostApplication` (`Main_OnCommand` 1013, `GetPlayState` & 4 lights the button). FL / Live / Bitwig: through
  RONE Control's own script in that DAW (FL impl 4, Live impl 2, Bitwig impl 2, 25.10). The rack writes a command
  file {seq, op: "record", on, at}; the script acts once per (seq, at) while fresh (5 s) - on = record armed + play,
  off = record off + stop - and writes a state file {recording, playing, host, at, ack} about every second (at once
  when something changed): a fresh one (3 s) lights the button and proves the script is there (`canRecord`), without
  it the button is dimmed with "<host>: open RONE Control once - its script does the recording". Live and Bitwig:
  `<data>/transport/cmd.json` + `state.json` (`RoneCtl::controlDataFolder()`, the script deletes the command). FL:
  FL's Python sandbox lets a script write only in its own folder and never delete, so both files sit in FL's
  Hardware/RONE Control folder as `transport_cmd.json` / `transport_state.json` (the registry's "Shared data" path,
  as RONE Control's FlBridge finds it) and the rack deletes the command once its "at" comes back as ack, or after 5 s.
  Standalone / Cubase / Studio One / Logic: dimmed with the reason. JUCE's `AudioPlayHead::transportRecord` is
  implemented by no JUCE wrapper, so it is not used.
* The processor forwards `VST3ClientExtensions::setIHostApplication` to the transport and the play head's
  `isRecording` from `processBlock`.

## 2d. Phase 0 tests in real hosts (2026-10-25, afternoon)
* REC verified in FL Studio 2026 (the FL script, impl 4) and in REAPER (direct API): press = the host records, the
  button turns red, second press stops. REAPER's 1013 only toggles record and leaves the transport playing, so
  `record (false)` now sends 1016 Transport: Stop (restore-10 -> 11). Live: the impl 2 script records and stops from
  `cmd.json`; the button inside Live with Throw loaded is still to press. Bitwig: not tested (Liran skipped it).
* pluginval strictness 10: SUCCESS on the rack-dev Throw VST3 (`_daw-lab\pv-throw-rack.log`). Steinberg validator:
  46 pass / 1 fail "Program 000 has no name" - identical in the released Throw, not the rack's.
* Scan time: no measurable difference (DLL + factory 4-5 ms vs 3 ms; full validator 0.23-0.26 s vs 0.22-0.23 s).
* Traps: the FIRST click into the rack WebView after its window gets focus is swallowed (second works) - to fix
  before public; never start the lab REAPER with a WebView debug port while another RONE process holds a WebView2
  profile with different arguments (white plugin window); an FX window moved to a monitor with another DPI is drawn
  at the old size with white margins (the known DPI trap).

## 2e. Liran's own test inside FL Studio 2026 (2026-10-25, ~15:10) - PASSED
Throw rack-dev VST3 (built 14:59 with the latest Core) beside RONE Control, the phone paired to RONE Control:
the `+` grew the plug-in window inside FL, the rack showed the hands at once (a peer of RONE Control, no QR),
pinch + raise moved THROW, REC started FL's recording and the second press stopped it, collapsing the rack
released the hands (nothing moved), reopening took them back. His words: "הכל עבד, הכל פיקס". First real-DAW
pass of the rack. Still open: Live with Throw (auto-scale, SHRINK), 200 open/close cycles, the swallowed first
click, Core -> Shared before any second plug-in.

## 2f. Phase 0 of the Rack-first rollout (2026-10-26, Shared once, before Throw 1.9.0)
Liran's decisions (plan https://claude.ai/artifact/GaEkpYNtgi39qB4i8PAnFM, all recommended): the model travels in every
rack installer (Hands folder), LITE licensing (slot A free, B.. ALL ACCESS), REC dimmed "comes with RONE Control",
waves Throw -> Rise/Stucker/Clipper -> Flanger/Iron/AFTERSPACE -> 1176/2A, 280 px fixed (narrow Clipper variant),
DEV channel first for every plugin. Built here:
* `prepareLinkFolder`: third source = `C:\Program Files\RONE Plugins\Hands` (copy into the link folder); the card
  reads HAND MODEL MISSING / reinstall from the Center. `installer/RoneThrow.iss` `hands` block + CI step
  `Hand model - the rack` (runs `scripts/ci/fetch_hands.py` when `Source/rack.json` exists).
* LITE in `Engine` (`refreshLite`, `locked(i)`) and in rack.js/css (locked cards, amber badge, the ONE KNOB IS FREE card).
* `RackTransport::why` for script hosts: "<host>: REC comes with RONE Control".
* rack.js `tap()`: every control acts on pointerdown and ignores the click that follows (the DAW-side first-click trap:
  a mousedown that reaches the page while the host activates the window still counts).
* Tests on the Throw standalone (real mouse, SetCursorPos + mouse_event, DPI 125 %): a click into the rack with the
  window unfocused delivers pointerdown + click and toggles LIVE - the swallowed first click did NOT reproduce in the
  standalone (DESIGN 2d saw it inside a DAW; Liran's FL test checks it there). LITE renders as designed.
  200 rack open / close cycles FOUND A BUG: the window came back 64 px too wide after every collapse (560 -> 910 -> 640
  real px) - the open limits' minimum (kMinW * (W+280)/W = 504 > 440) still clamped the setSize. Fixed in
  EditorHelper::setOpen: the limits narrow before the window does. After the fix 560 -> 910 -> 560 exactly.
  After the fix: 200 rack open / close cycles, 0 bad (910 open / 560 closed every time, ARMED taken and released),
  private bytes 69.3 -> 73.7 MB and flat from cycle ~180. pluginval --strictness-level 2 --repeat 200 (the editor
  opened and closed 200 times with the rack closed): SUCCESS, memory a sawtooth 88-153 MB (compared with RONE Rise
  without a rack in the rollout memory note).
* Not run here (needs Liran at the machine): Live auto-scale 100 % / 125 %, Bitwig, REAPER again, Zanon's Mac.

## 3. Rollout and restore points

1. Mockup (this folder) on Throw, Rise, Stucker - Liran tests on each, the map and the words settle.
2. `Core` -> `Shared/RoneControlRack` with RONE Control as its only host; RONE Control's tests (ControlHost
   23, midi-port 9, camera, phone) must stay green. Tag RoneControl before (`pre-rack`).
3. Rack in ONE plugin on the DEV channel (Throw: the one-knob plugin is the best demo). Tag that
   submodule before (`pre-rack`), ship only through `tools/dev_publish_local.py`.
4. Host matrix on that one plugin: FL, Live (auto-scale on and off, 100 % and 125 %), REAPER, Bitwig, Cubase
   trial, Logic + Ableton on Zanon's Mac. pluginval strictness 10. Rack closed must be byte-for-byte the
   same behaviour as before (open / close editor 200 times; no leak: the second WebView is destroyed with
   the editor).
5. Then each plugin on its next release through the Shared helper - never all at once.

No-go criteria (any one stops the rollout): a crash with the rack closed; a host that refuses the resize
AND looks unusable in fallback; CPU of the plugin with rack closed above today's; a DAW scan slower than
today.

## 4. Open questions for Liran
* The rack's width: 280 design px is 64 % of Throw and 33 % of Rise. One width for all, or per plugin?
* Open / closed is remembered per plugin instance within the session (the window reopens with its rack). Per plugin (machine) is still open.
* The `+` colour: decided 25.10 - grey at rest, laser green on hover, on every plugin.

## 5. 2026-10-26 - Core moved into the monorepo's Shared/

`RoneControl/Source/Core/{ControlCore,HandLink,LinkHub,LinkView}` + `Source/LinkUI.h` and the rack library of the
side repo now live in `Shared/RoneControlRack/` (this folder; see README.md). RONE Control compiles them from
`../Shared/RoneControlRack`; its DAW layer (MidiDevicePort, FlBridge, ReaperBridge, the scripts) stayed in
`Source/Core`. RONE Throw's `rack-dev` points at the same folder and no longer embeds the hand model: the shipped
rack reads the files RONE Control installed ("the model lives once" is implemented in `prepareLinkFolder`; without
them the rack shows the "RONE Control needed" card with an OPEN THE CENTER button). `Version.h::kCoreProtocol`
(= 1) is written into `link-status.json` as `core`. The CI path filters exclude `Shared/RoneControlRack/**` so a
rack change alone releases nothing. Restore points: RoneControl `pre-shared`, RoneControlRack `restore-13`,
RoneThrow `rack-dev-4`. The side repo's `Shared/` copy is retired (README points here); its `mockup/` and `docs/shots/`
stay there as history.

### 5a. ARMED includes RONE Control itself (Liran's FL test, 26.10 09:00)
Found: Throw's rack open and armed, RONE Control's window hidden -> the hand moved Throw AND RONE Control's slots.
RONE Control never read `rack-armed.json`. Now `RackArmed.h` holds the file and its rules for everyone: RONE Control
polls it at 6 Hz (`checkArmed`), yields the hub's hands while a rack of ANOTHER plugin is armed (`hubHands` returns
early; the frame goes stale in 0.35 s and the slots let go as with no hands), arms itself when its window opens
(`editorOpened`) and releases on close if it holds the arm, so the open rack takes over at its next check. Its status
line then says `HANDS IN RONE THROW · CLICK TO TAKE`; the click arms RONE Control and the rack shows ARMED ELSEWHERE.
Two RONE Control instances never yield to each other (a hidden instance driving DAW knobs over MIDI is the everyday use).

**TAP LIVE (26.10, after Liran's first FL run).** LIVE had been left off by a test (the state file is shared by every
instance of the plugin), the phone streamed at 23 FPS, and "I grab and nothing happens" was the whole experience - the
only sign was NOT LIVE in the status line. Now a hand in the picture while LIVE is off turns the LIVE button into a
pulsing TAP LIVE (`.r-live.ask`); it reads LIVE again the moment the hand goes or LIVE is on. Checked in the standalone
over CDP with the mouse sim (`scratchpad/taplive_test.py`: off+no hand = LIVE, off+hand = TAP LIVE ask, hand gone = LIVE,
on+hand = LIVE active).
