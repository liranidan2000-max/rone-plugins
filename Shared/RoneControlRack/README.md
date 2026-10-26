# Shared/RoneControlRack - RONE Control's hands, for every RONE plugin

The hands layer of RONE Control and the rack that puts those hands beside any plugin. Compiled into
RONE Control (its only host until a rack ships) and into each plugin that takes the rack. Moved here on
2026-10-26 from `RoneControl/Source/Core` (tag `pre-shared` in RoneControl) and from the side repo
`RoneControlRack` (tag `restore-13`), so the CI - which sees a plugin's submodule, `Shared/` and the
monorepo's own files only - can build a plugin with a rack.

## What is here

| File | Role |
|---|---|
| `ControlCore.*` | hand frame (+ binary codec), slot config, One Euro filter, `SlotMapper`, `controlDataFolder()` |
| `HandLink.*` | the localhost hand link (tests) and the owner -> peer hand server / client (TCP 47932) |
| `LinkHub.*` | one per DAW process (`SharedResourcePointer`): pairing, the link window, owner / peer, `link-status.json` |
| `LinkView.*` | Windows: the off-screen WebView2 running the link page; `prepareLinkFolder` writes its files |
| `LinkUI.h` | GENERATED from `RoneControl/Source/link/link.html` by `RoneControl/tools/build_webui.py` - never edit |
| `RackEngine.*` | per plugin instance: slots -> this plugin's APVTS parameters, ARMED, the user's map |
| `RackView.*` | the rack page in a second `WebBrowserComponent` |
| `RackEditorHelper.*` | the `+` handle, layout, aspect ratio, resize limits |
| `RackTransport.*` | the REC button (REAPER API, or RONE Control's script in FL / Live / Bitwig) |
| `RackArmed.h` | who has the hands: `rack-armed.json`, read and written by every rack AND by RONE Control itself |
| `../../scripts/ci/fetch_hands.py` | CI: the five model files for a rack plugin's installer (`hands` block), pinned by SHA-256 |
| `ui/` | `rack.html`, `rack.css`, `rack.js`, `vendor/qrcode.min.js` - a plugin ships them as BinaryData |
| `Version.h` | `kCoreProtocol` - bump when a shared file or the wire format changes shape |
| `DESIGN.md` | the design, the rollout and every host test so far |

What does NOT belong here: RONE Control's DAW layer (`MidiDevicePort`, `FlBridge`, `ReaperBridge`, the FL /
Live / Bitwig scripts). Nothing in this folder may include a file from `RoneControl/`.

## The model lives once (and every rack installer carries it)

The MediaPipe hand model and its runtime (five files, 20 MB raw, ~8 MB compressed) are Google's, unchanged
(Apache 2.0, `NOTICE.txt`). RONE Control carries them as resources (`link_assets.rc`); a plugin with a rack
embeds nothing - its Windows installer puts the same five files into the shared `C:\Program Files\RONE
Plugins\Hands\` (the `hands` block in `installer/<Plugin>.iss`, filled by CI with `scripts/ci/fetch_hands.py`
when `Source/rack.json` exists; `uninsneveruninstall`, so the folder outlives any one plugin). `prepareLinkFolder`
(LinkView.cpp) fills `%APPDATA%\RonePlugins\Control\link\mp` in this order: a resource in the running module >
a file already there > a copy from the Hands folder. With none of the three the camera cannot start and the rack
shows the "HAND MODEL MISSING - reinstall from the Center" card (`openCenter` -> `RoneUpdates::launchCenterFor`).
The phone needs no model and works regardless; macOS plug-ins cannot own the camera, so Mac installers ship none.
Rack-first (Liran, 26.10.2026): the racks ship before RONE Control, so a rack plugin's camera must not depend on it.
Tests: `RONE_RACK_HANDS_DIR=<folder>` points a build at another Hands folder.

## LITE: slot A is free, the rest is ALL ACCESS

`Engine::refreshLite` (every 2 s) asks the same question every plugin's lock screen asks -
`RemoteLicenseGate::isOpenMode() || BundleLicenseChecker::isBundleLicensed()` - and when the answer is no, only
slot A (index 0) is stepped; slots B.. are shown as shipped with an amber `ALL ACCESS` badge, cannot be edited,
switched or removed, `+ KNOB` reads `+ KNOB · ALL ACCESS`, and a tap on any of them shows the "ONE KNOB IS FREE /
SEE ALL ACCESS" card (`openCenter {what:"allaccess"}`). A lifetime licence of the plugin itself does not open the
extra slots: they are the subscription's. `RONE_RACK_FORCE_LITE=1` shows the locked rack on a licensed machine (tests).
RONE Control itself has no LITE (it is not an Engine).

## REC before RONE Control ships

In FL, Live and Bitwig the REC button works through RONE Control's own DAW script. Until RONE Control is installed
and has run once there, the button is dimmed and says `<host>: REC comes with RONE Control` (RackTransport::why).
REAPER records directly through its API and needs nothing.

## Taking the rack into a plugin

```cmake
set(RACK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Shared/RoneControlRack")
juce_add_binary_data(<Plugin>RackData SOURCES "${RACK_DIR}/ui/rack.html" "${RACK_DIR}/ui/rack.css"
    "${RACK_DIR}/ui/rack.js" "${RACK_DIR}/ui/vendor/qrcode.min.js" "${CMAKE_CURRENT_SOURCE_DIR}/Source/rack.json")
target_sources(<Plugin> PRIVATE "${RACK_DIR}/RackEngine.cpp" "${RACK_DIR}/RackView.cpp"
    "${RACK_DIR}/RackEditorHelper.cpp" "${RACK_DIR}/RackTransport.cpp" "${RACK_DIR}/ControlCore.cpp"
    "${RACK_DIR}/HandLink.cpp" "${RACK_DIR}/LinkHub.cpp" "${RACK_DIR}/LinkView.cpp")
target_include_directories(<Plugin> PRIVATE "${RACK_DIR}")
target_link_libraries(<Plugin> PRIVATE <Plugin>RackData)
```

Then `Source/rack.json` (the manifest), `RoneRack::Engine` in the processor and `RoneRack::EditorHelper` in
the editor - RONE Throw's branch `rack-dev` is the reference. No `link_assets.rc` in the plugin.

## CI

`.github/workflows/main.yml` excludes `Shared/RoneControlRack/**` from every plugin's path filter: a change
here rebuilds and re-releases nothing by itself. A plugin that takes the rack adds the folder to ITS filter
on that day, so a rack fix reaches customers with that plugin's next release and no one else's.
