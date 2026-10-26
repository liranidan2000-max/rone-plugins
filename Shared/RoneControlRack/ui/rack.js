/* RONE Control Rack - the rack page (production). The C++ Engine owns the slots, the mapper and the hands;
   this page shows them and sends the user's choices back. MOUSE = the mouse is the hand (RONE Control's
   SIM: hold = pinch, right button = fist, wheel = twist, shift+wheel = near / far), computed here and sent
   as a "hands" frame, exactly as the phone page and the link page send theirs.
     C++ -> page  rackState { open, armed, live, lite, name, accent, source, phone, cam, rec, hands, aspect, thumb, slots (each: locked), params }
     page -> C++  slotConfig / slotOn / slotParam / addSlot / removeSlot / reset / setLive / arm / setSource / hands /
                  newCode / camera / rec / rackReady / collapse */
(function () {
  'use strict';
  var clamp = function (v, a, b) { a = a === undefined ? 0 : a; b = b === undefined ? 1 : b; return v < a ? a : v > b ? b : v; };
  var lerp = function (a, b, q) { return a + (b - a) * q; };
  var $ = function (s) { return document.querySelector(s); };
  var SW = 248, SH = 160, LETTERS = 'ABCDEFGH';
  var MOVE_WORDS = { height: 'LIFT', side: 'SLIDE', depth: 'PUSH / PULL', twist: 'TWIST', pinch: 'OPEN / CLOSE', grip: 'SQUEEZE', spread: 'SPREAD' };
  var HOLD_WORDS = { pinch: 'PINCH', fist: 'FIST', always: 'NO GRAB' };
  var S = null, sel = -1, source = 'phone', sig = '', app = $('#rackApp'), stage = $('#rStage'), cv = $('#rCanvas'), ctx = cv.getContext('2d');
  var thumbImg = new Image(), thumbSeq = -1, dpr = 0;

  // the QR (qrcode-generator rides in the plugin: no internet needed to draw it)
  var qrFor = '';
  function renderQr(url) {
    if (!url || url === qrFor) return;
    qrFor = url;
    try { var q = qrcode(0, 'M'); q.addData(url); q.make(); $('#rQrBox').innerHTML = q.createSvgTag({ cellSize: 3, margin: 0 }); }
    catch (_) { $('#rQrBox').textContent = 'No code yet'; }
  }
  tap(document.getElementById('rNewCode'), function (e) { e.stopPropagation(); qrFor = ''; $('#rQrBox').innerHTML = '&hellip;'; emit('newCode', {}); });
  function emit(name, payload) { if (window.__JUCE__ && window.__JUCE__.backend) window.__JUCE__.backend.emitEvent(name, payload || {}); }
  // Inside a DAW the FIRST click into this WebView after its window regains focus was seen to do nothing (DESIGN.md 2d):
  // the host activates the window between mouse-down and mouse-up and no 'click' is born. So every control acts on
  // pointerdown and ignores the click that may follow it; a click on its own (keyboard, the test tools' .click()) still
  // works. (In the standalone the whole click arrives either way - Phase 0 probe, 26.10.2026.)
  var tapAt = -1e9;   // one clock for all: a tap may re-render, and the click then lands on a fresh element
  function tap(el, fn) {
    el.addEventListener('pointerdown', function (e) { if (e.button !== 0) return; tapAt = e.timeStamp; fn(e); });
    el.addEventListener('click', function (e) { if (e.timeStamp - tapAt < 700) { e.stopPropagation(); return; } fn(e); });
  }
  // LITE (no ALL ACCESS here): slot A runs, B.. are shown locked; a tap on a locked card or on + KNOB shows the card below
  var upsellUntil = 0, centerAction = null;
  function upsell() { upsellUntil = performance.now() + 7000; paintTop(); }

  // ---- fit: 280 design px to the view's width, the height follows ----
  function fit() {
    var w = window.innerWidth, h = window.innerHeight, k = Math.max(0.3, w / 280);
    app.style.transform = 'scale(' + k.toFixed(4) + ')';
    app.style.height = Math.round(h / k) + 'px';
    var d = Math.min(3, (window.devicePixelRatio || 1) * k);
    if (d !== dpr) { dpr = d; cv.width = Math.round(SW * d); cv.height = Math.round(SH * d); }
  }
  window.addEventListener('resize', fit); fit();

  // ============================================================ MOUSE = the hand (RONE Control's SIM)
  var OPEN = [[0,0],[-.30,-.18],[-.55,-.42],[-.74,-.64],[-.90,-.84],[-.30,-.95],[-.36,-1.38],[-.40,-1.66],[-.43,-1.90],[-.04,-1.00],[-.05,-1.48],[-.06,-1.78],[-.07,-2.02],[.20,-.93],[.24,-1.37],[.27,-1.63],[.29,-1.85],[.40,-.80],[.50,-1.12],[.56,-1.33],[.61,-1.52]];
  var PINCH = [[0,0],[-.30,-.18],[-.52,-.45],[-.64,-.72],[-.60,-1.02],[-.30,-.95],[-.40,-1.32],[-.50,-1.30],[-.57,-1.08],[-.04,-1.00],[-.05,-1.45],[-.06,-1.70],[-.07,-1.90],[.20,-.93],[.24,-1.34],[.27,-1.58],[.29,-1.78],[.40,-.80],[.50,-1.10],[.56,-1.29],[.60,-1.46]];
  var FIST = [[0,0],[-.28,-.20],[-.45,-.45],[-.40,-.66],[-.22,-.74],[-.30,-.95],[-.34,-1.22],[-.30,-1.02],[-.28,-.86],[-.04,-1.00],[-.05,-1.28],[-.04,-1.06],[-.03,-.88],[.20,-.93],[.23,-1.18],[.21,-.98],[.19,-.82],[.40,-.80],[.46,-1.02],[.43,-.86],[.40,-.72]];
  function simLandmarks(h) {
    var unit = 78 * h.depth * (SH / 330), a = h.twist * Math.PI / 180, ca = Math.cos(a), sa = Math.sin(a), mir = h.mirror ? -1 : 1;
    var posed = OPEN.map(function (p, i) { var x = lerp(p[0], PINCH[i][0], h.pinch), y = lerp(p[1], PINCH[i][1], h.pinch); x = lerp(x, FIST[i][0], h.fist); y = lerp(y, FIST[i][1], h.fist); return [x, y]; });
    var pcx = 0, pcy = 0; [0, 5, 9, 13, 17].forEach(function (i) { pcx += posed[i][0]; pcy += posed[i][1]; }); pcx /= 5; pcy /= 5;
    return posed.map(function (p) { var dx = (p[0] - pcx) * unit * mir, dy = (p[1] - pcy) * unit; return { x: h.x + (dx * ca - dy * sa) / SW, y: h.y + (dx * sa + dy * ca) / SH }; });
  }
  function features(lm, aspect) {
    var P = function (i) { return [lm[i].x * aspect, lm[i].y]; }, d = function (p, q) { return Math.hypot(p[0] - q[0], p[1] - q[1]); };
    var w = P(0), p5 = P(5), p9 = P(9), p13 = P(13), p17 = P(17);
    var pc = [(w[0] + p5[0] + p9[0] + p13[0] + p17[0]) / 5, (w[1] + p5[1] + p9[1] + p13[1] + p17[1]) / 5];
    var size = Math.max(d(w, p9), d(p5, p17) / 0.72);
    var curl = [8, 12, 16, 20].reduce(function (s, i) { return s + d(P(i), pc); }, 0) / 4 / size;
    var pinchR = d(P(4), P(8)) / size;
    return { pcN: [pc[0] / aspect, pc[1]], height: 1 - pc[1], side: pc[0] / aspect, depth: clamp(0.5 + Math.log2(size / 0.16) * 0.85),
      twist: Math.atan2(p9[0] - w[0], -(p9[1] - w[1])) * 180 / Math.PI, pinchR: pinchR, curl: curl, pinch: clamp((pinchR - 0.1) / 0.75), grip: clamp((1.08 - curl) / 0.78) };
  }
  var sim = { x: 0.5, y: 0.55, pinch: 0, fist: 0, pT: 0, fT: 0, twist: 0, depth: 1, inStage: false, wasIn: false, side: 'R' };
  var tr = { pinched: false, fisted: false, twU: 0, twPrev: null, born: 0, vis: false };
  var seq = 0;
  function simPos(e) { var r = stage.getBoundingClientRect(); sim.x = clamp((e.clientX - r.left) / r.width); sim.y = clamp((e.clientY - r.top) / r.height); }
  stage.addEventListener('pointermove', function (e) { if (source !== 'sim') return; simPos(e); sim.inStage = true; });
  stage.addEventListener('pointerleave', function (e) { if (source !== 'sim') return; var cap = false; try { cap = stage.hasPointerCapture(e.pointerId); } catch (_) {} if (!cap) sim.inStage = false; });
  stage.addEventListener('pointerdown', function (e) { if (source !== 'sim') return; try { stage.setPointerCapture(e.pointerId); } catch (_) {} sim.inStage = true; simPos(e); if (e.button === 2) sim.fT = 1; else sim.pT = 1; e.preventDefault(); });
  var simUp = function (e) { sim.pT = 0; sim.fT = 0; try { stage.releasePointerCapture(e.pointerId); } catch (_) {} };
  stage.addEventListener('pointerup', simUp); stage.addEventListener('pointercancel', simUp);
  stage.addEventListener('wheel', function (e) { if (source !== 'sim') return; e.preventDefault(); var st = e.deltaY > 0 ? -1 : 1; if (e.shiftKey) sim.depth = clamp(sim.depth * (st > 0 ? 1.06 : 1 / 1.06), 0.55, 1.8); else sim.twist = clamp(sim.twist + st * 6, -150, 150); }, { passive: false });
  document.addEventListener('contextmenu', function (e) { e.preventDefault(); });
  function simTick(dt, t) {
    var k = 1 - Math.exp(-dt / 0.06);
    sim.pinch += (sim.pT - sim.pinch) * k; sim.fist += (sim.fT - sim.fist) * k;
    var out = [];
    if (sim.inStage) {
      var lm = simLandmarks({ x: sim.x, y: sim.y, pinch: sim.pinch, fist: sim.fist, twist: sim.twist, depth: sim.depth, mirror: sim.side === 'L' }), f = features(lm, SW / SH);
      if (!tr.vis) { tr.vis = true; tr.born = t; tr.twPrev = null; tr.pinched = false; tr.fisted = false; }
      if (tr.twPrev === null) tr.twU = f.twist; else { var dd = f.twist - tr.twPrev; if (dd > 180) dd -= 360; if (dd < -180) dd += 360; tr.twU += dd; }
      tr.twPrev = f.twist;
      if (!tr.pinched && f.pinchR < 0.30 && f.curl > 0.62) tr.pinched = true; else if (tr.pinched && (f.pinchR > 0.45 || f.curl < 0.5)) tr.pinched = false;
      if (!tr.fisted && f.curl < 0.48) tr.fisted = true; else if (tr.fisted && f.curl > 0.66) tr.fisted = false;
      if (t - tr.born < 0.12) tr.pinched = false;
      var flat = []; lm.forEach(function (p) { flat.push(+p.x.toFixed(4), +p.y.toFixed(4)); });
      out.push({ h: sim.side, p: tr.pinched, f: tr.fisted, height: f.height, sideX: f.side, depth: f.depth, twist: tr.twU, pinch: f.pinch, grip: f.grip, pcx: f.pcN[0], pcy: f.pcN[1], lm: flat });
      sim.wasIn = true;
    } else { tr.vis = false; if (!sim.wasIn) return; sim.wasIn = false; }
    emit('hands', { seq: ++seq, t: t, aspect: SW / SH, source: 'sim', hands: out });
  }

  // ============================================================ the stage
  function glove(g, P, u, hv, alpha) {
    var col = 'rgb(' + Math.round(lerp(168, 82, hv)) + ',' + Math.round(lerp(175, 255, hv)) + ',' + Math.round(lerp(186, 69, hv)) + ')';
    g.save(); g.globalAlpha = alpha; g.lineCap = 'round'; g.lineJoin = 'round'; g.fillStyle = col; g.strokeStyle = col;
    if (hv > 0.05) { g.shadowColor = 'rgba(82,255,69,' + (0.55 * hv).toFixed(2) + ')'; g.shadowBlur = 16; }
    g.lineWidth = 0.4 * u; g.beginPath(); [0, 1, 5, 9, 13, 17].forEach(function (i, k) { if (k) g.lineTo(P[i][0], P[i][1]); else g.moveTo(P[i][0], P[i][1]); }); g.closePath(); g.fill(); g.stroke();
    g.shadowBlur = 0;
    [[17, 18, 19, 20, 0.2], [13, 14, 15, 16, 0.225], [9, 10, 11, 12, 0.24], [5, 6, 7, 8, 0.235], [1, 2, 3, 4, 0.27]].forEach(function (f) {
      var a = f[0], b = f[1], c2 = f[2], e = f[3], w = f[4];
      g.strokeStyle = '#101216'; g.lineWidth = w * u + 3.5;
      g.beginPath(); g.moveTo(lerp(P[a][0], P[b][0], 0.5), lerp(P[a][1], P[b][1], 0.5)); g.lineTo(P[b][0], P[b][1]); g.lineTo(P[c2][0], P[c2][1]); g.lineTo(P[e][0], P[e][1]); g.stroke();
      g.strokeStyle = col; g.lineWidth = w * u;
      g.beginPath(); g.moveTo(P[a][0], P[a][1]); g.lineTo(P[b][0], P[b][1]); g.lineTo(P[c2][0], P[c2][1]); g.lineTo(P[e][0], P[e][1]); g.stroke();
    });
    g.restore();
  }
  function drawStage() {
    var g = ctx, d = dpr || 1; g.setTransform(d, 0, 0, d, 0, 0); g.clearRect(0, 0, SW, SH);
    var showThumb = S && S.thumb && source !== 'sim' && thumbImg.complete && thumbImg.naturalWidth;
    if (showThumb) {
      var a = thumbImg.naturalWidth / thumbImg.naturalHeight, sa = SW / SH, dw, dh;
      if (a > sa) { dh = SH; dw = SH * a; } else { dw = SW; dh = SW / a; }
      g.drawImage(thumbImg, (SW - dw) / 2, (SH - dh) / 2, dw, dh);
      g.fillStyle = 'rgba(13,14,17,.3)'; g.fillRect(0, 0, SW, SH);
    } else {
      g.fillStyle = '#0D0E11'; g.fillRect(0, 0, SW, SH);
      g.strokeStyle = 'rgba(255,255,255,.035)'; g.lineWidth = 1;
      for (var x = 0; x <= SW; x += 31) { g.beginPath(); g.moveTo(x + 0.5, 0); g.lineTo(x + 0.5, SH); g.stroke(); }
      for (var y = 0; y <= SH; y += 31) { g.beginPath(); g.moveTo(0, y + 0.5); g.lineTo(SW, y + 0.5); g.stroke(); }
    }
    if (!S || !S.hands) return;
    var holders = { L: null, R: null };
    (S.slots || []).forEach(function (s) { if (s.on && s.held && s.driver) holders[s.driver === 'B' ? 'R' : s.driver] = s; });
    S.hands.forEach(function (h) {
      var hold = holders[h.h], hv = hold ? 1 : 0, P = [];
      for (var i = 0; i < 21; i++) P.push([h.lm[i * 2] * SW, h.lm[i * 2 + 1] * SH]);
      var u = Math.max(14, Math.hypot(P[9][0] - P[0][0], P[9][1] - P[0][1]) * 0.9);
      if (hold) {
        var c = hold.cfg; g.save(); g.setLineDash([3, 4]); g.strokeStyle = 'rgba(82,255,69,.35)'; g.lineWidth = 1.2;
        var cx = h.pcx * SW, cy = h.pcy * SH;
        if (c.src === 'height') { g.beginPath(); g.moveTo(cx, SH * 0.18); g.lineTo(cx, SH * 0.78); g.stroke(); }
        else if (c.src === 'side') { g.beginPath(); g.moveTo(SW * 0.2, cy); g.lineTo(SW * 0.8, cy); g.stroke(); }
        else if (c.src === 'twist') { g.beginPath(); g.arc(cx, cy, u * 1.9, -Math.PI * 0.72, -Math.PI * 0.28); g.stroke(); }
        g.restore();
      }
      glove(g, P, u, S.armed ? hv : 0, S.armed ? 1 : 0.45);
      if (h.p) { var px = (P[4][0] + P[8][0]) / 2, py = (P[4][1] + P[8][1]) / 2; g.save(); g.strokeStyle = '#fff'; g.lineWidth = 1.6; g.shadowColor = 'rgba(82,255,69,1)'; g.shadowBlur = 10; g.beginPath(); g.arc(px, py, u * 0.28, 0, 7); g.stroke(); g.restore(); }
      var lab = h.h === 'L' ? 'LEFT' : 'RIGHT'; if (hold) lab += ' · ' + hold.name;
      g.save(); g.font = '800 7.5px Manrope, sans-serif'; g.textAlign = 'center'; try { g.letterSpacing = '1.4px'; } catch (_) {}
      g.fillStyle = hold ? 'rgb(82,255,69)' : '#5B616B'; g.fillText(lab, clamp(h.pcx * SW, 40, SW - 40), clamp(h.pcy * SH + u * 1.9, 14, SH - 6)); g.restore();
    });
  }

  // ============================================================ words
  function gesture(c) {
    var move = MOVE_WORDS[c.src] || String(c.src).toUpperCase(), parts = [];
    if (c.src === 'pinch' || c.src === 'grip') parts.push(move);
    else { if (c.eng === 'always') parts.push('NO GRAB'); else parts.push(HOLD_WORDS[c.eng]); parts.push(move); }
    parts.push(c.rel === 'return' ? 'SPRINGS BACK' : 'STAYS');
    return parts.join('<i>·</i>');
  }
  function say(c) {
    if (c.src === 'pinch') return 'Open and close your thumb and finger: the gap between them is the value.';
    if (c.src === 'grip') return 'Close your hand: an open hand is the bottom, a fist is the top.' + (c.rel === 'return' ? ' Open it and it springs back.' : '');
    var mv = { height: 'move your hand up or down', side: 'move your hand left or right', depth: 'move your hand toward the camera or away', twist: 'turn your hand like a knob', spread: 'pull your hands apart or together' }[c.src];
    if (c.eng === 'always') return 'No grab needed: ' + (c.mode === 'follow' ? 'the knob follows as you ' + mv : 'just ' + mv) + '.' + (c.rel === 'return' ? ' Take your hand away and it springs back.' : '');
    var a = (c.eng === 'fist' ? 'Make a fist' : 'Pinch') + ', then ' + mv + '.';
    if (c.mode === 'follow') a += ' The knob jumps to where your hand is.';
    return a + (c.rel === 'return' ? ' Let go and it springs back.' : ' Let go and it stays.');
  }
  function seg(key, opts, cur) {
    return '<div class="r-seg" data-key="' + key + '">' + opts.map(function (o) { return '<span data-v="' + o[0] + '"' + (o[0] === cur ? ' class="on"' : '') + '>' + o[1] + '</span>'; }).join('') + '</div>';
  }
  function editorHTML(s) {
    var c = s.cfg, selHtml = '<select data-key="param">' + (S.params || []).map(function (p) { return '<option value="' + p.id + '"' + (p.id === s.param ? ' selected' : '') + '>' + p.name + (p.bool ? ' (SWITCH)' : '') + '</option>'; }).join('') + '</select>';
    return '<div class="r-edit">' +
      '<div class="r-erow"><label>KNOB</label>' + selHtml + '</div>' +
      '<div class="r-erow"><label>HAND</label>' + seg('hand', [['L', 'LEFT'], ['R', 'RIGHT'], ['ANY', 'ANY']], c.hand) + '</div>' +
      '<div class="r-erow"><label>MOVE</label>' + seg('src', [['height', 'UP / DOWN'], ['side', 'LEFT / RIGHT'], ['depth', 'NEAR / FAR'], ['twist', 'TWIST'], ['pinch', 'PINCH'], ['grip', 'FIST']], c.src) + '</div>' +
      (c.src === 'pinch' || c.src === 'grip' ? '' : '<div class="r-erow"><label>HOLD WITH</label>' + seg('eng', [['pinch', 'PINCH'], ['fist', 'FIST'], ['always', 'NO GRAB']], c.eng) + '</div>') +
      '<div class="r-erow"><label>LET GO</label>' + seg('rel', [['stay', 'STAYS'], ['return', 'SPRINGS BACK']], c.rel) + '</div>' +
      '<div class="r-say">' + say(c) + '</div>' +
      '<button class="r-remove">REMOVE THIS KNOB</button></div>';
  }

  // ============================================================ the slots
  var slotsEl = $('#rSlots');
  function renderSlots() {
    var slots = S.slots || [];
    var now = slots.map(function (s, i) { return [s.param, s.on, s.cfg.src, s.cfg.hand, s.cfg.eng, s.cfg.mode, s.cfg.rel, i === sel, s.why, s.locked].join(); }).join('|') + (S.params || []).length + (S.lite ? 'L' : '');
    if (now === sig) return;
    sig = now; slotsEl.innerHTML = '';
    slots.forEach(function (s, i) {
      var c = s.cfg, card = document.createElement('div');
      if (s.locked && i === sel) sel = -1;
      card.className = 'r-card' + (s.on ? '' : ' off') + (i === sel ? ' sel' : '') + (s.locked ? ' locked' : ''); card.dataset.i = i;
      card.innerHTML = '<div class="r-row1"><s>' + LETTERS[i] + '</s><span class="r-name"></span><em class="r-hand">' + (c.hand === 'ANY' ? 'L / R' : c.hand) + '</em>' +
        (s.locked ? '<em class="r-lock">ALL ACCESS</em>' : '<button class="r-on' + (s.on ? ' on' : '') + '">' + (s.on ? 'ON' : 'OFF') + '</button>') + '</div>' +
        '<div class="r-ges">' + gesture(c) + '</div>' +
        '<div class="r-val"><div class="r-bar"><i></i></div><b></b><em>HELD</em></div>' +
        (s.why && i !== sel ? '<div class="r-why"></div>' : '') + (i === sel ? editorHTML(s) : '');
      card.querySelector('.r-name').textContent = s.name;
      if (s.why && i !== sel) card.querySelector('.r-why').textContent = s.why;
      if (s.locked) { tap(card, function () { upsell(); }); slotsEl.appendChild(card); return; }
      tap(card, function (e) { if (e.target.closest('.r-edit') || e.target.closest('.r-on')) return; sel = sel === i ? -1 : i; sig = ''; renderSlots(); });
      tap(card.querySelector('.r-on'), function (e) { e.stopPropagation(); emit('slotOn', { i: i, on: !s.on }); });
      var ed = card.querySelector('.r-edit');
      if (ed) {
        tap(ed, function (e) {
          var sp = e.target.closest('.r-seg span'); if (!sp) return;
          var key = sp.closest('.r-seg').dataset.key, v = sp.dataset.v, cfg = {}; cfg[key] = v;
          if (key === 'src' && (v === 'pinch' || v === 'grip')) { cfg.eng = 'always'; cfg.mode = 'follow'; }
          else if (key === 'src' && c.eng === 'always' && c.mode === 'follow') cfg.mode = 'grab';
          emit('slotConfig', { i: i, cfg: cfg });
        });
        ed.querySelector('select').addEventListener('change', function (e) { emit('slotParam', { i: i, param: e.target.value }); });
        tap(ed.querySelector('.r-remove'), function () { sel = -1; emit('removeSlot', { i: i }); });
      }
      slotsEl.appendChild(card);
    });
    var add = $('#rAdd');
    add.disabled = slots.length >= (S.maxSlots || 4);
    add.textContent = S.lite ? '+ KNOB · ALL ACCESS' : '+ KNOB';
    add.classList.toggle('lite', !!S.lite);
  }
  function paintValues() {
    var cards = slotsEl.children, slots = S.slots || [];
    for (var i = 0; i < cards.length && i < slots.length; i++) {
      var s = slots[i], card = cards[i];
      var b = card.querySelector('.r-val b'); if (b.textContent !== s.text) b.textContent = s.text;
      var bar = card.querySelector('.r-bar i'), z = s.zero || 0;
      if (z > 0) { bar.style.left = (Math.min(z, s.norm) * 100) + '%'; bar.style.width = (Math.abs(s.norm - z) * 100) + '%'; }
      else { bar.style.left = '0'; bar.style.width = (s.norm * 100) + '%'; }
      var held = !!(s.held && (s.cfg.eng !== 'always' || (s.bool ? s.norm > 0.5 : true)));
      if (card.classList.contains('held') !== held) card.classList.toggle('held', held);
    }
  }

  // ============================================================ header, status, source
  function paintTop() {
    document.documentElement.style.setProperty('--acc', S.accent || '#D8E4EC');
    var m = /^#?([0-9a-f]{2})([0-9a-f]{2})([0-9a-f]{2})$/i.exec(S.accent || '');
    if (m) document.documentElement.style.setProperty('--acc-rgb', parseInt(m[1], 16) + ',' + parseInt(m[2], 16) + ',' + parseInt(m[3], 16));
    var seen = !!(S.hands && S.hands.length > 0), lb = $('#rLive');
    lb.classList.toggle('active', !!S.live);
    // a hand in the picture and LIVE off = "I grab and nothing happens" (Liran, FL 26.10): the button itself asks for the tap
    lb.classList.toggle('ask', !S.live && seen); lb.parentNode.classList.toggle('ask', !S.live && seen);   // the header makes room: the word CONTROL steps aside
    if (lb.textContent !== (!S.live && seen ? 'TAP LIVE' : 'LIVE')) lb.textContent = !S.live && seen ? 'TAP LIVE' : 'LIVE';
    var rc = S.rec || {}, rb = $('#rRec');
    rb.classList.toggle('on', !!rc.on); rb.classList.toggle('cant', !rc.can);
    rb.title = rc.can ? (rc.on ? 'Stop recording in ' + (rc.host || 'the DAW') : 'Record in ' + (rc.host || 'the DAW') + ' and play') : (rc.why || '');
    $('#rArmed').classList.toggle('on', !!S.armed);
    $('#rArmed span').textContent = S.armed ? 'ARMED' : 'ARMED ELSEWHERE · CLICK TO TAKE';
    source = S.source || 'phone';
    $('#rSrc').querySelectorAll('span').forEach(function (x) { x.classList.toggle('on', x.dataset.v === source); });
    $('#rSrcCam').classList.toggle('off', !S.canCamera);
    $('#rSide').hidden = source !== 'sim';
    var st = $('#rStatus'), txt = '', cls = '', center = $('#rCenter'), cb = $('#rCenterB'), cs = $('#rCenterS'), btn = $('#rCenterBtn'), showCenter = false, showQr = false;
    if (source === 'sim') { txt = 'MOUSE = ' + (sim.side === 'L' ? 'LEFT' : 'RIGHT') + ' HAND'; $('#rHint').textContent = 'MOVE THE MOUSE HERE · HOLD = PINCH · RIGHT = FIST · WHEEL = TWIST'; }
    else if (source === 'camera') {
      var c = S.cam || {}; $('#rHint').textContent = (c.tip || 'light on your hand · 50-80 cm from the camera').toUpperCase();
      if (S.peer) { txt = 'HANDS FROM RONE CONTROL IN ANOTHER PROGRAM'; cls = 'on'; }
      // the model lives once: this plugin's installer put it in the shared Hands folder; without it the camera cannot start here (the phone can)
      else if (S.linkError && /no hand model/i.test(S.linkError)) { txt = 'HAND MODEL MISSING'; cls = 'warn'; showCenter = true; cb.textContent = 'THE CAMERA NEEDS THE HAND MODEL'; cs.textContent = 'Reinstall ' + (S.name || 'this plugin') + ' from the RONE Plugins Center - its installer brings the model. Or use the phone: it needs none.'; btn.hidden = false; btn.textContent = 'OPEN THE CENTER'; centerAction = function () { emit('openCenter', { what: 'self' }); }; }
      else if (c.state === 'on') { txt = 'CAMERA · ' + Math.round(c.fps || 0) + ' FPS' + (c.label ? ' · ' + c.label.toUpperCase().slice(0, 18) : ''); cls = 'on'; }
      else if (c.state === 'starting' || c.state === 'model' || c.state === 'loading') txt = 'STARTING THE CAMERA…';
      else if (c.state === 'error' || c.state === 'none') { txt = 'NO CAMERA'; cls = 'warn'; showCenter = true; cb.textContent = 'NO CAMERA'; cs.textContent = c.error || 'This computer has no camera of its own. Use the phone instead.'; btn.hidden = false; btn.textContent = 'USE THE PHONE'; centerAction = function () { emit('setSource', { v: 'phone' }); }; }
      else txt = S.linkError ? 'LINK: ' + S.linkError.toUpperCase().slice(0, 40) : 'CAMERA OFF';
      if (c.still) { txt = 'NO PICTURE · ' + txt; cls = 'warn'; }
    } else {
      var p = S.phone || {}; $('#rHint').textContent = 'SAME WI-FI · TAP START ON THE PHONE';
      if (S.peer) { txt = 'HANDS FROM RONE CONTROL IN ANOTHER PROGRAM'; cls = 'on'; }
      else if (p.state === 'connected') { txt = 'PHONE · ' + Math.round(p.fps || 0) + ' FPS' + (p.via === 'relay' ? ' · RELAY' : ''); cls = 'on'; }
      else if (p.state === 'waiting' || p.state === 'connecting') { txt = p.state === 'connecting' ? 'PHONE CONNECTING…' : 'WAITING FOR THE PHONE…'; }
      else { txt = S.linkError ? 'LINK: ' + S.linkError.toUpperCase().slice(0, 40) : 'PHONE OFF'; }
      // the QR: shown until the phone is in (a phone paired in RONE Control comes straight here - same code)
      if (p.state !== 'connected' && p.url) { showQr = true; renderQr(p.url); $('#rNewCode').hidden = !!S.peer; $('#rQrS').textContent = S.peer ? 'RONE Control in another program holds the link - the same code works here.' : (p.state === 'connecting' ? 'The phone is connecting…' : 'Its camera becomes your hands. One scan - it stays paired.'); }
    }
    if (!S.live) { txt += ' · NOT LIVE'; cls = 'warn'; }
    if (!S.armed) { txt += ' · ARMED ELSEWHERE'; cls = cls || 'warn'; }
    if (st.textContent !== txt) st.textContent = txt;
    st.className = 'r-tl ' + cls;
    var hands = seen;
    if (S.lite && performance.now() < upsellUntil) {      // the LITE card, over everything, for a few seconds after a tap on a lock
      showCenter = true; hands = false; cb.textContent = 'ONE KNOB IS FREE'; cs.textContent = 'Slot A is yours in every RONE plugin. RONE ALL ACCESS opens every slot of this rack - and of every other RONE rack.';
      btn.hidden = false; btn.textContent = 'SEE ALL ACCESS'; centerAction = function () { emit('openCenter', { what: 'allaccess' }); };
    }
    center.hidden = !showCenter || hands;
    $('#rQr').hidden = !showQr || hands || showCenter;
  }
  tap($('#rSrc'), function (e) { var s = e.target.closest('span'); if (s && !s.classList.contains('off')) emit('setSource', { v: s.dataset.v }); });
  tap($('#rSide'), function (e) { var s = e.target.closest('span'); if (!s) return; sim.side = s.dataset.v; $('#rSide').querySelectorAll('span').forEach(function (x) { x.classList.toggle('on', x === s); }); });
  tap($('#rLive'), function () { emit('setLive', { on: !(S && S.live) }); });
  var whyT = 0;
  tap($('#rRec'), function () {
    var rc = (S && S.rec) || {};
    if (rc.can) { emit('rec'); return; }
    var w = $('#rWhy'); w.textContent = (rc.why || 'NO TRANSPORT').toUpperCase(); w.hidden = false;
    clearTimeout(whyT); whyT = setTimeout(function () { w.hidden = true; }, 2600);
  });
  tap($('#rArmed'), function () { emit('arm'); });
  tap($('#rAdd'), function () { if (S && S.lite) { upsell(); return; } sel = (S.slots || []).length; emit('addSlot'); });
  tap($('#rReset'), function () { sel = -1; emit('reset'); });
  tap($('#rClose'), function () { emit('collapse'); });
  tap($('#rCenterBtn'), function (e) { e.stopPropagation(); if (centerAction) centerAction(); });
  tap(stage, function (e) { if (source !== 'sim' && !e.target.closest('.o-go') && upsellUntil) { upsellUntil = 0; paintTop(); } });

  // ============================================================ loop + bridge
  var lastT = 0;
  function frame(now) {
    requestAnimationFrame(frame);
    var t = now / 1000, dt = lastT ? clamp(t - lastT, 0.001, 0.05) : 0.016; lastT = t;
    if (source === 'sim') simTick(dt, t);
    if (S) drawStage();
  }
  function whenBridgeReady(fn) {
    if (window.__JUCE__ && window.__JUCE__.backend) { fn(); return; }
    var iv = setInterval(function () { if (window.__JUCE__ && window.__JUCE__.backend) { clearInterval(iv); fn(); } }, 50);
  }
  whenBridgeReady(function () {
    window.__JUCE__.backend.addEventListener('rackState', function (st) {
      if (!st || !st.slots) return;
      S = st;
      if (st.thumb && st.thumbSeq !== thumbSeq) { thumbSeq = st.thumbSeq; thumbImg.src = st.thumb; }
      paintTop(); renderSlots(); paintValues();
    });
    emit('rackReady', {});
  });
  requestAnimationFrame(frame);
})();
