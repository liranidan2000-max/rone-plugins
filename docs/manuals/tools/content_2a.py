def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    m = {
        "id": "2a", "product": "RONE 2A", "eyebrow": "OPTICAL LEVELING AMPLIFIER",
        "title_html": "RONE <i>2A</i>", "accent": "#E04E38", "version": "0.9.5",
        "tagline": "The optical leveler behind a thousand vocals and bass tracks - two big knobs, a switch, and a smooth, program-dependent hand on the level. With a SCOPE that shows the wave and the gain reduction locked to the bar.",
        "formats": ["VST3", "AU", "Standalone"], "vst3": "RONE 2A.vst3", "au": "RONE 2A.component", "exe": "RONE 2A.exe",
        "pdf": "RONE 2A - User Manual.pdf", "cover_img": "2a/main.png",
        "subscription_only": True,
    }
    S = []

    S.append({"title": "Welcome to RONE 2A", "sub": "an optical leveler with a scope", "html": f"""
<p><strong>RONE 2A</strong> is a model of the classic tube optical leveling amplifier, fitted to measurements of the silver unit. In the original a light shines on a photocell; the louder the signal, the brighter the light and the more the cell turns the level down. That cell is slow and lazy in a musical way: it grabs gently, lets go in two stages and depends on what came before - which is why it levels a vocal or a bass without you hearing it work.</p>
<p>There are only two knobs. <strong>PEAK REDUCTION</strong> sets how hard the cell pulls down, <strong>GAIN</strong> sets the level that leaves. Press <strong>SCOPE</strong> and the window grows downwards: a scope draws the output wave and the gain reduction locked to the bars of your song.</p>
<div class="two">
<div>
<h3>Where it shines</h3>
<ul>
<li><strong>Vocals</strong> - smooth leveling that keeps the performance natural.</li>
<li><strong>Bass</strong> - an even, round low end.</li>
<li><strong>Mix bus and groups</strong> - a dB or two of glue.</li>
<li><strong>Pads and strings</strong> - slow, breathing control.</li>
</ul>
</div>
<div>
<h3>The 10-second version</h3>
{steps([
 "Insert RONE 2A on a vocal or bass.",
 "Leave the switch on COMPRESS.",
 "Turn PEAK REDUCTION up until GR moves 3-6 dB.",
 "Bring the level back with GAIN."])}
</div></div>
{note("Included in ALL ACCESS", "RONE 2A is part of the ALL ACCESS plan and opens with an active plan. It is not sold as a separate lifetime licence.")}
"""})

    S.append(G["install_section"](m))

    S.append({"title": "Quick start", "sub": "your first settings", "html": f"""
{img("2a/main.png", "<b>RONE 2A.</b> The LIMIT / COMPRESS switch, GAIN, the VU meter, PEAK REDUCTION and the METER knob.", "w80")}
{steps([
 "<strong>Insert</strong> RONE 2A on a track or bus. With PEAK REDUCTION at 0 it does not compress.",
 "<strong>Turn PEAK REDUCTION up</strong> while the meter shows GR, until the loud parts come down 3-6 dB.",
 "<strong>Bring the level back with GAIN</strong> so the bypassed and processed signal are about as loud, then compare with BYPASS.",
 "<strong>Flip to LIMIT</strong> if you want it firmer: a higher ratio for the peaks, same smooth cell.",
])}
{note("Hold Shift for fine moves", "Drag a knob with Shift held for fine steps, double-click it to return to the default, and right-click it for the host's automation menu.")}
"""})

    S.append({"title": "Interface tour", "sub": "every element on screen", "html": f"""
{img("2a/tour.png", "The unit as it opens, without the scope.")}
{legend([
 ("Header logo", "click to flip to the back panel (About, version, licence)"),
 ("SCOPE", "opens the scope under the unit; the window grows down"),
 ("BYPASS", "lit = the unit is out of the path"),
 ("LIMIT / COMPRESS", "the mode switch"),
 ("GAIN", "the output; the readout shows the dial and the gain in dB"),
 ("VU meter", "gain reduction or output level, chosen with METER"),
 ("PEAK REDUCTION", "how hard the cell pulls down"),
 ("METER", "+10, GR or +4 - click to step, wheel to turn"),
])}
{img("2a/scope.png", "SCOPE on, with the ADVANCED row open under it.")}
{legend([
 ("BARS", "how much of the song the scope shows: 1/4, 1/2, 1, 2 or 4 bars"),
 ("MATCH / IN", "draw the output at the input level, and the original as an outline"),
 ("HOLD", "freeze the display to look closer"),
 ("Scope", "the output wave on top, the gain reduction lane under it"),
 ("ADVANCED", "MORE opens the row of extra controls"),
 ("EMPHASIS", "makes the cell react more to the highs"),
 ("MIX", "parallel compression: dry against processed"),
 ("IN TRIM", "&plusmn;24 dB before the unit"),
 ("OUT TRIM", "-24 to +12 dB after the unit"),
 ("CELL", "SLOW, CLASSIC or FAST photocell"),
])}
"""})

    S.append({"title": "Controls reference", "sub": "what every control does, its range and default", "html": f"""
{ctl("PEAK REDUCTION", "0 to 100<br>default 0<br>automatable", "<p>How much the signal drives the light, and so how hard the cell pulls the level down. At 0 there is no reduction. Like on the hardware there is no threshold, attack or release: the cell's own timing follows the material.</p>", "Shift = fine, double-click = default.")}
{ctl("GAIN", "0 to 100 on the dial<br>default 21<br>automatable", "<p>The output amplifier - the make-up gain. It does not change how much the unit compresses. The readout shows the dial position and the real gain in dB.</p>")}
{ctl("LIMIT / COMPRESS", "COMPRESS / LIMIT<br>default COMPRESS<br>automatable", "<p><b>COMPRESS</b> is the gentle, low-ratio leveling the unit is known for. <b>LIMIT</b> raises the ratio so the peaks are held more firmly.</p>")}
{ctl("METER", "+10 / GR / +4<br>default GR", "<p>What the VU meter shows. <b>GR</b>: gain reduction in dB. <b>+10</b> and <b>+4</b>: the output level, with 0 VU at +10 or +4 dBu as on the hardware.</p>", "Click to step, or turn it with the mouse wheel. The meter is not a sound parameter and is not automated.")}
{ctl("BYPASS", "on / off", "<p>Takes the whole unit out of the path. Lit = bypassed.</p>")}
<h3>SCOPE</h3>
{ctl("SCOPE", "on / off", "<p>Opens the scope under the unit; the window grows down and remembers it with the project.</p>")}
{ctl("BARS", "1/4, 1/2, 1, 2, 4 bars<br>default 1", "<p>The width of the scope, locked to the song's bars while the host plays.</p>")}
{ctl("MATCH", "on / off", "<p>Draws the output at the input level - the make-up gain removed - so you see only what the leveling changed.</p>")}
{ctl("IN", "on / off", "<p>Draws the original signal as an outline behind the output.</p>")}
{ctl("HOLD", "on / off", "<p>Freezes the display. On the frozen screen the mouse wheel zooms, dragging scrolls and a double-click shows the whole window again. Hover the scope to read the position in the bar and the input, output and gain reduction there.</p>")}
<h3>ADVANCED</h3>
{ctl("EMPHASIS", "FLAT to 100 %<br>default FLAT", "<p>Tilts the cell's detection towards the high frequencies, so bright, sibilant parts are reduced more and the low end drives it less. FLAT is the unit's standard response.</p>")}
{ctl("MIX", "0 to 100 %<br>default 100 %", "<p>Dry against processed: lower it for parallel compression inside the plugin.</p>")}
{ctl("IN TRIM / OUT TRIM", "IN -24 to +24 dB, OUT -24 to +12 dB<br>default 0 dB", "<p>Level trims before and after the unit. IN TRIM sets how hard your track hits the cell without moving PEAK REDUCTION; OUT TRIM sets the final level.</p>")}
{ctl("CELL", "SLOW / CLASSIC / FAST<br>default CLASSIC", "<p>The photocell's timing. CLASSIC is the measured silver unit. SLOW is the older, lazier cell (it grabs in about 150 ms and takes seconds to let go), for long, smooth leveling; FAST is CLASSIC letting go sooner, for parts that move quickly.</p>")}
"""})

    S.append({"title": "Step-by-step workflows", "sub": "recipes", "html": f"""
{recipe("The natural vocal", "COMPRESS, CLASSIC",
 steps([
  "COMPRESS, CELL CLASSIC.",
  "PEAK REDUCTION up until the loud lines read 3-6 dB of GR.",
  "GAIN to match the bypassed level. Open SCOPE to see the phrases come out even.",
 ]))}
{recipe("Round, even bass", "LIMIT",
 steps([
  "LIMIT, CELL CLASSIC or FAST.",
  "PEAK REDUCTION until every note reads about the same on the scope.",
  "MIX 100 %; GAIN to taste.",
 ]))}
{recipe("A touch of glue", "the mix bus",
 steps([
  "COMPRESS, CELL SLOW.",
  "PEAK REDUCTION for 1-2 dB of GR at most.",
  "Pull MIX to 50-70 % if the transients soften too much.",
 ]))}
"""})

    S.append(G["conventions_section"](m))
    S.append(G["support_section"](m, [
        ("No gain reduction", "PEAK REDUCTION is at 0, or the track arrives too quietly - turn PEAK REDUCTION up or raise IN TRIM."),
        ("Esses jump out", "Raise EMPHASIS in ADVANCED so the highs drive the cell harder."),
        ("The scope stays empty", "It draws while the host plays. Press play; HOLD may be on."),
        ("Too much pumping on the bass", "Use CELL SLOW, or lower PEAK REDUCTION."),
    ]))

    m["sections"] = S
    return m
