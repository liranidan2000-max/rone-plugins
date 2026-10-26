// RONE Control manual scenarios (served copy only, never shipped). The page draws everything from the
// `ctlState` the plugin pushes at 30 Hz, so each scenario feeds one through the fake bridge (_mock.js).
// window.__scn_control('tour' | 'phone' | 'camera' | 'link')
(function () {
  // the SIM's 21-point poses (index.html), so the hand on the stage is the plugin's own glove
  var OPEN  = [[0,0],[-.30,-.18],[-.55,-.42],[-.74,-.64],[-.90,-.84],[-.30,-.95],[-.36,-1.38],[-.40,-1.66],[-.43,-1.90],[-.04,-1.00],[-.05,-1.48],[-.06,-1.78],[-.07,-2.02],[.20,-.93],[.24,-1.37],[.27,-1.63],[.29,-1.85],[.40,-.80],[.50,-1.12],[.56,-1.33],[.61,-1.52]];
  var PINCH = [[0,0],[-.30,-.18],[-.52,-.45],[-.64,-.72],[-.60,-1.02],[-.30,-.95],[-.40,-1.32],[-.50,-1.30],[-.57,-1.08],[-.04,-1.00],[-.05,-1.45],[-.06,-1.70],[-.07,-1.90],[.20,-.93],[.24,-1.34],[.27,-1.58],[.29,-1.78],[.40,-.80],[.50,-1.10],[.56,-1.29],[.60,-1.46]];
  var SW = 508, SH = 446;
  function lerp(a, b, t) { return a + (b - a) * t; }
  // a right hand at (x, y) of the stage (0..1), pinch 0..1, twisted by `twist` degrees, `depth` = size
  function hand(x, y, pinch, twist, depth) {
    var unit = 78 * depth, a = twist * Math.PI / 180, ca = Math.cos(a), sa = Math.sin(a);
    var posed = OPEN.map(function (p, i) { return [lerp(p[0], PINCH[i][0], pinch), lerp(p[1], PINCH[i][1], pinch)]; });
    var pcx = 0, pcy = 0; [0, 5, 9, 13, 17].forEach(function (i) { pcx += posed[i][0]; pcy += posed[i][1]; }); pcx /= 5; pcy /= 5;
    var lm = [];
    posed.forEach(function (p) { var dx = (p[0] - pcx) * unit, dy = (p[1] - pcy) * unit; lm.push(+(x + (dx * ca - dy * sa) / SW).toFixed(4), +(y + (dx * sa + dy * ca) / SH).toFixed(4)); });
    return { h: 'R', p: pinch > 0.6, f: false, lm: lm };
  }
  function slot(o) {
    var s = { enabled: true, name: '', src: 'height', hand: 'ANY', eng: 'pinch', mode: 'grab', rel: 'stay', curve: 'lin',
              smooth: 0.5, reach: 0.6, lo: 0, hi: 1, home: 0, cc: 74, ch: 1, target: '', targetOf: '', ref: '',
              pos: 0, out: 0, lmsg: '', held: false, active: false, returning: false, driver: '', grabFrom: -1, canUndo: false, allowed: true };
    for (var k in o) s[k] = o[k];
    return s;
  }
  function off(i) { return slot({ enabled: false, name: '', cc: 20 + i }); }
  var base = {
    licensed: true, live: true, preset: 0, offline: false, source: 1, yieldTo: '', hub: 'phone', canCamera: true, view: 'hand', linkError: '',
    phone: { state: 'connected', via: 'direct', fps: 30, rtt: 11, url: 'https://rone-control-lab.ronemusics.workers.dev/p/#k3Qm9xV2tR8wLp1Z.a7Hc2YnB5sDf0Ke4' },
    cam: { state: 'off', fps: 0, ms: 0, label: '', device: '', devices: [], tip: '', error: '', still: false },
    midiDevice: 'RONE Control', midiDeviceFailed: false, midiDeviceOwn: false, announce: -1, announceLeft: 0, midiOwnPossible: true, midiDeviceWanted: 'RONE Control',
    fl: { state: 'ready', host: 'fl', api: 45, port: 236, impl: 4, script: 4, midi: true },
    hands: [hand(0.56, 0.42, 1, -8, 1.05)], aspect: SW / SH,
    slots: [
      // FILTER RIDE, slot A held and linked in FL
      slot({ name: 'CUTOFF', src: 'height', eng: 'pinch', mode: 'grab', rel: 'stay', curve: 'soft', cc: 74, pos: 0.62, out: 0.62, held: true, active: true, driver: 'R', grabFrom: 0.25, canUndo: true, target: 'Filter 1 Freq', targetOf: 'Serum 2' }),
      slot({ name: 'PAN', src: 'side', eng: 'pinch', mode: 'grab', rel: 'stay', home: 0.5, cc: 10, pos: 0.5, out: 0.5, target: 'Pan', targetOf: 'Insert 3' }),
      slot({ name: 'RESO', src: 'twist', eng: 'pinch', mode: 'grab', rel: 'return', hi: 0.85, cc: 71, pos: 0.28, out: 0.28, home: 0.28 }),
      off(3), off(4), off(5), off(6), off(7)
    ]
  };
  function clone(o) { return JSON.parse(JSON.stringify(o)); }
  window.__scn_control = function (kind) {
    var st = clone(base);
    if (kind === 'phone') {            // the first thing the plugin shows: the QR
      st.phone.state = 'waiting'; st.phone.fps = 0; st.hands = null; st.slots[0].held = false; st.slots[0].active = false; st.slots[0].driver = '';
    } else if (kind === 'camera') {    // the computer's camera is the eye (Windows)
      st.hub = 'camera'; st.phone.state = 'off';
      st.cam = { state: 'running', fps: 30, ms: 12, label: 'Logitech C920', device: 'c920', devices: [{ id: 'c920', label: 'Logitech C920', virtual: false }], tip: '', error: '', still: false };
      st.hands = [hand(0.5, 0.5, 0, 0, 1.0)]; st.slots[0].held = false; st.slots[0].active = false; st.slots[0].driver = '';
    } else if (kind === 'idle') {      // nothing held: the slots at rest
      st.hands = [hand(0.5, 0.5, 0, 0, 1.0)]; st.slots[0].held = false; st.slots[0].active = false; st.slots[0].driver = '';
    }
    window.__mockEmit('uiConfig', { version: '1.0.0', host: kind === 'link' ? 'FL STUDIO' : '' });
    window.__mockEmit('ctlState', st);
    setTimeout(function () { window.__mockEmit('ctlState', clone(st)); }, 60);
    if (kind === 'link') window.__ctl.openLink(0);
    return kind;
  };
})();
