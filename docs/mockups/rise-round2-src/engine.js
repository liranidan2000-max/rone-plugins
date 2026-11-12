/* RONE Rise in the browser.
   RiseEngine + Reverb are a line-for-line port of RoneRise/Source/DSP/RoneRiseEngine.cpp and
   Shared/RoneReverbEngine.cpp as released in 1.1.1; RiseProc is Source/PluginProcessor.cpp
   (the phrase clock, LAND ON BAR, the landing latch, REST ON STOP, the drop detector).
   Added for the round-2 mockup: LAND 1 / 2 bars, RISE / FALL, GAP, AUDITION, a DRIVE macro,
   and `fixed` = the round-1 landing (sample-accurate, filters open ON the bar line). */
(function (root) {
'use strict';
const clampf = (v, a, b) => v < a ? a : v > b ? b : v;
const sstepf = (a, b, x) => { const t = clampf((x - a) / (b - a), 0, 1); return t * t * (3 - 2 * t); };
const nextPow2 = n => { let p = 1; while (p < n) p <<= 1; return p; };
const TWO_PI = Math.PI * 2;

// juce::SmoothedValue<float> (linear)
class Smooth {
  constructor() { this.cur = 0; this.tgt = 0; this.cd = 0; this.step = 0; this.steps = 0; }
  reset(sr, sec) { this.steps = Math.floor(sec * sr); this.setNow(this.tgt); }
  setNow(v) { this.cur = this.tgt = v; this.cd = 0; }
  set(v) { if (v === this.tgt) return; if (this.steps <= 0) { this.setNow(v); return; } this.tgt = v; this.cd = this.steps; this.step = (this.tgt - this.cur) / this.cd; }
  next() { if (this.cd <= 0) return this.tgt; if (--this.cd > 0) this.cur += this.step; else this.cur = this.tgt; return this.cur; }
  skip(n) { if (n >= this.cd) { this.setNow(this.tgt); return; } this.cur += this.step * n; this.cd -= n; }
  get smoothing() { return this.cd > 0; }
}

// juce::dsp::StateVariableTPTFilter, two channels. type: 0 low-pass, 2 high-pass
class SVF {
  constructor(type) { this.type = type; this.a = new Float64Array(2); this.b = new Float64Array(2); this.sr = 44100; this.fc = 1000; this.q = 0.707; this.g = 0; this.R2 = 1; this.h = 1; }
  prepare(sr) { this.sr = sr; this.update(); this.reset(); }
  set(fc, q) { if (fc === this.fc && (q === undefined || q === this.q)) return; this.fc = fc; if (q !== undefined) this.q = q; this.update(); }
  update() { this.g = Math.tan(Math.PI * this.fc / this.sr); this.R2 = 1 / this.q; this.h = 1 / (1 + this.R2 * this.g + this.g * this.g); }
  reset() { this.a[0] = this.a[1] = this.b[0] = this.b[1] = 0; }
  proc(ch, x) {
    const g = this.g, s1 = this.a[ch], s2 = this.b[ch];
    const hp = this.h * (x - s1 * (g + this.R2) - s2);
    const bp = hp * g + s1; this.a[ch] = hp * g + bp;
    const lp = bp * g + s2; this.b[ch] = bp * g + lp;
    return this.type === 0 ? lp : hp;
  }
}

// ============================================================================ the AFTERSPACE reverb
const BASE48 = [1687, 1931, 2213, 2503, 2801, 3109, 3413, 3719];
const DIFF_L48 = [254, 379, 538, 710], DIFF_R48 = [269, 397, 557, 733];
const DENS48 = [113, 149, 181, 211, 239, 271, 307, 337];
const ER_L = [7.1, 11.3, 17.9, 23.6, 31.7, 41.3, 53.9, 67.7], ER_R = [8.3, 13.1, 19.7, 26.3, 34.9, 43.7, 56.3, 71.9];
const SIGN_L = [1, 1, -1, -1, 1, 1, -1, -1], SIGN_R = [1, -1, -1, 1, 1, -1, -1, 1];
const IN_GAIN = 0.353553391, OUT_GAIN = 0.42, MAX_SIZE = 1.15, MOD_MARGIN = 24;
function fastSin01(ph) { const x = ph - 0.5; const y = x * (8 - 16 * Math.abs(x)); return y * (0.775 + 0.225 * Math.abs(y)); }
function readLin(buf, mask, w, d) { const rp = w - d, i0 = Math.floor(rp), fr = rp - i0, s0 = buf[i0 & mask]; return s0 + fr * (buf[(i0 + 1) & mask] - s0); }

class Reverb {
  constructor() { this.size = 0.55; this.decay = 2.2; this.modRate = 0.25; this.oL = 0; this.oR = 0; }
  prepare(sr) {
    this.sr = sr; this.k = sr / 48000;
    this.preSize = nextPow2(Math.floor(sr * 0.350) + 8);
    this.preL = new Float32Array(this.preSize); this.preR = new Float32Array(this.preSize); this.preW = 0;
    const S = sec => { const s = new Smooth(); s.reset(sr, sec); return s; };
    this.smPre = S(0.05); this.smPre.setNow(0.020 * sr);
    this.erG = new Float32Array(8); for (let t = 0; t < 8; t++) this.erG[t] = 0.32 * Math.pow(0.80, t);
    this.erTL = new Float32Array(8); this.erTR = new Float32Array(8); this.erSL = 0; this.erSR = 0;
    this.dL = []; this.dR = []; this.dpL = [0, 0, 0, 0]; this.dpR = [0, 0, 0, 0];
    for (let i = 0; i < 4; i++) { this.dL.push(new Float32Array(Math.max(4, Math.floor(DIFF_L48[i] * this.k)))); this.dR.push(new Float32Array(Math.max(4, Math.floor(DIFF_R48[i] * this.k)))); }
    this.smDiff = S(0.05); this.smDiff.setNow(0.25 + 0.5 * 0.70);
    this.fB = []; this.fMask = []; this.fW = []; this.smLen = []; this.smFb = []; this.nB = []; this.nP = []; this.lfoPh = []; this.lfoInc = new Float64Array(8); this.baseLen = new Float64Array(8);
    for (let i = 0; i < 8; i++) {
      const size = nextPow2(Math.floor(BASE48[i] * this.k * MAX_SIZE + MOD_MARGIN * this.k + 8));
      this.fB.push(new Float32Array(size)); this.fMask.push(size - 1); this.fW.push(0);
      this.smLen.push(S(0.15)); this.smFb.push(S(0.05));
      this.nB.push(new Float32Array(Math.max(4, Math.floor(DENS48[i] * this.k)))); this.nP.push(0);
      this.lfoPh.push((0.618034 * i) % 1);
    }
    this.smHi = S(0.05); this.smLo = S(0.05); this.smDens = S(0.05); this.smMod = S(0.10); this.smEr = S(0.05); this.smLate = S(0.05);
    this.hi = new Float64Array(8); this.lo = new Float64Array(8); this.line = new Float64Array(8);
    this.setSize(this.size); this.setDecay(this.decay); this.setModRate(this.modRate); this.setModDepth(0.15);
    this.setHighDamp(0.4); this.setLowDamp(0.2); this.setDensity(0.7); this.setEarlyLate(0.3);
    for (let i = 0; i < 8; i++) { this.smLen[i].setNow(this.smLen[i].tgt); this.smFb[i].setNow(this.smFb[i].tgt); }
    this.reset();
  }
  reset() {
    this.preL.fill(0); this.preR.fill(0);
    for (let i = 0; i < 4; i++) { this.dL[i].fill(0); this.dR[i].fill(0); }
    for (let i = 0; i < 8; i++) { this.fB[i].fill(0); this.nB[i].fill(0); this.hi[i] = 0; this.lo[i] = 0; }
    this.erSL = this.erSR = 0;
  }
  setSize(s) {
    this.size = clampf(s, 0, 1); const sc = 0.30 + 0.85 * this.size;
    for (let i = 0; i < 8; i++) { this.baseLen[i] = BASE48[i] * this.k * sc; this.smLen[i].set(this.baseLen[i]); }
    const er = (0.45 + 0.75 * this.size) * this.sr / 1000;
    for (let t = 0; t < 8; t++) { this.erTL[t] = ER_L[t] * er; this.erTR[t] = ER_R[t] * er; }
    this.updFb();
  }
  setDecay(sec) { this.decay = clampf(sec, 0.2, 30); this.updFb(); }
  updFb() { const t60 = this.decay * this.sr; for (let i = 0; i < 8; i++) this.smFb[i].set(clampf(Math.pow(10, -3 * this.baseLen[i] / t60), 0, 0.9995)); }
  setPreDelayMs(ms) { this.smPre.set(clampf(ms, 0, 250) * this.sr / 1000); }
  setDiffusion(a) { this.smDiff.set(0.25 + 0.5 * clampf(a, 0, 1)); }
  setDensity(a) { this.smDens.set(0.20 + 0.45 * clampf(a, 0, 1)); }
  setModRate(hz) { this.modRate = clampf(hz, 0.01, 5); for (let i = 0; i < 8; i++) this.lfoInc[i] = this.modRate * (0.82 + 0.05 * i) / this.sr; }
  setModDepth(a) { this.smMod.set(clampf(a, 0, 1.5) * 9 * this.k); }
  setHighDamp(a) { const c = 20000 * Math.pow(1200 / 20000, clampf(a, 0, 1)); this.smHi.set(clampf(1 - Math.exp(-TWO_PI * c / this.sr), 0, 1)); }
  setLowDamp(a) { const c = 15 * Math.pow(350 / 15, clampf(a, 0, 1)); this.smLo.set(clampf(1 - Math.exp(-TWO_PI * c / this.sr), 0, 1)); }
  setEarlyLate(b) { const x = (clampf(b, -1, 1) + 1) * 0.5; this.smEr.set(Math.cos(x * Math.PI / 2)); this.smLate.set(Math.sin(x * Math.PI / 2)); }
  proc(inL, inR) {
    const pm = this.preSize - 1, w = this.preW;
    this.preL[w] = inL; this.preR[w] = inR;
    const pre = this.smPre.next();
    const pdL = readLin(this.preL, pm, w, pre + 1), pdR = readLin(this.preR, pm, w, pre + 1);
    let erL = 0, erR = 0;
    for (let t = 0; t < 8; t++) { erL += this.erG[t] * readLin(this.preL, pm, w, pre + this.erTL[t]); erR += this.erG[t] * readLin(this.preR, pm, w, pre + this.erTR[t]); }
    this.erSL += 0.55 * (erL - this.erSL); this.erSR += 0.55 * (erR - this.erSR); erL = this.erSL; erR = this.erSR;
    this.preW = (w + 1) & pm;
    const dc = this.smDiff.next(); let dL = pdL, dR = pdR;
    for (let i = 0; i < 4; i++) {
      const bL = this.dL[i], pL = this.dpL[i], dl = bL[pL], wl = dL - dc * dl; bL[pL] = wl; dL = dl + dc * wl; this.dpL[i] = pL + 1 < bL.length ? pL + 1 : 0;
      const bR = this.dR[i], pR = this.dpR[i], dr = bR[pR], wr = dR - dc * dr; bR[pR] = wr; dR = dr + dc * wr; this.dpR[i] = pR + 1 < bR.length ? pR + 1 : 0;
    }
    const mod = this.smMod.next(), hiC = this.smHi.next(), loC = this.smLo.next(), densC = this.smDens.next();
    const line = this.line; let sum = 0;
    for (let i = 0; i < 8; i++) {
      const lfo = fastSin01(this.lfoPh[i]); this.lfoPh[i] += this.lfoInc[i]; if (this.lfoPh[i] >= 1) this.lfoPh[i] -= 1;
      let x = readLin(this.fB[i], this.fMask[i], this.fW[i], this.smLen[i].next() + lfo * mod);
      this.hi[i] += hiC * (x - this.hi[i]); x = this.hi[i];
      this.lo[i] += loC * (x - this.lo[i]); x -= this.lo[i] * 0.85 * (loC > 0 ? 1 : 0);
      line[i] = x; sum += x;
    }
    const kk = sum * (2 / 8);
    for (let i = 0; i < 8; i++) {
      const fb = this.smFb[i].next();
      let inj = ((i & 1) === 0 ? dL : dR) * IN_GAIN; if ((i & 3) >= 2) inj = -inj;
      let v = inj + fb * (line[i] - kk);
      const db = this.nB[i], dp = this.nP[i], dly = db[dp], ww = v - densC * dly; db[dp] = ww; v = dly + densC * ww; this.nP[i] = dp + 1 < db.length ? dp + 1 : 0;
      this.fB[i][this.fW[i]] = v; this.fW[i] = (this.fW[i] + 1) & this.fMask[i];
    }
    let lateL = 0, lateR = 0;
    for (let i = 0; i < 8; i++) { lateL += SIGN_L[i] * line[i]; lateR += SIGN_R[i] * line[i]; }
    lateL *= OUT_GAIN * IN_GAIN * 2; lateR *= OUT_GAIN * IN_GAIN * 2;
    const eg = this.smEr.next(), lg = this.smLate.next();
    this.oL = erL * eg + lateL * lg; this.oR = erR * eg + lateR * lg;
  }
}

// ============================================================================ RoneRiseEngine
const WEIGHTS = [
  { hpf: 0.35, rev: 1, del: 0.0, drive: 0.15, rateLink: 0 },   // LIFTOFF
  { hpf: 0.35, rev: 1, del: 0.8, drive: 0.2,  rateLink: 0 },   // ORBIT
  { hpf: 1.0,  rev: 1, del: 0.4, drive: 0.4,  rateLink: 0 },   // PULSAR
  { hpf: 0.25, rev: 1, del: 0.7, drive: 0.3,  rateLink: 0 },   // UNDERTOW
  { hpf: 1.0,  rev: 1, del: 0.8, drive: 0.8,  rateLink: 1 } ]; // MELTDOWN
const HA = [0.6923878, 0.9360654322959, 0.9882295226860, 0.9987488452737];
const HB = [0.4021921162426, 0.8561710882420, 0.9722909545651, 0.9952884791278];
function softSat(x) { const a = Math.abs(x); if (a <= 0.7) return x; const over = a - 0.7, y = 0.7 + over / (1 + over * 1.6); return x < 0 ? -y : y; }
const MAXB = 8192;

class RiseEngine {
  prepare(sr) {
    this.sr = sr;
    this.hpf = new SVF(2); this.hpf2 = new SVF(2); this.lpf = new SVF(0); this.lpf2 = new SVF(0);
    for (const f of [this.hpf, this.hpf2]) { f.prepare(sr); f.set(20, 0.707); }
    for (const f of [this.lpf, this.lpf2]) { f.prepare(sr); f.set(20000, 0.707); }
    this.gateCoeff = 1 - Math.exp(-1 / (0.0025 * sr));
    this.pitchHzCoeff = 1 - Math.exp(-1 / (0.02 * sr));
    this.reverb = new Reverb(); this.reverb.prepare(sr);
    this.reverb.setPreDelayMs(18); this.reverb.setDiffusion(0.7); this.reverb.setDensity(0.6);
    this.reverb.setModRate(0.3); this.reverb.setModDepth(0.3); this.reverb.setLowDamp(0.3); this.reverb.setEarlyLate(0.6);
    this.revHP = new SVF(2); this.revHP.prepare(sr); this.revHP.set(150, 0.707);
    this.lineLength = nextPow2(Math.floor(2.0 * sr));
    this.lines = [new Float32Array(this.lineLength), new Float32Array(this.lineLength)];
    this.delayCoeff = 1 - Math.exp(-1 / (0.08 * sr));
    this.loopLpCoeff = 1 - Math.exp(-TWO_PI * 3800 / sr);
    this.noiseHP = new SVF(2); this.noiseHP.prepare(sr); this.noiseHP.set(150, 1.3);
    this.noiseLP = new SVF(0); this.noiseLP.prepare(sr); this.noiseLP.set(9000, 0.707);
    this.wetL = new Float32Array(MAXB); this.wetR = new Float32Array(MAXB);
    this.nzL = new Float32Array(MAXB); this.nzR = new Float32Array(MAXB);
    this.dryL = new Float32Array(MAXB); this.dryR = new Float32Array(MAXB);
    const S = sec => { const s = new Smooth(); s.reset(sr, sec); return s; };
    this.revG = S(0.03); this.delG = S(0.03); this.noiseG = S(0.03); this.shepG = S(0.03); this.sendDel = S(0.03); this.fbSm = S(0.03);
    this.driveSm = S(0.03); this.widthSm = S(0.03); this.revFeed = S(0.03); this.dryG = S(0.03); this.pitchBlend = S(0.03);
    this.bypassMix = S(0.01); this.gapG = S(0.005);
    this.hpfMix = S(0.01); this.lpfMix = S(0.01); this.fadeSlow = Math.max(1, Math.floor(0.01 * sr)); this.fadeQuick = Math.max(1, Math.floor(0.005 * sr));   // 1.2: a filter leaves the path by a crossfade
    this.slow = Math.floor(0.03 * sr); this.quick = Math.floor(0.003 * sr);
    this.cutFadeLen = Math.max(32, Math.floor(0.02 * sr));
    this.hx = new Float64Array(3 * 2 * 2 * 4 * 2); this.hy = new Float64Array(3 * 2 * 2 * 4 * 2); this.hd = new Float64Array(6);
    this.rng = new Uint32Array([0x9E3779B9, 0x7F4A7C15]);
    this.loopLp = [0, 0]; this.oscPhase = [0, 0];
    this.ui = { hpf: 20, lpf: 20000, gate: 0, pitch: 0, rev: 0, del: 0, noise: -90, shep: 0, drive: 0, wet: 0, tail: 0 };
    this.delayT = 0.75 * 0.5 * sr; this.bypassOn = false;
    this.reset();
  }
  reset() {
    this.hpf.reset(); this.hpf2.reset(); this.hpfHz = 20; this.hpfQ = 0.707; this.hpfOn = false; this.hpfMix.setNow(0);
    this.lpf.reset(); this.lpf2.reset(); this.lpfHz = 20000; this.lpfOn = false; this.lpfMix.setNow(0); this.gateSm = 1; this.gateDiv = 0;
    this.pitchPhase = 0; this.pitchHz = 0; this.pitchBlend.setNow(0);
    this.dryG.setNow(1); this.gapG.setNow(1);
    this.reverb.reset(); this.revHP.reset(); this.revFeed.setNow(0); this.revLevel = this.revRelRef = 0; this.delRelRef = 0; this.uRoom = 0; this.tailHeld = 0;
    this.lines[0].fill(0); this.lines[1].fill(0); this.writePos = 0; this.delayLen = this.delayT; this.loopLp[0] = this.loopLp[1] = 0; this.delLevel = 0;
    this.noiseHP.reset(); this.noiseLP.reset(); this.noiseHz = 150; this.noiseLfo = 0;
    this.hx.fill(0); this.hy.fill(0); this.hd.fill(0);
    this.oscPhase[0] = this.oscPhase[1] = 0; this.cyclePhase = 0;
    for (const s of [this.revG, this.delG, this.noiseG, this.shepG, this.sendDel, this.fbSm, this.driveSm, this.widthSm]) s.setNow(0);
    this.bypassMix.setNow(this.bypassOn ? 1 : 0); this.bypassSettled = true;
    this.uiWetAcc = 0; this.cutFadeLeft = 0; this.inEnv = 0;
  }
  // single-sideband shift, Hilbert allpass pair (Niemitalo 4 + 4), per (voice, channel)
  freqShift(voice, ch, x, c, s) {
    const hx = this.hx, hy = this.hy; let c0 = 0, c1 = 0;
    for (let k = 0; k < 2; k++) {
      const coef = k === 0 ? HA : HB; let v = x;
      for (let st = 0; st < 4; st++) {
        const b = ((((voice * 2 + ch) * 2 + k) * 4) + st) * 2;
        const y = coef[st] * coef[st] * (v + hy[b + 1]) - hx[b + 1];
        hx[b + 1] = hx[b]; hx[b] = v; hy[b + 1] = hy[b]; hy[b] = y; v = y;
      }
      if (k === 0) c0 = v; else c1 = v;
    }
    const di = voice * 2 + ch, I = this.hd[di]; this.hd[di] = c0;
    return I * c + c1 * s;
  }
  readHermite(line, d) {
    const rp = this.writePos - d, i1 = Math.floor(rp), fr = rp - i1, m = this.lineLength - 1, b = this.lines[line];
    const y0 = b[(i1 - 1) & m], y1 = b[i1 & m], y2 = b[(i1 + 1) & m], y3 = b[(i1 + 2) & m];
    const c1 = 0.5 * (y2 - y0), c2 = y0 - 2.5 * y1 + 2 * y2 - 0.5 * y3, c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2);
    return ((c3 * fr + c2) * fr + c1) * fr + y1;
  }
  /* p: t, preset, rise, noise, space, delay, kill, dive, gate, sink, pitch, drive, delaySamples, shepCycleSec,
        tailSec, cut, bypass, barPhase, bpb, spb  (as RoneRiseEngine::setParameters)
        + hold (GAP: the dry is muted, the sends close, the tails keep ringing)
        + fast (round 1: the filters and the dry come back in ~10 ms) + runP (LAND progress, -1 = none) + fixed */
  process(L, R, off, n, p) {
    const sr = this.sr;
    this.bypassOn = !!p.bypass; this.bypassMix.set(p.bypass ? 1 : 0);
    const ui = this.ui;
    if (p.bypass && this.bypassSettled) { ui.hpf = 20; ui.lpf = 20000; ui.gate = 0; ui.pitch = 0; ui.rev = 0; ui.del = 0; ui.noise = -90; ui.shep = 0; ui.drive = 0; ui.wet = 0; ui.tail = 0; return; }
    const fading = this.bypassMix.smoothing;
    if (fading || p.bypass) for (let i = 0; i < n; i++) { this.dryL[i] = L[off + i]; this.dryR[i] = R[off + i]; }

    const u = clampf(p.t, 0, 1), w = WEIGHTS[clampf(p.preset | 0, 0, 4)];
    const rise = p.rise, noise = p.noise, space = p.space, kill = p.kill, dive = p.dive, gate = p.gate, sink = p.sink, pitch = p.pitch, echo = p.delay;
    const hold = !!p.hold, fast = !!p.fast;
    const blockSec = n / sr, coefBlock = 1 - Math.exp(-blockSec / 0.03), coefFast = 1 - Math.exp(-blockSec / 0.003);
    this.delayT = clampf(p.delaySamples, 32, this.lineLength - 8);
    if (p.cut && this.cutFadeLeft === 0) this.cutFadeLeft = this.cutFadeLen;

    { let pk = 0; for (let i = 0; i < n; i++) { const a = Math.abs(L[off + i]), b = Math.abs(R[off + i]); if (a > pk) pk = a; if (b > pk) pk = b; }
      const a = pk > this.inEnv ? 1 - Math.exp(-blockSec / 0.01) : 1 - Math.exp(-blockSec / 1.0); this.inEnv += (pk - this.inEnv) * a; }

    // ---- 1. HPF (KILL LOW) ----
    let hpfTarget = 20;
    if (kill > 0.02) { const maxHz = 20 * Math.pow(150, Math.pow(kill, 0.55)); hpfTarget = 20 * Math.pow(maxHz / 20, Math.pow(u, 1.4)); }
    if (fast) this.hpfHz = Math.exp(Math.log(this.hpfHz) + (Math.log(hpfTarget) - Math.log(this.hpfHz)) * coefFast);
    else this.hpfHz += (hpfTarget - this.hpfHz) * coefBlock;
    this.hpfQ += ((0.707 + 0.25 * u * kill) - this.hpfQ) * coefBlock;
    let hpfActive = this.hpfHz > 21;
    if (p.fixed) {
      // 1.2 (as the C++): the filter comes INTO the path at once and goes OUT of it by a crossfade to the input -
      // 5 ms on a drop, 10 ms when the knob simply came down. Dropping it in one sample was a click.
      const leaveNow = fast && hpfTarget <= 21, mixT = (this.hpfHz > 21 && !leaveNow) ? 1 : 0;
      if (!this.hpfOn && mixT > 0) { this.hpfOn = true; this.hpfMix.setNow(1); }
      else if (this.hpfOn) { this.hpfMix.steps = leaveNow ? this.fadeQuick : this.fadeSlow; this.hpfMix.set(mixT); }
      if (this.hpfOn) {
        const fc = clampf(this.hpfHz, 20, 0.45 * sr); this.hpf.set(fc, 0.707); this.hpf2.set(fc, this.hpfQ);
        for (let i = 0; i < n; i++) { const mix = this.hpfMix.next(), xl = L[off + i], xr = R[off + i], fl = this.hpf2.proc(0, this.hpf.proc(0, xl)), fr = this.hpf2.proc(1, this.hpf.proc(1, xr));
          L[off + i] = mix >= 1 ? fl : xl + (fl - xl) * mix; R[off + i] = mix >= 1 ? fr : xr + (fr - xr) * mix; }
        if (this.hpfMix.cd <= 0 && this.hpfMix.tgt <= 0) { this.hpfOn = false; this.hpf.reset(); this.hpf2.reset(); if (hpfTarget <= 21) this.hpfHz = hpfTarget; }
      }
      hpfActive = this.hpfOn;
    } else if (hpfActive) {
      const fc = clampf(this.hpfHz, 20, 0.45 * sr); this.hpf.set(fc, 0.707); this.hpf2.set(fc, this.hpfQ);
      for (let i = 0; i < n; i++) { L[off + i] = this.hpf2.proc(0, this.hpf.proc(0, L[off + i])); R[off + i] = this.hpf2.proc(1, this.hpf.proc(1, R[off + i])); }
    } else { this.hpf.reset(); this.hpf2.reset(); }

    // ---- 1b. LPF (DIVE) ----
    let lpfTarget = 20000;
    if (dive > 0.02) lpfTarget = 20000 * Math.pow(500 / 20000, Math.pow(u, 1.3) * dive);
    if (fast) this.lpfHz = Math.exp(Math.log(this.lpfHz) + (Math.log(lpfTarget) - Math.log(this.lpfHz)) * coefFast);
    else this.lpfHz += (lpfTarget - this.lpfHz) * coefBlock;
    let lpfActive = this.lpfHz < 19000;
    if (p.fixed) {
      const leaveNow = fast && lpfTarget >= 19000, mixT = (this.lpfHz < 19000 && !leaveNow) ? 1 : 0;
      if (!this.lpfOn && mixT > 0) { this.lpfOn = true; this.lpfMix.setNow(1); }
      else if (this.lpfOn) { this.lpfMix.steps = leaveNow ? this.fadeQuick : this.fadeSlow; this.lpfMix.set(mixT); }
      if (this.lpfOn) {
        const fc = clampf(this.lpfHz, 200, Math.min(20000, 0.45 * sr)); this.lpf.set(fc, 0.707); this.lpf2.set(fc, 0.707 + 0.5 * u * dive);
        for (let i = 0; i < n; i++) { const mix = this.lpfMix.next(), xl = L[off + i], xr = R[off + i], fl = this.lpf2.proc(0, this.lpf.proc(0, xl)), fr = this.lpf2.proc(1, this.lpf.proc(1, xr));
          L[off + i] = mix >= 1 ? fl : xl + (fl - xl) * mix; R[off + i] = mix >= 1 ? fr : xr + (fr - xr) * mix; }
        if (this.lpfMix.cd <= 0 && this.lpfMix.tgt <= 0) { this.lpfOn = false; this.lpf.reset(); this.lpf2.reset(); if (lpfTarget >= 19000) this.lpfHz = lpfTarget; }
      }
      lpfActive = this.lpfOn;
    } else if (lpfActive) {
      const fc = clampf(this.lpfHz, 200, Math.min(20000, 0.45 * sr)); this.lpf.set(fc, 0.707); this.lpf2.set(fc, 0.707 + 0.5 * u * dive);
      for (let i = 0; i < n; i++) { L[off + i] = this.lpf2.proc(0, this.lpf.proc(0, L[off + i])); R[off + i] = this.lpf2.proc(1, this.lpf.proc(1, R[off + i])); }
    } else { this.lpf.reset(); this.lpf2.reset(); }

    // ---- 1c. GATE ----
    const gateDepth = hold ? 0 : gate * Math.pow(u, 0.8);
    let gateDiv = 0;
    if (gateDepth > 0.02) {
      const beat0 = p.barPhase * p.bpb, spb = p.spb;
      if (p.fixed) {
        // round 1: the division only changes ON a beat line; in a LAND run it follows the phrase in four equal parts
        const x = p.runP >= 0 ? p.runP : u;
        const want = p.runP >= 0 ? (x < 0.25 ? 4 : x < 0.5 ? 8 : x < 0.75 ? 16 : 32) : (u < 0.3 ? 4 : u < 0.55 ? 8 : u < 0.8 ? 16 : 32);
        const onBeat = Math.floor(beat0) !== Math.floor(beat0 + n / spb) || beat0 - Math.floor(beat0) < n / spb;
        if (this.gateDiv === 0 || onBeat) this.gateDiv = want;
        gateDiv = this.gateDiv;
      } else gateDiv = u < 0.3 ? 4 : u < 0.55 ? 8 : u < 0.8 ? 16 : 32;
      const divBeats = 4 / gateDiv, closed = 1 - gateDepth;
      for (let i = 0; i < n; i++) {
        const pos = (beat0 + i / spb) / divBeats, target = (pos - Math.floor(pos)) < 0.5 ? 1 : closed;
        this.gateSm += (target - this.gateSm) * this.gateCoeff;
        L[off + i] *= this.gateSm; R[off + i] *= this.gateSm;
      }
    } else {
      // off, the drop or the GAP. 1.2 (as the C++): it opens over its own 2.5 ms - a run lets go 20 ms before the bar line,
      // where a 1/16 or 1/32 gate is shut, and opening it in one sample was a step
      this.gateDiv = 0;
      if (p.fixed && this.gateSm < 0.9995) for (let i = 0; i < n; i++) { this.gateSm += (1 - this.gateSm) * this.gateCoeff; L[off + i] *= this.gateSm; R[off + i] *= this.gateSm; }
      else this.gateSm = 1;
    }

    // ---- 1d. PITCH ----
    this.pitchBlend.steps = fast ? this.quick : this.slow;
    const pitchHzTarget = pitch * 200 * Math.pow(u, 1.5);
    this.pitchBlend.set(pitch > 0.02 ? sstepf(0, 0.12, u) : 0);
    if (this.pitchBlend.cur > 0.001 || this.pitchBlend.tgt > 0.001) {
      for (let i = 0; i < n; i++) {
        this.pitchHz += (pitchHzTarget - this.pitchHz) * this.pitchHzCoeff;
        this.pitchPhase += TWO_PI * this.pitchHz / sr; if (this.pitchPhase >= TWO_PI) this.pitchPhase -= TWO_PI;
        const c = Math.cos(this.pitchPhase), s = Math.sin(this.pitchPhase), b = this.pitchBlend.next();
        const xl = L[off + i]; L[off + i] = xl + (this.freqShift(2, 0, xl, c, s) - xl) * b;
        const xr = R[off + i]; R[off + i] = xr + (this.freqShift(2, 1, xr, c, s) - xr) * b;
      }
    } else { this.pitchBlend.skip(n); this.pitchHz = 0; }

    const wetL = this.wetL, wetR = this.wetR, nzL = this.nzL, nzR = this.nzR;
    for (let i = 0; i < n; i++) { wetL[i] = 0; wetR[i] = 0; }

    // ---- 2. NOISE ----
    const inGate = sstepf(0.001, 0.02, this.inEnv);
    const noiseGainT = hold ? 0 : noise * u * u * 0.22 * inGate;
    this.noiseG.set(noiseGainT);
    if (noiseGainT > 0.0005 || this.noiseG.cur > 0.0005) {
      this.noiseLfo += n / sr * 0.35;
      const lfo = Math.sin(this.noiseLfo * TWO_PI) * 0.12;
      const nzTarget = 150 * Math.pow(32, u + lfo);
      this.noiseHz += (nzTarget - this.noiseHz) * coefBlock;
      this.noiseHP.set(clampf(this.noiseHz, 60, 0.4 * sr));
      const rng = this.rng;
      for (let i = 0; i < n; i++) {
        const g = this.noiseG.next();
        for (let ch = 0; ch < 2; ch++) {
          let r = rng[ch]; r ^= r << 13; r >>>= 0; r ^= r >>> 17; r ^= r << 5; r >>>= 0; rng[ch] = r;
          let v = ((r & 0xFFFFFF) / 8388608 - 1) * 0.5;
          v = this.noiseLP.proc(ch, this.noiseHP.proc(ch, v)) * g;
          if (ch === 0) { nzL[i] = v; wetL[i] += v; } else { nzR[i] = v; wetR[i] += v; }
        }
      }
    } else { this.noiseG.skip(n); for (let i = 0; i < n; i++) { nzL[i] = 0; nzR[i] = 0; } }

    // ---- 3. REVERB ----
    const tailSec = p.tailSec, coef250 = 1 - Math.exp(-blockSec / 0.25);
    const revFromT = hold ? 0 : w.rev * space * Math.pow(u, 1.3) * 0.6;
    if (hold) { /* GAP: the level stays where it was, nothing new is sent in */ }
    else if (revFromT > 0.005) { this.revLevel = revFromT > this.revLevel ? revFromT : this.revLevel + (revFromT - this.revLevel) * coef250; this.revRelRef = this.revLevel; this.tailHeld = 0; }
    else if (tailSec > 0.001) { this.tailHeld += blockSec; if (this.tailHeld > tailSec * 0.35) this.revLevel = Math.max(0, this.revLevel - this.revRelRef * blockSec / (tailSec * 0.65)); }
    else this.revLevel = 0;
    this.revG.set(this.revLevel);
    this.revFeed.set(revFromT > 0.005 ? 1 : 0);
    if (this.revLevel > 0.001 || this.revG.cur > 0.001 || this.revFeed.cur > 0.001) {
      if (u > this.uRoom) this.uRoom = u; else if (revFromT > 0.005) this.uRoom += (u - this.uRoom) * coef250;
      const rv = this.reverb;
      rv.setSize(0.5 + 0.45 * this.uRoom);
      rv.setDecay(revFromT > 0.005 ? 1.6 + 4.5 * this.uRoom : Math.max(1.6 + 4.5 * this.uRoom, 1.3 * tailSec));
      rv.setHighDamp(0.4 - 0.2 * this.uRoom);
      this.revHP.set(clampf(Math.max(150, this.hpfHz * 1.2), 150, 0.4 * sr));
      for (let i = 0; i < n; i++) {
        const feed = this.revFeed.next();
        rv.proc((L[off + i] + nzL[i] * 0.7) * feed, (R[off + i] + nzR[i] * 0.7) * feed);
        const g = this.revG.next();
        wetL[i] += this.revHP.proc(0, rv.oL) * g; wetR[i] += this.revHP.proc(1, rv.oR) * g;
      }
    } else { this.revG.skip(n); this.revFeed.skip(n); }

    // ---- 4. DELAY ----
    const delFromT = hold ? 0 : echo * Math.pow(u, 1.2) * 0.5;
    if (hold) { /* held */ }
    else if (delFromT > 0.005) { this.delLevel = delFromT > this.delLevel ? delFromT : this.delLevel + (delFromT - this.delLevel) * coef250; this.delRelRef = this.delLevel; }
    else if (tailSec > 0.001) { if (this.tailHeld > tailSec * 0.35) this.delLevel = Math.max(0, this.delLevel - this.delRelRef * blockSec / (tailSec * 0.65)); }
    else this.delLevel = 0;
    this.delG.set(this.delLevel);
    if (this.delLevel > 0.001 || this.delG.cur > 0.001) {
      this.sendDel.set(delFromT > 0.005 ? 1 : 0);
      this.fbSm.set(0.3 + 0.5 * this.uRoom);
      this.widthSm.set(1 + 0.6 * u);
      const m = this.lineLength - 1, dl = this.lines[0], dr = this.lines[1];
      for (let i = 0; i < n; i++) {
        this.delayLen += (this.delayT - this.delayLen) * this.delayCoeff;
        const fb = this.fbSm.next(), send = this.sendDel.next();
        const inp = 0.5 * (L[off + i] + R[off + i]) * send;
        const oL = this.readHermite(0, this.delayLen), oR = this.readHermite(1, this.delayLen);
        this.loopLp[0] += (oR - this.loopLp[0]) * this.loopLpCoeff; this.loopLp[1] += (oL - this.loopLp[1]) * this.loopLpCoeff;
        dl[this.writePos] = softSat(inp + this.loopLp[0] * fb); dr[this.writePos] = softSat(this.loopLp[1] * fb);
        this.writePos = (this.writePos + 1) & m;
        const g = this.delG.next(), wd = this.widthSm.next(), mid = 0.5 * (oL + oR), side = 0.5 * (oL - oR) * wd;
        wetL[i] += (mid + side) * g; wetR[i] += (mid - side) * g;
      }
    } else { this.delG.skip(n); this.sendDel.skip(n); this.fbSm.skip(n); this.widthSm.skip(n); }

    // ---- 5. SHEPARD ----
    const shepGainT = hold ? 0 : rise * Math.pow(u, 1.4) * 0.55;
    this.shepG.set(shepGainT);
    let shepRange = 0;
    if (shepGainT > 0.0005 || this.shepG.cur > 0.0005) {
      shepRange = 180 + 520 * u;
      const cycleSec = Math.max(0.25, p.shepCycleSec) / (1 + 3 * u * w.rateLink), cycleInc = 1 / (cycleSec * sr);
      for (let i = 0; i < n; i++) {
        const g = this.shepG.next();
        this.cyclePhase += cycleInc; if (this.cyclePhase >= 1) this.cyclePhase -= 1;
        let yL = 0, yR = 0;
        for (let v = 0; v < 2; v++) {
          let ph = this.cyclePhase + 0.5 * v; if (ph >= 1) ph -= 1;
          const win = 0.5 - 0.5 * Math.cos(TWO_PI * ph);
          this.oscPhase[v] += TWO_PI * shepRange * ph / sr; if (this.oscPhase[v] >= TWO_PI) this.oscPhase[v] -= TWO_PI;
          const c = Math.cos(this.oscPhase[v]), s = Math.sin(this.oscPhase[v]);
          yL += win * this.freqShift(v, 0, L[off + i], c, s); yR += win * this.freqShift(v, 1, R[off + i], c, s);
        }
        wetL[i] += yL * g; wetR[i] += yR * g;
      }
    } else this.shepG.skip(n);

    // ---- the cut: 20 ms fade on everything wet, then the lines are emptied ----
    if (this.cutFadeLeft > 0) {
      for (let i = 0; i < n; i++) { const g = this.cutFadeLeft > 0 ? this.cutFadeLeft / this.cutFadeLen : 0; wetL[i] *= g; wetR[i] *= g; if (this.cutFadeLeft > 0) this.cutFadeLeft--; }
      if (this.cutFadeLeft === 0) {
        this.reverb.reset(); this.revHP.reset(); this.lines[0].fill(0); this.lines[1].fill(0); this.loopLp[0] = this.loopLp[1] = 0;
        this.revLevel = this.delLevel = this.revRelRef = this.delRelRef = 0; this.tailHeld = 0;
        this.revG.setNow(0); this.delG.setNow(0); this.noiseG.setNow(0); this.shepG.setNow(Math.min(this.shepG.tgt, this.shepG.cur));
      }
    }

    // ---- 6. DRIVE on the wet, SINK on the dry, sum ----
    const driveAmt = p.drive == null ? w.drive : p.drive;
    this.driveSm.set(driveAmt * u);
    this.dryG.steps = fast ? this.quick : this.slow;
    this.dryG.set(1 - sink * Math.pow(u, 1.6));
    this.gapG.set(hold ? 0 : 1);
    let wetPeak = 0;
    for (let i = 0; i < n; i++) {
      const d = this.driveSm.next(), dg = this.dryG.next() * this.gapG.next();
      const pre = 1 + 4 * d, post = 1 / (1 + 1.6 * d);
      const l = softSat(wetL[i] * pre) * post, r = softSat(wetR[i] * pre) * post;
      const al = Math.abs(l), ar = Math.abs(r); if (al > wetPeak) wetPeak = al; if (ar > wetPeak) wetPeak = ar;
      L[off + i] = L[off + i] * dg + l; R[off + i] = R[off + i] * dg + r;
    }

    // ---- BYPASS crossfade ----
    if (fading || p.bypass) {
      for (let i = 0; i < n; i++) { const bm = this.bypassMix.next(); L[off + i] += (this.dryL[i] - L[off + i]) * bm; R[off + i] += (this.dryR[i] - R[off + i]) * bm; }
      if (p.bypass && !this.bypassMix.smoothing) {
        this.reverb.reset(); this.revHP.reset(); this.lines[0].fill(0); this.lines[1].fill(0); this.loopLp[0] = this.loopLp[1] = 0; this.revLevel = this.delLevel = 0;
        for (const s of [this.revG, this.delG, this.noiseG, this.shepG, this.sendDel, this.revFeed]) s.setNow(0);
        this.bypassSettled = true;
      } else this.bypassSettled = false;
    } else { this.bypassMix.skip(n); this.bypassSettled = false; }

    for (let i = 0; i < n; i++) if (!isFinite(L[off + i]) || !isFinite(R[off + i])) { this.reset(); for (let j = 0; j < n; j++) { L[off + j] = 0; R[off + j] = 0; } return; }

    this.uiWetAcc += (wetPeak - this.uiWetAcc) * 0.3;
    ui.wet = clampf(this.uiWetAcc, 0, 1);
    ui.hpf = hpfActive ? this.hpfHz : 20; ui.lpf = lpfActive ? this.lpfHz : 20000; ui.gate = gateDiv;
    ui.pitch = this.pitchBlend.cur > 0.01 ? this.pitchHz : 0;
    ui.rev = clampf(this.revG.cur / 0.6, 0, 1); ui.del = clampf(this.delG.cur / 0.5, 0, 1);
    ui.noise = this.noiseG.cur > 0.0005 ? 20 * Math.log10(this.noiseG.cur * 0.5) : -90;
    ui.shep = this.shepG.cur > 0.0005 ? shepRange : 0;
    ui.drive = clampf(this.driveSm.cur, 0, 1);
    ui.tail = (u < 0.05 || hold) ? clampf(Math.max(this.revG.cur / 0.6, this.delG.cur / 0.5) * 1.5, 0, 1) : 0;
  }
}

// ============================================================================ the processor
const LAND_BARS = [0, 4, 8, 16, 32, 1, 2];   // by the LAND parameter index: 1 and 2 bars were appended in 1.2.0
const GAP_BEATS = [0, 0.5, 1, 2];
function shape(x, curve) {   // RoneBarClock::shape - 0 LIN, 1 EXP, 2 S
  x = clampf(x, 0, 1);
  if (curve === 1) return Math.pow(x, 2.6);
  if (curve === 2) { const e = Math.pow(x, 2.2), push = x > 0.75 ? Math.pow((x - 0.75) / 0.25, 2) : 0; return clampf(e * 0.8 + push * 0.2, 0, 1); }
  return x;
}

class RiseProc {
  constructor(sr) {
    this.sr = sr; this.engine = new RiseEngine(); this.engine.prepare(sr);
    this.P = { intensity: 0, preset: 0, bypass: 0, rise: 0, noise: 0, space: 0.73, delay: 0, kill: 0.35, dive: 0, gate: 0, sink: 0.55, pitch: 0, drive: 0.15,
               curve: 1, tail: 0, land: 0, dir: 0, gap: 0 };
    this.fixed = false;            // round 1 landing
    this.audition = false;
    this.ppq = 0; this.bpm = 145; this.bpb = 4; this.playing = false; this.prevPlaying = false;
    this.goReq = false; this.cancelReq = false;
    this.running = false; this.runDir = 0; this.runStart = 0; this.runBeats = 0;
    this.armed = false; this.armPpq = 0; this.armBars = 0;
    this.audNext = 0; this.holdUntil = -1; this.fastLeft = 0;
    this.recentMax = 0; this.latched = false; this.latchKnob = 0;
    this.ui = { t: 0, latched: false, running: false, bar: 0, beat: 0, bars: 0, bpm: 145, playing: false, phase: 0, bpb: 4, armed: false, audition: false, hold: false, prog: 0 };
  }
  spb() { return 60 / this.bpm * this.sr; }
  barStart() { return Math.floor((this.ppq + 1 / this.spb()) / this.bpb) * this.bpb; }   // a position within one sample of a bar line IS that line
  setAudition(on) {
    this.audition = !!on;
    if (on) { this.latched = false; this.running = false; this.armed = false; this.audNext = this.barStart(); }
    else { this.running = false; this.armed = false; this.latched = false; this.holdUntil = -1; }
  }
  // samples until the next moment the state changes inside a block (Infinity = none)
  nextEvent(spb) {
    let e = Infinity; const add = s => { if (s > 0.5 && s < e) e = s; };
    if (this.running) {
      const toLand = (this.runStart + this.runBeats - this.ppq) * spb;
      if (this.runDir === 0) { const gb = GAP_BEATS[this.P.gap | 0]; if (gb > 0) add(toLand - gb * spb); }
      if (this.fixed) add(toLand - 0.020 * this.sr);
    }
    if (this.armed) add((this.armPpq - this.ppq) * spb);
    if (this.audition && !this.running && !this.armed) add((this.audNext - this.ppq) * spb);
    if (this.holdUntil >= 0) add((this.holdUntil - this.ppq) * spb);
    return e;
  }
  process(L, R, n) {
    let pos = 0;
    while (pos < n) {
      const spb = this.spb();
      let len = this.fixed ? Math.min(32, n - pos) : n - pos;   // today: one decision per host block, as the plugin does
      const ev = this.nextEvent(spb); if (ev < len) len = Math.max(1, Math.ceil(ev));
      this.step(L, R, pos, len, spb);
      this.ppq += len / spb;
      pos += len;
    }
  }
  startRun(bars) {
    if (this.P.dir === 0) { this.running = true; this.runDir = 0; this.runStart = this.barStart(); this.runBeats = bars * this.bpb; }
    else {   // FALL starts ON a bar line: this one if we are on it, else the next
      const bs = this.barStart(), onLine = this.ppq - bs < 2 / this.spb();
      this.armed = true; this.armPpq = onLine ? bs : bs + this.bpb; this.armBars = bars;
    }
  }
  step(L, R, off, len, spb) {
    const P = this.P, sr = this.sr, bpb = this.bpb, ppq = this.ppq, eps = 0.5 / spb;
    let go = this.goReq; this.goReq = false;
    if (this.cancelReq) { this.cancelReq = false; this.running = false; this.armed = false; this.latched = false; this.audition = false; this.holdUntil = -1; }

    // REST ON STOP
    let stopCut = false;
    if (this.prevPlaying && !this.playing) { this.running = false; this.armed = false; this.audition = false; this.holdUntil = -1; stopCut = true; this.latched = true; this.latchKnob = P.intensity; }
    if (!this.prevPlaying && this.playing) this.latched = false;
    this.prevPlaying = this.playing;

    const landBars = LAND_BARS[clampf(P.land | 0, 0, 6)], tailChoice = clampf(P.tail | 0, 0, 4);
    const audBars = landBars > 0 ? landBars : 4;

    if (go) {
      this.latched = false;
      if (this.audition) { this.audition = false; this.running = false; this.armed = false; }
      else if (this.running || this.armed) { this.running = false; this.armed = false; }
      else if (landBars > 0) this.startRun(landBars);
    }
    if (this.audition && !this.running && !this.armed && ppq >= this.audNext - eps) { this.latched = false; this.startRun(audBars); }
    if (this.armed && ppq >= this.armPpq - eps) { this.armed = false; this.running = true; this.runDir = 1; this.runStart = this.armPpq; this.runBeats = this.armBars * bpb; }

    const knob = P.intensity;
    if (this.latched && Math.abs(knob - this.latchKnob) > 0.01) this.latched = false;
    let eff = this.latched ? 0 : knob, landedNow = false, hold = false, runP = -1;
    if (this.running) {
      const landPpq = this.runStart + this.runBeats;
      const toLand = (landPpq - ppq) * spb, early = this.fixed ? 0.020 * sr : 0;
      const p = Math.max(0, (ppq - this.runStart) / Math.max(1e-6, this.runBeats));
      if (toLand <= early + 0.5) {
        this.running = false; eff = 0; landedNow = this.runDir === 0; this.latched = true; this.latchKnob = knob;
        if (this.fixed && this.runDir === 0 && GAP_BEATS[P.gap | 0] > 0) this.holdUntil = landPpq - 0.005 * sr / spb;   // the dry opens ON the line
        if (this.audition) this.audNext = landPpq + (audBars <= 2 ? 1 : 2) * bpb;
      } else {
        const x = this.runDir === 0 ? p : 1 - p, curve = clampf(P.curve | 0, 0, 3);
        eff = curve === 0 ? x : curve === 1 ? Math.pow(x, 1.35) : shape(x, curve - 1);
        runP = p;
        if (this.runDir === 0) { const gb = GAP_BEATS[P.gap | 0]; if (gb > 0 && toLand <= gb * spb + 0.5) hold = true; }
      }
    }
    if (this.holdUntil >= 0) { if (ppq < this.holdUntil - eps) hold = true; else this.holdUntil = -1; }

    // the drop: a fast pull to zero (mouse, automation snap, the landing)
    let cut = false;
    this.recentMax = Math.max(eff, this.recentMax * Math.exp(-len / (0.12 * sr)));
    const dropped = landedNow || (this.recentMax > 0.35 && eff < 0.05);
    if (dropped) { this.recentMax = 0; if (tailChoice === 0) cut = true; if (this.fixed) this.fastLeft = Math.floor(0.06 * sr); }
    if (stopCut) { cut = true; this.recentMax = 0; }
    const tailSec = tailChoice === 0 ? 0 : tailChoice * bpb * 60 / this.bpm;

    const bs = Math.floor(ppq / bpb + 1e-9) * bpb, phase = (ppq - bs) / bpb, ui = this.ui;
    ui.t = eff; ui.latched = this.latched; ui.running = this.running; ui.armed = this.armed; ui.audition = this.audition; ui.hold = hold;
    ui.bars = this.audition ? audBars : landBars; ui.bpm = this.bpm; ui.playing = this.playing; ui.phase = phase; ui.bpb = bpb;
    if (this.running) { const bi = ppq - this.runStart; ui.bar = Math.floor(bi / bpb + 1e-9); ui.beat = Math.floor((bi % bpb) + 1e-9); ui.prog = clampf(bi / this.runBeats, 0, 1); }
    else { ui.bar = 0; ui.beat = 0; ui.prog = 0; }

    const fast = this.fastLeft > 0; if (fast) this.fastLeft -= len;
    this.engine.process(L, R, off, len, {
      t: eff, preset: P.preset, rise: P.rise, noise: P.noise, space: P.space, delay: P.delay, kill: P.kill, dive: P.dive, gate: P.gate, sink: P.sink, pitch: P.pitch, drive: P.drive,
      delaySamples: 0.75 * spb, shepCycleSec: 2 * bpb * 60 / this.bpm, tailSec, cut, bypass: P.bypass > 0.5,
      barPhase: phase, bpb, spb, hold, fast, runP, fixed: this.fixed });
  }
}

// ============================================================================ the built-in loop (145 BPM psy pattern, 4 bars)
function makeLoop(sr, bpm) {
  const spb = 60 / bpm * sr, len = Math.round(spb * 16), L = new Float32Array(len), R = new Float32Array(len);
  let seed = 0x1234567; const rnd = () => { seed |= 0; seed = seed + 0x6D2B79F5 | 0; let t = Math.imul(seed ^ seed >>> 15, 1 | seed); t = t + Math.imul(t ^ t >>> 7, 61 | t) ^ t; return ((t ^ t >>> 14) >>> 0) / 4294967296; };
  const add = (i, l, r) => { const k = ((i % len) + len) % len; L[k] += l; R[k] += r; };
  const s16 = spb / 4;
  for (let b = 0; b < 16; b++) {
    const t0 = Math.round(b * spb);
    // kick: a sine falling from ~220 Hz to 48 Hz, plus a 2 ms click
    { let ph = 0; const n = Math.floor(0.30 * sr);
      for (let i = 0; i < n; i++) { const t = i / sr, f = 48 + 175 * Math.exp(-t / 0.016); ph += TWO_PI * f / sr;
        const a = Math.exp(-t / 0.13) * (1 - Math.exp(-t / 0.0004)) * Math.min(1, (n - i) / (0.01 * sr));
        const v = Math.sin(ph) * a * 0.9 + (t < 0.002 ? (rnd() * 2 - 1) * 0.25 * (1 - t / 0.002) : 0); add(t0 + i, v, v); } }
    // rolling bass on the three sixteenths after the kick (G1, 49 Hz)
    for (let k = 1; k < 4; k++) { const s0 = Math.round(t0 + k * s16), n = Math.floor(s16 * 0.92); let ph = 0, lp = 0;
      for (let i = 0; i < n; i++) { const t = i / sr; ph += 49 / sr; if (ph >= 1) ph -= 1;
        const saw = 2 * ph - 1, fc = 180 + 1500 * Math.exp(-t / 0.035), c = 1 - Math.exp(-TWO_PI * fc / sr); lp += (saw - lp) * c;
        const a = Math.min(1, t / 0.002) * Math.exp(-t / 0.085) * Math.min(1, (n - i) / (0.004 * sr)); const v = lp * a * 0.62; add(s0 + i, v, v); } }
    // clap on 2 and 4
    if (b % 4 === 1 || b % 4 === 3) { const n = Math.floor(0.16 * sr); let hp = 0, prev = 0;
      for (let i = 0; i < n; i++) { const t = i / sr, x = rnd() * 2 - 1; hp = 0.82 * (hp + x - prev); prev = x;
        const a = Math.exp(-t / 0.045) * (0.6 + 0.4 * Math.abs(Math.sin(t * 190))) * 0.2; add(t0 + i, hp * a, hp * a * 0.9); } }
    // hats: open on the off-beat, closed on the sixteenths
    for (let k = 0; k < 4; k++) { const s0 = Math.round(t0 + k * s16), open = k === 2, n = Math.floor((open ? 0.07 : 0.025) * sr); let hp = 0, prev = 0;
      for (let i = 0; i < n; i++) { const t = i / sr, x = rnd() * 2 - 1; hp = 0.6 * (hp + x - prev); prev = x;
        const a = Math.exp(-t / (open ? 0.03 : 0.008)) * (open ? 0.11 : 0.045); add(s0 + i, hp * a * 0.8, hp * a); } }
  }
  // lead: a sixteenth arp in G minor, alternating sides
  const scale = [0, 3, 7, 12, 15, 12, 7, 3, 0, 7, 10, 15, 19, 15, 10, 7], roots = [0, 0, -4, -2];
  for (let s = 0; s < 64; s++) {
    if (s % 4 === 0) continue;                                   // leave the kick alone
    const bar = s >> 4, note = 55 + roots[bar] + scale[(s + bar * 3) % 16], f = 440 * Math.pow(2, (note - 69) / 12);
    const s0 = Math.round(s * s16), n = Math.floor(s16 * 1.6), pan = s % 2 ? 0.72 : 0.28; let ph = 0, ph2 = 0, lp = 0;
    for (let i = 0; i < n; i++) { const t = i / sr; ph += f / sr; if (ph >= 1) ph -= 1; ph2 += f * 1.006 / sr; if (ph2 >= 1) ph2 -= 1;
      const x = (2 * ph - 1) * 0.6 + (2 * ph2 - 1) * 0.4, fc = 900 + 3200 * Math.exp(-t / 0.06), c = 1 - Math.exp(-TWO_PI * fc / sr); lp += (x - lp) * c;
      const a = Math.min(1, t / 0.003) * Math.exp(-t / 0.11) * 0.17; add(s0 + i, lp * a * (1 - pan) * 1.4, lp * a * pan * 1.4); }
  }
  let pk = 0; for (let i = 0; i < len; i++) pk = Math.max(pk, Math.abs(L[i]), Math.abs(R[i]));
  const g = 0.7 / pk; for (let i = 0; i < len; i++) { L[i] *= g; R[i] *= g; }
  return { L, R, len };
}

const api = { RiseEngine, RiseProc, Reverb, makeLoop, WEIGHTS, LAND_BARS, GAP_BEATS };
if (typeof module !== 'undefined' && module.exports) module.exports = api; else root.RiseDSP = api;
})(typeof window !== 'undefined' ? window : globalThis);
