def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    m = {
        "id": "rise", "product": "RONE Rise", "eyebrow": "ONE KNOB. THE WHOLE BUILD-UP.",
        "title_html": "RONE <i>Rise</i>", "accent": "#E0449A", "version": "1.2.0",
        "tagline": "Turn one knob up over the phrase and the mix becomes a build-up - the lows leave, the reverb opens, the echoes climb. Snap it to zero and the drop hits whole, with the tail still spilling into it. Or press GO and let it land on the bar by itself.",
        "formats": ["VST3", "AU", "Standalone"], "vst3": "RONE Rise.vst3", "au": "RONE Rise.component", "exe": "RONE Rise.exe",
        "pdf": "RONE Rise - User Manual.pdf", "cover_img": "rise/tour_knob.png",
    }
    S = []

    S.append({"title": "Welcome to RONE Rise", "sub": "a build-up on one knob", "html": f"""
<p><strong>RONE Rise</strong> turns whatever you put through it into a build-up. One knob, <strong>INTENSITY</strong>, drives a whole chain the way you would automate it by hand over eight bars: a steep high-pass takes the low end away, a big reverb opens up, echoes and noise climb in, and in the bigger modes a gate speeds up, the pitch rides upward and a barber-pole shimmer climbs forever. Pull the knob back to zero on the downbeat and the drop lands - the low end back in a few milliseconds, the first kick whole, the reverb and the echoes still ringing under it.</p>
<p>The sun on the right is the knob made visible, with a face for every mode: it sleeps below the horizon at zero and rises as you turn it up - happy and pink in LIFTOFF, starry-eyed with a ring of moons in ORBIT, strobing gold in PULSAR, under a rising sea in UNDERTOW and furious in MELTDOWN. It pulses on the beat, holds its breath before the drop and puts its shades on when the drop lands.</p>
<div class="two">
<div>
<h3>Where it shines</h3>
<ul>
<li><strong>The last 8 bars before a drop</strong> - on the mix bus, the drum bus or a music group.</li>
<li><strong>Transitions</strong> - a one-bar fill or a four-bar lift between two sections without touching a single filter.</li>
<li><strong>Into a breakdown</strong> - FALL starts at the top on the bar line and comes down by itself.</li>
<li><strong>Live and in the DJ set</strong> - LAND ON BAR runs the rise for you and lands it on the bar.</li>
<li><strong>The drop that breathes</strong> - GAP leaves a beat of silence before it, TAIL keeps the wash under it.</li>
</ul>
</div>
<div>
<h3>The 10-second version</h3>
{steps([
 "Insert Rise on your mix or drum bus.",
 "Press play, then AUDITION, and step through the presets until one fits.",
 "Automate INTENSITY from 0 to 100 over the last 8 bars.",
 "Snap it back to 0 on the drop."])}
</div></div>
{note("Why it gets quieter at the top", "A build-up has to leave room for the drop. As INTENSITY rises the dry signal sinks (about -7 dB at 100 %, set by SINK in ADVANCED) while the reverb and echoes rise, so the moment you pull the knob down the full mix comes back louder than it left. That jump is the drop.")}
"""})

    S.append(G["install_section"](m))

    S.append({"title": "Quick start", "sub": "your first rise", "html": f"""
{img("rise/tour.png", "<b>RONE Rise.</b> INTENSITY on the left, the sun on the right, the presets on top, the five modes along the bottom.", "w80")}
{steps([
 "<strong>Insert</strong> Rise on the mix bus or a group bus. At 0 % it does nothing at all.",
 "<strong>Hear the presets in the music:</strong> press play in your host, then <span class='pill'>AUDITION</span> under the knob. The rise runs by itself in a loop - up, the drop, a bar or two of rest, again - while you step through the presets with the arrows on top or roll new ones with the dice. Press AUDITION again to stop.",
 "<strong>Pick a mode</strong> along the bottom - <span class='pill'>LIFTOFF</span> is the clean, classic lift - or a preset from the bar on top (17 of them, from CLEAN LIFT to CHAOS).",
 "<strong>Automate <span class='pill'>INTENSITY</span></strong> from 0 to 100 % over the bars before the drop. A curve that stays low and rises late sounds most natural.",
 "<strong>Pull it back to 0 on the drop.</strong> With TAIL on CUT (the default) the drop is a hard stop; set TAIL to 1-4 bars in the header and the reverb and echoes keep ringing into the drop.",
 "<strong>Or let it run itself:</strong> open ADVANCED, set <span class='pill'>LAND ON BAR</span> to 8 and press <span class='pill'>GO</span>. The rise starts from the bar you pressed it in and lands exactly eight bars later.",
])}
{note("Right-click the knob", "Right-click any control for the host's own menu - in FL Studio that is where you create the automation clip or link it to a controller. To type an exact value, click the number on the knob.")}
"""})

    S.append({"title": "Interface tour", "sub": "every element on screen", "html": f"""
{img("rise/tour.png", "A LAND ON BAR run in progress: bar 5 of 8, the outer ring counting the bars.")}
{legend([
 ("Header logo", "click to flip to the back panel (About, version, licence)"),
 ("Preset bar", "the loaded preset (or MODE and the card); click the name for the list, the arrows step through it"),
 ("DICE", "a new random build-up in one click, inside the limits of taste"),
 ("SAVE", "save the macros as your own preset, or as the mode card's own macros"),
 ("Settings", "RESET TO FACTORY MACROS, and DELETE a preset of yours"),
 ("TAIL", "CUT or 1-4 bars of reverb and echo after the knob falls"),
 ("BYPASS", "the untouched input, 10 ms crossfade"),
 ("INTENSITY", "the one knob, 0 to 100 %; click the number to type a value; the outer ring counts the bars of a run"),
 ("AUDITION", "the rise in a loop while the song plays, to hear presets in your music"),
 ("GO", "appears when LAND ON BAR is set: runs the rise and lands it"),
 ("Bar counter", "which bar of the run you are in, and the beat; NEXT BAR while a FALL waits, GAP in the silence"),
 ("Scene", "the sun's scene: NIGHT at zero, then the mode's own - DAWN to HIGH NOON, WARM to MELTDOWN"),
 ("Modes", "five chains: LIFTOFF, ORBIT, PULSAR, UNDERTOW, MELTDOWN"),
 ("ADVANCED", "slides up LAND ON BAR, RISE / FALL, CURVE, GAP, the macros and the chain strip"),
 ("Resize grip", "drag to resize the window"),
])}
{img("rise/adv.png", "ADVANCED open over the lower half of the window.")}
{legend([
 ("LAND ON BAR", "MANUAL, or 1 / 2 / 4 / 8 / 16 / 32 bars for GO"),
 ("RISE / FALL", "GO climbs to the drop, or falls from the top into a breakdown"),
 ("CURVE", "how a run moves: LIN, SOFT, EXP, S"),
 ("GAP", "silence just before the drop: OFF, half a beat, 1 or 2 beats"),
 ("Macros", "RISE, NOISE, REVERB, DELAY, KILL LOW, DIVE, GATE, SINK, PITCH, DRIVE"),
 ("Chain strip", "every stage with its live value"),
])}
"""})

    S.append({"title": "Controls reference", "sub": "what every control does, its range and default", "html": f"""
{img("rise/tour_knob.png", "<b>INTENSITY.</b> The pink arc is where the knob is; during a run the white outer ring fills bar by bar.", "w60")}
{ctl("INTENSITY", "0 to 100 %<br>default 0<br>automatable", "<p>The whole build-up on one knob. At 0 % the plugin passes the signal untouched (it nulls). As it rises every stage of the chain follows its own curve: the high-pass sweeps up faster near the top, the reverb and echoes open steadily, the noise comes in late. A fast pull back to zero is a drop: the high-pass and the low-pass let go within a few milliseconds and the dry comes back at once, so the first kick of the drop hits in full - and the tail keeps ringing (see TAIL).</p>", "Drag, or use the mouse wheel (Shift = fine). Double-click = 0. Click the number to type a value, then Enter (Esc leaves it). During a LAND run or AUDITION the knob shows the run; touching it cancels the run.")}
{ctl("Modes", "LIFTOFF / ORBIT / PULSAR / UNDERTOW / MELTDOWN<br>default LIFTOFF<br>automatable", "<p>A mode chooses the fixed parts of the chain (the Shepard speed-up in MELTDOWN) and loads the ten macros. <b>LIFTOFF</b>: a 24 dB/oct high-pass up to about 340 Hz and a 45 % reverb - the clean lift. <b>ORBIT</b>: LIFTOFF with a dotted-8th ping-pong echo and a breath of noise. <b>PULSAR</b>: the high-pass sweep to about 1.7 kHz, a tempo gate that speeds up, noise, a hint of the Shepard climb and the pitch rising 100 Hz. <b>UNDERTOW</b>: the opposite build-up - a low-pass closes the mix down to about 500 Hz under a huge reverb, and the drop opens the sky. <b>MELTDOWN</b>: everything at once, the pitch rising 200 Hz, the Shepard climb racing with the knob and heavy drive.</p>", "Move a macro and a dot appears on the mode card. Automating the mode from the host loads its macros too - your own, if you saved them for that card. Before 1.0.15 the modes were called SUNRISE, HORIZON, UPLIFT, DEEP DIVE and SUPERNOVA; older projects open on the same mode.")}
<div class="two">
{img("rise/melt.png", "<b>MELTDOWN at 100 %.</b> The sun is furious, red and melting.")}
{img("rise/under.png", "<b>UNDERTOW at 85 %.</b> The sea rises over it as DIVE closes the top.")}
</div>
{img("rise/presets.png", "<b>The preset list.</b> 17 factory presets, each a mode with its own macros, and below them the presets you saved.", "w60")}
{ctl("Presets", "17 factory + your own", "<p>The bar on top loads a whole preset: a mode and all ten macros. The factory set: CLEAN LIFT, AIR LIFT, FILTER CLIMB, WASH OUT, TAPE RISE (LIFTOFF); DUB THROW, PING PONG CLIMB, STARFIELD (ORBIT); STUTTER CLIMB, PSY GATE, TRANCE BUILD (PULSAR); SUBMARINE, MUFFLED ROOM, DEEP SPACE (UNDERTOW); BLAST OFF, WARP DRIVE, CHAOS (MELTDOWN). Click the name for the list; the arrows step through factory and user presets in turn. A <b>*</b> after the name means a macro was moved since it loaded.</p>", "Presets never touch INTENSITY, TAIL, LAND ON BAR, RISE / FALL or GAP.")}
{ctl("DICE", "the dice next to the arrows", "<p>Rolls a new build-up: a random mode and ten macros, inside limits that keep it musical - the reverb is always there, the dry always sinks a little, about one roll in five is an underwater (DIVE) build, and the loud stages (noise, the Shepard climb, the gate, the pitch) come in by chance. The bar shows it as DICE with its number. Like it? SAVE keeps it.</p>", "With AUDITION running you hear each roll in your music straight away.")}
{ctl("SAVE", "", "<p><b>SAVE PRESET</b> keeps the current mode and macros under a name of your choice (the same name again replaces it). <b>SAVE AS &lt;MODE&gt; MACROS</b> makes them the mode card's own: from then on a click on that card, or the host switching to that mode, loads your macros, and the bar shows the mode with (MINE).</p>", "Saved on your computer, for you, in every project and every DAW - not inside the song. On Windows: %APPDATA%\\RonePlugins\\RoneRise\\user-presets.json.")}
{ctl("Settings", "the gear", "<p><b>RESET TO FACTORY MACROS</b> brings every mode card back to its factory macros and reloads the card in use. <b>DELETE</b> removes the preset of yours that is loaded. Your saved presets stay until you delete them.</p>")}
{ctl("TAIL", "CUT / 1 / 2 / 3 / 4 bars<br>default CUT", "<p>What the reverb and echoes do when the knob falls. On <b>CUT</b> (the default) they stop dead on the drop - the hard stop. With 1-4 bars they are fed by the knob, but their level only follows it down slowly; once the knob reaches zero they hold for the first third of the TAIL, then fade out over the rest, in the room they had at the top. So a snap to zero and a quick fade both leave a real tail under the drop.</p>", "The switch sits in the header, next to BYPASS.")}
{ctl("BYPASS", "on / off", "<p>The untouched input with a 10 ms crossfade. While bypassed nothing runs, so the next un-bypass starts clean.</p>")}
{ctl("AUDITION", "on / off<br>not saved", "<p>The rise on a loop, so you can hear a preset in your own music without drawing any automation: it climbs over the LAND ON BAR length (4 bars when LAND ON BAR is MANUAL), lands on the bar line, lets the drop play for a bar after a run of 1-2 bars or two bars after a longer one, then starts again. Change the preset, roll the dice or move a macro while it runs and you hear it on the next pass; change LAND ON BAR or RISE / FALL and the loop starts again with the new setting.</p>", "It follows the song's bars, so start the song playing first. GO, moving the knob while a run climbs, stopping the song or closing the window ends it. It is only a listening aid - nothing about it is saved or automated.")}
<h3>ADVANCED</h3>
{img("rise/adv_panel.png", "<b>LAND ON BAR, RISE / FALL, CURVE, GAP, the ten macros and the chain strip.</b>", "w80")}
{ctl("LAND ON BAR", "MANUAL / 1 / 2 / 4 / 8 / 16 / 32 bars<br>default MANUAL", "<p>MANUAL: you move or automate INTENSITY yourself. With a bar count set, a <b>GO</b> button appears under the knob. GO starts the rise from the beginning of the bar you pressed it in and runs it to 100 % so that it lands exactly on the downbeat that many bars later; the outer ring of the knob counts the bars. The run lets go a hair before the bar line, so the drop is already whole ON the line, whatever buffer size your host uses. After the landing the plugin stays at zero until you touch the knob again. GO while running cancels, and so does moving the knob.</p>", "MIDI note C1 is GO too, and so is the Go parameter - automate a single step on it in the arrangement. 1 and 2 bars (since 1.2.0) are for fills and quick lifts. A run counts the bars it has played, so inside a loop shorter than the run it carries on through the loop and still lands.")}
{ctl("RISE / FALL", "RISE / FALL<br>default RISE<br>automatable", "<p><b>RISE</b>: GO climbs to the top and lands on the drop, as above. <b>FALL</b>: the other way, into a breakdown. GO arms it (the counter says NEXT BAR); on the next bar line the plugin jumps to 100 % and comes down to zero over the LAND ON BAR length, along the same CURVE mirrored, and rests there. A fall is a fade, so it never counts as a drop.</p>")}
{ctl("CURVE", "LIN / SOFT / EXP / S<br>default SOFT", "<p>How a run moves. LIN is a straight line; SOFT (default) stays a little lower and speeds up; EXP stays almost silent for the first half and puts everything in the last bars; S pushes once more at the very end.</p>")}
{ctl("GAP", "OFF / 1/2 / 1 / 2 beats<br>default OFF<br>automatable", "<p>Silence just before the drop of a LAND run (RISE): for the last half beat, beat or two beats the dry is muted, nothing new goes into the reverb and the echoes, the noise and the Shepard climb stop - and whatever was already ringing keeps ringing. With TAIL on 1-4 bars that is the classic breath before the drop, a wash with nothing under it. The dry comes back exactly on the bar line.</p>", "The scene on the sun says HOLD while the gap is on.")}
{ctl("RISE", "0 to 100 %", "<p>The Shepard climb: a barber-pole frequency shifter that seems to rise forever, cycling every two bars (faster with the knob in MELTDOWN).</p>")}
{ctl("NOISE", "0 to 100 %", "<p>White noise through a resonant high-pass that sweeps from 150 Hz to about 5 kHz as the knob rises. It also feeds the reverb, so it arrives as a washed riser, not as hiss.</p>")}
{ctl("REVERB", "0 to 100 %", "<p>How much reverb the chain adds. 0 turns the reverb off.</p>")}
{ctl("DELAY", "0 to 100 %", "<p>The dotted-8th ping-pong echo: its level, and how much it feeds back. 0 turns it off. On a mono track both sides of the ping-pong are folded in, so no echo goes missing.</p>")}
{ctl("KILL LOW", "0 to 100 %", "<p>How far the high-pass goes at the top of the knob: 35 % reaches about 340 Hz, 80 % about 1.7 kHz, 100 % about 3 kHz - the same in every mode (since 1.0.15; older projects were moved over so they sound exactly as before).</p>")}
{ctl("DIVE", "0 to 100 %", "<p>A 24 dB/oct low-pass that closes the top down to about 500 Hz as the knob rises - the underwater build.</p>")}
{ctl("GATE", "0 to 100 %", "<p>A tempo-synced gate on the dry signal. Its division halves as the knob rises: quarter notes, eighths, sixteenths, thirty-seconds - and it only changes on a beat, never in the middle of a chop. In a LAND run each division gets an equal quarter of the run. The amount sets how deep it chops.</p>")}
{ctl("SINK", "0 to 100 %<br>default 55 %", "<p>How far the dry signal sinks at 100 %: 55 % is about -7 dB. At 0 the dry never gets quieter and the plugin only adds.</p>")}
{ctl("PITCH", "0 to +200 Hz", "<p>Frequency-shifts the dry signal up as the knob rises, up to +200 Hz at 100 %. Every partial moves by the same number of hertz, so it reads as a tape-like lift with a metallic edge rather than a clean transposition.</p>")}
{ctl("DRIVE", "0 to 100 %<br>default 15 %", "<p>Soft saturation on the reverb, the echoes and the noise, growing with the knob. Each mode loads its own amount (LIFTOFF 15 %, ORBIT 20 %, PULSAR 40 %, UNDERTOW 30 %, MELTDOWN 80 %).</p>", "A macro since 1.2.0 - before, each mode had a fixed amount. Projects saved with an older version open with the amount their mode had, so they sound the same.")}
{ctl("Chain strip", "read-only", "<p>Every stage with what it is doing right now: HPF and DIVE in hertz, the GATE division, the PITCH shift, REVERB and DELAY levels, the NOISE level, the SHEPARD range and DRIVE.</p>")}
<p>Double-click a macro to return it to the loaded preset's value (or the mode card's). Click its number to type a value.</p>
"""})

    S.append({"title": "Step-by-step workflows", "sub": "recipes", "html": f"""
{recipe("The classic 8-bar build", "LIFTOFF, automated",
 steps([
  "Insert Rise on the mix bus (or on everything except the kick if you want the kick to keep pumping).",
  "LIFTOFF, TAIL 1 bar (the switch next to BYPASS).",
  "Draw INTENSITY from 0 at the start of the 8 bars to 100 % at the end, with a curve that stays low and rises late.",
  "Draw it straight back to 0 on the drop. The reverb rings under the first bar of the drop.",
 ]))}
{recipe("Find the build by ear", "AUDITION + DICE",
 steps([
  "Loop the bars before your drop in the host and press play.",
  "Press AUDITION. Set LAND ON BAR to the length you want (4 when it is MANUAL).",
  "Step through the presets with the arrows, or roll the dice, and listen to each pass.",
  "When one fits, press AUDITION again to stop, then SAVE it or use it with GO or your own automation.",
 ]))}
{recipe("Hands-free: LAND ON BAR", "Let the plugin do the automation",
 steps([
  "ADVANCED: LAND ON BAR 8, CURVE SOFT.",
  "Press GO (or send MIDI C1, or step the Go parameter) anywhere in the bar where the build starts.",
  "The rise runs from that bar and lands exactly eight bars later. The plugin stays at zero until you touch the knob.",
 ]))}
{recipe("The breath before the drop", "GAP",
 steps([
  "ADVANCED: LAND ON BAR 8, GAP 1 beat; TAIL 1 or 2 bars in the header.",
  "Press GO. On the last beat the music drops out and only the reverb and the echoes hang in the air.",
  "The drop comes in whole on the bar line.",
 ]))}
{recipe("Into the breakdown", "FALL",
 steps([
  "ADVANCED: LAND ON BAR 4, RISE / FALL on FALL, TAIL 2 bars.",
  "Press GO in the bar before the breakdown. The counter says NEXT BAR.",
  "On the bar line the plugin jumps to the top and falls back to zero over four bars - the energy drains out of the mix into the breakdown.",
 ]))}
{recipe("The underwater drop", "UNDERTOW",
 steps([
  "UNDERTOW (or the SUBMARINE preset), TAIL 2 bars.",
  "Rise over 8 bars: the mix closes down under a huge reverb.",
  "On the drop the full-range mix comes back all at once, with the wash still fading behind it.",
 ]))}
{recipe("Festival build", "PULSAR or MELTDOWN",
 steps([
  "PULSAR (or PSY GATE, TRANCE BUILD): the gate speeds up bar by bar, the pitch rises 100 Hz, noise sweeps in.",
  "For more, MELTDOWN (or BLAST OFF, CHAOS): +200 Hz, the Shepard climb racing, full noise.",
  "Turn PITCH down in ADVANCED if the lift sounds too metallic on your material, DRIVE down if it gets too dirty.",
 ]))}
{recipe("Your own build, every time", "SAVE",
 steps([
  "Load the preset or mode closest to what you want and move the macros in ADVANCED until it is yours.",
  "SAVE > type a name > SAVE PRESET. It is in the preset list of every project from now on.",
  "Or SAVE AS &lt;MODE&gt; MACROS to make it what that mode card loads; Settings > RESET TO FACTORY MACROS undoes it.",
 ]))}
"""})

    S.append({"title": "Tips, tricks and troubleshooting", "sub": "", "html": f"""
<h3>Tips</h3>
<ul>
<li><strong>Late is better.</strong> Most of the change should happen in the last two or three bars - use CURVE EXP for LAND runs, or draw the automation that way.</li>
<li><strong>Keep the kick out</strong> of the bus Rise sits on if you want the groove to keep driving through the build.</li>
<li><strong>SINK 0</strong> turns Rise into a pure add-on: nothing ever gets quieter.</li>
<li><strong>Stop means quiet.</strong> When the song stops, Rise lets its tail go and rests at zero until you press play or touch the knob, so a build left half-way never colours what you audition next.</li>
<li><strong>Presets are starting points.</strong> Every macro works in every mode: add DIVE to ORBIT, a GATE to LIFTOFF - then SAVE it.</li>
<li><strong>Mono tracks are fine.</strong> On a mono track Rise keeps every echo; on a mono-to-stereo insert the reverb and the ping-pong come out wide.</li>
</ul>
{G["logic_midi"]("RONE Rise", "GO on MIDI C1")}
<h3>Troubleshooting</h3>
{table(["Symptom", "Cause", "Fix"], [
 ["Nothing happens", "INTENSITY is at 0 (the plugin nulls there)", "Turn it up, press GO with LAND ON BAR set, or AUDITION with the song playing"],
 ["No GO button", "LAND ON BAR is MANUAL", "Set 1, 2, 4, 8, 16 or 32 bars in ADVANCED"],
 ["AUDITION plays nothing", "The song is not playing - AUDITION follows its bars", "Start the song, then press AUDITION"],
 ["AUDITION stopped by itself", "GO, moving the knob while a run climbs, stopping the song or closing the window ends it", "Press AUDITION again"],
 ["GO only lit up, nothing moved", "RISE / FALL is on FALL: it waits for the next bar line (NEXT BAR)", "Wait for the bar line, or set RISE"],
 ["The knob went back to 0 after a GO run", "That is the landing - the plugin stays at zero until the knob moves", "Move the knob to take over again"],
 ["The music cut out just before the drop", "GAP is on", "Set GAP to OFF in ADVANCED"],
 ["The build gets quieter", "SINK lowers the dry so the drop can jump", "Lower SINK in ADVANCED, 0 = never quieter"],
 ["The tail stops dead on the drop", "TAIL is CUT (the default since 1.0.15)", "Set TAIL to 1-4 bars with the switch next to BYPASS"],
 ["A mode card loads different macros than before", "You saved your own macros for that card", "Settings (the gear) > RESET TO FACTORY MACROS"],
 ["The effect switched off when I stopped the song", "REST ON STOP: when the host stops, Rise fades its tail and rests at zero, whatever the automation left INTENSITY at - it never keeps building or ringing into a stopped project", "Press play again, or touch the knob"],
 ["The lift sounds metallic", "PITCH is a frequency shift, not a transposition", "Lower PITCH in ADVANCED"],
 ["Old LAND ON BAR automation picks another length", "1 and 2 bars were added to LAND ON BAR in 1.2.0, so an automation lane drawn on it before may point at other steps (saved projects keep their setting)", "Redraw that lane"],
 ["The window is white in Ableton Live, the standalone works (Windows)", "Live's <strong>Auto-Scale Plug-In Window</strong> (seen with Windows display scaling such as 110 %)", "In Live's browser open Plug-Ins, right-click the plugin and untick Auto-Scale Plug-In Window, then delete the plugin from the track and insert it again"],
])}
"""})

    m["sections"] = S
    return m
