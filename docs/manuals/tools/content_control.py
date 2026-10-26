def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    m = {
        "id": "control", "product": "RONE Control", "eyebrow": "MOVE ANY KNOB WITH YOUR HAND.",
        "title_html": "RONE <i>Control</i>", "accent": "#2EA81E", "version": "1.0.0",
        "tagline": "Open the plugin and it shows a code. Your phone - or, on Windows, the computer's own camera - becomes the eye: pinch, lift, twist, and the knob you linked moves in your DAW, recorded as automation like any controller. No app to install, no MIDI keyboard, nothing to map by hand.",
        "formats": ["VST3", "AU"], "vst3": "RONE Control.vst3", "au": "RONE Control.component", "exe": None,
        "pdf": "RONE Control - User Manual.pdf", "cover_img": "control/tour_stage.png",
    }
    S = []

    S.append({"title": "Welcome to RONE Control", "sub": "your hand is the controller", "html": f"""
<p><strong>RONE Control</strong> is a controller plug-in: audio passes through it untouched, and its eight <strong>slots</strong> are knobs your DAW can link to anything. Each slot is driven by one move of your hand - lift it, slide it, turn it, open and close your fingers - seen by your phone's camera over Wi-Fi, or by the computer's camera on Windows. Link a slot to a filter cutoff, a send, a fader, and the knob follows your hand while you play. Press record and your DAW writes it as ordinary automation.</p>
<p>The plug-in is the whole program. There is no app to install on the phone (its browser does the work, and it stays paired after one scan), no driver to set up, and in FL Studio, Ableton Live, REAPER and Bitwig Studio no MIDI learn either: <strong>touch a knob, press LINK</strong>.</p>
<div class="two">
<div>
<h3>Where it shines</h3>
<ul>
<li><strong>Filter rides and build-ups</strong> - a hand that rises over eight bars is the most natural automation curve there is.</li>
<li><strong>Performing in the studio</strong> - two hands, four knobs, one take.</li>
<li><strong>Anything you would otherwise draw</strong> - sends, pans, dry/wet, a delay feedback that breathes.</li>
<li><strong>Every other RONE plugin</strong> - one pairing feeds them all.</li>
</ul>
</div>
<div>
<h3>The 10-second version</h3>
{steps([
 "Insert RONE Control on any track and open it.",
 "Scan the code with your phone, tap START, allow the camera.",
 "Touch a knob in your DAW, press LINK on slot A.",
 "Pinch in front of the camera and lift your hand."])}
</div></div>
{note("Nothing leaves your room", "The phone sends your hand's 21 points straight to this computer over your Wi-Fi, never through a server. The computer's camera runs the hand model inside the plug-in, offline. No picture is stored anywhere.")}
"""})

    S.append(G["install_section"](m))

    S.append({"title": "Quick start", "sub": "your first linked knob", "html": f"""
{img("control/phone.png", "<b>The first thing the plug-in shows:</b> a code for your phone. Same Wi-Fi as the computer; NEW CODE cuts off the old phone.", "w80")}
{steps([
 "<strong>Insert</strong> RONE Control on any track - a dedicated utility track is tidiest - and open its window.",
 "<strong>Scan the code</strong> with your phone's camera. The page that opens asks for the camera: tap <span class='pill'>START</span> and allow it. Stand the phone up facing you, or lay it flat, screen up, and play over it like a theremin. The screen stays awake while you play.",
 "<strong>Your hand appears on the stage</strong> and the status reads PHONE with the frame rate. From now on the phone stays paired: the next time you open any RONE Control it finds it by itself.",
 "<strong>Link slot A to a knob.</strong> In FL Studio, Ableton Live, REAPER or Bitwig Studio: touch the knob you want to move, then press <span class='pill'>LINK</span> on the card. The card shows what it holds. In any other DAW, LINK opens the step-by-step for the MIDI port (see <em>Linking to your DAW</em>).",
 "<strong>Pinch and lift.</strong> Slot A is CUTOFF in the FILTER RIDE preset: pinch your thumb and index finger to grab, move your hand up and down to turn, open them to let go - the knob stays where you left it.",
 "<strong>Record.</strong> Arm the track and record as you would with any hardware controller: the DAW writes the linked knob's own automation.",
])}
{note("No phone at hand?", "On Windows, choose <strong>CAMERA</strong> at the top and the computer's own webcam is the eye - the hand model runs inside the plug-in, offline. <strong>SIM</strong> makes the mouse a hand (hold = pinch, right button = fist, wheel = twist) so you can try every setting without a camera at all.")}
"""})

    S.append({"title": "Interface tour", "sub": "every element on screen", "html": f"""
{img("control/tour.png", "A right hand holding slot A (CUTOFF): the glove turns green while it grabs, the card reads HELD.")}
{legend([
 ("Header logo", "click to flip to the back panel (About, version, licence)"),
 ("Status", "where the hands come from and how fast they arrive - PHONE · 30 FPS, CAMERA · 30 FPS, or what is missing"),
 ("PHONE / CAMERA / SIM", "the eye: your phone, the computer's camera (Windows), or the mouse as a hand"),
 ("LIVE", "off = every slot holds its value and the hands are ignored"),
 ("The stage", "your hand as a glove - grey when free, green while it holds a knob; VIDEO / HAND shows the picture under it or the hand alone"),
 ("Slots", "one card per slot: the knob, its name, the value, the move and how it is held, LINK; + ADD SLOT makes another (8 at most)"),
 ("ADVANCED", "opens the selected slot's editor below the stage"),
 ("Presets", "FILTER RIDE, FREE HAND, SPRING BACK - three ways of moving"),
 ("PHONE", "shows the code again, or says the phone is in"),
 ("Resize grip", "drag to resize the window"),
])}
{img("control/adv.png", "ADVANCED: the slot editor, with a demo hand that plays this slot's own settings.")}
{legend([
 ("Slot name", "what the DAW sees ('A CUTOFF'); the card and the phone show it too"),
 ("UNDO MOVE", "puts the knob back where it was before the last grab"),
 ("LINK", "links this slot to the knob you touched in your DAW"),
 ("REMOVE", "takes the slot out of the rack (its parameter stays, at its value)"),
 ("The demo", "an animated hand playing the slot: grey = free, green = holding; the steps PINCH > MOVE > LET GO light up as it goes"),
 ("MOVE", "which move of the hand is the value: up/down, left/right, near/far, twist, pinch, grip, two hands"),
 ("HOLD THE KNOB WITH", "PINCH, FIST, or ALWAYS (no grab needed)"),
 ("WHEN YOU LET GO", "IT STAYS where you left it, or IT RETURNS home"),
 ("In one sentence", "what this slot does, in words - hover any option to preview it in the demo"),
 ("FINE TUNE", "HAND, MODE, CURVE and the fine knobs"),
])}
{img("control/fine.png", "FINE TUNE: which hand, how the knob takes over, the curve, and the fine knobs.", "w80")}
{legend([
 ("HAND", "LEFT, RIGHT or EITHER"),
 ("MODE", "GRAB: the knob moves from where it is as your hand moves. FOLLOW: it jumps to where your hand is"),
 ("CURVE", "how the hand's travel maps onto the knob: LIN, SOFT, EXP, S"),
 ("The fine knobs", "SMOOTH, REACH (how far the hand travels for the full range), RANGE LOW and HIGH, HOME, and the slot's MIDI CC"),
])}
"""})

    S.append({"title": "Controls reference", "sub": "what every control does", "html": f"""
<h3>The eye</h3>
{ctl("PHONE", "the default", "<p>The plug-in shows a code; the phone's browser opens a page that tracks your hand and sends its 21 points to this computer over your Wi-Fi - straight to the computer, never through a server. One scan pairs it: the pairing is kept on this computer, and every RONE Control (and every other RONE plug-in with hands) uses it. <span class='pill'>NEW CODE</span> makes a fresh pairing and cuts off the old phone.</p>", "Same Wi-Fi as the computer. If the two cannot see each other directly (a guest network, an isolated Wi-Fi) the hands are relayed through our server with a little more delay - the status says RELAY. The picture never is.")}
{ctl("CAMERA", "Windows only", "<p>The computer's own webcam. The hand model (Google's MediaPipe, carried inside the plug-in) runs right here, offline. Pick the camera in the list, see yourself on the stage with <span class='pill'>VIDEO</span>, aim it so your whole hand is in view 50-80 cm away, in good light.</p>", "A virtual camera with no app behind it (Camo, OBS, NVIDIA Broadcast...) sends a frozen picture and reads NO PICTURE - choose the real camera. On a Mac a plug-in cannot own the camera: use the phone.")}
{ctl("SIM", "", "<p>The mouse is the hand: move it over the stage, hold the left button to pinch, the right button to make a fist, the wheel to twist (Shift + wheel: near / far). Try every setting without a camera.</p>")}
{ctl("VIDEO / HAND", "per computer", "<p>On the stage: your picture under the hand, or the hand alone. The picture flows only while a window shows VIDEO.</p>")}
{ctl("LIVE", "on / off<br>automatable", "<p>Off: the slots hold their values and the hands are ignored - a safe switch for playback. The LIVE parameter can be automated, so a section of the song can take the hands away.</p>", "An offline render (export) always plays what is written: a hand in front of the camera never changes a bounce.")}
<h3>A slot</h3>
{img("control/tour_rack.png", "<b>The cards.</b> The knob and its value, HELD while your hand has it, what the slot holds in the DAW (or the move and how it is held), LINK; + ADD SLOT for the next one.", "w60")}
{ctl("MOVE", "UP / DOWN · LEFT / RIGHT · NEAR / FAR · TWIST · PINCH · GRIP · 2 HANDS", "<p>Which move of the hand is the value. <b>UP / DOWN</b>, <b>LEFT / RIGHT</b> and <b>NEAR / FAR</b> are the hand's position in the picture; <b>TWIST</b> turns the hand like a knob; <b>PINCH</b> is the gap between thumb and finger; <b>GRIP</b> is how closed the hand is (open = bottom, fist = top); <b>2 HANDS</b> is the distance between both hands.</p>")}
{ctl("HOLD THE KNOB WITH", "PINCH / FIST / ALWAYS<br>default PINCH", "<p>What grabs the knob. <b>PINCH</b>: thumb and index together. <b>FIST</b>: a closed hand. <b>ALWAYS</b>: no grab - the knob follows the hand whenever it is in view (the FREE HAND preset). PINCH and GRIP moves need no hold: the fingers are the value.</p>")}
{ctl("WHEN YOU LET GO", "IT STAYS / IT RETURNS<br>default IT STAYS", "<p><b>IT STAYS</b> leaves the knob where you released it. <b>IT RETURNS</b> glides it back to HOME - a filter that falls back shut, a send that fades out by itself (the SPRING BACK preset).</p>")}
{ctl("HAND", "LEFT / RIGHT / EITHER", "<p>Which hand drives this slot. With two hands in view, give the left one CUTOFF and the right one RESO.</p>")}
{ctl("MODE", "GRAB / FOLLOW<br>default GRAB", "<p><b>GRAB</b> is relative: the knob moves from where it is by as much as your hand moves - it never jumps, and REACH sets how far a full turn is. <b>FOLLOW</b> is absolute: the knob jumps to where the hand is.</p>")}
{ctl("CURVE", "LIN / SOFT / EXP / S", "<p>How the hand's travel maps onto the knob: LIN is straight, SOFT and EXP give more precision at the bottom, S at both ends.</p>")}
{ctl("Fine knobs", "SMOOTH · REACH · RANGE LOW · RANGE HIGH · HOME · MIDI CC", "<p><b>SMOOTH</b>: steadies a shaky hand - TIGHT follows every tremor, SILK glides. <b>REACH</b>: how far the hand travels for the full range of the knob (SHORT, MID, LONG). <b>RANGE LOW / HIGH</b>: the part of the knob's range the hand covers. <b>HOME</b>: where IT RETURNS goes. <b>MIDI CC</b>: the control this slot plays on the RONE Control port - LINK sets it for you; change it only for a hand-made MIDI learn.</p>")}
{ctl("Presets", "FILTER RIDE / FREE HAND / SPRING BACK", "<p>Three ways of moving, loaded into the slots. <b>FILTER RIDE</b>: pinch and move - height = CUTOFF, side = PAN, twist = RESO. <b>FREE HAND</b>: no grab, the knobs follow the hand. <b>SPRING BACK</b>: everything returns home when you let go. A preset keeps a linked slot's knob, so switching the way of moving never unlinks anything.</p>")}
{ctl("Right-click", "", "<p>Right-click a card for the host's own parameter menu - in FL Studio that is where you create an automation clip or link the slot by hand.</p>")}
"""})

    S.append({"title": "Linking to your DAW", "sub": "LINK in FL, Live, REAPER and Bitwig - the MIDI port everywhere else", "html": f"""
{img("control/link.png", "<b>The LINK helper</b> opens from a card: the steps for your DAW, and SEND for any other.", "w80")}
<h3>FL Studio</h3>
{steps([
 "Install RONE Control with the Center and <strong>start the Center before FL Studio</strong> once. The Center keeps the <em>RONE Control</em> MIDI port open and puts the plug-in's controller script where FL finds it; FL attaches it the next time it starts.",
 "In FL, <strong>touch the knob</strong> you want to move - a plug-in knob, a mixer fader, a channel's volume.",
 "Press <span class='pill'>LINK</span> on the slot. The card shows the knob's name (and its plug-in or mixer track). For a plug-in's knob FL may open its own link window: press <em>Accept</em> there (or turn on Options &rarr; MIDI settings &rarr; <em>Auto accept detected controller</em> once).",
 "Record with the track armed: FL writes the linked knob's automation, as with any hardware controller. The link is saved in the project.",
])}
{note("Why the Center first", "FL attaches a controller script only to a MIDI device it meets when it starts. The RONE Plugins Center starts with Windows and keeps the RONE Control port open from login, so FL always meets it. Without the Center, LINK still works on mixer faders, and a plug-in's knob links through FL's own window.")}
<h3>Ableton Live</h3>
{steps([
 "Once: Settings &rarr; Link, Tempo &amp; MIDI &rarr; a free <em>Control Surface</em> slot &rarr; <strong>RONE_Control</strong>, Input <strong>RONE Control</strong>. Restart Live once (Live finds a new control surface only when it starts).",
 "Click the knob in Live (a device's knob, a mixer control). A knob turned inside a plug-in's own window counts too - the one that moved last.",
 "Press <span class='pill'>LINK</span>. Live moves the knob from the slot and records it as the knob's own automation; the link is kept in the Set.",
])}
<p>Only parameters Live shows can be linked: for a big plug-in, add the knob with Live's <em>Configure</em> button first - Live's own rule for every controller.</p>
<h3>REAPER</h3>
{steps([
 "Touch the knob (any FX parameter on any track).",
 "Press <span class='pill'>LINK</span>. On RONE Control's own track the knob is linked through REAPER's parameter modulation - full resolution, no MIDI at all. On another track it is REAPER's MIDI learn on the slot's control; once, enable <em>RONE Control</em> under Preferences &rarr; MIDI Inputs with <em>Enable input for control messages</em> - the card says so when it is missing.",
])}
<h3>Bitwig Studio</h3>
{steps([
 "Once: Settings &rarr; Controllers &rarr; Add Controller &rarr; RONE PLUGINS &rarr; <strong>RONE Control</strong> (the port is filled in).",
 "Click the knob (Bitwig's <em>last clicked parameter</em>), press <span class='pill'>LINK</span>. The link is saved with the project.",
])}
<h3>Every other DAW: the MIDI port</h3>
<p>RONE Control is also a MIDI controller called <strong>RONE Control</strong>, which every DAW can learn from. Each slot plays one CC on it. The LINK helper walks you through it: press <span class='pill'>SEND</span> and for 12 seconds only that slot sends its CC - at its current value, so nothing moves - then open your DAW's MIDI learn on the knob and it picks exactly this slot. On Windows the port needs the free loopMIDI driver (the helper offers it); on a Mac it is built in.</p>
{note("One link per computer", "Two DAWs open at once (or a DAW that runs plug-ins in separate processes) share the hands: the first RONE Control to start owns the camera or phone, the others take their hands from it and read THROUGH YOUR OTHER DAW. When the first closes, the next takes over by itself.")}
"""})

    S.append({"title": "Step-by-step workflows", "sub": "recipes", "html": f"""
{recipe("The filter ride", "FILTER RIDE, one hand",
 steps([
  "FILTER RIDE. Link slot A (CUTOFF) to a filter's cutoff and slot C (RESO) to its resonance.",
  "Pinch, lift slowly over the last eight bars, open your fingers on the drop: the cutoff stays open.",
  "Pinch and twist the hand: resonance. Let go and RESO springs back - it is set to IT RETURNS.",
 ]))}
{recipe("Two hands, two knobs", "HAND = LEFT / RIGHT",
 steps([
  "Give slot A HAND = LEFT and slot B HAND = RIGHT in FINE TUNE.",
  "Link A to a reverb send and B to a delay feedback.",
  "Both hands in view: each drives its own knob, independently.",
 ]))}
{recipe("A send that falls back by itself", "SPRING BACK",
 steps([
  "SPRING BACK, or set WHEN YOU LET GO to IT RETURNS on one slot and its HOME to 0.",
  "Link it to a send. Pinch and lift for the fill; open your fingers and the send glides back to silence.",
 ]))}
{recipe("Record it", "any DAW",
 steps([
  "Arm the track and record as with a hardware controller: in FL, Live, REAPER and Bitwig the DAW writes the linked knob's own automation.",
  "In other DAWs the automation lands on the MIDI-learned knob, as with any controller.",
  "Or automate RONE Control's own slot parameters (A CUTOFF, B PAN...) and link afterwards - the slots are ordinary parameters.",
 ]))}
"""})

    S.append({"title": "Tips, tricks and troubleshooting", "sub": "", "html": f"""
<h3>Tips</h3>
<ul>
<li><strong>Light on your hand.</strong> The hand model wants your whole hand in view, 50-80 cm from the camera, lit from the front. The stage shows MORE LIGHT and KEEP YOUR WHOLE HAND IN VIEW when it matters.</li>
<li><strong>GRAB never jumps.</strong> A grab starts from where the knob is - so a knob you moved with the mouse, or automation left somewhere, is picked up exactly there.</li>
<li><strong>REACH is your resolution.</strong> A long REACH means a small hand move for a small change; a short one gets the full range in a flick.</li>
<li><strong>SMOOTH for a steady hand.</strong> More smoothing steadies a shaky picture at the cost of a little delay.</li>
<li><strong>UNDO MOVE</strong> on the slot puts the knob back where it was before the last grab.</li>
<li><strong>The phone screen stays awake</strong> while it tracks. Lock it when you are done - the pairing is kept.</li>
</ul>
<h3>Troubleshooting</h3>
{table(["Symptom", "Cause", "Fix"], [
 ["WAITING FOR THE PHONE", "The phone is not on the same network, or the page was closed", "Same Wi-Fi as the computer; open the code again from PHONE and tap START"],
 ["PHONE · RELAY", "No direct path between phone and computer (guest Wi-Fi, an isolated network)", "The hands still work, with a little more delay; a shared Wi-Fi makes it direct"],
 ["NO CAMERA / BLOCKED BY WINDOWS", "Windows denies desktop apps the camera, or another app holds it", "Settings &rarr; Privacy &rarr; Camera &rarr; let desktop apps use it; close the other app; TRY AGAIN"],
 ["NO PICTURE", "A virtual camera with nothing behind it (Camo, OBS...)", "Choose the real camera in the list"],
 ["The knob does not move", "LIVE is off, the slot is not linked, or ARMED elsewhere", "Turn LIVE on; press LINK; with several RONE plug-ins open, the one in front gets the hands"],
 ["LINK says TOUCH A KNOB FIRST", "Nothing was touched in the DAW since the last link", "Move the knob you want, then press LINK"],
 ["LINKED · NO NAME (FL)", "FL met the RONE Control port after it started", "Start the RONE Plugins Center before FL Studio once"],
 ["LINK is not offered in my DAW", "Only FL Studio, Live, REAPER and Bitwig have a direct link", "SEND, then your DAW's MIDI learn on the knob"],
 ["Only slot A works", "LITE: without a licence slot A is free, the rest need RONE Control or ALL ACCESS", "Sign in in the RONE Plugins Center"],
 ["The first grab after linking did nothing (FL)", "FL's takeover mode waits for the controller to pass the knob's value", "Grab again - the plug-in steps across the value for you from then on"],
])}
"""})

    m["sections"] = S
    return m
