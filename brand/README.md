# brand/

`brand.json` is the one place a RONE PLUGINS colour is decided: the graphite
neutrals, the fonts, and every product's neon (`screen`) plus its paper variant
(`print`, used by the PDF manuals when the screen colour vanishes on white).

```
python brand/brand_sync.py           # report what drifted, exit 1 if anything did
python brand/brand_sync.py --write   # fix it
```

It updates, as text and only the lines it owns:

| Surface | Where |
|---|---|
| Center card colour | `versions.json` `accent` (released plugins only) |
| Center fallback | `RonePluginsCenter/Resources/ui/src/catalog.js` `ACCENTS` |
| DEV channel cards | `<Plugin>/dev-channel.json` `accent` |
| PDF manuals | `docs/manuals/tools/content_*.py` `accent` (print colour) |
| Site | `../WEBSITE RONE AUDIO/assets/site.css` (`--c-*`, `.acc-*`), `fx.js`, `account.js`, `admin.js` |
| Social kit | `../WEBSITE RONE AUDIO/קידום/instagram/build.mjs` `ACC` |

Plugin UIs are not touched; each takes its colour with its own release.

This folder sits outside `Shared/` on purpose: anything under `Shared/**`
rebuilds every plugin in CI.

A new product: add it to `brand.json` first, then run `--write`.
A colour change: change it here, run `--write`, and the next PDF / site /
Center release carries it. Never type a product hex by hand anywhere else.

## Fonts and readable labels (phase B)

Every plugin page should carry its own type, so a studio with no network still
sees Sora / Manrope / IBM Plex Mono instead of Segoe UI:

1. `CMakeLists.txt`: `include(${CMAKE_CURRENT_SOURCE_DIR}/../Shared/fonts/RoneFonts.cmake)`
   and `rone_add_fonts(<Target>)`.
2. Editor `getResource()`: `#include "../../Shared/RoneFonts.h"`, then
   `if (auto font = roneFontResource (url)) return font;`.
3. The page: replace the `fonts.googleapis.com` link with a `<style>` holding
   `Shared/fonts/rone-fonts.css` verbatim.
4. Labels: nothing under 9 px on the design canvas, and `--faint` never colours
   text anyone has to read (2.3:1) - use `--dim` (4.5:1) or lighter.

`python brand/ui_check.py [Plugin]` shows where each plugin stands. RONE
Clipper was the first (26.10); the rest take it with their next release.

Decisions behind it (26.10.2026): master brand = neutral (white R on graphite;
violet belongs to Stucker only), name = RONE PLUGINS, Throw = ice `#D8E4EC`
(print `#9FB3C4`), Bassline = indigo `#5B54E8`, 1176 keeps `#FF8C2E`.
