def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    m = {
        "id": "reverb", "product": "RONE Reverb", "eyebrow": "THE LIGHT REVERB",
        "title_html": "RONE <i>Reverb</i>", "accent": "#E0789C", "version": "1.0.0",
        "tagline": "Five knobs, four spaces and an EQ on the tail: a clean reverb that stays out of the way of the sound it sits behind, and draws you exactly what it does.",
        "formats": ["VST3", "AU", "Standalone"], "vst3": "RONE Reverb.vst3", "au": "RONE Reverb.component", "exe": "RONE Reverb.exe",
        "pdf": "RONE Reverb - User Manual.pdf", "cover_img": "reverb/main.png",
        "channels": "Stereo in / stereo out &middot; mono in / stereo out &middot; mono in / mono out",
    }
    S = []

    S.append({"title": "Welcome to RONE Reverb", "sub": "a light reverb you can see", "html": f"""
<p><strong>RONE Reverb</strong> is the light reverb of the bundle: one window, five knobs, four spaces and a small EQ on the reverb. It is built to sound clean - a dense tail from the first millisecond that does not ring, does not get muddy and steps aside while the sound plays.</p>
<ul>
<li><strong>Five knobs</strong> - MIX, DECAY (0.2 to 50 s), PRE-DELAY (in milliseconds, or SYNC to a note value of the project's tempo), TONE (how fast the highs die next to the mids) and DUCK (the reverb bows while the sound plays and opens in the gaps).</li>
<li><strong>Four spaces</strong> - Room, Hall, Plate and Cathedral.</li>
<li><strong>An EQ on the reverb only</strong> - LOW CUT and HIGH CUT with 6, 12, 24 or 48 dB/oct, and one BELL. The dry sound is never touched.</li>
<li><strong>A picture of the reverb</strong> - the tail is drawn as a body over time. Drag its edge for the pre-delay, its end for the decay.</li>
<li><strong>45 presets</strong> by what they are for, each with a line that says why it is set the way it is, and <strong>MACRO 1-4</strong> for your own sounds.</li>
</ul>
<div class="two">
<div>
<h3>Where it shines</h3>
<ul>
<li><strong>Vocals</strong> - plates and halls that wait for the gaps, so the words stay in front.</li>
<li><strong>Psytrance and EDM leads</strong> - synced pre-delay, a deep duck and a low cut keep the riff sharp and the low end dry.</li>
<li><strong>Drums</strong> - short rooms and bright plates that do not blur the kick.</li>
<li><strong>Breakdowns and intros</strong> - tails of 20 to 50 seconds that turn one chord into a drone.</li>
</ul>
</div>
<div>
<h3>The 10-second version</h3>
{steps([
 "Insert RONE Reverb on a track (or on an FX / aux track).",
 "Play the sound: the default is a short, dark cathedral.",
 "Pick a preset by what it is for.",
 "Set MIX and DECAY to taste."])}
</div></div>
"""})

    inst = G["install_section"](m)
    a = "shows a <em>RONE Bundle License Required</em> screen"
    assert a in inst["html"], a
    inst["html"] = inst["html"].replace(a, "shows the <em>A RONE Reverb licence or RONE ALL ACCESS is required</em> screen (the dry sound passes through unchanged while it does)")
    S.append(inst)

    S.append({"title": "Quick start", "sub": "your first reverb", "html": f"""
{img("reverb/main.png", "<b>RONE Reverb</b> on its default sound: Cathedral, Mix 30 %, Decay 1.0 s, Pre-Delay 30 ms, Tone DARK 40, Duck 30 %.", "w80")}
{steps([
 "<strong>Insert</strong> RONE Reverb on the track you want to put in a space - or on an FX / aux track that you send several tracks to (then use a <em>Send 100%</em> preset, or turn MIX to 100 %).",
 "<strong>Play the sound.</strong> A fresh instance opens on the default: a short, dark cathedral (1 s, the highs gone above 1 kHz) that sits behind almost anything.",
 "<strong>Pick a preset</strong> - click the preset name. The categories are on the left; point at a preset to read what it is for, click it to load it.",
 "<strong>Set the amount with MIX</strong> and <strong>the length with DECAY</strong> (or drag the end of the picture).",
 "<strong>Keep the words or the riff in front</strong> with PRE-DELAY and DUCK.",
 "<strong>Clean the tail</strong> with the EQ: raise LOW CUT until the kick and bass are dry; lower HIGH CUT for a darker, further-away reverb.",
])}
{note("The picture is the manual", "Point at any knob or EQ point: the line at the top of the picture names the control, gives its value and says in a few words what it does, and the picture shows it on the body of the reverb.")}
"""})

    S.append({"title": "Interface tour", "sub": "every element on screen", "html": f"""
{img("reverb/tour.png", "The main window on the default sound, one hit 0.3 s after it played.")}
{legend([
 ("Header logo", "click to flip to the back panel (version, formats, licence)"),
 ("PRESET", "click the name for the preset browser; the arrows step through all 45 presets"),
 ("SPACE", "Room, Hall, Plate or Cathedral"),
 ("The picture", "the reverb drawn as a body over time. Drag its left edge for PRE-DELAY, further right for DECAY; the line on top explains the control under the mouse"),
 ("LOW CUT", "the frequency of the low cut; drag the number up or down"),
 ("BELL", "the frequency of the bell; drag the number up or down"),
 ("HIGH CUT", "the frequency of the high cut; drag the number up or down"),
 ("EQ curve", "the three points; the reverb's spectrum before (grey) and after (pink) the EQ"),
 ("SLOPE", "6, 12, 24 or 48 dB/oct for the selected cut; with the BELL selected: GAIN and Q"),
 ("MIX", "dry against reverb, 0 - 100 %"),
 ("DECAY", "how long the tail lasts, 0.2 - 50 s"),
 ("PRE-DELAY", "silence before the reverb starts, 0 - 250 ms"),
 ("SYNC", "PRE-DELAY as a note value of the project's tempo"),
 ("TONE", "DARK: the highs fade sooner &middot; BRIGHT: they ring longer"),
 ("DUCK", "the reverb steps back while the dry sound plays"),
 ("MACRO 1-4", "four keys for your own sounds, shared by every project"),
 ("Resize grip", "drag to scale the whole window"),
])}
"""})

    S.append({"title": "The picture", "sub": "the reverb as a body over time", "html": f"""
<p>The big screen on the left draws the reverb the way you hear it. Time runs from left to right - the first quarter of a second gets a quarter of the width, because that is where the pre-delay and the first echoes live - and the ruler at the bottom reads 0.1 s to 50 s.</p>
<ul>
<li><strong>The white ring on the left</strong> is the dry sound. It flashes at every hit; its size is the dry share of MIX.</li>
<li><strong>The body</strong> is the reverb: it starts after the pre-delay and ends at the decay. Its thickness is its level - more MIX, a thicker body and a smaller dry ring.</li>
<li><strong>The white core</strong> inside the body is the highs. TONE makes it shorter (DARK) or longer (BRIGHT).</li>
<li><strong>The rings</strong> along the body are the echoes: far apart at first, closer and closer as the space fills up - a Plate is dense at once.</li>
<li><strong>A white ring that travels down the body</strong> marks where each hit is right now.</li>
</ul>
<h3>Drag it</h3>
{table(["Gesture", "What it does"], [
 ("Drag the left edge (the handle after the dry ring)", "PRE-DELAY. In SYNC it lands on the nearest note value."),
 ("Drag anywhere further right", "DECAY: the round handle at the end of the body follows the mouse."),
 ("Point at a knob or an EQ point", "The line on top names it, gives its value and says what it does; the picture shows it."),
])}
<p>When nothing is touched the line reads <em>DRAG THE PICTURE &middot; OR TURN A KNOB</em>.</p>
<h3>What each control looks like</h3>
<p>The pictures below are Big Cathedral (60 ms, 7.5 s) with a hit 0.6 s old, while the mouse is on one control:</p>
<div class="two">
{img("reverb/pre_viz.png", "<b>PRE-DELAY</b> - the shaded gap before the body and its bracket.", "")}
{img("reverb/decay_viz.png", "<b>DECAY</b> - the bracket from the start of the body to its end.", "")}
</div>
<div class="two">
{img("reverb/mix_viz.png", "<b>MIX</b> - the DRY ring and the WET body, side by side.", "")}
{img("reverb/tone_viz.png", "<b>TONE</b> - the white outline is how far the highs reach.", "")}
</div>
<div class="two">
{img("reverb/duck_viz.png", "<b>DUCK</b> - the dashed line is the body without the duck.", "")}
{img("reverb/space_viz.png", "<b>SPACE</b> - the line on top describes the space (default sound).", "")}
</div>
"""})

    S.append({"title": "The five knobs", "sub": "MIX, DECAY, PRE-DELAY, TONE, DUCK", "html": f"""
{img("reverb/tour_knobs.png", "The knobs on the default sound, with MACRO 1-4 on the right.", "w80")}
{ctl("MIX", "0 - 100 % &middot; default 30 %", "Dry against reverb. 100 % is the reverb alone - for an FX / aux track that you send other tracks to (the <em>Send 100%</em> presets).")}
{ctl("DECAY", "0.2 - 50 s &middot; default 1.0 s", "How long the tail lasts (the time the mids take to die away). From a tight room to a 50-second drone; you can also drag the end of the picture.")}
{ctl("PRE-DELAY", "0 - 250 ms &middot; SYNC: OFF, 1/64 - 1/8 &middot; default 30 ms", "Silence between the dry sound and the start of the reverb. It keeps the attack of the sound dry and in front, so a vocal stays readable with a big space behind it.", "drag the left edge of the picture to set it by eye.")}
{ctl("SYNC", "the key under PRE-DELAY", "Turns PRE-DELAY into a note value of the project's tempo: OFF, 1/64, 1/32, 1/16T, 1/16, 1/8T, 1/16D, 1/8. The knob shows the note value and the line on top gives it in milliseconds at the current tempo, so the tail lands on the grid. Switching SYNC on keeps the length you had as the nearest note value, and off again as milliseconds.")}
{ctl("TONE", "DARK 100 - NEUTRAL - BRIGHT 100 &middot; default DARK 40", "How fast the highs die next to the mids. DARK: the highs fade sooner and the tail sounds warm and far away. BRIGHT: the highs ring longer, airy and glassy. NEUTRAL: the highs fade naturally.")}
{ctl("DUCK", "OFF - 100 % &middot; default 30 %", "The reverb steps back while the dry sound plays and opens up in the gaps - the space is heard between the notes, not on top of them. It follows the track's own level, so it works on a quiet and a loud track alike. At 100 % there is almost no reverb while the sound plays and the full tail right after it.")}
{note("Knob gestures", "Drag up or down (hold <span class='kbd'>Shift</span> for fine steps), or use the mouse wheel; in SYNC the wheel steps through the note values. Double-click resets a knob to its default; right-click opens your DAW's automation menu for it.")}
"""})

    S.append({"title": "SPACE", "sub": "Room, Hall, Plate, Cathedral", "html": f"""
<p>The four keys at the top right choose the kind of space. Each one is a different set of delay lengths inside the reverb - the way the first echoes arrive and how fast the tail becomes dense - while DECAY, TONE and the rest keep their values.</p>
{table(["Space", "Character", "Try it on"], [
 ("Room", "close walls, quick and tight", "drum kits, guitars, a vocal that should sound recorded in a real place"),
 ("Hall", "a big space that blooms slowly", "vocals, leads, pianos, pads"),
 ("Plate", "dense from the first moment", "vocals and snares, bright plucks, acid lines"),
 ("Cathedral", "a huge stone space, a long slow bloom", "big moments, strings, breakdowns - and the default sound"),
])}
{note("Switching while it plays", "Changing SPACE never cuts the tail: the old space stops taking in sound and rings out while the new one takes over.")}
"""})

    S.append({"title": "The EQ", "sub": "low cut, bell, high cut - on the reverb only", "html": f"""
<p>The screen on the right is an EQ on the <strong>reverb</strong>; the dry sound passes untouched. It has a low cut, one bell and a high cut. The white curve is what the EQ does; behind it you see the spectrum of the reverb right now - grey as it leaves the reverb, pink after the EQ.</p>
<div class="two">
{img("reverb/lc_eq.png", "<b>LOW CUT</b> selected: its slope is in the strip.", "")}
{img("reverb/bell_eq.png", "<b>BELL</b> selected: the strip shows GAIN and Q.", "")}
</div>
{ctl("LOW CUT", "20 Hz (OFF) - 2 kHz &middot; 6 / 12 / 24 / 48 dB/oct &middot; default 208 Hz, 24 dB/oct", "Keeps the lows out of the reverb, so the kick and the bass stay dry and the low end stays tight. All the way down reads OFF.", "on psytrance leads and vocals, 300 - 400 Hz at 24 dB/oct.")}
{ctl("BELL", "40 Hz - 16 kHz &middot; -12 to +12 dB &middot; Q 0.3 - 8 &middot; default 282 Hz, -6 dB, Q 1", "One band to pull an area of the tail back or lift it: take the boxy 300 - 500 Hz out of a vocal reverb, or the harsh 2.5 - 3 kHz out of a lead's. Higher Q = a narrower band.")}
{ctl("HIGH CUT", "1 kHz - 20 kHz (OFF) &middot; 6 / 12 / 24 / 48 dB/oct &middot; default 1 kHz, 12 dB/oct", "Keeps the highs out of the reverb: lower = darker and further away. All the way up reads OFF.")}
<h3>Working with it</h3>
{table(["Gesture", "What it does"], [
 ("Drag a point sideways", "Moves its frequency. The nearest point to the mouse is the one that moves."),
 ("Drag the bell point up or down", "Its gain."),
 ("Mouse wheel over a point", "On a cut: steps its slope. On the bell: its Q."),
 ("Double-click a point", "A cut: back to its default frequency and slope. The bell: gain and Q back to -6 dB and 1."),
 ("Drag a number up or down", "The frequencies on top, and GAIN / Q in the strip, are number boxes. Double-click one to reset it."),
 ("Click a point or a name", "Selects that band: the strip under the plot shows its SLOPE (a cut) or its GAIN and Q (the bell)."),
 ("<span class='kbd'>Shift</span> while dragging", "Fine steps."),
 ("Right-click", "Your DAW's automation menu for that band."),
])}
<p>The low cut always stays at least one octave below the high cut.</p>
"""})

    S.append({"title": "Presets", "sub": "45 starting points, by what they are for", "html": f"""
<div class="two">
{img("reverb/presets.png", "The browser on Psy Lead: the categories on the left, their presets with space and decay, and what the preset is for at the bottom.", "")}
<div>
<p>Click the preset name to open the browser:</p>
<ul>
<li><strong>DEFAULT</strong> sits at the top - the sound a new instance opens with.</li>
<li><strong>The categories</strong> are on the left with the number of presets in each; point at one (or click it) to see its presets.</li>
<li>Every preset shows its <strong>space and decay</strong>; point at it and the line at the bottom says <strong>what it is for</strong> and why it is set that way.</li>
<li>Click a preset to load it. <span class="kbd">Esc</span> or a click outside closes the browser.</li>
<li>The arrows beside the name step through all 45 presets in order.</li>
</ul>
</div></div>
{table(["Category", "Presets"], [
 ("Vocal (6)", "Vocal Plate, Vocal Hall, Vocal Air, Vocal Booth, Ballad Cathedral, Vocal Throw"),
 ("Drums (5)", "Snare Plate, Kit Room, Clap Hall, Arena Snare, Hat Shimmer"),
 ("Keys &amp; Plucks (5)", "Piano Hall, Pluck Space, Guitar Room, Keys Plate, Strings Hall"),
 ("Leads &amp; Synths (5)", "Lead Hall, Psy Lead, Acid Plate, Arp Sync, Supersaw Wide"),
 ("Pads &amp; Atmos (5)", "Dark Atmos, Big Cathedral, Endless Pad, Bright Cloud, Underwater"),
 ("Deep &amp; Endless (9)", "Infinite Drone, Frozen Cathedral, Deep Space, Shimmer Sky, Ambient Ocean, Breakdown Abyss, Vocal Eternity, Dark Matter, Send Infinite - tails of 18 to 50 s"),
 ("FX &amp; Transitions (5)", "Riser Wash, Dub Throw, Phone Verb, Lo-Fi Room, Ghost Tail"),
 ("Send 100% (4)", "Send Room, Send Plate, Send Hall, Send Cathedral - 100 % wet, for an FX / aux track"),
])}
<p>The presets follow a few rules you can reuse: pre-delay keeps the dry attack in front (a synced one lands the tail on the grid); DUCK on everything with words or a riff; the low cut keeps the kick and bass dry; the bell takes the boxy or harsh area out of the tail.</p>
"""})

    S.append({"title": "MACRO 1-4", "sub": "four of your own sounds", "html": f"""
{img("reverb/macros_knobs.png", "MACRO 1 and 2 saved (a small light on each); 2 is the one you are on.", "w80")}
<p>The four keys to the right of DUCK hold your own sounds:</p>
{table(["Action", "Result"], [
 ("Click an empty key", "Saves the current sound in it."),
 ("Click a full key", "Recalls it."),
 ("Hold a key (0.6 s)", "Saves the current sound over it."),
 ("Right-click a key", "Clears it."),
])}
<p>A macro keeps the whole sound: SPACE, the five knobs (PRE-DELAY as a note value when SYNC is on) and the EQ. A full key has a small light; the key you recalled or saved last is lit, until you move a control or load a preset.</p>
{note("Shared by every project", "The four macros belong to you, not to a project: every RONE Reverb on this computer, in every project, sees the same four. A project also carries them, so they come along to another computer that has none yet.")}
"""})

    S.append({"title": "Step-by-step workflows", "sub": "recipes", "html": f"""
{recipe("A vocal that stays in front", "pop, EDM, psytrance vocals",
 steps([
  "Preset <em>Vocal Plate</em> (or <em>Vocal Hall</em> for a bigger space).",
  "SYNC on with 1/16 or 1/8: the words start dry, the tail lands on the grid.",
  "DUCK 40 - 60 %: the space blooms between the phrases, not on top of them.",
  "LOW CUT 250 - 300 Hz at 24 dB/oct; pull the BELL down around 350 - 450 Hz if the tail sounds boxy.",
 ]))}
{recipe("A psytrance lead with a tail in the gaps", "leads and riffs",
 steps([
  "Preset <em>Psy Lead</em>: Hall, 1/16 pre-delay, 3.5 s, DUCK 55 %.",
  "LOW CUT around 400 Hz at 24 dB/oct keeps the bass and kick region dry.",
  "If the tail fights the dry lead, pull the BELL down at 2.5 - 3 kHz.",
 ]))}
{recipe("The last word of a line", "a vocal throw",
 steps([
  "Preset <em>Vocal Throw</em> (DUCK 85 %, 6 s).",
  "Automate MIX up on the last word only.",
  "The reverb hides under the voice and opens the moment the voice stops.",
 ]))}
{recipe("A drone for the breakdown", "deep and endless",
 steps([
  "Preset <em>Infinite Drone</em> (50 s) or <em>Deep Space</em> (120 ms, 30 s).",
  "Play one chord or one hit: it becomes a bed that holds under the whole breakdown.",
  "Lower HIGH CUT for a darker drone; raise LOW CUT before the drop so the kick has room.",
 ]))}
{recipe("One reverb for many tracks", "an FX / aux track",
 steps([
  "Insert RONE Reverb on an FX / aux track and choose a <em>Send 100%</em> preset (MIX 100 %).",
  "Send the tracks to it with your DAW's send knobs.",
  "One space for the whole mix: change it once and every track follows.",
 ]))}
"""})

    conv = G["conventions_section"](m)
    for a, b in (("when there is no DAW clock, tempo-synced controls follow the plugin's own BPM setting.",
                  "with no DAW clock a synced PRE-DELAY uses 120 BPM."),
                 ("<td>Resets it to the default value.</td>",
                  "<td>Resets it: MIX 30 %, DECAY 1.0 s, PRE-DELAY 30 ms, TONE DARK 40, DUCK 30 %. On the EQ, double-click a point or a number box.</td>"),
                 ("<td>Steps through segmented switches and lists where the plugin offers them.</td>",
                  "<td>Turns a knob (in SYNC: one note value per step); over an EQ point it steps a cut's slope or the bell's Q.</td>")):
        assert a in conv["html"], a
        conv["html"] = conv["html"].replace(a, b)
    S.append(conv)
    S.append(G["support_section"](m, [
        ("I hear no reverb", "Check MIX (0 % is the dry sound alone) and DUCK: at a high DUCK the reverb is quiet while the sound plays and opens only in the gaps. A LOW CUT very high together with a HIGH CUT very low leaves little of the tail."),
        ("The low end gets muddy", "Raise LOW CUT (200 - 400 Hz) or choose a steeper slope (24 or 48 dB/oct)."),
        ("The reverb covers the words or the riff", "Add PRE-DELAY (or SYNC 1/16 - 1/8) and raise DUCK."),
        ("The tail sounds harsh or thin", "Turn TONE towards DARK, lower HIGH CUT, or pull the BELL down at 2.5 - 3 kHz."),
        ("The synced pre-delay is not on the grid", "SYNC follows the project's tempo; with no DAW clock (standalone) it uses 120 BPM."),
        ("A MACRO key does nothing", "An empty key saves on a click, a full one recalls. To replace a full one, hold it for 0.6 s; right-click clears it."),
    ]))

    m["sections"] = S
    return m
