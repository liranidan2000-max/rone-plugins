def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    m = {
        "id": "afterspace", "product": "RONE AFTERSPACE", "eyebrow": "CREATIVE REVERB, DELAY AND ATMOSPHERE DESIGNER",
        "title_html": "RONE <i>AFTERSPACE</i>", "accent": "#FF8A3D", "version": "0.1 (public beta)",
        "tagline": "Huge, cinematic spaces that stay clean in the mix: a large musical reverb with built-in ducking, a tail de-esser, tempo-synced echo and bloom, in one plugin.",
        "formats": ["VST3", "AU", "Standalone"], "vst3": "RONE AFTERSPACE.vst3", "au": "RONE AFTERSPACE.component", "exe": "RONE AFTERSPACE.exe",
        "pdf": "RONE AFTERSPACE - User Manual.pdf", "cover_img": "afterspace/tour_knobs.png",
        "latency": "None added (the reverb itself has a pre-delay you set)",
    }
    S = []

    S.append({"title": "Welcome to RONE AFTERSPACE", "sub": "big, clean, immediate", "html": f"""
<p><strong>RONE AFTERSPACE</strong> replaces the usual send chain - reverb, EQ, sidechain compressor, delay - with one plugin that already knows what a producer wants from a big space: it should sound expensive immediately, stay out of the way while the source is playing, and bloom into the gaps.</p>
<p>Four ideas live in one workflow: a large musical <strong>reverb</strong>, intelligent <strong>ducking</strong> driven by the plugin's own input, an <strong>echo</strong> that can sit before, inside or after the space, and <strong>BLOOM</strong>, which makes the late tail expand and evolve instead of simply decaying. <strong>SILK</strong> keeps the tail from hissing, <strong>MONO LOW</strong> keeps the low end tight, and <strong>AUTO GAIN</strong> keeps the wet level steady while you turn SIZE and DECAY.</p>
<div class="two">
<div>
<h3>Where it shines</h3>
<ul>
<li><strong>Big clean vocals</strong> - long tail, strong ducking, moderate pre-delay.</li>
<li><strong>Psytrance / trance leads</strong> - synced echo into a bright hall, ducked while the lead plays.</li>
<li><strong>Transitions</strong> - long decay, high BLOOM, wide motion; FREEZE at the end of a phrase.</li>
<li><strong>Pads and atmospheres</strong> - large SIZE, subtle ducking, wide, low motion.</li>
<li><strong>Drums</strong> - short clean room with transient ducking.</li>
</ul>
</div>
<div>
<h3>The 10-second version</h3>
{steps([
 "Insert on a vocal or lead (insert or send, both work).",
 "Pick a preset or turn SIZE and DECAY.",
 "Raise DUCK until the space steps back while the source plays.",
 "Add ECHO if you want repeats; drag CHARACTER towards DREAM for more movement."])}
</div></div>
{note("Signal flow in one line", "Input &rarr; (ECHO if routed BEFORE) &rarr; pre-delay &rarr; reverb network with BLOOM, motion and width &rarr; (ECHO if INSIDE) &rarr; tone filters, damping, SILK &rarr; (ECHO if AFTER) &rarr; DUCK (driven by the dry input) &rarr; AUTO GAIN &rarr; MIX with the dry signal. FREEZE holds the reverb network; WET solo mutes the dry path for auditioning.")}
"""})

    S.append(G["install_section"](m))

    S.append({"title": "Quick start", "sub": "a big clean vocal space in a minute", "html": f"""
{img("afterspace/tour.png", "<b>RONE AFTERSPACE.</b> The default preset INIT - CLEAN SPACE.")}
{steps([
 "<strong>Insert</strong> AFTERSPACE on the vocal channel. As an insert, use MIX to balance; on a send, set MIX to 100 % and use the send level.",
 "<strong>Start from a preset.</strong> Step through them with &#9664; &#9654; or click the name for the list. <em>Huge Clean Vocal</em>, <em>Psy Lead Space</em> and <em>Festival Hall</em> are good starting points; <em>Init - Clean Space</em> is the neutral setting.",
 "<strong>Set the room.</strong> SIZE is how big the space feels; DECAY is how long the tail rings (0.2 s to 30 s). AUTO GAIN keeps the wet level steady while you change them.",
 "<strong>Keep it clean.</strong> Raise DUCK until the tail steps back while the vocal is singing and returns in the gaps. The DUCK chip in the display shows the gain reduction in dB; the envelope in the DUCK module draws it over time. SMART mode (default) ducks where the source is, leaving the rest of the atmosphere alive.",
 "<strong>Tame the top.</strong> SILK is a de-esser on the reverb tail: more SILK, fewer sibilant splashes.",
 "<strong>Add repeats.</strong> Turn ECHO up and pick a note value (1/4D is the default). The dots in the ECHO module show the repeats.",
 "<strong>Set the mood.</strong> BLOOM makes the late tail expand after the note; CHARACTER slides the whole engine from tight and CLEAN to wide and DREAMy.",
 "<strong>Audition.</strong> Press <span class='pill'>&#127911; WET</span> to hear only the space; press FREEZE at the end of a phrase to hold the tail.",
])}
"""})

    S.append({"title": "Interface tour", "sub": "every element on screen", "html": f"""
{img("afterspace/tour.png", "Main view.")}
{legend([
 ("Header logo", "click to flip to the back panel (About, version, licence)"),
 ("Preset browser", "previous / next; click the name for the full list"),
 ("WET solo", "listen to the space alone"),
 ("BYPASS", "hard bypass"),
 ("Space visualiser", "a nebula that grows with SIZE, spins with motion and brightens with level"),
 ("SPATIAL", "current stereo mode of the wet signal"),
 ("BPM", "host tempo used by synced echo times"),
 ("ENGINE", "ACTIVE, FROZEN or BYPASSED"),
 ("DUCK gain reduction", "how much the tail is being pushed down right now, in dB"),
 ("SIZE", "size of the space"),
 ("DECAY", "tail length in seconds"),
 ("BLOOM", "late-tail expansion"),
 ("SILK", "tail de-esser"),
 ("MIX", "dry / wet balance"),
 ("Duck envelope", "gain reduction drawn over time"),
 ("DUCK amount", "maximum attenuation of the wet signal"),
 ("Duck MODE", "SMART, FULL, VOCAL, TRANSIENT or CUSTOM"),
 ("Echo display", "the repeats and their level"),
 ("ECHO amount", "level of the echo"),
 ("Echo TIME", "synced note value or FREE (milliseconds)"),
 ("FREEZE", "hold the current tail"),
 ("CHARACTER", "CLEAN to DREAM macro"),
 ("ADVANCED", "opens the drawer with SPACE, TONE, MOTION &amp; STEREO, DUCK and ECHO details"),
 ("Resize grip", "drag to resize the window"),
])}
{img("afterspace/adv.png", "<b>ADVANCED drawer.</b> 1 SPACE (pre-delay, texture, AUTO GAIN) &middot; 2 TONE (low / high cut, damping) &middot; 3 MOTION &amp; STEREO (motion, width, MONO LOW) &middot; 4 DUCK (attack, hold, release, sensitivity - active in CUSTOM mode) &middot; 5 ECHO (feedback, free time, filters, routing, PING PONG).")}
"""})

    S.append({"title": "Controls reference", "sub": "what every control does, its range and default", "html": f"""
<h3>Main knobs</h3>
{img("afterspace/tour_knobs.png", "<b>SIZE, DECAY, BLOOM, SILK, MIX.</b>")}
{ctl("SIZE", "0 to 100 %<br>default 55 %", "<p>The dimensions of the space: early reflections spread out, the tail becomes denser and wider. Small values give rooms and plates; large values give halls and cathedrals.</p>", "Change SIZE with the source playing; the visualiser grows with it and AUTO GAIN keeps the level constant so you judge the size, not the loudness.")}
{ctl("DECAY", "0.2 s to 30 s<br>default 2.2 s", "<p>How long the tail takes to die away. The range is musically curved: the first half of the knob covers up to about 3 s, the rest stretches to 30 s.</p>", "For vocals, set DECAY so the tail ends just before the next phrase, then use DUCK to keep it out of the words.")}
{ctl("BLOOM", "0 to 100 %<br>default 10 %", "<p>The creative identity of AFTERSPACE. Instead of only decaying, the late tail <em>opens up</em> after the note: diffusion, width and modulation increase over time so the room seems to grow behind the source. 0 % is a conventional reverb; 25-50 % is an audible expansion that does not sound like chorus; 100 % is a full atmosphere generator.</p>", "BLOOM plus a long DECAY plus FREEZE is the fastest way to make a pad from a single vocal note.")}
{ctl("SILK", "0 to 100 %<br>default 35 %", "<p>A de-esser on the reverb input and tail. It stops sibilance and harsh transients from turning into a wash of hiss, without darkening the whole reverb the way a high cut would.</p>")}
{ctl("MIX", "0 to 100 %<br>default 25 %", "<p>Dry / wet balance. On a send set it to 100 %.</p>")}
<h3>DUCK module</h3>
{img("afterspace/tour_duck.png", "<b>DUCK.</b> Envelope, AMOUNT and MODE.", "w80")}
{ctl("AMOUNT", "0 to 100 %<br>default 20 %", "<p>How far the wet signal is pushed down while the dry input is active. The detector is the plugin's own input, so no sidechain routing is needed. 30-50 % is the classic 'reverb that gets out of the way'; higher values make the space appear only in the gaps.</p>")}
{ctl("MODE", "SMART &middot; FULL &middot; VOCAL &middot; TRANSIENT &middot; CUSTOM<br>default SMART", "<p><strong>SMART</strong> ducks by frequency region: the tail is attenuated where the source is dominant while the rest of the atmosphere stays alive. <strong>FULL</strong> is broadband ducking of the whole wet signal. <strong>VOCAL</strong> weights the detector for speech intelligibility. <strong>TRANSIENT</strong> reacts fast to attacks - drums and plucks. <strong>CUSTOM</strong> hands you the ATTACK, HOLD, RELEASE and SENS controls in the ADVANCED drawer.</p>")}
{ctl("ATTACK / HOLD / RELEASE / SENS", "0.1-200 ms &middot; 0-500 ms &middot; 20-3000 ms &middot; 0-100 %<br>defaults 10 ms &middot; 40 ms &middot; 300 ms &middot; 50 %<br>active in CUSTOM mode", "<p>The detector envelope. ATTACK is how fast the tail ducks when the source starts, HOLD keeps it down between syllables, RELEASE is how fast the space comes back, SENS is the detector threshold.</p>", "Long RELEASE (600-900 ms) on vocals makes the reverb swell back like a breath at the end of each line.")}
<h3>ECHO module</h3>
{img("afterspace/tour_echo.png", "<b>ECHO.</b> Repeats display, AMOUNT and TIME.", "w80")}
{ctl("AMOUNT", "0 to 100 %<br>default 0 %", "<p>Level of the echo. At 0 % the echo engine is silent.</p>")}
{ctl("TIME", "1/32 &hellip; 2 Bars (with dotted and triplet values) or FREE<br>default 1/4D", "<p>Delay time as a note value at the host tempo, or FREE, which uses the FREE MS control (10 to 2000 ms) in the ADVANCED drawer.</p>")}
{ctl("FEEDBACK", "0 to 95 %<br>default 25 %", "<p>How many repeats. The loop is limited internally, so even the maximum never runs away.</p>")}
{ctl("LOW CUT / HIGH CUT (echo)", "150 Hz &middot; 10 kHz defaults", "<p>Filters in the feedback path, so every repeat gets darker and thinner. Classic tape-style behaviour.</p>")}
{ctl("Routing: BEFORE / INSIDE / AFTER", "default BEFORE", "<p><strong>BEFORE</strong>: dry &rarr; echo &rarr; reverb; the repeats feed the space. <strong>INSIDE</strong>: the echo is cross-fed into the late reverb network; repeats dissolve into the space rather than staying separate. <strong>AFTER</strong>: reverb &rarr; echo; the tail itself echoes.</p>", "INSIDE is the one nobody else has - try it on a lead with a long DECAY.")}
{ctl("PING PONG", "on / off", "<p>Alternates repeats left and right.</p>")}
<h3>Bottom row</h3>
{img("afterspace/tour_bottom.png", "<b>FREEZE, CHARACTER, ADVANCED.</b>")}
{ctl("FREEZE", "on / off", "<p>Holds the current state of the reverb network for as long as you like - an infinite sustain, click-free in and out. The dry signal keeps playing normally. Automate it at the end of a phrase and release it on the next downbeat.</p>")}
{ctl("CHARACTER", "CLEAN &rarr; DREAM<br>default 25 %", "<p>A macro that moves the whole engine at once. Towards CLEAN: tighter diffusion, less modulation, controlled width, darker and cleaner tail, conservative echo feedback. Towards DREAM: more movement, wider late field, softer diffusion, more air, more bloom and more echo interaction. The mapping is musical, not a linear blend.</p>")}
<h3>ADVANCED drawer</h3>
{img("afterspace/adv_drawer.png", "<b>The five advanced groups.</b>")}
{table(["Group", "Control", "Range / default", "What it does"], [
 ("SPACE", "PRE-DLY", "0-250 ms, 20 ms", "Silence before the reverb starts; keeps the attack of the source clear."),
 ("", "TEXTURE", "0-100 %, 70 %", "Density and diffusion of the reflections. Low = sparse, grainy early space; high = smooth wash."),
 ("", "AUTO GAIN", "on by default", "Keeps the wet loudness steady when SIZE or DECAY change."),
 ("TONE", "LOW CUT / HIGH CUT", "120 Hz / 12 kHz", "Filters on the reverb input."),
 ("", "LO DAMP / HI DAMP", "20 % / 40 %", "How fast lows and highs die inside the tail. More HI DAMP = warmer, shorter top end."),
 ("MOTION &amp; STEREO", "MOTION", "0-100 %, 15 %", "Modulation inside the tail. Small amounts avoid metallic ringing; large amounts are chorus-like."),
 ("", "WIDTH", "0-150 %, 115 %", "Stereo width of the wet signal. 100 % is natural; above it is wider than the source."),
 ("", "MONO LOW", "on by default", "Keeps the wet signal mono below about 150 Hz for a solid low end."),
 ("DUCK", "ATTACK / HOLD / RELEASE / SENS", "see above", "Detector envelope, active in CUSTOM mode."),
 ("ECHO", "FEEDBACK / FREE MS / LOW CUT / HIGH CUT / routing / PING PONG", "see above", "Echo engine details."),
])}
<h3>Presets</h3>
<p>Factory presets: Init - Clean Space, Huge Clean Vocal, Wide Pop Tail, Dream Vocal Throw, Dark Intimate Hall, Psy Lead Space, Clean 1/8D Lead, Festival Hall, Acid Echo Space, Infinite Violet Cloud, Deep Temple, Blooming Sky, Frozen Air, Tight Transient Room, Big Snare Bloom, Percussion Chamber, Silk Pad Widener, End Of Phrase Throw, Riser Space, Cathedral Of Glass, Violet Dream Machine and Ducked Infinity. Presets never change BYPASS or MIX on a send.</p>
"""})

    S.append({"title": "Step-by-step workflows", "sub": "recipes", "html": f"""
{recipe("Huge clean vocal", "Pop, EDM, cinematic vocals",
 steps([
  "Insert on the lead vocal (or on a send with MIX 100 %). Load <em>Huge Clean Vocal</em>, or: SIZE 70 %, DECAY 3.5 s, PRE-DLY 40 ms, SILK 50 %.",
  "DUCK 45 %, MODE VOCAL. Play the verse: the tail should disappear behind the words and swell back at the ends of lines.",
  "In CUSTOM mode set RELEASE to 700 ms if the swell should be slower.",
  "BLOOM 20 %; CHARACTER 35 %. Keep MONO LOW on.",
 ]))}
{recipe("Psytrance lead space", "Synced echo into a bright hall",
 steps([
  "Load <em>Psy Lead Space</em>, or: SIZE 60 %, DECAY 2 s, HIGH CUT 14 kHz, HI DAMP 25 %.",
  "ECHO 35 %, TIME 1/8D, FEEDBACK 40 %, routing BEFORE, PING PONG on.",
  "DUCK 50 %, MODE SMART - the echo and the hall duck together while the lead plays.",
  "For the 'inside the hall' version switch routing to INSIDE.",
 ]))}
{recipe("End-of-phrase throw", "Vocal throws, FX hits",
 steps([
  "On a duplicate of the vocal, insert AFTERSPACE with MIX 100 % and load <em>End Of Phrase Throw</em>.",
  "Automate MIX (or the send) to open only on the last word of the phrase.",
  "Automate FREEZE on for the last beat of the bar and off on the next downbeat.",
 ]))}
{recipe("Transition atmosphere", "Breakdowns, intros",
 steps([
  "Load <em>Blooming Sky</em> on a pad or a held vocal note: DECAY 12 s, BLOOM 70 %, MOTION 35 %, WIDTH 130 %, CHARACTER 80 %.",
  "Press WET solo to hear only the space; press FREEZE and stop the source - the frozen tail becomes the transition bed.",
  "Automate HIGH CUT down over the last bar for a natural fade.",
 ]))}
{recipe("Tight drum room", "Drums, percussion",
 steps([
  "Load <em>Tight Transient Room</em>: SIZE 20 %, DECAY 0.6 s, PRE-DLY 5 ms, TEXTURE 45 %.",
  "DUCK 35 %, MODE TRANSIENT. The room speaks between hits, not on top of them.",
 ]))}
"""})

    S.append({"title": "Recommendations", "sub": "how to get the best out of it", "html": f"""
<div class="dos">
<div><h4>Do</h4><ul>
<li>Use DUCK instead of automating the send. It reacts to the performance, so it is right on every take.</li>
<li>Leave AUTO GAIN and MONO LOW on unless you have a reason not to. They are why AFTERSPACE stays clean at large sizes.</li>
<li>Reach for SILK before HIGH CUT when the tail is harsh - it removes the splashes and keeps the air.</li>
<li>Use CHARACTER as your first move when a preset is close but not quite: it changes many parameters musically at once.</li>
<li>Try ECHO routing INSIDE on sustained material; it is the setting that makes repeats feel like part of the room.</li>
</ul></div>
<div><h4>Avoid</h4><ul>
<li>BLOOM above 50 % on rhythmic sources at short DECAY; the expansion has no time to develop and can smear transients.</li>
<li>WIDTH above 130 % on material that must survive mono playback.</li>
<li>Stacking DUCK with an external sidechain compressor; one of them is enough and two will pump.</li>
<li>Judging the space with MIX at 25 % on a send - set it to 100 % and use the send level instead.</li>
</ul></div>
</div>
<h3>Sound tips</h3>
<ul>
<li><strong>PRE-DLY is definition.</strong> 20-40 ms separates the dry attack from the tail and makes a large space feel closer without making it smaller.</li>
<li><strong>TEXTURE below 40 %</strong> gives a grainy, almost granular early space that works beautifully on plucks.</li>
<li><strong>FREEZE plus MOTION</strong> keeps a frozen tail alive; without motion a frozen tail is static.</li>
<li><strong>The visualiser is a meter.</strong> The nebula brightens with level and spins with motion, so a glance tells you whether the space is doing something while the source plays.</li>
</ul>
"""})

    S.append(G["conventions_section"](m, note("Beta release", "AFTERSPACE is in public beta. The parameter set is final and sessions will load in every future version; what will still improve is the fine tuning of the algorithms behind the same controls.")))

    S.append(G["support_section"](m, [
        ("The reverb sounds distant and small even at high SIZE", "Check MIX (25 % default on an insert) and the send level. Also raise PRE-DLY: a space with 0 ms pre-delay reads as further away."),
        ("Ducking does not react", "DUCK reacts to the plugin's own input. On a send, the input is the send signal - make sure the send is pre-fader or the source is not muted. Raise SENS in CUSTOM mode for quiet sources."),
        ("Synced echo is out of time", "The host must send tempo and transport position (most do while playing). In the standalone app the plugin runs at its own BPM shown in the chip."),
        ("CPU is high", "Reduce TEXTURE and MOTION slightly, or use one instance on a send instead of several inserts."),
    ]))

    m["sections"] = S
    return m
