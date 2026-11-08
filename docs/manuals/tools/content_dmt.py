def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    m = {
        "id": "dmt", "product": "RONE DMT", "eyebrow": "THE GRANULAR INSTRUMENT",
        "title_html": "RONE <i>DMT</i>", "accent": "#14A58F", "version": "1.0.0",
        "tagline": "Drop a sample, play it from MIDI, and see every grain: the classic granular engine rebuilt with values you can set exactly and a recording of everything you hear.",
        "formats": ["VST3", "AU", "Standalone"], "vst3": "RONE DMT.vst3", "au": "RONE DMT.component", "exe": "RONE DMT.exe",
        "pdf": "RONE DMT - User Manual.pdf", "cover_img": "dmt/main.png",
        "channels": "Instrument: MIDI in, stereo or mono out",
    }
    S = []

    S.append({"title": "Welcome to RONE DMT", "sub": "a granular instrument you can read", "html": f"""
<p><strong>RONE DMT</strong> cuts a sample into small overlapping pieces - <em>grains</em> - and plays them back with their own timing. Slow a vocal down without changing its pitch, freeze one syllable into a pad, run a phrase backwards word by word, or chop it into rhythmic stutters.</p>
<p>The engine is a classic granular model, rebuilt with exact control - with the parts that are usually hard to use fixed:</p>
<ul>
<li><strong>Every value is exact</strong> - ATTACK, HOLD, SPACING, SPEED and PAN show real milliseconds and percentages, and the ONE GRAIN card spells out what they add up to.</li>
<li><strong>You see what happens</strong> - the SOURCE view marks every grain on the sample as it plays; the OUTPUT view records everything RONE DMT outputs, on the host grid.</li>
<li><strong>Drag the result into your DAW</strong> - the recording is a WAV you can drop straight onto a track.</li>
<li><strong>ONE KNOB</strong> - six ready-made directions (BLOOM, FREEZE, REVERSE, WIDE, SHATTER, TUNNEL) on one big knob.</li>
<li><strong>NOTE</strong> - a small piano turns the whole sample into one musical note, or lets your MIDI keyboard choose it.</li>
</ul>
<div class="two">
<div>
<h3>Where it shines</h3>
<ul>
<li><strong>Vocal textures</strong> - half-time and frozen vocals, reversed words, whispers into pads.</li>
<li><strong>Psytrance and techno FX</strong> - grain stutters on the grid, sweeps, risers out of any sound.</li>
<li><strong>Sound design</strong> - turn a one-shot into an evolving cloud.</li>
<li><strong>Resampling</strong> - play, drag the take out, keep going.</li>
</ul>
</div>
<div>
<h3>The 10-second version</h3>
{steps([
 "Insert RONE DMT on an instrument track.",
 "Drop a sample on the SOURCE view.",
 "Press PLAY (or any MIDI note).",
 "Pick a preset, then turn the knobs."])}
</div></div>
"""})

    inst = G["install_section"](m)
    inst["html"] = inst["html"].replace("shows a <em>RONE Bundle License Required</em> screen", "shows the <em>A RONE DMT licence or RONE ALL ACCESS is required</em> screen")
    S.append(inst)

    S.append({"title": "Quick start", "sub": "your first grains", "html": f"""
{img("dmt/main.png", "<b>RONE DMT</b> playing a vocal hook on SLOW MOTION: the grains on the sample above, the recording of what you hear below.", "w80")}
{steps([
 "<strong>Insert</strong> RONE DMT on an <strong>instrument</strong> track (in FL Studio: a channel in the Channel Rack). It is an instrument: it plays when it gets a MIDI note or when you press PLAY.",
 "<strong>Load a sample</strong> - drop a WAV, AIFF, FLAC, OGG or MP3 on the SOURCE view, or double-click the SOURCE to choose one. A fresh instance starts empty.",
 "<strong>Press PLAY</strong> or play any MIDI note. Every note starts the sample from the top; the pitch of the note does not change the sound (unless MIDI is on, see NOTE).",
 "<strong>Pick a preset</strong> - click the name for the list. Start with <span class='pill'>HALF TIME</span> or <span class='pill'>SLOW MOTION</span> on a vocal.",
 "<strong>Turn SPEED down</strong> to slow the sample without changing its pitch, then shape the grains with ATTACK.",
 "<strong>Drag the take out</strong> - when the note ends, the chip at the bottom reads <em>DRAG TO DAW</em>: drag it onto an audio track.",
])}
{note("The sample travels with the project", "RONE DMT stores the sample inside your DAW project (as FLAC). A project opens with its sound even if the original file was moved or deleted.")}
"""})

    S.append({"title": "Interface tour", "sub": "every element on screen", "html": f"""
{img("dmt/tour.png", "The main page with a vocal loaded and a 2-bar take recorded.")}
{legend([
 ("Header logo", "click to flip to the back panel (About, version, licence)"),
 ("PRESET", "the name opens the factory list by category; the arrows step through it"),
 ("MY SLOTS", "six keys for your own settings: tap = recall, hold = store"),
 ("ONE KNOB", "switches the five knobs over to one big macro knob"),
 ("SYNC / FREE", "HOLD as a note value of the host tempo, or in milliseconds"),
 ("RESET", "everything back to the start: the sample plays exactly as it is"),
 ("SOURCE", "the loaded sample; every grain is drawn on it as it plays. Drop a file here or double-click"),
 ("OUTPUT", "everything RONE DMT outputs, recorded on the host grid, with each grain on the lane above"),
 ("View length", "how much of the recording the OUTPUT view shows: 1, 2 or 4 bars, or the whole take"),
 ("The five knobs", "ATTACK, HOLD, SPACING, SPEED and PAN around the ONE GRAIN card"),
 ("PLAY / STOP", "plays the sample from the top without MIDI; press again to stop"),
 ("Host tempo", "the tempo SYNC and the OUTPUT grid follow"),
 ("NOTE piano", "turns the whole sample into one note; click the lit key again for the sample as it is"),
 ("MIDI", "your MIDI keyboard picks the note instead of the piano"),
 ("Octave", "the octave of the note"),
 ("OUTPUT level", "-24 to +12 dB after everything else"),
 ("DRAG TO DAW", "drag the recorded take onto a track"),
 ("SAVE", "writes the take as a WAV to Documents / RONE Plugins / Exports / DMT"),
 ("Resize grip", "drag to scale the whole window"),
])}
"""})

    S.append({"title": "The grain controls", "sub": "five knobs and the ONE GRAIN card", "html": f"""
{img("dmt/tour_knobs.png", "The five knobs around the ONE GRAIN card, which draws one grain after another with the values you set.", "w80")}
<p>A grain is a short piece of the sample with a fade in and a fade out. The grain is <strong>ATTACK + HOLD + ATTACK</strong> long; a new grain starts every <strong>(HOLD + ATTACK) x SPACING</strong>; and each grain reads the sample <strong>(HOLD + ATTACK) x SPEED</strong> further on. At SPACING 100 % and SPEED 100 % the grains stitch back into the original sample.</p>
{ctl("ATTACK", "5 - 500 ms &middot; default 40 ms", "The linear fade in and fade out of every grain. Short = clicky, gritty, rhythmic; long = soft, smeared, pad-like. It is added to HOLD twice, so long ATTACK values make long grains that overlap.")}
{ctl("HOLD", "0 - 500 ms (FREE) &middot; 1/64 - 1 BAR (SYNC) &middot; default 5 ms", "How long each grain stays at full level. In SYNC the step HOLD + ATTACK is a note value of the host tempo - grains land on the grid, so rhythmic chops stay in time.")}
{ctl("SPACING", "11 - 300 % &middot; default 100 %", "The distance between grains in what you <em>hear</em>, in % of HOLD + ATTACK. Lower = grains closer together and overlapping, a denser sound; higher = gaps between grains, a choppy, sparse sound.")}
{ctl("SPEED", "-300 - 300 % &middot; default 100 %", "How far each grain moves on in the <em>sample</em>. 100 % reads at normal speed; 50 % takes twice as long through the sample (half time) with the pitch unchanged; 0 % freezes on one spot; negative values walk backwards through the sample - each grain still plays forward, so words come in reverse order but stay readable.")}
{ctl("PAN", "0 - 100 % &middot; default 0 %", "Grains 1, 3, 5... go left and grains 2, 4, 6... go right. 100 % = hard left / hard right, the widest ping-pong of grains.")}
<h3>The ONE GRAIN card</h3>
<p>The card draws the grains with the values on the knobs and prints what they add up to: <strong>GRAIN</strong> (the length of one grain), <strong>OVERLAP</strong> (how much two grains share), <strong>EVERY</strong> (a new grain every ... ms) and <strong>READ SPEED</strong> (how fast the sample is read compared with the original - 0.25x = four times slower).</p>
{note("Slow reads start at the sound", "When SPEED reads slower than 3/4 of the original, a note starts at the first audible part of the sample instead of the silence before it - otherwise a slow preset could spend the whole note in the breath before the first word.")}
"""})

    S.append({"title": "SOURCE and OUTPUT", "sub": "see it, record it, drag it out", "html": f"""
<div class="two">
<div>
<h3>SOURCE</h3>
<p>The sample you loaded, with its name and length. While a note plays every grain appears on it as a shape - its fade in, hold and fade out - with a line running through it, so you see which part of the sample each grain reads and in which order. Odd and even grains have two colours (left and right with PAN).</p>
<ul>
<li><strong>Drop</strong> an audio file on it, or <strong>double-click</strong> it to choose one.</li>
<li>WAV, AIFF, FLAC, OGG and MP3; any sample rate (it is converted to the host rate).</li>
</ul>
<h3>OUTPUT</h3>
<p>Everything RONE DMT outputs is recorded, from the first note until about 1.5 s after the sound stops, and drawn on the host's bar grid. The lane above the waveform shows every grain as a short line, so you can read the rhythm of the grains. Clipped parts are drawn red.</p>
<p>The view buttons show the last 1, 2 or 4 bars while it records, or the <strong>WHOLE TAKE</strong>.</p>
</div>
<div>
{img("dmt/tour_bottom.png", "PLAY / STOP, the NOTE piano with MIDI and octave, OUTPUT level, DRAG TO DAW and SAVE.", "")}
<h3>Taking the sound out</h3>
{steps([
 "Play a note (or PLAY) and let it end.",
 "The chip changes from <em>RECORDING</em> to <em>DRAG TO DAW 3.14 s</em>.",
 "Drag the chip onto an audio track - or press <strong>SAVE</strong> to write the WAV to <code>Documents / RONE Plugins / Exports / DMT</code>.",
])}
<p>The next note starts a new take.</p>
</div></div>
"""})

    S.append({"title": "ONE KNOB", "sub": "six directions on one knob", "html": f"""
{img("dmt/oneknob.png", "ONE KNOB on, mode BLOOM, turned to SLOW 50.", "w80")}
{legend([("ONE KNOB", "the macro: left = SLOW, right = FAST, centre = the sample as it is"), ("Modes", "six directions for the same knob"), ("What the mode does", "a line that says what left and right do in this mode")])}
<p>Switch <strong>ONE KNOB</strong> on and the five knobs lock; one big knob in the middle moves all of them together. The centre is the sample as it is; turn left towards <strong>SLOW</strong> and right towards <strong>FAST</strong>. The small knobs keep showing what the macro made of them.</p>
{table(["Mode", "Left (SLOW)", "Right (FAST)"], [
 ("BLOOM", "almost stands still, every grain swells in", "short, tight, racing"),
 ("FREEZE", "slows to a dead stop, a soft pad", "chopped hard, big gaps"),
 ("REVERSE", "slows, freezes, then runs backwards fast", "rushes forward"),
 ("WIDE", "a huge slow cloud, spread left / right", "a hard, wide doubler"),
 ("SHATTER", "falls apart into sparks", "packs into a machine buzz"),
 ("TUNNEL", "sucked backwards, wide", "thrown forward in waves"),
])}
<p>Switch ONE KNOB off and the five knobs keep the values the macro had - the sound does not jump - so you can fine-tune from there. Double-click the big knob to go back to the centre.</p>
"""})

    S.append({"title": "NOTE and MIDI", "sub": "one sample, any note", "html": f"""
{img("dmt/note_note.png", "NOTE on G: the whole sample is flattened to G before the grains read it.", "w60")}
<p>The small piano turns the whole sample into <strong>one musical note</strong> - a sung phrase becomes a phrase on G, for example - before the grains read it. The flatten is RONE Iron's pitch engine. Click a key to choose the note, the <strong>- / +</strong> buttons for the octave, and the lit key again to go back to the sample as it is.</p>
<ul>
<li>The first time NOTE is used on a sample, RONE DMT analyses it and prepares all twelve notes in the background (about a second for a short vocal). A key that is still being prepared blinks.</li>
<li>Turn <strong>MIDI</strong> on and the keys you play choose the note. A key played over a sound that is already running only changes the note, without starting the sample again (legato); a key played on silence starts from the top.</li>
<li>Presets and RESET never change NOTE on their own - except RESET, which also clears the note.</li>
</ul>
{note("Rendering", "When the DAW renders offline, RONE DMT waits for the note's buffer before it starts, so a bounce never starts on the unflattened sample.")}
"""})

    S.append({"title": "Presets and MY SLOTS", "sub": "36 starting points and six of your own", "html": f"""
<div class="two">
{img("dmt/presets.png", "The factory list, by category.", "")}
<div>
<p>Click the preset name for the list; the arrows step through it. A <strong>*</strong> after the name means you changed something since loading it.</p>
{table(["Category", "What is inside"], [
 ("CLEAN", "INIT (the sample as it is) and a wide ping-pong"),
 ("SLOW", "half time, slow motion, stretches and frozen pads"),
 ("FAST", "double time and tight rushes"),
 ("RHYTHM", "grain gates and stutters on the host grid (SYNC)"),
 ("TEXTURE", "reversed words, clouds, sprays, buzz, dust"),
 ("PSYCHEDELIC", "long, wide, strange textures for psytrance and sound design"),
 ("ONE KNOB", "one preset per ONE KNOB mode"),
])}
<h3>MY SLOTS</h3>
<p>Six keys next to the preset name hold your own settings, shared by every RONE DMT in every project:</p>
{table(["Action", "Result"], [
 ("Hold a key (about 0.7 s)", "Stores the current settings in it."),
 ("Tap a key", "Recalls it."),
 ("The small x or a right-click", "Clears it."),
])}
<p>A slot keeps the five knobs, SYNC, ONE KNOB and OUTPUT - never the sample or the NOTE.</p>
</div></div>
"""})

    S.append({"title": "Step-by-step workflows", "sub": "recipes", "html": f"""
{recipe("Half-time vocal", "slower, same pitch",
 steps([
  "Load the vocal, preset HALF TIME (SPEED 50 %).",
  "Raise ATTACK for a smoother, more blurred sound, lower it for a grittier one.",
  "Add PAN 20-40 % for width.",
  "Drag the take onto an audio track.",
 ]))}
{recipe("A pad from one word", "freeze",
 steps([
  "Preset FROZEN PAD, or ONE KNOB in FREEZE turned left.",
  "SPEED near 0 % holds one spot of the sample; long ATTACK keeps it soft.",
  "Use NOTE to put the pad on the key of the track.",
 ]))}
{recipe("Grain stutter on the grid", "rhythm",
 steps([
  "Switch SYNC on and pick a HOLD value: 1/16 or 1/32.",
  "Short ATTACK (5-10 ms) for hard edges; SPACING 150-200 % for gaps.",
  "Program the MIDI notes where the stutter should start - every note restarts the sample.",
 ]))}
{recipe("Backwards words", "readable reverse",
 steps([
  "Preset BACKWARDS WORDS (SPEED -100 %).",
  "Each grain still plays forward, so syllables stay readable while the phrase runs backwards.",
  "Longer HOLD keeps whole syllables; shorter HOLD turns it into texture.",
 ]))}
{recipe("A riser from any sound", "ONE KNOB automation",
 steps([
  "Switch ONE KNOB on, mode BLOOM or TUNNEL.",
  "Automate the big knob from left to right over 4-8 bars.",
  "Ride OUTPUT up a few dB at the end.",
 ]))}
"""})

    conv = G["conventions_section"](m)
    for a, b in (("when there is no DAW clock, tempo-synced controls follow the plugin's own BPM setting.", "with no DAW clock RONE DMT runs at 120 BPM (SYNC values and the OUTPUT grid follow it). Play it from PLAY or a MIDI keyboard."),
                 ("<td>Resets it to the default value.</td>", "<td>Resets it: ATTACK 40 ms, HOLD 5 ms, SPACING 100 %, SPEED 100 %, PAN 0 %, OUTPUT 0 dB.</td>")):
        assert a in conv["html"], a
        conv["html"] = conv["html"].replace(a, b)
    S.append(conv)
    S.append(G["support_section"](m, [
        ("No sound", "Load a sample first (a new instance is empty), then play a MIDI note or press PLAY. RONE DMT is an instrument: put it on an instrument track, not on an audio track's effects."),
        ("Every note sounds the same", "That is the design: a note starts the sample from the top, the key does not change the pitch. To play the sample on different notes, turn on NOTE and MIDI."),
        ("The drop does nothing", "Drop on the SOURCE view, or double-click it to choose the file. Very large files (over 64 MB) must be opened by double-click."),
        ("NOTE does not change the sound yet", "The first use analyses the sample in the background; the key blinks until its note is ready. Samples over the analysis limit show a message instead."),
        ("It gets loud with long, dense grains", "Many overlapping grains add up. Raise SPACING, shorten ATTACK / HOLD, or lower OUTPUT."),
        ("A preset name has a * after it", "You changed something since loading it. Load it again to go back, or store your version in MY SLOTS."),
    ]))

    m["sections"] = S
    return m
