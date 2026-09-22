def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    m = {
        "id": "clipper", "product": "RONE Clipper", "eyebrow": "HARD CLIPPER THAT SHOWS YOU WHAT IT CUT",
        "title_html": "RONE <i>Clipper</i>", "accent": "#3D8BFF", "version": "1.0",
        "tagline": "A hard clipper you can trust with your eyes: the waveform after the clip stage, the removed peaks in blue, one CLIP knob - and a LOW CUT that keeps kick and bass out of the clipper.",
        "formats": ["VST3", "AU", "Standalone"], "vst3": "RONE Clipper.vst3", "au": "RONE Clipper.component", "exe": "RONE Clipper.exe",
        "pdf": "RONE Clipper - User Manual.pdf", "cover_img": "clipper/tour_knob.png",
    }
    S = []

    S.append({"title": "Welcome to RONE Clipper", "sub": "one threshold, no surprises", "html": f"""
<p><strong>RONE Clipper</strong> is a symmetric hard clipper: everything above the threshold is cut, everything below it passes untouched. What makes it a RONE plugin is that you <em>see</em> it. The graph shows the waveform after the clip stage, the peaks that were removed are painted blue, and the dashed line is the CLIP knob itself - turn it down and the history on screen recolours to show what that setting would have cut.</p>
<p>Two things clippers usually get wrong are handled for you. <strong>LOW CUT</strong> lets the band below a frequency you choose bypass the clipper completely, and filters out the low-frequency products the clipping itself creates, so a kick or a bass keeps its weight and the low end stays clean. And the oversampling was measured, not assumed: with nothing clipping the plugin nulls against its input at -100 dB.</p>
<div class="two">
<div>
<h3>Where it shines</h3>
<ul>
<li><strong>Drums and drum buses</strong> - shave the transient tops instead of turning a limiter into a pump.</li>
<li><strong>Before the limiter</strong> - 1-3 dB of clipping buys the limiter headroom without the smear.</li>
<li><strong>Kick + bass</strong> - LOW CUT keeps the fundamental out of the clipper; only the click and the harmonics get shaped.</li>
<li><strong>Loudness with a picture</strong> - SAVED tells you how much headroom you just freed.</li>
</ul>
</div>
<div>
<h3>The 10-second version</h3>
{steps([
 "Insert Clipper on a drum bus.",
 "Play the track and watch the waveform.",
 "Turn CLIP down until the blue caps appear on the loud hits.",
 "Press DELTA to hear only what is being removed, then release it."])}
</div></div>
{note("Hard, not soft", "There is one transfer function and it is a clamp. No knee, no saturation, no attack or release, no hidden ceiling. If the output should not exceed a level, that is a limiter's job - RONE Clipper reports its true output peak in ADVANCED instead of pretending.")}
"""})

    S.append(G["install_section"](m))

    S.append({"title": "Quick start", "sub": "your first clip", "html": f"""
{img("clipper/tour.png", "<b>RONE Clipper.</b> The waveform after the clip stage on top, the CLIP knob below, LOW CUT beside it.", "w60")}
{steps([
 "<strong>Insert</strong> Clipper on a drum bus, a kick or the mix bus.",
 "<strong>Play</strong> the track. The graph scrolls the last 8 seconds; the dashed line is the threshold.",
 "<strong>Turn <span class='pill'>CLIP</span> down.</strong> As the line drops below the loudest peaks, the parts above it turn blue - that is what the clipper removes. Start around 2-3 dB of blue on the biggest hits.",
 "<strong>Press <span class='pill'>DELTA</span></strong> to hear nothing but the removed part. If it sounds like clicks and transient tops, you are in the right place; if you hear tone, back off or raise LOW CUT.",
 "<strong>Turn <span class='pill'>LOW CUT</span> up</strong> to about 100-200 Hz on anything with a kick or a bass. The band below it bypasses the clipper and comes back untouched.",
 "<strong>Open ADVANCED</strong> for the INPUT / OUTPUT faders, the oversampling choice and AUTO GAIN.",
])}
{note("Drag to zoom", "Press on the graph and drag up to see fewer seconds (a faster scroll), drag down for more, double-click for the full 8 seconds. In DETAIL the same drag zooms between 5 and 500 milliseconds around the strongest clipped event.")}
"""})

    S.append({"title": "Interface tour", "sub": "every element on screen", "html": f"""
<div class="two">
{img("clipper/tour.png", "Compact view: WAVE with a drum loop clipping at -4.5 dBFS.")}
{img("clipper/adv.png", "ADVANCED open: the window grows to hold the faders.")}
</div>
{legend([
 ("Header logo", "click to flip to the back panel (About, version, licence)"),
 ("DELTA", "hear only what the clipper removed"),
 ("BYPASS", "latency-matched dry signal for A/B, 10 ms crossfade"),
 ("WAVE / PEAKS / DETAIL", "the three views of the same capture"),
 ("FREEZE", "holds the display; audio keeps running"),
 ("Reset", "clears the history and the held maximum"),
 ("Graph", "the last 8 seconds; drag up / down = time zoom, the dashed line is CLIP"),
 ("CLIP", "the threshold, -24 to 0 dBFS"),
 ("CLIP value", "double-click to type a value"),
 ("LOW CUT", "the band below this frequency bypasses the clipper; double-click = off"),
 ("ADVANCED", "opens the section below the knob"),
 ("Resize grip", "drag to resize the window"),
])}
<p>In ADVANCED: <span class="b" style="display:inline-flex;align-items:center;justify-content:center;width:5mm;height:5mm;border-radius:50%;background:#3D8BFF;color:#101216;font-weight:800;font-size:8pt">1</span> OS &nbsp; <span class="b" style="display:inline-flex;align-items:center;justify-content:center;width:5mm;height:5mm;border-radius:50%;background:#3D8BFF;color:#101216;font-weight:800;font-size:8pt">2</span> SAVED &nbsp; <span class="b" style="display:inline-flex;align-items:center;justify-content:center;width:5mm;height:5mm;border-radius:50%;background:#3D8BFF;color:#101216;font-weight:800;font-size:8pt">3</span> INPUT &nbsp; <span class="b" style="display:inline-flex;align-items:center;justify-content:center;width:5mm;height:5mm;border-radius:50%;background:#3D8BFF;color:#101216;font-weight:800;font-size:8pt">4</span> OUTPUT &nbsp; <span class="b" style="display:inline-flex;align-items:center;justify-content:center;width:5mm;height:5mm;border-radius:50%;background:#3D8BFF;color:#101216;font-weight:800;font-size:8pt">5</span> AUTO GAIN.</p>
"""})

    S.append({"title": "Controls reference", "sub": "what every control does, its range and default", "html": f"""
{img("clipper/tour_knob.png", "<b>CLIP.</b> The white dot is the threshold; the arc lights up as it comes down.", "w60")}
{ctl("CLIP", "-24 to 0 dBFS<br>default 0<br>automatable, smoothed (10 ms)", "<p>The threshold of the clamp, inside the oversampled stage. Everything above +CLIP or below -CLIP is cut flat; everything in between is untouched. It is drawn on the graph as the dashed line, so you set it by eye against the peaks as much as by ear.</p><p>It is not an output ceiling: after the reconstruction filter a clipped transient can overshoot the threshold by up to about a decibel (the Gibbs ringing every oversampled clipper has). The output peak in ADVANCED tells the truth.</p>", "Double-click the knob for 0 dBFS. Double-click the value to type one. Right-click for the host's automation menu. The mouse wheel moves it too (Shift = fine).")}
{ctl("LOW CUT", "off / 20 to 400 Hz<br>default off<br>automatable", "<p>Everything below this frequency bypasses the clipper and is added back untouched, through a Linkwitz-Riley crossover that keeps the sum exact when nothing clips. Only the band above it gets clipped.</p><p>Clipping the upper band also generates low-frequency products of its own (a clipped hat loop grows a bump between 20 and 400 Hz that was never in the source). Those are removed too, with a linear-phase filter, so the clipped peaks stay exactly where the threshold put them and nothing new appears below LOW CUT.</p>", "150-250 Hz on a kick or a bass keeps the fundamental whole while the click gets shaped. Note the graph shows what enters the clipper: with LOW CUT up, the lows disappear from the picture on purpose.")}
{ctl("DELTA", "on / off<br>automatable", "<p>The output becomes only the part the clipper removed - the input minus the clipped signal. It is the quickest way to judge a setting: transient tops and clicks are what you want to be cutting; a tone or a pitched bass means the threshold is in the body of the sound.</p>", "BYPASS wins over DELTA.")}
{ctl("BYPASS", "", "<p>Latency-matched dry signal with a 10 ms crossfade, so A/B comparison lands sample-accurately. The engine keeps running underneath.</p>")}
<h3>The graph</h3>
{img("clipper/peaks.png", "<b>PEAKS.</b> The rectified peak history: grey before, white after, blue removed.", "w60")}
{ctl("WAVE", "default view", "<p>The signed waveform after the clip stage, like a scope, for the last 8 seconds. White is the signal after clipping, blue is what was removed beyond +/-CLIP, both polarities. Amplitude is linear, so quiet material sits near the centre line and the peaks own the height - exactly what you want to see in a clipper.</p>")}
{ctl("PEAKS", "", "<p>The same history as a rectified peak envelope on a linear scale: grey is the pre-clip envelope, white the clipped one, blue the excess above the line. Easier to read on dense material.</p>")}
{ctl("DETAIL", "5 to 500 ms", "<p>The strongest clipped event of the last half second as actual oversampled samples of one channel (L / R selector), threshold at +/-1. Hover a column for BEFORE / AFTER / CUT in dBFS. Drag up and down to zoom the window.</p>")}
{ctl("Time zoom", "drag up / down<br>wheel<br>double-click", "<p>Press on the graph and drag up to see fewer seconds on screen (the display scrolls faster), drag down to see more, continuously between half a second and eight. The wheel does the same in steps; a double-click returns to 8 seconds. The vertical scale never changes: the top of the plot is 0 dBFS, or the INPUT gain when it is above 0.</p>")}
{ctl("FREEZE / Reset", "", "<p>FREEZE holds the graph and a snapshot of the raw samples for DETAIL; audio and the meters keep running and the badge says FROZEN. Reset clears the history and the held maximum.</p>")}
<h3>ADVANCED</h3>
{img("clipper/adv_panel.png", "<b>OS, SAVED, INPUT / OUTPUT and AUTO GAIN.</b>", "w60")}
{ctl("OS", "1x 2x 4x 8x 16x<br>default 4x<br>not automatable", "<p>How many times the sample rate the clip stage runs at. A hard clip creates harmonics without end; oversampling keeps them from folding back into the audible band as aliasing. Measured on a clipped 9.7 kHz tone the aliased energy is -9 dB at 1x, -29 at 2x, -43 at 4x, -54 at 8x and -66 at 16x. The latency is the same at every setting and the same setting is used in playback and export.</p>", "4x costs under one percent of a core and is the default. 8x for a master. 1x only when you want the aliasing as a colour.")}
{ctl("SAVED", "dB, last second", "<p>How much the peaks came down through the clipper: the pre-clip peak against the peak after reconstruction, before OUTPUT gain. Raise OUTPUT by this much and the RMS goes up by the same amount. MAX is held since the last reset; click it to clear.</p>")}
{ctl("INPUT", "-24 to +24 dB<br>default 0<br>automatable", "<p>Gain before the clip stage - the drive. The bar beside the fader is the incoming host level before this gain. Double-click the well to type a value.</p>")}
{ctl("OUTPUT", "-24 to +12 dB<br>default 0<br>automatable", "<p>Gain after reconstruction; it never changes what gets clipped. The bar beside it is the actual final output level.</p>")}
{ctl("AUTO GAIN", "on / off", "<p>Make-up for the threshold: the OUTPUT is raised by exactly what CLIP took away, so the clipped peaks land back at 0 dBFS. OUTPUT still works as an offset on top.</p>")}
"""})

    S.append({"title": "Step-by-step workflows", "sub": "recipes", "html": f"""
{recipe("Drum bus, 2 dB of transient shaving", "Punch without a pumping limiter",
 steps([
  "Insert Clipper on the drum bus, OS at 4x.",
  "Play the loudest section and turn CLIP down until the biggest hits show 2-3 dB of blue (SAVED in ADVANCED reads 2-3 dB).",
  "Press DELTA: you should hear clicks and the tops of the snare, not the body of the kick. If the kick is in there, raise LOW CUT to about 120 Hz.",
  "Release DELTA, switch AUTO GAIN on, and compare with BYPASS at equal loudness.",
 ]))}
{recipe("Kick + bass, keep the low end whole", "The fundamental never touches the clipper",
 steps([
  "Insert on the kick or the low-end group.",
  "Set LOW CUT to 150-200 Hz. The graph now shows only what enters the clipper - the click and the harmonics.",
  "Bring CLIP down until the click is trimmed. The sub passes through untouched and nothing new grows below LOW CUT.",
  "Check DETAIL on one hit: the white trace is the trimmed click, the blue is what went.",
 ]))}
{recipe("Before the limiter", "Buy the limiter 1-3 dB",
 steps([
  "Insert Clipper before the mastering limiter, OS at 8x.",
  "Clip 1-2 dB off the peaks (watch SAVED).",
  "Lower the limiter's input by the same amount. The limiter now works less, and the transients keep their edge.",
 ]))}
"""})

    S.append({"title": "Tips, tricks and troubleshooting", "sub": "", "html": f"""
<h3>Tips</h3>
<ul>
<li><strong>Set it with DELTA.</strong> Two seconds of listening to the removed part tells you more than any meter.</li>
<li><strong>The line is the knob.</strong> Turning CLIP recolours the whole history on screen, so you can find the right depth on material that already scrolled by.</li>
<li><strong>Drag on the graph</strong> to zoom time in or out; double-click resets. In DETAIL, hover for the numbers.</li>
<li><strong>SAVED = loudness.</strong> Whatever it reads, that is how far you can push OUTPUT (or how much less the limiter has to do).</li>
</ul>
<h3>Troubleshooting</h3>
{table(["Symptom", "Cause", "Fix"], [
 ["The graph says WAITING FOR AUDIO", "No signal has reached the plugin yet", "Play the track; the graph starts on the first sample"],
 ["The output peaks above CLIP", "Reconstruction overshoot after the oversampling filter (up to about 1 dB on hard transients)", "Expected. Use a limiter after the clipper if a hard ceiling is required"],
 ["LOW CUT makes the graph emptier", "The band below LOW CUT bypasses the clipper and is not drawn", "Intended - the graph shows what enters the clipper"],
 ["Latency", "384 samples at 48 kHz (8 ms): the oversampling plus the linear-phase LOW CUT filter, constant in every setting", "The host compensates it automatically"],
 ["Aliasing on bright material at 1x", "A hard clip at the base rate folds its harmonics back", "Use 4x or 8x"],
])}
"""})

    m["sections"] = S
    return m
