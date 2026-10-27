def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    m = {
        "id": "gater", "product": "RONE Gater", "eyebrow": "MIDI NOTE GATE",
        "title_html": "RONE <i>Gater</i>", "accent": "#8FB000", "version": "1.0",
        "tagline": "Write notes, hear chops. The sound on the track passes only while a note is held - your DAW's piano roll is the sequencer.",
        "formats": ["VST3", "AU", "Standalone"], "vst3": "RONE Gater.vst3", "au": "RONE Gater.component", "exe": "RONE Gater.exe",
        "pdf": "RONE Gater - User Manual.pdf", "cover_img": "gater/tour_knobs.png",
    }
    S = []

    S.append({"title": "Welcome to RONE Gater", "sub": "write notes, hear chops", "html": f"""
<p><strong>RONE Gater</strong> is a MIDI note gate. Put it on a pad, a vocal, a bass or a whole bus, send it notes, and the sound only passes while a note is held: <em>note on opens, note off closes, the note's length is how long it stays open.</em> There is no internal sequencer and no pattern grid - the notes you already know how to write are the pattern, with every tool your piano roll has: copy, quantize, swing, velocity, slide a note off the grid.</p>
<div class="two">
<div>
<h3>The three rules</h3>
<ul>
<li><strong>No note = closed, playing or not.</strong> Insert it and the track goes silent until a note arrives; BYPASS to hear the source while you design it.</li>
<li><strong>A new note in the middle of a release continues from where the envelope is</strong> - never a jump, never a click.</li>
<li><strong>Any key, any channel.</strong> The gate ignores pitch; write the notes on whatever key is comfortable.</li>
</ul>
</div>
<div>
<h3>The 10-second version</h3>
{steps([
 "Insert Gater on the track with the sound.",
 "Send it the notes of a MIDI track (SETUP shows how in your DAW).",
 "Write a 1/16 pattern and press play.",
 "Shape the chop with ATTACK and RELEASE."])}
</div></div>
{note("When the MIDI lamp lights, the routing works", "The lamp in the header lights on every note the gate receives. If the transport plays and no note has ever arrived, the gate is shut, the display says PLAYING · NO NOTES · GATE CLOSED and SETUP pulses.")}
"""})

    S.append(G["install_section"](m))

    S.append({"title": "Quick start", "sub": "your first chop", "html": f"""
{img("gater/tour.png", "<b>RONE Gater.</b> The last two bars on top - input in grey, what passes in chartreuse, the envelope line - and the three knobs below.")}
{steps([
 "<strong>Insert</strong> RONE Gater on the track you want to chop - a held pad is the classic.",
 "<strong>Route notes to it.</strong> Click <span class='pill'>SETUP</span>: the plugin knows which DAW it runs in and shows that host's steps first.",
 "<strong>Write notes</strong> on that MIDI track - a 1/16 pattern with a few gaps is a good start. Pitch does not matter.",
 "<strong>Press play.</strong> The MIDI lamp lights on every note and the display tag reads GATE FOLLOWS THE NOTES.",
 "<strong>Turn <span class='pill'>RELEASE</span></strong> to set the groove: short = tight chops, longer = the gaps fill in.",
 "<strong>Lower <span class='pill'>DEPTH</span></strong> if the gaps should breathe instead of going silent.",
])}
"""})

    S.append({"title": "Interface tour", "sub": "every element on screen", "html": f"""
{img("gater/tour.png", "A pad chopped by a 1/16 groove at 140 BPM, the last note still held.")}
{legend([
 ("Display", "the last two bars: input in grey, what passes in chartreuse, the envelope as a line; bar and beat lines while the host plays"),
 ("MIDI lamp", "lights on every note the gate receives - WAITING FOR MIDI until the first one"),
 ("SETUP", "the routing steps for this DAW; the other DAWs fold out beneath"),
 ("BYPASS", "the envelope rides to fully open - no switch, no click"),
 ("ATTACK", "how fast a note opens the gate, 0.2 to 100 ms"),
 ("RELEASE", "how fast it closes after the note ends, 1 to 1000 ms"),
 ("DEPTH", "how far it closes; 100 % = silence between notes"),
 ("VELOCITY", "IGNORE, or → LEVEL: the note's velocity sets how far the gate opens"),
])}
<p>Click the R logo in the header to flip the unit to its back panel (About, version, licence).</p>
"""})

    S.append({"title": "Controls reference", "sub": "what every control does, its range and default", "html": f"""
{img("gater/tour_knobs.png", "<b>ATTACK, RELEASE, DEPTH and VELOCITY.</b>", "w60")}
{ctl("ATTACK", "0.2 to 100 ms<br>default 1.5 ms<br>automatable", "<p>How fast a note opens the gate. Short values give a hard chop without a click; long values fade every note in, a swell instead of a cut.</p>", "The envelope is a one-pole that reaches 95 % of the way in the knob's time.")}
{ctl("RELEASE", "1 to 1000 ms<br>default 30 ms<br>automatable", "<p>How fast the gate closes after the note ends. This is the groove: 10-30 ms is a tight stutter, 80-200 ms lets each chop ring into the next gap, several hundred ms turns the pattern into a pulse.</p>", "A note that arrives during a release opens from wherever the envelope is.")}
{ctl("DEPTH", "0 to 100 %<br>default 100 %<br>automatable", "<p>How far the gate closes between notes. 100 % is silence; 50 % leaves the sound 12 dB down (breathing); 0 % does nothing. The value under the knob shows the level between notes in dB. Closed level = (1 − depth)².</p>")}
{ctl("VELOCITY", "IGNORE / → LEVEL<br>default IGNORE<br>automatable", "<p>With → LEVEL the note's velocity sets how far the gate opens, between the closed level and full. Accent the pattern in the piano roll's velocity lane.</p>")}
{ctl("BYPASS", "on / off", "<p>The envelope rides up to fully open, so switching it never clicks.</p>")}
{note("Every knob", "Drag (Shift = fine), mouse wheel (2 % a notch), double-click = default, right-click = the host's parameter menu (automation, link to controller).")}
<h3>The display</h3>
<p>The last two bars, sized from the host tempo. Grey is the input's peak history, chartreuse is what passes, the bright line is the gate envelope (top = open, bottom = closed). While the host plays, bar lines are drawn strong and beat lines faint. The tag in the corner tells you the state: <b>STOPPED · GATE OPEN</b>, <b>NOTE · GATE OPEN</b>, <b>GATE FOLLOWS THE NOTES</b>, <b>PLAYING · NO NOTES · GATE CLOSED</b> or <b>BYPASS</b>.</p>
"""})

    S.append({"title": "Routing - SETUP", "sub": "getting the notes to the gate", "html": f"""
{img("gater/setup.png", "<b>SETUP open.</b> This DAW's steps first; OTHER DAWS folds out the rest.", "w60")}
<p>RONE Gater is an audio effect that accepts MIDI. Every DAW routes a MIDI track into an effect differently; the SETUP chip shows the steps for the host it is running in.</p>
{table(["DAW", "Steps"], [
 ["FL Studio", "Add a <b>MIDI Out</b> channel and set its <b>Port</b> to 1 · open RONE Gater's wrapper ⚙ and set <b>Input port</b> to 1 · write the notes in MIDI Out's piano roll"],
 ["Cubase / Nuendo", "MIDI track → <b>Output Routing</b> → RONE Gater · write the notes on that track"],
 ["Ableton Live", "MIDI track → <b>MIDI To</b> → the audio track → RONE Gater · write the clip there"],
 ["Logic Pro", "Instrument track → <b>MIDI-controlled Effects</b> → RONE Gater · <b>Side Chain</b> = the track with the sound"],
 ["Studio One", "Instrument track → <b>Output</b> → RONE Gater"],
 ["Bitwig", "Route the note track into RONE Gater (<b>Note Receiver</b> / MIDI routing)"],
 ["Reaper", "Send MIDI from the note track to the audio track (<b>Route</b> → MIDI) · RONE Gater is on the audio track"],
 ["Standalone", "Play any key on your MIDI keyboard - the gate follows your notes"],
])}
"""})

    S.append({"title": "Step-by-step workflows", "sub": "recipes", "html": f"""
{recipe("Trance gate on a pad", "The classic 1/16 chop",
 steps([
  "Insert Gater on a sustained pad, route a MIDI track to it.",
  "Write sixteenths with a few gaps; leave ATTACK at 1.5 ms and RELEASE around 30 ms.",
  "Vary note lengths: a longer note is a longer open - the groove is in the piano roll.",
 ]))}
{recipe("Breathing bus", "Movement without holes",
 steps([
  "Set DEPTH to about 50 % (-12 dB between notes).",
  "RELEASE 100-200 ms so each chop leans into the next.",
  "Switch VELOCITY to → LEVEL and accent the off-beats.",
 ]))}
"""})

    S.append({"title": "Tips, tricks and troubleshooting", "sub": "", "html": f"""
<h3>Tips</h3>
<ul>
<li><strong>Short attack = hard chop.</strong> 0.2-2 ms cuts in on the note without a click; above 10 ms the chops start to swell.</li>
<li><strong>Release is the groove.</strong> The same notes with 15 ms and with 150 ms are two different patterns.</li>
<li><strong>DEPTH 50 %</strong> keeps the sound alive between notes - a pulse instead of silence.</li>
<li><strong>Before or after a reverb.</strong> Gate before the reverb and the tails ring through the gaps; gate after it and the reverb is chopped too, for the hard trance-gate sound.</li>
</ul>
<h3>Troubleshooting</h3>
{table(["Symptom", "Cause", "Fix"], [
 ["Silence while playing, SETUP pulses", "No note has reached the gate", "Follow SETUP for your DAW; the MIDI lamp must light"],
 ["The track is silent as soon as the plugin is inserted", "Rule one: no note = closed", "Intended - the sound passes only while a note is held; press BYPASS to hear the source"],
 ["The gate ignores which key I play", "Rule three: any key, any channel", "Intended - use velocity with → LEVEL for accents"],
 ["Clicks on the chops", "ATTACK or RELEASE at the very minimum on a low sound", "Raise ATTACK to 1-2 ms and RELEASE to 10 ms or more"],
])}
"""})

    m["sections"] = S
    return m
