// ---- The page's own choices (language, text size, contrast, motion, sounds,
// the DAW to show steps for, whether the welcome was seen) ----
// Kept in the WebView's storage, which lives in AppData and survives restarts
// and updates. The backend's choices (background downloads, start with the OS)
// live in the Center itself - see bridge.js getCenterSettings.
import { useCallback, useEffect, useState } from 'react'
import { defaultLang } from './i18n.js'

const KEY = 'rone_center_prefs_v2'

export const DEFAULT_PREFS = {
  lang: null,          // null = follow the system on first launch
  textSize: 'm',       // s | m | l
  contrast: false,
  reduceMotion: false,
  sounds: false,       // off by default: producers hate surprise sounds
  daw: null,           // the DAW whose steps are shown; null = the first one found
  onboarded: false,
}

export function loadPrefs () {
  try {
    const raw = JSON.parse(localStorage.getItem(KEY) || '{}')
    const p = { ...DEFAULT_PREFS, ...(raw && typeof raw === 'object' ? raw : {}) }
    if (!['s', 'm', 'l'].includes(p.textSize)) p.textSize = 'm'
    if (!p.lang) p.lang = defaultLang()
    return p
  } catch { return { ...DEFAULT_PREFS, lang: defaultLang() } }
}

export function usePrefs () {
  const [prefs, setPrefs] = useState(loadPrefs)
  const set = useCallback((patch) => {
    setPrefs(prev => {
      const next = { ...prev, ...patch }
      try { localStorage.setItem(KEY, JSON.stringify(next)) } catch {}
      return next
    })
  }, [])

  // Applied to the whole document: size scales every pixel of the page
  // (CSS zoom), contrast and motion switch tokens in index.css.
  useEffect(() => {
    const root = document.documentElement
    root.style.zoom = prefs.textSize === 's' ? '0.92' : prefs.textSize === 'l' ? '1.12' : ''
    root.classList.toggle('hc', !!prefs.contrast)
    root.classList.toggle('rm', !!prefs.reduceMotion)
    root.lang = prefs.lang || 'en'
  }, [prefs.textSize, prefs.contrast, prefs.reduceMotion, prefs.lang])

  return [prefs, set]
}

// Reduced motion: the user's choice here, or the system's.
export function prefersReducedMotion (prefs) {
  if (prefs?.reduceMotion) return true
  try { return window.matchMedia('(prefers-reduced-motion: reduce)').matches } catch { return false }
}
