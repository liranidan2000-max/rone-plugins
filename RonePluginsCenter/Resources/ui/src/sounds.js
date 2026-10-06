// The one sound the Center makes, and only when Settings > Sound effects is on:
// a short, soft two-tone click when an install finishes - the "power on" of
// the card. Made with WebAudio, no file.
let ctx = null

export function playPowerOn () {
  try {
    const AC = window.AudioContext || window.webkitAudioContext
    if (!AC) return
    ctx = ctx || new AC()
    if (ctx.state === 'suspended') ctx.resume()
    const now = ctx.currentTime
    const out = ctx.createGain()
    out.gain.value = 0.12
    out.connect(ctx.destination)
    ;[[880, 0], [1320, 0.07]].forEach(([f, at]) => {
      const o = ctx.createOscillator()
      const g = ctx.createGain()
      o.type = 'triangle'
      o.frequency.value = f
      g.gain.setValueAtTime(0, now + at)
      g.gain.linearRampToValueAtTime(1, now + at + 0.006)
      g.gain.exponentialRampToValueAtTime(0.0008, now + at + 0.16)
      o.connect(g).connect(out)
      o.start(now + at)
      o.stop(now + at + 0.18)
    })
  } catch {}
}
