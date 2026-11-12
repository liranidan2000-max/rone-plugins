# -*- coding: utf-8 -*-
"""RONE Rise 1.2.0: the new sun (sun12.js) replaces the sun <script> of Source/ui/index.html.
Safe to run again: it replaces whatever sits between the SUN header and the C++ -> JS block."""
import io, re, sys, os

path = sys.argv[1]
here = os.path.dirname(os.path.abspath(__file__))
s = io.open(path, encoding="utf-8").read()
sun = io.open(os.path.join(here, "sun12.js"), encoding="utf-8").read().rstrip("\n")

pat = re.compile(r"<script>\n// ─+\n// THE SUN.*?requestAnimationFrame\(animate\);\n</script>", re.S)
assert len(pat.findall(s)) == 1, "sun block not found"
s = pat.sub(lambda m: "<script>\n" + sun + "\n</script>", s)

# the SCENE chip names the mode's own scenes; in MELTDOWN it goes red at the top
if "function sceneName" not in s:
    old = "const LAND_BARS = [0, 4, 8, 16, 32, 1, 2];"
    assert s.count(old) == 1
    s = s.replace(old,
        "const SCENES = [['DAWN', 'SUNRISE', 'SHINING', 'HIGH NOON'], ['DUSK', 'DRIFT', 'ORBIT', 'DEEP SPACE'], ['FLICKER', 'STROBE', 'PULSE', 'SUPERNOVA'],\n"
        "                ['LOW TIDE', 'SWELL', 'UNDERTOW', 'DEEP DIVE'], ['WARM', 'HEATED', 'ANGRY', 'MELTDOWN']];\n"
        "function sceneName(t) { if (frame.hold) return 'HOLD'; if (CH.cool > .5) return 'DROP'; const s = SCENES[state.preset] || SCENES[0]; return t <= 0.02 ? 'NIGHT' : t < 0.3 ? s[0] : t < 0.6 ? s[1] : t < 0.85 ? s[2] : s[3]; }\n" + old)
    old = "const mc = 'p-mood rone-chip' + (t >= 0.6 ? ' up' : '');"
    assert s.count(old) == 1
    s = s.replace(old, "const mc = 'p-mood rone-chip' + (t >= 0.6 ? (state.preset === 4 ? ' hot' : ' up') : '');")

io.open(path, "w", encoding="utf-8", newline="\n").write(s)
print("sun applied:", len(s) // 1024, "KB")
