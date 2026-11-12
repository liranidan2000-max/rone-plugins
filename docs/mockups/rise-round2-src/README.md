# RONE Rise 1.2 - mockup sources (12.11.2026)

The mockup shows the REAL page of the dev branch (`RoneRise/Source/ui/index.html`, branch `dev-1.2`); nothing of
the plugin's screen is written here any more.

- `engine.js` - RoneRiseEngine + Shared/RoneReverbEngine + the processor, ported line for line to JavaScript,
  with the 1.2 additions (LAND 1 / 2, RISE / FALL, GAP, AUDITION, DRIVE macro) and `fixed` = the 1.2 landing.
  `LAND_BARS` is indexed by the LAND parameter: `[0, 4, 8, 16, 32, 1, 2]` (1 and 2 bars were appended).
- `test_engine.js` - `node test_engine.js`: null at 0, KILL LOW reach, the drop of 1.1.1 vs 1.2, GAP, FALL, cost.
- `build_mockup.py` - scopes the page's CSS into a frame and puts the engine behind the page's own bridge:
  `python build_mockup.py ../rise-round2.html [<page>]` (also writes `rise-round2-artifact.html`, the file the
  artifact is published from).
- `sun12.js` - the sun script of 1.2 (a character per mode) as it sits in the page; `apply_sun_12.py <page>` puts it
  back into the page after an edit. `apply_page_12.py` is the one-off that moved the round-2 controls into the
  1.1.1 page (kept for the record; it refuses to run twice).
- `index.html` - the page as released in Rise 1.1.1, for comparison only.

Artifact: https://claude.ai/artifact/Qxr8jpBkkgdvzLk6ZPTubU
