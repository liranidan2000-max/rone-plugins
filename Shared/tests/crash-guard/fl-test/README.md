# The crash guard inside a real DAW

`guard_proof` answers everything a fake host can. These two plugins answer the rest: does the guard
leave a real DAW alone, and what happens when a plugin really crashes in it.

`build.ps1` makes a throwaway copy of Clipper, at the commit this branch points to, and builds it twice:

| Plugin | What it is |
|---|---|
| `RONE CrashTest Guard` | Clipper as a release gets it, plus a crash strip above its window |
| `RONE CrashTest Plain` | The same with no guard, to compare with if the DAW dies |

Each has its own name and plugin code, so neither can replace an installed Clipper. The strip exists
only in that copy. Nothing in the monorepo includes `CrashTest.h` and CI never builds this folder.

| Part of the strip | What to do | Where it crashes |
|---|---|---|
| Rest: UI | Rest the mouse on it, do not press | On the message thread |
| Rest: Audio | Rest the mouse on it during playback, do not press | On the host's audio thread, in the next `processBlock` |
| Press | Press it | Inside JUCE, in FL Studio only, see below |

Each has its own crash site, because the guard throttles by address. A crash on a thread of the plugin's
own, where no host code can catch it, has no part of the strip and only runs with `RONE_CRASH_TEST=thread`:
it kills any host, and the Test Bench's `proof\crash-buttons.mjs` already proves it in a real JUCE host.

**Why resting and not pressing.** The first FL test, on 2026-11-05, had two buttons, and FL died on the
mouse press with neither crash ever running. JUCE 8.0.4 takes keyboard focus on mouse down, Windows then
sends the window `WM_IME_SETCONTEXT`, and JUCE answers with `ImmIsUIMessage (h, ...)`, which sends the
same message to the same window. The stack runs out in under a thousand turns. The bench host lives
through the same loop: its messages go through the kernel, and the loop ends there by itself. In FL Studio
they stay in user mode and nothing ends it. Other DAWs are unknown. JUCE 8.0.12 has `break` on that line.
The press stays in the strip because it is a stack overflow with the fault in `user32`, the case that crash
taught the guard.

**MIDI.** A DAW that dies holding MIDI ports can wedge the Windows MIDI service, as repeated kills did on
2026-10-23. Every WinMM call then hangs on the whole PC and only a reboot helps. So run this by hand, once,
never in a loop, and if a DAW freezes on start afterwards, reboot before suspecting the guard.

## What FL Studio did, 26.1.7, 2026-11-05

| Part | FL Studio | Guard's report |
|---|---|---|
| Rest: UI | Its Error dialog, then it needs a restart | One, with the line |
| Rest: Audio | "crashed while processing. Disabled.", and it runs on | One, with the line |
| Press | Freezes and dies | One, a stack overflow pointing at the JUCE line |

Plain behaved the same in the first two, and in the press on the first test. FL lives through Rest: Audio,
so restart it before Press: the guard reports once per process. With nothing touched, FL quits normally.

Deleting the plugin after Rest: Audio hangs FL, in Guard and in Plain alike. JUCE's VST3 wrapper takes
`FLStudioDIYSpecificationEnforcementLock` in `process()` and in `setActive()`, in FL only. FL catches the fault,
the lock is never released, and `setActive()` waits for it for ever.

## Installing

Into `C:\Program Files\Common Files\VST3\_CrashTest`, and into no other folder. The Test Bench takes
everything in `VST3\RONE` and anything at the top level named `RONE ...`, so that folder keeps the nightly
run away from a plugin that crashes on purpose. Delete the folder when the test is done.

## What each run should show

- Nothing touched: the DAW scans, loads, opens, closes and quits as with any plugin.
- A crash: one `DAW_CRASH` report in `%APPDATA%\RonePlugins\CrashReports\pending`, or on its way through the
  Center, with `RONE CrashTest Guard` as the product. Restart the DAW between crashes.
- The guard reports at most once per process and once in 12 hours per crash, so a repeat needs a new
  process and an empty `throttle` folder.
- If the DAW dies, do the same in `Plain`. Dying in both clears the guard. Dying only with it means the
  guard must not ship.
- A DAW writes a crash dump to `%LOCALAPPDATA%\CrashDumps` when Windows is set to, and the guard's own
  variables are in it: `crash.code` shows whether its handler ran. The PDB has to carry the name the binary
  asks for, which on a Visual Studio build is `%%%RONE CrashTest Guard.pdb%%%`.
