// The Center's two sounds (finish line, 22.11.2026 - Liran picked SWELL in docs/mockups/finish-line):
//
//   playSwell - the RONE sound: a reversed tail that sucks into an open Dmaj9 chord, the Reverse
//               Reverb DNA. Once per Center version, when the window comes up (App.jsx): the first
//               launch and the first open after an update. Settings > Startup sound turns it off.
//               The Center plays it (Resources/sounds/rone-swell.wav, MainComponent playRoneSound):
//               WebView2 holds the page's audio back until the first click.
//   playDone  - two soft glass notes with a short tail when an install finishes, only when
//               Settings > Sound effects is on (off by default: producers hate surprise sounds).
//               WebAudio: an install follows a click, so the page may play.
//
// Never late: the page schedules a sound only once its audio is running; a page that may not play
// yet answers false and queues nothing for the next click.
import { api } from './bridge'

export function playSwell () {
  return Promise.resolve(api.playSound()).then((ok) => ok === true).catch(() => false)
}

let ctx = null

function running (c) {
  if (c.state === 'running') return Promise.resolve(true)
  return new Promise((resolve) => {
    let settled = false
    const finish = (v) => { if (!settled) { settled = true; resolve(v) } }
    try { c.resume().then(() => finish(c.state === 'running'), () => finish(false)) } catch { finish(false) }
    setTimeout(() => finish(c.state === 'running'), 400)
  })
}

export function playDone () {
  try {
    const AC = window.AudioContext || window.webkitAudioContext
    if (!AC) return Promise.resolve(false)
    ctx = ctx || new AC()
    const c = ctx
    return running(c).then((ok) => { if (ok) done(c, c.currentTime + 0.03); return ok }).catch(() => false)
  } catch { return Promise.resolve(false) }
}

// ---- the voice (sonic.js "done") ----

function impulse (c, seconds, decay, tone) {
  const rate = c.sampleRate, len = Math.floor(rate * seconds)
  const buf = c.createBuffer(2, len, rate)
  for (let ch = 0; ch < 2; ch++) {
    const d = buf.getChannelData(ch)
    let lp = 0
    for (let i = 0; i < len; i++) {
      lp += tone * ((Math.random() * 2 - 1) - lp)   // one-pole lowpass = a darker tail
      d[i] = lp * Math.exp(-(i / rate) / decay) * (i < rate * 0.004 ? i / (rate * 0.004) : 1)
    }
  }
  return buf
}

function bus (c, wetAmt, decay) {
  const input = c.createGain()
  const wet = c.createGain(); wet.gain.value = wetAmt
  const verb = c.createConvolver(); verb.buffer = impulse(c, decay * 3.2, decay, 0.35)
  const hp = c.createBiquadFilter(); hp.type = 'highpass'; hp.frequency.value = 60
  const comp = c.createDynamicsCompressor()
  comp.threshold.value = -14; comp.ratio.value = 3; comp.attack.value = 0.004; comp.release.value = 0.2
  const out = c.createGain(); out.gain.value = 0.9
  input.connect(hp); input.connect(verb); verb.connect(wet); wet.connect(hp)
  hp.connect(comp); comp.connect(out); out.connect(c.destination)
  return input
}

function bell (c, dest, f, t, level, len) {
  const car = c.createOscillator(); car.type = 'sine'; car.frequency.value = f
  const mod = c.createOscillator(); mod.type = 'sine'; mod.frequency.value = f * 3.5
  const mg = c.createGain()
  mg.gain.setValueAtTime(f * 1.6, t); mg.gain.exponentialRampToValueAtTime(f * 0.05, t + len * 0.6)
  mod.connect(mg); mg.connect(car.frequency)
  const g = c.createGain()
  g.gain.setValueAtTime(0.0001, t)
  g.gain.exponentialRampToValueAtTime(level, t + 0.004)
  g.gain.exponentialRampToValueAtTime(0.0001, t + 0.004 + len)
  car.connect(g); g.connect(dest)
  car.start(t); mod.start(t); car.stop(t + len + 0.1); mod.stop(t + len + 0.1)
}

function done (c, t0) {
  const input = bus(c, 0.45, 0.6)
  bell(c, input, 880.0, t0, 0.09, 0.6)            // A5
  bell(c, input, 1318.51, t0 + 0.09, 0.08, 0.9)   // E6
}
