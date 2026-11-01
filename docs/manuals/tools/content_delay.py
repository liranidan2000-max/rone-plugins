def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    m = {
        "id": "delay", "product": "RONE Delay", "eyebrow": "THE SIMPLE DELAY",
        "title_html": "RONE <i>Delay</i>", "accent": "#B39352", "version": "1.1.1",
        "tagline": "One TIME dial with every note value written on it, a filter inside the loop, four knobs - and a screen that shows the sound going in and every echo coming out.",
        "formats": ["VST3", "AU", "Standalone"], "vst3": "RONE Delay.vst3", "au": "RONE Delay.component", "exe": "RONE Delay.exe",
        "pdf": "RONE Delay - User Manual.pdf", "cover_img": "delay/main.png",
        "channels": "Mono or stereo in / out (DUAL works on stereo: left and right are two separate delays)",
    }
    S = []

    S.append({"title": "Welcome to RONE Delay", "sub": "the delay you understand at a glance", "html": f"""
<p><strong>RONE Delay</strong> is a feedback delay built to be read, not studied. Turn one dial for the time, pick how the echoes sit in the stereo field, set how many come back and how they darken - and watch them on screen while the track plays.</p>
<ul>
<li><strong>One TIME dial</strong> - straight, dotted and triplet note values in one turn, sorted by length, from 1/64 to a dotted bar. The value is written out in the middle of the dial, never a bare dot.</li>
<li><strong>Four stereo modes</strong> - MONO, SPREAD, PING-PONG and DUAL (two delays, left and right, each with its own TIME).</li>
<li><strong>A filter inside the loop</strong> - every echo passes the high-pass and the low-pass once more, so each repeat is a little thinner and darker than the last.</li>
<li><strong>SEND and WET</strong> - close SEND and the tail rings on (the throw); close WET and the echoes are gone at once.</li>
<li><strong>Safe by design</strong> - FEEDBACK at 100 % holds the echoes forever instead of building up, and no echo ever passes 0 dBFS.</li>
</ul>
<p><strong>The dry signal is never touched.</strong> What you hear is your sound plus the echoes - closing every knob gives you back exactly what came in.</p>
<div class="two">
<div>
<h3>Where it shines</h3>
<ul>
<li><strong>Vocal throws</strong> - hold one word into the delay with HOLD and let it ring.</li>
<li><strong>Rhythmic leads and plucks</strong> - dotted and triplet echoes that sit on the groove.</li>
<li><strong>Slapback and doubling</strong> - short MS times for width without a tail.</li>
<li><strong>Dub</strong> - high feedback, a dark loop filter and KILL to stop it on cue.</li>
</ul>
</div>
<div>
<h3>The 10-second version</h3>
{steps([
 "Insert RONE Delay on the track.",
 "Pick a preset - the list is sorted by use.",
 "Turn TIME to the note value you want.",
 "Set FEEDBACK for how many echoes, WET for how loud."])}
</div></div>
"""})

    inst = G["install_section"](m)
    # this plugin's own licence screen
    inst["html"] = inst["html"].replace("shows a <em>RONE Bundle License Required</em> screen", "shows the <em>A RONE Delay licence or RONE ALL ACCESS is required</em> screen")
    S.append(inst)

    S.append({"title": "Quick start", "sub": "your first echo", "html": f"""
{img("delay/main.png", "<b>RONE Delay</b> on its opening preset, HOOK DOTTED: a vocal hook on top, its ping-pong echoes jumping left and right below it.", "w80")}
{steps([
 "<strong>Insert</strong> RONE Delay on a vocal, a lead or a pluck. Use it as an insert: there is no dry control - your sound always passes - so on an FX return the sent signal would be heard twice.",
 "<strong>Press play.</strong> The LIVE screen draws the sound going in (IN, white) and the echoes coming out (L and R, champagne) over the last two bars.",
 "<strong>Pick a preset</strong> - click the name for the list, or step with the arrows. <span class='pill'>HOOK DOTTED</span> is the opening setting: a dotted 8th, ping-pong, a band-passed loop.",
 "<strong>Turn TIME.</strong> Every step is a note value on the host tempo; the readout under the dial writes it out (<em>DOTTED 8TH &middot; 317 ms @ 142 BPM</em>).",
 "<strong>Set FEEDBACK</strong> for how many echoes come back, <strong>WET</strong> for how loud they are. The ONE HIT map shows where every echo lands and how loud it is before you play a note.",
 "<strong>Shape the loop</strong> - drag the HP and LP handles on the filter strip. Every repeat goes through them again.",
])}
{note("No tempo in the standalone", "Without a host the plugin runs at 120 BPM, so SYNC values are worked out at that tempo.")}
"""})

    S.append({"title": "Interface tour", "sub": "every element on screen", "html": f"""
{img("delay/tour.png", "The main page in PING-PONG with the opening preset.")}
{legend([
 ("Header logo", "click to flip to the back panel (About, version, licence)"),
 ("PRESET", "the name opens the factory list by category; the arrows step through it"),
 ("Stereo mode", "MONO, SPREAD, PING-PONG or DUAL"),
 ("BYPASS", "the echoes go, the dry stays"),
 ("SYNC / MS", "note values on the host tempo, or free time in milliseconds"),
 ("TIME dial", "the delay time; in DUAL two small dials, LEFT and RIGHT, take its place"),
 ("Time readout", "the value written out (DOTTED 8TH) and its length in ms at the host tempo"),
 ("STRAIGHT / DOTTED / TRIPLET", "which family the current step belongs to - a readout, not a switch"),
 ("LIVE scope", "the last two bars: IN (the dry) and the echoes on L and R"),
 ("ONE HIT map", "one hit and every echo it makes: when, which side, how loud"),
 ("Loop filter", "the HP and LP handles inside the feedback loop; the wheel sets each slope"),
 ("FEEDBACK", "how many echoes; 100 % = infinite hold"),
 ("SEND", "how much goes into the delay; closing it lets the tail ring"),
 ("WET", "how loud the echoes are; closing it cuts them"),
 ("DUCK", "the echoes dip while the sound plays and come back in the gaps"),
 ("KILL", "the tail is gone in 20 ms"),
 ("HOLD", "SEND at 100 % for as long as you hold the button"),
 ("Lamps", "REST ON STOP, SAFE and DUCKING light when the engine does that"),
 ("MACRO", "four slots for your own settings, saved with the project"),
 ("Resize grip", "drag to resize the window"),
])}
"""})

    S.append({"title": "TIME", "sub": "one dial, every note value", "html": f"""
<div class="two"><div>{img("delay/tour_time.png", "SYNC: the dial, the readout and the family of the step.", "")}</div><div>
<h3>SYNC - note values</h3>
<p>The dial holds <strong>19 note values in one turn</strong>, sorted by length - straight, dotted and triplet mixed in the order you hear them get longer:</p>
<p class="mono" style="font-size:8.6pt">1/64 &middot; 1/32T &middot; 1/32 &middot; 1/16T &middot; 1/32. &middot; 1/16 &middot; 1/8T &middot; 1/16. &middot; 1/8 &middot; 1/4T &middot; 1/8. &middot; 1/4 &middot; 1/2T &middot; 1/4. &middot; 1/2 &middot; 1/1T &middot; 1/2. &middot; 1/1 &middot; 1/1.</p>
<p>Around the dial, <strong>white dots</strong> with a label are the straight values, <strong>champagne dots</strong> the dotted ones and <strong>rings</strong> the triplets. The middle of the dial always writes the current value out (<em>1/8.</em>), the line under it names it (<em>DOTTED 8TH</em>) and gives its length at the host tempo (<em>317 ms @ 142 BPM</em>). The <strong>STRAIGHT / DOTTED / TRIPLET</strong> row lights the family of that step.</p>
</div></div>
{table(["Gesture", "What it does"], [
 ("Drag the dial up / down", "One step every 16 pixels; hold <span class='kbd'>Shift</span> for finer travel (40 pixels a step)."),
 ("Mouse wheel / arrow keys", "One step per notch or key press."),
 ("Click a dot or a label", "Jumps straight to that value."),
 ("Right-click the dial (VST3)", "The host's automation menu for TIME."),
])}
<h3>MS - free time</h3>
<div class="two"><div>{img("delay/ms_time.png", "MS: 110 ms, with the nearest note value under it.", "")}</div><div>
<p>Switch to <strong>MS</strong> for a time that is not tied to the tempo: <strong>5 ms to 2 s</strong>, on a log scale marked 5, 10, 25, 50, 100, 250, 500 ms, 1 s and 2 s (click a number to jump to it). The readout shows the nearest note value at the host tempo (<em>&asymp; 1/16 @ 142 BPM</em>), so you know where a free time sits against the beat. Drag, the wheel and the arrow keys work as in SYNC; <span class='kbd'>Shift</span> is fine.</p>
<p><strong>Switching between SYNC and MS keeps the time you hear:</strong> SYNC &rarr; MS takes the note's length in milliseconds, MS &rarr; SYNC takes the nearest note value.</p>
</div></div>
{note("A clean jump, no pitch glide", "When TIME changes, a second read head starts at the new time and the two crossfade over 30 ms. The echoes already in the delay stay where they are - no tape-style pitch bend, no click.")}
{ctl("TIME", "SYNC: 1/64 to 1/1. (19 steps)<br>MS: 5 ms to 2 s<br>automatable", "<p>The delay time. In SYNC it follows the host tempo; in MS it is fixed. The opening preset is 1/8. (dotted 8th).</p>")}
{ctl("SYNC / MS", "default SYNC<br>automatable", "<p>Which of the two the dial sets. Both values are kept, so switching back and forth never loses your setting.</p>")}
"""})

    S.append({"title": "Stereo modes", "sub": "MONO, SPREAD, PING-PONG, DUAL", "html": f"""
<p>The switch in the header decides how the echoes sit in the stereo field. In MONO, SPREAD and PING-PONG the delay is fed with your sound summed to mono; in DUAL each side feeds its own delay.</p>
{table(["Mode", "What you hear"], [
 ("MONO", "One delay line, the echoes in the middle."),
 ("SPREAD", "Two lines, the right one 17 ms longer: the same echoes, but wide."),
 ("PING-PONG", "The echoes jump from side to side: the first on the left, then right, left, right."),
 ("DUAL", "Two separate delays: left in &rarr; left out, right in &rarr; right out, each with its own TIME."),
])}
<p>Changing the mode rewires the loop, so the echoes in it fade out in 20 ms and the delay starts clean in the new mode.</p>
<h3>DUAL - two TIME dials</h3>
{img("delay/dual.png", "DUAL (the DUAL DOTTED preset): a dotted 8th on the left against a quarter on the right.", "w80")}
{legend([
 ("DUAL", "the fourth stereo mode"),
 ("LEFT", "the left delay's TIME, with its name and length under it"),
 ("RIGHT", "the right delay's TIME"),
])}
<p>In DUAL the big dial gives way to <strong>two small dials side by side, LEFT and RIGHT</strong>. Each sets its own side directly: drag, wheel and arrow keys as on the big dial, <span class='kbd'>Shift</span> for fine. The value is written inside each dial, and the name and length under it. SYNC / MS applies to both. FEEDBACK, SEND, WET, DUCK and the loop filter are shared by the two sides.</p>
<p>The right side opens as a quarter note (375 ms in MS). The ONE HIT map draws the left echoes above the line and the right ones below, so you see the two rhythms against each other.</p>
"""})

    S.append({"title": "The loop filter", "sub": "every echo a little thinner, a little darker", "html": f"""
{img("delay/filter_strip.png", "The filter strip with the pointer on it (RADIO TAIL): each handle shows its frequency, and its slope underneath.")}
<p>The strip under the screens is a <strong>high-pass and a low-pass inside the feedback loop</strong>. The first echo passes them once, the second twice, the third three times - so every repeat is thinner and darker than the one before, the way old tape and analogue delays faded out. The curve on the strip is the filter's real response.</p>
{table(["Gesture", "What it does"], [
 ("Drag on the strip", "The nearer handle follows. HP always stays a little below LP."),
 ("Mouse wheel on the strip", "The nearer handle's slope: 6, 12, 18, 24, 36 or 48 dB/oct (wheel up = steeper)."),
 ("Double-click near a handle", "Turns that filter off: HP to 20 Hz (OFF), LP to 20 kHz (OFF)."),
 ("Right-click near a handle (VST3)", "The host's automation menu for that frequency."),
])}
<p>At rest each handle shows only its frequency (<em>HP 450</em>, <em>LP 2.5k</em>). While the pointer is on the strip the slope appears as a second line under it.</p>
{ctl("HP / LP", "20 Hz to 20 kHz<br>HP at 20 Hz = OFF, LP at 20 kHz = OFF<br>automatable", "<p>Where the loop cuts the lows and the highs. The opening preset is HP 450 Hz, LP 2.5 kHz - a band that keeps the echoes out of the way of the dry.</p>", "Both cuts are Butterworth (maximally flat): no resonant bump and never above 0 dB, so a steep slope is safe inside the loop even at high feedback.")}
{ctl("Slopes", "6 / 12 / 18 / 24 / 36 / 48 dB/oct<br>default 12 dB/oct<br>automatable", "<p>How steep each cut is, set separately for HP and LP. 6 dB/oct darkens gently over many repeats; 36 and 48 dB/oct give the hard band-passed sound of a telephone or radio.</p>")}
"""})

    S.append({"title": "FEEDBACK, SEND, WET, DUCK", "sub": "the four knobs", "html": f"""
{img("delay/tour_knobs.png", "<b>FEEDBACK, SEND, WET and DUCK.</b>", "w60")}
{ctl("FEEDBACK", "0 to 100 %<br>100 % = &infin; hold<br>automatable", "<p>How much of each echo goes back into the delay - how many echoes you hear. Hover the knob: its tooltip tells you how long the tail lives now and how many repeats it has (<em>Now: &hellip; s, &hellip; repeats</em>, or <em>&infin; HOLD</em>).</p><p>At <strong>100 %</strong> the knob shows <strong>&infin;</strong>, its arc turns white and the echoes hold forever - an infinite loop you can play over. SAFE keeps it from building up.</p>")}
{img("delay/hold_map.png", "The ONE HIT map at FEEDBACK 100 % (INFINITE HOLD): every echo as loud as the first.", "")}
{ctl("SEND", "0 to 100 %<br>automatable", "<p>How much of your sound goes <em>into</em> the delay. Close it and what is already inside keeps ringing - the classic <strong>throw</strong>: open SEND for one word, close it, and the word repeats on its own.</p>")}
{ctl("WET", "0 to 100 %<br>automatable", "<p>How loud the echoes are. Close it and they are gone at once. The dry is never touched by WET (or by any other control).</p>")}
{ctl("DUCK", "0 to 100 %<br>0 = OFF<br>automatable", "<p>The echoes dip while your sound plays and come back up in the gaps, so a busy vocal stays clear and the tail fills the pauses. Ducking starts when the dry reaches -40 dBFS and is full at -10 dBFS; after the dry stops the echoes come back over about 150 ms. DUCK sets how deep they go - at 100 % they disappear under a loud phrase.</p>")}
{note("SEND or WET?", "Both make the echoes quieter, but not the same way. <strong>SEND</strong> changes what goes in: lower it and the echoes already in the delay ring on at full level. <strong>WET</strong> changes what comes out: lower it and every echo, old and new, drops at once. For a throw, automate SEND; to mute the delay, close WET.")}
{note("Every knob", "Drag (Shift = fine), mouse wheel (2 % a notch, Shift 0.5 %), right-click = the host's parameter menu. Double-click resets a knob: FEEDBACK to 45 %, SEND to 100 %, WET to 55 %, DUCK to OFF.")}
"""})

    S.append({"title": "The screens", "sub": "LIVE and ONE HIT", "html": f"""
<h3>LIVE &middot; 2 BARS</h3>
<p>The big screen shows the last <strong>two bars</strong> at the host tempo, scrolling to the left, with the newest moment at the right edge. Three lanes: <strong>IN</strong> is your sound going in (white), <strong>L</strong> and <strong>R</strong> are the echoes coming out of each side (champagne). Heights are peak levels in dB, so quiet late echoes stay visible. While the host plays, bar lines are drawn strong and beat lines faint.</p>
<p>Use it to see what the settings do: in PING-PONG the echoes alternate between the L and R lanes, in SPREAD they sit on both a little apart, in DUAL each lane follows its own TIME. With DUCK up you see the echoes shrink under the phrase and swell in the gap after it.</p>
<h3>ONE HIT &rarr; WHAT YOU HEAR</h3>
<p>The narrow screen is a prediction you can read before you play: the white bar on the left is one hit, and every champagne bar after it is an echo - <strong>where it lands in time, on which side and how loud</strong>. Left echoes point up from the centre line, right echoes down; in MONO each echo is drawn on both. The heights come from the real loop filter, FEEDBACK, SEND and WET, so the map changes as you turn them. Faint lines mark the beats, stronger ones the bars.</p>
{img("delay/ms.png", "VOCAL SLAP: MONO, 110 ms, FEEDBACK 12 %. The map shows three short echoes and nothing more; LIVE shows them right behind the phrase.", "w80")}
"""})

    S.append({"title": "Footer: KILL, HOLD, lamps, BYPASS", "sub": "the performance controls", "html": f"""
{img("delay/tour_foot.png", "The footer: KILL, HOLD, the three lamps and the MACRO slots.")}
{ctl("KILL", "button", "<p>Ends the tail now: the echoes fade out in 20 ms and the delay is emptied. Your settings do not change, and the next sound you send starts a fresh set of echoes.</p>")}
{ctl("HOLD = SEND 100%", "hold the button<br>automatable (Hold Send)", "<p>While you hold it, SEND is at 100 % whatever the knob says; let go and SEND is back to the knob. Set SEND to 0 and HOLD becomes a throw button: hold it over one word, release, and the word repeats on its own. As a host parameter it can be automated for exact throws.</p>")}
{ctl("BYPASS", "on / off<br>automatable", "<p>The echoes fade out and your sound passes untouched. Switching it never clicks.</p>")}
<h3>The lamps</h3>
<p>The three lamps are read-outs, not switches: the engine always does these things, and the lamp lights when it happens.</p>
{table(["Lamp", "Lights when"], [
 ("REST ON STOP", "The host has just started or stopped playing. On both edges the echoes fade out in 20 ms and the delay is emptied, so a stopped project is silent and every play starts clean - no old tail from where you were before. The standalone has no transport, so it never rests."),
 ("SAFE", "The loop is riding its safety knee. What goes into the delay is clean up to -3 dBFS and runs into a soft knee above it, so a loud input or FEEDBACK at 100 % holds instead of building up, and no echo ever passes 0 dBFS."),
 ("DUCKING", "DUCK is up and the echoes are being pulled down under your sound."),
])}
"""})

    S.append({"title": "Presets and MACRO", "sub": "30 factory presets and four slots of your own", "html": f"""
<div class="two">
<div>{img("delay/presets.png", "The preset list, opened from the name: grouped by category, the live preset marked, its time on the right.", "")}</div>
<div>
<h3>Presets</h3>
<p>Click the <strong>preset name</strong> to open the list. The 30 factory presets are grouped by use - <strong>VOCAL, RHYTHM, SLAP, DUB, SPACE, DUAL</strong> and <strong>FX</strong> - and each row shows its time on the right (a note value, a time in ms, or DUAL). The <strong>&#9664; &#9654;</strong> arrows step through the whole list.</p>
<p>A preset sets TIME (both sides for the DUAL presets), the stereo mode, FEEDBACK, SEND, WET, DUCK and the loop filter with its slopes. Change anything and a <strong>*</strong> appears after the name. The preset name is saved with the project.</p>
</div></div>
{table(["Category", "Presets"], [
 ("VOCAL", "HOOK DOTTED &middot; VOCAL SLAP &middot; VOCAL QUARTER &middot; WIDE VOCAL &middot; HOLD THROW &middot; RADIO TAIL"),
 ("RHYTHM", "CLEAN QUARTER &middot; EIGHTH PING &middot; DOTTED LEAD &middot; TRIPLET SPREAD &middot; TRIPLET PING &middot; 16TH SHIMMER &middot; DRUM GROOVE"),
 ("SLAP", "SLAP &middot; DOUBLER &middot; ROOM SLAP &middot; FLAM"),
 ("DUB", "DUB THROW &middot; DUB SPACE &middot; DARK DUB &middot; STEPPER"),
 ("SPACE", "BIG SPACE &middot; WASH &middot; INFINITE HOLD"),
 ("DUAL", "DUAL DOTTED &middot; DUAL SLAP &middot; DUAL POLY &middot; DUAL DRIFT"),
 ("FX", "TELEPHONE &middot; RISER FEED"),
])}
<h3>MACRO 1-4</h3>
<div class="two"><div>{img("delay/macros_slots.png", "Slots 1 and 2 hold a setting (the small dot); you are on slot 2.", "")}</div><div>
<p>Four slots in the footer for <strong>your own settings</strong>, saved with the project.</p>
{table(["Gesture", "What it does"], [
 ("Click an empty slot", "Saves the current settings there."),
 ("Click a full slot", "Recalls it."),
 ("Hold a slot (about 0.6 s)", "Saves over it."),
 ("Right-click a slot", "Clears it."),
])}
</div></div>
<p>A full slot shows a small dot; the slot you are on is lit, and it goes dark again as soon as you change something. A slot keeps TIME (SYNC / MS and both sides), the stereo mode, FEEDBACK, SEND, WET, DUCK and the loop filter with its slopes - not BYPASS or HOLD.</p>
"""})

    S.append({"title": "Step-by-step workflows", "sub": "recipes", "html": f"""
{recipe("The vocal throw", "one word repeats, the rest stays dry",
 steps([
  "Preset HOLD THROW (SEND 0, a dotted quarter, ping-pong).",
  "Play the vocal and hold <strong>HOLD</strong> over the word you want thrown; let go after it.",
  "Set FEEDBACK for how long it rings; darken it with LP.",
  "To print it, automate <em>Hold Send</em> in your DAW instead of holding the button.",
 ]))}
{recipe("The dotted lead", "echoes that answer the groove",
 steps([
  "Preset DOTTED LEAD, or TIME on 1/8. in PING-PONG.",
  "FEEDBACK 40-60 %, HP around 300 Hz so the echoes stay out of the bass.",
  "DUCK 30-50 % keeps the lead's notes clear and lets the echoes fill the gaps.",
 ]))}
{recipe("Slapback and doubling", "width without a tail",
 steps([
  "Preset VOCAL SLAP or DOUBLER - TIME in MS (20-120 ms).",
  "FEEDBACK low (0-15 %), SPREAD for width.",
  "Close WET until the double sits behind the voice.",
 ]))}
{recipe("A dub spin", "high feedback under control",
 steps([
  "Preset DUB THROW or DARK DUB.",
  "Ride FEEDBACK up towards 100 % - SAFE keeps the loop from running away.",
  "Pull LP down while it spins, then press <strong>KILL</strong> on the downbeat.",
 ]))}
{recipe("Two rhythms at once", "DUAL",
 steps([
  "Preset DUAL POLY (a triplet against a dotted 8th) or DUAL DOTTED.",
  "Set LEFT and RIGHT to two values that cross - the ONE HIT map shows the pattern they make.",
  "Keep FEEDBACK moderate; two lines fill up twice as fast.",
 ]))}
"""})

    conv = G["conventions_section"](m)
    # the shared text, fitted to this plugin: no BPM setting of its own, the wheel works on the dials / knobs / filter strip
    for a, b in (("when there is no DAW clock, tempo-synced controls follow the plugin's own BPM setting.", "with no DAW clock RONE Delay runs at 120 BPM."),
                 ("Steps through segmented switches and lists where the plugin offers them.", "Steps the TIME dials and the knobs; on the filter strip it sets the slope."),
                 ("<td>Resets it to the default value.</td>", "<td>Resets it: FEEDBACK 45 %, SEND 100 %, WET 55 %, DUCK OFF.</td>")):
        assert a in conv["html"], a
        conv["html"] = conv["html"].replace(a, b)
    S.append(conv)
    S.append(G["support_section"](m, [
        ("The echoes keep going after I stop the song", "They should not: REST ON STOP empties the delay when the host stops. In the standalone there is no transport - press KILL."),
        ("SYNC times are wrong in the standalone", "The standalone has no host tempo and runs at 120 BPM. Use MS for exact times there."),
        ("The tail builds up and gets loud", "SAFE stops it at its knee, but very high FEEDBACK with an open filter is dense by nature: lower FEEDBACK, raise HP or lower LP."),
        ("Closing SEND does not stop the echoes", "That is SEND's job - it lets the tail ring. Close WET (or press KILL) to stop them."),
        ("DUAL sounds like one delay", "DUAL keeps left and right apart, so a mono source feeds both sides the same sound - set LEFT and RIGHT to different times to hear two delays. On a mono track only the LEFT delay is heard."),
        ("A preset name has a * after it", "You changed something since loading it. Load it again to go back, or save your version in a MACRO slot."),
    ]))

    m["sections"] = S
    return m
