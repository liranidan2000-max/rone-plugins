# The legend is the callout table, in the callout order - see
# annotate.ANALYZER_CALLOUTS. Keeping a second list here is what put
# every badge's description one control out when the UI changed.
from annotate import ANALYZER_CALLOUTS as _CALLOUTS
ANALYZER_LEGEND = [(label, desc) for label, desc in _CALLOUTS.values()]


def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    m = {
        "id": "analyzer", "product": "RONE Analyzer", "eyebrow": "THE MIX, MEASURED",
        "title_html": "RONE <i>Analyzer</i>", "accent": "#1A9AB0", "version": "1.3",
        "tagline": "One window that is always on: a third-octave spectrogram with a reference record drawn over it, level meters, a vectorscope, EBU R128 loudness, a bar-locked scope - fed by your DAW's master through the Bridge, by what the computer plays, by an input or by a file.",
        "formats": ["Standalone", "VST3 + AU (Bridge, Channel)"], "vst3": "RONE Analyzer Bridge.vst3 / RONE Analyzer Channel.vst3",
        "au": "RONE Analyzer Bridge.component / RONE Analyzer Channel.component", "exe": "RONE Analyzer.exe",
        "pdf": "RONE Analyzer - User Manual.pdf", "cover_img": "analyzer/main.png",
        "channels": "Any stereo pair of the selected source; STEREO / SIDES / MONO fold",
        "engine": "Native analysis engine; the instruments are drawn in a web view",
        "latency": "Not applicable (measurement only; the Bridge and the Channel pass audio through untouched)",
    }
    S = []

    S.append({"title": "Welcome to RONE Analyzer", "sub": "what it measures and how it is meant to be used", "html": f"""
<p><strong>RONE Analyzer</strong> is a measurement window that stays open all session, on a second monitor or over the DAW. It is one picture of the whole mix: the spectrogram on the left, the level meters in the middle, the vectorscope with correlation, NOTE and the reference strip on the right, and the LOUDNESS panel under the spectrogram when you want it. It reads your DAW's master through the included <strong>RONE Analyzer Bridge</strong> plugin, whatever the computer is playing, a hardware input, or a file.</p>
<p>Two plugins come with it. The <strong>Bridge</strong> goes last on your master: it sends the mix to the analyser bit for bit, and it is the only source where band solo, zone solo and the mono fold are <em>heard</em> in the DAW itself. <strong>RONE Analyzer Channel</strong> goes on any mixer channel - KICK, BASS, VOX - and that channel's spectrum is drawn over the master in its own colour.</p>
<div class="two">
<div>
<h3>The instruments</h3>
<ul>
<li><strong>Spectrogram</strong> - thirty third-octave bands of LEDs with held peaks, or WAVES, an FFT spectrum drawn the way spectrum analysers everyone knows draw it. Zones you can solo by ear, bands you can press to hear, SIDES in violet, your Channels in their colours and a <em>reference record</em> drawn over it all.</li>
<li><strong>Level meters</strong> - RMS and PEAK per channel, MAX PEAK and MAX RMS, overs that stay lit until you reset them.</li>
<li><strong>Vectorscope</strong> - the stereo image as a phosphor trace, correlation, left / right balance, and a second scope for the reference or a Channel.</li>
<li><strong>NOTE</strong> - the note under the mix, the fundamental of the bass or the kick, in a pop-out window if you like.</li>
<li><strong>LOUDNESS</strong> - EBU R128: momentary, short-term, integrated, range, true peak, against a CLUB, STREAM or EBU target.</li>
<li><strong>BAR SCOPE</strong> - the waveform locked to the DAW's bar, from 4 bars down to 1/8, with the Channels drawn in.</li>
</ul>
</div>
<div>
<h3>The 10-second version</h3>
{steps([
 "Open RONE Analyzer.",
 "Put <strong>RONE Analyzer Bridge</strong> last on your DAW's master. The analyser picks it up by itself.",
 "Read the spectrogram, the meters, the scope.",
 "REFERENCE &rarr; load a record you admire. Its curve sits over your bars.",
 "Press <strong>F1</strong> for the built-in tour of everything."])}
<h3>Why third-octave bands</h3>
<p>An FFT has linear bins; an ear does not. The spectrogram runs thirty real bandpass filters on the standard third-octave centres from 25 Hz to 20 kHz, so every band is a third of an octave wide and a tone reads its own level. When you want the familiar FFT picture, WAVES is one key away.</p>
</div></div>
{note("No routing", "The analyser never needs your interface's outputs. It opens its sources with no output channels, so it sits beside the DAW without fighting for the device, and a Bridge or a Channel in the DAW needs no routing at all - insert it and it shows up.")}
"""})

    S.append(G["install_section"](m))

    S.append({"title": "Quick start", "sub": "seeing your mix in the analyser", "html": f"""
{img("analyzer/main.png", "<b>RONE Analyzer</b> with a file playing and a reference record loaded: spectrogram with the reference curve and the live average, LOUDNESS open, level meters, vectorscope, correlation, the reference strip, NOTE.")}
{steps([
 "<strong>Start</strong> RONE Analyzer from the Start menu (Windows) or Applications (macOS). It remembers its source, its settings, its theme and its window between runs.",
 "<strong>Measure the DAW.</strong> Insert <strong>RONE Analyzer Bridge</strong> on your master bus, last in the chain. The moment the plugin runs, the analyser switches to it by itself (its window says LINKED). Band solo, zone solo and the mono fold are then heard through the DAW.",
 "<strong>Or choose another source.</strong> Click <span class='pill'>SOURCE</span> (or F3). The menu lists the DAW master, what the computer plays (no routing needed), your inputs, ASIO devices and other device types, a sample rate, and <em>Analyse a file...</em>. Under them, the channel pairs of the open source.",
 "<strong>Or open a file.</strong> Menu &rarr; <em>Open a file and play</em> to hear and measure it, or <em>Open a file and scan</em> to push the whole file through the meters in seconds. PLAY / STOP and SCAN then appear in the header; <em>Loop the file</em> is in the menu.",
 "<strong>Load a reference.</strong> Press <span class='pill'>REFERENCE</span> on the right. Pick a record; its waveform comes up, the bright stretch is the 16 bars the curve is taken from, and the curve sits over your spectrogram. REF LINE / REF FULL chooses how it is drawn.",
 "<strong>Hear a range.</strong> Click a zone under the spectrogram - SUB, BASS, LOW MID - to hear only that range; press and hold a bar to hear one band. Escape lets everything back.",
 "<strong>Check the low end in mono.</strong> Press <span class='pill'>M</span>, or the STEREO chip on the vectorscope. If the bass thins out, it is not mono.",
 "<strong>Give it the whole screen.</strong> The chevron at the top right, or <span class='pill'>H</span>, takes the header and status bar away. <span class='pill'>Escape</span> brings them back.",
])}
{note("The tour", "Menu &rarr; <em>Hints: a tour of everything</em>, or F1, walks through every control with a short animation of the gesture. The instrument stays live underneath, so each step can be tried while it is up. Arrows step, Escape ends.")}
"""})

    S.append({"title": "Interface tour", "sub": "every element on screen", "html": f"""
<p>The window as it opens, with a file playing, the LOUDNESS panel open and a reference record loaded. The BAR SCOPE (SCOPE, bottom right) and the second vectorscope (the + on the vectorscope's title) open as extra panels and are described in the reference section.</p>
{img("analyzer/tour.png", "RONE Analyzer, every control numbered.")}
{legend(ANALYZER_LEGEND)}
"""})

    S.append({"title": "Spectrogram", "sub": "the bars, the zones, the reference and your Channels", "html": f"""
<p>Thirty third-octave bands of LEDs, 25 Hz to 20 kHz. The <strong>white LED</strong> on each column is the held peak. The gold rows across the plot are the 10 dB rules, the fainter ones the 5 dB rows. Hover a band for its readout.</p>
{ctl("BARS / WAVES", "chip in the title &middot; W", "<p><strong>BARS</strong> is the third-octave analyser. <strong>WAVES</strong> is the FFT spectrum: a 2048-point Hann window, 50 % overlap, a 2-second average, a 4.5 dB per octave slope and a -78 to -18 dB scale, so it reads like the spectrum analysers everyone knows. The reference and your Channels are drawn in both.</p>")}
{ctl("Setup (gear)", "F2 &middot; BARS: bands, range, resolution, decay, rise time, hold, 5 dB rows &middot; WAVES: Spectrum Mode", "<p>In BARS the gear opens the analyser's setup: how many bands, the range and the resolution of the scale, the decay and rise time of the bars, the peak hold, and whether the 5 dB rows are drawn. <strong>Freeze</strong> holds the bars where they are (F; the FROZEN badge shows while it is on).</p><p>In WAVES the gear opens the <strong>Spectrum Mode</strong> editor: routing (stereo, mid, side), window, type, block size, overlap, averaging time, the frequency and level range, slope, offset, smoothing, a filled display, a second spectrum, anti-alias, ALIGN 0 DB and BYPASS. DEFAULT puts the factory mode back. Drag a knob up or down, use the wheel (Shift for fine), double-click to type.</p>")}
{ctl("RULES", "chip in the title", "<p>One click hides the 5 dB rows, the next hides the 10 dB rows too, the next brings them all back.</p>")}
{ctl("SIDES", "chip in the title", "<p>Paints the side signal (L minus R) over the bars in violet: what is <em>wide</em>, band by band. A mono mix shows none. A band whose violet reaches the cyan is as wide as it is loud - wide bass, usually.</p>")}
{ctl("Zones", "SUB &middot; BASS &middot; LOW MID &middot; MID &middot; HIGH MID &middot; PRES &middot; AIR", "<p>The strip under the frequency numbers. <strong>Click</strong> a zone to hear that range alone; click more to add them. <strong>Right-click</strong> one to solo just it. <span class='kbd'>Esc</span> lets everything back. Through the Bridge the solo is heard in the DAW; on any other source it is drawn on screen and heard on the reference player.</p>", "Solo BASS while the drop plays. What you hear is what the sub really does under the kick.")}
{ctl("Band solo", "press and hold a bar", "<p>Hear only that band. Drag while holding to sweep across; the <strong>wheel</strong> narrows or widens the band; a double-press latches it, and Escape or another double-press releases it. The shaded width is the filter's own -3 dB width.</p>", "Hold a band around 200-400 Hz against the reference to hear mud, and one around 2-3 kHz to hear harshness.")}
{ctl("CHANNELS", "chips in the title row", "<p>Put <strong>RONE Analyzer Channel</strong> on a mixer channel, name it in the plugin (KICK, BASS, VOX), pick one of its six colours, and it appears here as a chip in that colour, drawn over the master with the master's own LEDs. <strong>Click</strong> a chip to show or hide it, <strong>S</strong> to see it alone, <strong>right-click</strong> to hear it alone (every other Channel ducks). <strong>Drag</strong> the chips left and right to set the drawing order: the rightmost is drawn on top. The <strong>MIX</strong> chip is the master itself - drag it among the Channels to put the master above or below them, or click it to hide the master and see only your Channels. Up to eight Channels; the row scrolls when they do not fit.</p>")}
{ctl("REF LINE / REF FULL", "chip in the title, shown while a reference is loaded", "<p>How the reference record is drawn. <strong>LINE</strong>: its curve over the plot, with the live average in white beside it, and a legend naming the record and its bars. <strong>FULL</strong>: the whole record in its own colour behind the live one - as bars in BARS, as a spectrum in WAVES - at the record's own level, so muting your kick does not move it.</p>")}
"""})

    S.append({"title": "Meters, scope, reference", "sub": "the rest of the window", "html": f"""
<h3>Level meters</h3>
{ctl("RMS / PEAK", "RMS inside, PEAK outside, per channel", "<p>Ten LEDs a step. The rows from -3 dB up are yellow, the OVR rows red: they light on an over and stay lit until RESET. The white LED holds the peak.</p>")}
{ctl("MAX PEAK / MAX RMS", "title row", "<p>The loudest moment since the last reset: the largest sample, and the RMS averaged over 217 ms of the louder channel - read the way a clipper reads them. Click either to restart.</p>")}
{ctl("Meter settings (gear)", "range, hold, release, over detection", "<p>How far down the scale goes, how long a peak is held, how fast the meters fall, and how many consecutive full-scale samples count as an over - one clipped sample is not an over.</p>")}
<h3>Vectorscope</h3>
{ctl("The trace", "M up, L and R on the diagonals, S across", "<p>A phosphor picture of the stereo image: every sample step leaves a dot, brighter the slower the beam moves, with afterglow. A thin vertical line is mono; a wide cloud is wide; a line leaning to L or R is a balance problem. <strong>MODE</strong> switches the frame between the diamond and a circle. <span class='kbd'>V</span> toggles LOCKED (the trace stays inside the diamond) and FREE (it runs to the frame). The AGC reading in the corner is how much the picture is being lifted; the gear sets the afterglow and the AGC.</p>")}
{ctl("STEREO / SIDES / MONO", "chip on the vectorscope &middot; M", "<p>Every click folds further: <strong>SIDES</strong> is the mix with its mid taken out, <strong>MONO</strong> is the fold. Every instrument then measures the fold - the meters drop wherever the channels were fighting, the spectrum changes shape, the trace stands as a line - and with the Bridge on the master the DAW plays the fold too (the dot on the chip lights while the Bridge is there). The fold never survives a restart.</p>", "Press M while the bass plays. If it thins out, the low end is not mono.")}
{ctl("Second vectorscope", "+ on the title", "<p>A fourth column with a second scope. Its sources are chips: the loaded reference record, by its name, or any of your Channels, in its colour - the reference's image beside yours, or the kick alone. With nothing loaded it says ADD A REFERENCE OR A CHANNEL. The X closes it.</p>")}
{ctl("Correlation and BAL", "-1 to +1", "<p><strong>+1</strong> is mono-safe, <strong>0</strong> is wide, below zero cancels in mono; the LED bar lights from zero to the reading, red on the cancelling side. <strong>BAL</strong> is the left / right energy balance in dB.</p>")}
<h3>Reference</h3>
{ctl("REFERENCE", "button under the correlation", "<p>Load a record to compare against. It is analysed in a few seconds; its waveform comes up in the strip with its tempo, and the <strong>16 bars</strong> the curve is taken from are the bright stretch - click anywhere on the waveform to move it there, the arrows step 16 bars back or forward. The curve sits over the spectrogram (REF LINE / REF FULL) and the record is a source for the second vectorscope.</p>")}
{ctl("PLAY / DAW-REF", "under the strip", "<p><strong>PLAY</strong> plays the reference's 16 bars, round and round, on this computer's output. Through the Bridge the button becomes <strong>DAW / REF</strong>: an A/B that mutes the DAW while the reference plays, so you switch between the two with one click and the meters follow. Band solo, zone solo and the mono fold apply to the reference as they do to the mix.</p>", "Set your DAW to loop the same 16 bars of your track, then A/B. Same bars, same meters, same ears.")}
{ctl("CURVES / CLEAR", "under the strip", "<p><strong>CURVES</strong> keeps the current curve under a name and brings back any curve you kept - a saved curve needs no record. <strong>CLEAR</strong> removes the reference and brings the REFERENCE button back.</p>")}
{ctl("NOTE", "under the strip", "<p>The note under the mix - the fundamental of the bass or the kick, from the analysed pair - with its octave, its hertz and how far off the note it sits, in cents. The small arrow pops NOTE out into a little window that stays on top of everything: put it over your EQ. Its X closes it; it remembers its place.</p>")}
<h3>LOUDNESS</h3>
<p>Closed until you ask for it: the small arrow at the bottom right of the spectrogram, or <span class='kbd'>L</span>, opens the panel under it, and the same arrow puts it away. It measures all the time, open or closed.</p>
{ctl("Readings", "MOMENTARY &middot; SHORT-TERM &middot; INTEGRATED &middot; RANGE &middot; TRUE PEAK &middot; TO TARGET", "<p>Momentary (400 ms), short-term (3 s) and gated integrated loudness in LUFS, loudness range in LU, 4&times; oversampled true peak in dBTP (rounded <em>up</em> to the next 0.1 dB, so the number is never under the peak it stands for; OVER -1 lights above -1.0 dBTP), and the integrated value's distance from the target. INT counts the audio integrated since the last reset; the gauge under the numbers shows integrated against the target.</p>")}
{ctl("Target", "CLUB -8 &middot; STREAM -14 &middot; EBU -23", "<p>A club or DJ master at -8 LUFS, streaming (Spotify, Apple Music, YouTube) at -14, broadcast at -23. The gauge and TO TARGET measure against the chosen one.</p>")}
{ctl("RESET", "the chip on the panel", "<p>Restarts Integrated, Range and the true-peak hold from zero; Momentary and Short-term keep running. The menu's <em>Reset all measurements</em> does this and clears the overs and held peaks everywhere.</p>", "Reset when the song starts so the integrated value covers only the material you mean, then let the whole track play.")}
<h3>BAR SCOPE</h3>
{ctl("SCOPE", "bottom right of the vectorscope &middot; O", "<p>A strip under the instruments: the waveform locked to the DAW's bar, so every pass of the loop lands on top of the last. The window is <strong>4 BARS</strong> down to <strong>1/8</strong>. Hold and drag up to zoom, sideways to scroll; double-click for the whole window; the wheel stretches the wave taller or flatter (the HEIGHT chip shows the stretch and puts it back); Shift-drag slides the trace onto the grid (the OFFSET chip puts it back). <strong>HOLD</strong> keeps a ghost of the loudest pass, so a beat without a kick fades the picture instead of erasing it. Your Channels are drawn in their colours: the chip cycles MIX + CH, CHANNELS, MIX. It needs a tempo and a position - the Bridge's, or a Channel's.</p>", "Put the kick and the bass in the scope at 1/4 and you see, not guess, where the bass sits against the kick's tail.")}
"""})

    S.append({"title": "Step-by-step workflows", "sub": "how a session with the analyser looks", "html": f"""
{recipe("Compare your mix to a reference", "Mixing and mastering",
 steps([
  "REFERENCE &rarr; pick a record in your genre. Click the waveform to put the bright 16 bars on its drop.",
  "Set your DAW to loop the drop of your own track. The reference curve sits over your bars; the white line is your live average.",
  "Where the two disagree, click that zone (or press and hold the band) to hear it alone - in your mix, and with DAW / REF, in the record.",
  "Open the second vectorscope (+) and put the record in it: width beside width.",
  "Fix it in your EQ, and watch the white line move towards the orange one.",
 ]))}
{recipe("Measure the master through the Bridge", "Final checks before export",
 steps([
  "Insert <strong>RONE Analyzer Bridge</strong> last on the DAW master, after the limiter. The analyser switches to it.",
  "Open LOUDNESS (L), choose the target, press RESET and play the whole song.",
  "Read Integrated, Range and True Peak; OVER -1 must not light for a streaming master. The meters' OVR rows must stay dark.",
  "Menu &rarr; <em>Bridge bypass</em> to compare with and without your master chain while watching the same meters.",
 ]))}
{recipe("Kick and bass", "The low end of a dance track",
 steps([
  "Put a <strong>RONE Analyzer Channel</strong> on the kick and one on the bass; name and colour them in the plugin. Both appear over the master.",
  "Click SUB and BASS to hear only the low end. Read NOTE: it is the note the bass is actually sitting on.",
  "Press M. If the low end thins out, part of it cancels in mono - look at SIDES to see which band.",
  "Open SCOPE at 1/4 and watch the bass against the kick's tail, pass after pass.",
 ]))}
{recipe("Check a delivered file", "Mastering QC",
 steps([
  "Menu &rarr; <em>Open a file and scan</em>. The whole file runs through every instrument in seconds.",
  "Read LOUDNESS (integrated, true peak), MAX PEAK and the overs.",
  "Menu &rarr; <em>Open a file and play</em> to listen to any section that looked suspicious; Space stops and starts it.",
 ]))}
"""})

    S.append({"title": "Recommendations", "sub": "reading meters well", "html": f"""
<div class="dos">
<div><h4>Do</h4><ul>
<li>Keep the Bridge last in the master chain, after the limiter, so you measure what leaves the DAW.</li>
<li>Compare against the reference on the same 16 bars of both tracks, at the same spot in the arrangement.</li>
<li>Reset before measuring integrated loudness, and let the whole track play.</li>
<li>Put it on a second monitor and press H. The instruments are then as large as the screen allows.</li>
<li>Use FULL to see the record whole, LINE to see your average against it.</li>
</ul></div>
<div><h4>Avoid</h4><ul>
<li>Reading "what the computer plays" while the DAW runs on ASIO - it will read silence. Use the Bridge, or the ASIO source.</li>
<li>Judging width by correlation alone; SIDES tells you which band it is coming from.</li>
<li>Leaving a band or a zone soloed. It changes what you hear; Escape lets everything back.</li>
<li>A Channel plugin from an older version beside a newer analyser - update both from the Center, they ship together.</li>
</ul></div>
</div>
<h3>Reading tips</h3>
<ul>
<li>The white LED is the held peak; the lit column is the level now. A column whose peak sits far above its LEDs is transient, not loud.</li>
<li>A hard-panned element shows in SIDES at its full level. That is physics, not a problem; a <em>kick</em> in SIDES is.</li>
<li>TRUE PEAK rounds up. A meter that rounds to nearest can read 0.1 lower on the same file.</li>
</ul>
"""})

    S.append({"title": "Interface conventions", "sub": "menus, keys, Appearance", "html": f"""
<h3>The menu</h3>
<p>The button at the right of the header: <em>Open a file and play</em>, <em>Open a file and scan</em>, Play / Stop and <em>Scan the whole file again</em> for the loaded file, <em>Loop the file</em>; <em>Appearance</em> (F4); <em>Reset all measurements</em>, <em>Restore default layout</em>; <em>Bridge bypass</em> while the Bridge is in the DAW; <em>Always on top</em>; <em>Start with Windows</em>; <em>Hints: a tour of everything</em> (F1); About; Quit.</p>
{table(["Key", "What it does"], [
 ("<code>F1</code>", "The Hints tour. Arrows step, Escape ends."),
 ("<code>F2</code>", "Setup - the spectrogram's, the meters' and the scope's settings."),
 ("<code>F3</code>", "The SOURCE menu."),
 ("<code>F4</code>", "Appearance."),
 ("<code>H</code>", "Hide the chrome: the header and the status bar go, the instruments fill the window. <code>Escape</code> brings them back."),
 ("<code>W</code>", "BARS / WAVES."),
 ("<code>F</code>", "Freeze the spectrogram's bars; again to let them go. The meters and the scope keep running."),
 ("<code>L</code>", "Show or hide LOUDNESS."),
 ("<code>M</code>", "Fold to mono - on screen, and in the DAW through the Bridge."),
 ("<code>V</code>", "Vectorscope LOCKED / FREE."),
 ("<code>O</code>", "Open or close the BAR SCOPE."),
 ("<code>Space</code>", "Play or stop the file that is the source."),
 ("<code>S</code>", "See the Channel under the pointer alone."),
 ("<code>Escape</code>", "Let go of a band or zone solo; close a panel; bring the chrome back."),
 ("Drop an audio file on the window", "Analyse it."),
 ("Close the window", "Quit. Source, window, theme and layout come back as they were."),
 ("Command line", "<code>--file &lt;path&gt;</code>, <code>--scan &lt;path&gt;</code>, <code>--reference &lt;path&gt;</code>, <code>--loopback [name]</code>, <code>--bridge</code>, <code>--asio [device]</code>, <code>--input &lt;device&gt;</code>. Without arguments it reopens the last source."),
])}
<div class="two">
<div><h4>Appearance</h4><p><em>Appearance</em> in the menu (or F4) opens the theme panel over the right of the window; the instrument behind it is the preview and changes as you choose. Eleven built-in themes - RONE, Graphite, Ice, Phosphor, Ember, Fjord, Tokyo, Mocha, Rose, Solar, Mono - and your own. <strong>Meter colour</strong> is the level ramp or the theme's own colour; <strong>meter style</strong> LED segments or solid bars; the 5 dB rows can be shown or hidden. <strong>Customize</strong> sets every colour - surfaces, meters, marks and text - by swatch or hex, with Copy and Paste on every row so one colour can be matched across panels. <em>Save</em> keeps your colours under a name, <em>Set as default</em> marks the theme that <em>Reset</em> returns to. Nothing is kept until <em>Apply theme</em>; Cancel or Escape puts everything back.</p></div>
<div><h4>The window</h4><p>The grip at the bottom-right corner resizes the whole instrument. <strong>FIXED</strong> (the lock in the status bar) keeps the window's shape and zooms the instrument with it; <strong>FREE</strong> lets it take any size. Source, channel pair, every setting, the theme and the window position persist between runs.</p>
<h4>Settings</h4><p>The spectrogram's, the meters' and the scope's gears each open their own section of Setup; F2 opens them all. Every value applies at once.</p></div>
</div>
"""})

    S.append(G["support_section"](m, [
        ("The analyser shows silence on a device that is playing", "Applications on ASIO or in exclusive mode bypass the system playback path. Insert the Bridge on the DAW's master, or choose the ASIO source; on an interface with its own mixer, a loopback of that submix makes the playback appear as an input."),
        ("A source is greyed out or 'something else is holding this device'", "Another application has the endpoint in exclusive mode. Close it, or pick a different endpoint."),
        ("The DAW master entry says 'insert RONE Analyzer Bridge on the master'", "The Bridge is not running in the DAW, or the DAW has not scanned it yet. Rescan plugins; on Windows the Bridge is at <code>C:\\Program Files\\Common Files\\VST3\\RONE Analyzer Bridge.vst3</code>, on macOS in <code>/Library/Audio/Plug-Ins/VST3</code> and <code>Components</code>. Its window must say LINKED."),
        ("A Channel is in the DAW but no chip appears", "An older Channel plugin beside a newer analyser, or a project opened before the plugin was updated. Update from the Center (the Analyzer and its plugins ship together) and reopen the project."),
        ("Zone or band solo is only drawn, not heard", "Only the Bridge plays the solo in the DAW. On other sources it is on screen, and on the reference player."),
        ("The BAR SCOPE stays empty", "It needs a tempo and a position: the Bridge's, or a Channel's. A file or an input has neither."),
        ("macOS: 'Listening to an output needs macOS 14.2'", "Older macOS cannot tap an output. Insert the Bridge on the DAW's master, or route the output through BlackHole."),
        ("The window opened off-screen or at a strange size", "Menu &rarr; <em>Restore default layout</em>, or switch FIXED to FREE in the status bar and resize."),
    ]))

    m["sections"] = S
    return m
