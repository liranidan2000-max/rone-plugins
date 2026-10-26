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
| `ui/` | `rack.html`, `rack.css`, `rack.js`, `vendor/qrcode.min.js` - a plugin ships them as BinaryData |
| `Version.h` | `kCoreProtocol` - bump when a shared file or the wire format changes shape |
| `DESIGN.md` | the design, the rollout and every host test so far |

What does NOT belong here: RONE Control's DAW layer (`MidiDevicePort`, `FlBridge`, `ReaperBridge`, the FL /
Live / Bitwig scripts). Nothing in this folder may include a file from `RoneControl/`.

## The model lives once

The MediaPipe hand model (~27 MB) rides only in RONE Control (`link_assets.rc`). `prepareLinkFolder` writes a
resource when the running module carries it, and otherwise accepts the file RONE Control already wrote into
`%APPDATA%\RonePlugins\Control\link\mp`. A plugin with a rack therefore embeds nothing; without RONE
Control installed its camera cannot start and the rack shows the "install RONE Control from the Center"
card (`openCenter` -> `RoneUpdates::launchCenterFor`). The phone needs no model and works regardless.

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
