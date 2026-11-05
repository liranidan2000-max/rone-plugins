# RONE Grain — work notes

Notes Liran collects while producing. Fixed together when he says "run".

## Fixed in 0.11.0 (2.11, RoneGrain 328946f, DEV 0.11.0.1)

1. **RESET button at the top** — one click returns every control to its starting
   point so the vocal plays as-is, as if the plugin were not acting on it at all
   (neutral / transparent sound). Put it in the top bar.
2. **Presets in the top bar** — a preset menu up top (house preset browser, like
   Stutter/Rise). Build a real set of great, useful presets for vocals and
   samples (stutters, stretches, shimmer/pad textures, rhythmic chops, wide pans,
   slow-motion freezes...), each one tuned by ear, not random values.
3. **HOLD TO PLAY → PLAY / STOP** — replace the hold-to-play button with a plain
   toggle: click PLAY to start, click STOP to stop (no need to keep the mouse
   held). MIDI notes keep working as before.
4. **ONE KNOB mode (macro)** — too many knobs (ATTACK, HOLD, GR. SPACING, WAVE
   SPACING, PAN...) when the user only wants "make it slow and blooming" or "make
   it fast and tight". A switch turns ONE KNOB on: the separate knobs lock and
   follow one big macro knob, which sweeps a single motion:
   - **Down side = slow & opening**: WAVE SPACING small → everything in slow
     motion; ATTACK long → each grain swells in softly; grains spread out.
   - **Up side = fast & tight**: ATTACK short/sharp, WAVE SPACING a bit faster
     (closer to real speed), grains packed close together.
   - Liran is not sure of the ideal mapping → build **several macro modes**
     (different curves / which knobs move and how far) so he can audition and
     pick; maybe ship the good ones as presets too.
   - Locked knobs still show their values moving with the macro (read-only),
     switching ONE KNOB off hands control back to the individual knobs.

Done: RESET + presets in the top bar (20 presets, groups CLEAN / SLOW / FAST / RHYTHM /
TEXTURE / ONE KNOB), PLAY / STOP, ONE KNOB with modes BLOOM / FREEZE / REVERSE / WIDE
(mapping in PluginProcessor.cpp kMacroEnds). Levels checked with tests/preset_levels.cpp:
all within about 2.5 dB of the dry sample (SPARSE DUST -4.8 dB, sparse by design).
Waiting for Liran's listen: which ONE KNOB mode feels right, and preset tweaks by ear.
