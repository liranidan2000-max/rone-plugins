# Shared/ui - the RONE UI kit (brand phase D)

The graphite house language of RONE Rise as one stylesheet and one small script,
so a new plugin starts from the same button, segment, chip, arc knob and glows
instead of drawing its own (the audit found about eight knob kits).

- `rone-ui.css` - neutrals from `brand/brand.json`, type roles (`.rone-label` is
  9 px in `--dim`, the floor), `.rone-btn`, `.rone-seg`, `.rone-chip`,
  `.rone-knob`, `.rone-glow-1/2/3`, `.rone-panel`. Set `--neon` and `--neon-rgb`
  once per plugin.
- `rone-ui.js` - `RoneUI.knob(el, {size, stroke, value, def, onChange})`: drag
  (shift = fine), wheel 2 %/notch, double-click = default; `.set(v)` from the host.
- `demo.html` - every piece, with a neon switcher. Open it from disk.

Paste both into the page after `Shared/fonts/rone-fonts.css` (pages are single
files served from the binary). New plugins use it from day one; an existing
plugin moves to it with its own next release, never all at once.
