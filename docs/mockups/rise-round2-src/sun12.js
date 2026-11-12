// ─────────────────────────────────────────────────────────────────
// THE SUN (1.2.0) - one character for every mode. It fills the whole body, its light spills over the knob side.
//   LIFTOFF  pink, happy             ORBIT     violet, wide-eyed, a ring and two moons
//   PULSAR   gold, wired, strobing   UNDERTOW  blue, holds its breath and goes under
//   MELTDOWN red, angry, melting
// What moves it is what the engine reports in `frame`: the gate chops the light, the delay throws rings,
// the reverb widens the halo, the noise blows spray, the riser sends chevrons up the sky, DIVE raises
// the sea over it. On the drop it lands with a bounce and puts its shades on; in a GAP it holds its breath.
// ─────────────────────────────────────────────────────────────────
const CH = { target: 0, cur: 0, vel: 0, dropT: -1, blink: 0, nextBlink: 2.5, burst: 0, lastT: 0, pulse: 0, barPhase: 0, beatPos: 0,
  cool: 0, boing: 9, swap: 0, mode: 0, idle: 0, sleep: 0, px: 0, py: 0, pAt: -1e9, ringAcc: 0, shootT: -1, nextShoot: 7, shootX: 0, shootY: 0,
  drop() { this.dropT = 0; this.burst = 1; this.cool = 1.7; this.boing = 0; } };
const MODES = [
  { col: [255, 95, 184],  warm: [255, 150, 120], b0: [255, 243, 250], b1: [255, 166, 220], b2: [255, 95, 184],  b3: [255, 47, 160], ink: [58, 11, 38], lid: [176, 40, 122] },
  { col: [170, 110, 255], warm: [255, 120, 214], b0: [248, 242, 255], b1: [214, 180, 255], b2: [163, 107, 255], b3: [122, 61, 240], ink: [31, 11, 61], lid: [96, 48, 190] },
  { col: [255, 196, 56],  warm: [255, 132, 72],  b0: [255, 252, 232], b1: [255, 228, 128], b2: [255, 194, 31],  b3: [255, 150, 0],  ink: [61, 34, 0],  lid: [204, 124, 0] },
  { col: [64, 168, 255],  warm: [72, 228, 214],  b0: [239, 250, 255], b1: [160, 222, 255], b2: [63, 168, 255],  b3: [31, 120, 230], ink: [6, 32, 61],  lid: [27, 95, 176] },
  { col: [255, 76, 48],   warm: [255, 164, 40],  b0: [255, 241, 224], b1: [255, 174, 107], b2: [255, 74, 46],   b3: [216, 30, 18],  ink: [61, 7, 3],   lid: [160, 18, 8] },
];
// the colours on screen glide from one mode's to the next; MW is how much of each mode's own props is showing
const PKEYS = ['col', 'warm', 'b0', 'b1', 'b2', 'b3', 'ink', 'lid'], PAL = {}, MW = [1, 0, 0, 0, 0];
PKEYS.forEach(k => { PAL[k] = MODES[0][k].slice(); });
const rgb = c => `rgb(${c[0] | 0},${c[1] | 0},${c[2] | 0})`, rgba = (c, a) => `rgba(${c[0] | 0},${c[1] | 0},${c[2] | 0},${(a < 0 ? 0 : a > 1 ? 1 : a).toFixed(3)})`;
const cv = $('#cv'), g = cv.getContext('2d'); let W = 860, H = 526, DPR = 1;
function sizeCanvas() { const sc = $('#scene'); W = sc.clientWidth || 860; H = sc.clientHeight || 526; DPR = Math.min(2.5, (window.devicePixelRatio || 1) * appScale()); cv.width = Math.round(W * DPR); cv.height = Math.round(H * DPR); }
function rnd(seed) { const x = Math.sin(seed * 9999.1) * 10000; return x - Math.floor(x); }
function rr(x, y, w, h, r) { g.beginPath(); g.moveTo(x + r, y); g.arcTo(x + w, y, x + w, y + h, r); g.arcTo(x + w, y + h, x, y + h, r); g.arcTo(x, y + h, x, y, r); g.arcTo(x, y, x + w, y, r); g.closePath(); }
const backOut = x => 1 + 2.70158 * Math.pow(x - 1, 3) + 1.70158 * Math.pow(x - 1, 2);
// its eyes follow the pointer
document.addEventListener('mousemove', e => { const b = cv.getBoundingClientRect(); if (!b.width) return; CH.px = (e.clientX - b.left) / b.width * W; CH.py = (e.clientY - b.top) / b.height * H; CH.pAt = performance.now(); });

// ---- the face: one pose (numbers), eased every frame, so every change of mood is a move and not a cut ----
const POSE = { open: .78, lid: 1, slant: 0, happy: 0, xe: 0, squint: 0, bIn: 8, bOut: 30, pr: 16, spark: 0, fire: 0, half: 54, curve: -42, mopen: 0, teeth: 0, oo: 0, cheek: 0, puff: 0, shades: 0, mask: 0, vein: 0, lx: 0, ly: 0 };
function poseFor(m, u, x) {
  const p = { open: lerp(.78, 1.02, u), lid: 1 - sstep(0, .3, u), slant: 0, happy: u >= .86 ? 1 : 0, xe: 0, squint: 0, bIn: lerp(8, -2, u), bOut: lerp(30, -12, u), pr: lerp(16, 20, u), spark: 0, fire: 0,
    half: lerp(54, 150, u), curve: u < .3 ? lerp(-42, 0, u / .3) : lerp(0, 74, sstep(.3, .75, u)), mopen: sstep(.55, 1, u) * 66, teeth: 0, oo: 0, cheek: sstep(.35, .9, u) * .25, puff: 0, shades: 0, mask: x.dive > .06 ? 1 : 0, vein: 0, lx: 0, ly: 0 };
  if (m === 1) {          // ORBIT: wide-eyed wonder, big shiny pupils, an "o" at the top
    p.happy = 0; p.pr = lerp(18, 28, u); p.spark = sstep(.15, .8, u); p.open = lerp(.8, 1.08, u); p.bIn = lerp(6, -16, u); p.bOut = lerp(26, -18, u);
    p.oo = sstep(.62, .92, u); p.half = lerp(54, 104, u); p.mopen = 0; p.curve = u < .3 ? lerp(-42, 0, u / .3) : lerp(0, 52, sstep(.3, .62, u));
  } else if (m === 2) {   // PULSAR: wired - small pupils, the grin opens early, the eyes squeeze shut at the top
    p.happy = 0; p.xe = u >= .86 ? 1 : 0; p.pr = lerp(15, 11, u); p.open = lerp(.8, 1.14, u); p.mopen = sstep(.28, .8, u) * 80; p.half = lerp(54, 158, u); p.bIn = lerp(8, -18, u); p.bOut = lerp(30, -22, u);
  } else if (m === 3) {   // UNDERTOW: calm, then it holds its breath
    p.happy = 0; p.puff = sstep(.45, .9, u); p.mopen = 0; p.half = lerp(54, 58, u); p.curve = u < .3 ? lerp(-42, 0, u / .3) : lerp(0, 22, sstep(.3, .75, u)); p.open = lerp(.78, 1.12, u); p.cheek = 0;
  } else if (m === 4) {   // MELTDOWN: annoyed -> furious -> roaring
    const a = sstep(.06, .7, u);
    p.happy = 0; p.bIn = lerp(8, 50, a); p.bOut = lerp(30, -34, a); p.lid = Math.max(p.lid * .7, a * .5); p.slant = a * .42; p.squint = a * .4; p.pr = lerp(16, 12, a); p.fire = sstep(.4, 1, u);
    p.curve = lerp(-42, -66, a); p.half = lerp(54, 132, u); p.teeth = sstep(.3, .52, u) * (1 - sstep(.84, .9, u)); p.mopen = sstep(.84, .95, u) * 88; p.cheek = 0; p.vein = sstep(.6, .78, u);
  }
  if (x.armed) { p.bIn = -16; p.bOut = -16; p.lid = 0; }                                                   // FALL is waiting for the bar line: wide awake
  if (x.hold) { p.puff = 1; p.happy = 0; p.xe = 0; p.open = 1.16; p.lid = 0; p.squint = 0; p.pr = 11; p.mopen = 0; p.teeth = 0; p.oo = 0; p.half = 38; p.curve = 0; p.bIn = -20; p.bOut = -20; }   // the GAP: not a breath
  if (x.cool) { p.shades = 1; p.happy = 0; p.xe = 0; p.mopen = 0; p.teeth = 0; p.oo = 0; p.puff = 0; p.half = 98; p.curve = 44; p.vein = 0; p.cheek = .22; p.bIn = -6; p.bOut = -14; }          // the drop landed
  if (x.sleep > .5) { p.open = .05; p.lid = 1; p.curve = 6; p.half = 40; }
  return p;
}
function face(cx, cy, k, u, blink, C) {
  const q = POSE, ink = C.ink, eyeDX = lerp(120, 136, u), eyeR = lerp(48, 58, u), ey = -40, tr = Math.max(q.happy, q.xe);
  g.save(); g.translate(cx, cy); g.scale(k, k); g.lineCap = 'round'; g.lineJoin = 'round';
  if (q.puff > .02) [-1, 1].forEach(s => { const x = s * (eyeDX + 58), y = ey + 98, R = 30 + 44 * q.puff;        // cheeks full of air
    g.fillStyle = C.b1; g.globalAlpha = .5 * q.puff; g.beginPath(); g.arc(x, y, R, 0, TAU); g.fill();
    g.globalAlpha = .8 * q.puff; g.strokeStyle = ink; g.lineWidth = 7; g.beginPath(); g.arc(x, y, R, s > 0 ? -.5 : Math.PI - .9, s > 0 ? .9 : Math.PI + .5); g.stroke(); g.globalAlpha = 1; });
  if (q.shades < .6) [-1, 1].forEach(s => { const x = s * eyeDX;
    if (tr > .5) { const a = (tr - .5) * 2; g.strokeStyle = ink; g.lineWidth = 18;                               // laughing eyes: ^ ^, or > < when it is wired
      if (q.xe > q.happy) { g.beginPath(); g.moveTo(x + s * eyeR * .7, ey - eyeR * .6 * a); g.lineTo(x - s * eyeR * .5, ey + 4); g.lineTo(x + s * eyeR * .7, ey + 8 + eyeR * .6 * a); g.stroke(); }
      else { g.beginPath(); g.arc(x, ey + 8, eyeR * .95 * (.6 + .4 * a), Math.PI * 1.15, Math.PI * 1.85); g.stroke(); }
      return; }
    const open = Math.max(.06, q.open * (1 - blink) * (1 - tr * 2)), ry = eyeR * open;
    g.fillStyle = '#fff'; g.beginPath(); g.ellipse(x, ey, eyeR, ry, 0, 0, TAU); g.fill();
    if (open > .2) { g.save(); g.clip();
      const pr = q.pr, px = x + q.lx * (eyeR - pr) * .75, py = ey + lerp(10, -4, u) + q.ly * (eyeR - pr) * .55;
      if (q.fire > .02) { g.fillStyle = `rgba(255,70,20,${(.9 * q.fire).toFixed(3)})`; g.beginPath(); g.arc(px, py, pr * 1.6, 0, TAU); g.fill(); }
      g.fillStyle = ink; g.beginPath(); g.arc(px, py, pr, 0, TAU); g.fill();
      g.fillStyle = '#fff'; g.beginPath(); g.arc(px - pr * .35, py - pr * .38, pr * (.34 + .14 * q.spark), 0, TAU); g.fill();
      g.beginPath(); g.arc(px + pr * .32, py + pr * .3, pr * (.14 + .14 * q.spark), 0, TAU); g.fill();
      g.fillStyle = C.lid;                                                                                        // the lids: heavy when sleepy, slanted when angry, the lower one rises in a squint
      if (q.lid > .02) { g.beginPath(); g.ellipse(x, ey - eyeR * lerp(1.45, .55, q.lid), eyeR * 1.3, eyeR * .75, -s * q.slant, 0, TAU); g.fill(); }
      if (q.squint > .02) { g.beginPath(); g.ellipse(x, ey + eyeR * lerp(1.75, 1.0, q.squint), eyeR * 1.3, eyeR * .75, 0, 0, TAU); g.fill(); }
      g.restore(); }
  });
  const browY = ey - lerp(70, 92, u); g.strokeStyle = ink; g.lineWidth = 18; g.globalAlpha = 1 - q.mask;
  if (q.mask < .98) [-1, 1].forEach(s => { g.beginPath(); g.moveTo(s * (eyeDX - eyeR - 6), browY + q.bIn); g.quadraticCurveTo(s * eyeDX, browY - 12 + (q.bIn + q.bOut) * .25, s * (eyeDX + eyeR * .5), browY + q.bOut); g.stroke(); });
  g.globalAlpha = 1;
  if (q.cheek > .01) { g.fillStyle = `rgba(255,255,255,${q.cheek.toFixed(3)})`; [-1, 1].forEach(s => { g.beginPath(); g.ellipse(s * (eyeDX + 62), ey + 52, 30, 18, 0, 0, TAU); g.fill(); }); }
  if (q.vein > .02) { g.save(); g.translate(eyeDX + 74, ey - 132); g.scale(q.vein * (1 + .12 * CH.pulse), q.vein * (1 + .12 * CH.pulse)); g.rotate(.3); g.strokeStyle = ink; g.lineWidth = 11;   // the anger mark
    for (let i = 0; i < 4; i++) { g.rotate(Math.PI / 2); g.beginPath(); g.arc(31, 31, 20, Math.PI, Math.PI * 1.5); g.stroke(); } g.restore(); }
  // the mouth
  const my = 100, half = q.half, curve = q.curve, open = q.mopen;
  g.lineWidth = 14; g.strokeStyle = ink;
  if (q.oo > .5) { const ro = lerp(22, 46, (q.oo - .5) * 2); g.fillStyle = C.mouth; g.beginPath(); g.ellipse(0, my + 16, ro * .82, ro, 0, 0, TAU); g.fill(); g.stroke(); }
  else if (q.teeth > .25 && open < 45) {                                                                          // gritted teeth
    const hh = lerp(8, 30, q.teeth), w = half * .92, c0 = my + curve * 1.6;
    g.beginPath(); g.moveTo(-w, my); g.quadraticCurveTo(0, c0 - hh * 2, w, my); g.quadraticCurveTo(0, c0 + hh * 2, -w, my); g.closePath(); g.fillStyle = '#fff'; g.fill(); g.lineWidth = 12; g.stroke();
    g.save(); g.clip(); g.lineWidth = 7; g.beginPath(); g.moveTo(-w, my); g.quadraticCurveTo(0, c0, w, my); for (let i = -3; i <= 3; i++) { g.moveTo(i * w / 3.6, my - 120); g.lineTo(i * w / 3.6, my + 60); } g.stroke(); g.restore();
  }
  else if (open < 2) { g.beginPath(); g.moveTo(-half, my); g.quadraticCurveTo(0, my + curve * 1.6, half, my); g.stroke(); }
  else { const top = my + Math.min(0, curve) * .9, bot = my + Math.max(0, curve) * 1.6 + open, roar = curve < -8;
    g.fillStyle = C.mouth; g.beginPath(); g.moveTo(-half, my - 8); g.quadraticCurveTo(0, top, half, my - 8); g.quadraticCurveTo(0, bot, -half, my - 8); g.closePath(); g.fill(); g.stroke();
    g.save(); g.clip();
    g.fillStyle = '#ff4d8d'; g.beginPath(); if (roar) g.ellipse(0, my + open * .5, half * .5, open * .3, 0, 0, TAU); else g.ellipse(10, my + curve * 1.1 + open * .55, half * .55, open * .62, 0, 0, TAU); g.fill();
    g.strokeStyle = '#fff'; g.lineWidth = lerp(28, 52, u); g.beginPath(); g.moveTo(-half, my - 8); g.quadraticCurveTo(0, top, half, my - 8); g.stroke();   // the upper teeth follow the lip
    if (roar) { g.fillStyle = '#fff'; [-1, 1].forEach(s => { const x = s * half * .46, y = my - 8 + .385 * (top - my + 8); g.beginPath(); g.moveTo(x - 20, y); g.lineTo(x + 20, y); g.lineTo(x, y + 46); g.closePath(); g.fill(); }); }
    g.restore(); }
  // the diving mask and the snorkel: on whenever DIVE closes the filter, in any mode
  if (q.mask > .02) { const mw = eyeDX + eyeR + 30, mh = eyeR + 30; g.globalAlpha = q.mask; g.strokeStyle = ink;
    g.lineWidth = 24; g.beginPath(); g.moveTo(-mw, ey); g.lineTo(-322, ey - 26); g.moveTo(mw, ey); g.lineTo(322, ey - 26); g.stroke();
    rr(-mw, ey - mh, mw * 2, mh * 2 + 14, 58); g.fillStyle = 'rgba(200,244,255,.2)'; g.fill(); g.lineWidth = 16; g.stroke();
    g.strokeStyle = 'rgba(255,255,255,.55)'; g.lineWidth = 9; g.beginPath(); g.moveTo(-mw + 44, ey - mh + 52); g.lineTo(-mw + 86, ey - mh + 24); g.stroke();
    g.strokeStyle = ink; g.lineWidth = 26; g.beginPath(); g.moveTo(half + 6, my + 4); g.quadraticCurveTo(mw + 84, my + 34, mw + 78, ey + 10); g.lineTo(mw + 78, ey - 300); g.stroke();
    g.strokeStyle = C.warm; g.beginPath(); g.moveTo(mw + 78, ey - 300); g.lineTo(mw + 78, ey - 350); g.stroke(); g.globalAlpha = 1; }
  // the shades drop in from above when the drop lands
  if (q.shades > .02) { g.save(); g.translate(0, -260 * (1 - backOut(clamp(q.shades, 0, 1)))); g.globalAlpha = Math.min(1, q.shades * 3); g.fillStyle = '#0c0c12'; g.strokeStyle = '#0c0c12'; g.lineWidth = 16;
    g.beginPath(); g.moveTo(-eyeDX + 60, ey - 26); g.lineTo(eyeDX - 60, ey - 26); g.moveTo(-eyeDX - 70, ey - 22); g.lineTo(-326, ey - 46); g.moveTo(eyeDX + 70, ey - 22); g.lineTo(326, ey - 46); g.stroke();
    [-1, 1].forEach(s => { rr(s * eyeDX - 78, ey - 46, 156, 100, 34); g.fillStyle = '#0c0c12'; g.fill();
      g.save(); g.clip(); g.fillStyle = 'rgba(255,255,255,.26)'; g.beginPath(); g.moveTo(s * eyeDX - 62, ey + 62); g.lineTo(s * eyeDX - 10, ey - 60); g.lineTo(s * eyeDX + 24, ey - 60); g.lineTo(s * eyeDX - 28, ey + 62); g.closePath(); g.fill(); g.restore(); });
    g.restore(); }
  g.restore();
}
function glowCircle(x, y, r, c, a) { const gr = g.createRadialGradient(x, y, 0, x, y, r); gr.addColorStop(0, rgba(c, a)); gr.addColorStop(1, rgba(c, 0)); g.fillStyle = gr; g.fillRect(x - r, y - r, r * 2, r * 2); }
function stars(n, u, time, seed) { for (let i = 0; i < n; i++) { const x = rnd(i + seed) * W, y = rnd(i * 3 + seed) * H * .7, tw = .4 + .6 * Math.abs(Math.sin(time * (1 + rnd(i * 7)) + i)); g.fillStyle = `rgba(255,255,255,${(tw * u * .8).toFixed(3)})`; g.beginPath(); g.arc(x, y, .6 + rnd(i * 5) * 1.4, 0, TAU); g.fill(); } }
function confetti(P, dt, gravity) { for (const p of P) { p.x += p.vx * dt; p.y += p.vy * dt; p.vy += gravity * dt; p.r += p.vr * dt; p.life -= dt; } for (let i = P.length - 1; i >= 0; i--) if (P[i].life <= 0 || P[i].y > H + 20) P.splice(i, 1);
  for (const p of P) { g.save(); g.translate(p.x, p.y); g.rotate(p.r); g.globalAlpha = clamp(p.life, 0, 1); g.fillStyle = p.c; g.fillRect(-p.s / 2, -p.s * .3, p.s, p.s * .6); g.restore(); } }
function burst(P, x, y, n, spread, cols) { for (let i = 0; i < n; i++) { const a = -Math.PI / 2 + (Math.random() - .5) * spread, sp = 200 + Math.random() * 420; P.push({ x, y, vx: Math.cos(a) * sp, vy: Math.sin(a) * sp, vr: (Math.random() - .5) * 12, r: Math.random() * TAU, s: 5 + Math.random() * 7, c: cols[i % cols.length], life: 1.6 + Math.random() }); } }
// a ring of petals around the body: calm ones for the calm modes, flickering flames for MELTDOWN
function spikes(cx, cy, r0, n, len, wd, rot, fill, flick, time) {
  g.fillStyle = fill; g.beginPath();
  for (let i = 0; i < n; i++) { const a = rot + i * TAU / n, L = len * (i % 2 ? .62 : 1) * (flick ? .6 + .55 * Math.abs(Math.sin(time * 9 + i * 2.3)) : 1), a2 = a + (flick ? Math.sin(time * 5 + i) * .1 : 0), rt = r0 + L, rm = r0 + L * .45, wm = wd * .78;
    g.moveTo(cx + Math.cos(a - wd) * r0, cy + Math.sin(a - wd) * r0);
    g.quadraticCurveTo(cx + Math.cos(a - wm) * rm, cy + Math.sin(a - wm) * rm, cx + Math.cos(a2) * rt, cy + Math.sin(a2) * rt);
    g.quadraticCurveTo(cx + Math.cos(a + wm) * rm, cy + Math.sin(a + wm) * rm, cx + Math.cos(a + wd) * r0, cy + Math.sin(a + wd) * r0); g.closePath(); }
  g.fill();
}

const sun = { P: [], sparks: [], rings: [], dive: 0, dim: 1, clouds: [{ x: .12, y: .16, s: 1.25, v: 5 }, { x: .5, y: .08, s: .9, v: 3.5 }, { x: .8, y: .22, s: 1.05, v: 4.5 }],
  draw(u, dt, time, blink) {
    const col = PAL.col, warm = PAL.warm, m1 = MW[1], m2 = MW[2], m3 = MW[3], m4 = MW[4], hz = H * 0.64;
    // what the chain is doing right now, 0..1 each
    const dv = frame.lpf < 19000 ? clamp(Math.log(20000 / frame.lpf) / Math.log(40), 0, 1) : 0, rv = clamp(frame.rev, 0, 1), dl = clamp(frame.del, 0, 1),
      nz = frame.noise > -80 ? clamp((frame.noise + 60) / 50, 0, 1) : 0, sp = clamp(frame.shep / 1100, 0, 1), dr = clamp(frame.drive, 0, 1), pt = clamp(frame.pitch / 200, 0, 1);
    this.dive += (dv - this.dive) * Math.min(1, dt * 6); this.dim += ((frame.hold ? .4 : 1) - this.dim) * Math.min(1, dt * 12);
    const wv = this.dive, dim = this.dim;
    // the gate chops the light - less and less as the division gets faster: nothing on screen may flash hard at 5 Hz and up
    let lum = 1; if (frame.gate > 0 && frame.playing && ((CH.beatPos * frame.gate / 4) % 1) >= .5) lum = 1 - (frame.gate >= 16 ? .14 : frame.gate >= 8 ? .3 : .42) * (.55 + .45 * m2);
    // where it is: the spring lets it overshoot a little, a landing makes it bounce, speed stretches it
    const shake = m4 * sstep(.55, 1, u) * 2.6, cx = W - 215 + (shake > .05 ? Math.sin(time * 61) * shake : 0);
    const sy = hz + lerp(30, -186, sstep(0, 1, u) + (CH.cur - u) * 1.5) + (shake > .05 ? Math.cos(time * 53) * shake * .6 : 0);
    const bo = CH.boing < 1.2 ? Math.exp(-CH.boing * 5.5) * Math.cos(CH.boing * 19) * .2 : 0, sw = Math.sin(clamp(CH.swap, 0, 1) * Math.PI) * .1;
    const stretch = clamp(Math.abs(CH.vel) * .07, 0, .16) + pt * .07, r = lerp(58, 84, u) * (1 + .05 * CH.pulse + bo + sw);
    const C = { ink: rgb(PAL.ink), lid: rgb(PAL.lid), b1: rgb(PAL.b1), warm: rgb(warm), mouth: rgb([PAL.ink[0] * .7, PAL.ink[1] * .7, PAL.ink[2] * .7]) };

    const sky = g.createLinearGradient(0, 0, 0, hz); sky.addColorStop(0, rgba(col, lerp(.02, .16, u) * dim)); sky.addColorStop(.7, rgba(col, lerp(.04, .3, u) * dim)); sky.addColorStop(1, rgba(warm, lerp(.06, .5, u) * dim)); g.fillStyle = sky; g.fillRect(0, 0, W, hz);
    stars(70, 1 - u, time, 11);
    if (u < .2) { CH.nextShoot -= dt; if (CH.nextShoot <= 0) { CH.shootT = 0; CH.nextShoot = 7 + Math.random() * 9; CH.shootX = W * (.12 + Math.random() * .5); CH.shootY = H * (.05 + Math.random() * .2); } }
    if (CH.shootT >= 0) { CH.shootT += dt / .7; const f = CH.shootT; if (f >= 1) CH.shootT = -1; else { const x = CH.shootX + f * 170, y = CH.shootY + f * 62, gr = g.createLinearGradient(x - 70, y - 26, x, y);   // a shooting star, now and then, at night
      gr.addColorStop(0, 'rgba(255,255,255,0)'); gr.addColorStop(1, `rgba(255,255,255,${(Math.sin(f * Math.PI) * .9 * Math.max(0, 1 - u * 4)).toFixed(3)})`); g.strokeStyle = gr; g.lineWidth = 1.6; g.beginPath(); g.moveTo(x - 70, y - 26); g.lineTo(x, y); g.stroke(); } }
    const sea = g.createLinearGradient(0, hz, 0, H); sea.addColorStop(0, rgba(warm, lerp(.05, .35, u) * dim)); sea.addColorStop(.28, 'rgba(30,22,36,.92)'); sea.addColorStop(1, 'rgba(14,13,18,1)'); g.fillStyle = sea; g.fillRect(0, hz, W, H - hz);
    glowCircle(cx, sy, W * .95, col, lerp(.10, .42, u) * dim);
    const haze = g.createLinearGradient(0, hz - 80, 0, hz + 60); haze.addColorStop(0, rgba(warm, 0)); haze.addColorStop(.5, rgba(warm, lerp(.08, .38, u) * dim)); haze.addColorStop(1, rgba(warm, 0)); g.fillStyle = haze; g.fillRect(0, hz - 80, W, 140);
    // the rays in the sky
    g.save(); g.globalCompositeOperation = 'lighter'; g.translate(cx, sy);
    for (let layer = 0; layer < 2; layer++) { const n = layer ? 12 : 24, rot = time * (layer ? -.12 : .2), len = r + lerp(30, layer ? 280 : 180, sstep(.1, 1, u)), wd = layer ? .17 : .06, al = (layer ? .10 : .22) * lerp(.35, 1, u) * lum * dim;
      g.save(); g.rotate(rot); for (let i = 0; i < n; i++) { const a = i * TAU / n, L = len * (1 + Math.sin(time * 2.5 + i * 1.3) * .12); const gr = g.createLinearGradient(0, 0, Math.cos(a) * L, Math.sin(a) * L); gr.addColorStop(0, rgba(col, al)); gr.addColorStop(1, rgba(col, 0)); g.fillStyle = gr;
        g.beginPath(); g.moveTo(Math.cos(a - wd) * r * .9, Math.sin(a - wd) * r * .9); g.lineTo(Math.cos(a) * L, Math.sin(a) * L); g.lineTo(Math.cos(a + wd) * r * .9, Math.sin(a + wd) * r * .9); g.closePath(); g.fill(); } g.restore(); }
    // PULSAR: two beams that turn once a bar, like a lighthouse
    if (m2 > .02 && u > .03) { g.rotate(frame.playing ? CH.barPhase * TAU : time * .7);
      for (let i = 0; i < 2; i++) { g.rotate(Math.PI); const L = 560, gr = g.createLinearGradient(0, 0, L, 0); gr.addColorStop(0, rgba(PAL.b1, .36 * u * m2 * lum * dim)); gr.addColorStop(1, rgba(PAL.b1, 0)); g.fillStyle = gr; g.beginPath(); g.moveTo(r * .8, -3); g.lineTo(L, -48); g.lineTo(L, 48); g.lineTo(r * .8, 3); g.closePath(); g.fill(); } }
    g.restore();
    // the halo: the reverb widens it
    glowCircle(cx, sy, r * (2.6 + 2.2 * rv * u), col, (lerp(.25, .7, u) + .2 * CH.pulse) * lum * dim);
    // the riser: chevrons climb both sides of the sun and never arrive
    if (sp > .02) { g.save(); g.strokeStyle = rgb(PAL.b1); g.lineWidth = 2; g.lineCap = 'round'; g.lineJoin = 'round'; const y0 = Math.min(hz - 8, sy + r);
      [-1, 1].forEach(s => { for (let i = 0; i < 5; i++) { const f = (time * (.3 + .5 * u) + i / 5) % 1, x = cx + s * (r * 1.7 + 26), y = y0 - f * 170; g.globalAlpha = sp * Math.sin(f * Math.PI) * .6 * dim; g.beginPath(); g.moveTo(x - 11, y + 6); g.lineTo(x, y); g.lineTo(x + 11, y + 6); g.stroke(); } }); g.restore(); }

    g.save(); g.beginPath(); g.rect(0, 0, W, hz + 2); g.clip();                                                // everything of the sun itself sits behind the horizon
    // the delay throws a ring on every dotted eighth
    if (dl > .02) { CH.ringAcc += dt * (frame.bpm || 120) / 60 / .75; if (CH.ringAcc >= 1) { CH.ringAcc %= 1; if (this.rings.length < 6) this.rings.push({ a: 0, l: dl }); } }
    for (let i = this.rings.length - 1; i >= 0; i--) { const q = this.rings[i]; q.a += dt / 1.5; if (q.a >= 1) { this.rings.splice(i, 1); continue; }
      g.strokeStyle = rgba(PAL.b1, q.l * .3 * Math.pow(1 - q.a, 2) * dim); g.lineWidth = 2; g.beginPath(); g.arc(cx, sy, r * 1.25 + q.a * 210, 0, TAU); g.stroke(); }
    // ORBIT: a tilted ring with two moons - the far half goes behind the body, the near half in front
    const ringRx = r * 1.85, ringRy = r * .52, orbA = time * (.5 + 1.3 * u);
    const orbit = front => { if (m1 < .02) return; g.save(); g.translate(cx, sy); g.rotate(-.32); g.globalAlpha = m1 * lerp(.35, .9, u);
      g.strokeStyle = rgb(PAL.b1); g.lineWidth = 2.2; g.beginPath(); g.ellipse(0, 0, ringRx, ringRy, 0, front ? 0 : Math.PI, front ? Math.PI : TAU); g.stroke();
      for (let i = 0; i < 2; i++) { const a = orbA + i * Math.PI, sn = Math.sin(a); if ((sn >= 0) !== front) continue;
        g.fillStyle = rgb(i ? warm : PAL.b0); g.beginPath(); g.arc(Math.cos(a) * ringRx, sn * ringRy, (i ? 4.5 : 6.5) * (1 + .25 * sn), 0, TAU); g.fill(); }
      g.restore(); };
    orbit(false);
    const al = lerp(.35, .95, u) * dim;
    if (MW[0] > .02) { g.globalAlpha = MW[0] * al; spikes(cx, sy, r * 1.07, 12, r * (.1 + .1 * u + .08 * CH.pulse), .15, time * .25, rgb(PAL.b2), false, time); }
    if (m2 > .02) { g.globalAlpha = m2 * al * lum; spikes(cx, sy, r * 1.1, 8, r * (.3 + .35 * u) * (lum < 1 ? .6 : 1), .07, time * .6, rgb(PAL.b1), false, time); }
    if (m3 > .02) { g.globalAlpha = m3 * al * (1 - .6 * wv); spikes(cx, sy, r * 1.07, 12, r * (.1 + .08 * u), .15, Math.sin(time * .8) * .25, rgb(PAL.b2), false, time); }
    if (m4 > .02) { g.globalAlpha = m4 * al; spikes(cx, sy, r * 1.03, 16, r * (.14 + .32 * u), .15, time * .15, rgb(warm), true, time); g.globalAlpha = m4 * al * .9; spikes(cx, sy, r, 16, r * (.07 + .16 * u), .12, time * .15 + .2, rgb(PAL.b2), true, time + 3); }
    g.globalAlpha = 1;
    // MELTDOWN: it melts - drops run down from under it
    const melt = m4 * sstep(.35, .8, u);
    if (melt > .02) { g.fillStyle = rgb(PAL.b2); for (let i = 0; i < 3; i++) { const f = (time * .33 + i * .37) % 1, x = cx + (i - 1) * r * .5 + (i === 1 ? r * .12 : 0), y0 = sy + r * .82, y = y0 + f * f * r * .95, w = r * .11 * (1 - f * .45) * melt;
      const ym = y0 + (y - y0) * .4; g.beginPath(); g.moveTo(x - w * 1.3, y0); g.quadraticCurveTo(x - w * .3, ym, x - w * .3, y); g.arc(x, y, w * .75, Math.PI, 0, true); g.lineTo(x + w * .3, y); g.quadraticCurveTo(x + w * .3, ym, x + w * 1.3, y0); g.closePath(); g.fill(); } }
    // the body: squashed and stretched around its centre
    g.save(); g.translate(cx, sy); g.scale(1 - stretch * .6, 1 + stretch); g.translate(-cx, -sy);
    const sg = g.createRadialGradient(cx - r * .3, sy - r * .35, r * .1, cx, sy, r); sg.addColorStop(0, rgb(PAL.b0)); sg.addColorStop(.35, rgb(PAL.b1)); sg.addColorStop(.8, rgb(PAL.b2)); sg.addColorStop(.94, rgb(PAL.b3)); sg.addColorStop(1, rgba(col, 0));
    g.fillStyle = sg; g.beginPath(); g.arc(cx, sy, r, 0, TAU); g.fill();
    face(cx, sy, r / 330, u, blink, C);
    g.restore();
    orbit(true);
    // MELTDOWN: steam out of its ears
    const steam = m4 * sstep(.55, .85, u);
    if (steam > .02) [-1, 1].forEach(s => { for (let i = 0; i < 3; i++) { const f = (time * 1.1 + i / 3) % 1; g.fillStyle = `rgba(255,255,255,${(steam * (1 - f) * .5).toFixed(3)})`; g.beginPath(); g.arc(cx + s * (r * 1.08 + f * 34), sy - r * .25 - f * 44, 4 + f * 11, 0, TAU); g.fill(); } });
    g.restore();

    g.save(); g.globalCompositeOperation = 'lighter'; const refl = sstep(0, 1, u);
    for (let i = 0; i < 22; i++) { const yy = hz + 6 + i * (H - hz - 10) / 22, f = 1 - i / 22, wob = Math.sin(time * 2.2 + i * .9) * lerp(6, 16, u), wdt = lerp(30, 150, refl) * (.6 + f * .8) * (.7 + .3 * Math.sin(time * 3 + i));
      g.fillStyle = rgba(col, lerp(.05, .26, u) * f * lum * dim); g.beginPath(); g.ellipse(cx + wob, yy, wdt / 2, 2.2, 0, 0, TAU); g.fill(); }
    g.restore();
    // DIVE: the sea comes up over the sun, and it breathes through the snorkel
    if (wv > .01) { const top = Math.max(14, lerp(hz, Math.min(sy - r * 1.7, hz - 60), wv)), wave = x => top + Math.sin(x * .021 + time * 1.7) * 5 + Math.sin(x * .047 - time * 2.3) * 3;
      g.beginPath(); g.moveTo(0, hz); for (let x = 0; x <= W; x += 20) g.lineTo(x, wave(x)); g.lineTo(W, hz); g.closePath();
      const wg = g.createLinearGradient(0, top, 0, hz); wg.addColorStop(0, `rgba(60,170,255,${(.2 * wv).toFixed(3)})`); wg.addColorStop(1, `rgba(20,70,190,${(.42 * wv).toFixed(3)})`); g.fillStyle = wg; g.fill();
      g.strokeStyle = `rgba(210,245,255,${(.55 * wv).toFixed(3)})`; g.lineWidth = 1.5; g.beginPath(); for (let x = 0; x <= W; x += 20) g.lineTo(x, wave(x)); g.stroke();
      const y0 = Math.min(hz, sy - r * .9); g.lineWidth = 1.3;
      for (let i = 0; i < 9; i++) { const f = (time * (.35 + rnd(i + 3) * .3) + rnd(i)) % 1, by = y0 - f * (y0 - top); if (y0 < top + 12 || by < top + 5) continue;
        g.strokeStyle = `rgba(220,246,255,${(.7 * wv * (1 - f * .6)).toFixed(3)})`; g.beginPath(); g.arc(cx + r * (.72 + rnd(i * 7) * .4) + Math.sin(time * 2 + i) * 5, by, 1.5 + rnd(i * 5) * 3.2, 0, TAU); g.stroke(); } }
    for (const c of this.clouds) { const x = ((c.x * W + time * c.v) % (W + 220)) - 110, y = c.y * H + Math.sin(time * .4 + c.s) * 4; g.fillStyle = rgba(warm, lerp(.03, .13, u) * (1 - .7 * wv));
      [[0, 0, 62, 18], [-42, 6, 40, 13], [44, 5, 46, 14]].forEach(([dx, dy, rx, ry]) => { g.beginPath(); g.ellipse(x + dx * c.s, y + dy * c.s, rx * c.s, ry * c.s, 0, 0, TAU); g.fill(); }); }
    // the noise blows spray across the sky
    if (nz > .02) { g.save(); g.strokeStyle = '#fff'; g.lineCap = 'round'; for (let i = 0; i < 34; i++) { const spd = 180 + rnd(i + 40) * 380, x = W + 60 - ((time * spd + rnd(i + 9) * (W + 160)) % (W + 160)), y = rnd(i * 3 + 2) * hz * .96, L = 10 + rnd(i * 11) * 30;
      g.globalAlpha = nz * (.07 + rnd(i * 13) * .2) * dim; g.lineWidth = .8 + rnd(i * 17) * 1.2; g.beginPath(); g.moveTo(x, y); g.lineTo(x + L, y - L * .08); g.stroke(); } g.restore(); }
    // DRIVE throws sparks (MELTDOWN always does)
    const heat = clamp(dr * .6 + m4 * .35, 0, 1) * u;
    if (heat > .05 && this.sparks.length < 60 && Math.random() < heat) { const a = Math.random() * TAU; this.sparks.push({ x: cx + Math.cos(a) * r, y: sy + Math.sin(a) * r, vx: Math.cos(a) * (120 + Math.random() * 220), vy: Math.sin(a) * (120 + Math.random() * 220), life: .8 }); }
    if (this.sparks.length) { for (const p of this.sparks) { p.x += p.vx * dt; p.y += p.vy * dt; p.life -= dt; } this.sparks = this.sparks.filter(p => p.life > 0);
      g.save(); g.globalCompositeOperation = 'lighter'; for (const p of this.sparks) { g.fillStyle = `rgba(255,220,180,${Math.max(0, p.life).toFixed(3)})`; g.beginPath(); g.arc(p.x, p.y, 2, 0, TAU); g.fill(); } g.restore(); }
    // asleep: z z z
    if (CH.sleep > .05) { g.save(); g.fillStyle = rgb(PAL.b1); g.textAlign = 'center'; for (let i = 0; i < 3; i++) { const f = (time * .28 + i / 3) % 1; g.globalAlpha = CH.sleep * Math.sin(f * Math.PI) * .9; g.font = `800 ${Math.round(9 + f * 11)}px Sora, sans-serif`; g.fillText('z', cx + r * .9 + f * 34 + Math.sin(f * 6) * 4, Math.min(hz - 6, sy - r * .7) - f * 46); } g.restore(); }
    // the drop: confetti in the mode's colours, two shockwaves
    if (CH.burst > 0 && CH.dropT >= 0) { const M = MODES[CH.mode]; burst(this.P, cx, sy, 70, 6.3, [rgb(M.b2), '#fff', rgb(M.warm), '#FFD02B', '#2BD9FF']); CH.burst = 0; }
    if (CH.dropT >= 0) for (let k = 0; k < 2; k++) { const d = CH.dropT - k * .12; if (d < 0 || d > 1) continue; g.strokeStyle = rgba(k ? PAL.b1 : [255, 255, 255], (1 - d) * (k ? .5 : .8)); g.lineWidth = lerp(k ? 14 : 30, 2, d); g.beginPath(); g.arc(cx, sy, r + d * 900, 0, TAU); g.stroke(); }
    confetti(this.P, dt, 300);
  } };

let last = performance.now(), tAcc = 0, pageVisible = true, skipFrame = false, seenPhase = -1, seenAt = 0;
document.addEventListener('visibilitychange', () => { pageVisible = !document.hidden; });
function animate(now) {
  requestAnimationFrame(animate);
  if (!pageVisible) { last = now; return; }                                   // hidden editor: draw nothing
  const busy = CH.cur > 0.01 || frame.running || frame.tail > 0.01 || frame.playing || CH.dropT >= 0 || CH.cool > 0 || frame.armed || now - CH.pAt < 1500;
  if (!busy) { skipFrame = !skipFrame; if (skipFrame) return; }                // at rest: 30 fps is plenty
  const dt = Math.min(.08, (now - last) / 1000); last = now; tAcc += dt;
  // the host's bar position arrives about 30 times a second: run it on in between, so the beat and the gate stay smooth
  if (frame.phase !== seenPhase) { seenPhase = frame.phase; seenAt = now; }
  const bpb = frame.bpb || 4, ph = frame.playing ? seenPhase + Math.min(.25, (now - seenAt) / 1000) * (frame.bpm || 120) / 60 / bpb : 0;
  CH.barPhase = ph % 1; CH.beatPos = ph * bpb;
  // a new mode: the colours glide over, the sun blinks and pops
  const mode = clamp(state.preset | 0, 0, 4); if (mode !== CH.mode) { CH.mode = mode; CH.swap = 1; }
  if (CH.swap > 0) CH.swap = Math.max(0, CH.swap - dt * 4);
  const M = MODES[mode], kp = Math.min(1, dt * 7);
  for (let i = 0; i < 5; i++) MW[i] += ((i === mode ? 1 : 0) - MW[i]) * kp;
  for (const k of PKEYS) { const a = PAL[k], b = M[k]; a[0] += (b[0] - a[0]) * kp; a[1] += (b[1] - a[1]) * kp; a[2] += (b[2] - a[2]) * kp; }
  if (CH.cool > 0) CH.cool = Math.max(0, CH.cool - dt);
  if (CH.boing < 9) CH.boing += dt;
  if (CH.dropT >= 0) { CH.dropT += dt; if (CH.dropT > 1.2) CH.dropT = -1; }
  // the tail keeps the grin: the sun stays up while the reverb spills. After a drop it stays a moment in its shades.
  let tgt = Math.max(CH.target, frame.running ? 0 : frame.tail * 0.85);
  if (CH.cool > .55) tgt = Math.max(tgt, .62); if (frame.armed) tgt = Math.max(tgt, .1);
  CH.vel += ((tgt - CH.cur) * 170 - CH.vel * 18) * dt; CH.cur += CH.vel * dt;   // a spring, a touch under-damped
  const u = clamp(CH.cur, 0, 1);
  // left alone at night with the transport stopped, it falls asleep; the pointer wakes it
  const awake = u > .02 || frame.playing || frame.armed || CH.cool > 0 || now - CH.pAt < 6000;
  CH.idle = awake ? 0 : CH.idle + dt; CH.sleep += ((CH.idle > 3 ? 1 : 0) - CH.sleep) * Math.min(1, dt * 3);
  CH.nextBlink -= dt; if (CH.nextBlink <= 0) { CH.blink = 1; CH.nextBlink = 2 + Math.random() * 3.5; } if (CH.blink > 0) CH.blink = Math.max(0, CH.blink - dt * 9);
  const blink = Math.max(CH.blink > 0 ? Math.sin((1 - CH.blink) * Math.PI) : 0, CH.swap > 0 ? Math.sin(CH.swap * Math.PI) : 0);
  // where it looks: at the pointer, at the knob while a run is on, or around
  let lx, ly; const sunY = H * .64 + lerp(30, -186, sstep(0, 1, u));
  if (frame.hold || CH.sleep > .5) { lx = 0; ly = 0; }
  else if (now - CH.pAt < 3000) { lx = clamp((CH.px - (W - 215)) / 240, -1, 1); ly = clamp((CH.py - sunY) / 180, -1, 1); }
  else if (frame.running || frame.armed) { lx = -.75; ly = .35; }
  else { lx = Math.sin(tAcc * .5) * .45; ly = Math.cos(tAcc * .37) * .25; }
  const ll = Math.hypot(lx, ly); if (ll > 1) { lx /= ll; ly /= ll; }
  const tp = poseFor(mode, u, { hold: frame.hold, cool: CH.cool > 0, sleep: CH.sleep, dive: sun.dive, armed: frame.armed }); tp.lx = lx; tp.ly = ly;
  const kq = Math.min(1, dt * 14); for (const k in tp) POSE[k] += (tp[k] - POSE[k]) * kq;
  g.setTransform(DPR, 0, 0, DPR, 0, 0); g.clearRect(0, 0, W, H);
  // the beat: the sun pulses on every beat while the host plays
  const beatFrac = frame.playing ? CH.beatPos % 1 : 1;
  CH.pulse = frame.playing ? Math.exp(-beatFrac * 7) * (0.35 + 0.65 * u) : 0;
  sun.draw(u, dt, tAcc, blink);
}
requestAnimationFrame(animate);
