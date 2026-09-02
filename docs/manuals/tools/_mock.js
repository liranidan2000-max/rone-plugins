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
