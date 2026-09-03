"""Numbered callouts and detail crops on the screenshots, one set per manual.

Selectors are DOM ids ('#knob'), data-param names ('@smooth'), substrings of the
collected rect keys ('wave-btn'), unions ('#a+#b'), or explicit [x, y, w, h] boxes
in image pixels for native UIs (the Analyzer, the Center capture)."""
import json
from PIL import Image, ImageDraw, ImageFont
from paths import SHOTS, DOCIMG, FONT, ensure

ACCENT = {"stutter": "#FFD02B", "stucker": "#9D6BFF", "flanger": "#FF3D6E", "reversereverb": "#2BD9FF",
          "afterspace": "#FF8A3D", "analyzer": "#22E36B", "center": "#9D6BFF"}

def load_rects(name):
    p = SHOTS / f"{name}.rects.json"
    if not p.exists(): return {}, 2
    d = json.load(open(p, encoding="utf-8"))
    return (d or {}).get("rects", {}), (d or {}).get("dpr", 2)

def find(rects, sel):
    if isinstance(sel, (list, tuple)): return list(sel)
    boxes = []
    for part in sel.split("+"):
        if part.startswith("#") or part.startswith("@"):
            hits = [k for k in rects if k == part or (part.startswith("@") and k.startswith(part + "["))]
        else:
            hits = [k for k in rects if part in k]
        for k in hits: boxes.append(rects[k])
    if not boxes: return None
    x0 = min(b[0] for b in boxes); y0 = min(b[1] for b in boxes)
    x1 = max(b[0] + b[2] for b in boxes); y1 = max(b[1] + b[3] for b in boxes)
    return [x0, y0, x1 - x0, y1 - y0]

def annotate(img_name, out_name, items, accent, scale=None, crops=None, badge_scale=1.0):
    src = SHOTS / f"{img_name}.png"
    if not src.exists():
        print(f"  skip {out_name}: missing screenshot {src.name}"); return
    rects, dpr = load_rects(img_name)
    im = Image.open(src).convert("RGBA")
    k = scale if scale is not None else dpr
    ov = Image.new("RGBA", im.size, (0, 0, 0, 0)); d = ImageDraw.Draw(ov)
    R = int(19 * badge_scale * (im.width / 1400)) + 12
    font = ImageFont.truetype(FONT, int(R * 1.15))
    missing = []
    for n, (sel, _label) in enumerate(items, 1):
        explicit = isinstance(sel, (list, tuple))
        r = list(sel) if explicit else find(rects, sel)
        if r is None: missing.append(sel); continue
        if not explicit: r = [v * k for v in r]
        x, y, w, h = r
        pad = 4
        d.rounded_rectangle([x - pad, y - pad, x + w + pad, y + h + pad], radius=8, outline=accent + "B0", width=3)
        cx, cy = x - pad, y - pad
        d.ellipse([cx - R, cy - R, cx + R, cy + R], fill=accent, outline="#101216", width=3)
        t = str(n); bb = d.textbbox((0, 0), t, font=font)
        d.text((cx - (bb[2] - bb[0]) / 2 - bb[0], cy - (bb[3] - bb[1]) / 2 - bb[1]), t, font=font, fill="#101216")
    res = Image.alpha_composite(im, ov).convert("RGB")
    (DOCIMG / out_name).parent.mkdir(parents=True, exist_ok=True)
    res.save(DOCIMG / out_name, optimize=True)
    for cname, csel, cpad in (crops or []):
        explicit = isinstance(csel, (list, tuple))
        r = list(csel) if explicit else find(rects, csel)
        if r is None: missing.append(csel); continue
        if not explicit: r = [v * k for v in r]
        x, y, w, h = r; p = cpad * k
        box = (max(0, int(x - p)), max(0, int(y - p)), min(im.width, int(x + w + p)), min(im.height, int(y + h + p)))
        im.convert("RGB").crop(box).save(DOCIMG / out_name.replace(".png", f"_{cname}.png"), optimize=True)
    print(f"  {out_name}" + (f"  MISSING {missing}" if missing else ""))

def plain(img_name, out_name):
    src = SHOTS / f"{img_name}.png"
    if not src.exists():
        print(f"  skip {out_name}: missing screenshot {src.name}"); return
    (DOCIMG / out_name).parent.mkdir(parents=True, exist_ok=True)
    Image.open(src).convert("RGB").save(DOCIMG / out_name, optimize=True)

def main():
    ensure()
    # ------------------------------------------------------------ STUTTER
    A = ACCENT["stutter"]
    annotate("stutter_m_loaded", "stutter/tour.png", [
        ("#pLogo", "Header logo"), ("#fileName+#fileInfo", "Loaded file"), ("#browseBtn", "BROWSE"),
        ("#timelinePanel", "Waveform"), ("#transientNav", "PREV / NEXT"), ("#divisionBtns", "GRID"),
        ("#sliceMs", "SLICE"), ("#barsMinus+#barsPlus", "BARS"), ("#bpmInput+#bpmSource", "BPM"),
        ("#fadeInKnob", "FADE IN"), ("#fadeOutKnob", "FADE OUT"), ("#stereoKnob", "STEREO"),
        ("#playOrigBtn", "Play ORIGINAL"), ("#applyBtn", "STUTTER"), ("#playProcBtn", "Play RESULT"),
        ("#exportBtn", "EXPORT"), ("#statusText", "Status line"), ("#resize-handle", "Resize grip")], A,
        crops=[("grid", "#divisionBtns+#bpmSource", 14), ("knobs", "#fadeInArc+#stereoVal", 14), ("transport", "#playOrigBtn+#exportBtn", 12)])
    annotate("stutter_m_result", "stutter/result.png", [("#viewToggle", "ORIGINAL / RESULT")], A)
    plain("stutter", "stutter/empty.png"); plain("stutter_about", "stutter/about.png")
    # ------------------------------------------------------------ STUCKER
    A = ACCENT["stucker"]
    annotate("stucker", "stucker/tour.png", [
        ("#pLogo", "Header logo"), ("#bypassBtn", "BYPASS"), ("#syncSeg", "SYNC / FREE"), ("#gridLabel", "GRID"),
        ("#knob", "STUCK"), ("#loopMs", "LOOP"), ("#leds", "LEDs"), ("#advToggle", "ADVANCED"),
        ("#mixKnob", "MIX"), ("#resize-handle", "Resize grip")], A, crops=[("knob", "#stage", 10), ("header", "#syncSeg+#gridLabel", 10)])
    annotate("stucker_adv", "stucker/adv.png", [
        ("@smooth", "SMOOTH"), ("@range", "RANGE"), ("@lowcut", "LOW CUT"), ("#lenGroup", "LENGTH")], A,
        crops=[("panel", "#advPanel", 8)])
    plain("stucker_about", "stucker/about.png")
    # ------------------------------------------------------------ FLANGER
    A = ACCENT["flanger"]
    annotate("flanger", "flanger/tour.png", [
        ("#p-logo", "Header logo"), ("#p-prev+#preset-name+#p-next", "Preset browser"), ("#bypass", "BYPASS"),
        ("#comb-wrap", "Comb display"), ("#chip-mode", "COMB"), ("#chip-notch", "NOTCH"), ("#chip-bpm", "BPM"),
        ("#k-feedback", "FEEDBACK"), ("#k-delay", "SWEEP"), ("#k-mix", "MIX"), ("#t-again", "AUTO GAIN"),
        ("#inf-power", "INFINITE"), ("#inf-dir", "RISE / FALL"), ("#inf-rate", "Sweep length"),
        ("#gate-power", "GATE"), ("#gate-seg", "Gate rate"), ("#leds", "Gate LEDs"), ("#adv-btn", "ADVANCED"),
        ("#resize-handle", "Resize grip")], A,
        crops=[("display", "#comb-wrap", 10), ("knobs", "#main", 6), ("infinite", "#sweep", 6), ("gate", "#gate", 6)])
    annotate("flanger_adv", "flanger/adv.png", [("#k-stereo", "STEREO"), ("#k-output", "OUTPUT")], A, crops=[("drawer", "#drawer", 8)])
    plain("flanger_fx", "flanger/fx.png"); plain("flanger_about", "flanger/about.png")
    # ------------------------------------------------------------ REVERSE REVERB
    A = ACCENT["reversereverb"]
    annotate("reversereverb_m_loaded", "reversereverb/tour.png", [
        ("#p-logo", "Header logo"), ("#waveform-container", "Waveform"), ("#status-text", "Status"),
        ("#play-btn", "PLAY"), ("#mode-btn", "Mode"), ("#knob-lowcut", "LOW CUT"), ("#knob-width", "WIDTH"),
        ("#knob-tailDiv", "TAIL LENGTH"), ("#knob-roomSize", "ROOM SIZE"), ("#knob-gain", "DRY/WET"),
        ("#tremolo-power", "TREMOLO"), ("#resize-handle", "Resize grip")], A,
        crops=[("knobs", "#knobs", 8), ("buttons", "#button-row", 6)])
    annotate("reversereverb_m_trem", "reversereverb/tremolo.png", [
        ("#knob-tremDepth", "DEPTH"), ("#knob-tremPan", "PAN"), ("#knob-tremRate", "RATE"), ("wave-btn", "SHAPE"),
        ("#tremModeSeg", "MODE"), ("#trem-ramp-options", "FROM / TO")], A, crops=[("panel", "#tremolo-controls", 6)])
    plain("reversereverb_m", "reversereverb/empty.png"); plain("reversereverb_m_about", "reversereverb/about.png")
    # ------------------------------------------------------------ AFTERSPACE
    A = ACCENT["afterspace"]
    annotate("afterspace", "afterspace/tour.png", [
        ("#p-logo", "Header logo"), ("#p-prev+#preset-name+#p-next", "Preset browser"), ("#solowet-btn", "WET solo"),
        ("#bypass", "BYPASS"), ("#viz", "Visualiser"), ("#chip-spatial", "SPATIAL"), ("#chip-bpm", "BPM"),
        ("#chip-engine", "ENGINE"), ("#chip-duck", "DUCK GR"), ("#knob-size", "SIZE"), ("#knob-decay", "DECAY"),
        ("#knob-bloom", "BLOOM"), ("#knob-silk", "SILK"), ("#knob-mix", "MIX"), ("#duck-meter", "Duck envelope"),
        ("#knob-duck", "DUCK amount"), ("#sel-duckmode", "Duck MODE"), ("#echo-dots", "Echo display"), ("#knob-echo", "ECHO amount"),
        ("#sel-echotime", "Echo TIME"), ("#freeze-btn", "FREEZE"), ("#char-wrap", "CHARACTER"), ("#adv-btn", "ADVANCED"),
        ("#resize-handle", "Resize grip")], A, badge_scale=1.1,
        crops=[("knobs", "#main-knobs", 6), ("duck", "#duck-meter+#sel-duckmode", 10), ("echo", "#echo-dots+#sel-echotime", 10), ("bottom", "#bottom-row", 6)])
    annotate("afterspace_adv", "afterspace/adv.png", [
        ("#adv-space", "SPACE"), ("#adv-tone", "TONE"), ("#adv-mod", "MOTION & STEREO"), ("#adv-duck", "DUCK"),
        ("#adv-echo", "ECHO")], A, badge_scale=1.1, crops=[("drawer", "#adv-drawer", 6)])
    plain("afterspace_about", "afterspace/about.png")
    # ------------------------------------------------------------ ANALYZER (native UI, hand-placed boxes; capture is 3458x1398)
    A = ACCENT["analyzer"]
    annotate("analyzer_raw", "analyzer/tour.png", [
        ([26, 52, 216, 48], "Logo"), ([311, 60, 69, 32], "SOURCE"), ([406, 60, 75, 32], "LAYOUT"), ([512, 60, 38, 32], "FILE"),
        ([581, 60, 97, 32], "REFERENCE"), ([712, 59, 178, 34], "Source chip"), ([3000, 60, 233, 32], "Layout preset"),
        ([3251, 60, 65, 32], "RESET"), ([3337, 60, 43, 32], "App menu"), ([21, 130, 2005, 1236], "Totalyser"),
        ([1681, 144, 74, 24], "Band resolution"), ([1767, 144, 156, 24], "PEAK / HOLD / RTA"), ([1936, 144, 35, 24], "Stereo link"),
        ([1988, 144, 23, 24], "Panel settings"), ([2044, 130, 415, 1236], "Level Meters"), ([2476, 130, 956, 1236], "Vectorscope"),
        ([3230, 145, 150, 24], "STEREO / SQUARE"), ([2485, 1133, 947, 77], "Correlation meter")], A, scale=1, badge_scale=0.5)
    plain("analyzer_raw", "analyzer/main.png")
    # ------------------------------------------------------------ CENTER (1100x1000 render at DPR 2)
    A = ACCENT["center"]
    annotate("center_tall", "center/tour.png", [
        ([22, 165, 412, 396], "Navigation"), ([506, 27, 330, 72], "Account"), ([1331, 38, 143, 45], "Sync status"),
        ([1507, 27, 440, 67], "Search"), ([1985, 27, 67, 67], "Refresh"), ([2085, 27, 65, 67], "Settings"),
        ([508, 163, 1640, 244], "Updates summary"), ([1815, 244, 288, 81], "UPDATE ALL"), ([508, 440, 924, 60], "Filters"),
        ([1875, 440, 273, 60], "Sort"), ([508, 566, 1640, 402], "Plugin card"), ([2085, 605, 22, 27], "Card menu"),
        ([1925, 869, 193, 66], "OPEN"), ([1908, 1303, 210, 70], "INSTALL"), ([24, 1738, 410, 242], "Plan")], A, scale=1, badge_scale=0.75)
    plain("center_tall", "center/main.png")

if __name__ == "__main__":
    main()
