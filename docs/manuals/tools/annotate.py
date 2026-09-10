"""Numbered callouts and detail crops on the screenshots, one set per manual.

Selectors are DOM ids ('#knob'), data-param names ('@smooth'), substrings of the
collected rect keys ('wave-btn'), unions ('#a+#b'), or explicit [x, y, w, h] boxes
in image pixels for native UIs (the Analyzer, the Center capture)."""
import json
from PIL import Image, ImageDraw, ImageFont
from paths import SHOTS, DOCIMG, FONT, ensure

ACCENT = {"stutter": "#FFD02B", "stucker": "#9D6BFF", "flanger": "#FF3D6E", "reversereverb": "#2BD9FF",
          "afterspace": "#FF8A3D", "analyzer": "#22E36B", "center": "#9D6BFF",
          # Throw's on-screen accent is Ice White #D8E4EC, which vanishes on paper; the
          # manual uses this printable ice-grey for badges and headings instead.
          "throw": "#9FB3C4"}

# The analyzer's callouts, by selector. pipeline.capture_native() measures these
# against the running app and writes the boxes; nothing here is a coordinate, so
# a control that moves takes its label with it.
# Selector -> (badge label, what the manual says about it).
#
# pipeline.capture_native() measures these against the running app and writes
# the boxes, and content_analyzer builds its legend from the same dict. Nothing
# here is a coordinate and nothing is duplicated: legend() numbers by position,
# so a second hand-kept list in a different order would silently give every
# badge somebody else's description. That is what happened to the old
# eighteen-entry list when the UI changed.
ANALYZER_CALLOUTS = {
    "#brand":
        ("Logo", "the R mark opens About - there is no ABOUT button"),
    ".readbar":
        ("Source readout", "current source, channel pair and sample rate; "
                           "the dot lights while audio is arriving"),
    '.chip[data-act="source"]':
        ("SOURCE", "choose the input: ASIO, loopback, live input or DAW master"),
    '.chip[data-act="channels"]':
        ("CH", "which pair of a multichannel device to analyse"),
    '.chip[data-act="file"]':
        ("FILE", "open and play in real time, open and scan a whole file in "
                 "seconds, or loop"),
    '.chip[data-act="reference"]':
        ("REF", "add reference tracks and show or clear the reference corridor "
                "behind the analyser"),
    '.chip[data-act="reset"]':
        ("RESET", "clear every accumulating measurement: peak holds, "
                  "the over counter, integrated loudness"),
    '.chip[data-act="setup"]':
        ("SETUP", "range, scale, ballistics, hold times, mono fold, "
                  "goniometer release and AGC"),
    "#bare":
        ("Full screen", "takes the header and status bar off so the instruments "
                        "have the whole screen. H toggles it; Escape comes back"),
    '.chip[data-act="menu"]':
        ("App menu", "Appearance, Totalyser Setup, Bridge bypass (A/B), "
                     "Start with Windows, About, Quit"),
    "#specCard":
        ("Totalyser", "the spectrum analyser: thirty third-octave bands on "
                      "6th-order Butterworth filters, not an FFT"),
    "#specCard .meta":
        ("Resolution, bands, range", "what the analyser is currently set to; "
                                     "change it in SETUP"),
    "#specAxis":
        ("Frequency axis", "ISO 266 third-octave centres, 25 Hz to 20 kHz, "
                           "staggered over two rows"),
    "#levCard":
        ("Level Meters", "RMS L, Peak L, Peak R, RMS R, with the OVR counter "
                         "above and peak / RMS / crest printed below"),
    "#scopeBox":
        ("Vectorscope", "the goniometer, rotated 45 degrees so mono is a "
                        "vertical line"),
    "#clipmode":
        ("LOCKED / FREE", "LOCKED holds the trace inside the graticule, so a "
                          "full signal reads as a diamond; FREE lets it run to "
                          "the corners. V toggles it"),
    "#corrBox":
        ("Correlation", "-1 to +1 with the minimum held, and the L/R balance"),
    "#status":
        ("Status bar", "sample rate, device, over count, dropped frames, CPU"),
    "#grip":
        ("Resize grip", "drag to resize the window"),
}

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
    # ------------------------------------------------------------ THROW
    A = ACCENT["throw"]
    # Order here IS the legend order in content_throw.py - keep the two lists in step.
    annotate("throw", "throw/tour.png", [
        ("#pLogo", "Header logo"), ("#bypassBtn", "BYPASS"), ("#presetBar", "Preset bar"), ("#macroStrip", "Macro slots"),
        ("#gridWrap", "GRID"), ("#delayMs+#bpmLabel", "MS / BPM"), ("#knob", "THROW"), ("#taps", "Tap trail"),
        ("#bandHz+#tailInfo", "BAND / TAIL"), ("#advToggle", "ADVANCED"), ("#mixKnob", "MIX"), ("#resize-handle", "Resize grip")], A,
        crops=[("knob", "#stage", 10), ("top", "#presetBar+#macroStrip", 8)])
    annotate("throw_adv", "throw/adv.png", [
        ("@tone", "TONE"), ("@feedback", "FEEDBACK"), ("@width", "WIDTH"), ("@duck", "DUCK"), ("@space", "SPACE"),
        ("#timeGroup", "L / TIME"), ("#dualBtn", "DUAL"), ("#timeGroupR", "R")], A,
        crops=[("panel", "#advPanel", 8)])
    plain("throw_menu", "throw/menu.png"); plain("throw_about", "throw/about.png")
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
    # ------------------------------------------------------------ ANALYZER
    # Boxes are measured off the running app (see ANALYZER_CALLOUTS above). The
    # hand-placed list this replaced was pinned to a 3458x1398 capture of a UI
    # that no longer exists.
    A = ACCENT["analyzer"]
    rects_file = SHOTS / "analyzer_rects.json"
    if rects_file.exists():
        measured = json.loads(rects_file.read_text(encoding="utf-8"))
        # The badge is drawn on a box's top-left corner, so a control flush
        # against the window edge gets a badge with half of it outside the
        # picture. Nudge those few boxes in; a dozen pixels on a 3200-wide
        # capture is not visible, and a clipped number is.
        img_w, img_h = Image.open(SHOTS / "analyzer_raw.png").size
        def _fit(b):
            x, y, w, h = b
            nx = min(max(x, 26), img_w - 44)
            ny = min(max(y, 26), img_h - 44)
            return [nx, ny, max(8, w - (nx - x)), max(8, h - (ny - y))]

        boxes = [(_fit(measured[sel]), entry[0])
                 for sel, entry in ANALYZER_CALLOUTS.items()
                 if measured.get(sel)]
        if boxes:
            annotate("analyzer_raw", "analyzer/tour.png", boxes, A, scale=1, badge_scale=0.5)
            missed = [s for s in ANALYZER_CALLOUTS if not measured.get(s)]
            if missed:
                print("  analyzer: selectors matched nothing: " + ", ".join(missed))
        else:
            print("  analyzer: every selector missed - tour.png left unannotated")
    else:
        print("  analyzer: no measured rects - run the capture step first")
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
