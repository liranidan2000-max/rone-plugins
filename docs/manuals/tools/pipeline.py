"""One command regenerates every RONE user manual from the plugin sources.

  python docs/manuals/tools/pipeline.py            everything (UIs -> screenshots -> callouts -> PDFs)
  python docs/manuals/tools/pipeline.py --no-native  skip launching the installed Analyzer / capturing native windows
  python docs/manuals/tools/pipeline.py --pdf-only    only rebuild the PDFs from the existing work/docimg images

Steps
  1. extract   the embedded WebUI.h HTML (Stutter, Stucker, AFTERSPACE, Choir) and copy the
               file-based UIs (Reverse Reverb, Flanger) and the Center's built React bundle into work/ui
  2. patch     add _shot.js (state + DOM rects) and, for the sample-based plugins and the Center,
               _mock.js (a fake JUCE bridge with real default values, a loaded file, plugin data)
  3. serve     a local static server on 127.0.0.1:PORT
  4. shoot     every UI state with headless Edge (pixel-exact, 2x)
  5. native    the Analyzer window (PrintWindow capture of the installed app) - optional
  6. annotate  numbered callouts + crops (annotate.py)
  7. build     HTML -> PDF (build_manuals.py)

Needs: Python 3 with Pillow, Microsoft Edge, network access for Google Fonts.
"""
import sys, shutil, subprocess, json, time, socket, os
from paths import ROOT, TOOLS, WORK, UI, SHOTS, EDGE, PORT, ensure
import extract_ui, shoot, annotate, build_manuals

PLUGINS = {  # served name -> (WebUI.h to extract | folder to copy)
    "stutter":    ROOT / "RoneStutter" / "Source" / "WebUI.h",
    "stucker":    ROOT / "RoneStucker" / "Source" / "WebUI.h",
    "afterspace": ROOT / "RONE AFTER SPACE" / "Source" / "WebUI.h",
    "choir":      ROOT / "RoneChoir" / "Source" / "WebUI.h",
    "reversereverb": ROOT / "ReverseReverbVST" / "Resources" / "ui",
    "flanger":       ROOT / "rone-flanger-" / "Resources" / "ui",
}

# every screenshot the manuals use: (served name, suffix, state, size)
STATES = [
    ("stutter", "", "", None), ("stutter", "_about", "click:pLogo", None),
    ("stutter_m", "_loaded", "js:window.__scn_stutter(false)", None), ("stutter_m", "_result", "js:window.__scn_stutter(true)", None),
    ("stucker", "", "", None), ("stucker", "_adv", "click:advToggle", None), ("stucker", "_about", "click:pLogo", None),
    ("flanger", "", "", None), ("flanger", "_adv", "click:adv-btn", None), ("flanger", "_fx", "click:inf-power,click:gate-power", None), ("flanger", "_about", "click:p-logo", None),
    ("reversereverb_m", "", "", None), ("reversereverb_m", "_loaded", "js:window.__scn_rr()", None),
    ("reversereverb_m", "_trem", "click:tremolo-power", None), ("reversereverb_m", "_about", "click:p-logo", None),
    ("afterspace", "", "", None), ("afterspace", "_adv", "click:adv-btn", None), ("afterspace", "_about", "click:p-logo", None),
    ("center", "_tall", "", (1100, 1000)),
]

def log(msg): print(msg, flush=True)

def prepare_ui():
    ensure()
    if UI.exists(): shutil.rmtree(UI)
    UI.mkdir(parents=True)
    for name, src in PLUGINS.items():
        dst = UI / name; dst.mkdir()
        if src.is_dir():
            for f in src.iterdir():
                if f.is_file(): shutil.copy(f, dst / f.name)
        elif src.exists():
            html = extract_ui.resolve("getIndexHTML", extract_ui.parse(src.read_text(encoding="utf-8")))
            (dst / "index.html").write_text(html, encoding="utf-8")
        else:
            log(f"  (no source for {name}: {src})"); continue
        log(f"  ui {name}")
    for f in ("harness.html", "_shot.js", "_mock.js"):
        shutil.copy(TOOLS / f, UI / f)
    # the Center: built React bundle + real plugin data from versions.json
    center = UI / "center"; center.mkdir()
    dist = ROOT / "RonePluginsCenter" / "Resources" / "ui" / "dist"
    shutil.copy(dist / "bundle.js", center / "bundle.js"); shutil.copy(dist / "styles.css", center / "styles.css")
    html = (dist / "index.html").read_text(encoding="utf-8").replace('src="/bundle.js"', 'src="bundle.js"').replace('href="/styles.css"', 'href="styles.css"')
    html = html.replace("<head>", '<head>\n<script src="/_mock.js"></script>\n<script src="/_center_mock.js"></script>', 1)
    (center / "index.html").write_text(html, encoding="utf-8")
    logos = UI / "logos"; logos.mkdir()
    v = json.loads((ROOT / "versions.json").read_text(encoding="utf-8"))
    plugins = []
    for p in v["plugins"]:
        icon = ROOT / "RonePluginsCenter" / "Resources" / f"{p['id']}_icon.png"
        if icon.exists(): shutil.copy(icon, logos / f"{p['id']}.png")
        status = "update_available" if p["id"] == "RoneFlanger" else ("not_installed" if p["id"] == "RONEAnalyzer" else "up_to_date")
        plugins.append({"id": p["id"], "name": p["name"], "description": p["description"], "remoteVersion": p["version"],
                        "installedVersion": p["version"] if status == "up_to_date" else ("2.0.0.147" if status == "update_available" else ""),
                        "status": status, "downloadProgress": 0, "formats": p["formats"], "type": p["type"], "whatsNew": p["whats_new"],
                        "logoUrl": "/logos/" + p["id"] + ".png", "hasStandalone": bool(p["standalone_exe"]),
                        "standaloneInstalled": status == "up_to_date", "hasManual": bool(p.get("manual"))})
    (UI / "_center_mock.js").write_text(
        "window.__mockNative.getPlugins = function () { return %s; };\n" % json.dumps({"plugins": plugins}) +
        "window.__mockNative.getLicenseStatus = function () { return { licensed: true, customerName: 'RONE User', licenseKey: 'RONE-ALL-ACCESS', message: '' }; };\n"
        "window.__mockNative.getAccountStatus = function () { return { signedIn: true, licensed: true, email: 'you@roneaudio.com', name: 'RONE User', plan: 'all-access', deviceLimit: 2, renewsAt: Date.now() + 21 * 86400000, expiresAt: 0, message: '' }; };\n"
        "window.__mockNative.getAppVersion = function () { return { version: '%s' }; };\n" % v["center_installer"]["version"], encoding="utf-8")
    # patch: _shot.js everywhere, _mock.js copies for the sample-based plugins (they need a backend to show defaults / a file)
    for d in UI.iterdir():
        f = d / "index.html"
        if not d.is_dir() or not f.exists(): continue
        html = f.read_text(encoding="utf-8")
        if "_shot.js" not in html:
            html = html.replace("</body>", '<script src="/_shot.js"></script>\n</body>', 1) if "</body>" in html else html + '\n<script src="/_shot.js"></script>'
        f.write_text(html, encoding="utf-8")
    for src in ("stutter", "reversereverb"):
        dst = UI / (src + "_m")
        shutil.copytree(UI / src, dst)
        f = dst / "index.html"
        f.write_text(f.read_text(encoding="utf-8").replace("<head>", '<head>\n<script src="/_mock.js"></script>', 1), encoding="utf-8")

class Server:
    def __enter__(self):
        self.p = subprocess.Popen([sys.executable, "-m", "http.server", str(PORT), "--bind", "127.0.0.1", "--directory", str(UI)],
                                  stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        for _ in range(50):
            try:
                with socket.create_connection(("127.0.0.1", PORT), timeout=0.2): return self
            except OSError: time.sleep(0.1)
        raise RuntimeError(f"static server did not start on {PORT}")
    def __exit__(self, *a):
        self.p.terminate()

def capture_native():
    exe = os.environ.get("ProgramFiles", r"C:\Program Files") + r"\RONE Plugins\RONE Analyzer.exe"
    if not os.path.exists(exe):
        log("  Analyzer not installed - keeping the previous capture if any"); return
    out = SHOTS / "analyzer_raw.png"
    r = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(TOOLS / "capture_window.ps1"),
                        "-Exe", exe, "-Out", str(out), "-WaitSec", "12", "-Maximize"], capture_output=True, text=True, timeout=120)
    log("  " + (r.stdout.strip().splitlines() or ["(no output)"])[-1])
    subprocess.run(["powershell", "-NoProfile", "-Command", "Get-Process | Where-Object { $_.ProcessName -like 'RONE Analyzer*' } | Stop-Process -Force"], capture_output=True)

def main(argv):
    pdf_only = "--pdf-only" in argv; native = "--no-native" not in argv
    if not pdf_only:
        log("1-2. preparing UIs"); prepare_ui()
        log("3-4. screenshots")
        with Server():
            for name, suffix, state, size in STATES:
                shoot.shoot(name, suffix, state, size)
        if native:
            log("5. native captures"); capture_native()
        log("6. callouts"); annotate.main()
    log("7. PDFs")
    ok = all(build_manuals.build(n) for n in build_manuals.ALL)
    log("done" if ok else "FAILED")
    return 0 if ok else 1

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
