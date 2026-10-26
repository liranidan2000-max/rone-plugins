# Shared/ui - the RONE UI kit v2 (26.10.2026)

The graphite house language as one stylesheet and one script, decided with Liran on the
controls-finish mockup (claude.ai/artifact/PL1XNPMJeg44Dqu24SAQXC):

- the knob is the `.arck` of Reverse Reverb / Flanger: value inside, white dot, arc outside, no
  ticks; sizes S 58 / M 104 / L 124 / XL 200 (hero). The lit arc glows only while touched
  (`.touch`, 0.5 s linger) and on the hero (`.hero`). A 3 % grain sits on the cap.
- keys (`.rone-btn`) are machined: no 1 px grey border, a top highlight and a dark bottom edge,
  ONE on-state (neon fill; `.hot` = red for BYPASS / KILL), `.act` carries a LED, `.dd` a chevron.
  A pressed key only sinks 1 px - no animation.
- only things that MOVE animate: the segment pill (`.rone-seg`, `RoneUI.seg`) glides 380 ms with
  the `--pop` overshoot, the switch thumb (`.rone-switch`) pops 320 ms.
- one popover for everything that opens: `.rone-menu` via `RoneUI.menu(trigger, {...})`;
  `RoneUI.select(selectEl)` wraps a native `<select>` (the OS list never shows again) and keeps
  `select.value` / `change` working.
- `.rone-chip` read-outs, `.rone-preset` bar, `.rone-grain` panel overlay, `.rone-label` (9 px, dim).

Paste both files into the page after `Shared/fonts/rone-fonts.css` (pages are single files served
from the binary; a WebUI.h takes them as extra raw-string parts under 14 KB each). Set the neon once:
`:root { --neon:#FF5FB8; --neon-rgb:255,95,184; }`. `demo.html` shows every piece - open it from disk.

Conversion recipe for an existing plugin: see the session brief of 26.10 (CONVERT.md) - keep ids,
handlers and layout, add the kit classes to the existing DOM, delete the old control CSS, call
`RoneUI.seg` / `RoneUI.select` once after the DOM exists, and verify headless with a screenshot.
