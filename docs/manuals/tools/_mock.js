// Fake JUCE bridge for documentation screenshots (served copy only).
(function () {
  var listeners = {};
  var backend = {
    addEventListener: function (name, cb) { (listeners[name] = listeners[name] || []).push(cb); },
    emitEvent: function (name, payload) {
      if (name === '__juce__invoke' && payload && typeof payload.resultId === 'number') {
        var r = (window.__mockNative && window.__mockNative[payload.name]) ? window.__mockNative[payload.name](payload.params) : null;
        setTimeout(function () { backend.dispatch('__juce__complete', { promiseId: payload.resultId, result: r }); }, 5);
      }
    },
    dispatch: function (name, data) { (listeners[name] || []).forEach(function (cb) { try { cb(data); } catch (e) { console.warn(name, e); } }); }
  };
  window.__JUCE__ = { backend: backend, initialisationData: {} };
  window.__mockNative = {
    getLicenseStatus: function () { return { licensed: true, customerName: 'RONE User' }; },
    getAllParameters: function () { return { reverbSize: 0.8, dryWet: 1.0, outputGain: 1.0, tailDivision: 4, lowCutFreq: 20, stereoWidth: 0.5,
      preDelay: 0, highCutFreq: 20000, manualBpm: 120, transitionMode: false, tremoloEnabled: false, tremoloDepth: 0.5, tremoloPan: 0,
      tremoloRate: 4.0, tremoloWaveform: 0, tremoloSyncEnabled: false, tremoloSyncDivision: 2, tremoloRateRampEnabled: false,
      tremoloStartDivision: 5, tremoloEndDivision: 7, fadeIn: 0, fadeOut: 0, fadeCurveIn: 2, fadeCurveOut: 2 }; },
    getPluginInfo: function () { return { version: '1.0.0', format: 'VST3', isStandalone: false, name: 'RONE Reverse Reverb' }; }
  };
  window.__mockEmit = function (name, data) { backend.dispatch(name, data); };
  // deterministic pseudo-random helper for waveform mocks
  window.__mockRand = function (seed) { var s = seed || 1; return function () { s = (s * 16807) % 2147483647; return s / 2147483647; }; };
})();

// ---- Scenario: ReverseReverb with a vocal sample loaded ----
window.__scn_rr = function () {
  var N = 1400, rnd = window.__mockRand(7), mn = [], mx = [];
  for (var i = 0; i < N; i++) {
    var x = i / N;
    // reversed swell: quiet -> loud, with a few syllable bumps
    var env = Math.pow(x, 1.6) * 0.95;
    var bumps = 0.25 * Math.exp(-Math.pow((x - 0.35) / 0.05, 2)) + 0.35 * Math.exp(-Math.pow((x - 0.62) / 0.04, 2)) + 0.2 * Math.exp(-Math.pow((x - 0.8) / 0.03, 2));
    var a = Math.min(1, env + bumps) * (0.55 + 0.45 * rnd());
    mx.push(a); mn.push(-a * (0.7 + 0.3 * rnd()));
  }
  window.__mockEmit('fileLoaded', { fileName: 'Vocal_Phrase_128bpm.wav' });
  window.__mockEmit('playbackState', { isPlaying: false, playbackProgress: 0, sampleLoaded: true, effectiveBpm: 128 });
  window.__mockEmit('waveformData', { min: mn, max: mx });
};

// ---- Scenario: Stutter with a drum loop loaded (original view) / processed (result view) ----
// 1.4.0: the tour shows AUTO at work - STEREO 100 % walking down to the centre,
// FADE OUT 30 % walking up - so the travel rings are in the picture.
window.__scn_stutter_auto = function () {
  window.__scn_stutter(true);
  state.stereoDelay = 1; state.stereoAuto = 2; state.fadeOut = 0.3; state.fadeOutAuto = 1;
  if (window.updateStereoSlider) window.updateStereoSlider(1);
  if (window.updateFadeOutSlider) window.updateFadeOutSlider(0.3);
  updateSegUI();
};
// 1.4.0: FREE + TREMOLO. The result is re-drawn with repeats that glide from
// 180 ms to 25 ms, so the picture shows what the controls describe.
window.__scn_stutter_free = function () {
  window.__scn_stutter(true);
  setAdvanced(true);
  state.division = 7; state.freeMs = 180; state.tremolo = 1; state.freeEndMs = 25;
  state.fadeOut = 0.55; if (window.updateFadeOutSlider) window.updateFadeOutSlider(0.55);
  updateSegUI(); updateDivisionUI();
  var rnd = window.__mockRand(7), P = [], M = 2400, clip = 1.875, pos = 0, starts = [];
  while (pos < clip) { starts.push(pos); pos += (180 + (25 - 180) * (pos / clip)) / 1000; }
  for (var j = 0; j < M; j++) {
    var t = j / M * clip, k = 0;
    while (k + 1 < starts.length && starts[k + 1] <= t) k++;
    var len = (k + 1 < starts.length ? starts[k + 1] : clip) - starts[k], u = (t - starts[k]) / len;
    var a = 0.92 * Math.exp(-u * 3.2) * (u < 0.45 ? 1 : Math.max(0, 1 - (u - 0.45) / 0.55)) * (0.55 + 0.45 * rnd());
    P.push(j % 2 === 0 ? a : -a * 0.7);
  }
  window.__mockEmit('waveformData', { type: 'processed', samples: P, lengthSamples: 90000 });
  window.__mockEmit('renderComplete', { success: true });
};
// ---- Scenario: Stutter 2.0 LIVE. The scope is fed as the host would: a channel running, bar and beat
// marks, one caught fill (flags: 1 fill, 2 first slice, 4 beat, 8 bar, 16 catch). trig: 0 AUTO, 1 HOLD ----
window.__scn_stutter_live = function (trig) {
  window.__scn_stutter(false);
  setLiveMode(true, true);
  state.liveTrigMode = trig; updateLiveSegs();
  window.__mockEmit('livePads', { pads: [
    { name: 'Roll 1/16', user: false }, { name: 'Speed-Up', user: false }, { name: 'Gate 1/16', user: false },
    { name: 'Catch & Roll', user: false }, { name: 'Drop Brake', user: false }, { name: 'Alien Talk', user: false }] });
  var rnd = window.__mockRand(3), N = 860, cols = '', hop = 12, fillAt = 560, fillLen = 200;
  function hex(v) { return ('0' + v.toString(16)).slice(-2); }
  for (var i = 0; i < N; i++) {
    var beat = i % 48, bar = i % 192 === 0, inFill = i >= fillAt && i < fillAt + fillLen, u = (i - fillAt) % 12;
    var a = (0.16 + 0.80 * Math.exp(-beat / 11)) * (0.6 + 0.4 * rnd());
    if (inFill) a = 0.78 * Math.exp(-u / 7) + 0.08;
    var f = (beat === 0 ? 4 : 0) | (bar ? 8 : 0) | (inFill ? (i < fillAt + 12 ? 2 : 1) : 0) | (i === fillAt ? 16 : 0);
    cols += hex(Math.min(255, Math.round(Math.sqrt(a) * 255))) + hex(f);
  }
  window.__mockEmit('liveState', { cols: cols, active: -1, waiting: -1, midi: 0, needsFile: false, rolling: true, toAuto: 6 });
};
// ---- Scenario: Stutter COMPLEX, its first preset's row on the lane, step 2 open in the editor ----
window.__scn_stutter_complex = function () {
  window.__scn_stutter(true);
  setAdvanced(true);
  document.getElementById('complexBtn').click();
  if (window.openCxEditor) openCxEditor(1);
};
window.__scn_stutter = function (processed) {
  var N = 1800, rnd = window.__mockRand(11), s = [];
  var hits = [0.0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875];
  for (var i = 0; i < N; i++) {
    var x = i / N, a = 0.06;
    for (var h = 0; h < hits.length; h++) { var d = x - hits[h]; if (d >= 0) a += (h % 2 === 0 ? 0.95 : 0.6) * Math.exp(-d * 28); }
    a = Math.min(1, a) * (0.5 + 0.5 * rnd());
    s.push(i % 2 === 0 ? a : -a * (0.6 + 0.4 * rnd()));
  }
  window.__mockEmit('bpmState', { dawAvailable: true, dawBPM: 128, manualBPM: 120, effectiveBPM: 128 });
  window.__mockEmit('fileLoaded', { fileName: 'Drum_Loop_128.wav', sampleRate: 48000, lengthSamples: 90000, duration: 1.875 });
  window.__mockEmit('waveformData', { type: 'original', samples: s });
  window.__mockEmit('transientsDetected', { transients: hits.map(function (h) { return h + 0.002; }), count: hits.length });
  if (processed) {
    // the selected hit (2nd snare) repeated on a 1/16 grid, shrinking fades
    var P = [], M = 2400, slices = 32;
    for (var j = 0; j < M; j++) {
      var y = j / M, k = Math.floor(y * slices), u = (y * slices) - k;
      var a2 = 0.9 * Math.exp(-u * 4) * (0.55 + 0.45 * rnd()) * (1 - 0.45 * (k / slices));
      P.push(j % 2 === 0 ? a2 : -a2 * 0.7);
    }
    window.__mockEmit('selectionState', { selectedNorm: 0.252, chunkLengthNorm: 0.031 });
    window.__mockEmit('waveformData', { type: 'processed', samples: P, lengthSamples: 180000 });
    window.__mockEmit('renderComplete', { success: true });
  }
};

// ---- Scenario: Clipper with a drum loop running (feeds the `frame` events the host would push) ----
window.__scn_clipper = function (view) {
  var rnd = window.__mockRand(5), bps = 8000, t = 0;
  function env(tt) {
    var beat = tt % 0.5, kp = -1.2;
    var kick = beat < 0.05 ? kp - beat * 50 : (kp - 2.5) - (beat - 0.05) * 95;
    var hb = (tt + 0.25) % 0.5, hat = hb < 0.07 ? -9 - hb * 140 : -99;
    var sb = tt % 1.0 - 0.5, snare = (sb > 0 && sb < 0.16) ? (-4 - sb * 45) : -99;
    return Math.max(-26 + rnd() * 2.6 - 1.3, kick, hat, snare);
  }
  state.clip_db = -4.5; state.lowcut = 20; renderKnob(); renderLowcut();
  for (var k = 0; k < 70; k++) {
    var b = [];
    for (var i = 0; i < 1000; i++) {
      t += 1 / bps; var a = Math.pow(10, env(t) / 20); var ph = Math.sin(2 * Math.PI * 55 * t) * a;
      b.push(Math.min(ph, -a * 0.3 * rnd()), Math.max(ph, a * 0.3 * rnd()));
    }
    onFrame({ bps: bps, sr: 44100, ch: 2, os: 2, transition: false, latency: 384, inPk: 0.5, outPk: 0.6, tp: 0.62, wetPk: 0.6, inSq: 1, outSq: 0.9, sqN: 2, buckets: b });
  }
  if (view === 'peaks') setView('peaks');
  if (view === 'detail') {
    setView('detail');
    var n = 1100, maxs = [], mins = [], tt = [], r2 = window.__mockRand(3);
    for (var j = 0; j < n; j++) { var ms = j / 22; var amp = Math.exp(-ms / 18) * 1.9; var f = 52 + 120 * Math.exp(-ms / 4);
      var v = amp * Math.sin(2 * Math.PI * f * ms / 1000) + (ms < 1.2 ? (r2() - 0.5) * 1.4 : 0); maxs.push(v + 0.02); mins.push(v - 0.02); tt.push(0.6); }
    det = { rate: 176400, channel: 0, spanMs: 50, endOffsetMs: 12.5, availMs: 500, frozen: false, raw: false, mins: mins, maxs: maxs, t: tt };
    dirty = true;
  }
};
window.__scn_clipper_adv = function () {
  var st = document.createElement('style'); st.textContent = '*{transition:none!important;animation:none!important}'; document.head.appendChild(st);
  document.getElementById('advToggle').click();
  state.autogain = true; renderBypass();
};

// ---- Scenario: Gater - a held pad chopped by a 1/16 groove at 140 BPM, last note held ----
// The page draws only from host `frame` events: {count, bps, env, held, midiSeen, playing, bpm, ppq, buckets:[peak, env, ...]}.
window.__scn_gater = function () {
  var st = document.createElement('style'); st.textContent = '*{transition:none!important;animation:none!important}'; document.head.appendChild(st);
  state.attack_ms = 1.5; state.release_ms = 45; state.depth = 100; state.velocity = false; renderAll();
  var bpm = 140, bps = 400, step = 60 / bpm * bps / 4, rnd = window.__mockRand(11);
  // 32 sixteenths over two bars: note length in steps (0 = rest)
  // 32 sixteenths over two bars: note length in sixteenths (0 = rest); each note is let go a little early
  var pat = [1,0,1,1, 0,1,2,0, 1,0,1,1, 3,0,0,1,  1,0,1,1, 0,1,2,0, 1,1,0,1, 2,0,1,0];
  var total = Math.ceil(32 * step * 1.15), pre = total - Math.round(32 * step) - 4;
  var env = 0, tauA = 1.5 / 3 / 1000 * bps, tauR = 45 / 3 / 1000 * bps, b = [], held = 0;
  for (var i = 0; i < total; i++) {
    var s = (i - pre) / step, on = 0;
    for (var j = 0; j < 32; j++) if (pat[j] && s >= j && s < j + pat[j] - 0.4) on = 1;
    held = on;
    var tgt = on ? 1 : 0, tau = tgt > env ? tauA : tauR;
    env += (tgt - env) * (1 - Math.exp(-1 / Math.max(tau, 0.05)));
    var t = i / bps, peak = 0.62 + 0.14 * Math.sin(2 * Math.PI * t * 0.9) + 0.06 * Math.sin(2 * Math.PI * t * 7.3) + (rnd() - 0.5) * 0.08;
    b.push(Math.max(0, Math.min(1, peak)), env);
  }
  var f = { count: total, bps: bps, env: env, held: 0, midiSeen: true, playing: true, bpm: bpm, ppq: (total - pre) / (step * 4), buckets: b };
  Object.keys(frame).forEach(function (key) { if (f[key] !== undefined) frame[key] = f[key]; });
  onFrame(f); renderStatus();
  document.getElementById('midiBox').classList.add('on');   // the lamp as it flashes on a note
  // ids for the callouts (the knobs are .k[data-k] only)
  document.querySelectorAll('.k').forEach(function (el) { el.id = 'k_' + el.dataset.k; });
  var vb = document.querySelector('.p-velbox'); if (vb) vb.id = 'velBox';
};

// ---- Scenario: RONE Delay ----
// The LIVE scope draws only from host `frame` events (5 ms peak buckets: IN / L / R / duck gain), so this
// feeds two bars at 142 BPM: one short vocal hook at the start, its echoes after it. The echoes are computed
// from the page's own model - taps() (the real loop filter, FEEDBACK, SEND, WET), the stereo mode's
// routing and the engine's DUCK curve - so the picture follows the settings the shot poses.
// opt.preset: a factory preset name to load first; opt.ppq: where the playhead stands.
window.__scn_delay = function (opt) {
  opt = opt || {};
  var st = document.createElement('style'); st.textContent = '*{transition:none!important;animation:none!important}'; document.head.appendChild(st);
  frame.bpm = 142; frame.bps = 200; frame.playing = true;
  if (opt.preset) { var pi = PRESETS.findIndex(function (p) { return p.n === opt.preset; }); if (pi >= 0) loadPreset(pi); }
  if (opt.state) { Object.assign(state, opt.state); touched(); }
  var bps = 200, bpm = 142, N = 2400, ppqNow = opt.ppq != null ? opt.ppq : 71.6, rnd = window.__mockRand(5);
  var ppqOf = function (i) { return ppqNow + (i - (N - 1)) / bps * bpm / 60; };
  // the hook, once, on beat 64: three syllables (beats 0, .5, 1) - short, so every echo stands apart
  var syl = [[0, .3, .78], [.5, .25, .64], [1, .55, .72]];
  var x = new Float32Array(N);
  for (var i = 0; i < N; i++) {
    var b = ppqOf(i) - 64, a = 0;
    syl.forEach(function (s) { var u = b - s[0]; if (u >= 0 && u < s[1]) a = Math.max(a, s[2] * Math.min(1, u / .03) * (1 - .45 * u / s[1])); });
    x[i] = a > 0 ? Math.min(1, a * (.86 + .14 * rnd())) : 0;
  }
  var T = taps(), sL = secOf('L'), sR = secOf('R'), L = new Float32Array(N), R = new Float32Array(N);
  var add = function (arr, delaySec, g) { var d = Math.round(delaySec * bps); for (var i = d; i < N; i++) arr[i] += x[i - d] * g; };
  T.list.forEach(function (tp) {
    var g = Math.pow(10, tp.db / 20); if (g < 1e-3) return;
    var t = (tp.n + 1) * T.T;
    if (state.stereo === 3) { add(L, (tp.n + 1) * sL, g); add(R, (tp.n + 1) * sR, g); }
    else if (state.stereo === 2) add(tp.n % 2 === 0 ? L : R, t, g);
    else if (state.stereo === 1) { add(L, t, g); add(R, (tp.n + 1) * (T.T + SPREAD), g); }
    else { add(L, t, g); add(R, t, g); }
  });
  var out = [], env = 0;
  for (var j = N - 900; j < N; j++) {
    env = Math.max(x[j], env * .967);                 // 150 ms release at 5 ms a bucket
    var amt = env > 1e-4 ? Math.max(0, Math.min(1, (20 * Math.log10(env) + 40) / 30)) : 0, dg = 1 - state.duck / 100 * amt;
    out.push(x[j], Math.min(1, L[j] * dg), Math.min(1, R[j] * dg), dg);
  }
  var f = { bpm: bpm, ppq: ppqNow, playing: true, bps: bps, safe: 0, rests: 0, buckets: out };
  Object.keys(frame).forEach(function (k) { if (f[k] !== undefined) frame[k] = f[k]; });
  onFrame(f); renderAll();
  // ids for the callouts (the knobs and the DUAL dials carry data attributes only)
  document.querySelectorAll('.kn').forEach(function (el) { el.id = 'k_' + el.dataset.k; });
  document.querySelectorAll('#duo .dd').forEach(function (el) { el.id = 'duo' + el.dataset.s; });
  document.querySelectorAll('.scr').forEach(function (el, k) { el.id = ['scrScope', 'scrMap', 'scrFilt'][k]; });
};
