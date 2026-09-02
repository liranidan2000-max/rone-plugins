def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    m = {
        "id": "flanger", "product": "RONE Flanger", "eyebrow": "TRANSITION RISER FLANGER",
        "title_html": "RONE <i>Flanger</i>", "accent": "#FF3D6E", "version": "2.0",
        "tagline": "A flanger you play by hand. Sweep the comb yourself, or engage INFINITE for endless bar-locked rises and falls, with feedback, stereo offset and a rhythmic gate.",
        "formats": ["VST3", "Standalone"], "vst3": "RONE Flanger.vst3", "au": "", "exe": "RONE Flanger.exe",
        "pdf": "RONE Flanger - User Manual.pdf", "cover_img": "flanger/tour_knobs.png",
    }
    S = []

    S.append({"title": "Welcome to RONE Flanger", "sub": "a manual flanger for transitions", "html": f"""
<p>Most flangers wobble on their own with an LFO. <strong>RONE Flanger</strong> puts the sweep in your hand: the SWEEP knob <em>is</em> the delay time, so a rise or a fall is one automation line drawn exactly the length you want. When you want the movement to never end, <strong>INFINITE</strong> mode runs a barberpole sweep that rises (or falls) forever, locked to bars of your song's tempo. A rhythmic <strong>GATE</strong> chops the effect in and out on a note grid.</p>
<p>The effect itself is a classic single-delay comb with positive or negative feedback, a stereo offset and automatic gain compensation, so the tone stays where you put it while everything above it moves.</p>
<div class="two">
<div>
<h3>Where it shines</h3>
<ul>
<li><strong>Risers</strong> - a 4- or 8-bar INFINITE rise on a pad, a white-noise layer or the whole drum bus.</li>
<li><strong>Drop falls</strong> - a 2-bar FALL that lands on the downbeat.</li>
<li><strong>Jet flange</strong> - the classic manual sweep on drums or guitars, drawn by hand.</li>
<li><strong>Rhythmic texture</strong> - GATE at 1/16 turns a lead or a pad into a pulsing sequence.</li>
</ul>
</div>
<div>
<h3>The 10-second version</h3>
{steps([
 "Insert on the channel; MIX around 25-50 %.",
 "Drag SWEEP and listen to the notch move.",
 "Turn INFINITE on, choose RISE and 4 BAR.",
 "Add GATE if you want rhythm."])}
</div></div>
{note("Reading the display", "The comb display shows the frequency response you are hearing. NOTCH is the frequency of the first cancellation (1 divided by twice the delay time), COMB shows whether feedback is positive (+) or negative (-), and BPM is the tempo the bar-locked functions follow.")}
"""})

    S.append(G["install_section"](m))

    S.append({"title": "Quick start", "sub": "first sweep, first riser", "html": f"""
{img("flanger/tour.png", "<b>RONE Flanger</b> with the Default preset.", "w80")}
{steps([
 "<strong>Insert</strong> the plugin on a pad, a noise riser or a drum bus.",
 "<strong>Set MIX</strong> to about 50 % for a strong effect (25 % is the subtle default).",
 "<strong>Sweep by hand.</strong> Drag <span class='pill'>SWEEP</span> up and down. The NOTCH readout climbs as the delay shortens; the red marker on the display follows.",
 "<strong>Add feedback.</strong> Push FEEDBACK to 70-85 % for a resonant jet; try negative values for a hollow, phase-like tone.",
 "<strong>Engage INFINITE.</strong> Press the &infin; button, choose RISE and 4 BAR. The sweep now rises continuously, one full cycle every four bars of the host tempo, without ever reaching an end.",
 "<strong>Make it rhythmic.</strong> Turn GATE on and pick 1/8. The effect switches on and off on eighth notes; the LEDs show the pattern.",
 "<strong>Save it.</strong> Open the preset menu (click the preset name) and save a user preset, or step through the factory presets with &#9664; &#9654;.",
])}
{img("flanger/fx.png", "<b>INFINITE and GATE engaged.</b> Both rows light up in the accent colour when active.", "w80")}
"""})

    S.append({"title": "Interface tour", "sub": "every element on screen", "html": f"""
{img("flanger/tour.png", "Main view.")}
{legend([
 ("Header logo", "click to flip to the back panel (About, version, licence)"),
 ("Preset browser", "previous / next factory or user preset; click the name for the menu"),
 ("BYPASS", "hard bypass"),
 ("Comb display", "live frequency response with the notch position"),
 ("COMB +/-", "feedback polarity readout"),
 ("NOTCH", "frequency of the first notch"),
 ("BPM", "host tempo (or the standalone tempo)"),
 ("FEEDBACK", "-100 to +100 %"),
 ("SWEEP", "the delay time, 10 ms to 0.1 ms"),
 ("MIX", "dry / wet balance"),
 ("AUTO GAIN", "keeps the level steady across feedback and mix changes"),
 ("INFINITE on/off", "barberpole (endless) sweep"),
 ("RISE / FALL", "direction of the endless sweep"),
 ("Sweep length", "8, 4, 2, 1 or 1/2 bar per cycle"),
 ("GATE on/off", "rhythmic bypass"),
 ("Gate rate", "1 bar to 1/32"),
 ("Gate LEDs", "position inside the gate pattern"),
 ("ADVANCED", "opens the drawer with STEREO and OUTPUT"),
 ("Resize grip", "drag to resize the window"),
])}
{img("flanger/adv.png", "<b>ADVANCED drawer.</b> 1 STEREO offset &middot; 2 OUTPUT gain.", "w80")}
"""})

    S.append({"title": "Controls reference", "sub": "what every control does, its range and default", "html": f"""
<h3>Main knobs</h3>
{img("flanger/tour_knobs.png", "<b>FEEDBACK, SWEEP, MIX and AUTO GAIN.</b>", "w80")}
{ctl("SWEEP", "10 ms to 0.1 ms<br>default 10 ms (fully left)<br>skewed for precision at short times", "<p>The delay time of the comb, and the heart of the plugin. Turning clockwise <em>shortens</em> the delay and moves the notches up in frequency - that is the 'rise'. The knob is scaled so that the short, dramatic end of the range gets most of the travel. In INFINITE mode the knob is driven automatically and shows the sweep position.</p>", "Automate SWEEP with a curve, not a straight line: a rise that accelerates at the end sounds much more like a real jet.")}
{ctl("FEEDBACK", "-100 to +100 %<br>default +50 %", "<p>How much of the delayed signal is fed back into the delay. Positive values emphasise the peaks between the notches - the resonant 'tube' sound. Negative values invert the comb, which sounds hollower and more like a phaser. Near the extremes the comb rings; AUTO GAIN keeps that in check.</p>", "The COMB chip shows + or - so you always know which side you are on.")}
{ctl("MIX", "0 to 100 %<br>default 25 %", "<p>Dry / wet balance. 50 % is the deepest comb (equal dry and delayed signal). Above 50 % the notches fill in again and the effect becomes more of a delayed, resonant copy.</p>")}
{ctl("AUTO GAIN", "on / off<br>default off", "<p>Compensates the level changes caused by FEEDBACK and MIX so that sweeping does not also mean pumping. Leave it on while designing, decide at the end whether you prefer the raw version.</p>")}
<h3>INFINITE - barberpole sweep</h3>
{img("flanger/tour_infinite.png", "<b>The INFINITE row.</b>", "w80")}
{ctl("&infin; INFINITE", "on / off", "<p>Turns on the endless sweep. Two comb voices are cross-faded so that when one reaches the end of its travel the other has already taken over at the start - the ear hears a sweep that never stops rising (or falling), like a barberpole. SWEEP is driven automatically while INFINITE is on.</p>")}
{ctl("RISE / FALL", "default RISE", "<p>Direction of the endless sweep. RISE for builds, FALL for post-drop releases and comedowns.</p>")}
{ctl("8 BAR / 4 BAR / 2 BAR / 1 BAR / 1/2", "default 4 BAR", "<p>How long one full cycle takes, in bars of the host tempo. Because the cycle is bar-locked, a rise that starts on bar 1 of a 4-bar phrase peaks exactly when the phrase ends. Use 1/2 for a fast, nervous shimmer.</p>", "Match the value to the length of your build: an 8-bar build gets 8 BAR, a 2-bar fill gets 2 BAR.")}
<h3>GATE - rhythmic bypass</h3>
{img("flanger/tour_gate.png", "<b>The GATE row.</b>", "w80")}
{ctl("&#9211; GATE", "on / off", "<p>Switches the effect in and out on a note grid, with a short crossfade so nothing clicks. The dry signal is untouched while the gate is closed, so the result is a rhythmic 'flange pulse' rather than a volume gate.</p>")}
{ctl("1 BAR &middot; 1/2 &middot; 1/4 &middot; 1/8 &middot; 1/16 &middot; 1/32", "default 1/8", "<p>Gate rate. The LED row shows where you are in the pattern; the gate follows the host transport so it stays aligned to the bar.</p>")}
<h3>ADVANCED drawer</h3>
{img("flanger/adv_drawer.png", "<b>STEREO and OUTPUT.</b>", "w60")}
{ctl("STEREO", "0 to 3 ms<br>default 0", "<p>Offsets the delay time of the right channel from the left. Even 0.5 ms puts the notches at different frequencies in each channel, which reads as width; 2-3 ms gives a wide, swirling image.</p>", "Check mono compatibility above 1.5 ms - the two combs partly cancel when summed.")}
{ctl("OUTPUT", "-12 to +12 dB<br>default 0 dB", "<p>Output trim after the effect, for matching levels between presets or against the bypassed signal.</p>")}
<h3>Presets</h3>
<p>Fifteen factory presets are built in, in four groups: <strong>Risers</strong> (Riser 4 Bars, Riser 8 Bars, Drop Fall 2 Bars), <strong>Classic</strong> (Classic Jet, Subtle Shimmer, Deep Sweep, Metallic), <strong>Character</strong> (Wide Stereo, Negative Flange, Hollow Tube, Resonant Peak) and <strong>Gated</strong> (Gate Pulse 1/8, Gate Chop 1/16, Slow Gate), plus Default. Click the preset name to open the menu, save your own presets there, and delete user presets you no longer need. User presets are stored in your user folder and survive updates.</p>
"""})

    S.append({"title": "Step-by-step workflows", "sub": "recipes", "html": f"""
{recipe("8-bar riser on a noise layer", "Builds, intros",
 steps([
  "Put white noise or a filtered noise loop on its own channel and insert Flanger. Load <em>Riser 8 Bars</em> (or: MIX 50 %, FEEDBACK 85 %, STEREO 0.5 ms, AUTO GAIN on).",
  "Turn INFINITE on, RISE, 8 BAR. Start the noise at the beginning of the 8-bar build.",
  "Automate MIX from 25 % to 60 % across the build so the effect intensifies as the sweep rises.",
  "On the drop, mute the channel or automate MIX to 0.",
 ]))}
{recipe("Drop fall that lands on the one", "Post-drop release",
 steps([
  "Insert on the lead or the drum bus. INFINITE on, FALL, 2 BAR.",
  "Turn INFINITE on exactly two bars before the section change; turn it off on the downbeat. Because the cycle is bar-locked, the fall completes exactly on the one.",
 ]))}
{recipe("Hand-played jet flange on drums", "Rock, breakbeat, drum edits",
 steps([
  "Load <em>Classic Jet</em>. INFINITE off.",
  "Draw SWEEP automation: hold at 10 ms for a bar, sweep to 0.3 ms over the next two bars with an exponential curve, then back down over one bar.",
  "Add STEREO 1 ms for width and turn AUTO GAIN on so the drums do not jump in level.",
 ]))}
{recipe("Pulsing pad", "Trance, techno, ambient",
 steps([
  "Load <em>Gate Chop 1/16</em> on a sustained pad.",
  "Set INFINITE on, RISE, 4 BAR so the pulses also drift upward.",
  "Adjust MIX to taste - around 40 % keeps the pad recognisable underneath the rhythm.",
 ]))}
"""})

    S.append({"title": "Recommendations", "sub": "how to get the best out of it", "html": f"""
<div class="dos">
<div><h4>Do</h4><ul>
<li>Use INFINITE for anything that has to line up with bars; use hand-drawn SWEEP automation for one-off jet sweeps.</li>
<li>Keep AUTO GAIN on while you tweak FEEDBACK; decide at the end.</li>
<li>Use negative FEEDBACK on bright sources (hats, noise) - it thins them out instead of making them ring.</li>
<li>Put Flanger before a reverb or delay when the sweep should be part of the space, after it when the whole space should sweep.</li>
<li>Save your go-to settings as user presets; they are stored per machine and survive updates.</li>
</ul></div>
<div><h4>Avoid</h4><ul>
<li>FEEDBACK above 90 % with MIX at 50 % on bass-heavy material; the comb peaks at low frequencies can overload. Lower MIX or add a high-pass before the plugin.</li>
<li>Large STEREO offsets on material that must stay mono compatible.</li>
<li>Using GATE at 1/32 with a slow tempo; the gate becomes a buzz. 1/16 is the practical limit below about 100 BPM.</li>
</ul></div>
</div>
<h3>Sound tips</h3>
<ul>
<li>The notch frequency is 1/(2 &times; delay). At 10 ms the first notch sits at 50 Hz (barely audible as a notch, but the comb above it is dense); at 0.1 ms it sits at 5 kHz with a single wide notch. That is why the top end of SWEEP sounds so different from the bottom.</li>
<li>INFINITE + GATE + FALL on a snare bus is an instant "reverse-feeling" fill without any reversed audio.</li>
<li>Two instances in series, one at 8 BAR RISE and one at 2 BAR FALL, create a complex, evolving movement for long ambient sections.</li>
</ul>
"""})

    S.append(G["conventions_section"](m, note("Standalone tempo", "The standalone app cannot read a DAW clock; INFINITE and GATE follow the BPM shown in the display, which you can set from the preset menu's tempo field.")))

    S.append(G["support_section"](m, [
        ("INFINITE does not move", "The sweep follows the host transport - press play in the DAW. In the standalone app it runs continuously at the displayed BPM."),
        ("GATE is out of time", "The gate is aligned to the host's bar position, so it needs a running transport. If your DAW does not send position data (rare), use INFINITE without GATE or gate with automation."),
        ("The effect pumps in level", "Turn AUTO GAIN on, or reduce FEEDBACK. Very high feedback with MIX near 50 % naturally changes loudness as the notches sweep through the energy of your source."),
        ("No sound at the top of the sweep with negative feedback", "That is the comb cancelling most of a mono source. Add STEREO offset or reduce MIX below 50 %."),
    ]))

    m["sections"] = S
    return m
