# RONE AFTERSPACE — Full Product & Development Specification

**Status:** Build specification for Claude Code
**Design language:** RONE Graphite

## 1. Executive Summary

Product name (working): AFTERSPACE
Brand family: RONE Bundle
Product type: Creative Reverb / Delay / Atmosphere Designer
Primary promise: Huge, cinematic, atmospheric spaces that stay clean in the mix with almost no routing work.

AFTERSPACE should feel simple enough for a producer to understand in 10 seconds, but deep enough to replace a multi-plugin send chain when the user wants advanced control.

The product is not positioned as “the most realistic room simulator”. Its value is musical scale, clarity, speed and creativity. It combines four ideas in one coherent workflow:
1. Large musical reverb.
2. Built-in intelligent ducking.
3. Delay that can live before, inside or after the reverb.
4. Atmosphere / bloom / tail-capture behavior for evolving sound design.

The interface must never expose the complete DSP complexity by default. The default view should communicate only: SIZE, DECAY, BLOOM, DUCK, ECHO, CHARACTER, MIX and FREEZE/CAPTURE.

## 2. Product Vision and Design Principles

VISION
A producer loads AFTERSPACE on a vocal, lead, synth, FX or atmosphere and gets a large expensive-sounding space immediately. The reverb should move out of the way while the source is active, then bloom into the gaps. The user should not need to create buses, sidechain compressors, EQs and separate delay sends for common workflows.

CORE PRINCIPLES
• Immediate: a good default sound with no setup.
• Musical: every control has a useful range; avoid dead zones.
• Clean by default: low-frequency buildup and masking should be controlled automatically.
• Deep on demand: advanced parameters exist behind progressive disclosure.
• One-screen mental model: no mandatory tab hopping.
• Visual feedback without visual noise.
• Automation-safe: no clicks, zipper noise or unstable transitions.
• CPU-aware: quality modes and graceful scaling.
• DAW-agnostic behavior across common sample rates and buffer sizes.

ANTI-GOALS
• Do not imitate a hardware room simulator UI.
• Do not expose 30 knobs on launch.
• Do not require sidechain input for basic ducking. The default ducking detector is the plugin input itself.
• Do not force a preset browser to achieve a usable sound.
• Do not use excessive neon, gradients, glow or decorative animation.

## 3. Target Users and Main Use Cases

PRIMARY USERS
• Electronic music producers, especially psytrance, trance, melodic techno, progressive, cinematic electronic and pop.
• Producers who like large vocals, leads, pads, risers, impacts and atmospheric transitions.
• Users who value speed over engineering-heavy routing.

TOP USE CASES
1. Big clean vocal reverb: long tail, strong ducking, moderate pre-delay.
2. Psytrance lead space: tempo-synced echo into a bright hall, ducked while the lead is active.
3. Huge transition atmosphere: long decay + bloom + wide modulation.
4. FX throw: automate Mix/Echo/Freeze at the end of a phrase.
5. Pad enlargement: subtle ducking, large size, high width, low modulation.
6. Tail creation: capture a reverb tail and turn it into a sustained texture.
7. Drum ambience: short clean room with transient ducking.

## 4. RONE Graphite Visual Language

The product must follow the RONE Graphite design system used across the bundle.

BASE PALETTE
• Main background: #14161A (soft graphite; never pure black).
• Secondary panels: approximately #181B20 to #1D2026.
• Raised surface: approximately #20242A.
• Hairline borders: white at ~6–10% opacity.
• Primary text: near-white, approximately #E8EAF0.
• Secondary text: approximately #9298A3.
• Disabled text: approximately #5F6570.

PLUGIN ACCENT
Recommended AFTERSPACE accent: restrained electric lavender / violet around #9A7BFF. It is an accent, not a background color.
Use accent for:
• active parameter rings,
• selected states,
• small focus indicators,
• waveform / space visualization highlights,
• current values,
• hover/focus affordances.

TYPOGRAPHY
• Sora or Manrope.
• Section labels: uppercase with increased letter-spacing.
• Parameter values can use tabular numerals where available.
• Avoid extremely small text. Primary controls must remain readable at 100% scaling.

KNOBS
• Charcoal body with subtle dimensional shading.
• Small white indicator dot / marker.
• Thin accent-colored value arc.
• No chrome / metallic look.
• Parameter name above, value below.

LAYOUT LANGUAGE
• Use spacing and luminance for hierarchy more than boxes.
• Borders only where they communicate grouping.
• Rounded corners subtle, not mobile-app bubbly.
• No bright colored fills for ordinary buttons.
• Hover: slightly brighter surface / text, not glow explosion.

## 5. Main UI Layout

The default interface should fit conceptually into three layers.

A. TOP BAR
Left: RONE mark + AFTERSPACE.
Center: preset name with previous/next arrows.
Right: A/B, undo/redo (optional in early build), MIX, BYPASS, settings menu.

B. HERO AREA
Large central spatial visualization occupying approximately 35–45% of the visible content area. Under/around it are the three main reverb controls:
• SIZE
• DECAY
• BLOOM

The visualization must react to parameter changes but remain subtle. It should support the mental model of a “space” growing, widening and evolving.

C. UTILITY AREA
Two primary compact modules:
• DUCK
• ECHO

Bottom row:
• FREEZE / CAPTURE
• CHARACTER slider: CLEAN <-> DREAM
• ADVANCED drawer toggle

Suggested default desktop dimensions: ~1100–1250 px wide x ~720–820 px high. Resize support is strongly recommended, with vector/scale-safe rendering.

## 6. Main Controls - Exact UX Behavior

SIZE
Purpose: perceived size of the space, not merely decay time.
UI range: 0–100%.
Default: ~65%.
DSP macro candidates: early reflection scale, diffusion geometry, modulation depth scaling, late-field density and stereo decorrelation.
UX: changing Size must not unpredictably change wet loudness.

DECAY
Purpose: tail duration.
Recommended display: seconds, with a musically curated nonlinear range, for example 0.20 s to 30 s plus optional “Infinite” region only when intentionally enabled.
Default: ~2.8 s for general preset, longer in atmosphere presets.
UX: smoothing required; large automation sweeps should not click.

BLOOM
Purpose: transforms a static reverb tail into an evolving atmospheric expansion.
UI range: 0–100%.
Default: ~25–35% in general presets.
At low values: minimal effect.
At medium values: later tail becomes wider, denser and more modulated.
At high values: increasingly atmospheric, optionally with subtle pitch/shimmer/granular contribution depending on architecture.
Important: Bloom is a macro, not one algorithm parameter.

MIX
0–100% wet/dry.
Optional context menu: Lock Mix While Browsing Presets.
For send usage, users should be able to set 100% wet without gain surprises.

CHARACTER: CLEAN <-> DREAM
Single horizontal macro.
CLEAN should bias toward tighter diffusion, lower modulation, controlled width, darker/cleaner tail and conservative echo feedback.
DREAM should progressively increase movement, late-field width, diffusion/softness, air, bloom contribution and creative echo/reverb interaction.
This must be mapped musically; it cannot simply be a linear blend of random parameters.

## 7. Ducking System - Core USP

GOAL
Make huge reverbs remain intelligible without requiring an external send + compressor + sidechain chain.

DEFAULT DETECTOR
The default detector is the dry input signal entering AFTERSPACE. External sidechain support may be added later, but it is not required for MVP.

MAIN UI
One large DUCK amount knob (0–100%) plus a mode dropdown.
Default mode: SMART.

MODES
1. SMART
Frequency-aware ducking designed to reduce masking around the source while leaving useful ambience alive.
2. FULL
Broadband wet-level ducking.
3. VOCAL
Preset detector / spectral weighting optimized for vocal intelligibility.
4. TRANSIENT
Fast detector emphasizing attacks; useful for drums/plucks.
5. CUSTOM
Exposes detailed Advanced controls.

ADVANCED DUCK PARAMETERS
• Amount / Range: maximum attenuation of wet path, recommended 0 to -24 dB.
• Attack: approximately 0.1–200 ms.
• Hold: 0–500 ms.
• Release: approximately 20 ms–3 s.
• Detector HPF / LPF.
• Sensitivity / threshold.
• Spectral focus / bands if spectral ducking is implemented.
• Optional external sidechain selector in future build.

SMART DUCK BEHAVIOR
Preferred architecture: split wet return into broad perceptual regions (for example Low / Mid / High or more detailed dynamic spectral bands). The detector estimates active source energy. Attenuation is stronger in frequency regions where the dry source is dominant, while upper atmosphere can remain partially present.

If true spectral processing is too heavy for MVP, implement a high-quality broadband duck first but preserve the API / parameter model so spectral ducking can be added without breaking sessions.

METERING
The Duck module should provide a restrained gain-reduction indication, not a giant compressor meter. Example: a thin animated arc or small waveform line showing attenuation.

## 8. Echo / Delay Engine

PURPOSE
Integrate the most common reverb+delay workflow without forcing a second plugin.

MAIN UI
• ECHO amount 0–100%.
• Time dropdown (tempo synchronized by default).

TIME OPTIONS
Suggested: 1/32, 1/16, 1/16D, 1/8, 1/8D, 1/4, 1/4D, 1/2, triplet variants, 1 bar, 2 bars, plus FREE ms.

ADVANCED ECHO PARAMETERS
• Feedback 0–95% with safe limiter/soft saturation in feedback loop.
• Stereo / Ping Pong mode.
• Stereo offset.
• Low Cut / High Cut in feedback path.
• Diffusion / smear.
• Modulation amount.
• Width.

ROUTING MODES - IMPORTANT DIFFERENTIATOR
1. BEFORE: Dry -> Delay -> Reverb. Echo repeats feed the space.
2. INSIDE: Delay is embedded in / cross-fed into the late reverb network. Repeats dissolve into the space rather than remain separate taps.
3. AFTER: Reverb -> Delay. The reverb tail itself echoes.

Default routing for general use: BEFORE or INSIDE depending on final sound quality.

SAFETY
Feedback must never become uncontrolled digital runaway. Add gain limiting / soft saturation and clamp unsafe combinations.

## 9. Bloom / Atmosphere Engine

BLOOM is the creative identity of AFTERSPACE.

CONCEPT
Bloom changes the temporal evolution of the late tail. Instead of the reverb only decaying, it can expand after the direct sound has passed.

POSSIBLE INTERNAL MAPPINGS
• Increase late diffusion over time.
• Increase stereo decorrelation over time.
• Slowly increase modulation depth.
• Shift damping / air balance in the late tail.
• Add a low-level shimmer or pitch-diffused component at high settings.
• Increase grain / smear contribution for atmosphere presets.

IMPORTANT MUSICAL RULE
Bloom must not feel like a cheap chorus. At moderate values, the user should perceive “the room opening” more than “pitch modulation”.

ENVELOPE
A useful implementation is a late-tail envelope: Bloom onset begins after a configurable internal delay related to pre-delay / source activity, then rises and decays smoothly.

ADVANCED PARAMETERS (optional after MVP)
• Bloom Delay.
• Bloom Rise.
• Bloom Color.
• Motion.
• Shimmer / Pitch Spread.

Only BLOOM itself appears on the main UI.

## 10. Freeze and Tail Capture

MAIN CONTROL
Single FREEZE button in the simple UI.

FREEZE
Captures/holds the current late-reverb state to create an infinite sustain.
Requirements:
• Click-free engage/disengage.
• Dry signal continues normally unless user chooses otherwise.
• Optional fade time internally to avoid abrupt transition.

EXPANDED ACTION MENU
• FREEZE
• CAPTURE TAIL
• REVERSE TAIL (future / creative feature)

CAPTURE TAIL
The user triggers Capture and the plugin grabs a short representation of the current wet tail, then sustains / loops / grains it as an atmosphere layer.
Potential controls in Advanced:
• Capture length.
• Pitch.
• Formant (if technically viable).
• Reverse.
• Grain size / texture.
• Filter.
• Width.

MVP recommendation: implement Freeze first. Tail Capture can be Phase 2 because it materially increases DSP and state complexity.

## 11. Advanced Drawer and Progressive Disclosure

The ADVANCED button should expand a drawer downward or increase plugin height. Avoid opening a completely separate floating window.

Suggested sections inside the drawer:

SPACE
• Pre-Delay
• Density
• Diffusion
• Early/Late balance
• Damping

DUCK
• Attack
• Hold
• Release
• Range
• Detector sensitivity
• Detector filters

TONE
• Low Cut
• High Cut
• Low Damp
• High Damp
• Air

MODULATION
• Rate
• Depth
• Random / smooth character

STEREO
• Width
• Early Width
• Late Width
• Motion

ECHO
• Feedback
• Routing: Before / Inside / After
• Ping Pong / Stereo
• Echo filters

RULES
• Main controls remain visible while Advanced is open when possible.
• Advanced controls update immediately and are automation-addressable.
• When a macro such as BLOOM changes multiple advanced parameters, show the resulting parameter positions only if this can be done without confusing automation ownership. Otherwise keep macro mappings internally independent.

## 12. Tooltips, Microcopy and Learnability

Tooltips should be one sentence, plain language, and appear after a short hover delay.

Examples:
SIZE: “Changes the perceived size of the space.”
DECAY: “Controls how long the reverb tail lasts.”
BLOOM: “Makes the late reverb expand into an evolving atmosphere.”
DUCK: “Moves the wet signal out of the way while the source is active.”
ECHO: “Blends tempo-synced echoes into the space.”
CHARACTER: “Moves the engine from tight and clean to wide and atmospheric.”
FREEZE: “Holds the current reverb tail.”

Avoid engineering jargon in default tooltips. Advanced controls can use more technical terminology.

## 13. Preset System

PRESET BROWSER CATEGORIES
• VOCALS
• LEADS
• ATMOSPHERES
• DRUMS
• PADS
• FX / THROWS
• HUGE SPACES
• CREATIVE

TAGS
• CLEAN
• DREAM
• DARK
• BRIGHT
• WIDE
• SHORT
• LONG
• DUCKED
• RHYTHMIC

PRESET HEADER
Current preset name in top bar with previous/next arrows.

PRESET RULES
• Presets store all plugin parameters except optionally locked parameters.
• Support “Lock Mix” while browsing.
• Later: Lock Duck, Lock Output, or lock groups if useful.
• Initial factory library should be curated, not huge: 40–80 excellent presets are more valuable than 300 repetitive ones.

EXAMPLE FACTORY PRESETS
Vocal: Huge Clean Vocal, Wide Pop Tail, Dream Vocal Throw, Dark Intimate Hall.
Lead: Psy Lead Space, Clean 1/8D Lead, Festival Hall, Acid Echo Space.
Atmosphere: Infinite Violet Cloud, Deep Temple, Blooming Sky, Frozen Air.
Drums: Tight Transient Room, Big Snare Bloom, Percussion Chamber.

## 14. DSP Signal Flow - Recommended Logical Architecture

This section describes the logical flow, not a mandatory algorithm implementation.

INPUT
1. Stereo input.
2. Input conditioning / optional DC removal.
3. Dry path retained for Mix output and duck detector.

WET GENERATION
4. Pre-delay.
5. Optional echo BEFORE stage.
6. Early reflections / initial diffusion.
7. Late reverb network.
8. Bloom / modulation / atmosphere layer.
9. Optional echo INSIDE or AFTER depending routing mode.
10. Wet tone shaping (HPF/LPF/damping).
11. Ducking gain stage or spectral duck stage.
12. Stereo width / output conditioning.

OUTPUT
13. Equal-power wet/dry Mix.
14. Output safety / denormal protection / optional soft clip only if required for feedback safety.

DUCK DETECTOR
Detector should generally read the dry input before wet processing. Detector filters and mode weighting alter the control signal, not the audible dry path.

FREEZE
Freeze should act primarily on the late reverb state / feedback network, not simply repeat an output buffer unless the chosen algorithm requires it.

## 15. Reverb Algorithm Requirements

The exact reverb algorithm is implementation-dependent, but it must satisfy these audible requirements:

• Smooth late tail without obvious metallic ringing at normal settings.
• Size changes should sound continuous and musical.
• Decay should be stable across sample rates.
• Stereo field should be wide but mono-compatible enough for practical production.
• Damping must prevent harsh long tails.
• Low frequencies should not create uncontrolled rumble.
• Modulation should reduce resonances without sounding obviously chorused at moderate settings.
• Freeze should remain stable indefinitely.

POSSIBLE IMPLEMENTATION APPROACHES
• FDN (Feedback Delay Network) with modulated delay lines.
• Diffusion stages + FDN.
• Hybrid algorithmic reverb with separate early and late networks.

Avoid convolution as the core identity unless used only for a specialty early-reflection layer; the product needs continuous size/decay/bloom manipulation and creative feedback behavior.

## 16. Parameter Specification - Suggested Ranges and Defaults

MAIN
Mix: 0–100%, default 30%.
Size: 0–100%, default 65%.
Decay: 0.20–30.0 s nonlinear, default 2.8 s.
Bloom: 0–100%, default 25%.
Duck Amount: 0–100%, default 45% for Huge Clean Vocal style preset, lower for Init.
Echo Amount: 0–100%, default 0% in Init.
Character: 0 Clean to 100 Dream, default 35%.

SPACE ADVANCED
PreDelay: 0–250 ms, default 20 ms; optional sync mode.
Density: 0–100%, default 70%.
Diffusion: 0–100%, default 70%.
Early/Late: -100 Early to +100 Late, default +30 Late.

TONE
Low Cut: Off / 20–1000 Hz, default 120 Hz.
High Cut: 2–20 kHz / Off, default 12 kHz.
Low Damp: 0–100%.
High Damp: 0–100%.
Air: -100 to +100 macro, default 0.

MOD
Rate: 0.01–5 Hz, default ~0.25 Hz.
Depth: 0–100%, default 15%.

STEREO
Width: 0–200%, default 120%.
Early Width: 0–200%, default 90%.
Late Width: 0–200%, default 130%.

DUCK
Range: 0 to -24 dB.
Attack: 0.1–200 ms.
Hold: 0–500 ms.
Release: 20–3000 ms.
Sensitivity: implementation-defined normalized parameter.

ECHO
Time: sync enum + free ms.
Feedback: 0–95%, default 25% when active.
Width: 0–200%.
Low Cut / High Cut similar to tone path.
Routing enum: Before / Inside / After.

All numerical ranges are starting recommendations and should be tuned by ear during development.

## 17. Parameter Smoothing and Automation

Every continuous DSP parameter that can cause discontinuities must be smoothed.

REQUIREMENTS
• No clicks when automating Mix, Size, Decay, Bloom, Duck or Echo.
• Avoid expensive topology rebuilds on the audio thread when a parameter moves.
• If Size changes delay-line lengths, use interpolation / crossfade / continuously modulated transitions.
• Freeze engage/disengage must crossfade.
• Preset changes should use short parameter ramps where appropriate.

AUTOMATION IDS
Parameter IDs must be stable from the first public version. Never use visible display names as the only identifier.
Suggested pattern:
main.mix
main.size
main.decay
main.bloom
main.character
duck.amount
duck.mode
duck.attack
...
echo.amount
echo.time
echo.routing
...

Once released, parameter IDs and normalized mappings should be treated as backwards-compatible API.

## 18. Tempo Sync and Host Integration

HOST DATA
• BPM.
• Transport play/stop where useful.
• Time signature if needed for bars.

TEMPO-SYNCED PARAMETERS
• Echo time.
• Optional pre-delay.
• Future modulation rates.

Behavior when host BPM is unavailable: use 120 BPM fallback or retain last known BPM, but never crash or output invalid timing.

Tempo changes during playback should transition smoothly for delay times where possible. Large synced-delay changes may require crossfading between delay taps to avoid pitch jumps unless a tape-style transition is an intentional mode.

## 19. Stereo, Mono and Phase Behavior

• Plugin accepts mono and stereo input configurations where supported by framework.
• Stereo output is preferred for the full experience.
• Mono-in/stereo-out should generate decorrelated stereo wet signal.
• Width = 0 should collapse the wet field safely.
• Avoid excessive anti-phase low-frequency content.
• Consider keeping frequencies below a configurable internal threshold more centered in the wet path.
• Test correlation at extreme Width and Dream values.
• Plugin must not radically change dry signal phase when Mix is 0%.

## 20. Gain Staging and Safety

• Unity-like dry path at Mix 0%.
• Wet path should be level-compensated reasonably across Size/Decay changes.
• Echo feedback must include stability protection.
• Freeze must not accumulate infinite DC or denormals.
• Protect against NaNs / infinities.
• Denormal prevention required in feedback networks.
• Avoid hard clipping unless it is an intentional internal safeguard; if used, it should not color normal operation.
• Consider optional Wet Output trim in Advanced if testing reveals a need.

## 21. CPU, Quality Modes and Performance

QUALITY menu can offer:
• ECO
• NORMAL
• HIGH
• ULTRA

Suggested behavior:
ECO: fewer diffusion stages / lower modulation oversampling / reduced atmosphere complexity.
NORMAL: default production mode.
HIGH: higher network density / improved interpolation.
ULTRA: offline-quality or high-end real-time mode, but must still be usable on modern systems.

REQUIREMENTS
• No allocations on the real-time audio thread after initialization.
• No locks/mutex waits on the audio thread.
• Background tasks must never block processing.
• DSP code should handle common sample rates: 44.1, 48, 88.2, 96, 176.4, 192 kHz if framework allows.
• Common block sizes: 16–4096 samples.
• State changes and GUI animations must not create audio spikes.

UI visualizer should decouple from audio rate and use a lightweight analysis buffer / atomic parameter snapshot.

## 22. UI Interaction Details

KNOBS
• Drag vertically by default.
• Shift-drag = fine adjustment.
• Double click = reset to default.
• Right click = context menu: reset, enter value, MIDI learn (if supported), automation info.
• Mouse wheel changes value only when hovered/focused, with sensible increments.

VALUES
• Clicking value may permit text entry.
• Seconds, ms, Hz, %, dB must display correct units.

FOCUS MODE
Optional enhancement: clicking a small expand icon on Duck or Echo focuses that module while other UI elements dim to ~30–40% opacity. This is optional, not required for MVP.

KEYBOARD
• Tab navigation should be predictable if accessibility support is implemented.
• Escape closes popovers.
• Enter confirms typed values.

RESIZE
Scale UI proportionally with sensible minimum size. Font and hit targets must remain readable.

## 23. Visualization Specification

The center visual is a spatial/energy visualization, not an audio analyzer that must be scientifically accurate.

VISUAL CHARACTER
• Soft particle/cloud/sphere/tunnel concept.
• Graphite background with subtle violet energy.
• No distracting full-screen animation.
• Motion should feel premium and slow.

PARAMETER MAPPINGS
• Size -> radius / scale.
• Decay -> persistence / trail lifetime.
• Bloom -> secondary expansion / particle diffusion.
• Width -> horizontal spread.
• Duck -> temporary contraction or brightness reduction while source is active.
• Freeze -> motion becomes suspended / stabilized.

PERFORMANCE
Target low GPU/CPU cost. Cap animation frame rate if needed. UI animation must never affect DSP.

ACCESSIBILITY / PREFERENCE
Provide Reduced Motion option in settings if animation is significant.

## 24. State Management and Session Recall

Plugin state must serialize:
• All parameter values.
• Selected preset or at least current parameter state.
• Quality mode.
• UI size.
• Advanced drawer open/closed state is optional.
• Freeze state: consider carefully. It may be safer to restore Freeze as off because captured DSP buffers can be large and non-deterministic. If capture-tail becomes a saved creative state, explicitly serialize required audio/state data.

STATE VERSIONING
Include a state schema version from day one. Migrate older state versions explicitly when parameters are added or mappings change.

## 25. Plugin Formats and Platform Targets

Recommended first targets for a Windows-based production workflow:
• VST3 64-bit.

Then:
• AU 64-bit for macOS.
• Standalone test host only if useful for development; not required as a commercial format.
• AAX later if there is real demand.

ARCHITECTURE
A cross-platform C++ audio framework such as JUCE is a practical choice. If the existing RONE bundle uses another framework, AFTERSPACE should match the existing codebase to maximize shared infrastructure.

Do not introduce a second UI/DSP framework solely for this plugin unless there is a strong technical reason.

## 26. Suggested Code Architecture

Exact names may differ, but keep DSP, parameter/state and UI layers separated.

Suggested modules:

/core
  PluginProcessor
  PluginEditor
  ParameterLayout
  StateSerializer
  HostSync

/dsp
  ReverbEngine
  EarlyReflections
  LateReverbFDN
  Diffuser
  ModulatedDelay
  BloomEngine
  DuckDetector
  DuckProcessor
  SpectralDuckProcessor (Phase 2)
  EchoEngine
  ToneProcessor
  StereoProcessor
  FreezeController
  SafetyLimiter

/ui
  ThemeRoneGraphite
  MainView
  TopBar
  SpaceVisualizer
  Knob
  MacroSlider
  DuckPanel
  EchoPanel
  AdvancedDrawer
  PresetBrowser
  TooltipManager

/presets
  PresetManager
  FactoryPresetData

/tests
  DSPTests
  StateRecallTests
  ParameterTests
  Render/Smoke tests where feasible

DSP engines should be independently testable without the GUI.

## 27. Threading Rules

AUDIO THREAD
• DSP only.
• No disk I/O.
• No network.
• No dynamic memory allocation during normal processing.
• No blocking locks.

MESSAGE/UI THREAD
• UI state.
• Preset browser interactions.
• File dialogs.
• Animation.

BACKGROUND THREAD
• Preset scanning if needed.
• Heavy capture processing if Tail Capture eventually requires it, provided real-time safety is maintained.

Parameter handoff between UI and DSP should use the framework's real-time-safe parameter system, atomics or lock-free mechanisms.

## 28. MVP Scope - Build This First

The first playable build should intentionally be smaller than the final vision.

MVP DSP
• High-quality stereo algorithmic reverb.
• Size.
• Decay.
• Pre-delay.
• Basic tone: Low Cut / High Cut / damping.
• Width.
• Modulation.
• Broadband internal-input ducking.
• Echo with tempo sync.
• Echo routing: BEFORE and AFTER first; INSIDE can follow if architecture is not ready.
• Mix.
• Freeze.

MVP UI
• RONE Graphite main screen.
• Top bar.
• Space visualization (simple placeholder is acceptable initially).
• Size / Decay / Bloom knob (Bloom may initially map to modulation+density+width).
• Duck module.
• Echo module.
• Character macro.
• Advanced drawer.

MVP PRESETS
10–20 presets sufficient for development validation.

MVP SUCCESS CRITERIA
A user can load the plugin, choose Huge Clean Vocal, hear a large clean reverb with ducking and synced echo, automate parameters without clicks, save/reopen the DAW session and get the same sound.

## 29. V1 Scope

After MVP stability:
• Smart / frequency-aware ducking.
• Echo INSIDE routing.
• Better Bloom envelope / late-tail evolution.
• Full preset browser with categories/tags.
• A/B compare.
• Undo/redo if shared framework supports it.
• Quality modes.
• Refined visualizer.
• More factory presets.
• Host edge-case testing.
• macOS/AU build if targeted.

## 30. Phase 2 Creative Features

Only after V1 quality is solid:
• Tail Capture.
• Reverse Tail.
• Shimmer / pitch diffusion layer.
• Granular atmosphere layer.
• External sidechain detector.
• MIDI trigger for Freeze/Capture.
• Modulation sync.
• Snapshot/morph system.

Do not allow these features to compromise the clarity of the default UI.

## 31. Acceptance Criteria - Audio

The build is not “done” because controls move. It must satisfy listening criteria.

REVERB
• No obvious metallic ringing at normal settings.
• Long tails remain smooth.
• Decay is predictable.
• Size feels perceptually meaningful.

DUCK
• At medium values, source intelligibility improves clearly.
• Release returns naturally without obvious pumping unless extreme settings are chosen.
• No clicks when detector engages.

ECHO
• Sync is musically correct.
• Feedback remains stable.
• Routing modes sound materially different.

BLOOM
• 0% behaves close to conventional reverb.
• 25–50% creates audible late-tail expansion without sounding like chorus.
• 100% becomes creative but remains controllable.

FREEZE
• Holds stable tail for at least several minutes with no drift into clipping/NaN.

MIX
• 0% = dry signal only.
• 100% = wet signal only.

## 32. Acceptance Criteria - UX/UI

FIRST 10 SECONDS TEST
A new user should identify Size, Decay, Bloom, Duck, Echo and Mix without reading documentation.

FIRST 60 SECONDS TEST
The user should be able to produce a large ducked vocal reverb and tempo-synced echo without opening Advanced.

VISUAL
• Graphite, not black.
• Violet accent used sparingly.
• No overlapping text at supported scale factors.
• Controls have consistent spacing and sizing.
• Values are legible.
• Hover/focus/active/disabled states are consistent.

ADVANCED
Opening Advanced should reveal depth without changing the mental model of the plugin.

## 33. Test Matrix

DAW / HOST
At minimum test in the main development DAW plus several common hosts if available.

SAMPLE RATES
44.1 / 48 / 96 kHz minimum; add others if framework target supports them.

BUFFER SIZES
32 / 64 / 128 / 256 / 512 / 1024 / 2048 samples.

CHANNEL CONFIGURATIONS
Mono -> stereo where supported.
Stereo -> stereo.

AUTOMATION STRESS
Rapid Mix automation.
Decay sweeps.
Size sweeps.
Duck amount + release automation.
Echo time changes while playing.
Freeze toggles.
Preset changes while stopped and while playing.

STATE
Save/reload session.
Duplicate plugin instance.
Copy channel / preset.
Open old state after schema change.

EDGE CASES
Zero input.
Very loud input.
Denormal-level input.
Long feedback.
Transport tempo changes.
BPM unavailable.
Unsupported bus layouts.

## 34. Known UX Risks and How to Avoid Them

RISK: Too many concepts.
Solution: only 6–7 primary controls visible.

RISK: Bloom is vague.
Solution: visual response + tooltip + carefully tuned macro so its sonic meaning is obvious.

RISK: Smart Duck feels unpredictable.
Solution: conservative default behavior, GR feedback, Custom mode for experts.

RISK: Delay makes UI crowded.
Solution: amount + time only on main UI; detailed routing in expansion/Advanced.

RISK: Preset changes destroy Mix.
Solution: Lock Mix option.

RISK: “Dream” macro creates unusable phasey sound.
Solution: curated nonlinear mapping and hard limits on dangerous width/modulation combinations.

RISK: Freeze state is confusing on recall.
Solution: default to restoring Freeze Off unless persistent capture is intentionally supported.

## 35. Suggested Initialization Preset

INIT - CLEAN SPACE
Mix: 25%
Size: 55%
Decay: 2.2 s
Bloom: 10%
Duck: 20%, Smart/Full fallback
Echo: 0%
Character: 25%
PreDelay: 20 ms
Low Cut: 120 Hz
High Cut: 12 kHz
Width: 115%
Mod Depth: 10–15%

The Init preset should sound good immediately on a vocal or lead and should not be overly dramatic.

## 36. Reference Mockup

The following mockup is a visual direction reference, not a pixel-perfect mandatory implementation. Preserve its hierarchy and simplicity more than its exact geometry.

Key takeaways from the reference:
• Strong top bar hierarchy.
• Large central visual with restrained violet energy.
• Three hero knobs.
• Duck and Echo as compact modules.
• Character macro and Freeze/Advanced on bottom.
• Minimal border usage.
• High perceived quality through spacing and typography rather than decorative complexity.

## 37. Instructions to Claude Code

Use this document as the product specification. Do not implement every Phase 2 feature in the first pass.

FIRST TASK
1. Inspect the existing RONE plugin codebase and identify the framework, shared theme components, parameter/state system and build pipeline.
2. Reuse existing RONE Graphite components wherever possible.
3. Create an AFTERSPACE branch/module with a minimal compilable plugin shell.
4. Implement parameter model with stable IDs before building the final UI.
5. Implement the MVP DSP signal path and unit/smoke tests.
6. Implement the main UI using the attached mockup as reference.
7. Only then add Advanced and presets.

IMPORTANT ENGINEERING RULES
• Do not perform real-time-unsafe work on the audio thread.
• Keep DSP independent from GUI.
• Keep parameter IDs stable.
• Introduce state schema versioning.
• Add TODO markers for deferred Smart Spectral Duck, Tail Capture and advanced Bloom rather than half-implementing them.
• Prefer a smaller stable implementation over a giant fragile feature set.
• Every control needs a defined parameter mapping and default.
• Any deviation from this spec should be documented in a DEVELOPMENT_NOTES.md file with reason and impact.

EXPECTED FIRST MILESTONE
A build that loads in the target DAW, processes stereo audio, provides Size/Decay/Mix/Duck/Echo/Width/PreDelay controls, saves state correctly and has the RONE Graphite main shell. Bloom and Character may initially use simpler mappings, but their public parameter IDs should already exist.

## 38. Definition of Done

AFTERSPACE V1 is ready for user testing when:
• It loads reliably in supported plugin format(s).
• It passes repeated session save/restore tests.
• No clicks or crashes occur under normal parameter automation.
• CPU is acceptable in Normal quality.
• Reverb quality is competitive enough to use in a real mix.
• Ducking audibly improves clarity.
• Echo routing is useful and stable.
• Bloom has a recognizable musical identity.
• UI matches RONE Graphite and remains simple at first glance.
• At least 40 curated factory presets exist.
• Basic documentation/tooltips are complete.
• Major parameters have stable IDs and normalized ranges.
• Test matrix has been run and known limitations are documented.
