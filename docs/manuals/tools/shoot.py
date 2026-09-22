"""Pixel-exact screenshots of the plugin web UIs with headless Edge.

Each UI is rendered inside an iframe of exactly the editor's size (harness.html),
so the letterboxed #app scaling every RONE plugin uses is at 1:1. The page's
_shot.js applies an optional state (clicks, JS) and posts the DOM rects of every
control back, which annotate.py uses to place the numbered callouts.

Edge is driven over DevTools (edge_cdp.py), not with --screenshot / --dump-dom:
since Edge 153 those flags return an un-stated page and an empty DOM. The
screenshot is taken only once _shot.js has posted the rects, so the state the
manual describes is the state in the picture, and the rects come from the very
same page - no second run, no transition-start offsets.

  python shoot.py                      all base states
  python shoot.py stutter flanger      some base states
  python shoot.py "stucker=_adv:click:advToggle" "flanger=_adv:click:adv-btn@680x640"
"""
import json, sys
from PIL import Image
from paths import UI, SHOTS, EDGE, PORT, ensure
from edge_cdp import Edge

# editor sizes (kBaseW x kBaseH from each PluginEditor.h; the Center is its default window)
SIZES = {"reversereverb": (700, 650), "stutter": (900, 600), "stucker": (440, 520),
         "flanger": (680, 520), "afterspace": (1150, 780), "choir": (1000, 750), "throw": (440, 560), "clipper": (340, 680),
         "center": (920, 640)}
DPR = 2

def shoot(name, suffix="", state="", size=None):
    ensure()
    w, h = size or SIZES[name.split("_")[0]]
    out = SHOTS / f"{name}{suffix}.png"
    url = f"http://127.0.0.1:{PORT}/harness.html?p={name}&w={w}&h={h}&state={state}"
    W, H = max(w, 600) + 20, max(h, 400) + 20
    with Edge(EDGE, W, H, DPR) as edge:
        edge.goto(url)
        # _shot.js: fonts ready -> 800 ms -> state applied -> 1200 ms -> rects posted to the harness
        raw = edge.wait_for("(function(){var s=document.getElementById('__rects');return s?s.textContent:''})()", timeout=40)
        rects = json.loads(raw)
        import time; time.sleep(0.4)          # let the last transition frame paint
        edge.screenshot(out, w, h)
    im = Image.open(out)
    if im.size != (w * DPR, h * DPR):
        im = im.crop((0, 0, min(im.size[0], w * DPR), min(im.size[1], h * DPR)))
        if im.size != (w * DPR, h * DPR): im = im.resize((w * DPR, h * DPR), Image.LANCZOS)
        im.save(out)
    (SHOTS / f"{name}{suffix}.rects.json").write_text(json.dumps(rects, indent=1), encoding="utf-8")
    print(f"  shot {name}{suffix} {im.size} rects={len(rects['rects']) if rects else 0}", flush=True)
    return out

def parse_spec(arg):
    """'name=suffix:state[@WxH]' or plain 'name'."""
    if "=" not in arg:
        return arg, "", "", None
    name, spec = arg.split("=", 1)
    suffix, state = spec.split(":", 1)
    size = None
    if "@" in state:
        state, sz = state.rsplit("@", 1); size = tuple(int(x) for x in sz.split("x"))
    return name, suffix, state, size

if __name__ == "__main__":
    for arg in (sys.argv[1:] or list(SIZES)):
        shoot(*parse_spec(arg))
