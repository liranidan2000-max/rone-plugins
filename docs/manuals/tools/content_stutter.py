def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    m = {
        "id": "stutter", "product": "RONE Stutter", "eyebrow": "TEMPO-LOCKED STUTTER ENGINE",
        "title_html": "RONE <i>Stutter</i>", "accent": "#FFD02B", "version": "1.1",
        "tagline": "Load a hit, pick the transient, choose a grid and RONE Stutter renders a perfectly tempo-locked stutter you can drag straight into your arrangement.",
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
 "Drop an audio file on the waveform.",
 "Click the hit you want (or use PREV / NEXT).",
 "Pick a GRID value and how many BARS.",
 "Press STUTTER, listen with RESULT.",
 "Drag the result into your DAW or press EXPORT."])}
</div></div>
{note("How the render works", "Stutter copies the selected slice at every grid position for the chosen number of bars. Each copy gets its own fade in / fade out, odd and even copies can be pushed left and right for stereo motion, and the whole clip can fade globally. Because it is rendered offline the timing is sample-accurate and independent of your buffer size.")}
"""})

    S.append(G["install_section"](m))

    S.append({"title": "Quick start", "sub": "first stutter in one minute", "html": f"""
<div class="two">
{img("stutter/empty.png", "<b>Fresh instance.</b> Everything waits for a file: drop one on the dashed area, double-click it, or press BROWSE.")}
{img("stutter/tour.png", "<b>File loaded.</b> Transients are detected automatically and marked with triangles; the first one is selected (yellow line).")}
</div>
{steps([
 "<strong>Insert the plugin</strong> on any mixer channel. An empty channel is ideal, because the plugin plays its previews through that channel. In the standalone app just open it.",
 "<strong>Load a file.</strong> Drag a WAV, AIFF, FLAC or MP3 onto the waveform, double-click the waveform to browse, or press <span class='pill'>BROWSE</span>. The header shows the file name, length and sample rate.",
 "<strong>Choose the moment.</strong> Stutter finds the transients for you. Step through them with <span class='pill'>&#9664; PREV</span> / <span class='pill'>NEXT &#9654;</span>, or click anywhere on the waveform to set the slice start by hand. Play <span class='pill'>&#9654; ORIGINAL</span> to hear the source.",
 "<strong>Set the rhythm.</strong> GRID is the note value of one repeat (1/16 is the classic roll). The SLICE chip tells you how long one repeat is at the current BPM. BARS is the length of the finished clip.",
 "<strong>Check BPM.</strong> Inside a DAW the tempo is read from the host and the chip shows <em>DAW</em>. In the standalone app, or if the host reports no tempo, type the BPM into the box (the chip shows <em>MANUAL</em>).",
 "<strong>Shape it.</strong> FADE IN / FADE OUT soften each repeat; STEREO bounces alternating repeats left and right.",
 "<strong>Render.</strong> Press <span class='pill'>STUTTER</span>. The waveform switches to the RESULT view (yellow). Audition with <span class='pill'>&#9654; RESULT</span>; flip between ORIGINAL and RESULT with the toggle in the top-right corner of the waveform.",
 "<strong>Get it into the song.</strong> Drag from the <strong>DRAG TO EXPORT</strong> strip under the interface straight onto a track in your DAW, or press <span class='pill'>EXPORT</span> to save a WAV.",
])}
{img("stutter/result.png", "<b>After rendering.</b> The RESULT view shows the finished clip: the selected hit repeated on a 1/16 grid for 2 bars, with the status line confirming STUTTER APPLIED. 1 ORIGINAL / RESULT toggle.", "w80")}
"""})

    S.append({"title": "Interface tour", "sub": "every element on screen", "html": f"""
{img("stutter/tour.png", "RONE Stutter with a drum loop loaded.")}
{legend([
 ("Header logo", "click to flip to the back panel (About, version, licence)"),
 ("Loaded file", "name, duration and sample rate of the source"),
 ("BROWSE", "open a file dialog (you can also drop files or double-click the waveform)"),
 ("Waveform", "the source or the result; click to set the slice start, scroll to zoom"),
 ("PREV / NEXT", "step through the transients Stutter detected"),
 ("GRID", "note value of one repeat, 1/2 to 1/32"),
 ("SLICE", "length of one repeat in milliseconds at the current BPM"),
 ("BARS", "length of the rendered clip, 0.5 to 8 bars in half-bar steps"),
 ("BPM", "tempo used for the grid; DAW (read from the host) or MANUAL (typed)"),
 ("FADE IN", "fade at the start of every repeat"),
 ("FADE OUT", "fade at the end of every repeat"),
 ("STEREO", "ping-pong amount: alternate repeats lean left / right"),
 ("Play ORIGINAL", "audition the source file from the slice start"),
 ("STUTTER", "render the stutter with the current settings"),
 ("Play RESULT", "audition the rendered clip"),
 ("EXPORT", "save the rendered clip as a WAV file"),
 ("Status line", "what the plugin is doing and what it expects next"),
 ("Resize grip", "drag to resize the window"),
])}
<p>Below the interface (not visible in the picture) sits the <strong>DRAG TO EXPORT</strong> strip. Once a result exists, press the mouse on the strip and drag into your DAW's arrangement; the plugin hands over a WAV file. It also works into Explorer / Finder.</p>
"""})

    S.append({"title": "Controls reference", "sub": "what every control does, its range and default", "html": f"""
<h3>Source and selection</h3>
{ctl("Waveform", "click: set slice start<br>scroll: zoom<br>drag scrollbar: pan", "<p>Shows the source (grey) or the result (yellow). Clicking sets the start of the slice that will be repeated; the selection is shown as a bright line. Detected transients are marked with triangles above the waveform. The time ruler under the waveform follows zoom.</p>", "Zoom in for precision on fast material - a snare has its energy in the first few milliseconds, and starting a few milliseconds early gives a cleaner attack.")}
{ctl("PREV / NEXT", "transient navigation", "<p>Moves the selection to the previous / next detected transient. The status line tells you how many transients were found. If the detector finds nothing (pads, noise), click the waveform to select manually.</p>")}
{ctl("BROWSE", "file dialog", "<p>Loads WAV, AIFF, FLAC, MP3 or OGG. Stereo and mono files are both fine; a mono file renders a mono clip.</p>")}
<h3>Rhythm</h3>
{img("stutter/tour_grid.png", "<b>GRID, SLICE, BARS and BPM.</b>", "w80")}
{ctl("GRID", "1/2 &middot; 1/4 &middot; 1/6 &middot; 1/8 &middot; 1/12 &middot; 1/16 &middot; 1/32<br>default 1/16", "<p>The note value of one repeat. 1/6 and 1/12 are triplet values. The repeat length is calculated from the BPM: at 120 BPM a 1/16 repeat is 125 ms, at 128 BPM it is 117 ms.</p>", "Automate nothing - render two clips (1/16 and 1/32) and cut between them in the arrangement. It is faster and always in time.")}
{ctl("SLICE", "read-only", "<p>The length of one repeat in milliseconds. It changes when you change GRID or BPM. If the selected slice of audio is shorter than this, the repeat simply contains silence after the hit.</p>")}
{ctl("BARS", "0.5 to 8 bars, half-bar steps<br>default 2", "<p>The length of the rendered clip. Repeats are placed until the clip is full; if the clip length is not an exact multiple of the grid, the last repeat is cut to fit.</p>")}
{ctl("BPM", "20 to 300<br>default 120<br>DAW or MANUAL", "<p>When the plugin runs inside a DAW that reports its tempo, the box is locked and shows the host tempo (chip reads <em>DAW</em>). Otherwise type a tempo; the chip reads <em>MANUAL</em>. The grid, the SLICE readout and the clip length all follow this value.</p>", "Rendering at the song's tempo is what makes the clip land on the grid when you drag it in. If you later change the song tempo, render again.")}
<h3>Shape</h3>
{img("stutter/tour_knobs.png", "<b>FADE IN, FADE OUT, STEREO.</b>", "w60")}
{ctl("FADE IN", "0 to 100 % of a repeat<br>default 0 %<br>curve: quadratic", "<p>A fade at the start of every repeat. Small values (5-15 %) remove clicks when the slice does not start at a zero crossing; large values turn hard hits into soft pulses.</p>")}
{ctl("FADE OUT", "0 to 100 % of a repeat<br>default 0 %<br>curve: quadratic", "<p>A fade at the end of every repeat. Use it to make each repeat decay before the next one, which gives the classic 'gated' roll.</p>", "FADE OUT around 60-80 % with 1/32 GRID sounds like a machine-gun roll; 0 % sounds like a hard loop.")}
{ctl("STEREO", "0 to 100 %<br>default 0 %", "<p>Ping-pong: even repeats are attenuated in the right channel and odd repeats in the left, by the amount you set. 100 % alternates fully left / right; 30 % gives gentle movement that still sums to mono without holes.</p>", "Check the mix in mono when you push STEREO high - full ping-pong at 1/32 turns into a buzz in mono.")}
<h3>Transport and output</h3>
{img("stutter/tour_transport.png", "<b>ORIGINAL, STUTTER, RESULT and EXPORT.</b>", "w80")}
{ctl("&#9654; ORIGINAL", "", "<p>Plays the loaded file from the selected slice start. Press again to stop.</p>")}
{ctl("STUTTER", "render", "<p>Renders the clip with the current settings. Rendering is instant for normal clip lengths. Every change to GRID, BARS, fades or STEREO needs a new render - the status line reminds you.</p>")}
{ctl("&#9654; RESULT", "", "<p>Plays the rendered clip through the plugin's channel. The playback position is drawn on the waveform.</p>")}
{ctl("EXPORT", "save WAV", "<p>Saves the rendered clip as a WAV file at the source sample rate. The suggested file name contains the source name, the grid and the bar count.</p>")}
{ctl("DRAG TO EXPORT", "strip under the interface", "<p>Drag from the strip directly into your DAW. The clip is written to a temporary file and handed to the DAW as an audio drop; most DAWs copy it into the project folder.</p>", "Drop it at the bar line where the fill should start. Because the clip is exactly N bars long, its end lands on the next bar line.")}
<h3>Host parameters without a knob</h3>
{table(["Parameter", "Range", "What it does"], [
 ("Fade In Curve / Fade Out Curve", "0.1 to 4 (default 2)", "Shape of the fades. 1 is linear, 2 (default) is a gentle quadratic, higher values are sharper."),
 ("Global Fade In / Global Fade Out", "0 to 100 % of the clip", "A fade over the whole rendered clip, on top of the per-repeat fades - a two-bar swell from silence is Global Fade In at 100 %."),
 ("Mix", "0 to 100 %", "Level of the plugin's playback in the channel (audition level)."),
])}
<p>These are visible in your DAW's parameter list and can be automated or set from a controller; they are saved with the project like everything else.</p>
"""})

    S.append({"title": "Step-by-step workflows", "sub": "recipes you will use every week", "html": f"""
{recipe("Snare roll into a drop", "Fills, transitions, any genre",
 steps([
  "Bounce or drag the snare hit (or the whole loop) into Stutter.",
  "Select the snare transient with NEXT / PREV.",
  "GRID 1/16, BARS 1, FADE OUT 50 %. Press STUTTER and listen.",
  "Render a second clip with GRID 1/32 and BARS 0.5.",
  "Drag the 1/16 clip to the bar before the drop and the 1/32 clip onto its last two beats. Add a short reverse cymbal on top and you are done.",
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
<li>Name your exports; the suggested name already contains the grid and bar count.</li>
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
        ("Nothing happens when I press STUTTER", "A slice must be selected first: click the waveform or use NEXT. The STUTTER button is disabled while there is no selection."),
        ("The roll is out of time in my DAW", "Check the BPM chip. If it says MANUAL inside a DAW, the host is not sending tempo (some hosts only do so while playing); type the project tempo and render again."),
        ("Drag to export does nothing", "Render first - the strip only becomes active after a result exists. Drag from the strip itself, not from the waveform."),
        ("No transients are found", "The material is too soft or sustained for the detector. Click the waveform to place the selection by hand; everything else works the same."),
    ]))

    m["sections"] = S
    return m
