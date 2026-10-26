def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    m = {
        "id": "sidechain", "product": "RONE Sidechain", "eyebrow": "DRAWN SIDECHAIN · PHASE · SPACE",
        "title_html": "RONE <i>Sidechain</i>", "accent": "#7B8CFF", "version": "1.0.0",
        "tagline": "Draw the duck on your bass and let every kick restart it - silent at the kick's first sample, open exactly where you drew it. Then turn the bass's sub to meet the kick, and cut only where the two really collide.",
        "formats": ["VST3", "AU", "Standalone"], "vst3": "RONE Sidechain.vst3", "au": "RONE Sidechain.component", "exe": "RONE Sidechain.exe",
        "pdf": "RONE Sidechain - User Manual.pdf", "cover_img": "sidechain/main.png",
        "channels": "Mono or stereo in / out, plus a mono or stereo sidechain input for the kick",
        "latency": "About 29 ms, constant, reported to the host (compensated automatically)",
    }
    S = []

    S.append({"title": "Welcome to RONE Sidechain", "sub": "a drawn duck, a locked sub, room for the kick", "html": f"""
<p><strong>RONE Sidechain</strong> goes on your bass and makes room for the kick in three ways, one page each.</p>
<ul>
<li><strong>SIDECHAIN</strong> - you draw the shape of the duck. Every kick restarts the curve, and the bass is <strong>exactly silent at the kick's first sample</strong>: no leftover level under the kick, no tail and no click.</li>
<li><strong>PHASE</strong> - turns only the bass's sub so that, where the kick's tail hands over to the bass, the two add up instead of cancelling.</li>
<li><strong>SPACE</strong> - small dynamic bells that cut the bass only at the frequencies and moments where the kick and the bass really collide. Everything else passes untouched.</li>
</ul>
<p>The kick can come from a sidechain route, from a kick sample you drop on the plugin (it plays silently on the host's beat), or from nothing at all - then the curve runs on the grid.</p>
<div class="two">
<div>
<h3>Where it shines</h3>
<ul>
<li><strong>Rolling psytrance and techno basslines</strong> - the kick's 16th closed, the bass back exactly on the next note.</li>
<li><strong>House pump</strong> - a slow, musical curve over the whole beat.</li>
<li><strong>808s and sub basses</strong> - duck only the sub and keep the bass's top.</li>
<li><strong>Muddy low ends</strong> - PHASE and SPACE clean up the handover without an EQ that stays on.</li>
</ul>
</div>
<div>
<h3>The 10-second version</h3>
{steps([
 "Insert RONE Sidechain on the bass.",
 "Route the kick into its sidechain input - or drop a kick sample on it.",
 "Pick a preset for your genre.",
 "Shape the curve until the bass breathes with the kick."])}
</div></div>
"""})

    inst = G["install_section"](m)
    # this plugin's own licence screen reads "Sign in to unlock RONE Sidechain"
    inst["html"] = inst["html"].replace("shows a <em>RONE Bundle License Required</em> screen", "shows the <em>Sign in to unlock RONE Sidechain</em> screen")
    S.append(inst)

    S.append({"title": "Quick start", "sub": "your first sidechain", "html": f"""
{img("sidechain/main.png", "<b>RONE Sidechain</b> on the SIDECHAIN page: the kick on top, the bass under it with the curve drawn over it.", "w80")}
{steps([
 "<strong>Insert</strong> RONE Sidechain on the bass track (or the bass bus).",
 "<strong>Give it the kick.</strong> Route the kick track into the plugin's sidechain input - click the KICK chip in the header for the steps in your DAW - or drag a kick sample onto the plugin. The chip shows where the kick comes from.",
 "<strong>Pick a preset</strong> from <span class='pill'>PRESET</span> - they are sorted by genre. <span class='pill'>PSYTRANCE &middot; ROLLING</span> is the opening setting.",
 "<strong>Press play.</strong> The kick draws in the top lane, the bass under it, ducked by the curve. The readout above the lanes tells you what is left of the bass <em>under the kick</em> and when it is fully open again.",
 "<strong>Shape it:</strong> pick another shape, drag a point or a line, set <span class='pill'>RATE</span> to the length of your bass pattern and <span class='pill'>DEPTH</span> to how far it closes.",
 "<strong>Optional:</strong> switch to the PHASE and SPACE pages. Both start working as soon as there is a kick; AUTO turns the sub by itself.",
])}
{note("No sidechain in your DAW?", "Drop your kick sample on the plugin (or press LOAD KICK&hellip;). It plays silently on the host's beat, so the curve follows the beat and PHASE and SPACE know your kick - no routing at all.")}
"""})

    S.append({"title": "Interface tour", "sub": "every element on screen", "html": f"""
{img("sidechain/tour.png", "The SIDECHAIN page in LANES view.")}
{legend([
 ("Header logo", "click to flip to the back panel (About, version, licence)"),
 ("PRESET", "full settings by genre and your own presets; shows CUSTOM once you change something"),
 ("RATE", "the curve's length: 1/16, 1/8, 1/4, 1/2 or 1 BAR, synced to the host tempo"),
 ("KICK chip", "where the kick comes from; click it for the routing steps and the input meters"),
 ("BYPASS", "lit = the plugin is out of the path"),
 ("Pages", "SIDECHAIN, PHASE and SPACE; the plugin remembers the last one"),
 ("UNDO / REDO", "step back and forward through your edits - also Ctrl+Z and Ctrl+Shift+Z (Ctrl+Y)"),
 ("Kick lane", "the kick as it arrives, one cycle - for looking, not drawing"),
 ("Bass lane + curve", "the bass before (faint) and after the duck, with the curve to draw on"),
 ("FREE / QUANTIZE", "free drawing, or points and lines snapped to the grid"),
 ("GRID", "draws the rate's musical lines (16ths, beats) clearly over the lanes"),
 ("Readout", "UNDER THE KICK (what is left of the bass there), OPEN AT (when it is fully back) and the DUCK range"),
 ("LANES / OVERLAY", "the kick above the bass, or both on one row with a taller box to draw in"),
 ("SHAPES", "the factory curves and your own; drag a tile to move it, its x deletes it"),
 ("+ SAVE", "keeps the current curve as your own shape"),
 ("DEPTH", "how far the curve closes; 100 % = exactly silent"),
 ("CLICK", "the fade before the kick that keeps the cut from clicking"),
 ("DUCK", "FULL, SUB or BAND - where in the spectrum the duck works"),
 ("LOAD KICK / DROP", "load a kick sample, or drop one here or anywhere on the plugin"),
 ("Resize grip", "drag to resize the window"),
])}

"""})

    S.append({"title": "The SIDECHAIN page", "sub": "the curve, RATE, DEPTH, CLICK, DUCK", "html": f"""
<h3>How the curve works</h3>
<p>The curve is the bass's level over one cycle: at the bottom the bass is silent, at the top it plays at full level. <strong>Every kick restarts the curve</strong> from its left edge, so the duck always lines up with the real kick - whatever pattern you program, fills and offbeat kicks included. The first point, the diamond on the left, is <strong>the kick's point</strong>: it always sits at silence, and it is the kick's first sample.</p>
<p>That is the promise of the plugin: at the kick's first sample the bass is <strong>exactly silent</strong> (at DEPTH 100 %). To get there without a click, the bass fades out just <em>before</em> the kick. That fade is <strong>CLICK</strong>, and it is complete at the kick's first sample. Because the plugin works a little ahead of time (the latency below), it knows the kick is coming before it arrives.</p>
{note("Nothing left under the kick", "A point within 5 % of silence snaps to silence, and one within 5 % of full snaps to full - a floor that looks like silence but is not is how a bass leaks under a kick. The readout UNDER THE KICK shows what really remains there; it turns red when the bass is louder than -50 dB under the kick.")}
<h3>Drawing the curve</h3>
{img("sidechain/draw.png", "The STEP shape: the kick's 16th closed, the first bass note at half, full from the second.")}
{legend([
 ("Kick's point", "fixed at silence on the kick's first sample"),
 ("Step", "a vertical line: drag it to move it in time, or up and down with the levels either side"),
 ("Level", "a flat line: drag it up or down (and its ends in time) as one piece"),
 ("Point", "drag it anywhere between its neighbours; the tooltip shows its time in ms and its level in dB"),
 ("Playhead", "where the cycle is now while the host plays"),
])}
{img("sidechain/bend.png", "The CLASSIC shape: a slope with its bend circle.")}
{legend([
 ("Bend circle", "the circle in the middle of every slope - drag it up or down to bend the line"),
 ("Point", "the end of the slope"),
])}
{table(["Gesture", "What it does"], [
 ("Drag a point", "Moves it, in time and level. The last point always stays at the end of the cycle."),
 ("Drag a flat level or a vertical step", "Moves the whole line. A step carries the flat levels on either side with it, so nothing tilts."),
 ("Drag the circle in the middle of a slope", "Bends the slope - from a fast rise to a slow one."),
 ("Double-click the bass lane", "Adds a point there."),
 ("Double-click a point", "Removes it."),
 ("FREE / QUANTIZE", "QUANTIZE snaps points, moved lines and new points to the grid: 16ths of the cycle and 5 % levels. FREE puts them exactly where you drag."),
 ("Hold <span class='kbd'>Shift</span>", "Does the opposite of FREE / QUANTIZE for that one drag or double-click."),
 ("GRID", "On by default and remembered. Draws the musical lines of the RATE clearly, so you see where you draw: 16ths at 1/4 and longer, with the beats stronger from 1/2; 32nds at 1/8 and 64ths at 1/16. QUANTIZE snaps to a finer grid that holds every one of these lines."),
])}
<p>The tag in the top-left corner of the view shows the current RATE and the host tempo (for example <code>1/4 &middot; 140 BPM</code>). Along the bottom of the view the grid is marked in milliseconds, and a white playhead runs along the cycle while the host plays.</p>
<p>The kick lane above is for looking only. In <strong>OVERLAY</strong> view the kick is drawn in grey on the bass's row and the whole box is yours to draw in.</p>
<div class="two"><div>{img("sidechain/overlay.png", "OVERLAY: the kick and the bass on one row, the curve over both.", "")}</div><div><h3>Shapes</h3>
<p>The <strong>SHAPES</strong> row starts with thirteen factory curves - STEP, ROLL, CLASSIC, TIGHT, HOLD, SNAP, PUMP, SOFT, LATE, SWELL, DOUBLE, BOUNCE and GENTLE (hover a tile to read its name). Click one to load it, then edit it freely.</p></div></div>
{img("sidechain/hover.png", "The SHAPES row with the mouse over a tile: its x appears at the top-right corner.")}
<ul>
<li><strong>+ SAVE</strong> adds the current curve to the end of the row as <em>MY 1</em>, <em>MY 2</em> and so on - up to 16 of your own; the oldest goes when you save a 17th.</li>
<li><strong>Drag a tile</strong> left or right to move it in the row; a click still just loads it.</li>
<li><strong>The x</strong> that appears when you hover a tile deletes it.</li>
<li><strong>Right-click the row</strong> &rarr; <strong>RESTORE FACTORY SHAPES</strong> brings back any factory shape you deleted, so none is ever lost for good.</li>
</ul>
<p>The row is remembered on this computer, for every instance of RONE Sidechain and every project. Changes to it can be undone like any other edit.</p>
<h3>The controls</h3>
{ctl("RATE", "1/16, 1/8, 1/4, 1/2, 1 BAR<br>default 1/4<br>automatable", "<p>How long one cycle of the curve is, synced to the host tempo. 1/4 = one beat, the usual four-on-the-floor kick. A kick that comes before the cycle is over restarts it there.</p>")}
{ctl("DEPTH", "0 to 100 %<br>default 100 %<br>automatable", "<p>How far the curve closes. At 100 % the bottom of the curve is silence; at 50 % the bass never drops below half its level.</p>")}
{ctl("CLICK", "0.5 to 20 ms<br>default 2.0 ms<br>automatable", "<p>The fade <em>before</em> the kick that keeps the cut from clicking. It is complete at the kick's first sample, so the bass is silent when the kick starts however long you set it. Shorter = a tighter cut; longer = softer on low, sustained basses.</p>")}
{ctl("DUCK", "FULL / SUB / BAND<br>default FULL<br>automatable", "<p>Where in the spectrum the duck works - <strong>the rest passes untouched</strong>.</p><p><b>FULL</b> ducks the whole bass. <b>SUB</b> ducks only its sub, below SPLIT (set on the PHASE page). <b>BAND</b> ducks only between the two handles of the strip under the switch: drag a handle and the switch goes to BAND; double-click the strip for FULL. The bands are split linear-phase, so the part that is not ducked passes exactly as it came.</p>", "BAND's handles run from 20 Hz to 20 kHz (default 20 Hz - 200 Hz); the view's readout shows the range you set.")}
{img("sidechain/band_duck.png", "DUCK in BAND: the strip shows the range the duck works in.", "w60")}
"""})

    S.append({"title": "The kick: routed, dropped or the grid", "sub": "where the kick comes from", "html": f"""
<p>The <strong>KICK chip</strong> in the header always shows what the curve is following:</p>
{table(["Chip", "What it means"], [
 ("KICK IN &middot; ROUTED", "A kick arrives on the sidechain input. Every hit restarts the curve, whatever the pattern."),
 ("SAMPLE &middot; <i>name</i>", "You dropped or loaded a kick sample. It plays silently on the host's beat and restarts the curve there."),
 ("LAST KICK &middot; ON THE BEAT", "The routed kick stopped arriving. The last clean kick heard plays on, silently, on every beat."),
 ("GRID &middot; NO KICK", "No kick at all. The curve restarts on its own at every RATE. PHASE and SPACE wait for a kick."),
])}
<h3>Routing the kick (sidechain input)</h3>
<p>Click the KICK chip to open <strong>SEND THE KICK TO RONE SIDECHAIN</strong>. It shows the steps for the DAW you are in, and two meters: <strong>IN</strong> (the bass on the plugin's input) and <strong>SC</strong> (the kick on the sidechain input). When the routing is right, the SC meter moves on every kick. <strong>OTHER DAWS</strong> lists the steps for the others.</p>
{img("sidechain/setup.png", "The SETUP popover, opened from the KICK chip.", "w60")}
{legend([
 ("KICK chip", "click to open and close the popover"),
 ("IN / SC meters", "the bass on the input, the kick on the sidechain"),
 ("Your DAW", "the routing steps for the host the plugin is running in"),
 ("OTHER DAWS", "the steps for FL Studio, Ableton, Cubase, Logic, Studio One, Bitwig and REAPER"),
])}
<h4>FL Studio</h4>
{steps([
 "In the Mixer, select the <strong>kick</strong> track, then right-click the arrow at the bottom of the <strong>bass</strong> track &rarr; <strong>Sidechain to this track</strong>.",
 "In RONE Sidechain's wrapper: <strong>&#9881; &rarr; Processing &rarr; Connections</strong> - set the <strong>sidechain input</strong> to the kick track.",
 "The SC meter in the popover now moves on every kick.",
])}
{table(["DAW", "Routing"], [
 ("Ableton Live", "Open the RONE Sidechain device I/O (&#9656;) &rarr; <b>Sidechain</b> on &rarr; <b>Audio From</b> = the kick track."),
 ("Cubase", "Turn on <b>Side-Chain</b> in the RONE Sidechain header; kick track &rarr; Sends &rarr; <b>RONE Sidechain - Side-Chain</b>."),
 ("Logic Pro", "RONE Sidechain header &rarr; <b>Side Chain</b> = the kick track."),
 ("Studio One", "RONE Sidechain header &rarr; <b>Side-Chain</b> on; kick track &rarr; Sends &rarr; RONE Sidechain."),
 ("Bitwig", "RONE Sidechain device &rarr; <b>Sidechain input</b> = the kick track."),
 ("REAPER", "Kick track &rarr; Route: send to the bass track on channels <b>3/4</b>; RONE Sidechain pin connector: sidechain = 3/4."),
])}
{note("A sidechain that carries the bass is ignored", "FL Studio, before a sidechain route is made, feeds the bass's own signal onto the plugin's sidechain bus. RONE Sidechain recognises that - even when it arrives at another level or a few milliseconds late - and does not treat the bass as a kick: the curve follows the beat until you route the real kick.")}
<h3>Dropping a kick sample</h3>
<p>No routing at all? Drag a kick file (WAV, AIFF, MP3 or FLAC) from your browser or the desktop and drop it anywhere on the plugin - the page shows <em>DROP YOUR KICK</em> - or press <strong>LOAD KICK&hellip;</strong>. The sample plays <strong>silently</strong> on the host's beat: the curve follows the beat, and PHASE and SPACE know your kick's sound. It is saved with the project.</p>
<p>A dropped kick <strong>always wins over the routing</strong>. The button turns into <strong>&#10005; CLEAR KICK</strong>; press it to hand control back to the routed kick.</p>
<h3>Rendering the bass alone</h3>
<p>When the kick stops arriving - you solo the bass, or render the bass track on its own - the <strong>last kick heard plays on every beat, silently</strong>. The curve, PHASE and SPACE stay exactly as they were, so a render of the bass sounds like the bass in the mix. The remembered kick is saved with the project.</p>
<h3>The standalone</h3>
<p>The standalone app has no sidechain input: the curve follows its own clock (GRID). Drop a kick sample to give PHASE and SPACE a kick.</p>
"""})

    S.append({"title": "The PHASE page", "sub": "the sub turned to meet the kick", "html": f"""
<p>Where the kick's tail hands over to the bass, the two low ends overlap. If they are out of phase they cancel, and the low end gets thinner exactly where it should be strongest. PHASE turns <strong>only the bass's sub</strong> (below SPLIT) so the two add up. The turn is a linear-phase rotation: the bass's attack is not smeared and nothing above SPLIT moves.</p>
{img("sidechain/phase.png", "The PHASE page: the kick's low band (grey) and the bass's sub as you hear it (periwinkle) at the handover.", "w80")}
{legend([
 ("Handover scope", "the first 160 ms of the cycle: the kick's low band in grey, the bass's sub as heard in periwinkle - locked, they read as one continuous wave"),
 ("ON / OFF", "switch PHASE off to hear what the turn does; the duck and SPACE stay"),
 ("PHASE", "the angle the sub is turned by, -180&deg; to +180&deg;"),
 ("SPLIT", "where the sub ends: what PHASE turns and what DUCK SUB ducks"),
 ("MODE / &Oslash; FLIP", "AUTO or MANUAL; &Oslash; FLIP turns the sub's polarity (+180&deg;)"),
 ("Status", "LOCKED, TURNING TO &hellip;, NOTHING TO TURN, PHASE OFF"),
 ("Readout", "the bass's note, kick + bass together in dB, and what they would be turned"),
])}
{ctl("ON / OFF", "default ON<br>automatable", "<p>Off = the sub as it came. Use it to compare.</p>")}
{ctl("MODE", "AUTO / MANUAL<br>default AUTO<br>automatable", "<p><b>AUTO</b> measures where the kick's tail and the bass add up and turns the sub there by itself. It turns <strong>only while the duck is closed</strong>, so you never hear it moving. In AUTO the PHASE knob shows the angle and is not yours to turn. <b>MANUAL</b> leaves PHASE to you; the status line tells you where AUTO would turn it and how much louder the low end would be.</p>")}
{ctl("PHASE", "-180&deg; to +180&deg;<br>default 0&deg;<br>automatable", "<p>How far the sub is turned against the kick. Set by AUTO, or by hand in MANUAL.</p>")}
{ctl("SPLIT", "50 to 200 Hz<br>default 90 Hz<br>automatable", "<p>Where the sub ends. Everything below it is what PHASE turns and what DUCK = SUB ducks; everything above passes as it was.</p>")}
{ctl("&Oslash; FLIP", "on / off<br>automatable", "<p>Turns the sub's polarity around (+180&deg; on top of PHASE).</p>")}
{note("PHASE needs a kick", "With GRID &middot; NO KICK the page reads ROUTE THE KICK OR DROP A SAMPLE - there is nothing to turn towards. If the kick and the bass never overlap, the status says NOTHING TO TURN.")}
"""})

    S.append({"title": "The SPACE page", "sub": "bells only where kick and bass meet", "html": f"""
<p>A static EQ cut on the bass takes the low end away all the time. SPACE works like a dynamic EQ instead: up to 48 narrow <strong>bells</strong> that cut the bass <strong>only where the kick is loud and the bass has something at the same frequency, at the same moment</strong>. Between kicks, and at frequencies where they do not meet, the bass is untouched.</p>
{img("sidechain/space.png", "The SPACE page: the bells as a live dynamic EQ on the bass, and when they work in the cycle.")}
{legend([
 ("The bells", "a live dynamic EQ on the bass: the kick (grey) and the bass (blue) faint behind; outside LOW .. HIGH the display is shaded"),
 ("Cut now", "the bells' response in red, dipping where they cut and coming back; the dot names the deepest point. The dashed line = the deepest they went this cycle"),
 ("LOW / HIGH", "drag the two handles on the display: the range the bells may work in"),
 ("dB scale", "the EQ's own scale: 0 dB at the top line, then -6, -12 and -18 dB"),
 ("WHEN THEY WORK", "one cycle: the kick, the bass as it leaves, and in red the moments the bells cut"),
 ("SPACE ON / OFF", "off = no bells; the duck and PHASE stay"),
 ("AMOUNT", "how deep the bells go where kick and bass meet (up to 18 dB)"),
 ("RELEASE", "how long a bell follows the kick after its hit"),
])}
{note("What you see and what you hear", "The red dip drops at once and returns over about 150 ms, so the eye can follow it. That is only the display: the bells in the audio are as fast as before, set by RELEASE.")}
{ctl("SPACE ON / OFF", "default ON<br>automatable (Space On)", "<p>Off = no bells at all, so you hear the bass without them. The duck and PHASE stay as they are. The display then reads SPACE IS OFF.</p>")}
{ctl("AMOUNT", "0 to 100 %<br>default 30 %<br>automatable", "<p>How deep the bells cut where the kick and the bass collide - 100 % is up to 18 dB. At 0 SPACE does nothing.</p>")}
{ctl("RELEASE", "15 to 125 ms<br>default 29 ms<br>automatable", "<p>How long a bell keeps following the kick after its hit. Short leaves the next bass note alone; longer keeps the room open through a long kick tail.</p>")}
{ctl("LOW / HIGH", "20 Hz to 20 kHz<br>default 20 Hz - 250 Hz<br>automatable", "<p>The range the bells may work in. Click or drag on the display and the nearer handle follows.</p>")}
{note("When SPACE cuts", "Mostly under the first bass note after the kick, where the kick's tail is still ringing. With a curve that keeps the kick's 16th closed, there is nothing to cut under the kick itself - by design. SPACE needs a kick: with GRID &middot; NO KICK it waits.")}
"""})

    S.append({"title": "Presets, undo, bypass and latency", "sub": "the rest of the header", "html": f"""
<h3>Presets</h3>
<div class="two">
<div>
{img("sidechain/psave.png", "The PRESET popup with one preset of your own, starred as the default, and the SAVE AS row open.")}
{legend([
 ("Genre presets", "full settings by genre"),
 ("Star", "click to make that preset the default"),
 ("MY PRESETS", "the presets you saved; the x deletes one"),
 ("Name + SAVE", "type a name, then SAVE (or Enter)"),
 ("+ SAVE PRESET AS", "opens the name row"),
])}
</div>
<div>
<p>Click <strong>PRESET</strong> in the header. A <strong>genre preset</strong> sets the curve, RATE, DEPTH, CLICK, DUCK, AUTO and SPACE; <strong>PHASE stays yours.</strong> Change anything and the header shows CUSTOM.</p>
{table(["Preset", "Rate &middot; shape"], [
 ("PSYTRANCE &middot; ROLLING", "1/4 &middot; STEP"), ("PSYTRANCE &middot; FULL-ON", "1/4 &middot; TIGHT"),
 ("TECHNO &middot; DEEP", "1/4 &middot; CLASSIC"), ("TECHNO &middot; PEAK", "1/4 &middot; HOLD"),
 ("HOUSE &middot; PUMP", "1/4 &middot; PUMP"), ("EDM &middot; BIG ROOM", "1/4 &middot; CLASSIC"),
 ("DNB &middot; HALF-TIME", "1/2 &middot; HOLD"), ("TRAP &middot; 808", "1/2 &middot; SWELL"),
])}
<p><strong>+ SAVE PRESET AS&hellip;</strong> opens a name field: type a name and press <strong>SAVE</strong>. Your preset keeps everything except BYPASS - every control on all three pages, plus the curve - and appears under <strong>MY PRESETS</strong>; saving under an existing name updates it. The <strong>x</strong> next to one of your presets deletes it.</p>
<p><strong>The star</strong> marks the default: the preset every new RONE Sidechain opens with. Click a star to make that preset the default; click the filled star again to go back to the factory default.</p>
</div></div>
<h3>Undo and redo</h3>
<p>The two arrows at the right of the tab row, on every page, step back and forward through your edits: <span class="kbd">Ctrl</span>+<span class="kbd">Z</span> undoes, <span class="kbd">Ctrl</span>+<span class="kbd">Shift</span>+<span class="kbd">Z</span> or <span class="kbd">Ctrl</span>+<span class="kbd">Y</span> redoes. They cover the parameters, the curve and the SHAPES row. One drag is one step, however long it is. BYPASS, host automation and loading or clearing a kick sample are not undo steps.</p>
<h3 style="break-before:page">BYPASS</h3>
<p>Takes the whole plugin out of the path. The bypassed signal is delayed by the same latency as the processed one, so the host's compensation still lines up and nothing jumps in time when you switch.</p>
<h3>Latency</h3>
<p>RONE Sidechain works about <strong>29 ms</strong> ahead: it is what lets the duck be silent at the kick's very first sample and what makes PHASE's linear-phase turn exact. The latency is constant and reported to the host, which compensates for it automatically. On a live input with monitoring you will hear it as a small delay.</p>
"""})

    S.append({"title": "Step-by-step workflows", "sub": "recipes", "html": f"""
{recipe("The rolling psytrance bass", "RATE 1/4, STEP",
 steps([
  "Preset PSYTRANCE &middot; ROLLING (STEP, DEPTH 100 %, CLICK 2 ms).",
  "Route the kick, or drop it on the plugin.",
  "Check UNDER THE KICK reads -&infin; dB, then drag the middle level so the first bass note sits where you want it.",
  "PHASE on AUTO; SPACE around 30 %.",
 ]))}
{recipe("A house pump", "the whole beat breathes",
 steps([
  "Preset HOUSE &middot; PUMP.",
  "Lower DEPTH until the pump sits in the groove (60-80 %).",
  "Bend the slope with its circle - a later rise pumps harder.",
 ]))}
{recipe("Duck only the sub", "keep the bass's top over the kick",
 steps([
  "DUCK = SUB, or BAND with the handles at 20 Hz and 120-200 Hz.",
  "Set SPLIT on the PHASE page to where your bass's sub ends.",
  "The top of the bass now plays straight through the kick.",
 ]))}
{recipe("Render the bass alone", "stems and bounces",
 steps([
  "Play the project once with the kick going in, so the plugin hears it.",
  "Solo or render the bass: the chip shows LAST KICK &middot; ON THE BEAT and the duck stays on the beat.",
  "Or drop the kick sample - it wins over the routing and is saved with the project.",
 ]))}
"""})

    S.append(G["conventions_section"](m, extras="<h4>Pages and views</h4><p>The page (SIDECHAIN, PHASE, SPACE), LANES / OVERLAY, FREE / QUANTIZE and GRID are remembered between sessions; they are view settings, not part of the sound.</p>"))
    S.append(G["support_section"](m, [
        ("The chip says GRID &middot; NO KICK", "No kick reaches the sidechain input. Click the chip and follow your DAW's steps until the SC meter moves on every kick - or drop a kick sample."),
        ("The SC meter moves but the curve only follows the beat (FL Studio)", "Before a route is made FL feeds the bass itself onto the sidechain bus; the plugin ignores it and follows the beat. Set the sidechain input to the kick under &#9881; &rarr; Processing &rarr; Connections."),
        ("Bass still audible under the kick", "Check UNDER THE KICK: raise DEPTH to 100 % and pull the curve's first level down to silence."),
        ("A click at the kick", "Raise CLICK a little, especially on low sustained basses."),
        ("A dropped kick does not follow my routed kick", "A dropped sample always wins. Press &#10005; CLEAR KICK to go back to the routed kick."),
        ("A factory shape is gone", "Right-click the SHAPES row &rarr; RESTORE FACTORY SHAPES."),
        ("PHASE or SPACE do nothing", "They need a kick (routed or dropped). SPACE must be ON; PHASE also needs the kick and the bass to overlap."),
    ]))

    m["sections"] = S
    return m
