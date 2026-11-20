"""A key never moves out from under the mouse - a check for any RONE plugin page (RONE Iron 1.2.3, 20.11.2026).

The click goes where the mouse button is let go. Iron's LFO key was centred with transform:translateX(-50%) and the
kit's `.rone-btn:active{transform:translateY(1px)}` REPLACED that transform while the key was held: the key jumped 24 px,
the button was let go over another control and the click was lost (Windows WebView2 and macOS WebKit alike). The 1 px
sink itself lost a press on a key's top pixel row or a corner, and the header logo (hover 1.06, press .99) lost a press
on the right end of the name. The kit's fix (Shared/ui/rone-ui.css): an empty ghost of the key's box stays under the
mouse while it is held, and keys are never placed with a transform of their own.

This loads a page in headless Edge (the WebView2 engine) and presses every visible key - every .rone-btn, the logo, and
every element a CSS :active rule moves - on a grid of points with REAL mouse events (Input.dispatchMouseEvent, never
element.click(), which skips hit-testing), the page's own handlers blocked so nothing toggles. Each press must start and
end on the same key, no key may slide 2 px or more while held, and the element under the mouse must stay the same one
while it is held: WebKit (macOS) lets go of :active when it changes - from the name to the logo's ghost as the logo
shrinks, say - and the key pops back up mid-hold, often out from under the mouse. Edge keeps :active, so this check
finds the WebKit fault from here. Run it on the page after every UI change:

    python Shared/tests/press-hold/press_hold.py RoneClipper/Source/WebUI.h --size 340x680
    python Shared/tests/press-hold/press_hold.py rone-flanger-/Resources/ui/index.html --size 680x520 --view "document.querySelector('#adv-btn').click()"

<page> is a page file (index.html) or a Source/WebUI.h (the page is rebuilt from getIndexHTML()'s raw strings). Each
--view is a JS statement run on a fresh page before its sweep (open a drawer, a tab); the main page is always swept.
Exit 1 if a press is lost, a key slides, the element under the mouse changes while a key is held, or a press ghost
sits over the key's content (both make WebKit drop :active). Uses docs/manuals/tools/edge_cdp.py (the house CDP client).
"""
import argparse, functools, http.server, json, pathlib, re, shutil, sys, tempfile, threading, time

sys.dont_write_bytecode = True
MONO = pathlib.Path(__file__).resolve().parents[3]
sys.path.insert(0, str(MONO / "docs" / "manuals" / "tools"))
from edge_cdp import Edge                                    # the house CDP client (run, never edited)

EDGE = next((p for p in (r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
                         r"C:\Program Files\Microsoft\Edge\Application\msedge.exe") if pathlib.Path(p).exists()), None)
FX, FY = (0.03, 0.25, 0.5, 0.75, 0.97), (0.03, 0.5, 0.97)


def page_from_webui(h):
    """getIndexHTML(): every `inline std::string NAME()` = its raw strings and calls to the others, in source order."""
    src = pathlib.Path(h).read_text(encoding="utf-8"); funcs = {}
    for m in re.finditer(r"inline (?:const )?std::string\s*&?\s*(\w+)\(\)\s*\{", src):
        i = j = m.end(); depth = 1
        while depth:
            r = re.compile(r'R"(\w*)\(').match(src, j)
            if r: j = src.index(")" + r.group(1) + '"', r.end()) + len(r.group(1)) + 2; continue
            depth += {"{": 1, "}": -1}.get(src[j], 0); j += 1
        funcs[m.group(1)] = src[i:j - 1]
    def render(name, seen=()):
        body, out, j = funcs[name], [], 0
        for m in re.finditer(r'R"(\w*)\(|(\w+)\s*\(\s*\)', body):
            if m.start() < j: continue
            if m.group(0).startswith('R"'):
                end = body.index(")" + m.group(1) + '"', m.end()); out.append(body[m.end():end]); j = end + len(m.group(1)) + 2
            elif m.group(2) in funcs and m.group(2) not in seen: out.append(render(m.group(2), seen + (name,)))
        return "".join(out)
    return render("getIndexHTML")


class Handler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, *a): pass
    def handle(self):                                        # Edge drops its sockets on every navigation
        try: super().handle()
        except (ConnectionResetError, ConnectionAbortedError, BrokenPipeError): pass
    def translate_path(self, path):                          # the plugins serve the house fonts at fonts/<file>
        p = path.split("?")[0]
        if "/fonts/" in p: return str(MONO / "Shared" / "fonts" / p.rsplit("/", 1)[1])
        return super().translate_path(path)


# in the page: the candidates, and the probe that blocks the page's own handlers
FIND = r"""(() => {
  const GEO = /^(transform|translate|scale|rotate|top|left|right|bottom|inset|margin|padding|width|height|zoom)/;
  const split = s => { const o = []; let d = 0, c = ''; for (const ch of s) { if (ch === '(') d++; if (ch === ')') d--; if (ch === ',' && !d) { o.push(c.trim()); c = ''; } else c += ch; } if (c.trim()) o.push(c.trim()); return o; };
  const base = (s, pc) => { let j = s.indexOf(pc), d = 0; for (; j < s.length; j++) { const ch = s[j]; if (ch === '(') d++; else if (ch === ')') d--; else if (!d && ' >+~'.includes(ch)) break; } return s.slice(0, j).replace(/:active|:hover|::?(before|after)/g, '').trim() || '*'; };
  const rules = []; const walk = l => { for (const r of l) { if (r.selectorText !== undefined) rules.push(r); if (r.cssRules && r.cssRules.length) walk(r.cssRules); } };
  for (const ss of document.styleSheets) { try { walk(ss.cssRules); } catch (e) {} }
  const act = [], hov = [];
  for (const r of rules) { const ps = [...r.style].filter(p => GEO.test(p)); for (const s of split(r.selectorText)) {
    if (s.includes(':active') && ps.length) act.push(base(s, ':active'));
    if (s.includes(':hover') && ps.some(p => /^(transform|translate|scale)/.test(p))) hov.push(base(s, ':hover')); } }
  const set = new Set(document.querySelectorAll('.rone-btn, #pLogo, .p-logo, #p-logo, #brand'));
  for (const b of act) { try { document.querySelectorAll(b).forEach(e => set.add(e)); } catch (e) {} }
  const vis = e => { const r = e.getBoundingClientRect(), cs = getComputedStyle(e); if (r.width < 4 || r.height < 4 || cs.visibility === 'hidden' || e.disabled) return false;
    if (r.right < 0 || r.bottom < 0 || r.left > innerWidth || r.top > innerHeight) return false;
    const c = document.elementFromPoint(Math.min(innerWidth - 1, Math.max(0, r.left + r.width / 2)), Math.min(innerHeight - 1, Math.max(0, r.top + r.height / 2))); return !!c && (c === e || e.contains(c)); };
  const dur = e => { const cs = getComputedStyle(e), ps = cs.transitionProperty.split(','), ds = cs.transitionDuration.split(','); let m = 0;
    ps.forEach((p, i) => { p = p.trim(); if (['all', 'transform', 'translate', 'scale'].includes(p)) m = Math.max(m, parseFloat(ds[i % ds.length]) || 0); }); return m; };
  document.querySelectorAll('[data-ph]').forEach(e => delete e.dataset.ph);
  const L = [...set].filter(vis); L.forEach((e, i) => e.dataset.ph = i);
  if (!window.__ph) { window.__ph = {}; const stop = ev => { ev.stopImmediatePropagation(); if (ev.type === 'mousedown') __ph.down = ev.target; if (ev.type === 'click') { __ph.click = ev.target; ev.preventDefault(); } };
    ['mousedown', 'mouseup', 'click', 'pointerdown', 'pointerup', 'dblclick', 'auxclick', 'contextmenu'].forEach(t => addEventListener(t, stop, true)); }
  return JSON.stringify(L.map(e => ({ i: +e.dataset.ph, id: e.id ? '#' + e.id : e.tagName.toLowerCase() + '.' + [...e.classList].join('.'),
    txt: (e.textContent || '').replace(/\s+/g, ' ').trim().slice(0, 16), hover: hov.some(h => { try { return e.matches(h); } catch (x) { return false; } }), dur: dur(e) })));
})()"""
EL = "document.querySelector('[data-ph=\"%d\"]')"
HIT = "(() => { __ph.%s = document.elementFromPoint(%f, %f); return 1; })()"
# a press ghost (an absolute :active::after / ::before with content) must sit BEHIND the key's content: over an icon or a
# label it becomes the element under the mouse, and WebKit then lets go of :active (the key pops back up mid-hold)
GHOSTS = r"""(() => { const out = []; const walk = l => { for (const r of l) { if (r.selectorText !== undefined && /:active[^,]*::?(after|before)/.test(r.selectorText)
    && r.style.position === 'absolute' && r.style.content && r.style.content !== 'none' && !(parseInt(r.style.zIndex, 10) < 0)) out.push(r.selectorText);
    if (r.cssRules && r.cssRules.length) walk(r.cssRules); } };
  for (const ss of document.styleSheets) { try { walk(ss.cssRules); } catch (e) {} } return JSON.stringify(out); })()"""


def sweep(ws, w, h):
    mouse = lambda kind, x, y: ws.call("Input.dispatchMouseEvent", {"type": kind, "x": x, "y": y, "button": "none" if kind == "mouseMoved" else "left",
                                                                   "buttons": 1 if kind == "mousePressed" else 0, "clickCount": 1})
    rect = lambda i: json.loads(ws.eval("(() => { const e = %s; if (!e) return 'null'; const r = e.getBoundingClientRect(); return JSON.stringify([r.left, r.top, r.width, r.height]); })()" % (EL % i)))
    bad, presses = [], 0
    for k in json.loads(ws.eval(FIND)):
        hov, hold = (0.26 if k["hover"] else 0), max(0.11, k["dur"] + 0.06)
        lost, swaps, moved = [], [], 0.0
        for fy in FY:
            for fx in FX:
                mouse("mouseMoved", w - 2, h - 2); time.sleep(hov)
                if hov:                                      # a key that grows on hover is pressed where it is once hovered
                    r = rect(k["i"])
                    if not r: break
                    mouse("mouseMoved", r[0] + r[2] / 2, r[1] + r[3] / 2); time.sleep(hov)
                r0 = rect(k["i"])
                if not r0: break
                x, y = r0[0] + r0[2] * fx, r0[1] + r0[3] * fy
                if not ws.eval("(() => { const e = %s, h = document.elementFromPoint(%f, %f); __ph.down = __ph.click = null; return !!h && (h === e || e.contains(h)); })()" % (EL % k["i"], x, y)):
                    continue                                 # outside a rounded corner: not a press on the key
                mouse("mouseMoved", x, y); time.sleep(0.04 if hov else 0); ws.eval(HIT % ("h0", x, y))
                mouse("mousePressed", x, y); time.sleep(hold)
                r1 = rect(k["i"]); ws.eval(HIT % ("h1", x, y))
                mouse("mouseReleased", x, y); time.sleep(0.015)
                down, ok, same = json.loads(ws.eval("(() => { const e = %s; if (!e) return '[false,false,true]'; const t = __ph.click, d = __ph.down; return JSON.stringify([!!d && (d === e || e.contains(d)), !!t && (t === e || e.contains(t)), __ph.h0 === __ph.h1]); })()" % (EL % k["i"])))
                if not down or not r1: continue
                presses += 1
                if abs(r1[2] - r0[2]) < 1 and abs(r1[3] - r0[3]) < 1:   # a slide (a shrink is judged by the clicks alone)
                    moved = max(moved, abs(r1[0] - r0[0]), abs(r1[1] - r0[1]))
                if not ok: lost.append(f"{round(fx * 100)}/{round(fy * 100)}")
                elif not same: swaps.append(f"{round(fx * 100)}/{round(fy * 100)}")
        if lost or swaps or moved >= 2:
            bad.append(f"{k['id']} {k['txt']}: " + "  ".join(t for t in (f"{len(lost)} lost at {' '.join(lost)}" if lost else "",
                       f"moves {moved:.1f} px while held" if moved >= 2 else "",
                       f"the element under the mouse changes while held at {' '.join(swaps)} (WebKit lets go of :active)" if swaps else "") if t))
    return presses, bad


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("page"); ap.add_argument("--size", default="1000x700"); ap.add_argument("--view", action="append", default=[])
    a = ap.parse_args()
    w, h = map(int, a.size.lower().split("x"))
    src = pathlib.Path(a.page).resolve()
    tmp = None
    if src.suffix == ".h":
        tmp = pathlib.Path(tempfile.mkdtemp(prefix="presshold_")); (tmp / "index.html").write_text(page_from_webui(src), encoding="utf-8")
        root, name = tmp, "index.html"
    else:
        root, name = src.parent, src.name
    srv = http.server.ThreadingHTTPServer(("127.0.0.1", 0), functools.partial(Handler, directory=str(root)))
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    url = f"http://127.0.0.1:{srv.server_address[1]}/{name}"
    fails = 0
    try:
        with Edge(EDGE, w, h, 1) as ed:
            for view in [""] + a.view:
                ed.goto(url); ed.wait_for("document.readyState === 'complete'"); time.sleep(1.5)
                if view: ed.ws.eval(view); time.sleep(0.8)
                if not view:
                    over = json.loads(ed.ws.eval(GHOSTS))
                    print(f"{'[OK  ]' if not over else '[FAIL]'} press ghosts behind the key's content (z-index < 0): {len(over)} not")
                    for g in over: print('         ' + g)
                    fails += bool(over)
                n, bad = sweep(ed.ws, w, h)
                print(f"{'[OK  ]' if not bad else '[FAIL]'} {view or 'main page'}: {n} presses, {len(bad)} keys with a problem")
                for b in bad: print("         " + b)
                fails += bool(bad)
    finally:
        srv.shutdown()
        if tmp: shutil.rmtree(tmp, ignore_errors=True)
    sys.exit(1 if fails else 0)


if __name__ == "__main__":
    main()
