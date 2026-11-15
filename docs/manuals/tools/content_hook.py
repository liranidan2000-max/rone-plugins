import re


def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    m = {
        "id": "hook", "product": "RONE Hook", "eyebrow": "THE HOOK CATCHER",
        "title_html": "RONE <i>Hook</i>", "accent": "#FF2E4D", "version": "1.0.0",
        "tagline": "Search YouTube or paste a link, get the voice alone - music and sounds out - with every word on screen, and drag the hook straight into your DAW.",
        "formats": ["VST3", "Standalone"], "vst3": "RONE Hook.vst3", "exe": "RONE Hook.exe",
        "pdf": "RONE Hook - User Manual.pdf", "cover_img": "hook/main.png",
        "channels": "Stereo out (the voice you listen to); exports 44.1 kHz 24-bit WAV",
        "latency": "None added to the host; the voice is made outside the DAW's audio",
        "engine": "JUCE 8, WebView interface (WebView2) &middot; voice separation in its own process (onnxruntime + DirectML: any graphics card, the processor when there is none)",
    }
    S = []

    S.append({"title": "Welcome to RONE Hook", "sub": "a voice out of any video", "html": f"""
<p><strong>RONE Hook</strong> catches vocal hooks. Search YouTube from inside the plugin, paste a link, or type a word somebody says - HOOK downloads the sound, takes the <strong>voice</strong> out of it with an AI model, and lays it on a waveform with every word of the video under it. Mark the line you want, shape it with a few knobs, and drag it into your project.</p>
<ul>
<li><strong>Find it without leaving the DAW</strong> - type to search YouTube, or add <em>a word they say</em>: HOOK finds every moment those words are said in the results.</li>
<li><strong>The voice alone</strong> - an AI vocal model separates the voice from the music, on the graphics card when there is one. The start plays while the rest is still being made.</li>
<li><strong>Every word on screen</strong> - the video's words as one flowing text: click a word to hear it, shift-click to mark from word to word, FIND any word.</li>
<li><strong>Ready for the track</strong> - clean-up knobs, CHARACTER colours, ONE NOTE, FIT to your key and tempo, word CHOPS with MIDI, several MARKS at once.</li>
<li><strong>The HOOK BANK</strong> - keep the hooks you like for any project.</li>
</ul>
<p><strong>RONE Hook is free.</strong> No licence and no plan are needed to use it.</p>
<div class="two">
<div>
<h3>Where it shines</h3>
<ul>
<li><strong>Vocal hooks</strong> - a sung line from an acapella or a live session, on your key and tempo.</li>
<li><strong>Spoken drops</strong> - a sentence from a talk or a speech, cleaned and on the grid.</li>
<li><strong>Chops</strong> - every word of a line on its own key, ready to play.</li>
<li><strong>Collecting</strong> - a bank of hooks you can reach from any project.</li>
</ul>
</div>
<div>
<h3>The 10-second version</h3>
{steps([
 "Open RONE Hook (on any track, or the standalone app).",
 "Type a search, or press PASTE LINK with a YouTube link copied.",
 "Wait a few seconds: the voice appears.",
 "Drag across the line you want, then drag the DRAG TO DAW tile onto a track."])}
</div></div>
{note("Use what you have the right to use", "RONE Hook takes audio from videos that belong to other people. <strong>You are responsible for having the rights to anything you take and use</strong> - your own material, material you have permission for, or material whose licence allows it. The <strong>SAFE TO USE &middot; CC</strong> filter shows videos the uploader marked Creative Commons; that is the uploader's claim, so check it, and credit the owner.")}
"""})

    S.append({"title": "Installation with the RONE Plugins Center", "sub": "Center, the runtime, file locations", "html": f"""
<p>RONE Hook is installed and updated through the <strong>RONE Plugins Center</strong>, like every RONE product. It is free: it needs no licence and no plan.</p>
{img("center/tour.png", "<b>RONE Plugins Center.</b> 1 Navigation &middot; 2 Your account &middot; 3 Sync status &middot; 4 Search &middot; 5 Refresh &middot; 6 Settings &middot; 7 Updates summary &middot; 8 UPDATE ALL &middot; 9 Filters &middot; 10 Sort &middot; 11 A plugin card &middot; 12 Card menu (Details, Open, Manual) &middot; 13 OPEN the standalone app &middot; 14 INSTALL &middot; 15 Your plan", "w80")}
<h3>Step by step</h3>
{steps([
 "Download the Center from <a href='https://roneaudio.com'>roneaudio.com</a> and run <code>RONE_Plugins_Center_Installer.exe</code>. It also adds the Microsoft WebView2 runtime if your PC does not have it yet; the plugin's interface needs it.",
 "Open the Center, find <strong>RONE Hook</strong> and press <span class='pill'>INSTALL</span>. The Center downloads the signed installer, checks it and installs it silently.",
 "When the card shows <span class='pill'>INSTALLED</span>, start (or restart) your DAW and rescan plugins if it does not pick up new plugins by itself.",
 "Updates appear on the same card: <span class='pill'>UPDATE</span>, or <span class='pill'>UPDATE ALL</span> at the top. Close the standalone RONE Hook before updating it."])}
<h3>Everything it needs comes with it</h3>
<p>Besides the plugin, the installer puts HOOK's own runtime on the computer, so nothing has to be installed before:</p>
{table(["Part", "What it does"], [
 ("The engine", "Separates the voice, in its own process - a busy graphics card can never take the DAW down. onnxruntime with DirectML: any graphics card, the processor when there is none."),
 ("The vocal model", "The AI model that tells the voice from the music (67 MB). Installed with the plugin: there is nothing to download on the first open."),
 ("yt-dlp + QuickJS", "Opens YouTube links and searches YouTube."),
 ("ffmpeg (LGPL build)", "Decodes and cuts the sound."),
])}
<p><strong>yt-dlp keeps itself current.</strong> YouTube changes often, so HOOK updates yt-dlp once a day, and again at once whenever a link fails (then it tries the link again).</p>
<h3>Where the files go (Windows)</h3>
{table(["File", "Location"], [
 ("VST3", "<code>C:\\Program Files\\Common Files\\VST3\\RONE\\RONE Hook.vst3</code>"),
 ("Standalone app", "<code>C:\\Program Files\\RONE Plugins\\RONE Hook.exe</code>"),
 ("Runtime (engine, yt-dlp, ffmpeg, model)", "<code>C:\\ProgramData\\RONE Plugins\\Hook</code>"),
 ("Your exports", "<code>Documents\\RONE Plugins\\Exports\\Hook</code>"),
 ("The HOOK BANK", "<code>Documents\\RONE Plugins\\Hook Bank</code>"),
 ("This manual", "<code>C:\\Program Files\\RONE Plugins\\Manuals\\</code>"),
])}
{note("macOS", "RONE Hook is Windows-only for now (VST3 + Standalone). A Mac version will follow.")}
{note("Manual inside the Center", "The plugin's card menu (the three dots) has a <strong>Manual</strong> entry. It opens this PDF from the install folder, or from roneaudio.com if the local copy is missing.")}
<h3>Uninstalling</h3>
<p>Settings &rarr; Apps &rarr; <em>RONE Hook (RONE)</em>. The runtime in <code>ProgramData</code> goes with it; your exports and the HOOK BANK in Documents are kept.</p>
"""})

    S.append({"title": "Quick start", "sub": "your first hook", "html": f"""
{img("hook/empty.png", "<b>RONE Hook</b> as it opens: empty, waiting for a search or a link.", "w60")}
{steps([
 "<strong>Open RONE Hook</strong> - insert it on any track (it makes no sound of its own until you play a voice), or open the standalone app.",
 "<strong>Find the video.</strong> Type in the search field and pick a result, or copy a YouTube link and press <span class='pill'>PASTE LINK</span> (or paste it into the field with <span class='kbd'>Ctrl</span>+<span class='kbd'>V</span>: a link starts at once).",
 "<strong>Wait a few seconds.</strong> HOOK downloads the sound and takes the music out. The words of the video arrive first; the start of the voice plays while the rest is still being made.",
 "<strong>Find the line.</strong> Click a word in WORDS to hear it, or type a word in FIND.",
 "<strong>Mark it.</strong> Drag across the waveform, or click the first word and shift-click the last. <span class='pill'>PLAY MARK</span> loops the mark.",
 "<strong>Shape it</strong> with DENOISE, DE-VERB and the other knobs, a CHARACTER, ONE NOTE or FIT.",
 "<strong>Drag the DRAG TO DAW tile</strong> onto an audio track. The file is also saved in <code>Documents\\RONE Plugins\\Exports\\Hook</code>.",
])}
{note("A word they say", "Type a subject in the search field and a word or phrase in <strong>+ a word they say</strong>, then press <span class='pill'>FIND</span>: HOOK reads the words of every result and lists each moment they are said. Pick one and HOOK takes that video with the line already marked.")}
"""})

    S.append({"title": "Interface tour", "sub": "every element on screen", "html": f"""
{img("hook/tour.png", "A voice taken out of a 3:16 vocal session: one line marked, FIND on &ldquo;higher&rdquo;, the knobs dialled in.")}
{legend([
 ("Header logo", "click to flip to the back panel (version, link to roneaudio.com)"),
 ("Voice readout", "the voice's key and tempo (or TALK), its length, how long it took, 44.1K 24-BIT"),
 ("GPU / CPU", "where the voice is separated: on the graphics card (GPU, lit) or the processor (CPU)"),
 ("BANK", "opens the HOOK BANK - every hook you kept, with their number. Key B"),
 ("OPEN FOLDER", "opens Documents / RONE Plugins / Exports / Hook"),
 ("DELETE", "click twice: HOOK empties; the video's cache files go to the Recycle Bin, your exports stay"),
 ("Search / link", "type to search YouTube, or paste a link; it opens on JUST OPENED"),
 ("A word they say", "words said in the video: HOOK finds every moment they are said"),
 ("PASTE LINK", "takes a link from the clipboard and starts"),
 ("GO / SEARCH / FIND", "starts what is in the fields (Enter); its name says what it will do"),
 ("The voice", "the separated voice. Click: play from there. Drag: mark. Wheel: zoom"),
 ("Mark and position", "the mark's start and end, and where the playhead is"),
 ("The words as they play", "the line being said, the word lit as you hear it, the next line under it"),
 ("PLAY", "plays the voice; with a mark, PLAY MARK loops the mark. Space"),
 ("The six knobs", "DENOISE, DE-VERB, LOW CUT, COMP, AIR, REVERB"),
 ("NORMALIZE", "the file's peak at -1 dB"),
 ("VOCAL SEP", "on: the voice alone; off: the whole song at the same spot. Key S"),
 ("DRAG TO DAW", "drag the mark (or the whole voice) onto a track; click: show the file"),
 ("Keep", "the heart: keeps the mark, as it sounds now, in the HOOK BANK"),
 ("CHARACTER", "seven colours for the hook: CLEAN, RADIO, MEGAPHONE, DARK -5, REVERSE TAIL, STUTTER END, WHISPER AIR"),
 ("ONE NOTE", "the voice on one straight note, like RONE IRON"),
 ("Tabs", "WORDS, FIT, CHOPS and MARKS"),
 ("FIND", "a word or a few: every place they are said lights up; the arrows (Enter / Shift+Enter) step through them"),
 ("WORDS", "every word of the video as one text, with its time"),
 ("Resize grip", "drag to scale the whole window"),
])}
"""})

    S.append({"title": "Finding the video", "sub": "search, a link, a word they say", "html": f"""
{img("hook/finder.png", "&ldquo;higher&rdquo; said in four videos, cleanest first.")}
{legend([
 ("Search", "what you are looking for: a subject, a title, a channel - or a link"),
 ("A word they say", "the words you want to hear said"),
 ("What was found", "the words, the search and how many moments were found"),
 ("SORT", "CLEANEST first (the voice most alone) or in YouTube's order"),
 ("Filters", "SPEECH, SONG / ACAPELLA, MOVIE &middot; ENGLISH, HEBREW &middot; &lt; 10 MIN &middot; SAFE TO USE &middot; CC"),
 ("Keep", "a heart on a moment keeps just those seconds in the HOOK BANK, the music taken out"),
 ("Hear it", "plays the moment straight from YouTube before you take it"),
 ("+20 MORE", "the next videos"),
])}
<h3>Three ways in</h3>
{table(["You do", "HOOK does"], [
 ("Click the empty field", "Shows <strong>JUST OPENED</strong>: the YouTube videos you opened last in Chrome, Edge or Brave, and a link on the clipboard. Nothing leaves the computer."),
 ("Type a search", "Searches YouTube as you type (about 2 seconds). Pick a result to take it."),
 ("Paste a link", "Takes that video at once."),
 ("Search + <em>a word they say</em>", "Reads the words of the results and lists every moment those words are said, with the line around them. Taking a moment opens the video with that line marked."),
])}
<h3>CLEAN</h3>
<p>Each moment gets a <strong>CLEAN</strong> score from 1 to 99: how alone the voice is there - quiet around the words, no music under them. A podcast scores in the 90s; a speech with music under it, 50s and lower. A clean moment separates best.</p>
<p>A result's <strong>ONLY THIS CHANNEL</strong> link limits the list to that channel; click the channel chip again to show all.</p>
{note("Long videos", "A video longer than 20 minutes is not refused: HOOK takes the <strong>20 minutes around the moment you picked</strong> (or the start of the video when you pasted a link).")}
"""})

    S.append({"title": "The voice and the words", "sub": "listen, zoom, mark", "html": f"""
{img("hook/working.png", "While the voice is being made: 1 the progress &middot; 2 GPU or CPU &middot; 3 how much is ready. The part that is ready plays already; the bright edge shows where HOOK is.", "w80")}
<h3>While it works</h3>
<p>The tag above the waveform tells you where it is - LOOKING, DOWNLOADING, TAKING THE MUSIC OUT with a percentage - and the header shows <strong>GPU</strong> or <strong>CPU</strong>. The words come first (from YouTube's own captions, in a few seconds); the voice fills in from the left, and you can play and mark the part that is ready. A video you took before opens at once from the cache.</p>
<h3>The waveform</h3>
{table(["Gesture", "What it does"], [
 ("Click", "Plays from there."),
 ("Drag", "Marks a part (the rest dims). PLAY then reads PLAY MARK and loops it, with short fades at the seam."),
 ("Mouse wheel", "Zooms in and out around the mouse - down to 2 seconds across. A red bar at the bottom shows where you are."),
 ("<span class='kbd'>Shift</span> + wheel", "Moves along the voice."),
 ("Double-click", "Shows the whole voice again."),
 ("<span class='kbd'>Space</span> &middot; <span class='kbd'>Esc</span>", "Play / pause &middot; remove the mark."),
])}
<h3>WORDS</h3>
<p>Every word of the video as one flowing text. The thin lines under the waveform show where words are said.</p>
{table(["Gesture", "What it does"], [
 ("Click a word", "Plays from that word."),
 ("<span class='kbd'>Shift</span> + click another word", "Marks from the first word to this one, and plays the mark."),
 ("FIND", "Type a word or a few: every place they are said (whole words) lights up. <span class='kbd'>Enter</span> or &#9660; goes to the next one and plays it; <span class='kbd'>Shift</span>+<span class='kbd'>Enter</span> or &#9650; the one before. <span class='kbd'>Esc</span> clears it."),
])}
<p>The words come from YouTube's captions, in the video's own language. A video without captions is listened to on your computer instead (the line count then reads LISTENING).</p>
"""})

    S.append({"title": "Shaping the voice", "sub": "knobs, switches, CHARACTER, ONE NOTE", "html": f"""
{img("hook/tour_deck.png", "PLAY, the six knobs, NORMALIZE and VOCAL SEP, and the DRAG TO DAW tile with its heart.", "w80")}
<p>The knobs work on what you hear and on the file you drag. Each one starts at OFF; double-click a knob to set it back to OFF.</p>
{ctl("DENOISE", "OFF - 100 %", "Takes out hiss, hum and the music left between the words.")}
{ctl("DE-VERB", "OFF - 100 %", "Takes the room's reverb out of the voice - the echo of a hall, a bathroom, a stage. The knob is how wet the room is assumed to be.")}
{ctl("LOW CUT", "OFF &middot; 20 - 400 Hz", "Cuts rumble, mic thumps and boominess below the frequency.")}
{ctl("COMP", "OFF - 100 %", "Evens the loud and the soft words out.")}
{ctl("AIR", "OFF - +9 dB", "Brightness and presence: a high shelf at 8 kHz.")}
{ctl("REVERB", "OFF - 100 %", "A vocal plate.")}
{ctl("NORMALIZE", "on / off &middot; default on", "The file's peak at -1 dB.")}
{ctl("VOCAL SEP", "on / off &middot; default on &middot; key S", "Off: you hear the <strong>whole song</strong> at the same spot instead of the voice alone - handy to hear the line in its place. It is for listening only: the file you drag is always the voice.")}
<div class="two">
<div>
<h3>CHARACTER</h3>
{table(["Chip", "What it does"], [
 ("CLEAN", "As it is."),
 ("RADIO", "A small speaker: 350 Hz - 3.2 kHz, a little drive."),
 ("MEGAPHONE", "A horn: 600 Hz - 2.6 kHz, driven hard."),
 ("DARK -5", "Five semitones down, the timing kept."),
 ("REVERSE TAIL", "Its own reverb, reversed, swelling into the first word."),
 ("STUTTER END", "The last word's start three times on a 1/16 of your tempo."),
 ("WHISPER AIR", "Thin and bright, breathy."),
])}
</div>
<div>
{img("hook/char.png", "1 CHARACTER on RADIO &middot; 2 the ONE NOTE list open, on A.", "")}
<h3>ONE NOTE</h3>
<p>Puts the voice on one straight note - the same flatten as RONE IRON. Pick the note from the list; OFF gives the voice back as it was sung or spoken.</p>
</div></div>
{note("PLAY MARK and the finished file", "With FIT, ONE NOTE, DARK, REVERSE TAIL or STUTTER END on, PLAY MARK plays the finished file - exactly what you will drag. Mark a part first.")}
"""})

    S.append({"title": "FIT, CHOPS and MARKS", "sub": "into your track", "html": f"""
{img("hook/fit_pane.png", "FIT on: a voice in F# minor at 140 BPM, moved to A minor at 145 BPM.", "")}
<h3>FIT - into your track's key and tempo</h3>
{table(["Control", "What it does"], [("FIT", "on: the hook in your track's key and on its grid"), ("The voice", "the key and tempo HOOK heard in the voice (TALK for speech)"), ("Track key", "your track's key"), ("Track tempo", "your track's BPM - the DAW's when it sends one"), ("What FIT does", "semitones and the time stretch, e.g. +3 ST &middot; x0.97")])}
<p>A sung hook is moved to your key and stretched to your tempo; speech is moved to your key and lands on whole beats. FIT works on the file you drag.</p>
{img("hook/chops_pane.png", "CHOPS: every word of the mark on its own key.", "")}
<h3>CHOPS - every word on a key</h3>
{table(["Control", "What it does"], [("Word keys", "click a key to hear that word"), ("DRAG CHOPS + MIDI", "drags every word as a numbered file plus a MIDI file: C3 up, in the words' own timing at your tempo"), ("To RONE IRON", "drags the hook straight into RONE IRON, which slices it on its own keys")])}
<p>CHOPS follow the mark: mark a line first. The chops come with FIT and CHARACTER as they are set. A plain click on a drag button shows the files instead.</p>
{img("hook/marks_pane.png", "MARKS: three hooks of the same video, ready to drag as one pack.", "")}
<h3>MARKS - several hooks at once</h3>
{table(["Control", "What it does"], [("The marks", "every mark kept, with its time and words; play or remove each one"), ("+ ADD THIS MARK", "keeps the current mark in the pack. Key M"), ("DRAG THE PACK", "drags every mark at once - numbered files, with FIT and CHARACTER")])}
"""})

    S.append({"title": "Taking it out", "sub": "DRAG TO DAW, the folder, the HOOK BANK", "html": f"""
<h3>DRAG TO DAW</h3>
<p>The tile at the right of the knobs says what you will get: <strong>THE MARK &middot; 0:09</strong> with a mark, <strong>THE WHOLE VOICE</strong> without one. Hold the mouse on it and drag onto an audio track. A plain click shows the file in Explorer. While the knobs are being applied the tile reads <em>getting it ready with the knobs&hellip;</em> for a moment.</p>
<p>Every file is written to <code>Documents\\RONE Plugins\\Exports\\Hook</code> as 44.1 kHz 24-bit WAV, named after the video and the mark. <strong>OPEN FOLDER</strong> opens it.</p>
{img("hook/bank_panel.png", "The HOOK BANK: hooks kept from three videos, with their tags.", "w80")}
<h3>The HOOK BANK</h3>
{table(["Control", "What it does"], [("Find a hook", "its words, the video, a tag"), ("Tags", "ALL and your tags, the most used first"), ("Kept hooks", "play, drag a row into the DAW, add a tag, remove (x, to the Recycle Bin)"), ("OPEN FOLDER", "Documents / RONE Plugins / Hook Bank")])}
<p>The bank is shared by every RONE Hook in every project. Two ways to fill it:</p>
<ul>
<li><strong>The heart on the DRAG TO DAW tile</strong> keeps the mark (or the whole voice) as it sounds now - with the knobs, FIT and CHARACTER.</li>
<li><strong>A heart on a search result</strong> keeps just those seconds of that video; HOOK takes the music out in the background (about 5 seconds). A hook kept with the music still in shows <em>WITH THE MUSIC</em>.</li>
</ul>
<p>Open the bank with <strong>BANK</strong> in the header or the <span class='kbd'>B</span> key; <span class='kbd'>Esc</span> or CLOSE closes it.</p>
"""})

    S.append({"title": "Step-by-step workflows", "sub": "recipes", "html": f"""
{recipe("A sung hook in your key", "acapella or live session",
 steps([
  "Search for the song's acapella or a live session, with the line's main word in <em>a word they say</em>.",
  "Pick the cleanest moment (CLEAN at the top).",
  "Adjust the mark on the waveform; PLAY MARK until it loops cleanly.",
  "DENOISE 30-50 %, DE-VERB if the room shows, AIR a little.",
  "FIT tab: switch FIT on, set your key (the tempo comes from the DAW).",
  "Drag the tile onto a track.",
 ]))}
{recipe("A spoken drop", "talk, speech, film",
 steps([
  "Filter SPEECH or MOVIE and search the subject, with the sentence's words in <em>a word they say</em>.",
  "Mark from the first to the last word with click + shift-click in WORDS.",
  "COMP 30-40 % and LOW CUT around 80-120 Hz for a forward voice.",
  "CHARACTER RADIO or MEGAPHONE for a classic drop sound, or STUTTER END before the drop.",
 ]))}
{recipe("Chop a line into a playable instrument", "CHOPS + MIDI",
 steps([
  "Mark the line.",
  "CHOPS tab: click the keys to hear each word.",
  "Drag <strong>DRAG CHOPS + MIDI</strong> into your sampler, or <strong>&rarr; RONE IRON</strong> to slice it there.",
 ]))}
{recipe("A pack of hooks from one video", "MARKS",
 steps([
  "Mark the first hook, press <span class='kbd'>M</span>.",
  "Mark the next, <span class='kbd'>M</span> again - as many as you like.",
  "MARKS tab: DRAG THE PACK drops them all, numbered.",
 ]))}
"""})

    conv = G["conventions_section"](m)
    for a, b in (("<td>Resets it to the default value.</td>", "<td>Resets it to OFF.</td>"),
                 ("Steps through segmented switches and lists where the plugin offers them.", "Turns a knob; over the waveform it zooms (see <em>The voice and the words</em>)."),
                 ("version, format, licence holder, a link to roneaudio.com and a DEACTIVATE button for this device.", "version, format and a link to roneaudio.com."),
                 ("Every knob and switch is a host parameter. Automate it from the DAW's automation lane or by right-clicking the control inside the plugin (VST3). Values shown in the plugin always match the DAW's.",
                  "The knobs and switches are host parameters: the DAW saves them with the project, and right-clicking one inside the plugin (VST3) opens the DAW's menu for it. HOOK works on files, so automating them changes the next file you drag, not the audio in your timeline."),
                 ("The plugin's full state is saved with your DAW project, so a project reopens exactly as you left it. Where the plugin has its own preset browser (the name field with &#9664; &#9654; arrows), factory presets are built in and your own presets are stored in your user folder.",
                  "The knobs, CHARACTER, ONE NOTE and FIT are saved with your DAW project. The voices themselves live in HOOK's cache, so a voice you took before opens at once; the files you dragged are in your project and in the Exports folder."),
                 ("It uses your system's default audio device; when there is no DAW clock, tempo-synced controls follow the plugin's own BPM setting.",
                  "It uses your system's default audio device; without a DAW, FIT uses the tempo you type in the FIT tab.")):
        assert a in conv["html"], a
        conv["html"] = conv["html"].replace(a, b)
    S.append(conv)

    sup = G["support_section"](m, [
        ("A link does not open", "YouTube changes often. HOOK then <strong>updates its YouTube tool (yt-dlp) by itself and tries the link again</strong> - give it a few seconds. It also updates once a day. If it still fails, check the internet connection and that the video plays in a browser (private, age-restricted and removed videos cannot be opened)."),
        ("It is slow / the header shows CPU", "With a graphics card (GPU, lit) a 10-minute video is separated in about half a minute, and the start plays after a few seconds. Without one (CPU) the voice is made on the processor, several times slower - the start still plays while the rest is made. Keep the graphics driver up to date."),
        ("Where did my file go?", "Every file you drag or save is in <code>Documents\\RONE Plugins\\Exports\\Hook</code> (OPEN FOLDER). Kept hooks are in <code>Documents\\RONE Plugins\\Hook Bank</code>."),
        ("A long video only shows part of it", "Videos over 20 minutes: HOOK takes the 20 minutes around the moment you picked. Pick the moment with <em>a word they say</em> to choose which 20 minutes."),
        ("Music is still under the voice", "Raise DENOISE. Try another upload of the same thing - a live session or an acapella separates much cleaner (look for a high CLEAN score)."),
        ("No words appear", "The video has no captions: HOOK listens for the words itself, which takes longer (the line count reads LISTENING)."),
        ("VOCAL SEP off says NO SONG", "The whole song could not be fetched again; the voice keeps playing. Try again later."),
        ("The DRAG TO DAW tile is grey", "The voice is still being made. It lights up when the voice is done."),
    ])
    h = sup["html"]
    for a, b in ((" (Windows: VST3 + Standalone; macOS: VST3 + Standalone)", " (Windows; a Mac version will follow)"),
                 ("Windows 10 / 11 (64-bit) &middot; macOS 11 or newer (Intel and Apple silicon, universal)", "Windows 10 / 11 (64-bit) &middot; a graphics card is used when there is one (DirectML), the processor otherwise &middot; an internet connection to reach YouTube")):
        assert a in h, a
        h = h.replace(a, b)
    # a free plugin: no licence row; the generic Mac rescan advice does not apply yet
    h = re.sub(r"<tr><td>A licence screen appears</td>.*?</tr>", "", h, flags=re.S)
    h = h.replace(" On macOS restart the DAW after installing; Logic and GarageBand may need a full AU rescan.", "")
    sup["html"] = h
    S.append(sup)

    m["sections"] = S
    return m
