"""Numbered callouts and detail crops on the screenshots, one set per manual.

Selectors are DOM ids ('#knob'), data-param names ('@smooth'), substrings of the
collected rect keys ('wave-btn'), unions ('#a+#b'), or explicit [x, y, w, h] boxes
in image pixels for native UIs (the Analyzer, the Center capture)."""
import json
from PIL import Image, ImageDraw, ImageFont
from paths import SHOTS, DOCIMG, FONT, ensure

ACCENT = {"stutter": "#FFD02B", "stucker": "#9D6BFF", "flanger": "#FF3D6E", "reversereverb": "#2BD9FF",
          "afterspace": "#FF8A3D", "center": "#9D6BFF",
          # the Analyzer is RONE cyan everywhere since 1.3.34 - never the old green
          "analyzer": "#5AD2E6",
          # Throw's on-screen accent is Ice White #D8E4EC, which vanishes on paper; the
          # manual uses this printable ice-grey for badges and headings instead.
          "throw": "#9FB3C4", "clipper": "#3D8BFF",
          # Rise Pink #FF5FB8 is too light for white paper badges; the manual uses a deeper pink
          "rise": "#E0449A",
          # Iron's UV Orchid #E552FF glows on graphite but is thin on paper; a deeper orchid for badges
          "iron": "#B530D6",
          # Gater Chartreuse #D4FF3D disappears on white paper; a deeper olive-chartreuse for badges
          "gater": "#8FB000",
          # the compressors' neons (#FF8C2E, #FF6F59) are thin on paper; deeper shades for the badges
          "c1176": "#E0701A", "c2a": "#E04E38",
          # Sidechain's periwinkle prints as it is
          "sidechain": "#7B8CFF"}

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
    # --- header
    "#brand":
        ("Logo", "the R mark opens About (version, licence, links)"),
    ".readbar":
        ("Source readout", "what is being measured: source, channel pair and "
                           "sample rate; the dot lights while audio is arriving"),
    '[data-act="source"]':
        ("SOURCE", "what to measure: your DAW's master through RONE Analyzer "
                   "Bridge, what the computer plays, an input, a file - and the "
                   "channel pair of the open source. F3"),
    "#playChip":
        ("PLAY / STOP", "the file transport, shown while a file is the source. Space"),
    "#scanChip":
        ("SCAN", "push the whole file through the meters again, as fast as they "
                 "can take it"),
    '[data-act="menu"]':
        ("Menu", "open a file, Appearance, reset every measurement, Bridge "
                 "bypass, always on top, start with Windows, the Hints tour, About"),
    "#chrome":
        ("Hide the chrome", "takes the header and the status bar away so the "
                            "instruments have the whole window. H; Escape brings them back"),
    # --- spectrogram
    "#chanRow":
        ("CHANNELS", "every RONE Analyzer Channel in your DAW appears here as a "
                     "chip in its colour; click shows or hides it, S sees it alone, "
                     "right-click hears it alone, drag changes the drawing order"),
    "#sidesChip":
        ("SIDES", "paints the side signal (L minus R) over the bars in violet: "
                  "what is wide, band by band"),
    "#refModeChip":
        ("REF LINE / FULL", "how the reference is drawn: its curve over the plot, "
                            "or the whole record in its own colour behind the live one"),
    "#rulesChip":
        ("RULES", "the gold 10 dB rows and the fainter 5 dB rows: one click hides "
                  "the fives, the next the tens too, the next brings them back"),
    "#viewChip":
        ("BARS / WAVES", "the third-octave analyser, or the FFT spectrum the way "
                         "spectrum analysers everyone knows draw it. W"),
    '[data-act="setupSpectrum"]':
        ("Setup", "bands, range, resolution, decay, rise time, hold; in WAVES "
                  "the Spectrum Mode editor. F2"),
    "#specPlot":
        ("Spectrogram", "thirty third-octave bands of LEDs; the white LED is the "
                        "held peak. Press and hold a bar to hear only that band"),
    "#specAxis":
        ("Zones and frequencies", "the band centres, and SUB, BASS, LOW MID ... AIR: "
                                  "click a zone to hear that range alone, right-click "
                                  "to solo just it"),
    "#ldToggle":
        ("Loudness toggle", "opens and closes the LOUDNESS panel. L"),
    # --- loudness
    "#ldTarget":
        ("Target", "CLUB -8, STREAM -14 or EBU -23 LUFS: the gauge and TO TARGET "
                   "measure against it"),
    '[data-act="lufsReset"]':
        ("RESET", "restart Integrated, Range and the true-peak hold; Momentary "
                  "and Short-term keep running"),
    "#ldRow":
        ("Loudness readings", "Momentary, Short-term, Integrated, Range, True Peak "
                              "(OVER -1 lights above -1.0 dBTP) and the distance to the target"),
    # --- level meters
    "#lvMax":
        ("MAX PEAK / MAX RMS", "the loudest moment since the last reset - the "
                               "largest sample, and the RMS of the louder channel; "
                               "click to restart"),
    "#lvGear":
        ("Meter settings", "range, hold, release and over detection"),
    "#levels":
        ("Level meters", "RMS inside, PEAK outside, per channel; the top rows "
                         "turn yellow from -3 dB and the OVR rows red on an over"),
    # --- vectorscope
    "#shapechip":
        ("MODE", "the frame: diamond or circle"),
    "#monochip":
        ("STEREO / SIDES / MONO", "folds everything: every instrument then measures "
                                  "the fold, and the Bridge plays it in the DAW. M"),
    "#vec2Open":
        ("Second vectorscope", "a second scope beside this one for the reference "
                               "record or one of your Channels, in its colour"),
    '[data-act="setupScope"]':
        ("Scope settings", "the phosphor's afterglow and the AGC"),
    "#scopeBox":
        ("Vectorscope", "the stereo image: M up, L and R on the diagonals, S "
                        "across. V toggles LOCKED and FREE"),
    "#corrCv":
        ("Correlation", "+1 is mono-safe, 0 is wide, below zero cancels in mono"),
    "#refWave":
        ("Reference record", "the loaded record; the bright stretch is the 16 bars "
                             "the curve is taken from - click to move it, the arrows "
                             "step 16 bars"),
    "#noteRow":
        ("NOTE", "the note under the mix - the bass or kick fundamental - with its "
                 "octave, hertz and cents; the small arrow pops it out into a window "
                 "that stays on top"),
    "#refCtl":
        ("PLAY / CURVES / CLEAR", "play the reference loop; save this curve under "
                                  "a name or bring one back; remove the reference"),
    ".balWrap":
        ("BAL", "left / right energy balance"),
    "#bsChip":
        ("SCOPE", "opens the BAR SCOPE: the waveform locked to the DAW's bar. O"),
    # --- status bar
    "#status":
        ("Status bar", "sample rate and channels, the source or file and its "
                       "position, MONO, overs, dropped frames, CPU"),
    "#sizeLock":
        ("FIXED / FREE", "FIXED keeps the window's shape and zooms the instrument; "
                         "FREE lets it take any size"),
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
        ("#pLogo", "Header logo"), ("#fileName+#fileInfo", "Loaded file"),
        ("#presetBar", "PRESETS"), ("#macroStrip", "MACRO SLOTS"), ("#browseBtn", "BROWSE"),
        ("#timelinePanel", "Waveform"), ("#transientNav", "PREV / NEXT"), ("#viewToggle", "ORIGINAL / RESULT"),
        ("#divisionBtns", "GRID"), ("#rampToBtns", "RAMP TO"), ("#rampCurveBtns", "CURVE"),
        ("#sliceMs", "SLICE"), ("#barsMinus+#barsPlus", "BARS"), ("#bpmInput+#bpmSource", "BPM"),
        ("#fadeInKnob", "FADE IN"), ("#fadeOutKnob", "FADE OUT"), ("#stereoKnob", "STEREO"),
        ("#fadeOutAutoBtns", "FADE OUT AUTO"), ("#stereoAutoBtns", "STEREO AUTO"),
        ("#playBtn", "PLAY"), ("#onBarBtn", "ON BAR"), ("#applyBtn", "STUTTER"),
        ("#exportBtn", "EXPORT"), ("#advToggle", "ADVANCED"), ("#extrasToggle", "EXTRAS"),
        ("#statusText", "Status line"), ("#resize-handle", "Resize grip")], A,
        crops=[("grid", "#divisionBtns+#bpmSource", 14), ("knobs", "#fadeInArc+#stereoAutoBtns", 14), ("transport", "#playBtn+#exportBtn", 12)])
    annotate("stutter_m_result", "stutter/result.png", [
        ("#viewToggle", "ORIGINAL / RESULT"), ("#playBtn", "PLAY"),
        ("#transientNav", "PREV / NEXT"), ("#advToggle", "ADVANCED")], A)
    annotate("stutter_m_free", "stutter/free.png", [
        ("#divisionBtns", "GRID = FREE"), ("#sliceChip", "SLICE (start length)"),
        ("#tremoloBtns", "TREMOLO"), ("#freeEndChip", "END length")], A)
    annotate("stutter_m_adv", "stutter/adv.png", [
        ("#mixKnob", "MIX"), ("#fadeInCurveKnob", "IN CURVE"), ("#fadeOutCurveKnob", "OUT CURVE"), ("#globalFadeInKnob", "GLOBAL IN"),
        ("#globalFadeOutKnob", "GLOBAL OUT"), ("#pitchKnob", "PITCH"), ("#pitchRampKnob", "PITCH RAMP"), ("#keyRootBtns", "KEY"), ("#keyModeBtns", "SCALE")], A,
        crops=[("panel", "#advPanel", 8)])
    plain("stutter", "stutter/empty.png"); plain("stutter_about", "stutter/about.png")
    plain("stutter_m_menu", "stutter/menu.png")
    # ------------------------------------------------------------ STUCKER
    A = ACCENT["stucker"]
    annotate("stucker", "stucker/tour.png", [
        ("#pLogo", "Header logo"), ("#bypassBtn", "BYPASS"), ("#syncSeg", "SYNC / FREE"), ("#gridLabel", "GRID"),
        ("#rideGroup", "RIDE"), ("#rideGo", "GO"),
        ("#knob", "STUCK"), ("#loopMs", "LOOP"), ("#leds", "LEDs"), ("#landChip", "LANDS"), ("#advToggle", "ADVANCED"),
        ("#mixKnob", "MIX"), ("#resize-handle", "Resize grip")], A, crops=[("knob", "#stage", 10), ("header", "#syncSeg+#gridLabel", 10)])
    annotate("stucker_adv", "stucker/adv.png", [
        ("@smooth", "SMOOTH"), ("@range", "RANGE"), ("@lowcut", "LOW CUT"), ("#lenGroup", "LENGTH"),
        ("#keyRootGroup", "KEY"), ("#dropGroup", "ON THE DROP"), ("#rideCurveGroup", "CURVE")], A,
        crops=[("panel", "#advPanel", 8)])
    plain("stucker_about", "stucker/about.png"); plain("stucker_ride", "stucker/ride.png")
    # ------------------------------------------------------------ THROW
    A = ACCENT["throw"]
    # Order here IS the legend order in content_throw.py - keep the two lists in step.
    annotate("throw", "throw/tour.png", [
        ("#pLogo", "Header logo"), ("#killBtn", "KILL"), ("#bypassBtn", "BYPASS"), ("#presetBar", "Preset bar"), ("#macroStrip", "Macro slots"),
        ("#gridWrap", "GRID"), ("#delayMs+#bpmLabel", "MS / BPM"), ("#knob", "THROW"), ("#taps", "Tap trail"),
        ("#bandHz+#tailInfo", "BAND / TAIL"), ("#advToggle", "ADVANCED"), ("#pitchModeBtn", "HZ / NOTE"), ("#pitchKnob", "PITCH"), ("#gainKnob", "GAIN"), ("#mixKnob", "MIX"), ("#resize-handle", "Resize grip")], A,
        crops=[("knob", "#stage", 10), ("top", "#presetBar+#macroStrip", 8)])
    annotate("throw_adv", "throw/adv.png", [
        ("@tone", "TONE"), ("@feedback", "FEEDBACK"), ("@width", "WIDTH"), ("@duck", "DUCK"), ("@space", "SPACE"),
        ("#timeGroup", "L / TIME"), ("#dualBtn", "DUAL"), ("#timeGroupR", "R"),
        ("#duckKeyGroup", "DUCK KEY")], A,
        crops=[("panel", "#advPanel", 8)])
    plain("throw_menu", "throw/menu.png"); plain("throw_about", "throw/about.png"); plain("throw_note", "throw/note.png")
    # ------------------------------------------------------------ CLIPPER
    A = ACCENT["clipper"]
    # Order here IS the legend order in content_clipper.py - keep the two lists in step.
    annotate("clipper", "clipper/tour.png", [
        ("#pLogo", "Header logo"), ("#deltaBtn", "DELTA"), ("#bypassBtn", "BYPASS"), ("#viewSeg", "WAVE / PEAKS / DETAIL"),
        ("#freezeBtn", "FREEZE"), ("#resetBtn", "Reset"), ("#graphWrap", "Graph"), ("#knob", "CLIP"), ("#clipWell", "CLIP value"),
        ("#lowcutKnob", "LOW CUT"), ("#advToggle", "ADVANCED"), ("#resize-handle", "Resize grip")], A,
        crops=[("knob", "#knobStage", 10)])
    annotate("clipper_adv", "clipper/adv.png", [
        ("#osSeg", "OS"), ("#depthNum", "SAVED"), ("#fIn", "INPUT"), ("#fOut", "OUTPUT"), ("#autoBtn", "AUTO GAIN")], A,
        crops=[("panel", "#advPanel", 8)])
    plain("clipper_peaks", "clipper/peaks.png"); plain("clipper_detail", "clipper/detail.png"); plain("clipper_about", "clipper/about.png")
    # ------------------------------------------------------------ RISE
    A = ACCENT["rise"]
    # Order here IS the legend order in content_rise.py - keep the two lists in step.
    annotate("rise", "rise/tour.png", [
        ("#pLogo", "Header logo"), ("#bypassBtn", "BYPASS"), ("#knob", "INTENSITY"), ("#goBtn", "GO"), ("#clockTxt", "Bar counter"),
        ("#mood", "Mood"), ("#presets", "Presets"), ("#advBtn", "ADVANCED"), ("#resize-handle", "Resize grip")], A,
        crops=[("knob", "#knob", 10)])
    annotate("rise_adv", "rise/adv.png", [
        ("#landSeg", "LAND ON BAR"), ("#curveSeg", "CURVE"), ("#tailBar", "TAIL"), ("#macros", "Macros"), ("#chain", "Chain strip")], A,
        crops=[("panel", "#drawer", 8)])
    plain("rise_about", "rise/about.png")
    # ------------------------------------------------------------ RONE 1176 / RONE 2A (1080 wide at DPR 2)
    # The faces are drawn by face_kit.js and their knobs carry no ids, so the boxes are image pixels
    # measured on the shots (296 tall without the scope, 718 with the scope + ADVANCED open).
    # Order here IS the legend order in content_1176.py / content_2a.py - keep the lists in step.
    # The v2 headers (27.10) differ per face, so each has its own boxes: 1176 = label + toggle, 2A = a button.
    LOGO, SCOPE_BTN, BYPASS_BTN = [43, 30, 248, 87], [1620, 22, 229, 64], [1876, 22, 246, 64]
    LOGO_2A, SCOPE_2A, BYPASS_2A = [43, 30, 319, 48], [1617, 27, 222, 55], [1877, 27, 234, 55]
    BARS, DISPLAY, HOLD, MORE = [221, 621, 481, 113], [743, 621, 270, 113], [1091, 621, 135, 113], [1972, 621, 139, 76]
    A = ACCENT["c1176"]
    annotate("c1176", "1176/tour.png", [
        (LOGO, "Header logo"), (SCOPE_BTN, "SCOPE"), (BYPASS_BTN, "BYPASS"), ([119, 162, 313, 421], "INPUT"),
        ([583, 162, 313, 421], "OUTPUT"), ([1021, 113, 151, 200], "ATTACK"), ([1021, 356, 151, 221], "RELEASE"),
        ([1226, 130, 167, 416], "RATIO"), ([1436, 151, 464, 394], "VU meter"), ([1949, 130, 184, 372], "METER")], A)
    annotate("c1176_adv", "1176/scope.png", [
        (BARS, "BARS"), (DISPLAY, "MATCH / IN"), (HOLD, "HOLD"), ("#graph", "Scope"), (MORE, "ADVANCED"),
        ([184, 1280, 270, 97], "HEADROOM"), ([605, 1280, 227, 97], "MIX"), ([1026, 1280, 243, 97], "IN TRIM"),
        ([1447, 1280, 254, 97], "OUT TRIM"), ([1744, 1301, 286, 77], "SC FILTER")], A)
    plain("c1176", "1176/main.png"); plain("c1176_about", "1176/about.png")
    A = ACCENT["c2a"]
    annotate("c2a", "2a/tour.png", [
        (LOGO_2A, "Header logo"), (SCOPE_2A, "SCOPE"), (BYPASS_2A, "BYPASS"), ([140, 140, 168, 330], "LIMIT / COMPRESS"),
        ([443, 162, 313, 421], "GAIN"), ([837, 138, 486, 356], "VU meter"), ([1404, 162, 313, 421], "PEAK REDUCTION"),
        ([1836, 205, 216, 313], "METER")], A)
    annotate("c2a_adv", "2a/scope.png", [
        (BARS, "BARS"), (DISPLAY, "MATCH / IN"), (HOLD, "HOLD"), ("#graph", "Scope"), (MORE, "ADVANCED"),
        ([173, 1280, 254, 97], "EMPHASIS"), ([567, 1280, 221, 97], "MIX"), ([972, 1280, 238, 97], "IN TRIM"),
        ([1372, 1280, 248, 97], "OUT TRIM"), ([1728, 1285, 394, 119], "CELL")], A)
    plain("c2a", "2a/main.png"); plain("c2a_about", "2a/about.png")
    # ------------------------------------------------------------ RONE SIDECHAIN (960x560 at DPR 2)
    # The knobs (.k[data-k]) and the canvas lanes carry no ids, so those boxes are image pixels measured
    # on the shots (the demo clock is frozen 300 ms after a kick, 125 ms on SPACE - see pipeline.STATES).
    # Order here IS the legend order in content_sidechain.py - keep the lists in step.
    A = ACCENT["sidechain"]
    K1, K2 = [80, 900, 122, 192], [262, 900, 122, 192]           # the first two knobs of a page's control row
    annotate("sidechain", "sidechain/tour.png", [
        ("#pLogo", "Header logo"), ("#presetBtn", "PRESET"), ("#rateSeg", "RATE"), ("#kickBox", "KICK chip"), ("#bypassBtn", "BYPASS"),
        ("#tabSeg", "Pages"), ("#undoBtn+#redoBtn", "UNDO / REDO"), ([60, 262, 1800, 136], "Kick lane"), ([60, 416, 1800, 330], "Bass lane + curve"),
        ("#drawSeg", "FREE / QUANTIZE"), ("#gridSeg", "GRID"),
        ("#cvRead", "Readout"), ("#viewSeg", "LANES / OVERLAY"), ("#shapes", "SHAPES"), ("#saveShape", "+ SAVE"),
        (K1, "DEPTH"), (K2, "CLICK"), ("#rangeSeg+#dkStrip", "DUCK"), ("#loadKick+#dropBox", "LOAD KICK / DROP"),
        ("#resize-handle", "Resize grip")], A)
    # the curve's parts on STEP: the kick's point, a vertical step, a flat level, a point, the playhead
    annotate("sidechain", "sidechain/draw_full.png", [
        ([50, 726, 24, 24], "Kick's point"), ([492, 600, 24, 140], "Step"), ([640, 566, 290, 20], "Level"),
        ([948, 402, 24, 24], "Point"), ([1332, 300, 20, 430], "Playhead")], A)
    annotate("sidechain_shape", "sidechain/bend_full.png", [([454, 454, 24, 24], "Bend circle"), ([858, 402, 24, 24], "Point")], A)
    annotate("sidechain_phase", "sidechain/phase.png", [
        ("#phCv", "Handover scope"), ("#phOnBtn", "ON / OFF"), ([180, 900, 170, 192], "PHASE"), ([388, 900, 124, 192], "SPLIT"),
        ("#modeSeg+#polBtn", "MODE / FLIP"), ("#phState", "Status"), ("#phRead", "Readout")], A)
    annotate("sidechain_space", "sidechain/space.png", [
        ("#spCv", "The bells"), ([236, 300, 120, 80], "Cut now"), ([706, 238, 24, 350], "LOW / HIGH"), ([1800, 262, 64, 270], "dB scale"),
        ("#stCv", "WHEN THEY WORK"), ("#spOnBtn", "SPACE ON / OFF"), ([860, 900, 140, 192], "AMOUNT"), ([1090, 900, 130, 192], "RELEASE")], A)
    annotate("sidechain_psave", "sidechain/psave_full.png", [
        ("#presetList", "Genre presets"), ("pico[14]", "Star"), ("#myPresetsH+#myPresetList", "MY PRESETS"),
        ("#psaveRow", "Name + SAVE"), ("#psaveBtn", "+ SAVE PRESET AS")], A)
    annotate("sidechain_hover", "sidechain/hover_full.png", [], A)            # no badge: it would cover the x
    annotate("sidechain_setup", "sidechain/setup_full.png", [
        ("#kickBox", "KICK chip"), ("#mIn+#mSc", "IN / SC meters"), ("#setupMe", "Your DAW"), ("#setupMore", "OTHER DAWS")], A)
    annotate("sidechain_band", "sidechain/band.png", [], A, crops=[("duck", "#rangeSeg+#dkStrip", 12)])
    # crops of the annotated images (annotate()'s own crops come from the clean shot)
    for full, out, box in (("draw_full", "draw", [0, 190, 1900, 600]), ("bend_full", "bend", [0, 190, 1900, 600]),
                           ("setup_full", "setup", [940, 0, 980, 660]),
                           ("psave_full", "psave", [690, 90, 670, 890]), ("hover_full", "hover", [0, 790, 1920, 92])):
        p = DOCIMG / "sidechain" / f"{full}.png"
        if p.exists():
            x, y, w, h = box
            Image.open(p).crop((x, y, x + w, y + h)).save(DOCIMG / "sidechain" / f"{out}.png", optimize=True)
    plain("sidechain", "sidechain/main.png"); plain("sidechain_overlay", "sidechain/overlay.png")
    plain("sidechain_presets", "sidechain/presets.png"); plain("sidechain_about", "sidechain/about.png")
    # ------------------------------------------------------------ IRON (1000x684, demo page posed by shoot_iron.py)
    A = ACCENT["iron"]
    # #chopCv is one canvas: the waveform, the pan lane and the keys are boxes in image pixels (DPR 2).
    WAVE, LANE, KEYS = [108, 186, 1834, 236], [60, 426, 1882, 108], [106, 538, 1836, 82]
    # Order here IS the legend order in content_iron.py - keep the lists in step.
    annotate("iron", "iron/tour.png", [
        ("#pLogo", "Header logo"), ("#ipPre", "Preset browser"), ("#macroRow", "Macros"), ("#midiLed+#bpmRead", "MIDI / tempo"),
        ("#vFile+#vMeta", "Vocal"), ("#sliceSeg", "AUTO SLICE"), ("#vCount", "Chops / keys"), ("#vInfo", "Chop readout"),
        ("#browseBtn+#ejectBtn", "BROWSE / EJECT"), (WAVE, "Chop map"), (LANE, "Pan lane"), (KEYS, "Chop keys")], A,
        crops=[("vocal", "#vCard", 6)])
    annotate("iron", "iron/play.png", [
        ("#notePill", "NOTE"), ("#octCtl", "OCTAVE"), ("#midiBtn", "MIDI NOTE"), ("#arpBtn", "ARP"), ("#arpRow", "ARP order"),
        ("#nRead", "Note readout"), ("#kComb", "Comb readout"), ("#ironKnob", "IRON"), ("#kFade", "FADE OUT"), ("#kRing", "RING"),
        ("#autoPan", "AUTO PAN"), ("#panStyleSeg", "PAN STYLE"), ("#rangeKnob", "RANGE"), ("#panMeter", "Pan meter")], A,
        crops=[("hero", "#hero", 0)])
    annotate("iron", "iron/foot.png", [
        ("#advBtn", "ADVANCED"), ("#grooveBtn", "GROOVE"), ("#gSteps", "Step strip"), ("#dragMidiFoot", "DRAG MIDI"),
        ("#gRead", "Groove readout"), ("#mixKnob", "MIX"), ("#mixNote", "NOTE"), ("#outKnob", "OUT"), ("#safeLamp", "SAFE"),
        ("#resize-handle", "Resize grip")], A)
    annotate("iron_groove", "iron/groove.png", [
        ("#gpToggle", "GROOVE"), ("#barsSeg", "BARS"), ("#rerollBtn", "REROLL"), ("#histBack+#histPos+#histFwd", "History"),
        ("#lockAll", "LOCK STEPS"), ("#clearEdits", "CLEAR EDITS"), ("#gpPre", "Pattern"), ("#dragMidi", "DRAG MIDI"),
        ("#gGrid", "Steps"), ("#gpKnobsA", "Density / chops / variation"), ("#gpKnobsB", "Length / hold / flip / rolls")], A,
        crops=[("panel", "#gPanel", 0), ("steps", [88, 744, 720, 326], 0)])
    annotate("iron_adv", "iron/adv.png", [
        ("#monoSeg", "VOICE"), ("#combSeg", "COMB"), ("#limSeg", "SAFE"), ("#advFlat", "FLATTEN + SLICE"),
        ("#advSound", "SOUND"), ("#advKeys", "KEYS")], A,
        crops=[("drawer", "#drawer", 0)])
    annotate("iron_presets", "iron/presets.png", [
        ("#ipPre", "Preset browser"), ("#ipList", "Preset list"), ("#ipDetail", "Description"),
        ([380, 836, 1150, 62], "MY SLOTS"), ("#ipInit", "INIT")], A)
    annotate("iron_arp", "iron/arp.png", [("#arpBtn", "ARP"), ("#notePill", "Chord notes"), ("#arpOrderSeg", "Order"),
        ("#arpOctSeg", "Octaves"), ("#nRead", "Note readout"), ("#kComb", "Combs")], A,
        crops=[("note", "#octCtl+#notePill+#arpRow+#nRead+#kComb", 10)])
    # ------------------------------------------------------------ GATER
    A = ACCENT["gater"]
    # Order here IS the legend order in content_gater.py - keep the two lists in step.
    annotate("gater", "gater/tour.png", [
        ("#view", "Display"), ("#midiBox", "MIDI lamp"), ("#setupBtn", "SETUP"), ("#bypassBtn", "BYPASS"),
        ("#k_attack_ms", "ATTACK"), ("#k_release_ms", "RELEASE"), ("#k_depth", "DEPTH"), ("#velSeg", "VELOCITY")], A,
        crops=[("knobs", "#knobs", 8)])
    plain("gater_setup", "gater/setup.png"); plain("gater_about", "gater/about.png")
    plain("iron", "iron/main.png"); plain("iron_locks", "iron/locks.png"); plain("iron_empty", "iron/empty.png"); plain("iron_about", "iron/about.png")
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
        ("#p-logo", "Header logo"), ("#presetBar", "Preset bar"), ("#preset-save", "SAVE"),
        ("#waveform-container", "Waveform"), ("#status-text", "Status"),
        ("#play-btn", "PLAY"), ("#mode-btn", "Mode"),
        ("#knob-lowcut", "LOW CUT"), ("#knob-hicut", "HI CUT"), ("#knob-width", "WIDTH"),
        ("#knob-tailDiv", "TAIL LENGTH"), ("#knob-roomSize", "ROOM SIZE"), ("#knob-gain", "OUTPUT"),
        ("#tremolo-power", "TREMOLO"), ("#resize-handle", "Resize grip")], A,
        crops=[("knobs", "#knobs", 8), ("buttons", "#button-row", 6)])
    annotate("reversereverb_m_trem", "reversereverb/tremolo.png", [
        ("#knob-tremDepth", "DEPTH"), ("#knob-tremPan", "PAN"), ("#knob-tremRate", "RATE"), ("wave-btn", "SHAPE"),
        ("#tremModeSeg", "MODE")], A, crops=[("panel", "#tremolo-controls", 6)])
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
        # the badge radius annotate() will use (badge_scale 0.5), plus air
        edge = int(19 * 0.5 * (img_w / 1400)) + 12 + 10
        def _fit(b):
            x, y, w, h = b
            nx = min(max(x, edge), img_w - edge)
            ny = min(max(y, edge), img_h - edge)
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
