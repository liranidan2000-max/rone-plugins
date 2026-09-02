"""Pixel-exact screenshots of the plugin web UIs with headless Edge.

Each UI is rendered inside an iframe of exactly the editor's size (harness.html),
so the letterboxed #app scaling every RONE plugin uses is at 1:1. The page's
_shot.js applies an optional state (clicks, JS) and posts the DOM rects of every
control back, which annotate.py uses to place the numbered callouts.

  python shoot.py                      all base states
  python shoot.py stutter flanger      some base states
  python shoot.py "stucker=_adv:click:advToggle" "flanger=_adv:click:adv-btn@680x640"
"""
import subprocess, json, re, sys, tempfile, shutil
from PIL import Image
from paths import UI, SHOTS, EDGE, PORT, ensure

# editor sizes (kBaseW x kBaseH from each PluginEditor.h; the Center is its default window)
SIZES = {"reversereverb": (700, 650), "stutter": (900, 600), "stucker": (440, 520),
         "flanger": (680, 520), "afterspace": (1150, 780), "choir": (1000, 750), "center": (920, 640)}
DPR = 2

def run(args, timeout=120):
    return subprocess.run(args, capture_output=True, text=True, timeout=timeout, encoding="utf-8", errors="replace")

def shoot(name, suffix="", state="", size=None):
    ensure()
    w, h = size or SIZES[name.split("_")[0]]
    out = SHOTS / f"{name}{suffix}.png"
    prof = tempfile.mkdtemp(prefix="edgeshot_")
    url = f"http://127.0.0.1:{PORT}/harness.html?p={name}&w={w}&h={h}&state={state}"
    W, H = max(w, 600) + 20, max(h, 400) + 20
    base = [EDGE, "--headless=new", "--disable-gpu", "--hide-scrollbars", "--no-first-run", "--disable-extensions",
            "--no-default-browser-check", f"--user-data-dir={prof}", f"--window-size={W},{H}",
            f"--force-device-scale-factor={DPR}", "--virtual-time-budget=8000", "--run-all-compositor-stages-before-draw"]
    run(base + [f"--screenshot={out}", url])
    im = Image.open(out); im = im.crop((0, 0, w * DPR, h * DPR)); im.save(out)
    r2 = run(base + ["--dump-dom", url])
    m = re.search(r'<script type="application/json" id="__rects">(.*?)</script>', r2.stdout, re.S)
    rects = json.loads(m.group(1)) if m else None
    (SHOTS / f"{name}{suffix}.rects.json").write_text(json.dumps(rects, indent=1), encoding="utf-8")
    shutil.rmtree(prof, ignore_errors=True)
    print(f"  shot {name}{suffix} {im.size} rects={len(rects['rects']) if rects else 0}")
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
