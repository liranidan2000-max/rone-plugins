def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    m = {
        "id": "stutter", "product": "RONE Stutter", "eyebrow": "TEMPO-LOCKED STUTTER ENGINE",
        "title_html": "RONE <i>Stutter</i>", "accent": "#FFD02B", "version": "1.4",
        "tagline": "Drop a sound in and it is already a stutter - rendered on the first hit, bar-exact, ready to pull straight out of the waveform into your arrangement.",
        "formats": ["VST3", "AU", "Standalone"], "vst3": "RONE Stutter.vst3", "au": "RONE Stutter.component", "exe": "RONE Stutter.exe",
        "pdf": "RONE Stutter - User Manual.pdf", "cover_img": "stutter/tour_grid.png" and "stutter/empty.png",
        "latency": "None (offline render, playback only)",
    }
    m["cover_img"] = "stutter/empty.png"
    S = []

    S.append({"title": "Welcome to RONE Stutter", "sub": "what it is, when to reach for it", "html": f"""
<p><strong>RONE Stutter</strong> is a stutter <em>renderer</em>, not a live chopper. You give it a sound, tell it which moment of that sound to grab and which note value to repeat it on, and it writes a finished, bar-exact stutter fill. The result is auditioned inside the plugin and then exported, or simply dragged into your DAW as an audio clip.</p>
<p>That workflow is deliberate. Live stutter effects are great on stage; in a production session they cost you automation clips, timing corrections and re-renders. Stutter gives you a printed clip that lands exactly on the grid, every time.</p>
<div class="two">
<div>
<h3>Where it shines</h3>
<ul>
<li><strong>Fills and edits</strong> - the classic "snare roll into the drop", a vocal syllable repeated 1/16 then 1/32.</li>
<li><strong>Glitch builds</strong> - repeat a tiny slice for two bars with a fade-in so it swells out of nothing.</li>
<li><strong>Transition FX</strong> - grab a cymbal or a word and turn it into a rhythmic riser.</li>
<li><strong>Sound design</strong> - repeat a slice at 1/32 with 100% STEREO for a buzzing, ping-ponging texture.</li>
</ul>
</div>
<div>
<h3>The 10-second version</h3>
{steps([
 "Drop an audio file on the waveform. It is rendered on the spot.",
 "Listen with <span class='pill'>&#9654; RESULT</span>.",
 "Not the right hit? <span class='pill'>NEXT &#9654;</span> re-rolls it from the next one.",
 "Change GRID or BARS and it re-renders by itself.",
 "Drag the clip out of the waveform into your DAW."])}
</div></div>
{note("How the render works", "Stutter copies the selected slice at every grid position for the chosen number of bars. Each copy gets its own fade in / fade out, odd and even copies can be pushed left and right for stereo motion, and the whole clip can fade globally. Because it is rendered offline the timing is sample-accurate and independent of your buffer size.")}
"""})

    S.append(G["install_section"](m))

    S.append({"title": "Quick start", "sub": "first stutter in one minute", "html": f"""
<div class="two">
{img("stutter/empty.png", "<b>Fresh instance.</b> Everything waits for a file: drop one on the dashed area, double-click it, or press BROWSE.")}
{img("stutter/result.png", "<b>One second later.</b> The file was dropped and the fill already exists - no transient to pick, no button to press. This is the minimal page; ADVANCED opens the rest.")}
</div>
{steps([
 "<strong>Insert the plugin</strong> on any mixer channel. An empty channel is ideal, because the plugin plays its previews through that channel. In the standalone app just open it.",
 "<strong>Load a file.</strong> Drag a WAV, AIFF, FLAC or MP3 onto the waveform, double-click the waveform to browse, or press <span class='pill'>BROWSE</span>. The header shows the file name, length and sample rate.",
 "<strong>Choose the moment</strong> - if the automatic one is not the one you wanted. Stutter starts on the first transient that actually sounds; <span class='pill'>&#9664; PREV</span> / <span class='pill'>NEXT &#9654;</span> re-roll the fill from the neighbouring hits, and clicking the ORIGINAL waveform sets the start by hand. Every one of those renders again immediately.",
 "<strong>Set the rhythm.</strong> GRID is the note value of one repeat (1/16 is the classic roll). The SLICE chip tells you how long one repeat is at the current BPM. BARS is the length of the finished clip.",
 "<strong>Check BPM.</strong> Inside a DAW the tempo is read from the host and the chip shows <em>DAW</em>. In the standalone app, or if the host reports no tempo, type the BPM into the box (the chip shows <em>MANUAL</em>).",
 "<strong>Shape it.</strong> FADE IN / FADE OUT soften each repeat; STEREO bounces alternating repeats left and right.",
 "<strong>Listen.</strong> One transport button plays whatever the waveform is showing - the toggle in the top-right corner of the waveform chooses. Every change above re-renders on its own; the <span class='pill'>STUTTER</span> button in ADVANCED is only there when you want to force one.",
 "<strong>Get it into the song.</strong> Press the mouse anywhere on the lower part of the RESULT waveform and pull into your DAW's arrangement, or press <span class='pill'>EXPORT</span> to save a WAV.",
])}
{img("stutter/tour.png", "<b>ADVANCED.</b> The same instance with everything unfolded: RAMP TO, CURVE, FADE IN and the STUTTER button come back, and EXTRAS appears next to ADVANCED in the footer.", "w80")}
"""})

    S.append({"title": "Interface tour", "sub": "every element on screen", "html": f"""
<p>The page opens <strong>minimal</strong>: the waveform, GRID, SLICE, BARS, FADE OUT, STEREO, one transport and EXPORT. That is the whole plugin for most fills. <span class='pill'>ADVANCED</span> in the footer brings the rest of the controls bar and the <span class='pill'>STUTTER</span> button back; <span class='pill'>EXTRAS</span>, which appears beside it, opens the drawer. The picture shows everything unfolded.</p>
{img("stutter/tour.png", "RONE Stutter with a drum loop loaded, ADVANCED on.")}
{legend([
 ("Header logo", "click to flip to the back panel (About, version, licence)"),
 ("Loaded file", "name, duration and sample rate of the source"),
 ("BROWSE", "open a file dialog (you can also drop files or double-click the waveform)"),
 ("Waveform", "the source or the result; click to set the slice start, scroll to zoom, and pull the result out of the lower part into your DAW"),
 ("PREV / NEXT", "re-roll the fill from the previous / next transient"),
 ("ORIGINAL / RESULT", "which of the two the waveform shows - and therefore what the transport plays"),
 ("GRID", "note value of one repeat, 1/2 to 1/32 - or FREE, a length in milliseconds off the tempo grid"),
 ("RAMP TO", "OFF, 1/16, 1/32 or 1/64: the roll accelerates from the GRID to this value across the bars"),
 ("CURVE", "LIN: every stage takes the same time; EXP: each finer stage takes half the time"),
 ("SLICE", "length of one repeat in milliseconds at the current BPM"),
 ("BARS", "length of the rendered clip, 0.5 to 8 bars in half-bar steps"),
 ("BPM", "tempo used for the grid; DAW (read from the host) or MANUAL (typed). Minimal hides it while the host supplies one"),
 ("FADE IN", "fade at the start of every repeat"),
 ("FADE OUT", "fade at the end of every repeat"),
 ("STEREO", "ping-pong amount: alternate repeats lean left / right"),
 ("FADE OUT AUTO", "OFF / up / down: FADE OUT travels from the knob to 100 % or 0 % across the clip; the outer ring shows the road"),
 ("STEREO AUTO", "the same for STEREO - 100 % and down starts in full ping-pong and lands in the centre"),
 ("PLAY", "audition whatever the waveform is showing"),
 ("ON BAR", "when lit, playback starts on the next bar line so the fill is heard in context"),
 ("STUTTER", "force a render; normally every change renders itself"),
 ("EXPORT", "save the rendered clip as a WAV file"),
 ("ADVANCED", "unfold the rest of the controls bar and the STUTTER button"),
 ("EXTRAS", "open the drawer: MIX, fade curves, global fades, PITCH, PITCH RAMP, KEY and SCALE"),
 ("Status line", "what the plugin is doing and what it expects next"),
 ("Resize grip", "drag to resize the window"),
])}
"""})

    S.append({"title": "Controls reference", "sub": "what every control does, its range and default", "html": f"""
<h3>Source and selection</h3>
{ctl("Waveform", "click: set slice start<br>scroll: zoom<br>drag scrollbar: pan", "<p>Shows the source (grey) or the result (yellow). Clicking sets the start of the slice that will be repeated; the selection is shown as a bright line. Detected transients are marked with triangles above the waveform. The time ruler under the waveform follows zoom.</p>", "Zoom in for precision on fast material - a snare has its energy in the first few milliseconds, and starting a few milliseconds early gives a cleaner attack.")}
{ctl("PREV / NEXT", "transient navigation", "<p>Moves the selection to the previous / next detected transient. The status line tells you how many transients were found. If the detector finds nothing (pads, noise), click the waveform to select manually.</p>")}
{ctl("BROWSE", "file dialog", "<p>Loads WAV, AIFF, FLAC, MP3 or OGG. Stereo and mono files are both fine; a mono file renders a mono clip.</p>")}
<h3>Rhythm</h3>
{img("stutter/tour_grid.png", "<b>GRID, SLICE, BARS and BPM.</b>", "w80")}
{ctl("GRID", "1/2 &middot; 1/4 &middot; 1/6 &middot; 1/8 &middot; 1/12 &middot; 1/16 &middot; 1/32 &middot; FREE<br>default 1/16", "<p>The note value of one repeat. 1/6 and 1/12 are triplet values. The repeat length is calculated from the BPM: at 120 BPM a 1/16 repeat is 125 ms, at 128 BPM it is 117 ms.</p>", "Automate nothing - render two clips (1/16 and 1/32) and cut between them in the arrangement. It is faster and always in time.")}
{ctl("RAMP TO and CURVE", "OFF &middot; 1/16 &middot; 1/32 &middot; 1/64<br>LIN &middot; EXP<br>default OFF / LIN", "<p>The accelerating roll in one render. From the GRID value the repeats double in rate, stage by stage, until they reach RAMP TO at the end of the clip: GRID 1/8 with RAMP TO 1/32 renders 8ths, then 16ths, then 32nds - the fill every producer arranges by hand from three separate renders. Every stage holds a whole number of repeats, so each stage change lands on the grid. <strong>LIN</strong> gives every stage the same share of the clip; <strong>EXP</strong> halves the share for each finer stage, so the roll spends most of the clip on the slow repeats and snaps into the fast ones at the end.</p><p>Triplet grids double too (1/6 to 1/12 to 1/24), and a target below the GRID simply renders the GRID.</p>", "GRID 1/8, RAMP TO 1/64, BARS 1, EXP: the last beat is a buzz. Add PITCH RAMP +12 and it rises with it.")}
{img("stutter/free.png", "<b>FREE and TREMOLO.</b> GRID on FREE: the SLICE chip is the repeat length and can be dragged; TREMOLO takes RAMP TO's place, with its END length beside it.", "w80")}
{ctl("FREE", "the last GRID button<br>SLICE: 5 to 2000 ms<br>default 120 ms", "<p>Takes the repeat length off the tempo grid. The SLICE chip turns yellow and becomes the control: drag it up or down, use the wheel, hold shift for fine steps. The scale is logarithmic, so the fast rolls and the slow ones get the same room.</p><p>Only the <em>rate</em> is free - the clip is still BARS long at the song tempo, so it still ends exactly on the next bar line. RAMP TO does not apply in FREE (it doubles grid values); TREMOLO takes its place.</p>", "FREE is for the lengths no grid has: 37 ms on a vocal is a buzz with a pitch the grid would never land on.")}
{ctl("TREMOLO", "OFF &middot; ON, plus END<br>FREE only<br>END: 5 to 2000 ms, default 30 ms", "<p>Glides the repeat length continuously from SLICE to END across the clip. SLICE longer than END: wide pulses that tighten into a buzz. SLICE shorter than END: a buzz that opens up into slow pulses. CURVE sets the shape of the glide - LIN evenly, EXP holding back and rushing at the end.</p><p>Unlike RAMP TO, which jumps between grid values in stages, TREMOLO has no steps: every repeat is a little shorter (or longer) than the one before.</p>", "SLICE 180 ms, END 25 ms, CURVE EXP, one bar: the classic tape-stop-in-reverse riser into a drop.")}
{ctl("SLICE", "read-only", "<p>The length of one repeat in milliseconds. It changes when you change GRID or BPM. If the selected slice of audio is shorter than this, the repeat simply contains silence after the hit.</p>")}
{ctl("BARS", "0.5 to 8 bars, half-bar steps<br>default 2", "<p>The length of the rendered clip. Repeats are placed until the clip is full; if the clip length is not an exact multiple of the grid, the last repeat is cut to fit.</p>")}
{ctl("BPM", "20 to 300<br>default 120<br>DAW or MANUAL", "<p>When the plugin runs inside a DAW that reports its tempo, the box is locked and shows the host tempo (chip reads <em>DAW</em>). Otherwise type a tempo; the chip reads <em>MANUAL</em>. The grid, the SLICE readout and the clip length all follow this value.</p>", "Rendering at the song's tempo is what makes the clip land on the grid when you drag it in. If you later change the song tempo, render again.")}
<h3>Shape</h3>
{img("stutter/tour_knobs.png", "<b>FADE IN, FADE OUT, STEREO</b> and the AUTO chips under the last two. The thin outer ring is where AUTO walks each knob.", "w60")}
{ctl("FADE IN", "0 to 100 % of a repeat<br>default 0 %<br>curve: quadratic", "<p>A fade at the start of every repeat. Small values (5-15 %) remove clicks when the slice does not start at a zero crossing; large values turn hard hits into soft pulses.</p>")}
{ctl("FADE OUT", "0 to 100 % of a repeat<br>default 0 %<br>curve: quadratic", "<p>A fade at the end of every repeat. Use it to make each repeat decay before the next one, which gives the classic 'gated' roll.</p>", "FADE OUT around 60-80 % with 1/32 GRID sounds like a machine-gun roll; 0 % sounds like a hard loop.")}
{ctl("STEREO", "0 to 100 %<br>default 0 %", "<p>Ping-pong: even repeats are attenuated in the right channel and odd repeats in the left, by the amount you set. 100 % alternates fully left / right; 30 % gives gentle movement that still sums to mono without holes.</p>", "Check the mix in mono when you push STEREO high - full ping-pong at 1/32 turns into a buzz in mono.")}
{ctl("AUTO", "OFF &middot; up &middot; down<br>under FADE OUT and STEREO<br>default OFF", "<p>Moves the knob by itself across the clip. The knob is where the clip <em>starts</em>; up walks it to 100 % and down to 0 %, repeat by repeat, following CURVE (LIN: a straight line, EXP: slow first, fast at the end). The last repeat always gets the full value.</p><p>Two knobs have it because they are the two that tell a story over a fill. STEREO 100 % with AUTO down opens in full left / right ping-pong and closes in the centre, right into the drop. FADE OUT 0 % with AUTO up starts with repeats that ring into each other and ends with tight, gated hits.</p><p>A thin ring outside the knob's arc shows the stretch it will travel, and while the RESULT plays a dot rides that ring at the value the repeat under the playhead got - the same idea as an LFO display in a synth. On the RESULT waveform the stereo colouring (yellow / white repeats) fades along with it.</p>", "STEREO 100 % down + FADE OUT 20 % up on a 1/16 snare roll: it starts wide and loose, and lands narrow and tight.")}
<h3>Pitch and key (ADVANCED)</h3>
{img("stutter/adv.png", "<b>The ADVANCED drawer.</b> MIX, the two fade curves and the global fades were host-only parameters until 1.2; PITCH, PITCH RAMP, KEY and SCALE are new.")}
{ctl("PITCH", "-12 to +12 semitones<br>default 0", "<p>Every repeat is re-pitched by this amount - the content moves, the grid does not. The slice is read faster or slower from the transient, so a repeat pitched up contains more of the source and one pitched down contains less.</p>", "+7 (a fifth) on a snare roll is the classic 'psy' fill; -12 on a vocal chop is a different singer.")}
{ctl("PITCH RAMP", "-24 to +24 semitones<br>default 0", "<p>Added to PITCH across the clip, from nothing at the start to the full amount on the last repeat (following the CURVE). A rising roll that also rises in pitch is the build; a falling one is the drop.</p>")}
{ctl("KEY and SCALE", "KEY: OFF, C .. B<br>SCALE: Phrygian, Phrygian dominant, harmonic minor, minor, major, root + 5th<br>default OFF / Phrygian", "<p>At 1/64 and beyond, a roll repeats faster than 20 times a second and the ear hears it as a note - the note is the repeat rate, whatever the slice contains. With a KEY chosen, every stage fast enough to be a tone is nudged to the nearest note of the scale, so the buzz at the end of the fill is in key with the bass instead of a random pitch. Slower stages stay exactly on the grid.</p>", "At 128 BPM 1/64 is 34 Hz, between C#1 and D1; KEY D puts it on the root.")}
{ctl("MIX", "0 to 100 %<br>default 100 % (LOCK)", "<p>The preview level in the channel. At 100 % the result replaces the channel (LOCK); below that the channel stays at full level and the result is added on top - a send, not a crossfade. It never touches the export.</p>")}
{ctl("IN CURVE / OUT CURVE", "0.1 to 4<br>default 2", "<p>The shape of FADE IN and FADE OUT on every repeat: 1 is linear, 2 a gentle curve, 4 sharp.</p>")}
{ctl("GLOBAL IN / GLOBAL OUT", "0 to 95 % of the clip<br>default 0", "<p>A fade over the whole clip on top of the per-repeat fades. GLOBAL IN at 90 % is a two-bar swell from silence.</p><p>They are usually set on the waveform rather than here. In the top band of the RESULT view each fade is a full-height <strong>column</strong> you can grab at any height - hover and it lights up - and dragging it sideways sets the length. The dot on the curve below is the shape: pull it up or down (the cursor turns vertical) to go from a gentle curve to a sharp one. Clicking the bare left or right end of the band starts a fade that does not exist yet.</p>")}
<h3>Transport and output</h3>
{img("stutter/tour_transport.png", "<b>ORIGINAL, STUTTER, RESULT and EXPORT.</b>", "w80")}
{ctl("&#9654; PLAY", "one transport", "<p>Plays whatever the waveform is showing, through the plugin's channel: the RESULT clip, or the source from the slice start when the view is on ORIGINAL. The label follows the view, the playback position is drawn on the waveform, and pressing it again stops. Switching the view while it plays stops playback rather than carrying on under the wrong name.</p>")}
{ctl("STUTTER", "ADVANCED only", "<p>Forces a render with the current settings. You rarely need it: a file that lands is rendered immediately, and every later change - GRID, BARS, the fades, STEREO, a new transient - renders again on its own. The button is there for the times you want to re-roll deliberately, and it is hidden on the minimal page.</p>")}
{ctl("ON BAR", "default on", "<p>When lit, ORIGINAL and RESULT do not start at the click but on the next bar line of the host's song position (in the standalone app, of the BPM clock). The button reads NEXT BAR while it waits. A fill previewed on the bar is heard exactly where it will sit in the arrangement.</p>", "Start the DAW two bars before the drop, press RESULT: the roll plays into the drop, in time, before you have exported anything.")}
{ctl("EXPORT", "save to the Exports folder", "<p>Saves the rendered clip into <code>Documents\\RONE Plugins\\Exports\\RONE Stutter\\</code> as <em>&lt;source&gt;_stutter_&lt;grid&gt;_&lt;bars&gt;_&lt;date&gt;_&lt;time&gt;.wav</em>, at the session's sample rate. The status line shows the file name. Nothing is ever deleted from that folder by the plugin.</p>")}
{ctl("Drag out of the waveform", "lower part of the RESULT view", "<p>Press the mouse anywhere on the lower part of the result and pull: the plugin writes the clip and hands it to your DAW as an audio drop. A small <strong>DRAG TO EXPORT</strong> badge marks the area. The file is saved to <code>Documents\\RONE Plugins\\Exports\\RONE Stutter\\</code> first - a permanent folder that is never cleaned up - so a project that references it always finds it again, even if your DAW does not copy dropped files into the project folder. It works into Explorer / Finder too.</p><p>The top of the waveform belongs to the fades, so a press there moves a fade instead of starting a drag. Nothing is ambiguous: the two areas do not overlap.</p>", "Drop it at the bar line where the fill should start. Because the clip is exactly N bars long, its end lands on the next bar line.")}
<h3>Host parameters</h3>
<p>Everything above is a host parameter: visible in the DAW's parameter list, automatable, mappable to a controller and saved with the project. Since 1.2 a project that is reopened also shows its file and selection in the window straight away.</p>
"""})

    S.append({"title": "Step-by-step workflows", "sub": "recipes you will use every week", "html": f"""
{recipe("Snare roll into a drop", "Fills, transitions, any genre",
 steps([
  "Bounce or drag the snare hit (or the whole loop) into Stutter.",
  "Select the snare transient with NEXT / PREV.",
  "GRID 1/8, RAMP TO 1/32, CURVE EXP, BARS 1, FADE OUT 50 %. Press STUTTER: one clip, 8ths into 16ths into 32nds.",
  "With ON BAR lit, press RESULT while the song plays into the drop to hear it in place.",
  "Drag the clip to the bar before the drop. Add a short reverse cymbal on top and you are done.",
 ]))}
{recipe("Vocal stutter build", "Vocal chops, pop / EDM intros",
 steps([
  "Load the vocal phrase and click the first syllable of the word you want (zoom in; a vowel start is usually cleaner than a consonant).",
  "GRID 1/8, BARS 2, FADE IN 10 %, FADE OUT 30 % so each repeat breathes.",
  "STEREO 40 % for movement. Press STUTTER.",
  "Set <em>Global Fade In</em> (host parameter) to 100 % if you want the two bars to rise from silence.",
  "Drag the result in, and send it to the same reverb as the lead vocal so it sits in the same space.",
 ]))}
{recipe("Glitch texture from a tiny slice", "Sound design, IDM, transitions",
 steps([
  "Load any percussive or noisy source; select a very short, bright moment.",
  "GRID 1/32, BARS 4, FADE IN 0 %, FADE OUT 0 %, STEREO 100 %.",
  "Render. You get a pitched, buzzing texture that sits hard left / right on alternate repeats.",
  "Render again with STEREO 0 % and layer both; low-pass the mono one, high-pass the wide one.",
 ]))}
{recipe("Triplet edit", "Trap, hip-hop, breaks",
 steps([
  "Select a hi-hat or clap. GRID 1/12 (that is a 1/8 triplet) or 1/6 (1/4 triplet).",
  "BARS 0.5, FADE OUT 70 %. Render and place it on the last half bar of a phrase.",
 ]))}
"""})

    S.append({"title": "Recommendations", "sub": "how to get the best out of it", "html": f"""
<div class="dos">
<div><h4>Do</h4><ul>
<li>Render at the project tempo and drop clips on bar lines; the timing will be perfect without nudging.</li>
<li>Use small FADE IN values (5-10 %) whenever you hear a click at the start of the repeats.</li>
<li>Keep a dedicated "Stutter" mixer channel. All previews play through it, so you can EQ and compress the auditions the same way you will treat the printed clip.</li>
<li>Render variations rather than automating: 1/8 &rarr; 1/16 &rarr; 1/32 clips placed back to back is the fastest way to build a roll that accelerates.</li>
<li>Exports are named for you (source, grid, bars, date) and collected in <code>Documents\\RONE Plugins\\Exports</code>; back that folder up with your projects.</li>
</ul></div>
<div><h4>Avoid</h4><ul>
<li>Selecting far ahead of the transient. Silence before the hit is repeated too, and the roll loses its punch.</li>
<li>Very long source files. Stutter only needs the hit; long files just slow down loading and detection.</li>
<li>STEREO at 100 % on low-frequency material; alternate-channel kicks cancel in mono.</li>
<li>Forgetting to re-render after changing settings - the RESULT you hear is always the last render.</li>
</ul></div>
</div>
<h3>Sound tips</h3>
<ul>
<li><strong>Fade out plus fade in</strong> at similar values turns a stutter into a tremolo-like pulse; useful on pads and vocals.</li>
<li><strong>Pitch is length.</strong> At 1/32 and 1/64-equivalent lengths (high BPM) the repeat rate reaches the audible range and the roll takes on a pitch. That pitch follows the tempo, which is why it always sounds in tune with the track's rhythm, if not its key.</li>
<li><strong>Layer with the original.</strong> Keep the untouched hit on beat one and start the stutter on the "and"; the first hit keeps its weight.</li>
</ul>
"""})

    S.append(G["conventions_section"](m, note("Standalone tempo", "The standalone app has no host clock; type the tempo into the BPM box before rendering so the clip fits the song you will drag it into.")))

    S.append(G["support_section"](m, [
        ("I cannot find the STUTTER button", "There is not one on the minimal page, because you do not need it: the file renders when it lands, and again after every change. Press ADVANCED in the footer to bring it back."),
        ("The roll is out of time in my DAW", "Check the BPM chip. If it says MANUAL inside a DAW, the host is not sending tempo (some hosts only do so while playing); type the project tempo and render again."),
        ("Dragging out does nothing", "Check you are on the RESULT view - ORIGINAL is not a drag source, so that clicking a transient stays instant - and that the press started on the lower part of the waveform. The top band belongs to the fades."),
        ("A fade moved when I meant to drag the clip out", "The press started in the top 40 % of the waveform, which is the fades' band. Start lower down - the DRAG TO EXPORT badge sits in the area that drags."),
        ("No transients are found", "The material is too soft or sustained for the detector. Click the waveform to place the selection by hand; everything else works the same."),
        ("The DAW says a dropped clip is missing", "Clips are saved to <code>Documents\\RONE Plugins\\Exports\\RONE Stutter</code>; point the DAW there. Versions before 1.1.1 wrote to the temp folder, which Windows and macOS clean up - re-export those clips once."),
    ]))

    m["sections"] = S
    return m
