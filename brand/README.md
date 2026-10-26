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

Decisions behind it (26.10.2026): master brand = neutral (white R on graphite;
violet belongs to Stucker only), name = RONE PLUGINS, Throw = ice `#D8E4EC`
(print `#9FB3C4`), Bassline = indigo `#5B54E8`, 1176 keeps `#FF8C2E`.
