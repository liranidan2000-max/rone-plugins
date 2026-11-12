// Offline checks of the browser port against what the C++ says it does.
const { RiseProc, makeLoop } = require('./engine.js');
const SR = 48000, N = 1024, BPM = 145;
const db = x => (20 * Math.log10(Math.max(x, 1e-9))).toFixed(1);
let fails = 0; const ok = (c, msg) => { console.log((c ? 'PASS ' : 'FAIL ') + msg); if (!c) fails++; };

function mk(fixed) { const p = new RiseProc(SR); p.bpm = BPM; p.playing = true; p.fixed = !!fixed; return p; }
function run(p, blocks, src, cb) {   // src(i) -> sample; cb(L, R, blockIndex)
  let i = 0; const L = new Float32Array(N), R = new Float32Array(N);
  for (let b = 0; b < blocks; b++) { for (let k = 0; k < N; k++) { const v = src(i++); L[k] = v; R[k] = v; } p.process(L, R, N); if (cb) cb(L, R, b, i); }
}
const sine = f => i => 0.5 * Math.sin(2 * Math.PI * f * i / SR);

// 1. null at zero: bit-transparent
{ const p = mk(); let maxd = 0; let i0 = 0;
  run(p, 40, sine(100), (L, R, b, i) => { for (let k = 0; k < N; k++) maxd = Math.max(maxd, Math.abs(L[k] - Math.fround(sine(100)(i - N + k)))); });
  ok(maxd === 0, 'null at INTENSITY 0 (max diff ' + maxd + ')'); }

// 2. KILL LOW reach: 100 % -> 3 kHz, 35 % -> ~330 Hz (the C++ comment)
for (const [kill, want] of [[1, 3000], [0.35, 330]]) {
  const p = mk(); p.P.kill = kill; p.P.space = 0; p.P.intensity = 1; run(p, 60, sine(100));
  ok(Math.abs(p.engine.ui.hpf - want) / want < 0.05, `KILL ${kill} at 100 % -> HPF ${p.engine.ui.hpf.toFixed(0)} Hz (want ~${want})`); }

// 3. the drop on a 50 Hz tone: how long until the tone is back within 3 dB
function dropRecovery(fixed, kill) {
  const p = mk(fixed); p.P.kill = kill; p.P.space = 0; p.P.sink = 0.55; p.P.intensity = 1;
  run(p, 100, sine(50));
  p.P.intensity = 0; let t3 = -1, t1 = -1, n = 0; const win = Math.round(SR / 50);   // one period
  const out = [];
  run(p, 20, sine(50), (L) => { for (let k = 0; k < N; k++) out.push(L[k]); });
  for (let s = 0; s + win < out.length; s += 48) { let pk = 0; for (let k = 0; k < win; k++) pk = Math.max(pk, Math.abs(out[s + k]));
    if (t3 < 0 && pk >= 0.5 * 0.708) t3 = s / SR * 1000; if (t1 < 0 && pk >= 0.5 * 0.891) t1 = s / SR * 1000; }
  return { t3, t1 };
}
{ const a = dropRecovery(false, 1), b = dropRecovery(false, 0.35), c = dropRecovery(true, 1), d = dropRecovery(true, 0.35);
  console.log(`  manual snap, 50 Hz back within 3 dB / 1 dB:  today KILL 100 %: ${a.t3.toFixed(0)} / ${a.t1.toFixed(0)} ms   today KILL 35 %: ${b.t3.toFixed(0)} / ${b.t1.toFixed(0)} ms`);
  console.log(`                                              fixed KILL 100 %: ${c.t3.toFixed(0)} / ${c.t1.toFixed(0)} ms   fixed KILL 35 %: ${d.t3.toFixed(0)} / ${d.t1.toFixed(0)} ms`);
  ok(a.t3 > 60 && c.t3 < 25, 'today is slow to give the lows back, fixed is quick'); }

// 4. LAND ON BAR 1 bar on the built-in loop: level of the first 30 ms after the bar line vs the dry loop
function landing(fixed, gap, preset) {
  const p = mk(fixed), loop = makeLoop(SR, BPM), spb = 60 / BPM * SR;
  Object.assign(p.P, preset || { kill: 1, space: 0.73, sink: 0.55 }); p.P.land = 5; p.P.gap = gap || 0; p.P.tail = 0;
  const out = []; let fired = false, pos = 0;
  const L = new Float32Array(N), R = new Float32Array(N);
  while (pos < spb * 12) { for (let k = 0; k < N; k++) { L[k] = loop.L[(pos + k) % loop.len]; R[k] = loop.R[(pos + k) % loop.len]; }
    if (!fired && pos >= spb * 4 - N) { /* GO inside bar 1 (0-based) so the run covers bar 1 and lands on bar 2 */ }
    if (!fired && pos + N > spb * 4.5) { p.goReq = true; fired = true; }
    p.process(L, R, N); for (let k = 0; k < N; k++) out.push(L[k]); pos += N; }
  const line = Math.round(spb * 8);   // GO in bar 1 -> lands at beat 8
  const rms = (a, s, n) => { let e = 0; for (let k = 0; k < n; k++) e += a[s + k] * a[s + k]; return Math.sqrt(e / n); };
  const w = Math.round(0.03 * SR), dry = rms(loop.L, line % loop.len, w), wet = rms(out, line, w);
  const pre = rms(out, line - Math.round(0.25 * spb), Math.round(0.2 * spb));
  return { rel: 20 * Math.log10(wet / dry), pre: 20 * Math.log10(pre + 1e-9) };
}
{ const a = landing(false), b = landing(true);
  console.log(`  LAND 1 bar, first 30 ms of the drop kick vs dry:  today ${a.rel.toFixed(1)} dB   fixed ${b.rel.toFixed(1)} dB`);
  ok(b.rel > -0.5 && a.rel < -3, 'fixed lands with the full kick, today does not');
  const g = landing(true, 2), g0 = landing(true, 0);
  console.log(`  GAP 1 beat: level in the last quarter beat before the line ${g.pre.toFixed(1)} dBFS (no gap ${g0.pre.toFixed(1)} dBFS), drop kick ${g.rel.toFixed(1)} dB`);
  ok(g.pre < g0.pre - 6 && g.rel > -0.5, 'GAP mutes the dry before the line and opens on it'); }

// 5. everything on, 20 s of the loop: finite, sane peak, and the cost
{ const p = mk(true), loop = makeLoop(SR, BPM); Object.assign(p.P, { rise: 1, noise: 1, space: 1, delay: 0.9, kill: 1, gate: 1, sink: 0.8, pitch: 1, preset: 4, drive: 0.8, land: 1, tail: 2, gap: 1 });
  p.setAudition(true);
  const L = new Float32Array(N), R = new Float32Array(N); let pk = 0, bad = false, pos = 0; const t0 = process.hrtime.bigint();
  for (let b = 0; b < Math.round(20 * SR / N); b++) { for (let k = 0; k < N; k++) { L[k] = loop.L[(pos + k) % loop.len]; R[k] = loop.R[(pos + k) % loop.len]; } p.process(L, R, N); pos += N;
    for (let k = 0; k < N; k++) { if (!isFinite(L[k])) bad = true; pk = Math.max(pk, Math.abs(L[k]), Math.abs(R[k])); } }
  const ms = Number(process.hrtime.bigint() - t0) / 1e6;
  ok(!bad && pk < 2.5, `CHAOS audition 20 s: finite, peak ${db(pk)} dBFS, ${(ms / 20000 * 100).toFixed(1)} % of real time`); }

// 6. FALL: armed until the bar line, then 100 % falling to 0
{ const p = mk(true); p.P.land = 5; p.P.dir = 1; p.P.curve = 0; const spb = 60 / BPM * SR; let log = [];
  const L = new Float32Array(N), R = new Float32Array(N); let pos = 0;
  while (pos < spb * 10) { if (Math.abs(pos - spb * 1.5) < N / 2) p.goReq = true; p.process(L, R, N); log.push([pos / spb, p.ui.t, p.ui.armed, p.ui.running]); pos += N; }
  const at = b => log.reduce((a, x) => Math.abs(x[0] - b) < Math.abs(a[0] - b) ? x : a);
  ok(at(3)[2] && at(3)[1] === 0 && at(4.2)[3] && at(4.2)[1] > 0.9 && Math.abs(at(6)[1] - 0.5) < 0.05 && at(8.5)[1] === 0, `FALL: armed at beat 3, ${at(4.2)[1].toFixed(2)} after the line, ${at(6)[1].toFixed(2)} half way, ${at(8.5)[1]} after`); }

console.log(fails ? fails + ' FAILED' : 'all good'); process.exit(fails ? 1 : 0);
