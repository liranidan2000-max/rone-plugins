def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    m = {
        "id": "stucker", "product": "RONE Stucker", "eyebrow": "ONE-KNOB ROLL-UP RISER",
        "title_html": "RONE <i>Stucker</i>", "accent": "#9D6BFF", "version": "1.0",
        "tagline": "One knob grabs the loop and rolls it up: the captured slice shrinks tighter and tighter until it screams, then lets go right back into the beat.",
        "formats": ["VST3", "AU", "Standalone"], "vst3": "RONE Stucker.vst3", "au": "RONE Stucker.component", "exe": "RONE Stucker.exe",
        "pdf": "RONE Stucker - User Manual.pdf", "cover_img": "stucker/tour_knob.png",
    }
    S = []

    S.append({"title": "Welcome to RONE Stucker", "sub": "one knob, one job", "html": f"""
<p><strong>RONE Stucker</strong> is a live buffer-retrigger effect built around a single gesture: turn the big knob up and whatever was just playing gets <em>stuck</em>. The captured slice loops in place of the live signal, and as you keep turning, the loop gets shorter and shorter. Past a certain point the repeats are so fast they become a pitch, and that pitch climbs. Turn the knob back to zero and the live signal returns exactly where it should be.</p>
<p>Producers call this a "stutter riser", "roll-up" or "buffer freeze build". Stucker makes it a one-knob move so you can perform it, automate it with a single line, or map it to a controller.</p>
<div class="two">
<div>
<h3>Where it shines</h3>
<ul>
<li><strong>Build-ups</strong> - the last bar before a drop: automate STUCK from 0 to 100 % and cut to 0 on the downbeat.</li>
<li><strong>Live performance</strong> - map STUCK to a knob or a mod wheel and play the effect by hand.</li>
<li><strong>Glitch accents</strong> - short automation blips at 30-50 % give tight, in-time stutters without any editing.</li>
<li><strong>Sound design</strong> - park the knob high and the captured slice becomes a tempo-derived drone.</li>
</ul>
</div>
<div>
<h3>The 10-second version</h3>
{steps([
 "Insert Stucker on a drum bus, a loop, or the master of a stem.",
 "Play the track.",
 "Turn STUCK up - the beat freezes and rolls up.",
 "Turn it back to OFF on the next downbeat."])}
</div></div>
{note("What is being captured", "The input is recorded continuously into a circular buffer. When STUCK leaves zero, the most recent slice of one grid unit (LENGTH) is frozen. In SYNC mode that slice is aligned to the host's grid, so the loop always starts on a beat even if you grabbed the knob a little late. Returning to OFF releases the buffer with a short crossfade (SMOOTH) - no clicks, no gap.")}
"""})

    S.append(G["install_section"](m))

    S.append({"title": "Quick start", "sub": "your first roll-up", "html": f"""
{img("stucker/tour.png", "<b>RONE Stucker.</b> The big knob is the whole story; everything else supports it.", "w60")}
{steps([
 "<strong>Insert</strong> Stucker on the channel you want to roll up - a drum loop is the classic choice, but it works on anything rhythmic, including a full mix bus.",
 "<strong>Leave SYNC on</strong> (default) so the captured slice locks to the beat, and leave GRID at 1/4: one beat of audio is captured.",
 "<strong>Play</strong> the track and turn <span class='pill'>STUCK</span> slowly clockwise. The value display switches from OFF to a percentage, the LOOP chip shows the current loop length in milliseconds, and the LEDs light up as the loop shrinks.",
 "<strong>Keep turning.</strong> Around 70-80 % the loop is a few milliseconds long and the repeats become a rising tone.",
 "<strong>Release</strong> by turning back to OFF (double-click the knob to snap there). The live signal comes back on the beat.",
 "<strong>Automate it.</strong> Draw an automation ramp from 0 to 100 % over the last bar of a build, then a vertical drop back to 0 exactly on the downbeat.",
])}
{note("Bypass vs. OFF", "At OFF the plugin passes audio through untouched - you do not need BYPASS in normal use. BYPASS is there for A/B checks while the knob is up.")}
"""})

    S.append({"title": "Interface tour", "sub": "every element on screen", "html": f"""
<div class="two">
{img("stucker/tour.png", "Main view.")}
{img("stucker/adv.png", "ADVANCED panel open.")}
</div>
{legend([
 ("Header logo", "click to flip to the back panel (About, version, licence)"),
 ("BYPASS", "hard bypass for A/B comparison"),
 ("SYNC / FREE", "lock the captured slice to the host grid, or capture freely from the moment you engage"),
 ("GRID", "shows the LENGTH setting: the size of the captured slice in bar fractions"),
 ("STUCK", "the one knob: OFF = live signal; higher = shorter loop, higher pitch"),
 ("LOOP", "current loop length in milliseconds while engaged"),
 ("Roll-up LEDs", "how far into the shrink you are"),
 ("ADVANCED", "opens the panel with SMOOTH, RANGE, LOW CUT and LENGTH"),
 ("Resize grip", "drag to resize the window"),
])}
<p>In the ADVANCED panel: <span class="b" style="display:inline-flex;align-items:center;justify-content:center;width:5mm;height:5mm;border-radius:50%;background:#9D6BFF;color:#101216;font-weight:800;font-size:8pt">1</span> SMOOTH &nbsp; <span class="b" style="display:inline-flex;align-items:center;justify-content:center;width:5mm;height:5mm;border-radius:50%;background:#9D6BFF;color:#101216;font-weight:800;font-size:8pt">2</span> RANGE &nbsp; <span class="b" style="display:inline-flex;align-items:center;justify-content:center;width:5mm;height:5mm;border-radius:50%;background:#9D6BFF;color:#101216;font-weight:800;font-size:8pt">3</span> LOW CUT &nbsp; <span class="b" style="display:inline-flex;align-items:center;justify-content:center;width:5mm;height:5mm;border-radius:50%;background:#9D6BFF;color:#101216;font-weight:800;font-size:8pt">4</span> LENGTH.</p>
"""})

    S.append({"title": "Controls reference", "sub": "what every control does, its range and default", "html": f"""
{img("stucker/tour_knob.png", "<b>STUCK.</b> The white dot shows the position, the violet arc shows how far the roll-up has gone.", "w60")}
{ctl("STUCK", "0 to 100 %<br>default OFF (0 %)<br>automatable, smoothed", "<p>The amount of roll-up. At 0 % the effect is off and the input passes through. The moment the knob leaves zero the last grid slice is frozen and looped. As the value increases the loop length shrinks exponentially over the number of octaves set by RANGE; at the top the loop is only a few dozen samples long and reads as a rising pitch.</p><p>The knob is heavily smoothed, so fast automation and MIDI controllers never click.</p>", "Double-click the knob to snap back to OFF - the fastest way to release on the downbeat when performing by hand.")}
{ctl("SYNC / FREE", "default SYNC", "<p><strong>SYNC</strong> aligns the captured slice to the host's beat grid: the loop starts on the last grid line of the LENGTH division, so a slightly late grab still loops a full beat in time. <strong>FREE</strong> captures the most recent slice from the exact moment you engage, which is what you want on material without a fixed tempo, or in the standalone app.</p>")}
{ctl("BYPASS", "", "<p>Hard bypass. Use it to compare with and without the effect while STUCK is engaged.</p>")}
<h3>Advanced panel</h3>
{img("stucker/adv_panel.png", "<b>SMOOTH, RANGE, LOW CUT and LENGTH.</b>", "w80")}
{ctl("SMOOTH", "0 to 100 %<br>default 35 %", "<p>How gently the loop length follows the knob and how long the engage / release crossfades are. Low values are snappy and can sound grainy when the knob moves fast; high values glide between loop lengths and hide every seam.</p>", "35 % is the sweet spot for drums; go to 60-70 % on sustained material like pads and vocals.")}
{ctl("RANGE", "3 to 10 octaves<br>default 8", "<p>How far the loop shrinks across the knob's travel. At 3 octaves the full turn only takes a one-beat loop down to 1/8 of its length - a rhythmic roll with no pitch rise. At 10 octaves it dives all the way to audio rate and the top of the knob screams.</p>", "Lower RANGE when you want a musical, rhythmic roll; raise it for the classic 'rising scream' build.")}
{ctl("LOW CUT", "20 to 500 Hz<br>default 20 Hz", "<p>A high-pass filter on the looped (wet) signal only. When the loop reaches audio rate a lot of low-end buzz appears; a LOW CUT around 120-200 Hz keeps the build clean under a kick that is still playing.</p>")}
{ctl("LENGTH", "1/1 &middot; 1/2 &middot; 1/4 &middot; 1/8 of a bar<br>default 1/4", "<p>The size of the slice that is captured when STUCK engages, and the grid it locks to in SYNC mode. 1/4 grabs one beat - a kick or a snare hit; 1/1 grabs a whole bar and rolls the phrase; 1/8 starts from an already tight loop.</p>", "The GRID readout in the header always shows this value.")}
<h3>Readouts</h3>
{ctl("LOOP", "ms", "<p>The current loop length while engaged. It falls as you turn the knob up. Below roughly 50 ms the repeats are faster than 20 per second and start to sound like a tone.</p>")}
{ctl("LEDs", "", "<p>Twelve LEDs show the roll-up depth; when they are all lit you are at the audio-rate end of the range.</p>")}
"""})

    S.append({"title": "Step-by-step workflows", "sub": "recipes", "html": f"""
{recipe("The one-bar build", "Any drop, any genre",
 steps([
  "Insert Stucker on the drum bus. SYNC on, LENGTH 1/4, RANGE 8, SMOOTH 35 %.",
  "In the last bar before the drop, draw STUCK automation rising from 0 to 100 %. Make the curve exponential (slow start, fast end) - it feels more urgent.",
  "Put a vertical drop to 0 % exactly on the downbeat of the drop.",
  "Optional: automate LOW CUT up to 200 Hz across the same bar so the roll-up stays out of the kick's way.",
 ]))}
{recipe("Half-bar stab", "Accents, fills",
 steps([
  "LENGTH 1/8, RANGE 5, SMOOTH 20 %.",
  "Draw a short automation bump: 0 &rarr; 60 % &rarr; 0 over half a bar, ending on a beat.",
  "The result is a tight, in-time stutter - no slicing required.",
 ]))}
{recipe("Drone from a chord", "Pads, transitions, intros",
 steps([
  "Insert on a chord stab or a pad. FREE mode, LENGTH 1/1, SMOOTH 80 %.",
  "Turn STUCK to about 40 % and leave it. The captured bar loops as a slowly shrinking texture.",
  "Send the channel to a big reverb (RONE AFTERSPACE is a good partner) and automate STUCK slowly to change the texture over time.",
 ]))}
{recipe("Performing it live", "Controllers, DJ-style sets",
 steps([
  "Map STUCK to a physical knob or the mod wheel (right-click the knob in VST3 and use your DAW's controller link).",
  "Practice the release: the knob must reach OFF on the downbeat. Double-clicking releases instantly if your controller cannot get there in time.",
 ]))}
"""})

    S.append({"title": "Recommendations", "sub": "how to get the best out of it", "html": f"""
<div class="dos">
<div><h4>Do</h4><ul>
<li>Put Stucker <strong>after</strong> your drum processing and <strong>before</strong> reverb sends; the reverb tail then follows the roll-up naturally.</li>
<li>Use SYNC in the DAW. It forgives late knob moves and keeps loops on the grid.</li>
<li>Match LENGTH to the material: 1/4 for drums, 1/1 for chords and vocals, 1/8 for already busy loops.</li>
<li>Draw the release as a vertical line in the automation lane. The crossfade is handled for you.</li>
<li>Try RANGE 4-5 on melodic material; it produces musical rhythmic subdivisions rather than a scream.</li>
</ul></div>
<div><h4>Avoid</h4><ul>
<li>Leaving STUCK at a low value by accident - even 2 % engages the loop. Double-click to be sure it is OFF.</li>
<li>Very low SMOOTH with fast automation; you will hear zipper-like grain.</li>
<li>Rolling up a channel that also carries the sub bass without LOW CUT; the audio-rate end of the range piles up low-mid energy.</li>
</ul></div>
</div>
<h3>Sound tips</h3>
<ul>
<li>The pitch you reach at the top of the knob is set by the loop length, not by the note in the audio. It therefore rises smoothly through the octaves, which is what makes it feel like a riser.</li>
<li>Two instances on two stems with different RANGE values (say 5 and 9) give a build that thickens in stages.</li>
<li>Automating LENGTH mid-build is legal: switching from 1/4 to 1/8 halves the loop instantly for a "gear change".</li>
</ul>
"""})

    S.append(G["conventions_section"](m, note("Standalone", "Without a DAW clock there is no grid to sync to. Use FREE mode in the standalone app - or feed it a fixed tempo source and switch to SYNC once the host provides a clock.")))

    S.append(G["support_section"](m, [
        ("The loop does not start on the beat", "Switch to SYNC and make sure the DAW is playing (the grid comes from the host transport). In FREE mode the loop starts exactly when you engage."),
        ("I hear a click when releasing", "Raise SMOOTH. Also check that the automation actually reaches 0 % - a value of 1-2 % keeps the loop engaged."),
        ("The top of the knob is a low buzz instead of a scream", "Raise RANGE towards 10 and raise LOW CUT to remove the low-frequency component of the very short loop."),
    ]))

    m["sections"] = S
    return m
