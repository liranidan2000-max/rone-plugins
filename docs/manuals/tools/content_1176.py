def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    m = {
        "id": "1176", "product": "RONE 1176", "eyebrow": "FET LIMITING AMPLIFIER",
        "title_html": "RONE <i>1176</i>", "accent": "#E0701A", "version": "0.9.4",
        "tagline": "The FET limiter that made vocals jump out and drums hit harder - INPUT drives it, OUTPUT makes up, four ratio buttons and the famous ALL. With a SCOPE that shows the wave and the gain reduction locked to the bar.",
        "formats": ["VST3", "AU", "Standalone"], "vst3": "RONE 1176.vst3", "au": "RONE 1176.component", "exe": "RONE 1176.exe",
        "pdf": "RONE 1176 - User Manual.pdf", "cover_img": "1176/main.png",
        "subscription_only": True,
    }
    S = []

    S.append({"title": "Welcome to RONE 1176", "sub": "a FET limiter with a scope", "html": f"""
<p><strong>RONE 1176</strong> is a model of the classic FET limiting amplifier, fitted to measurements of the Rev E (black-face) unit: the way the gain reduction grabs and lets go, how the release depends on the material, and the colour the circuit adds when you push it. It works like the hardware: there is no threshold knob. <strong>INPUT</strong> drives the signal into a fixed threshold, <strong>OUTPUT</strong> sets the level that leaves, and ATTACK and RELEASE run from 1 (slowest) to 7 (fastest).</p>
<p>Press <strong>SCOPE</strong> and the window grows downwards: a scope draws the output wave and the gain reduction locked to the bars of your song, so you see what the limiter did to every hit instead of guessing from a needle.</p>
<div class="two">
<div>
<h3>Where it shines</h3>
<ul>
<li><strong>Vocals</strong> - fast attack, 4:1, a few dB on the needle: forward and steady.</li>
<li><strong>Drums and room mics</strong> - ALL (every ratio button in) for the aggressive, pumping sound.</li>
<li><strong>Bass</strong> - slow attack lets the pluck through, the body stays even.</li>
<li><strong>Parallel</strong> - MIX blends the squashed signal under the dry one.</li>
</ul>
</div>
<div>
<h3>The 10-second version</h3>
{steps([
 "Insert RONE 1176 on a vocal.",
 "RATIO 4, ATTACK 4, RELEASE 5.",
 "Turn INPUT up until the needle (GR) moves 3-7 dB on the loud words.",
 "Match the level with OUTPUT."])}
</div></div>
{note("Included in ALL ACCESS", "RONE 1176 is part of the ALL ACCESS plan and opens with an active plan. It is not sold as a separate lifetime licence.")}
"""})

    S.append(G["install_section"](m))

    S.append({"title": "Quick start", "sub": "your first settings", "html": f"""
{img("1176/main.png", "<b>RONE 1176.</b> INPUT and OUTPUT on the left, ATTACK and RELEASE in the middle, RATIO, the VU meter and the METER buttons on the right.", "w80")}
{steps([
 "<strong>Insert</strong> RONE 1176 on a track or bus.",
 "<strong>Pick a ratio:</strong> 4 for gentle compression, 8 or 12 for firm control, 20 for limiting, ALL for the aggressive sound.",
 "<strong>Turn INPUT up</strong> while watching the VU meter on GR. The more input, the more gain reduction - the threshold itself never moves.",
 "<strong>Set ATTACK and RELEASE.</strong> 7 is the fastest on both. A slower attack (1-3) lets transients through; a faster release (6-7) makes it louder and livelier.",
 "<strong>Bring the level back with OUTPUT</strong> so the bypassed and processed signal are about as loud, then compare with BYPASS.",
])}
{note("Hold Shift for fine moves", "Drag a knob with Shift held for fine steps, double-click it to return to the default, and right-click it for the host's automation menu.")}
"""})

    S.append({"title": "Interface tour", "sub": "every element on screen", "html": f"""
{img("1176/tour.png", "The unit as it opens, without the scope.")}
{legend([
 ("Header logo", "click to flip to the back panel (About, version, licence)"),
 ("SCOPE", "opens the scope under the unit; the window grows down"),
 ("BYPASS", "lit = the unit is out of the path"),
 ("INPUT", "drives the limiter; the readout shows the dial and the gain in dB"),
 ("OUTPUT", "the make-up gain"),
 ("ATTACK", "1 = slowest (800 &micro;s) to 7 = fastest (20 &micro;s)"),
 ("RELEASE", "1 = slowest (1.1 s) to 7 = fastest (50 ms)"),
 ("RATIO", "4, 8, 12, 20 - press several for ALL"),
 ("VU meter", "gain reduction or output level, chosen with METER"),
 ("METER", "GR, +8, +4 or OFF"),
])}
{img("1176/scope.png", "SCOPE on, with the ADVANCED row open under it.")}
{legend([
 ("BARS", "how much of the song the scope shows: 1/4, 1/2, 1, 2 or 4 bars"),
 ("MATCH / IN", "draw the output at the input level, and the original as an outline"),
 ("HOLD", "freeze the display to look closer"),
 ("Scope", "the output wave on top, the gain reduction lane under it"),
 ("ADVANCED", "MORE opens the row of extra controls"),
 ("HEADROOM", "the unit's headroom setting"),
 ("MIX", "parallel compression: dry against processed"),
 ("IN TRIM", "&plusmn;24 dB before the unit"),
 ("OUT TRIM", "&plusmn;24 dB after the unit"),
 ("SC FILTER", "a high-pass in the sidechain: the bass stops pumping the limiter"),
])}
"""})

    S.append({"title": "Controls reference", "sub": "what every control does, its range and default", "html": f"""
{ctl("INPUT", "0 to 100 on the dial<br>default as the unit opens<br>automatable", "<p>The input level into the limiter. Like on the hardware, the threshold is fixed: turning INPUT up drives more of the signal over it, so you get more gain reduction and more of the circuit's colour. The readout under the knob shows the dial position and the real gain in dB.</p>", "Shift = fine, double-click = default.")}
{ctl("OUTPUT", "0 to 100 on the dial<br>automatable", "<p>The output amplifier - the make-up gain. It never changes how much the unit compresses, only how loud the result leaves.</p>")}
{ctl("ATTACK", "1 to 7<br>default 4", "<p>How fast the gain reduction grabs, from 800 &micro;s at 1 to 20 &micro;s at 7. Fully clockwise is the fastest, as on the hardware. Fast attacks shave the transients and add the characteristic low-frequency grit on bass and kick; slow attacks let the front of each hit through.</p>")}
{ctl("RELEASE", "1 to 7<br>default 5", "<p>How fast it lets go, from 1.1 s at 1 to 50 ms at 7. The release also depends on the material: after a long, heavy stretch of gain reduction it recovers more slowly, as the hardware does.</p>")}
{ctl("RATIO", "4:1 / 8:1 / 12:1 / 20:1 / ALL<br>default 4:1<br>automatable", "<p>4 and 8 compress, 12 and 20 limit. Pressing more than one button gives <b>ALL</b> (every button in, the trick engineers used on the original): a harder knee, more distortion and the pumping, exploding sound on drums and rooms.</p>")}
{ctl("METER", "GR / +8 / +4 / OFF<br>default GR", "<p>What the VU meter shows. <b>GR</b>: gain reduction in dB. <b>+8</b> and <b>+4</b>: the output level, with 0 VU at +8 or +4 dBu as on the hardware. <b>OFF</b>: the needle rests.</p>", "The meter setting is not a sound parameter and is not automated.")}
{ctl("BYPASS", "on / off", "<p>Takes the whole unit out of the path. Lit = bypassed.</p>")}
<h3>SCOPE</h3>
{ctl("SCOPE", "on / off", "<p>Opens the scope under the unit; the window grows down and remembers it with the project.</p>")}
{ctl("BARS", "1/4, 1/2, 1, 2, 4 bars<br>default 1", "<p>The width of the scope, locked to the song's bars while the host plays.</p>")}
{ctl("MATCH", "on / off", "<p>Draws the output at the input level - the make-up gain removed - so you see only what the compression changed.</p>")}
{ctl("IN", "on / off", "<p>Draws the original signal as an outline behind the output.</p>")}
{ctl("HOLD", "on / off", "<p>Freezes the display. On the frozen screen the mouse wheel zooms, dragging scrolls and a double-click shows the whole window again. Hover the scope to read the position in the bar and the input, output and gain reduction there.</p>")}
<h3>ADVANCED</h3>
{ctl("HEADROOM", "4 to 28 dB<br>default 16 dB", "<p>The unit's headroom setting, stored with the project.</p>")}
{ctl("MIX", "0 to 100 %<br>default 100 %", "<p>Dry against processed: lower it for parallel compression inside the plugin.</p>")}
{ctl("IN TRIM / OUT TRIM", "-24 to +24 dB<br>default 0 dB", "<p>Level trims before and after the unit. IN TRIM sets how hot your track arrives without moving the INPUT dial; OUT TRIM sets the final level.</p>")}
{ctl("SC FILTER", "on / off<br>default off", "<p>A high-pass in the sidechain, so the bass and the kick stop pumping the limiter. The audio itself is not filtered.</p>")}
"""})

    S.append({"title": "Step-by-step workflows", "sub": "recipes", "html": f"""
{recipe("The forward vocal", "4:1, fast",
 steps([
  "RATIO 4, ATTACK 5-6, RELEASE 5-6.",
  "INPUT up until GR shows 3-7 dB on the loud words.",
  "OUTPUT to match the bypassed level. Open SCOPE to see each word come out even.",
 ]))}
{recipe("Smashed drum room", "ALL",
 steps([
  "Press several ratio buttons for ALL. ATTACK 3, RELEASE 7.",
  "Push INPUT until the needle slams; it should pump with the groove.",
  "Pull MIX down to 30-50 % to keep the dry punch under it.",
 ]))}
{recipe("Even bass, punchy front", "slow attack",
 steps([
  "RATIO 4 or 8, ATTACK 1-2, RELEASE 5.",
  "SC FILTER on if the low notes pump more than the high ones.",
  "Check the scope: the start of each note passes, the sustain sits flat.",
 ]))}
"""})

    S.append(G["conventions_section"](m))
    S.append(G["support_section"](m, [
        ("No gain reduction", "INPUT is too low - the threshold is fixed, turn INPUT up (or raise IN TRIM)."),
        ("The bass distorts", "That is the FET at fast attacks. Use a slower ATTACK or a slower RELEASE."),
        ("The scope stays empty", "It draws while the host plays. Press play; HOLD may be on."),
        ("The low end pumps the whole mix", "Switch SC FILTER on in ADVANCED."),
    ]))

    m["sections"] = S
    return m
