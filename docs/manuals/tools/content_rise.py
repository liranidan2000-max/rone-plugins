def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    m = {
        "id": "rise", "product": "RONE Rise", "eyebrow": "ONE KNOB. THE WHOLE BUILD-UP.",
        "title_html": "RONE <i>Rise</i>", "accent": "#E0449A", "version": "1.0.1",
        "tagline": "Turn one knob up over the phrase and the mix becomes a build-up - the lows leave, the reverb opens, the echoes climb. Snap it to zero and the drop hits, with the tail still spilling into it. Or press GO and let it land on the bar by itself.",
        "formats": ["VST3", "AU", "Standalone"], "vst3": "RONE Rise.vst3", "au": "RONE Rise.component", "exe": "RONE Rise.exe",
        "pdf": "RONE Rise - User Manual.pdf", "cover_img": "rise/tour_knob.png",
    }
    S = []

    S.append({"title": "Welcome to RONE Rise", "sub": "a build-up on one knob", "html": f"""
<p><strong>RONE Rise</strong> turns whatever you put through it into a build-up. One knob, <strong>INTENSITY</strong>, drives a whole chain the way you would automate it by hand over eight bars: a steep high-pass takes the low end away, a big reverb opens up, echoes and noise climb in, and in the bigger presets a gate speeds up, the pitch rides upward and a barber-pole shimmer climbs forever. Pull the knob back to zero on the downbeat and the drop lands - with the reverb and the echoes still ringing under it instead of dying on the one.</p>
<p>The sun on the right is the knob made visible: it sleeps below the horizon at zero, rises and grins as you turn it up, and pulses on every beat while the host plays.</p>
<div class="two">
<div>
<h3>Where it shines</h3>
<ul>
<li><strong>The last 8 bars before a drop</strong> - on the mix bus, the drum bus or a music group.</li>
<li><strong>Transitions</strong> - a four-bar lift between two sections without touching a single filter.</li>
<li><strong>Live and in the DJ set</strong> - LAND ON BAR runs the rise for you and lands it on the bar.</li>
<li><strong>The drop that breathes</strong> - TAIL keeps the wash under the first bar of the drop.</li>
</ul>
</div>
<div>
<h3>The 10-second version</h3>
{steps([
 "Insert Rise on your mix or drum bus.",
 "Pick SUNRISE.",
 "Automate INTENSITY from 0 to 100 over the last 8 bars.",
 "Snap it back to 0 on the drop."])}
</div></div>
{note("Why it gets quieter at the top", "A build-up has to leave room for the drop. As INTENSITY rises the dry signal sinks (about -7 dB at 100 %, set by SINK in ADVANCED) while the reverb and echoes rise, so the moment you pull the knob down the full mix comes back louder than it left. That jump is the drop.")}
"""})

    S.append(G["install_section"](m))

    S.append({"title": "Quick start", "sub": "your first rise", "html": f"""
{img("rise/tour.png", "<b>RONE Rise.</b> INTENSITY on the left, the sun on the right, the presets along the bottom.", "w80")}
{steps([
 "<strong>Insert</strong> Rise on the mix bus or a group bus. At 0 % it does nothing at all.",
 "<strong>Pick a preset</strong> along the bottom. <span class='pill'>SUNRISE</span> is the clean, classic lift.",
 "<strong>Automate <span class='pill'>INTENSITY</span></strong> from 0 to 100 % over the bars before the drop. A curve that stays low and rises late sounds most natural.",
 "<strong>Pull it back to 0 on the drop.</strong> The reverb and echoes keep ringing for a bar (TAIL) and then let go.",
 "<strong>Or let it run itself:</strong> open ADVANCED, set <span class='pill'>LAND ON BAR</span> to 8 and press <span class='pill'>GO</span>. The rise starts from the bar you pressed it in and lands exactly eight bars later.",
])}
{note("Right-click the knob", "Right-click any control for the host's own menu - in FL Studio that is where you create the automation clip or link it to a controller.")}
"""})

    S.append({"title": "Interface tour", "sub": "every element on screen", "html": f"""
{img("rise/tour.png", "A LAND ON BAR run in progress: bar 5 of 8, the outer ring counting the bars.")}
{legend([
 ("Header logo", "click to flip to the back panel (About, version, licence)"),
 ("BYPASS", "the untouched input, 10 ms crossfade"),
 ("INTENSITY", "the one knob, 0 to 100 %; the outer ring counts the bars of a LAND run"),
 ("GO", "appears when LAND ON BAR is set: runs the rise and lands it"),
 ("Bar counter", "which bar of the run you are in, and the beat"),
 ("Mood", "what the sun is feeling - NIGHT to HIGH NOON"),
 ("Presets", "five chains: SUNRISE, HORIZON, UPLIFT, DEEP DIVE, SUPERNOVA"),
 ("ADVANCED", "slides up LAND ON BAR, TAIL, the macros and the chain strip"),
 ("Resize grip", "drag to resize the window"),
])}
{img("rise/adv.png", "ADVANCED open over the lower half of the window.")}
{legend([
 ("LAND ON BAR", "MANUAL, or 4 / 8 / 16 / 32 bars for GO"),
 ("CURVE", "how a LAND run climbs: LIN, SOFT, EXP, S"),
 ("TAIL", "how long reverb and echoes ring after the knob falls"),
 ("Macros", "RISE, NOISE, SPACE, KILL LOW, DIVE, GATE, SINK, PITCH"),
 ("Chain strip", "every stage with its live value"),
])}
"""})

    S.append({"title": "Controls reference", "sub": "what every control does, its range and default", "html": f"""
{img("rise/tour_knob.png", "<b>INTENSITY.</b> The pink arc is where the knob is; during a LAND run the white outer ring fills bar by bar.", "w60")}
{ctl("INTENSITY", "0 to 100 %<br>default 0<br>automatable", "<p>The whole build-up on one knob. At 0 % the plugin passes the signal untouched (it nulls). As it rises every stage of the chain follows its own curve: the high-pass sweeps up faster near the top, the reverb and echoes open steadily, the noise comes in late. A fast pull back to zero is a drop: the tail keeps ringing (see TAIL).</p>", "Drag, or use the mouse wheel (Shift = fine). Double-click = 0. During a LAND run the knob shows the run; touching it cancels the run.")}
{ctl("Presets", "SUNRISE / HORIZON / UPLIFT / DEEP DIVE / SUPERNOVA<br>default SUNRISE<br>automatable", "<p>A preset chooses the fixed parts of the chain and loads the eight macros. <b>SUNRISE</b>: a 24 dB/oct high-pass up to about 340 Hz and a 45 % reverb - the clean lift. <b>HORIZON</b>: SUNRISE with a dotted-8th ping-pong echo and a breath of noise. <b>UPLIFT</b>: the full high-pass sweep to 3 kHz, a tempo gate that speeds up, noise, a hint of the Shepard climb and the pitch rising 100 Hz. <b>DEEP DIVE</b>: the opposite build-up - a low-pass closes the mix down to about 500 Hz under a huge reverb, and the drop opens the sky. <b>SUPERNOVA</b>: everything at once, the pitch rising 200 Hz and the Shepard climb racing with the knob.</p>", "Move a macro and a dot appears on the preset chip. Automating the preset from the host loads its macros too.")}
{ctl("BYPASS", "on / off", "<p>The untouched input with a 10 ms crossfade. While bypassed nothing runs, so the next un-bypass starts clean.</p>")}
<h3>ADVANCED</h3>
{img("rise/adv_panel.png", "<b>LAND ON BAR, CURVE, TAIL, the eight macros and the chain strip.</b>", "w80")}
{ctl("LAND ON BAR", "MANUAL / 4 / 8 / 16 / 32 bars<br>default MANUAL", "<p>MANUAL: you move or automate INTENSITY yourself. With a bar count set, a <b>GO</b> button appears under the knob. GO starts the rise from the beginning of the bar you pressed it in and runs it to 100 % so that it lands exactly on the downbeat that many bars later; the outer ring of the knob counts the bars. After the landing the plugin stays at zero until you touch the knob again. GO while running cancels, and so does moving the knob.</p>", "MIDI note C1 is GO too, and so is the Go parameter - automate a single step on it in the arrangement.")}
{ctl("CURVE", "LIN / SOFT / EXP / S<br>default SOFT", "<p>How a LAND run climbs. LIN is a straight line; SOFT (default) stays a little lower and speeds up; EXP stays almost silent for the first half and puts everything in the last bars; S pushes once more at the very end.</p>")}
{ctl("TAIL", "CUT / 1 / 2 / 3 / 4 bars<br>default 1 bar", "<p>What the reverb and echoes do when the knob falls. They are fed by the knob, but their level only follows it down slowly; once the knob reaches zero they hold for the first third of the TAIL, then fade out over the rest, in the room they had at the top. So a snap to zero and a quick fade both leave a real tail under the drop. <b>CUT</b> stops them dead on the drop.</p>")}
{ctl("RISE", "0 to 100 %", "<p>The Shepard climb: a barber-pole frequency shifter that seems to rise forever, cycling every two bars (faster with the knob in SUPERNOVA).</p>")}
{ctl("NOISE", "0 to 100 %", "<p>White noise through a resonant high-pass that sweeps from 150 Hz to about 5 kHz as the knob rises. It also feeds the reverb, so it arrives as a washed riser, not as hiss.</p>")}
{ctl("SPACE", "0 to 100 %", "<p>How much reverb and echo the chain adds.</p>")}
{ctl("KILL LOW", "0 to 100 %", "<p>How far the high-pass goes. Combined with the preset: in SUNRISE 100 % reaches about 340 Hz, in UPLIFT and SUPERNOVA about 3 kHz.</p>")}
{ctl("DIVE", "0 to 100 %", "<p>A 24 dB/oct low-pass that closes the top down to about 500 Hz as the knob rises - the underwater build.</p>")}
{ctl("GATE", "0 to 100 %", "<p>A tempo-synced gate on the dry signal. Its division halves as the knob rises: quarter notes, eighths, sixteenths, thirty-seconds. The amount sets how deep it chops.</p>")}
{ctl("SINK", "0 to 100 %<br>default 55 %", "<p>How far the dry signal sinks at 100 %: 55 % is about -7 dB. At 0 the dry never gets quieter and the plugin only adds.</p>")}
{ctl("PITCH", "0 to +200 Hz", "<p>Frequency-shifts the dry signal up as the knob rises, up to +200 Hz at 100 %. Every partial moves by the same number of hertz, so it reads as a tape-like lift with a metallic edge rather than a clean transposition.</p>")}
{ctl("Chain strip", "read-only", "<p>Every stage with what it is doing right now: HPF and DIVE in hertz, the GATE division, the PITCH shift, REVERB and DELAY levels, the NOISE level, the SHEPARD range and DRIVE.</p>")}
<p>Double-click a macro to return it to the preset's value.</p>
"""})

    S.append({"title": "Step-by-step workflows", "sub": "recipes", "html": f"""
{recipe("The classic 8-bar build", "SUNRISE, automated",
 steps([
  "Insert Rise on the mix bus (or on everything except the kick if you want the kick to keep pumping).",
  "SUNRISE, TAIL 1 bar.",
  "Draw INTENSITY from 0 at the start of the 8 bars to 100 % at the end, with a curve that stays low and rises late.",
  "Draw it straight back to 0 on the drop. The reverb rings under the first bar of the drop.",
 ]))}
{recipe("Hands-free: LAND ON BAR", "Let the plugin do the automation",
 steps([
  "ADVANCED: LAND ON BAR 8, CURVE SOFT.",
  "Press GO (or send MIDI C1, or step the Go parameter) anywhere in the bar where the build starts.",
  "The rise runs from that bar and lands exactly eight bars later. The plugin stays at zero until you touch the knob.",
 ]))}
{recipe("The underwater drop", "DEEP DIVE",
 steps([
  "DEEP DIVE, TAIL 2 bars.",
  "Rise over 8 bars: the mix closes down under a huge reverb.",
  "On the drop the full-range mix comes back all at once, with the wash still fading behind it.",
 ]))}
{recipe("Festival build", "UPLIFT or SUPERNOVA",
 steps([
  "UPLIFT: the gate speeds up bar by bar, the pitch rises 100 Hz, noise sweeps in.",
  "For more, SUPERNOVA: +200 Hz, the Shepard climb racing, full noise.",
  "Turn PITCH down in ADVANCED if the lift sounds too metallic on your material.",
 ]))}
"""})

    S.append({"title": "Tips, tricks and troubleshooting", "sub": "", "html": f"""
<h3>Tips</h3>
<ul>
<li><strong>Late is better.</strong> Most of the change should happen in the last two or three bars - use CURVE EXP for LAND runs, or draw the automation that way.</li>
<li><strong>Keep the kick out</strong> of the bus Rise sits on if you want the groove to keep driving through the build.</li>
<li><strong>SINK 0</strong> turns Rise into a pure add-on: nothing ever gets quieter.</li>
<li><strong>Stop means quiet.</strong> When the song stops, Rise lets its tail go and rests at zero until you press play or touch the knob, so a build left half-way never colours what you audition next.</li>
<li><strong>Presets are starting points.</strong> Every macro works in every preset: add DIVE to HORIZON, a GATE to SUNRISE.</li>
</ul>
<h3>Troubleshooting</h3>
{table(["Symptom", "Cause", "Fix"], [
 ["Nothing happens", "INTENSITY is at 0 (the plugin nulls there)", "Turn it up, or press GO with LAND ON BAR set"],
 ["No GO button", "LAND ON BAR is MANUAL", "Set 4, 8, 16 or 32 bars in ADVANCED"],
 ["The knob went back to 0 after a GO run", "That is the landing - the plugin stays at zero until the knob moves", "Move the knob to take over again"],
 ["The build gets quieter", "SINK lowers the dry so the drop can jump", "Lower SINK in ADVANCED, 0 = never quieter"],
 ["The tail stops dead on the drop", "TAIL is CUT", "Set TAIL to 1-4 bars"],
 ["The effect switched off when I stopped the song", "REST ON STOP: when the host stops, Rise fades its tail and rests at zero, whatever the automation left INTENSITY at - it never keeps building or ringing into a stopped project", "Press play again, or touch the knob"],
 ["The lift sounds metallic", "PITCH is a frequency shift, not a transposition", "Lower PITCH in ADVANCED"],
])}
"""})

    m["sections"] = S
    return m
