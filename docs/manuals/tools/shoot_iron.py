"""RONE Iron's manual screenshots.

Iron's page shows a vocal only through its own demo block (ui-preview.html, used when there is no
JUCE bridge), which is posed with URL options (?state=ready&gp=1 ...) rather than _shot.js states,
so it has its own harness (harness_iron.html passes the demo query through) and its own state list.
pipeline.py calls prepare() after prepare_ui() and shoot_all() inside its server.

  python shoot_iron.py              every Iron state (starts its own server)
  python shoot_iron.py iron_adv     one state
"""
import os, sys, json, time, pathlib
TOOLS = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))
from paths import UI, SHOTS, EDGE, PORT, ensure
from edge_cdp import Edge
from PIL import Image

IRON = TOOLS.parents[2] / "RoneIron"
W, H, DPR = 1000, 684, 2
STATES = {   # name -> demo query
    "iron":         "state=ready&groove=1",
    "iron_empty":   "state=empty",
    "iron_groove":  "state=ready&groove=1&gp=1&gdemo=1",
    "iron_arp":     "state=ready&arp=1&notes=0,4&order=0",
    "iron_adv":     "state=ready&adv=1&tone=0.41&level=0.3&lowcut=150",
    "iron_presets": "state=ready&iplist=1&slots=1,3",
    "iron_locks":   "state=ready&locks=iron,ring,mix&ipreset=4&premod=1",
    "iron_about":   "state=ready&about=1",
}

def prepare():
    ensure()
    d = UI / "iron"; d.mkdir(parents=True, exist_ok=True)
    html = (IRON / "ui-preview.html").read_text(encoding="utf-8")
    html = html.replace("</body>", '<script src="/_shot_iron.js"></script>\n</body>', 1) if "</body>" in html else html + '\n<script src="/_shot_iron.js"></script>'
    # the demo block still carries the round-6 preset table: show the shipped 15 (docs/iron-presets.json)
    import re
    real = json.loads((IRON / "docs" / "iron-presets.json").read_text(encoding="utf-8"))
    rows = ",\n".join("    { name: %s, category: %s, hint: %s, p: {}, g: 0, m: [0, 0, 0, 0], t: T0 }"
                       % (json.dumps(x["name"]), json.dumps(x["category"]), json.dumps(x["hint"])) for x in real)
    html, n = re.subn(r"const IP = \[\n.*?\n  \];", lambda m: "const IP = [\n" + rows + "\n  ];", html, count=1, flags=re.S)
    assert n == 1, "demo preset table not found"
    (d / "index.html").write_text(html, encoding="utf-8")
    shot = (TOOLS / "_shot.js").read_text(encoding="utf-8").replace("q.get('state')", "q.get('shot')")
    (UI / "_shot_iron.js").write_text(shot, encoding="utf-8")
    (UI / "harness_iron.html").write_text("""<!DOCTYPE html><html><head><meta charset="utf-8"><style>html,body{margin:0;background:#000;overflow:hidden}iframe{border:0;display:block}</style></head>
<body><script>
const q = new URLSearchParams(location.search);
const w = +q.get('w'), h = +q.get('h');
const f = document.createElement('iframe');
f.width = w; f.height = h; f.style.width = w + 'px'; f.style.height = h + 'px';
f.src = '/iron/index.html?' + (q.get('dq') || '') + '&shot=';
document.body.appendChild(f);
window.addEventListener('message', e => {
  if (!e.data || e.data.type !== 'rects') return;
  const s = document.createElement('script'); s.type = 'application/json'; s.id = '__rects';
  s.textContent = JSON.stringify(e.data.payload); document.body.appendChild(s);
});
</script></body></html>""", encoding="utf-8")

def shoot(name, dq):
    import urllib.parse
    out = SHOTS / f"{name}.png"
    url = f"http://127.0.0.1:{PORT}/harness_iron.html?w={W}&h={H}&dq={urllib.parse.quote(dq)}"
    with Edge(EDGE, W + 20, H + 20, DPR) as edge:
        edge.goto(url)
        raw = edge.wait_for("(function(){var s=document.getElementById('__rects');return s?s.textContent:''})()", timeout=40)
        time.sleep(0.6)
        edge.screenshot(out, W, H)
    rects = json.loads(raw)
    im = Image.open(out)
    if im.size != (W * DPR, H * DPR):
        im = im.crop((0, 0, W * DPR, H * DPR)); im.save(out)
    (SHOTS / f"{name}.rects.json").write_text(json.dumps(rects, indent=1), encoding="utf-8")
    print(f"  shot {name} {im.size} rects={len(rects['rects'])}", flush=True)

def shoot_all(which=None):
    for n in (which or list(STATES)): shoot(n, STATES[n])

if __name__ == "__main__":
    import pipeline
    prepare()
    with pipeline.Server():
        shoot_all(sys.argv[1:])
