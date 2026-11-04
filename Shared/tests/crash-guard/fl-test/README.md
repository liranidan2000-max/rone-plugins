# The crash guard inside a real DAW

`guard_proof` answers everything a fake host can. These two plugins answer the rest: does the guard
leave a real DAW alone, and what happens when a plugin really crashes in it.

`build.ps1` makes a throwaway copy of Clipper, at the commit this branch points to, and builds it twice:

| Plugin | What it is |
|---|---|
| `RONE CrashTest Guard` | Clipper as a release gets it, plus two crash buttons above its window |
| `RONE CrashTest Plain` | The same with no guard, to compare with if the DAW dies |

Each has its own name and plugin code, so neither can replace an installed Clipper. The buttons exist
only in that copy. Nothing in the monorepo includes `CrashTest.h` and CI never builds this folder.

| Button | Where it crashes |
|---|---|
| UI crash | On the message thread, inside the click |
| Audio crash | On the host's audio thread, in the next `processBlock` |

Each has its own crash site, because the guard throttles by address. A third crash, on a thread of the
plugin's own where no host code can catch it, has no button and only runs with `RONE_CRASH_TEST=thread`:
it kills any host, and the Test Bench's `proof\crash-buttons.mjs` already proves it in a real JUCE host.

**MIDI.** A DAW that dies holding MIDI ports can wedge the Windows MIDI service, as repeated kills did on
2026-10-23. Every WinMM call then hangs on the whole PC and only a reboot helps. So run this by hand, once,
never in a loop, and if a DAW freezes on start afterwards, reboot before suspecting the guard.

## Installing

Into `C:\Program Files\Common Files\VST3\_CrashTest`, and into no other folder. The Test Bench takes
everything in `VST3\RONE` and anything at the top level named `RONE ...`, so that folder keeps the nightly
run away from a plugin that crashes on purpose. Delete the folder when the test is done.

## What each run should show

- Nothing pressed: the DAW scans, loads, opens, closes and quits as with any plugin.
- A button pressed: one `DAW_CRASH` report in `%APPDATA%\RonePlugins\CrashReports\pending`, or on its way
  through the Center, with `RONE CrashTest Guard` as the product. Restart the DAW between the two buttons.
- The guard reports at most once per process and once in 12 hours per crash, so a repeat needs a new
  process and an empty `throttle` folder.
- If the DAW dies, press the same button in `Plain`. Dying in both clears the guard. Dying only with it
  means the guard must not ship.
