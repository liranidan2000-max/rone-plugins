# AFTERSPACE — Development Notes

Deviations from `RONE_AFTERSPACE_Claude_Code_Spec.md`, with reason and impact
(required by spec §37).

## Build status

MVP scaffold (spec §28): reverb engine, broadband duck, tempo-synced echo with
all three routings, freeze, bloom/character macros, full RONE Graphite main UI,
advanced drawer, 22 factory presets, state recall with schema versioning,
DSP smoke tests.

## Deviations / simplifications (MVP)

| Area | Spec | What was built | Reason / impact |
|------|------|----------------|-----------------|
| Smart Duck | §7 frequency-aware spectral ducking | High-quality broadband duck; SMART/FULL/VOCAL/TRANSIENT differ by detector filters + timing presets | Spec §7 explicitly allows this fallback for MVP. Parameter model (`duck.*`) already matches the future spectral version, so sessions won't break. TODO in `DuckProcessor.h`. |
| Echo INSIDE routing | §8 delay embedded/cross-fed into the late network | INSIDE = echo feeds the reverb input only (repeats are not audible directly, they dissolve into the space); BEFORE additionally keeps repeats audible in the wet mix | True in-network cross-feed deferred to V1 (spec §29). Audibly the three routings are materially different, which is the §31 acceptance bar. |
| Bloom | §9 late-tail envelope evolution | Macro mapping to modulation depth + density + width (+ character-scaled), per spec §28 "Bloom may initially map to modulation+density+width" | Bloom envelope (Bloom Delay/Rise/Color/Motion) is Phase-2 per §9. Public ID `main.bloom` is live and automatable now. |
| Echo time changes | §18 crossfade between taps preferred | Smoothed interpolated delay (~150ms ramp) → tape-style glide | Spec allows tape-style as intentional behavior. Crossfade-tap version can replace it without parameter changes. |
| Tail Capture / Reverse | §10 | Not built (Freeze only) | Explicit MVP recommendation in §10. |
| Quality modes | §21 ECO..ULTRA | Not built | V1 scope per §29. Current engine is light (8-line FDN). |
| A/B, undo/redo | §5 top bar | Not built | Marked "optional in early build" in §5. |
| UI resize | §5 strongly recommended | Implemented: proportional scaling 0.7x–1.6x, host frame + HTML corner grip, size persisted | See "Workflow features" below. |
| External sidechain | §7 | Not built | Explicitly not required for MVP; detector is the plugin input. |
| Preset browser | §13 categories + tags | Categories + Lock Mix implemented; tags/search not | 22 curated presets ship in the JS layer; tags become useful with a larger library (V1). |
| Wet auto-gain | §20 "level-compensated reasonably" | Fixed structural gains (1/√8 injection, tuned output tap gain) | No dynamic loudness compensation yet; tune by ear during listening pass, then consider RMS-based wet trim. |
| MIDI learn / text entry on values | §22 | Not built | MVP keeps drag/dblclick-reset/wheel/shift-fine. |

## UI design language

The UI follows the **current family design language** (taken from
`ReverseReverbVST/Resources/ui/style.css` — cyan — and
`rone-flanger-/Resources/ui/style.css` — magenta), not the older spec §4
palette: identical token set (`--ground/--panel/--card/--drawer/--line/--line2`
+ text scale), per-plugin neon accent (**AFTERSPACE = Nebula Orange #FF8A3D** — violet was taken by Stucker) with
glow, `.arck` arc knobs (value inside, label below, sizes s/m/l), `.chip`
overlays on the display card, neon-filled segmented controls, Sora/Manrope
type. This supersedes the spec's "no glow" note — the shipped family look wins.

## Workflow features (post-MVP additions, chosen 2026-08-31)

Mindset: built-in chain replacements that always improve the sound (like the
ducker) — reasons to open AFTERSPACE instead of a generic reverb + routing.

- **SILK** (`silk.amount`, main knob row): de-esser built into the wet path
  (`dsp/SilkProcessor.h`). 12dB/oct complementary split @4.6kHz, fast
  stereo-linked compression of the sibilance band, up to -15dB. Replaces the
  classic "de-esser after the reverb send" chain.
- **AUTO-GAIN** (`main.autogain`, toggle in Advanced/Space, default ON):
  analytic wet compensation ~sqrt(refT60/T60) x size term, clamped +/-9dB,
  smoothed 80ms. No envelope tracking -> can never pump.
- **MONO-LOW** (`stereo.monolow`, toggle in Advanced/Stereo, default ON): the
  side signal is low-cut at 150Hz (one-pole on S), so wet lows stay centered.
- **SOLO WET** (`main.solowet`, WET button in header): audition the space
  alone; crossfaded, never restored ON from session state, never in presets.
- **Resizable UI**: host frame + HTML corner grip -> `requestResize` event ->
  `setSize`; #app scales proportionally (0.7x–1.6x), size persisted in state.

### Parameter-model changes (pre-release, no released sessions to break)
- `space.density` + `space.diffusion` REMOVED -> merged into `space.texture`
  (drives both: diffusion 0.45+0.45t, density 0.40+0.60t).
- `space.earlylate` and `mod.rate` kept as automatable parameters but removed
  from the UI; `mod.depth` is displayed as "Motion".
- Added: `silk.amount`, `main.autogain`, `main.solowet`, `stereo.monolow`.
- Deliberately rejected for now (kept the UI lean): THROW button, duck GATE
  mode.

## House-pattern notes

- Follows `rone sync verb` architecture: JUCE 8.0.4 via FetchContent, WebView2
  UI served from `WebUI.h`, `Shared/BundleLicenseChecker.h` +
  `Shared/RoneAboutOverlay.h`, `CustomTitleBar` for standalone.
- Unlike sync verb, parameters use **APVTS** with dotted stable IDs
  (`main.size`, `duck.amount`, …) per spec §17 — the editor bridge is generic
  (one `setParameter` event handles every parameter), so adding a parameter is
  one line in `Parameters.h` + one knob config line in `WebUI.h`.
- Note for the bundle: sync verb's JS calls `emit("showAbout")` while its C++
  registers `showAbout` as a *native function* — that path is dead code there.
  AFTERSPACE registers About/Center/URL as event listeners on both sides.

## State

- Root tag `RoneAfterspaceState`, attribute `schemaVersion` = 1, `presetName`,
  child = APVTS tree. Migrations go in `setStateInformation`.
- `freeze.active` is forced OFF on restore (spec §34 risk: frozen buffers are
  not serialized).

## Tests

`Source/tests/DSPTests.cpp`, console target `AfterspaceTests`
(EXCLUDE_FROM_ALL — build with `cmake --build build --target AfterspaceTests`):
impulse tail decay at 44.1/48/96k, 3-minute freeze stability, max-feedback echo
boundedness, duck attenuate/release, odd block sizes.

## Release TODO (before beta)

- App icons (`Source/icon_1024.png` / `icon_256.png`) — CMake lines are ready.
- Add AFTERSPACE to `versions.json` + installer + CI (`.github/workflows/main.yml`)
  via the release pipeline.
- Listening pass: tune FDN base delays / diffusion coefficients / bloom scaling
  by ear (spec §16 "tuned by ear during development").
- `RONE_BETA_MODE` is active in `Shared/BundleLicenseChecker.h` (bundle-wide).
