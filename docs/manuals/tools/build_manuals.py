"""RONE user manuals: content modules -> HTML -> PDF (headless Edge).

  python build_manuals.py                 all manuals
  python build_manuals.py stutter         one manual

Reads the annotated images from work/docimg (see pipeline.py), embeds JPEG copies,
writes work/html/<plugin>.html for proofreading and the PDF next to this folder's
parent (docs/manuals/<Product> - User Manual.pdf) - the file the installers ship.
"""
import sys, pathlib, subprocess, tempfile, shutil, importlib, html as H, datetime
from PIL import Image
from paths import TOOLS, DOCIMG, DOCIMG_JPG, HTML, OUT, EDGE, ensure

sys.path.insert(0, str(TOOLS))
DATE = datetime.date.today().strftime("%B %Y")

CSS = r"""
@import url('https://fonts.googleapis.com/css2?family=Sora:wght@600;700;800&family=Manrope:wght@500;600;700;800&family=IBM+Plex+Mono:wght@400;500;600&display=swap');
@page { size: A4; margin: 17mm 16mm 18mm 16mm; }
:root { --accent: ACCENT; --ink: #16181C; --ink2: #3C4149; --dim: #7A7F88; --line: #E3E5E8; --paper: #FFFFFF; --panel: #F4F5F7; --graphite: #14161A; }
* { box-sizing: border-box; }
html, body { margin: 0; padding: 0; background: var(--paper); color: var(--ink); font-family: 'Manrope', 'Segoe UI', Arial, sans-serif; font-size: 10.6pt; line-height: 1.5; }
h1, h2, h3, h4 { font-family: 'Sora', 'Manrope', sans-serif; letter-spacing: -0.01em; margin: 0; }
h2 { font-size: 20pt; font-weight: 800; margin: 0 0 4mm 0; padding-bottom: 2.5mm; border-bottom: 2.5pt solid var(--accent); break-after: avoid; }
h2 .num { color: var(--accent); margin-right: 3mm; }
h3 { font-size: 12.5pt; font-weight: 700; margin: 6mm 0 2mm 0; break-after: avoid; }
h4 { font-size: 10.6pt; font-weight: 700; margin: 4mm 0 1.5mm 0; color: var(--ink2); text-transform: uppercase; letter-spacing: 0.08em; }
p { margin: 0 0 2.6mm 0; }
ul, ol { margin: 0 0 3mm 0; padding-left: 5.5mm; }
li { margin: 0 0 1.2mm 0; }
li::marker { color: var(--accent); font-weight: 700; }
strong { font-weight: 700; color: var(--ink); }
code, .mono { font-family: 'IBM Plex Mono', Consolas, monospace; font-size: 9.2pt; background: var(--panel); padding: 0.2mm 1.2mm; border-radius: 1mm; }
a { color: var(--ink); text-decoration: none; border-bottom: 1px solid var(--accent); }
section.page { break-before: page; }
section.page:first-of-type { break-before: auto; }
.keep { break-inside: avoid; }

/* cover: the dark ground is an absolutely positioned full-page box so it bleeds into the page margins */
.cover { color: #E8EAED; min-height: 262mm; padding: 9mm 4mm 2mm 4mm; position: relative; z-index: 0; display: flex; flex-direction: column; break-after: page; }
.cover .bg { position: absolute; top: -17mm; left: -16mm; width: 210mm; height: 297mm; background: var(--graphite); z-index: -1; overflow: hidden; }
.cover .brand { display: flex; align-items: center; gap: 4mm; }
.cover .brand svg { width: 22mm; height: auto; }
.cover .brand .wm { font-family: 'Sora'; font-weight: 800; font-size: 13pt; letter-spacing: 0.22em; color: #E8EAED; }
.cover .brand .wm small { display: block; font-size: 7.5pt; letter-spacing: 0.32em; color: var(--dim); font-weight: 600; margin-top: 1mm; }
.cover .title { margin-top: 30mm; }
.cover .eyebrow { font-size: 9pt; letter-spacing: 0.34em; color: var(--accent); font-weight: 700; }
.cover h1 { font-size: 40pt; font-weight: 800; line-height: 1.02; margin: 4mm 0 4mm 0; color: #FFFFFF; }
.cover h1 i { font-style: normal; color: var(--accent); }
.cover .tag { font-size: 13pt; color: #B9BEC7; max-width: 130mm; line-height: 1.4; font-weight: 500; }
.cover .shot { margin: 14mm auto 0 auto; border-radius: 3mm; overflow: hidden; border: 1px solid rgba(255,255,255,0.08); box-shadow: 0 18px 60px rgba(0,0,0,0.6), 0 0 0 1px rgba(255,255,255,0.03); max-width: 100%; max-height: 118mm; }
.cover .shot img { display: block; max-width: 100%; max-height: 118mm; width: auto; height: auto; }
.cover .foot { margin-top: auto; padding-top: 10mm; display: flex; justify-content: space-between; align-items: flex-end; font-size: 8.5pt; color: var(--dim); letter-spacing: 0.14em; text-transform: uppercase; }
.cover .foot b { color: #E8EAED; }
.cover .glow { position: absolute; right: -20mm; top: -20mm; width: 140mm; height: 140mm; border-radius: 50%; background: radial-gradient(circle, ACCENT33 0%, transparent 62%); pointer-events: none; }

/* toc */
.toc ol { list-style: none; padding: 0; margin: 0; column-count: 1; }
.toc li { display: flex; align-items: baseline; gap: 3mm; padding: 1.6mm 0; border-bottom: 1px dotted var(--line); font-weight: 600; font-size: 11pt; }
.toc li .n { color: var(--accent); font-family: 'Sora'; font-weight: 800; min-width: 8mm; }
.toc li .sub { color: var(--dim); font-weight: 500; font-size: 9.2pt; margin-left: auto; text-align: right; max-width: 90mm; }

/* figures */
figure { margin: 3mm 0 4mm 0; break-inside: avoid; }
figure img { width: 100%; display: block; border-radius: 2mm; border: 1px solid #23262C; }
figure.small img { width: 70%; margin: 0 auto; }
figure.w60 img { width: 60%; margin: 0 auto; }
figure.w80 img { width: 80%; margin: 0 auto; }
figcaption { font-size: 8.6pt; color: var(--dim); margin-top: 1.6mm; }
figcaption b { color: var(--ink2); }
.two { display: grid; grid-template-columns: 1fr 1fr; gap: 5mm; align-items: start; break-inside: avoid; }
.three { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 4mm; align-items: start; }

/* callout legend */
.legend { display: grid; grid-template-columns: 1fr 1fr; gap: 1mm 7mm; margin: 2mm 0 3mm 0; font-size: 9.4pt; }
.legend div { display: flex; gap: 2.2mm; align-items: baseline; padding: 0.7mm 0; border-bottom: 1px solid var(--line); }
.legend .b { display: inline-flex; align-items: center; justify-content: center; min-width: 5.2mm; height: 5.2mm; border-radius: 50%; background: var(--accent); color: var(--graphite); font-family: 'Sora'; font-weight: 800; font-size: 8pt; flex: none; position: relative; top: 0.6mm; }
.legend b { font-weight: 700; }

/* control reference blocks */
.ctl { display: grid; grid-template-columns: 34mm 1fr; gap: 2mm 5mm; margin: 0 0 3.2mm 0; padding: 2.8mm 3mm; background: var(--panel); border-left: 3pt solid var(--accent); border-radius: 0 2mm 2mm 0; break-inside: avoid; }
.ctl .name { font-family: 'Sora'; font-weight: 800; font-size: 10.2pt; letter-spacing: 0.06em; }
.ctl .meta { font-size: 8.3pt; color: var(--dim); margin-top: 1mm; line-height: 1.35; font-family: 'IBM Plex Mono', monospace; }
.ctl .body p { margin: 0 0 1.4mm 0; }
.ctl .tip { font-size: 9.2pt; color: var(--ink2); margin-top: 1mm; }

/* steps + notes */
.steps { counter-reset: s; list-style: none; padding: 0; margin: 0 0 3mm 0; }
.steps li { position: relative; padding-left: 10mm; margin: 0 0 2.6mm 0; }
.steps li::before { counter-increment: s; content: counter(s); position: absolute; left: 0; top: 0.2mm; width: 6.4mm; height: 6.4mm; border-radius: 50%; background: var(--graphite); color: var(--accent); font-family: 'Sora'; font-weight: 800; font-size: 9pt; display: flex; align-items: center; justify-content: center; }
.note { padding: 2.6mm 3.2mm 2.6mm 3.6mm; border-radius: 2mm; background: var(--panel); border-left: 3pt solid var(--accent); margin: 2.5mm 0 3.5mm 0; break-inside: avoid; font-size: 9.8pt; }
.note.dark { background: var(--graphite); color: #E8EAED; border-left-color: var(--accent); }
.note .t { font-family: 'Sora'; font-weight: 800; font-size: 8.4pt; letter-spacing: 0.18em; text-transform: uppercase; color: var(--accent); display: block; margin-bottom: 0.8mm; }
.recipe { border: 1px solid var(--line); border-radius: 2.4mm; padding: 3.2mm 3.6mm; margin: 0 0 4mm 0; break-inside: avoid; }
.recipe h4 { margin-top: 0; color: var(--ink); text-transform: none; letter-spacing: 0; font-size: 11pt; font-family: 'Sora'; }
.recipe .use { font-size: 8.8pt; color: var(--dim); margin: -1mm 0 2mm 0; }
table { width: 100%; border-collapse: collapse; margin: 1.5mm 0 4mm 0; font-size: 9.4pt; break-inside: avoid; }
th { text-align: left; font-family: 'Sora'; font-size: 8.2pt; letter-spacing: 0.12em; text-transform: uppercase; color: var(--dim); padding: 1.6mm 2mm; border-bottom: 1.5pt solid var(--ink); }
td { padding: 1.7mm 2mm; border-bottom: 1px solid var(--line); vertical-align: top; }
td:first-child { font-weight: 700; white-space: nowrap; }
.pill { display: inline-block; padding: 0.4mm 2mm; border-radius: 3mm; background: var(--graphite); color: var(--accent); font-family: 'Sora'; font-weight: 700; font-size: 7.8pt; letter-spacing: 0.1em; }
.dos { display: grid; grid-template-columns: 1fr 1fr; gap: 5mm; break-inside: avoid; }
.dos > div { padding: 3mm 3.4mm; border-radius: 2.4mm; background: var(--panel); }
.dos h4 { margin-top: 0; }
.dos ul { margin: 0; }
.spec td:first-child { width: 42mm; }
/* Chromium's print pipeline mirrors fixed offsets: top:-10mm lands 10mm below the content box, i.e. in the bottom margin */
.runfoot { position: fixed; left: 0; right: 0; top: -10mm; display: flex; justify-content: space-between; font-family: 'Sora'; font-size: 7pt; letter-spacing: 0.18em; text-transform: uppercase; color: #9AA0A8; }
.runfoot b { color: var(--accent); }
.footer-note { font-size: 8.6pt; color: var(--dim); margin-top: 6mm; border-top: 1px solid var(--line); padding-top: 2mm; }
.kbd { display: inline-block; font-family: 'IBM Plex Mono', monospace; font-size: 8.6pt; padding: 0 1.4mm; border: 1px solid #C9CDD3; border-bottom-width: 2px; border-radius: 1.2mm; background: #fff; }
"""

LOGO_SVG = """<svg viewBox="0 0 100 52" xmlns="http://www.w3.org/2000/svg" aria-label="RONE"><path fill="ACCENT" d="M 0 0 L 4.9 0 L 4.9 16.4 L 0 19.2 Z"/><path fill="none" stroke="ACCENT" stroke-width="4.2" stroke-linecap="round" stroke-linejoin="round" d="M 0 2.1 H 46.7 A 10.5 10.5 0 0 1 46.7 23.1 H 11.6 A 6.9 6.9 0 0 0 11.6 36.9 H 34 C 46 36.9 47 48.2 59 48.2 H 100"/></svg>"""

# ---------------------------------------------------------------- HTML helpers (the content modules receive these)
def img(path, caption="", cls=""):
    jp = DOCIMG_JPG / pathlib.Path(path).with_suffix(".jpg")
    p = (jp if jp.exists() else DOCIMG / path).as_uri()
    cap = f"<figcaption>{caption}</figcaption>" if caption else ""
    return f'<figure class="{cls}"><img src="{p}" alt=""/>{cap}</figure>'

def legend(items):
    cells = "".join(f'<div><span class="b">{i}</span><span><b>{H.escape(n)}</b>{(" &mdash; " + d) if d else ""}</span></div>' for i, (n, d) in enumerate(items, 1))
    return f'<div class="legend">{cells}</div>'

def ctl(name, meta, body, tip=""):
    t = f'<div class="tip"><b>Tip:</b> {tip}</div>' if tip else ""
    return f'<div class="ctl"><div><div class="name">{name}</div><div class="meta">{meta}</div></div><div class="body">{body}{t}</div></div>'

def note(title, body, dark=False):
    return f'<div class="note{" dark" if dark else ""}"><span class="t">{title}</span>{body}</div>'

def steps(items):
    return '<ol class="steps">' + "".join(f"<li>{s}</li>" for s in items) + "</ol>"

def recipe(title, use, body):
    return f'<div class="recipe"><h4>{title}</h4><div class="use">{use}</div>{body}</div>'

def table(headers, rows, cls=""):
    th = "".join(f"<th>{h}</th>" for h in headers)
    tr = "".join("<tr>" + "".join(f"<td>{c}</td>" for c in r) + "</tr>" for r in rows)
    return f'<table class="{cls}"><thead><tr>{th}</tr></thead><tbody>{tr}</tbody></table>'

# ---------------------------------------------------------------- shared sections
def install_section(m):
    win_rows = []
    if "VST3" in m["formats"]:
        win_rows.append(("VST3", f'<code>C:\\Program Files\\Common Files\\VST3\\RONE\\{m["vst3"]}</code>'))
    if m.get("exe"):
        win_rows.append(("Standalone app", f'<code>C:\\Program Files\\RONE Plugins\\{m["exe"]}</code>'))
    win_rows.append(("This manual", '<code>C:\\Program Files\\RONE Plugins\\Manuals\\</code>'))
    mac_rows = []
    if "VST3" in m["formats"]:
        mac_rows.append(("VST3", f'<code>/Library/Audio/Plug-Ins/VST3/{m["vst3"]}</code>'))
    if "AU" in m["formats"]:
        mac_rows.append(("Audio Unit", f'<code>/Library/Audio/Plug-Ins/Components/{m["au"]}</code>'))
    if m.get("exe"):
        mac_rows.append(("Standalone app", f'<code>/Applications/{m["exe"].replace(".exe", ".app")}</code>'))
    mac_rows.append(("This manual", '<code>/Users/Shared/RONE Plugins/Manuals/</code>'))
    body = f"""
<p>Every RONE product is installed, updated and licensed through one application: the <strong>RONE Plugins Center</strong>. You never download individual installers by hand, and you never enter serial numbers into the plugin itself.</p>
{img("center/tour.png", "<b>RONE Plugins Center.</b> 1 Navigation &middot; 2 Your account and licence &middot; 3 Sync status &middot; 4 Search &middot; 5 Refresh &middot; 6 Settings &middot; 7 Updates summary &middot; 8 UPDATE ALL &middot; 9 Filters &middot; 10 Sort &middot; 11 A plugin card &middot; 12 Card menu (Details, Open, Manual) &middot; 13 OPEN the standalone app &middot; 14 INSTALL &middot; 15 Your plan", "w80")}
<h3>Step by step</h3>
{steps([
 "Download the Center from <a href='https://roneaudio.com'>roneaudio.com</a> and run the installer (Windows: <code>RONE_Plugins_Center_Installer.exe</code>, macOS: <code>RONE_Plugins_Center.dmg</code>). On Windows the installer also adds the Microsoft WebView2 runtime if your PC does not have it yet; it is required for the plugin interfaces.",
 "Open the Center and sign in with your <strong>roneaudio.com account</strong> (the same e-mail and password you used on the website). Your plan and device slots are checked automatically; an active ALL ACCESS plan unlocks every plugin.",
 f"Find <strong>{m['product']}</strong> in the list and press <span class='pill'>INSTALL</span>. The Center downloads the signed installer, verifies its checksum, installs it silently and registers the version.",
 "When the card shows <span class='pill'>INSTALLED</span>, start (or restart) your DAW and rescan plugins if it does not pick up new plugins automatically. All RONE plugins appear next to each other in your plugin list because every name starts with <strong>RONE</strong>.",
 "Updates appear on the same card. Press <span class='pill'>UPDATE</span> on one plugin or <span class='pill'>UPDATE ALL</span> at the top. Close the standalone version of a plugin before updating it."])}
<h3>Where the files go</h3>
<div class="two">
<div><h4>Windows</h4>{table(["File", "Location"], win_rows)}</div>
<div><h4>macOS</h4>{table(["File", "Location"], mac_rows)}</div>
</div>
{note("Manual inside the Center", "Every installed plugin has a <strong>Manual</strong> entry in its card menu (the three dots on the card). It opens this PDF from the install folder, or from roneaudio.com if the local copy is missing.")}
<h3>Licence and activation</h3>
<p>Licensing is tied to your account, not to a key. The plugin checks the licence file that the Center writes when you sign in. If a plugin ever shows a <em>RONE Bundle License Required</em> screen, open the Center, make sure you are signed in and your plan is active, then reopen the plugin. Two devices can be signed in at the same time; sign out on an old machine from the Center's Account page to free a slot.</p>
<h3>Uninstalling</h3>
<p>Windows: Settings &rarr; Apps &rarr; <em>{m['product']} (RONE)</em>. macOS: delete the bundles listed above. Presets and settings in your user folder are kept.</p>
"""
    return {"title": "Installation with the RONE Plugins Center", "sub": "Center, account, file locations", "html": body}

def conventions_section(m, extras=""):
    body = f"""
<p>All RONE plugins share the same graphite interface language, so once you know one, you know them all.</p>
{table(["Gesture", "What it does"], [
 ("Drag a knob up/down", "Changes the value. The lit arc shows where you are in the range; the value is printed inside the knob."),
 ("Hold <span class='kbd'>Shift</span> while dragging", "Fine adjustment."),
 ("Double-click a knob", "Resets it to the default value."),
 ("Right-click a control (VST3)", "Opens your DAW's automation / parameter menu for that control (for example FL Studio's <em>Create automation clip</em>, <em>Link to controller</em>). RONE never shows a browser context menu."),
 ("Mouse wheel", "Steps through segmented switches and lists where the plugin offers them."),
 ("Drag the triangle in the bottom-right corner", "Resizes the window. The interface scales proportionally and keeps its aspect ratio."),
 ("Click the R logo / plugin name in the header", "Flips the plugin over to its <strong>back panel</strong>: version, format, licence holder, a link to roneaudio.com and a DEACTIVATE button for this device."),
])}
<div class="two">
{img(m['id'] + "/about.png", "<b>The back panel.</b> Click the header brand to open it; click CLOSE (or the panel edge) to flip back.", "")}
<div>
<h4>Automation</h4>
<p>Every knob and switch is a host parameter. Automate it from the DAW's automation lane or by right-clicking the control inside the plugin (VST3). Values shown in the plugin always match the DAW's.</p>
<h4>Presets and state</h4>
<p>The plugin's full state is saved with your DAW project, so a project reopens exactly as you left it. Where the plugin has its own preset browser (the name field with &#9664; &#9654; arrows), factory presets are built in and your own presets are stored in your user folder.</p>
<h4>Standalone version</h4>
<p>The standalone app is the same plugin in its own window, useful for auditioning and exporting without a DAW. It uses your system's default audio device; when there is no DAW clock, tempo-synced controls follow the plugin's own BPM setting.</p>
{extras}
</div></div>
"""
    return {"title": "Interface conventions", "sub": "knobs, right-click, resize, back panel", "html": body}

def support_section(m, trouble_rows):
    body = f"""
<h3>Troubleshooting</h3>
{table(["Symptom", "What to do"], trouble_rows + [
 ("The plugin does not appear in the DAW", "Rescan plugins. On Windows the VST3 lives in <code>C:\\Program Files\\Common Files\\VST3\\RONE</code>; make sure that folder (or its parent) is in the DAW's VST3 search path. On macOS restart the DAW after installing; Logic and GarageBand may need a full AU rescan."),
 ("The window is blank or shows an error page (Windows)", "The Microsoft WebView2 runtime is missing or damaged. Reinstall the RONE Plugins Center; it repairs WebView2. Also check that <code>WebView2Loader.dll</code> sits next to the standalone executable."),
 ("A licence screen appears", "Open the RONE Plugins Center, sign in, confirm your plan is active, then reopen the plugin. Check that you have not exceeded your device limit."),
 ("The plugin shows an older version than the Center", "Close every DAW and standalone RONE app, then press UPDATE again. A running standalone keeps the old executable locked."),
 ("Crash", "RONE plugins write a crash report to your user folder; the Center uploads it the next time it runs. Please also tell us what you did right before the crash."),
])}
<h3>Specifications</h3>
{table(["", ""], [
 ("Product", m["product"]),
 ("Version", f'{m["version"]} &nbsp;<span class="mono" style="color:#7A7F88">(manual revised {DATE})</span>'),
 ("Formats", " &middot; ".join(m["formats"]) + (" (Windows: VST3 + Standalone; macOS: VST3 + AU + Standalone)" if "AU" in m["formats"] else " (Windows: VST3 + Standalone; macOS: VST3 + Standalone)" if "VST3" in m["formats"] else "")),
 ("Systems", "Windows 10 / 11 (64-bit) &middot; macOS 11 or newer (Intel and Apple silicon, universal)"),
 ("Channels", m.get("channels", "Stereo in / stereo out")),
 ("Sample rates", m.get("srates", "44.1 kHz to 192 kHz")),
 ("Latency", m.get("latency", "None added by the plugin")),
 ("Engine", m.get("engine", "JUCE 8, WebView interface (WebView2 on Windows, WebKit on macOS)")),
], "spec")}
<h3>Support</h3>
<p>Website, presets, updates and support: <a href="https://roneaudio.com">roneaudio.com</a>. When writing to us, include the plugin version (shown on the back panel), your DAW and operating system, and the steps to reproduce.</p>
<p class="footer-note">{m['product']} is designed and coded in Israel by Liran &ldquo;RONE&rdquo; Kalifa. &copy; 2024&ndash;2026 RONE PLUGINS. All rights reserved. All product names are trademarks of their respective owners.</p>
"""
    return {"title": "Troubleshooting, specifications and support", "sub": "when something is off", "html": body}

# ---------------------------------------------------------------- assembly
def build_html(m):
    accent = m["accent"]
    css = CSS.replace("ACCENT33", accent + "33").replace("ACCENT", accent)
    logo = LOGO_SVG.replace("ACCENT", accent)
    secs = m["sections"]
    toc = "".join(f'<li><span class="n">{i}</span><span>{H.escape(s["title"])}</span><span class="sub">{H.escape(s.get("sub", ""))}</span></li>' for i, s in enumerate(secs, 1))
    body = "".join(f'<section class="page"><h2><span class="num">{i}</span>{H.escape(s["title"])}</h2>{s["html"]}</section>' for i, s in enumerate(secs, 1))
    return f"""<!doctype html><html><head><meta charset="utf-8"><title>{H.escape(m['product'])} - User Manual</title><style>{css}</style></head><body>
<div class="cover"><div class="bg"><div class="glow"></div></div>
 <div class="brand">{logo}<div class="wm">RONE PLUGINS<small>USER MANUAL</small></div></div>
 <div class="title"><div class="eyebrow">{H.escape(m['eyebrow'])}</div><h1>{m['title_html']}</h1><div class="tag">{m['tagline']}</div></div>
 <div class="shot">{img(m["cover_img"], "", "")}</div>
 <div class="foot"><span>Version <b>{m['version']}</b> &middot; {" &middot; ".join(m['formats'])}</span><span><b>roneaudio.com</b></span></div>
</div>
<div class="runfoot"><span><b>RONE</b> {H.escape(m['product'].replace('RONE ', ''))} &middot; User manual</span><span>roneaudio.com</span></div>
<section class="toc"><h2><span class="num">&nbsp;</span>Contents</h2><ol>{toc}</ol></section>
{body}
</body></html>"""

def make_jpegs():
    """JPEG copies of the annotated PNGs keep each PDF around 1.5-2 MB instead of 4-6 MB."""
    for p in DOCIMG.rglob("*.png"):
        out = DOCIMG_JPG / p.relative_to(DOCIMG).with_suffix(".jpg")
        if out.exists() and out.stat().st_mtime >= p.stat().st_mtime: continue
        out.parent.mkdir(parents=True, exist_ok=True)
        im = Image.open(p).convert("RGB")
        if im.width > 1800: im = im.resize((1800, int(im.height * 1800 / im.width)), Image.LANCZOS)
        im.save(out, "JPEG", quality=88, optimize=True, subsampling=0)

def to_pdf(html_path, pdf_path):
    prof = tempfile.mkdtemp(prefix="edgepdf_")
    args = [EDGE, "--headless=new", "--disable-gpu", "--no-first-run", "--disable-extensions", "--no-default-browser-check",
            f"--user-data-dir={prof}", "--no-pdf-header-footer", "--virtual-time-budget=15000", "--allow-file-access-from-files",
            f"--print-to-pdf={pdf_path}", html_path.as_uri()]
    subprocess.run(args, capture_output=True, text=True, timeout=240, encoding="utf-8", errors="replace")
    shutil.rmtree(prof, ignore_errors=True)
    return pdf_path.exists() and pdf_path.stat().st_size > 1000

def build(name):
    ensure(); make_jpegs()
    mod = importlib.import_module(f"content_{name}")
    m = mod.manual(globals())
    hp = HTML / f"{name}.html"
    hp.write_text(build_html(m), encoding="utf-8")
    pp = OUT / m["pdf"]
    ok = to_pdf(hp, pp)
    print(f"  {pp.name}: {'OK' if ok else 'FAILED'} {round(pp.stat().st_size / 1e6, 2) if ok else ''} MB")
    return ok

ALL = ["stutter", "stucker", "flanger", "reversereverb", "afterspace", "analyzer"]

if __name__ == "__main__":
    for n in (sys.argv[1:] or ALL): build(n)
