def manual(G):
    img, legend, ctl, note, steps, recipe, table = (G[k] for k in ("img", "legend", "ctl", "note", "steps", "recipe", "table"))
    m = {
        "id": "analyzer", "product": "RONE Analyzer", "eyebrow": "PROFESSIONAL MEASUREMENT SUITE",
        "title_html": "RONE <i>Analyzer</i>", "accent": "#22E36B", "version": "1.0",
        "tagline": "A DIGICheck-style measurement suite: 30-band third-octave analyser with reference corridors, vectorscope with phosphor afterglow, level meters, EBU R128 loudness and bit statistics, reading ASIO, loopback, a live input, a file or your DAW's master bus.",
        "formats": ["Standalone", "VST3 bridge"], "vst3": "RONE Analyzer Bridge.vst3", "au": "", "exe": "RONE Analyzer.exe",
        "pdf": "RONE Analyzer - User Manual.pdf", "cover_img": "analyzer/main.png",
        "channels": "Any stereo pair of the selected source; mono fold available",
        "engine": "JUCE 8, native interface (no WebView needed)",
        "latency": "Not applicable (measurement only; the bridge plugin is transparent)",
    }
    S = []

    S.append({"title": "Welcome to RONE Analyzer", "sub": "what it measures and why it is built this way", "html": f"""
<p><strong>RONE Analyzer</strong> is a standalone measurement application in the shape of the classic RME DIGICheck: modular instruments in a panel grid, a real bandpass spectrum analyser rather than an FFT, a vectorscope with proper phosphor afterglow, bit statistics and EBU R128 loudness. It reads audio from an ASIO interface (alongside your DAW), from whatever Windows is playing (loopback), from a live input, from a file, or from your DAW's master bus through the included <strong>RONE Analyzer Bridge</strong> VST3.</p>
<p>The point of a separate application is that it is always on screen, always measuring the same thing, whatever project or plugin you have open. The point of the bridge is that you can also put the analyser exactly where a plugin would sit: on your master, after your limiter.</p>
<div class="two">
<div>
<h3>The instruments</h3>
<ul>
<li><strong>Totalyser</strong> - 30-band ISO third-octave analyser (also 1/1 and 1/6 octave), PEAK / HOLD / RTA traces, L/R or Mid/Side, per-band stereo strip, hover readout, band solo by ear, and a <em>reference corridor</em> built from your reference tracks.</li>
<li><strong>Level Meters</strong> - peak and RMS per channel, crest factor, configurable over detection, K-System scales.</li>
<li><strong>Stereo Vectorscope</strong> - goniometer with bright / dim afterglow, correlation meter with a low-water mark.</li>
<li><strong>Loudness (EBU R128)</strong> - momentary, short-term, integrated, LRA, true peak, target deviation.</li>
<li><strong>Bit Statistic &amp; Noise</strong> - true word length, stuck bits, SNR, noise floor, DC offset.</li>
<li><strong>Spectroscope</strong> and <strong>Oscilloscope</strong>, plus a <strong>Global Level Meter</strong> for many-channel interfaces.</li>
</ul>
</div>
<div>
<h3>The 10-second version</h3>
{steps([
 "Open RONE Analyzer.",
 "SOURCE &rarr; pick the endpoint with the live meter next to it.",
 "Read the spectrum, the meters and the scope.",
 "Put the RONE Analyzer Bridge on your DAW master to measure the mix itself."])}
<h3>Why bandpass, not FFT</h3>
<p>An FFT has linear bins; an ear does not. The Totalyser runs thirty real bandpass filters on the ISO 266 centres (25 Hz to 20 kHz), so every band is a third of an octave wide and a tone reads its own level. It is the same axis you see on DIGICheck and on every third-octave RTA in a mastering room.</p>
</div></div>
"""})

    S.append(G["install_section"](m))

    S.append({"title": "Quick start", "sub": "seeing your mix in the analyser", "html": f"""
{img("analyzer/main.png", "<b>RONE Analyzer</b> with the default three-panel layout: Totalyser, Level Meters, Vectorscope.")}
{steps([
 "<strong>Start</strong> RONE Analyzer from the Start menu (Windows) or Applications (macOS). It remembers its source, layout and window position between runs.",
 "<strong>Choose a source.</strong> Click <span class='pill'>SOURCE</span>. The menu lists every audio endpoint on the machine with a small live meter next to each - the one that moves is the one carrying your mix. Pick an ASIO device to read your interface's inputs (works alongside a running DAW on multiclient drivers such as RME), a loopback endpoint to read whatever Windows is playing (Spotify, YouTube, references), or a live input for a microphone or line signal.",
 "<strong>Or open a file.</strong> <span class='pill'>FILE</span> &rarr; <em>Open and play (real time)</em> to hear and measure it, or <em>Open and scan (fast, whole file)</em> to push the whole file through the meters in seconds. You can also drop a file on the window.",
 "<strong>Measure inside the DAW.</strong> Insert <strong>RONE Analyzer Bridge</strong> on your master bus (last in the chain). Then choose <em>DAW master (RONE Bridge plugin)</em> in the SOURCE menu. The bridge taps the master and sends it to the analyser; band solo and mono fold then act inside the DAW's own signal path.",
 "<strong>Arrange the panels.</strong> <span class='pill'>LAYOUT</span> offers one to four panels in five arrangements. Each panel's &#8942; button swaps the instrument in that panel or opens its settings; drag a panel's top-left grip onto another to reorder; drag the splitters to resize; double-click a splitter to reset.",
 "<strong>Save the workspace.</strong> The layout preset menu at the top right saves, loads and deletes layouts and can set one as default. A <em>*</em> next to the name means the current layout has unsaved changes.",
])}
{note("Zero output channels", "The analyser opens your device with no output channels so it can sit beside your DAW without fighting for the outputs. Monitoring (band solo by ear) needs an output and therefore reopens the device; it is never on at start-up and is only offered on hardware sources.")}
"""})

    S.append({"title": "Interface tour", "sub": "every element on screen", "html": f"""
{img("analyzer/tour.png", "The application window.")}
{legend([
 ("Logo", "RONE Analyzer; the header collapses to a thin strip with the chevron at its right end"),
 ("SOURCE", "choose the input: ASIO, loopback, live input, file or DAW master"),
 ("LAYOUT", "1 to 4 panels, five arrangements"),
 ("FILE", "open and play, open and scan, loop, close source"),
 ("REFERENCE", "add reference tracks, show or clear the reference corridor"),
 ("Source chip", "the current source and sample rate; green dot = running"),
 ("Layout preset", "MY LAYOUT (DEFAULT): save, load, delete, set as default"),
 ("RESET", "reset all measurements (peak holds, integrated loudness, overs)"),
 ("App menu", "Appearance, Totalyser Setup, Bridge bypass (A/B), Start with Windows, About, Quit"),
 ("Totalyser", "the spectrum analyser panel"),
 ("Band resolution", "1/1, 1/3 or 1/6 octave"),
 ("PEAK / HOLD / RTA", "trace type"),
 ("Stereo link", "summed, L/R or Mid/Side bars"),
 ("Panel settings", "gear: range, scale, decay, hold, smoothing"),
 ("Level Meters", "RMS L, Peak L, Peak R, RMS R with OVR counter"),
 ("Vectorscope", "goniometer with correlation meter and the RONE wordmark"),
 ("STEREO / SQUARE", "mono fold and display aspect"),
 ("Correlation meter", "-1 to +1 with a low-water mark"),
])}
"""})

    S.append({"title": "Instruments reference", "sub": "how to read each one", "html": f"""
<h3>Totalyser</h3>
{ctl("Bands", "1/1 (10) &middot; 1/3 (30) &middot; 1/6 (60) octave<br>default 1/3", "<p>Third-octave bands on the ISO 266 series: 25, 31.5, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500, 630, 800, 1k, 1k25, 1k6, 2k, 2k5, 3k15, 4k, 5k, 6k3, 8k, 10k, 12k5, 16k, 20k. Labels alternate between two rows so all thirty stay legible.</p>")}
{ctl("PEAK / HOLD / RTA", "trace", "<p><strong>PEAK</strong> shows the instantaneous peak per band; <strong>HOLD</strong> adds peak-hold caps; <strong>RTA</strong> shows the averaged (RMS) spectrum, which is the trace to compare against a reference.</p>")}
{ctl("Stereo link", "sum &middot; L/R &middot; M/S", "<p>Bars show the summed spectrum, both channels, or Mid and Side. Under the plot a per-band <strong>stereo strip</strong> shows width or mono damage: mono content loses nothing when summed, hard-panned content loses 3 dB (the panning law, not a fault), and anything worse than -3 dB is real cancellation and is coloured as such.</p>")}
{ctl("Hover readout", "", "<p>Hover a band to read its level, crest factor, nearest note, Mid/Side split and mono delta in one box.</p>")}
{ctl("Band solo (monitor)", "press and hold a band", "<p>Hear only that band. Drag while holding to sweep between centres; the mouse wheel changes Q while holding and the highlighted width is the filter's own -3 dB width. Double-click latches the solo; any single press releases it. The other bands go dark while soloing, so eyes and ears agree. MONO / SIDE / L / R, polarity flip, mono-below-X, low / high pass and monitor gain live in the header's Monitor menu. Available on hardware sources only.</p>", "Soloing a band around 200-400 Hz while comparing to a reference is the quickest way to hear mud.")}
{ctl("Reference corridor", "REFERENCE menu", "<p>Add several reference tracks and the analyser reduces them to a <em>range</em> per band rather than a single curve - two records you admire will disagree by several dB in any given band, and the shape they share is the only part worth aiming at. The corridor is drawn as a tint behind the bars with edges in front; a white curve shows your live average spectrum, which is what is actually comparable. The readout reports distance outside the corridor and exactly zero inside it.</p>")}
{ctl("Setup (gear / Totalyser Setup...)", "range, scale, decay, hold, smoothing, Rise T., FSC for OVR, Correlator Int.", "<p>DIGICheck's own setup dialog with DIGICheck's own words, so existing users can type their numbers straight in. Rise T. is the band detector attack (0 = instant, 15 ms = RME default). Analyser release is in seconds to fall the whole range; meter release is in dB per second - both as in DIGICheck.</p>")}
<h3>Level Meters</h3>
{ctl("Layout", "RMS L | Peak L | Peak R | RMS R", "<p>Inner bars are peak (AC+DC), outer bars are RMS (AC only). The width of the coloured region between them is the crest factor, also printed underneath with peak and RMS. 0 dBFS is referenced to a full-scale sine, so RMS carries the +3.01 dB the RME convention requires. The scale is DIGICheck's non-linear one, with the top 20 dB stretched.</p>")}
{ctl("Overs", "N consecutive samples, 1-20", "<p>An <em>over</em> is a run of N consecutive full-scale samples - one clipped sample is not an over. Overs are counted next to the OVR cap. Click the meter to clear. Peak hold 0.2-100 s; range down to -160 dB; K-20 / K-14 / K-12 scales.</p>")}
<h3>Stereo Vectorscope</h3>
{ctl("Goniometer", "45&deg; rotated: mono is a vertical line", "<p>Draws continuous lines between successive samples with intensity inversely proportional to segment length, the way a real beam deposits energy - transients leave a faint streak, sustained tones a bright figure. Afterglow has separate decay rates for bright and dim pixels (Release 1 / Release 2, in seconds) plus AGC; Polar and Lissajous modes are in the panel settings.</p>")}
{ctl("Correlation", "-1 to +1", "<p>Fills from -1 to the reading, so more fill is better. Zero is marked hard: one side of that line sums to mono, the other cancels. Below zero the caption reads OUT OF PHASE and turns red. A low-water mark remembers the most negative reading of the last few seconds, so a dip you did not see is still reported.</p>")}
{ctl("STEREO / mono fold", "top-left of the panel", "<p>Folds every instrument to mono at once. It sits on the vectorscope because that is where you are looking when you want it.</p>")}
<h3>Loudness (EBU R128)</h3>
{ctl("Readings", "M &middot; S &middot; I &middot; LRA &middot; True Peak", "<p>Momentary (400 ms), short-term (3 s) and gated integrated loudness in LUFS, loudness range in LU and 4&times; oversampled true peak in dBTP. Targets for streaming, podcast and broadcast show the deviation in LU. The K-weighting filter is rebuilt for every sample rate from BS.1770's design, so 44.1, 88.2 and 96 kHz measure exactly as 48 kHz does.</p>", "Press RESET when the song starts so the integrated value covers only the material you mean.")}
<h3>Bit Statistic &amp; Noise</h3>
{ctl("Per-bit state", "green = toggling, blue = never set, red = stuck", "<p>Count the green bits and you have the true word length regardless of what the file header claims. Flat and A-weighted SNR, measured noise floor and DC offset sit below. This is the instrument that catches a '24-bit' file that is really 16, a truncated export, or a converter with an offset.</p>")}
<h3>Spectroscope and Oscilloscope</h3>
{ctl("Spectroscope", "scrolling spectrogram", "<p>Same third-octave axis as the analyser, over time. Resonances and intermittent noise appear as horizontal streaks that a bar graph cannot show.</p>")}
{ctl("Oscilloscope", "triggered, with afterglow", "<p>A triggered scope with a 10&times;8 graticule for waveform shape, clipping and DC.</p>")}
"""})

    S.append({"title": "Step-by-step workflows", "sub": "how a session with the analyser looks", "html": f"""
{recipe("Compare your mix to references", "Mixing and mastering",
 steps([
  "REFERENCE &rarr; <em>Add reference tracks...</em> and pick three to five commercial tracks in your genre. Each is scanned in a few seconds.",
  "Enable <em>Show reference</em>. The corridor appears behind the Totalyser bars.",
  "Set the trace to RTA and play your mix. Watch the white average curve against the corridor: inside is fine, outside is where to look.",
  "Press and hold a band that sits outside the corridor to hear it in isolation, then fix it in your EQ.",
 ]))}
{recipe("Measure the master through the bridge", "Final checks before export",
 steps([
  "Insert <strong>RONE Analyzer Bridge</strong> last on the DAW master (after the limiter).",
  "In the analyser choose SOURCE &rarr; <em>DAW master (RONE Bridge plugin)</em>.",
  "Put Loudness in one panel and Level Meters in another. Press RESET, play the whole song, read Integrated, LRA and True Peak.",
  "Use the app menu's <em>Bridge bypass (A/B)</em> to compare with and without your master chain while watching the meters.",
 ]))}
{recipe("Check a delivered file", "Mastering QC",
 steps([
  "FILE &rarr; <em>Open and scan (fast, whole file)</em>. The whole file runs through every instrument in seconds.",
  "Read Bit Statistic (true word length, DC), Loudness (integrated, true peak) and the Level Meters' over count.",
  "Switch to <em>Open and play (real time)</em> to listen to any section that looked suspicious.",
 ]))}
{recipe("Mono compatibility", "Any mix",
 steps([
  "Watch the correlation meter and its low-water mark through the chorus.",
  "Look at the stereo strip under the Totalyser: bands coloured for mono damage are cancelling, not just wide.",
  "Press STEREO on the vectorscope panel to fold everything to mono and listen.",
 ]))}
"""})

    S.append({"title": "Recommendations", "sub": "reading meters well", "html": f"""
<div class="dos">
<div><h4>Do</h4><ul>
<li>Use the RTA trace when comparing tonal balance; PEAK is for transient checks.</li>
<li>Build the reference corridor from several tracks, not one. One master's accidents are not a target.</li>
<li>Reset before measuring integrated loudness, and let the whole track play.</li>
<li>Save a layout per task: tracking, mixing, mastering. Set your most-used one as default.</li>
<li>Keep the bridge last in the master chain, after the limiter, so you measure what leaves the DAW.</li>
</ul></div>
<div><h4>Avoid</h4><ul>
<li>Reading the loopback source while an ASIO application bypasses Windows audio - it will read silence. Use the ASIO source instead.</li>
<li>Judging width by correlation alone; the per-band strip tells you where it is coming from.</li>
<li>Leaving band solo latched. It blanks the display and changes what you hear; a single press releases it.</li>
</ul></div>
</div>
<h3>Reading tips</h3>
<ul>
<li>A mix that is too bright by the crest factor of its top end will look fine against the corridor with the PEAK trace and wrong with RTA. RTA is the honest one.</li>
<li>Hard-panned elements show -3 dB in the mono delta. That is physics, not a problem; below -3 dB is.</li>
<li>Two green bits fewer than expected in the Bit Statistic usually means a dither or truncation stage you forgot about.</li>
</ul>
"""})

    S.append({"title": "Interface conventions", "sub": "panels, menus, shortcuts", "html": f"""
{table(["Gesture", "What it does"], [
 ("&#8942; on a panel", "Change the instrument in that panel, or open its settings."),
 ("Drag a panel's top-left grip onto another panel", "Swap the two panels."),
 ("Drag a splitter", "Resize panels. Double-click a splitter to reset the sizes."),
 ("Chevron at the right end of the header", "Collapse the header to a 16 px strip; panel controls remain in each panel's top-right corner."),
 ("Click a level meter", "Clear peak hold and the over counter."),
 ("Press and hold a Totalyser band", "Solo that band by ear (hardware sources). Drag to sweep; wheel for Q; double-click to latch."),
 ("Drop an audio file on the window", "Scan it."),
 ("Start with Windows (app menu)", "Launch at login, minimised to the tray. The tray icon restores the window."),
 ("Command line", "<code>--file &lt;path&gt;</code>, <code>--scan &lt;path&gt;</code>, <code>--loopback [name]</code>, <code>--asio [device]</code>, <code>--input &lt;device&gt;</code>, <code>--add-references &lt;paths...&gt;</code>. Without arguments it reopens the last source."),
])}
<div class="two">
<div><h4>Appearance</h4><p>The app menu's <em>Appearance...</em> switches themes and the accent colour. The graphite theme is the default and matches the RONE plugins.</p></div>
<div><h4>Settings and layouts</h4><p>Source, layout, channel pair and setup values persist between runs in your user folder. Layout presets live in the same place.</p></div>
</div>
"""})

    S.append(G["support_section"](m, [
        ("The analyser shows silence on a device that is playing", "Windows applications that use ASIO or WASAPI exclusive mode bypass the loopback path. Choose the ASIO source and, on RME hardware, enable a Loopback on that submix in TotalMix so the playback appears as an input."),
        ("A source is greyed out or reads 'something else is holding this device'", "Another application has the endpoint in exclusive mode. Close it, or pick a different endpoint."),
        ("No 'DAW master' entry appears", "The RONE Analyzer Bridge is not loaded in the DAW, or the DAW has not scanned it yet. Rescan plugins; the bridge is at <code>C:\\Program Files\\Common Files\\VST3\\RONE Analyzer Bridge.vst3</code>."),
        ("Monitoring (band solo) is not offered", "Only hardware sources can monitor. Loopback capture has no output half and a file is pushed by its own thread."),
        ("The window opened off-screen or at a strange size", "Use the app menu's <em>Restore default layout</em>, or delete the window state from the settings file in your user folder while the app is closed."),
    ]))

    m["sections"] = S
    return m
