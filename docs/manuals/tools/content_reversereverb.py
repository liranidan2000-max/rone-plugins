def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    m = {
        "id": "reversereverb", "product": "RONE Reverse Reverb", "eyebrow": "TEMPO-SYNCED REVERSE REVERB",
        "title_html": "RONE Reverse <i>Reverb</i>", "accent": "#2BD9FF", "version": "1.0",
        "tagline": "Drop in a sound, choose how many bars the swell should take, and RONE Reverse Reverb renders a reversed reverb tail that lands exactly on the beat - shaped with fades, filters, stereo width and a rhythmic tremolo.",
        "formats": ["VST3", "AU", "Standalone"], "vst3": "RONE Reverse Reverb.vst3", "au": "RONE Reverse Reverb.component", "exe": "RONE Reverse Reverb.exe",
        "pdf": "RONE Reverse Reverb - User Manual.pdf", "cover_img": "reversereverb/tour_knobs.png",
        "latency": "None (offline render, playback only)",
    }
    S = []

    S.append({"title": "Welcome to RONE Reverse Reverb", "sub": "the swell before the hit", "html": f"""
<p>A reverse reverb is the sound of a reverb tail played backwards: it grows out of silence and arrives exactly at the note it belongs to. Producers love it for vocal entries, risers, snare swells and transitions - and hate building it by hand, because it means bouncing, reversing, adding reverb, reversing again and nudging the result until it lands on the beat.</p>
<p><strong>RONE Reverse Reverb</strong> does all of that in one step. Load a sound, choose a <strong>TAIL LENGTH</strong> in bars or note values, and the plugin renders a swell that is exactly that long at your song's tempo. Play it from the interface or with a MIDI note, drag the fade handles on the waveform to shape it, add a tempo-synced tremolo for a rhythmic riser, and export.</p>
<div class="two">
<div>
<h3>Where it shines</h3>
<ul>
<li><strong>Vocal entries</strong> - a one-bar swell of the first word before the vocal starts.</li>
<li><strong>Snare and clap swells</strong> - 1/4 or 1/2 tails into every second bar.</li>
<li><strong>Risers</strong> - 4- or 8-bar tails with the TREMOLO in RAMP mode.</li>
<li><strong>Transitions</strong> - TRANSITION mode glues the swell to the original hit with a shared reverb.</li>
</ul>
</div>
<div>
<h3>The 10-second version</h3>
{steps([
 "Drop a short sound (up to 8 seconds) on the waveform.",
 "Set TAIL LENGTH to the number of bars the swell should take.",
 "Press PLAY or send a MIDI note.",
 "Drag the result to your DAW so it ends on the downbeat."])}
</div></div>
{note("How the render works", "The sample is fed into a reverb whose tail is stretched to the chosen length; the whole result is reversed, filtered (LOW CUT), widened (WIDTH), faded with the handles on the waveform and, if enabled, chopped by the tremolo. Everything is rendered offline at your tempo, so it is sample-accurate whatever your buffer size. The plugin re-renders automatically after every change.")}
"""})

    S.append(G["install_section"](m))

    S.append({"title": "Quick start", "sub": "first swell in one minute", "html": f"""
<div class="two">
{img("reversereverb/empty.png", "<b>Fresh instance.</b> Drag a file onto the waveform area to begin.")}
{img("reversereverb/tour.png", "<b>Sample loaded.</b> The waveform shows the rendered (reversed) result; the status line shows the file name and the tempo in use.")}
</div>
{steps([
 "<strong>Insert</strong> the plugin on a channel (an empty channel is ideal, since the plugin plays its result through it). In the standalone app just open it.",
 "<strong>Load a sound.</strong> Drag a WAV, AIFF, FLAC or MP3 of up to 8 seconds onto the waveform. Short, distinct sounds - a word, a snare, a chord stab - give the clearest swells.",
 "<strong>Choose the length.</strong> TAIL LENGTH runs from 1/32 to 8 BAR. It is the total length of the result at the current tempo (host tempo in a DAW; the BPM slider in the standalone app).",
 "<strong>Set the room.</strong> ROOM SIZE controls how dense and long the reverb is inside that length; LOW CUT removes rumble from the tail; WIDTH sets how wide the swell is.",
 "<strong>Listen.</strong> Press <span class='pill'>PLAY</span>. In a DAW you can also trigger playback with any MIDI note routed to the plugin - note on starts, note off stops.",
 "<strong>Shape it.</strong> Drag the fade handles on the left and right edges of the waveform to fade the result in and out, and drag the small curve dots to bend the fades.",
 "<strong>Choose the mode.</strong> REVERSE ONLY renders the reversed tail alone. TRANSITION renders the reversed tail followed by the original sound with a matching forward reverb - the complete 'swell into the hit'.",
 "<strong>Add rhythm (optional).</strong> Turn the TREMOLO on. In RAMP mode it chops the swell faster and faster towards the end - the classic riser.",
 "<strong>Export.</strong> Drag the result from the plugin into your DAW, or use your DAW's render if you prefer to record the playback.",
])}
"""})

    S.append({"title": "Interface tour", "sub": "every element on screen", "html": f"""
{img("reversereverb/tour.png", "RONE Reverse Reverb with a vocal phrase loaded.")}
{legend([
 ("Header logo", "click to flip to the back panel (About, version, licence)"),
 ("Waveform", "the rendered result, with fade handles at both ends and curve dots for the fade shapes"),
 ("Status / file name", "what is loaded and the tempo in use (BPM shown on the right)"),
 ("PLAY", "play the result through the plugin's channel; press again to stop"),
 ("Mode", "REVERSE ONLY or TRANSITION"),
 ("LOW CUT", "high-pass filter on the result"),
 ("WIDTH", "stereo width of the swell"),
 ("TAIL LENGTH", "total length of the result in bars or note values"),
 ("ROOM SIZE", "size of the reverb"),
 ("DRY/WET", "balance between the reversed original and the reversed reverb"),
 ("TREMOLO on/off", "opens the tremolo section"),
 ("Resize grip", "drag to resize the window"),
])}
{img("reversereverb/tremolo.png", "<b>TREMOLO section open.</b> 1 DEPTH &middot; 2 PAN &middot; 3 RATE &middot; 4 SHAPE &middot; 5 MODE (RAMP / SYNC / FREE) &middot; 6 FROM / TO divisions for RAMP (a single Division list appears in SYNC mode).")}
"""})

    S.append({"title": "Controls reference", "sub": "what every control does, its range and default", "html": f"""
<h3>Source and playback</h3>
{ctl("Waveform", "drop zone &middot; fade handles &middot; curve dots", "<p>Drop files here. Once a sound is loaded it shows the reversed result. Drag the handle at the left edge to set FADE IN and the handle at the right edge to set FADE OUT; drag the small dots to change the fade curve from logarithmic to exponential. The playback cursor moves across the waveform while playing.</p>", "A short fade in (5-10 %) hides the abrupt start that some reversed tails have; a fade out that ends a little before the end keeps the last transient clean.")}
{ctl("PLAY", "", "<p>Plays the rendered result. In a DAW, MIDI note on also triggers playback and note off stops it, so you can place a MIDI note where the swell should start.</p>")}
{ctl("REVERSE ONLY / TRANSITION", "default REVERSE ONLY", "<p><strong>REVERSE ONLY</strong>: the output is the reversed reverb tail; the original sound is not included. <strong>TRANSITION</strong>: the reversed tail is followed by the original sound, itself sent through the same reverb forwards - swell, hit and decay in one clip, already glued.</p>")}
<h3>Main knobs</h3>
{img("reversereverb/tour_knobs.png", "<b>LOW CUT, WIDTH, TAIL LENGTH, ROOM SIZE, DRY/WET.</b>")}
{ctl("TAIL LENGTH", "8 BAR &middot; 4 BAR &middot; 2 BAR &middot; 1 BAR &middot; 1/2 &middot; 1/4 &middot; 1/8 &middot; 1/16 &middot; 1/32<br>default 1/2", "<p>The total length of the rendered result at the current tempo. This is what makes the swell land: an 8 BAR tail dropped eight bars before the downbeat arrives exactly on it.</p>", "Use short values (1/4, 1/8) on drums for tight pre-hits; long values (2-8 BAR) on vocals and chords for cinematic entries.")}
{ctl("ROOM SIZE", "0 to 100 %<br>default 80 %", "<p>The size of the reverb used to create the tail. Larger rooms give denser, smoother swells; small rooms give short, grainy pre-echoes that still stretch to the chosen length.</p>")}
{ctl("LOW CUT", "20 to 500 Hz<br>default 20 Hz", "<p>A high-pass filter on the result. Reversed tails carry a lot of low-mid rumble; 100-200 Hz keeps the swell out of the way of bass and kick.</p>")}
{ctl("WIDTH", "0 to 100 %<br>default 50 %", "<p>Stereo width of the result. 0 % is mono, 50 % is the natural reverb width, 100 % is as wide as it goes.</p>", "Check the mix in mono above 80 %.")}
{ctl("DRY/WET", "0 to 100 %<br>default 50 % on the dial (fully wet)", "<p>Balance between the reversed original sound and the reversed reverb tail. At the default the output is the reverb tail; turning down blends in the reversed dry sound for a more defined, 'sucked-in' character.</p>")}
<h3>Tremolo</h3>
{img("reversereverb/tremolo_panel.png", "<b>The tremolo section.</b>")}
{ctl("TREMOLO on/off", "power button", "<p>Enables a rhythmic amplitude gate on the result - the difference between a swell and a riser.</p>")}
{ctl("DEPTH", "0 to 100 %<br>default 50 %", "<p>How deep the tremolo cuts. 100 % is a hard gate.</p>")}
{ctl("PAN", "0 to 100 %<br>default 0 %", "<p>Turns the tremolo into an auto-pan: alternate pulses lean left and right by this amount.</p>")}
{ctl("RATE", "0.1 to 20 Hz<br>default 4 Hz", "<p>Speed of the tremolo in FREE mode. In RAMP and SYNC modes the speed comes from the tempo and this knob is inactive.</p>")}
{ctl("SHAPE", "sine &middot; triangle &middot; square<br>default sine", "<p>The shape of the pulse. Sine is smooth, triangle is sharper, square is a hard gate.</p>")}
{ctl("MODE", "RAMP &middot; SYNC &middot; FREE<br>default RAMP", "<p><strong>RAMP</strong> starts at the FROM division and accelerates to the TO division across the length of the tail - the accelerating riser gate. <strong>SYNC</strong> holds one tempo-synced division for the whole tail. <strong>FREE</strong> runs at the RATE in Hz.</p>", "RAMP from 1/4 to 1/32 over an 8 BAR tail is the riser everybody knows.")}
{ctl("FROM / TO (RAMP) &middot; Division (SYNC)", "8 Bar to 1/128<br>defaults 1/4 &rarr; 1/32", "<p>The tempo divisions the tremolo runs at. In RAMP mode FROM is the starting speed and TO the speed reached at the end of the tail.</p>")}
<h3>Standalone extras</h3>
{ctl("BPM", "20 to 300<br>default 120<br>standalone only", "<p>A slider row appears under the knobs in the standalone app, because there is no host tempo to read. Set it to your song's tempo before exporting.</p>")}
<h3>Presets</h3>
<p>Twelve factory presets are built in and appear as programs in hosts that show plugin programs: Default, Dreamy Pad, Short Impact, Cinematic Swell, Dark Atmosphere, Bright Shimmer, Transition Rise, Tremolo Wash, Pulsing Reverse, Lo-Fi Reverse, 50/50 Mix and Wide Hall. Your own presets can be saved from the DAW's preset system; the plugin state is saved with the project in any case.</p>
"""})

    S.append({"title": "Step-by-step workflows", "sub": "recipes", "html": f"""
{recipe("Vocal entry swell", "Any vocal, any genre",
 steps([
  "Bounce the first word of the vocal (or the first second of the phrase) and drop it on the plugin.",
  "TAIL LENGTH 1 BAR, ROOM SIZE 85 %, LOW CUT 150 Hz, WIDTH 60 %. Mode REVERSE ONLY.",
  "Fade out the last 10 % with the right handle so the swell does not step on the real vocal.",
  "Place the result so it ends exactly where the vocal starts. Send it to the same reverb as the vocal.",
 ]))}
{recipe("Snare swell every second bar", "Techno, house, pop drums",
 steps([
  "Drop the snare sample. TAIL LENGTH 1/2, ROOM SIZE 60 %, LOW CUT 200 Hz.",
  "Trigger it with a MIDI note half a bar before every second snare, or drag the result into the arrangement and repeat it.",
 ]))}
{recipe("8-bar riser with tremolo", "Builds",
 steps([
  "Drop a chord stab or a held vocal note. TAIL LENGTH 8 BAR, ROOM SIZE 100 %, WIDTH 80 %.",
  "TREMOLO on, DEPTH 80 %, SHAPE square, MODE RAMP, FROM 1/4, TO 1/32, PAN 40 %.",
  "Fade in the first 30 % with the left handle. Place the result so it ends on the drop.",
 ]))}
{recipe("Transition hit", "Section changes, FX",
 steps([
  "Drop a crash, a vocal shout or an impact. Mode TRANSITION.",
  "TAIL LENGTH 2 BAR, ROOM SIZE 90 %. The result is the swell, the hit and a forward tail in one clip.",
  "Align the point where the swell ends with the downbeat of the new section.",
 ]))}
"""})

    S.append({"title": "Recommendations", "sub": "how to get the best out of it", "html": f"""
<div class="dos">
<div><h4>Do</h4><ul>
<li>Feed it short, clean sounds. The plugin stretches the reverb to the chosen length, so the source only needs to define the character.</li>
<li>Use LOW CUT generously - a reversed tail without low end sits behind the mix instead of on top of it.</li>
<li>Trigger with MIDI when you want the swell to follow arrangement edits; drag the file when you want to edit it as audio.</li>
<li>Re-render after changing the song tempo; the length is baked in at render time.</li>
<li>Try TRANSITION mode on drum hits; the shared reverb makes the hit sound like it belongs to the swell.</li>
</ul></div>
<div><h4>Avoid</h4><ul>
<li>Files longer than 8 seconds; the plugin refuses them. Trim in your DAW first.</li>
<li>WIDTH at 100 % on low-frequency material; it can cancel in mono.</li>
<li>Deep square tremolo on soft pads; sine or triangle keeps the swell musical.</li>
</ul></div>
</div>
<h3>Sound tips</h3>
<ul>
<li><strong>Room size is texture, length is timing.</strong> A small room stretched to 4 bars sounds like a grainy pre-echo; a large one like a smooth cloud. The timing is identical.</li>
<li><strong>Two swells</strong> - one 1 BAR, one 1/4 - layered into the same downbeat give both build and snap.</li>
<li><strong>PAN at 100 % with a fast TO value</strong> spins the end of a riser around the listener.</li>
</ul>
"""})

    S.append(G["conventions_section"](m, note("MIDI", "Route a MIDI track to the plugin to trigger playback: note on plays the result from the start, note off stops it. This works in the VST3 and AU versions.")))

    S.append(G["support_section"](m, [
        ("The file does not load", "Files longer than 8 seconds are rejected. Trim the sound in your DAW and try again. WAV, AIFF, FLAC and MP3 are supported."),
        ("The swell does not land on the beat", "Check the tempo shown next to the status line. In the standalone app set the BPM slider; in a DAW the host tempo is used - re-render if you changed it."),
        ("PLAY is greyed out", "No sound is loaded yet, or the render is still running (very long tails take a moment)."),
        ("I hear nothing when I send MIDI", "The MIDI must reach the plugin's channel; in some DAWs an effect plugin needs a MIDI routing or a 'MIDI to plugin' setting. The PLAY button works regardless."),
    ]))

    m["sections"] = S
    return m
