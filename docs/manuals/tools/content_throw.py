def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    # Ice White (#D8E4EC) is the plugin's on-screen accent; it does not print. The manual
    # uses the same printable ice-grey as annotate.py so badges and headings match.
    m = {
        "id": "throw", "product": "RONE Throw", "eyebrow": "ONE-KNOB ATMOSPHERE THROW",
        "title_html": "RONE <i>Throw</i>", "accent": "#9FB3C4", "version": "1.0",
        "tagline": "One knob throws whatever is playing into a tempo-locked atmosphere: a dotted-8th ping-pong delay whose feedback loop cleans itself, opening into a wash - and the original sound is never touched.",
        "formats": ["VST3", "AU", "Standalone"], "vst3": "RONE Throw.vst3", "au": "RONE Throw.component", "exe": "RONE Throw.exe",
        "pdf": "RONE Throw - User Manual.pdf", "cover_img": "throw/tour_knob.png",
    }
    S = []

    S.append({"title": "Welcome to RONE Throw", "sub": "one knob, one gesture", "html": f"""
<p><strong>RONE Throw</strong> is a delay built around a single move. Turn the big knob up and whatever is playing is <em>thrown</em> into a tempo-locked ping-pong delay - dotted 8th by default - whose repeats bloom into a reverb wash the further you go. The feedback loop is band-limited, so it can run almost infinitely without ever turning to mud: the tail cleans itself into a fixed midrange cloud that sits behind the mix. Turn the knob back down and the atmosphere fades on its own timing.</p>
<p>Producers do this with a send bus, a ping-pong delay, an EQ and a limiter on the return, and a sidechain compressor keyed from the source. Throw folds that whole chain into one knob so you can perform it, automate it with a single line, or map it to a controller.</p>
<div class="two">
<div>
<h3>Where it shines</h3>
<ul>
<li><strong>End of a build</strong> - automate THROW up over the last bar and the riser dissolves into the drop as atmosphere.</li>
<li><strong>Vocal throws</strong> - the last word of a phrase echoes away into space while the next line stays clear (DUCK does that for you).</li>
<li><strong>Psytrance leads and plucks</strong> - dotted-8th and 3:2 cross-rhythm delays that stay in the pocket at any feedback.</li>
<li><strong>Instant atmosphere</strong> - park the knob high on a pad or a chord and it becomes a bed.</li>
</ul>
</div>
<div>
<h3>The 10-second version</h3>
{steps([
 "Insert Throw on a lead, a vocal, a riser - anything with a shape.",
 "Play the track.",
 "Turn THROW up - repeats appear on the grid, then open into a wash.",
 "Turn it back down; the tail rings out by itself."])}
</div></div>
{note("What the knob actually does", "THROW is a macro. As it opens it raises the send into the delay, the feedback of the loop and the amount of reverb on the repeats, all on curves tuned to feel like one movement. What it never does is touch the dry signal: Throw only ever <em>adds</em> a layer. The source stays exactly as loud as it was, at every knob position.")}
"""})

    S.append(G["install_section"](m))

    S.append({"title": "Quick start", "sub": "your first throw", "html": f"""
{img("throw/tour.png", "<b>RONE Throw.</b> The big knob is the whole story; the strip at the top holds the presets and six macro slots.", "w60")}
{steps([
 "<strong>Insert</strong> Throw on the channel you want to throw - a lead or a vocal is the classic choice. No send bus needed.",
 "<strong>Tap macro slot 1.</strong> The button lights white and the preset bar reads END OF BUILD. Slot 1 is the sound the plugin was built for.",
 "<strong>Play</strong> the track and turn <span class='pill'>THROW</span> up slowly. Repeats appear on the dotted-8th grid, bouncing left and right; past the middle of the knob they open into a reverb wash.",
 "<strong>Keep turning.</strong> Near the top the tail runs for a long time (TAIL shows how long), but the band it settles into is fixed, so it never piles up.",
 "<strong>Turn it back down.</strong> Nothing is cut off - what is already in the loop rings out on its own.",
 "<strong>Automate it.</strong> Draw a ramp from 0 to 90-100 % over the last bar of a build, and let it fall back over the first bar of the drop.",
])}
{note("Bypass vs. 0 %", "At 0 % the plugin is transparent - the dry passes through untouched and nothing is added. You do not need BYPASS in normal use; it is there for A/B checks while the knob is up.")}
"""})

    S.append({"title": "Interface tour", "sub": "every element on screen", "html": f"""
<div class="two">
{img("throw/tour.png", "Main view, macro slot 1 lit.")}
{img("throw/adv.png", "ADVANCED panel open, DUAL on.")}
</div>
{legend([
 ("Header logo", "click to flip to the back panel (About, version, licence)"),
 ("BYPASS", "hard bypass for A/B comparison; the tail keeps ringing"),
 ("Preset bar", "arrows step through the 26 factory presets, click the name for the menu"),
 ("Macro slots", "six quick-recall buttons: tap to recall, hold to store your own"),
 ("GRID", "the tap division - L and R separately when DUAL is on"),
 ("MS / BPM", "the tap spacing in milliseconds at the host tempo"),
 ("THROW", "the one knob: 0 % = nothing added; higher = longer, denser, more space"),
 ("Tap trail", "the repeats as they will sound - left above the line, right below, fading at the feedback rate"),
 ("BAND / TAIL", "the band the tail collapses into, and how long it rings"),
 ("ADVANCED", "opens the panel with TONE, FEEDBACK, WIDTH, DUCK, SPACE and the divisions"),
 ("MIX", "how much of the effect is added - a send level, the dry is never reduced"),
 ("Resize grip", "drag to resize the window"),
])}
<p>In the ADVANCED panel: <span class="b" style="display:inline-flex;align-items:center;justify-content:center;width:5mm;height:5mm;border-radius:50%;background:#9FB3C4;color:#101216;font-weight:800;font-size:8pt">1</span> TONE &nbsp; <span class="b" style="display:inline-flex;align-items:center;justify-content:center;width:5mm;height:5mm;border-radius:50%;background:#9FB3C4;color:#101216;font-weight:800;font-size:8pt">2</span> FEEDBACK &nbsp; <span class="b" style="display:inline-flex;align-items:center;justify-content:center;width:5mm;height:5mm;border-radius:50%;background:#9FB3C4;color:#101216;font-weight:800;font-size:8pt">3</span> WIDTH &nbsp; <span class="b" style="display:inline-flex;align-items:center;justify-content:center;width:5mm;height:5mm;border-radius:50%;background:#9FB3C4;color:#101216;font-weight:800;font-size:8pt">4</span> DUCK &nbsp; <span class="b" style="display:inline-flex;align-items:center;justify-content:center;width:5mm;height:5mm;border-radius:50%;background:#9FB3C4;color:#101216;font-weight:800;font-size:8pt">5</span> SPACE &nbsp; <span class="b" style="display:inline-flex;align-items:center;justify-content:center;width:5mm;height:5mm;border-radius:50%;background:#9FB3C4;color:#101216;font-weight:800;font-size:8pt">6</span> L / TIME &nbsp; <span class="b" style="display:inline-flex;align-items:center;justify-content:center;width:5mm;height:5mm;border-radius:50%;background:#9FB3C4;color:#101216;font-weight:800;font-size:8pt">7</span> DUAL &nbsp; <span class="b" style="display:inline-flex;align-items:center;justify-content:center;width:5mm;height:5mm;border-radius:50%;background:#9FB3C4;color:#101216;font-weight:800;font-size:8pt">8</span> R.</p>
"""})

    S.append({"title": "Controls reference", "sub": "what every control does, its range and default", "html": f"""
{img("throw/tour_knob.png", "<b>THROW.</b> The white dot is the position; the arc lights up as the atmosphere opens.", "w60")}
{ctl("THROW", "0 to 100 %<br>default 0 %<br>automatable, smoothed", "<p>The one knob. It drives three things at once, each on its own curve: the <em>send</em> into the delay, the <em>feedback</em> of the loop (from 0.30 at the bottom to 0.98 at the top - a tail of well over a minute), and the amount of <em>reverb</em> on the repeats. Low values give a clean rhythmic delay; the middle thickens it; the top third is atmosphere.</p><p>The send is normalised against the feedback, so turning the knob up adds length and density rather than volume. And the dry signal is never touched - at 100 % the source is exactly as loud as at 0 %, with the atmosphere added on top.</p>", "Double-click the knob to snap back to 0 %. Right-click it for the host's automation menu.")}
<h3>Presets and macro slots</h3>
{img("throw/tour_top.png", "<b>The strip.</b> Preset bar on the left, six macro slots on the right.", "w80")}
{ctl("Preset bar", "26 factory presets", "<p>The arrows step through the presets; click the name to open the menu, grouped by use: INIT, THROWS, LEADS, DUAL, ATMOSPHERES, VOCALS, DRUMS and CREATIVE. <em>Equinox - The Reference</em> is the measured source material with SPACE at 0 - the delay exactly as the reference does it, with no reverb, for comparison.</p><p><strong>LOCK THROW WHILE BROWSING</strong> (at the bottom of the menu) holds the big knob where you left it while you audition presets, so browsing never yanks the knob mid-performance.</p>")}
{ctl("Macro slots", "six buttons<br>tap = recall<br>hold = store", "<p>Six one-tap sounds that are always at the top of the window. <strong>Tap</strong> a slot to recall it - it lights white, and the preset bar shows what is in it. <strong>Hold</strong> a slot for about a second - the button fills, then flashes - to store the current settings into it, whatever they came from: a tweaked preset, something you dialled by hand, or a dual-time experiment you want back tomorrow.</p><p>The slots are yours, not the project's: they are saved per user and shared by every instance in every session, so \"macro 6\" is the same sound in every song. Out of the box they hold the six strongest presets. <em>RESET MACRO SLOTS TO FACTORY</em> in the preset menu puts them back.</p>", "Store your six go-to throws once, and every new session starts with them one tap away - no browsing.")}
{ctl("BYPASS", "", "<p>Hard bypass for A/B comparison. The engine keeps running with the knob at zero, so the tail that is already in the loop rings out instead of being cut, and coming back is click-free.</p>")}
{ctl("MIX", "0 to 100 %<br>default 100 %<br>automatable, smoothed", "<p>How much of the effect is added, in the bottom right corner. Throw is strictly additive, so this is a send level rather than a crossfade: the dry is never reduced. At 100 % you hear the plugin exactly as it was designed; at 0 % nothing is added at all.</p>", "At the top of the knob the output is louder than the input - that is the point of a send. Pull MIX down for gain staging rather than the knob. Double-click to snap back to 100 %.")}
<h3>Advanced panel</h3>
{img("throw/adv_panel.png", "<b>TONE, FEEDBACK, WIDTH, DUCK, SPACE, and the divisions.</b>", "w60")}
{ctl("TIME (L) and R", "1/16 &middot; 1/8 &middot; 1/8. &middot; 1/4 &middot; 1/4.<br>default 1/8. (L), 1/8 (R)", "<p>The tap division, always locked to the host tempo. The dotted 8th is the sound this plugin exists for - it is what the reference material uses - so it is the default. With DUAL off there is one division and the repeats bounce left, right, left, right one division apart. With DUAL on the right side gets its own row.</p>", "Without a host clock (the standalone app) the grid falls back to 120 BPM.")}
{ctl("DUAL", "default off", "<p>Two independent delays instead of one ping-pong. Each side is fed from the input and recirculates into itself, so the two divisions never interact and you hear a true cross-rhythm. The default pairing - dotted 8th left, straight 8th right - is the classic 3:2. The GRID readout shows both, and the tap trail draws each side at its own rate.</p>")}
{ctl("TONE", "dark to bright<br>default 759 Hz", "<p>Where the tail settles. The feedback loop is a band-pass; each pass through it removes a little above and below the band, so after a few dozen repeats the tail has collapsed into a fixed midrange cloud. TONE slides that band up and down by two octaves in each direction. The BAND readout shows its centre.</p>", "Darker keeps the atmosphere under a bright lead; brighter lets a dark pad be heard. The band is what keeps a near-infinite tail clean.")}
{ctl("FEEDBACK", "-100 to +100<br>default 0", "<p>A trim on the feedback the knob is already setting - not an absolute. Push it up for sparser material that needs a longer tail, or all the way for a tail that barely decays; pull it down when the track is dense. TAIL in the readout shows the result.</p>")}
{ctl("WIDTH", "0 to 150 %<br>default 100 %", "<p>How far apart the left and right repeats sit. 0 % folds the repeats to mono in the centre, 100 % is the measured ping-pong, and above that the two sides are pushed wider than they arrived.</p>")}
{ctl("DUCK", "0 to 100 %<br>default 35 %", "<p>The atmosphere steps aside while the source is playing and blooms back the moment it stops. This is the sidechain compressor on a return bus, built in and keyed from the dry input - a fast attack and a musical release. Raise it on vocals so every word stays clear; lower it on pads that should sit in their own wash.</p>", "Around 60-70 % on a vocal, the delay only really appears at the end of each phrase - the classic vocal throw with no automation at all.")}
{ctl("SPACE", "0 to 100 %<br>default 65 %", "<p>The reverb on the repeats. It opens with the big knob (there is none at the bottom of the knob, a lot at the top) and SPACE sets how far that goes. At 0 the plugin is the measured delay exactly - discrete repeats, no wash. At 100 the top of the knob is almost pure atmosphere.</p>", "The reverb only ever hears the wet signal, never the dry, and it is ducked together with the repeats.")}
<h3>Readouts</h3>
{ctl("GRID and MS / BPM", "", "<p>The division in use (L and R separately with DUAL) and the resulting tap spacing in milliseconds at the current host tempo.</p>")}
{ctl("BAND", "Hz", "<p>The centre of the band the tail collapses into. Follows TONE.</p>")}
{ctl("TAIL", "seconds", "<p>How long the atmosphere rings after the source stops, at the current knob position and FEEDBACK trim.</p>")}
{ctl("Tap trail", "", "<p>The repeats as they will play: left channel above the line, right below, spaced by the division and fading at the feedback rate. With DUAL on, each side steps at its own rate and you can see the cross-rhythm before you hear it.</p>")}
"""})

    S.append({"title": "Step-by-step workflows", "sub": "recipes", "html": f"""
{recipe("The end-of-build throw", "Any drop, any genre",
 steps([
  "Insert Throw on the lead or the riser. Tap macro slot 1 (END OF BUILD).",
  "Over the last bar of the build, draw THROW automation rising from 0 to 95 %. An exponential curve (slow start, fast end) feels more urgent.",
  "Let it fall back to 0 over the first bar of the drop. Do not cut it - the tail carries the energy across the downbeat.",
  "If the atmosphere fights the drop's lead, lower TONE a little so the tail sits under it.",
 ]))}
{recipe("The vocal throw", "Last word of a phrase",
 steps([
  "Tap macro slot 6 (VOCAL THROW) or set DUCK to about 65 %.",
  "Leave THROW at 60-70 % for the whole song. DUCK keeps the atmosphere almost silent while the vocal is singing and lets it bloom in the gaps.",
  "For a bigger throw on one word, automate THROW up to 100 % on that word only.",
 ]))}
{recipe("A 3:2 lead delay", "Psytrance leads and plucks",
 steps([
  "Open ADVANCED, switch DUAL on. Leave L at 1/8. and R at 1/8.",
  "THROW around 65-75 %, SPACE down to 30-40 % so the repeats stay rhythmic.",
  "Store it into a macro slot - hold the button - and it is one tap away in every session.",
 ]))}
{recipe("Freeze into a drop", "Transitions",
 steps([
  "Tap slot 4 (INFINITE CLOUD), or push FEEDBACK to +100.",
  "Automate THROW to 100 % on the last hit before a break; the hit hangs as a cloud for as long as you like.",
  "Bring THROW down over a couple of bars when the next section arrives.",
 ]))}
{recipe("Six macros for a live set", "Controllers, DJ-style sets",
 steps([
  "Dial in six throws you actually use and hold each into a slot. They are saved per user, so the same six are there in every project.",
  "Map THROW to a physical knob or the mod wheel (right-click the knob in VST3 and use your DAW's controller link).",
  "During the set, tap a slot to change the character and ride the one knob.",
 ]))}
"""})

    S.append({"title": "Recommendations", "sub": "how to get the best out of it", "html": f"""
<div class="dos">
<div><h4>Do</h4><ul>
<li>Use it as an <strong>insert</strong>. It only adds, so there is nothing to gain from a send bus - and DUCK needs to hear the dry.</li>
<li>Store the six throws you use into the macro slots, once. Browsing is for finding sounds; slots are for performing them.</li>
<li>Let the tail fall on its own when the knob comes down. Cutting it wastes the best part.</li>
<li>Match TONE to the source: dark under bright leads, bright over dark pads.</li>
<li>Use LOCK THROW WHILE BROWSING when auditioning presets during a session.</li>
</ul></div>
<div><h4>Avoid</h4><ul>
<li>Fighting the level with THROW. The output is meant to be louder at the top; use MIX for gain staging.</li>
<li>FEEDBACK at +100 on busy material without a plan to bring THROW down - that tail is longer than the song.</li>
<li>WIDTH above 100 % on a mono-checked mix; the pushed sides partly cancel in mono.</li>
</ul></div>
</div>
<h3>Sound tips</h3>
<ul>
<li>Why it never turns to mud: the feedback loop is a band-pass, so each repeat is a little narrower than the last. After a few dozen passes the tail is a fixed midrange cloud, whatever went in. That is what lets the feedback run this high.</li>
<li>SPACE at 0 is the reference delay exactly, measured from real psytrance hook FX. Everything above 0 is the atmosphere Throw adds to it.</li>
<li>Two instances - one at 1/8. and one in DUAL at 1/4 against 1/8. - on two stems give a build that thickens in stages.</li>
</ul>
"""})

    S.append(G["conventions_section"](m, note("Standalone", "Without a DAW clock there is no tempo to lock to, so the grid runs at 120 BPM in the standalone app. In a DAW the divisions follow the host tempo, including tempo changes.")))

    S.append(G["support_section"](m, [
        ("Nothing is added when I turn the knob", "Check MIX is not at 0 % and BYPASS is off. If the window shows a sign-in screen, the plugin is locked - sign in through the RONE Plugins Center."),
        ("The repeats are out of time", "The divisions follow the host tempo. Make sure the DAW is playing and reports its tempo; in the standalone app the grid runs at 120 BPM."),
        ("A macro slot is different in another project", "By design: the six slots are saved per user, not per project, so they are the same in every session. Store the sound you want with a hold, or use the preset menu's RESET MACRO SLOTS TO FACTORY."),
        ("The tail is too bright or too dark", "TONE moves the band the tail settles into by two octaves either way. The BAND readout shows the centre."),
        ("It gets loud at 100 %", "That is the send adding on top of the untouched dry - the source itself is never reduced. Pull MIX down to taste."),
    ]))

    m["sections"] = S
    return m
