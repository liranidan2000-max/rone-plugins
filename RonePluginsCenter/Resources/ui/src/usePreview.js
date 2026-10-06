// ---- Audio previews: the same loop dry and with the plugin ----
// Two audio elements play the dry and the wet file in lockstep; DRY/WET only
// changes which one is heard, so the switch is instant and the bar stays the
// same bar. Both run through one AnalyserNode, and the visualizers draw what
// is really playing (the files are served with CORS by roneaudio.com).
import { useCallback, useEffect, useRef, useState } from 'react'

let shared = null   // one engine for the whole page

function engine () {
  if (shared) return shared
  const listeners = new Set()
  const state = { id: null, wet: true, status: 'idle' }   // idle | loading | playing | error
  let ctx = null, analyser = null
  let dry = null, wet = null, dryGain = null, wetGain = null

  const emit = () => listeners.forEach(fn => fn({ ...state }))

  function ensureGraph () {
    if (ctx) return
    const AC = window.AudioContext || window.webkitAudioContext
    if (!AC) return
    ctx = new AC()
    analyser = ctx.createAnalyser()
    analyser.fftSize = 256
    analyser.smoothingTimeConstant = 0.78
    analyser.connect(ctx.destination)
  }

  function makeEl (url) {
    const el = new Audio()
    el.crossOrigin = 'anonymous'
    el.preload = 'auto'
    el.loop = true
    el.src = url
    return el
  }

  function stop () {
    for (const el of [dry, wet]) { if (el) { el.pause(); el.removeAttribute('src'); el.load() } }
    try { dryGain?.disconnect(); wetGain?.disconnect() } catch {}
    dryGain = wetGain = null
    dry = wet = null
    state.id = null
    state.status = 'idle'
    emit()
  }

  async function play (plugin) {
    if (!plugin?.preview?.dry || !plugin?.preview?.wet) return
    stop()
    ensureGraph()
    state.id = plugin.id
    state.status = 'loading'
    emit()

    dry = makeEl(plugin.preview.dry)
    wet = makeEl(plugin.preview.wet)
    const fail = () => { if (state.id === plugin.id) { stop(); state.status = 'error'; emit() } }
    dry.onerror = fail
    wet.onerror = fail

    try {
      if (ctx) {
        if (ctx.state === 'suspended') await ctx.resume()
        dryGain = ctx.createGain(); wetGain = ctx.createGain()
        ctx.createMediaElementSource(dry).connect(dryGain).connect(analyser)
        ctx.createMediaElementSource(wet).connect(wetGain).connect(analyser)
        dryGain.gain.value = state.wet ? 0 : 1
        wetGain.gain.value = state.wet ? 1 : 0
      } else {
        dry.volume = state.wet ? 0 : 1
        wet.volume = state.wet ? 1 : 0
      }
      await Promise.all([dry.play(), wet.play()])
      // Same start, same loop: keep them together
      wet.currentTime = dry.currentTime
      if (state.id === plugin.id) { state.status = 'playing'; emit() }
    } catch { fail() }
  }

  function setWet (on) {
    state.wet = !!on
    if (dryGain && wetGain && ctx) {
      const t = ctx.currentTime
      dryGain.gain.setTargetAtTime(on ? 0 : 1, t, 0.015)
      wetGain.gain.setTargetAtTime(on ? 1 : 0, t, 0.015)
    } else if (dry && wet) {
      dry.volume = on ? 0 : 1
      wet.volume = on ? 1 : 0
    }
    if (dry && wet && Math.abs(dry.currentTime - wet.currentTime) > 0.05) wet.currentTime = dry.currentTime
    emit()
  }

  // Levels 0..1 for `bands` bars; null when nothing plays.
  function levels (bands) {
    if (!analyser || state.status !== 'playing') return null
    const data = new Uint8Array(analyser.frequencyBinCount)
    analyser.getByteFrequencyData(data)
    const out = new Array(bands).fill(0)
    const usable = Math.floor(data.length * 0.8)
    for (let b = 0; b < bands; b++) {
      // log-ish spread: more bars for the low end
      const from = Math.floor(Math.pow(b / bands, 1.8) * usable)
      const to = Math.max(from + 1, Math.floor(Math.pow((b + 1) / bands, 1.8) * usable))
      let sum = 0
      for (let i = from; i < to; i++) sum += data[i]
      out[b] = sum / (to - from) / 255
    }
    return out
  }

  shared = {
    play, stop, setWet, levels,
    get state () { return { ...state } },
    subscribe (fn) { listeners.add(fn); return () => listeners.delete(fn) },
  }
  return shared
}

export function usePreview () {
  const e = engine()
  const [state, setState] = useState(e.state)
  useEffect(() => e.subscribe(setState), [e])
  const toggle = useCallback((plugin) => {
    if (state.id === plugin?.id && state.status !== 'error') e.stop()
    else e.play(plugin)
  }, [state.id, state.status, e])
  return { ...state, toggle, stop: e.stop, setWet: e.setWet, levels: e.levels }
}

// Draws live bars into a canvas while something plays; flat idle bars otherwise.
export function useBars (canvasRef, { color, bands = 24, active = true, reduced = false }) {
  const e = engine()
  const smooth = useRef(new Array(bands).fill(0.06))
  useEffect(() => {
    let raf = 0
    const draw = () => {
      const c = canvasRef.current
      if (c) {
        const g = c.getContext('2d')
        const w = c.width, h = c.height
        g.clearRect(0, 0, w, h)
        const lv = active ? e.levels(bands) : null
        const bw = w / bands
        for (let i = 0; i < bands; i++) {
          const target = lv ? Math.min(1, lv[i] * 1.25) : 0.06
          smooth.current[i] += (target - smooth.current[i]) * (reduced ? 1 : 0.35)
          const bh = Math.max(3, smooth.current[i] * h)
          const grd = g.createLinearGradient(0, h - bh, 0, h)
          grd.addColorStop(0, lv ? '#FFFFFF' : color)
          grd.addColorStop(0.35, color)
          grd.addColorStop(1, 'rgba(0,0,0,0)')
          g.globalAlpha = lv ? 0.95 : 0.35
          g.fillStyle = grd
          g.fillRect(i * bw + bw * 0.22, h - bh, bw * 0.56, bh)
        }
        g.globalAlpha = 1
      }
      raf = requestAnimationFrame(draw)
    }
    raf = requestAnimationFrame(draw)
    return () => cancelAnimationFrame(raf)
  }, [canvasRef, color, bands, active, reduced, e])
}
