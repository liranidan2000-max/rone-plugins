def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    m = {
        "id": "stucker", "product": "RONE Stucker", "eyebrow": "ONE-KNOB ROLL-UP RISER",
        "title_html": "RONE <i>Stucker</i>", "accent": "#9D6BFF", "version": "1.1",
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
<li><strong>Build-ups</strong> - the last bar before a drop: set RIDE to 1 bar and press GO; the knob climbs by itself and releases exactly on the downbeat.</li>
<li><strong>In key</strong> - choose the song's KEY and the scream at the top lands on the root, stepping through the scale on the way.</li>
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
 ("RIDE", "OFF / 1 BAR / 2 / 4 / 8: how many bars the knob takes to ride itself to 100 %"),
 ("GO", "arms the ride - it starts on the next bar and releases on the downbeat (MIDI C2)"),
 ("STUCK", "the one knob: OFF = live signal; higher = shorter loop, higher pitch"),
 ("LOOP", "current loop length in milliseconds while engaged"),
 ("Roll-up LEDs", "how far into the shrink you are"),
 ("LANDS", "with a KEY chosen: the note the scream lands on at the top of the knob"),
 ("ADVANCED", "opens the panel with SMOOTH, RANGE, LOW CUT, LENGTH, REV, KEY, SCALE, ON THE DROP and CURVE"),
 ("MIX", "100 % = LOCK, the loop replaces the source; 99 % and below = the source at full level with the effect added at MIX"),
 ("Resize grip", "drag to resize the window"),
])}
<p>In the ADVANCED panel: <span class="b" style="display:inline-flex;align-items:center;justify-content:center;width:5mm;height:5mm;border-radius:50%;background:#9D6BFF;color:#101216;font-weight:800;font-size:8pt">1</span> SMOOTH &nbsp; <span class="b" style="display:inline-flex;align-items:center;justify-content:center;width:5mm;height:5mm;border-radius:50%;background:#9D6BFF;color:#101216;font-weight:800;font-size:8pt">2</span> RANGE &nbsp; <span class="b" style="display:inline-flex;align-items:center;justify-content:center;width:5mm;height:5mm;border-radius:50%;background:#9D6BFF;color:#101216;font-weight:800;font-size:8pt">3</span> LOW CUT &nbsp; <span class="b" style="display:inline-flex;align-items:center;justify-content:center;width:5mm;height:5mm;border-radius:50%;background:#9D6BFF;color:#101216;font-weight:800;font-size:8pt">4</span> LENGTH.</p>
"""})

    S.append({"title": "Controls reference", "sub": "what every control does, its range and default", "html": f"""
{img("stucker/tour_knob.png", "<b>STUCK.</b> The white dot shows the position, the violet arc shows how far the roll-up has gone.", "w60")}
{ctl("STUCK", "0 to 100 %<br>default OFF (0 %)<br>automatable, smoothed", "<p>The amount of roll-up. At 0 % the effect is off and the input passes through. The moment the knob leaves zero the last grid slice is frozen and looped. As the value increases the loop length shrinks exponentially over the number of octaves set by RANGE; at the top the loop is only a few dozen samples long and reads as a rising pitch.</p><p>The knob is heavily smoothed, so fast automation and MIDI controllers never click.</p>", "Double-click the knob to snap back to OFF - the fastest way to release on the downbeat when performing by hand.")}
{ctl("SYNC / FREE", "default SYNC", "<p><strong>SYNC</strong> aligns the captured slice to the host's beat grid: the loop starts on the last grid line of the LENGTH division, so a slightly late grab still loops a full beat in time. <strong>FREE</strong> captures the most recent slice from the exact moment you engage, which is what you want on material without a fixed tempo, or in the standalone app.</p>")}
{ctl("BYPASS", "", "<p>Hard bypass. Use it to compare with and without the effect while STUCK is engaged.</p>")}
<h3>Ride</h3>
{img("stucker/ride.png", "<b>A ride in progress.</b> RIDE set to 2 bars, GO reads RIDING, the knob climbs by itself and LANDS shows the note it is heading for.", "w60")}
{ctl("RIDE and GO", "OFF &middot; 1 BAR &middot; 2 &middot; 4 &middot; 8<br>default OFF<br>GO: momentary, automatable, MIDI C2", "<p>The knob rides itself, on the bar. Pick a length and press <strong>GO</strong>: the button reads ARMED, and on the <em>next bar</em> the STUCK starts climbing from wherever the knob is to 100 %, over that many bars, landing on the downbeat - where it snaps straight back to the knob. With the knob at OFF that is the release, exactly on the drop, with whatever ON THE DROP says. The big knob follows on screen.</p><p>It is the snare roll-up automation of every build, without drawing it, and it cannot be late. The bar comes from the host's song position; in the standalone app, or while the transport is stopped, the clock free-runs at the tempo. Setting RIDE to OFF cancels a ride in progress.</p>", "GO is also a parameter (Ride Go): automate one step at the top of the last phrase, or send MIDI C2 from a pad. The CURVE row in ADVANCED sets the shape.")}
{ctl("MIX", "0 to 100 %<br>default 100 %<br>automatable, smoothed", "<p>Two behaviours in one knob, in the bottom right corner. At 100 % the readout says <strong>LOCK</strong> and the knob turns white: the frozen loop replaces the source, exactly as the plugin has always sounded. The moment you leave 100 % the knob is purple again and the readout is a percentage: the source comes straight back to full level and the loop sits on top of it at the MIX level - a send, not a crossfade, so the original never gets quieter. At OFF the control does nothing at all - the effect only exists while STUCK is up.</p><p>The wet path keeps running underneath, so you can take MIX to zero in the middle of a roll and bring it back without a click or a jump in the loop.</p>", "Automate MIX down over the second half of a long build: the roll keeps rising but steps out of the way of the vocal. Double-click the knob to snap back to 100 %.")}
<h3>Advanced panel</h3>
{img("stucker/adv_panel.png", "<b>SMOOTH, RANGE, LOW CUT and LENGTH.</b>", "w80")}
{ctl("SMOOTH", "0 to 100 %<br>default 35 %", "<p>How gently the loop length follows the knob and how long the engage / release crossfades are. Low values are snappy and can sound grainy when the knob moves fast; high values glide between loop lengths and hide every seam.</p>", "35 % is the sweet spot for drums; go to 60-70 % on sustained material like pads and vocals.")}
{ctl("RANGE", "3 to 10 octaves<br>default 8", "<p>How far the loop shrinks across the knob's travel. At 3 octaves the full turn only takes a one-beat loop down to 1/8 of its length - a rhythmic roll with no pitch rise. At 10 octaves it dives all the way to audio rate and the top of the knob screams.</p>", "Lower RANGE when you want a musical, rhythmic roll; raise it for the classic 'rising scream' build.")}
{ctl("LOW CUT", "20 to 500 Hz<br>default 20 Hz", "<p>A high-pass filter on the looped (wet) signal only. When the loop reaches audio rate a lot of low-end buzz appears; a LOW CUT around 120-200 Hz keeps the build clean under a kick that is still playing.</p>")}
{ctl("KEY and SCALE", "KEY: OFF, C .. B<br>SCALE: Phrygian, Phrygian dominant, harmonic minor, minor, major, root + 5th<br>default OFF / Phrygian", "<p>A scream at an arbitrary pitch fights the bass; on the root it is part of the music. Choose the song's root and the top of the knob lands exactly on it - in the octave nearest to where it would have landed anyway at this tempo and RANGE - and the LANDS chip on the main view shows the note. On the way up, once the loop is at audio rate, the pitch steps through the chosen scale instead of sliding, each step glided by SMOOTH, so the roll-up becomes a run up the scale that resolves on the root on the downbeat.</p><p>Phrygian and Phrygian dominant are the psytrance scales; ROOT + 5TH is the power-chord version - only two notes, the biggest jumps.</p>", "Psytrance in E: KEY E, SCALE PHRYG, RANGE 8. The scream resolves on E on the drop, over an E bass.")}
{ctl("ON THE DROP", "CUT &middot; TAIL &middot; REVERSE<br>default CUT", "<p>What happens at the release - when STUCK returns to OFF, or the ride lands. <strong>CUT</strong> is the classic: the loop stops and the source is back, click-free. <strong>TAIL</strong> lets the final loop ring on at its last pitch and fade over one beat on top of the returning source. <strong>REVERSE</strong> plays the captured slice backwards once, at its original length, fading out - the downlifter that the good transition guides ask for after the impact, without a second plugin.</p>", "REVERSE with LENGTH 1/4 on a drum bus is a reversed snare across the first beat of the drop.")}
{ctl("REV", "default off", "<p>The slice is read backwards while it shrinks. Same buffer, opposite direction: a psychedelic texture that is cheap to switch on and off mid-build.</p>")}
{ctl("CURVE", "LIN &middot; EXP &middot; S<br>default LIN", "<p>The shape of the ride. LIN is the classic roll-up: the loop halves at even intervals, so the rate doubles like a snare roll (the shrink itself is already exponential). EXP holds back for the first half and does everything at the end; S is EXP with an extra push before it lands.</p>")}
{ctl("LENGTH", "1/1 &middot; 1/2 &middot; 1/4 &middot; 1/8 of a bar<br>default 1/4", "<p>The size of the slice that is captured when STUCK engages, and the grid it locks to in SYNC mode. 1/4 grabs one beat - a kick or a snare hit; 1/1 grabs a whole bar and rolls the phrase; 1/8 starts from an already tight loop.</p>", "The GRID readout in the header always shows this value.")}
<h3>Readouts</h3>
{ctl("LOOP", "ms", "<p>The current loop length while engaged. It falls as you turn the knob up. Below roughly 50 ms the repeats are faster than 20 per second and start to sound like a tone.</p>")}
{ctl("LEDs", "", "<p>Twelve LEDs show the roll-up depth; when they are all lit you are at the audio-rate end of the range.</p>")}
"""})

    S.append({"title": "Step-by-step workflows", "sub": "recipes", "html": f"""
{recipe("The one-bar build", "Any drop, any genre",
 steps([
  "Insert Stucker on the drum bus. SYNC on, LENGTH 1/4, RANGE 8, SMOOTH 35 %.",
  "Set RIDE to 1 BAR. Two bars before the drop, press GO (or send MIDI C2, or automate one step of Ride Go).",
  "On the next bar the knob climbs by itself and releases exactly on the downbeat of the drop. Nothing to draw.",
  "Optional: LOW CUT at 200 Hz so the roll-up stays out of the kick's way; ON THE DROP = REVERSE for a downlifter across the first beat.",
 ]))}
{recipe("In key", "Psytrance, any melodic drop",
 steps([
  "Open ADVANCED. KEY = the song's root, SCALE = PHRYG (or PHRYG DOM for the darker flavour).",
  "The LANDS chip on the main view shows the note the scream will resolve on. If it is an octave too high or low, change RANGE by one.",
  "Ride or automate STUCK to 100 %: the run up steps through the scale and lands on the root on the downbeat.",
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
<li>Use RIDE for every build. It lands on the bar; a hand on the knob usually does not.</li>
<li>Set KEY whenever the drop has a bass note. A scream that resolves on the root is a musical event; one that does not is noise.</li>
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
        ("GO is greyed out", "RIDE is OFF. Pick a length (1, 2, 4 or 8 bars) first."),
        ("The ride does not start", "It starts on the next bar of the host's song position, so the transport has to be rolling - or, in the standalone, the free-running clock has to reach the next bar. GO reads ARMED while it waits."),
        ("LANDS shows a note but the scream lands elsewhere", "The landing note is exact at the top of the knob; if your automation stops at 90 % the run has not finished. Ride, or automate all the way to 100 %."),
        ("MIDI C2 does nothing", "Route a MIDI track (or a pad) to the plugin; C2 (note 48) arms the ride, with a RIDE length set."),
    ]))

    m["sections"] = S
    return m
