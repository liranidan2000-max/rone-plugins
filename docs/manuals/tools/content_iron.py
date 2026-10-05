def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    m = {
        "id": "iron", "product": "RONE Iron", "eyebrow": "DROP A VOCAL. PLAY METAL.",
        "title_html": "RONE <i>Iron</i>", "accent": "#B530D6", "version": "1.0.0",
        "tagline": "Drop any vocal and it becomes a metallic chop instrument: flattened to the note you pick, sliced onto your keys, rung through a comb tuned to that note and thrown from left to right. Play it from the piano roll, or let the GROOVE write the pattern.",
        "formats": ["VST3", "AU", "Standalone"], "vst3": "RONE Iron.vst3", "au": "RONE Iron.component", "exe": "RONE Iron.exe",
        "pdf": "RONE Iron - User Manual.pdf", "cover_img": "iron/main.png",
    }
    S = []

    S.append({"title": "Welcome to RONE Iron", "sub": "a whole vocal-chop chain in one instrument", "html": f"""
<p><strong>RONE Iron</strong> does in one step what usually takes four plugins and an afternoon: it <strong>flattens</strong> a vocal to a single note (the formants stay, so it still sounds like a voice), <strong>slices</strong> it into one-shot chops on its transients and lays them out on your MIDI keys, rings every chop through a <strong>comb tuned to that note</strong> so the voice turns to metal, and <strong>pans</strong> each chop to its own side so the hits jump from one speaker to the other.</p>
<p>The result is the chopped, metallic, left-right vocal you hear in psytrance and festival drops - in tune with your track, from any acapella, in seconds.</p>
<div class="two">
<div>
<h3>Where it shines</h3>
<ul>
<li><strong>Drop hooks</strong> - a rhythmic metal vocal on the note of your bassline.</li>
<li><strong>Build-ups</strong> - ride the macros CHOP and MOTION up into the drop.</li>
<li><strong>Leads from a voice</strong> - ARP turns the chops into a melody over a chord.</li>
<li><strong>Breaks and intros</strong> - long rings and soft pans for a hypnotic bed.</li>
</ul>
</div>
<div>
<h3>The 10-second version</h3>
{steps([
 "Insert Iron as an instrument on a MIDI track.",
 "Drag a vocal onto the plugin.",
 "Click the note of your track on the piano.",
 "Press GROOVE and play the song - or play the keys."])}
</div></div>
{note("Your vocal travels with the project", "Iron keeps the vocal you dropped inside your project (losslessly compressed), with its chops, pans and groove edits. Moving or deleting the original file never breaks a saved project.")}
"""})

    S.append(G["install_section"](m))

    S.append({"title": "Quick start", "sub": "your first metal vocal", "html": f"""
{img("iron/empty.png", "<b>A fresh RONE Iron.</b> Drop a vocal or any sample anywhere on the card, or press the folder button.", "w80")}
{steps([
 "<strong>Insert Iron</strong> on an instrument (MIDI) track. In FL Studio: Channel rack, + , RONE Iron.",
 "<strong>Drop a vocal - or any sample</strong> onto the card - WAV, AIFF, FLAC, MP3 or OGG, from your DAW's browser or from Explorer / Finder. The first 60 seconds are used. A few seconds later the chops are on the keys.",
 "<strong>Pick the note</strong> of your track on the piano (<span class='pill'>A</span> by default). The whole vocal is flattened to the octave of that note nearest the voice, and the comb follows it.",
 "<strong>Play.</strong> Every chop has a key, starting at C5 (FL Studio's naming; middle C = C5). Click a chop on the screen to hear it.",
 "<strong>Or let it play itself:</strong> press <span class='pill'>GROOVE</span> and start your song. Iron plays a 16th-note pattern of chops on the host tempo. <span class='pill'>DRAG MIDI</span> drops that pattern into your piano roll as notes.",
 "<strong>Try the presets</strong> at the top and the four macros next to them. They change the whole sound, not one knob.",
])}
{note("Right-click any control", "Right-click a knob for the host's own menu - in FL Studio that is where you create an automation clip or link it to a controller.")}
"""})

    S.append({"title": "Interface tour", "sub": "every element on screen", "html": f"""
{img("iron/tour.png", "The header and the vocal card.")}
{legend([
 ("Header logo", "click to flip to the back panel (About, version, licence)"),
 ("Preset browser", "15 presets in six groups; arrows step through them, the name opens the list"),
 ("Macros", "METAL, CHOP, SPACE, MOTION - four knobs that move the whole sound"),
 ("MIDI / tempo", "the MIDI light flashes with incoming notes; the host tempo the GROOVE follows"),
 ("Vocal", "the file you dropped and its length"),
 ("AUTO SLICE", "SMALL (fewer, longer chops), MEDIUM, LARGE (many more)"),
 ("Chops / keys", "how many chops, and the keys they sit on"),
 ("Chop readout", "the chop under the mouse or playing: its number, key, length and pan"),
 ("BROWSE / EJECT", "open a file, or empty the plugin"),
 ("Chop map", "the flattened vocal with every slice marked; click a chop to hear it"),
 ("Pan lane", "one bar per chop: up = left, down = right; drag a bar to pan by hand"),
 ("Chop keys", "the keyboard of chops; the lit key is the one playing"),
])}
{img("iron/play.png", "The middle: the note, the metal and the pans.")}
{legend([
 ("NOTE", "the one note the whole vocal is flattened to"),
 ("OCTAVE", "move that note an octave down or up (0 = nearest the vocal)"),
 ("MIDI NOTE", "lets the octave under the chops on your keyboard choose the note"),
 ("ARP", "lets you light several notes; every chop then takes one of them"),
 ("ARP order", "UP, DOWN, UP-DOWN or FOLLOW, over 1 or 2 octaves"),
 ("Note readout", "the vocal's own pitch, the note it goes to, and its frequency"),
 ("Comb readout", "where the comb is tuned and how far apart its teeth are"),
 ("IRON", "how much metal: the tuned comb's share of the sound"),
 ("FADE OUT", "shortens every chop"),
 ("RING", "how long the metal rings"),
 ("AUTO PAN", "a new left-right pattern for every chop"),
 ("PAN STYLE", "PING-PONG, SPREAD or WIDE"),
 ("RANGE", "how far to the sides the pans go"),
 ("Pan meter", "where the chop that plays sits right now"),
])}
{img("iron/foot.png", "The footer.")}
{legend([
 ("ADVANCED", "opens the voice, comb, flatten, sound and key settings"),
 ("GROOVE", "the automatic pattern on or off"),
 ("Step strip", "the pattern at a glance; EDIT opens the GROOVE panel"),
 ("DRAG MIDI", "drag the pattern into your piano roll as MIDI notes"),
 ("Groove readout", "hits in the pattern, locked steps and the pattern's name"),
 ("MIX", "from the full effect back towards the plain chops"),
 ("NOTE", "makes MIX keep the flattened note (only the metal fades)"),
 ("OUT", "output level"),
 ("SAFE", "the output ceiling; green while it is on"),
 ("Resize grip", "drag to resize the window"),
])}
"""})

    S.append({"title": "The vocal, the note and the metal", "sub": "controls reference", "html": f"""
{img("iron/tour_vocal.png", "<b>The vocal card.</b> The slices on the waveform, the pan of every chop under it, and the keys they play from.", "")}
{ctl("Drop / BROWSE", "WAV, AIFF, FLAC, MP3, OGG<br>up to 60 s", "<p>Drop a file on the card or press the folder button. Iron reads the first 60 seconds from the first sound, finds the vocal's pitch, flattens it to your note, finds the transients and builds the chops - usually in well under a second per ten seconds of audio. EJECT empties the plugin. Breaths, S sounds and other noises stay on the keys, but the GROOVE and the ARP never pick them.</p>")}
{ctl("AUTO SLICE", "SMALL / MEDIUM / LARGE<br>default MEDIUM", "<p>How finely the vocal is cut. SMALL gives fewer, longer chops (whole words); LARGE gives many more short ones (syllables). DETAIL in ADVANCED fine-tunes it between the three. At most 128 chops; when there are more chops than keys above ROOT, the key map moves down to the highest C where every chop has a key.</p>", "Clicking a chop on the waveform or on the keys plays it.")}
{ctl("NOTE", "C to B<br>default A", "<p>The note the whole vocal is flattened to, in the octave nearest the voice. Pick the key of your track (the root of your bassline). Changing it re-renders the vocal in the background; the old chops keep playing until the new ones are ready.</p>", "The mouse wheel over the piano steps the note.")}
{ctl("OCTAVE", "-2 to +2<br>default 0", "<p>Moves the target note whole octaves down or up from the octave nearest the vocal. Big jumps change the character - down is darker and more robotic.</p>")}
{ctl("MIDI NOTE", "on / off<br>default off<br>automatable", "<p>With MIDI NOTE on, the twelve keys right below the first chop key (with the chops starting at C5, that is C4 to B4) choose the NOTE instead of playing chops. Hold a key with your left hand while the GROOVE runs and the whole vocal follows - you can play the groove like an instrument. The switch is instant: Iron prepares the other notes in the background.</p>")}
{ctl("IRON", "0 to 100 %<br>default 34 %<br>automatable", "<p>The amount of metal. The comb sits on the note: its first notch is the note itself and its teeth repeat on the note's harmonics, so the ring is always in key. At 0 % you hear the flattened chops alone.</p>", "Drag, or use the mouse wheel (Shift = fine). Double-click = default.")}
{ctl("RING", "0.10 to 3.00 s<br>default 0.70 s<br>automatable", "<p>How long the comb rings after each chop - short for tight, clanking hits, long for a shimmering, bell-like tail.</p>")}
{ctl("FADE OUT", "0 to 100 %<br>default off<br>automatable", "<p>Shortens every chop. At 0 each chop plays to its end; turning it up fades every chop out earlier, for tighter, more percussive hits. Automate it to open the chops up over a build.</p>")}
{ctl("AUTO PAN, PAN STYLE, RANGE", "PING-PONG / SPREAD / WIDE<br>RANGE 0 to 100 %, default 80 %<br>automatable", "<p>Every chop has its own pan. AUTO PAN rolls a new set; chops you panned by hand keep theirs. PING-PONG alternates hard left and right in the order the hits play (the classic left-right explosion), SPREAD places them at random but never three in a row on one side, WIDE pushes them hard and wide. RANGE scales how far out they go.</p>", "Drag a bar in the pan lane to pan one chop by hand. Double-click a bar to give it back to AUTO.")}
{ctl("MIX and NOTE", "MIX 0 to 100 %, default 100 %<br>NOTE on / off<br>automatable", "<p>MIX blends from the full effect (100 %) towards the plain chops: at 0 % you hear the untouched vocal chops, with the same slices, pans and fades. Turn on <b>NOTE</b> and MIX only takes the metal away - at 0 % you hear the vocal flattened to your note, without the comb. Use MIX to bring back more of the original voice.</p>")}
{ctl("OUT and SAFE", "OUT -24 to +12 dB<br>SAFE on by default", "<p>OUT sets the level. SAFE is a ceiling at -0.3 dBFS on the output: it never touches normal levels and only catches the peaks that a long ring or many voices can pile up. The lamp next to OUT is green while SAFE is on.</p>")}
"""})

    S.append({"title": "ARP", "sub": "a melody from one vocal", "html": f"""
{img("iron/arp.png", "<b>ARP on</b> with A, C and E lit: the chops play a minor triad, with one comb per note.", "w80")}
{legend([
 ("ARP", "on / off"),
 ("Chord notes", "click keys to add or remove notes; the NOTE is always one of them"),
 ("Order", "UP, DOWN, UP-DOWN or FOLLOW"),
 ("Octaves", "the notes in one octave, or continued into a second"),
 ("Note readout", "the notes in use"),
 ("Combs", "one comb per note, each tuned to its own note"),
])}
<p>With ARP on, every chop takes one of the lit notes. In <b>UP</b>, <b>DOWN</b> and <b>UP-DOWN</b> the chops walk through the notes in order - chop 1 on the first note, chop 2 on the next and so on - so a GROOVE or a run of keys becomes an arpeggio. <b>FOLLOW</b> works differently: each chop goes to the lit note nearest its own pitch, so the vocal keeps its melody but snaps to your chord or scale. The <b>x</b> clears the extra notes. 2 OCT continues the order an octave higher (FOLLOW ignores it).</p>
{note("Light the scale, use FOLLOW", "Light the notes of your track's scale and choose FOLLOW: the chops stay in key but keep the shape of the original singing - the best way to keep two different vocals sounding different.")}
"""})

    S.append({"title": "GROOVE", "sub": "the automatic pattern", "html": f"""
{img("iron/groove.png", "<b>The GROOVE panel</b> (EDIT in the footer). Each card is a 16th note: the chop it plays, its length bar and its marks.")}
{legend([
 ("GROOVE", "on / off - it plays the moment it is on, and follows your song while the DAW plays"),
 ("BARS", "a 1-bar or 2-bar pattern"),
 ("REROLL", "a new pattern; locked steps keep theirs"),
 ("History", "back to an earlier roll and forward again (32 kept)"),
 ("LOCK STEPS", "freeze every hit as it plays now"),
 ("CLEAR EDITS", "every step back to the generator"),
 ("Pattern", "16 researched patterns, or CUSTOM"),
 ("DRAG MIDI", "drag the pattern into your piano roll"),
 ("Steps", "16 cards per bar; hover a card for its controls"),
 ("Density / chops / variation", "how many hits, how many different chops, how different bar 2 is"),
 ("Length / hold / flip / rolls", "how long the hits are, how many ring on, pan flips and 1/32 rolls"),
])}
{img("iron/groove_steps.png", "<b>Step cards.</b> The power button switches a step on or off, the lock freezes it, and the arrows that appear on hover choose the previous or next chop. Marks show a pan flip, a roll, or a step whose chop no longer exists.", "w60")}
{ctl("Patterns", "16 + CUSTOM", "<p>RONE, HOOK, GALLOP, SYLLABLES, ROLLING, OFFBEAT, BACKBEAT, PROG STAB, TRESILLO, DOTTED, FLIPSIDE, CALL ANSWER, RATCHET, BUILDUP, HALFTIME and BREAKDOWN - patterns built around the kick and bass grid of psytrance, progressive and techno. A pattern fixes the steps; the knobs and the chop choice still apply. Edit the steps or REROLL and it becomes CUSTOM.</p>")}
{ctl("DENSITY", "0 to 100 %<br>automatable", "<p>How many of the 16 steps play (the readout counts them).</p>")}
{ctl("CHOPS", "0 to 100 %<br>automatable", "<p>How many different chops the pattern uses - a few for a hypnotic loop, many for a talking line. The GROOVE only picks strong, tonal chops.</p>")}
{ctl("VARIATION", "0 to 100 %<br>2 BARS only", "<p>How much bar 2 differs from bar 1.</p>")}
{ctl("LENGTH, HOLD", "LENGTH in 16ths<br>HOLD 0 to 100 %", "<p>LENGTH is how long each hit lasts, in 16th notes. HOLD lets a share of the hits ring on to the next one.</p>")}
{ctl("FLIP, ROLLS", "0 to 100 %", "<p>FLIP sends some hits to the other side, on top of the chop's own pan. ROLLS turns some hits into quick 1/32 repeats (2 to 4 hits, each a little quieter).</p>")}
<p><b>DRAG MIDI</b> (in the panel and in the footer) writes the pattern exactly as it plays, edits and rolls included, as a MIDI clip you drag into your piano roll. Turn GROOVE off and edit the notes like any other clip - Iron's keys play the same chops, so the clip sounds the same.</p>
{note("GROOVE plays at once, then follows the host", "Switch GROOVE on and the pattern plays straight away at your project's tempo, even with the DAW stopped. Press play and it locks to your song's bars; press stop and it stops with the song. The footer then reads STOPPED &middot; RUN: click it to run the pattern on its own again. A project saved with GROOVE on stays quiet until you play or click. In the standalone app GROOVE is play / stop on its own clock.")}
"""})

    S.append({"title": "Presets, macros, locks and slots", "sub": "the whole sound at once", "html": f"""
{img("iron/presets.png", "<b>The preset list.</b> The line under the list describes the preset under the mouse; MY SLOTS keep six sounds of your own.")}
{legend([
 ("Preset browser", "arrows step through the presets; the name opens the list"),
 ("Preset list", "SIGNATURE, STABS, ARP, SOFT, SPACE and FX"),
 ("Description", "what the preset is for"),
 ("MY SLOTS", "hold a slot to store the current sound, tap to recall it"),
 ("INIT", "every control back to its default"),
])}
<p>A preset sets the whole IRON sound: the metal, the fades, the pans, the ARP notes, the groove and what the four macros do. It never changes your vocal, your NOTE, OCTAVE or ROOT, or the pans you set by hand. <b>SIGNATURE</b>: IRON (the signature sound for the main drop) and FORGE (peak-time full-on). <b>STABS</b>: RAZOR, ANVIL, PISTON. <b>ARP</b>: TRIAD, ASCEND, MELODY, PHRYGIAN. <b>SOFT</b>: HUMAN, HALO. <b>SPACE</b>: MOLTEN, CATHEDRAL. <b>FX</b>: SHRAPNEL, IGNITION.</p>
{ctl("Macros", "METAL, CHOP, SPACE, MOTION<br>0 to 100 %<br>automatable", "<p>Each macro moves several controls at once, in the way the current preset defines: METAL adds metal and ring, CHOP tightens the hits, SPACE opens the rings and the stereo, MOTION adds pan movement, density and rolls. The macros never move the knobs themselves - the knobs show where a macro has taken them - so turning a macro back to 0 always returns exactly to the preset. Automate them into a drop.</p>", "Hover a macro to light the controls it moves.")}
{img("iron/locks.png", "<b>Locks.</b> The small locks next to IRON, RING and MIX: those controls keep their values when you switch presets.", "w80")}
{ctl("Locks", "12 lockable controls", "<p>Every lockable control has a small lock next to it, grey while it is open; click it to lock and it lights up. A locked control stays where it is when you switch presets, so you can audition presets around a metal amount or a groove you like. Lockable: IRON, RING, FADE OUT, MIX, LIFE, the pans, the ARP notes, the GROOVE and each of the four macros.</p>")}
{ctl("MY SLOTS", "6 slots", "<p>Hold a slot button to store the current sound in it, tap it to recall. Slots are kept on your computer, so they are there in every project and every instance.</p>")}
"""})

    S.append({"title": "ADVANCED", "sub": "voice, flattening, sound and keys", "html": f"""
{img("iron/adv.png", "<b>ADVANCED</b> open: the switches on top, three groups below.")}
{legend([
 ("VOICE", "MONO: a new chop cuts the last one; POLY: up to 16 ring together"),
 ("COMB", "the metal on or off"),
 ("SAFE", "the -0.3 dBFS output ceiling on or off"),
 ("FLATTEN + SLICE", "LIFE, FINE, DETAIL - these re-render the vocal"),
 ("SOUND", "TONE, LEVEL, LOW CUT - live"),
 ("KEYS", "ROOT, STEAL, VEL - live"),
])}
{ctl("LIFE", "0 to 100 %<br>default 15 %", "<p>How much of the vocal's own pitch movement survives the flattening. 0 % is a dead-straight robot note; more keeps the scoops and vibrato of the singer around the note.</p>")}
{ctl("FINE", "-50 to +50 cents<br>default 0", "<p>Tunes the target note in cents, for tracks that are not at A = 440 Hz.</p>")}
{ctl("DETAIL", "-1 to +1<br>default 0", "<p>Fine control over the slicing between the AUTO SLICE steps: right = more chops, left = fewer. The readout shows the new count.</p>")}
{ctl("TONE", "0 to 100 %<br>default 0<br>automatable", "<p>Darkens the metal: higher values damp the comb's upper teeth, for a softer, warmer ring.</p>")}
{ctl("LEVEL", "0 to 100 %<br>default 0<br>automatable", "<p>Evens out the chops' levels, so quiet syllables hit as hard as loud ones.</p>")}
{ctl("LOW CUT", "OFF, 20 to 400 Hz<br>default OFF<br>automatable", "<p>A high-pass on the output - useful when the vocal is flattened low (OCTAVE -1 or -2) and starts to fight the bass.</p>")}
{ctl("ROOT", "MIDI 0 to 127<br>default C5 (60)<br>automatable", "<p>The key of the first chop. The others follow upward, one chop per key, black keys included.</p>")}
{ctl("STEAL", "0 to 100 ms<br>default 20 ms", "<p>In MONO, how quickly a chop fades when the next one cuts it. Shorter is choppier; longer is smoother.</p>")}
{ctl("VEL", "0 to 100 %<br>default 0", "<p>How much the MIDI velocity sets each chop's level. At 0 every note plays at full level.</p>")}
"""})

    S.append({"title": "Step-by-step workflows", "sub": "recipes", "html": f"""
{recipe("The classic metal drop hook", "IRON preset, GROOVE",
 steps([
  "Drop an acapella and pick the root note of your bassline.",
  "Preset IRON. Press GROOVE and play the drop.",
  "REROLL until the pattern talks; lock the steps you love and REROLL the rest.",
  "DRAG MIDI the pattern into the piano roll if you want to edit it as notes.",
 ]))}
{recipe("Play it over a chord progression", "MIDI NOTE",
 steps([
  "Turn on GROOVE and MIDI NOTE.",
  "With the chops from C5, the keys C4 to B4 choose the note: hold the root of each chord while the groove runs.",
  "Record that left hand into the piano roll with the rest of the part.",
 ]))}
{recipe("A lead from a vocal", "ARP + FOLLOW",
 steps([
  "Turn on ARP and light the notes of your scale (or a triad).",
  "FOLLOW keeps the singer's melody snapped to your notes; UP or UP-DOWN turns the chops into an arpeggio.",
  "Try the ARP presets: TRIAD, ASCEND, MELODY, PHRYGIAN.",
 ]))}
{recipe("Build-up into the drop", "Macros",
 steps([
  "Preset IGNITION, or any preset you like.",
  "Automate CHOP and MOTION from 0 up over the last 8 bars, METAL up in the last two.",
  "Snap them back to 0 on the drop: the sound returns exactly to the preset.",
 ]))}
"""})

    S.append({"title": "Tips, tricks and troubleshooting", "sub": "", "html": f"""
<h3>Tips</h3>
<ul>
<li><strong>Clean acapellas work best.</strong> A dry vocal with little reverb slices into clearer chops and flattens more cleanly.</li>
<li><strong>Every vocal sounding alike?</strong> One note and one comb bring vocals closer together by design. Use ARP FOLLOW, pull MIX down a little, or raise LIFE.</li>
<li><strong>Too low and muddy?</strong> OCTAVE +1, or LOW CUT in ADVANCED.</li>
<li><strong>Tighter hits:</strong> FADE OUT and a shorter RING; MONO with a short STEAL.</li>
<li><strong>Bigger stereo:</strong> PAN STYLE WIDE, RANGE 100 %, FLIP up in the GROOVE.</li>
</ul>
<h3>Troubleshooting</h3>
{table(["Symptom", "Cause", "Fix"], [
 ["No sound from the keys", "No vocal loaded, or you play below the first chop key", "Drop a vocal; play from the ROOT key (C5 by default) upward"],
 ["Some low keys change the note instead of playing", "MIDI NOTE is on: the octave under the chops chooses the note", "Turn MIDI NOTE off, or play the chops from the ROOT key up"],
 ["The GROOVE is silent", "GROOVE is off, or the DAW's stop ended it", "Switch GROOVE on, press play in your DAW, or click STOPPED &middot; RUN in the footer"],
 ["Only the first part of my file is used", "Iron loads up to 60 seconds", "Trim the vocal to the part you want before dropping it"],
 ["The file is refused", "Not an audio file Iron can read, or no audio in it", "Use WAV, AIFF, FLAC, MP3 or OGG"],
 ["A short wait after changing NOTE, LIFE or the slicing", "The vocal is re-rendered for the new setting", "The old chops play until the new ones are ready"],
 ["It sounds too much like a robot", "LIFE is low and MIX is at 100 %", "Raise LIFE, or pull MIX down (NOTE on keeps the note)"],
 ["Peaks sound squashed", "SAFE is catching overs (many voices, long rings)", "Lower OUT, IRON or RING, or switch POLY to MONO"],
])}
"""})

    m["sections"] = S
    return m
