import React, { useEffect, useRef } from 'react'
import { createPortal } from 'react-dom'

// ---- Icons (stroke icons drawn for the Center, currentColor) ----
const P = { fill: 'none', stroke: 'currentColor', strokeWidth: 1.9, strokeLinecap: 'round', strokeLinejoin: 'round' }
export const Icon = {
  library: (p) => <svg viewBox="0 0 24 24" {...P} {...p}><rect x="3" y="3" width="7" height="7" rx="1.5" /><rect x="14" y="3" width="7" height="7" rx="1.5" /><rect x="3" y="14" width="7" height="7" rx="1.5" /><rect x="14" y="14" width="7" height="7" rx="1.5" /></svg>,
  updates: (p) => <svg viewBox="0 0 24 24" {...P} {...p}><path d="M12 3v12m0 0l-4-4m4 4l4-4M4 17v2a2 2 0 002 2h12a2 2 0 002-2v-2" /></svg>,
  rack: (p) => <svg viewBox="0 0 24 24" {...P} {...p}><rect x="3" y="4" width="18" height="6" rx="1.5" /><rect x="3" y="14" width="18" height="6" rx="1.5" /><circle cx="7" cy="7" r=".9" fill="currentColor" /><circle cx="7" cy="17" r=".9" fill="currentColor" /></svg>,
  learn: (p) => <svg viewBox="0 0 24 24" {...P} {...p}><rect x="3" y="5" width="18" height="14" rx="2" /><path d="M10 9.5v5l4.5-2.5z" fill="currentColor" /></svg>,
  settings: (p) => <svg viewBox="0 0 24 24" {...P} {...p}><path d="M4 7h10M18 7h2M4 17h4M12 17h8" /><circle cx="16" cy="7" r="2" /><circle cx="10" cy="17" r="2" /></svg>,
  account: (p) => <svg viewBox="0 0 24 24" {...P} {...p}><circle cx="12" cy="8" r="3.5" /><path d="M5 20a7 7 0 0114 0" /></svg>,
  globe: (p) => <svg viewBox="0 0 24 24" {...P} {...p}><circle cx="12" cy="12" r="9" /><path d="M3 12h18M12 3a14 14 0 010 18M12 3a14 14 0 000 18" /></svg>,
  search: (p) => <svg viewBox="0 0 24 24" {...P} strokeWidth={2} {...p}><circle cx="11" cy="11" r="7" /><path d="M21 21l-5-5" /></svg>,
  check: (p) => <svg viewBox="0 0 24 24" {...P} strokeWidth={2.4} {...p}><path d="M5 13l4 4L19 7" /></svg>,
  checkCircle: (p) => <svg viewBox="0 0 24 24" {...P} {...p}><circle cx="12" cy="12" r="9" /><path d="M8.5 12.5l2.2 2.2 4.8-5" /></svg>,
  offline: (p) => <svg viewBox="0 0 24 24" {...P} {...p}><path d="M2 8.5a15 15 0 0120 0M5.5 12a10 10 0 0113 0M9 15.5a5 5 0 016 0" /><path d="M3 3l18 18" /></svg>,
  play: (p) => <svg viewBox="0 0 12 12" {...p}><path d="M3 1.6v8.8l7.4-4.4z" fill="currentColor" /></svg>,
  stop: (p) => <svg viewBox="0 0 12 12" {...p}><rect x="2.6" y="2.6" width="6.8" height="6.8" rx="1" fill="currentColor" /></svg>,
  download: (p) => <svg viewBox="0 0 24 24" {...P} strokeWidth={2.4} {...p}><path d="M12 4v10m0 0l-4-4m4 4l4-4M5 19h14" /></svg>,
  open: (p) => <svg viewBox="0 0 24 24" {...P} strokeWidth={2.4} {...p}><path d="M7 17L17 7M9 7h8v8" /></svg>,
  retry: (p) => <svg viewBox="0 0 24 24" {...P} strokeWidth={2.2} {...p}><path d="M4 4v5h5M20 20v-5h-5M5.1 15a8 8 0 0013.8 2.5M18.9 9A8 8 0 005.1 6.5" /></svg>,
  bolt: (p) => <svg viewBox="0 0 24 24" {...p}><path d="M13 2L4 14h6l-1 8 9-12h-6z" fill="currentColor" /></svg>,
  shield: (p) => <svg viewBox="0 0 24 24" {...P} {...p}><path d="M12 3l8 3v6c0 4.5-3.4 8-8 9-4.6-1-8-4.5-8-9V6z" /><path d="M8.5 12l2.5 2.5 4.5-5" /></svg>,
  back: (p) => <svg viewBox="0 0 24 24" {...P} strokeWidth={2.4} {...p}><path d="M15 6l-6 6 6 6" /></svg>,
  close: (p) => <svg viewBox="0 0 24 24" {...P} strokeWidth={2.2} {...p}><path d="M6 6l12 12M18 6L6 18" /></svg>,
  dots: (p) => <svg viewBox="0 0 24 24" {...p}><circle cx="12" cy="5" r="1.7" fill="currentColor" /><circle cx="12" cy="12" r="1.7" fill="currentColor" /><circle cx="12" cy="19" r="1.7" fill="currentColor" /></svg>,
  grid: (p) => <svg viewBox="0 0 24 24" {...P} strokeWidth={2} {...p}><rect x="4" y="4" width="6.5" height="6.5" rx="1" /><rect x="13.5" y="4" width="6.5" height="6.5" rx="1" /><rect x="4" y="13.5" width="6.5" height="6.5" rx="1" /><rect x="13.5" y="13.5" width="6.5" height="6.5" rx="1" /></svg>,
  pdf: (p) => <svg viewBox="0 0 24 24" {...P} {...p}><path d="M14 3H7a2 2 0 00-2 2v14a2 2 0 002 2h10a2 2 0 002-2V8z" /><path d="M14 3v5h5M9 13h6M9 17h4" /></svg>,
  folder: (p) => <svg viewBox="0 0 24 24" {...P} {...p}><path d="M3 7a2 2 0 012-2h4l2 2h8a2 2 0 012 2v8a2 2 0 01-2 2H5a2 2 0 01-2-2z" /></svg>,
  refresh: (p) => <svg viewBox="0 0 24 24" {...P} strokeWidth={2} {...p}><path d="M4 4v5h5M20 20v-5h-5M5.1 15a8 8 0 0013.8 2.5M18.9 9A8 8 0 005.1 6.5" /></svg>,
}

// R mark (canonical path, memory r-glyph-header-logo)
export function RGlyph ({ color = '#9D6BFF', className = '' }) {
  return (
    <svg className={className} viewBox="0 0 100 52" aria-hidden="true">
      <path d="M 0 0 L 4.9 0 L 4.9 16.4 L 0 19.2 Z" fill={color} />
      <path d="M 0 2.1 H 46.7 A 10.5 10.5 0 0 1 46.7 23.1 H 11.6 A 6.9 6.9 0 0 0 11.6 36.9 H 34 C 46 36.9 47 48.2 59 48.2 H 100" fill="none" stroke={color} strokeWidth="4.2" />
    </svg>
  )
}

// ---- Switch: a real switch for screen readers (role="switch", aria-checked) ----
export function Switch ({ checked, onChange, label, disabled = false }) {
  return (
    <button
      type="button" role="switch" aria-checked={!!checked} aria-label={label} disabled={disabled}
      onClick={() => onChange(!checked)}
      className={`relative w-[42px] h-[24px] rounded-full flex-none transition-colors disabled:opacity-50 ${checked ? 'bg-rone-purple' : 'bg-rone-border-2'}`}
    >
      <span className={`absolute top-[3px] w-[18px] h-[18px] rounded-full bg-white transition-all ${checked ? 'left-[21px]' : 'left-[3px]'}`} />
    </button>
  )
}

// ---- Segmented choice ----
export function Seg ({ value, options, onChange, label }) {
  return (
    <div role="radiogroup" aria-label={label} className="inline-flex border border-rone-border-2 rounded-[9px] p-[2px] gap-[2px]">
      {options.map(([v, text]) => (
        <button key={v} type="button" role="radio" aria-checked={value === v} onClick={() => onChange(v)}
                className={`px-3 h-[28px] rounded-[7px] text-[12px] font-bold transition-colors ${value === v ? 'bg-rone-purple text-rone-neon-dark' : 'text-rone-text-dim hover:text-rone-text-primary'}`}>
          {text}
        </button>
      ))}
    </div>
  )
}

// ---- Dialog: role="dialog", focus moves in and stays in, Esc closes, focus returns ----
export function Dialog ({ open, onClose, label, children, width = 420, closeOnBackdrop = true }) {
  const box = useRef(null)
  const returnTo = useRef(null)

  useEffect(() => {
    if (!open) return
    returnTo.current = document.activeElement
    const t = setTimeout(() => {
      const first = box.current?.querySelector('[data-autofocus], button, [href], input, select, textarea')
      first?.focus()
    }, 30)
    const onKey = (e) => {
      if (e.key === 'Escape') { e.stopPropagation(); onClose?.() }
      if (e.key === 'Tab' && box.current) {
        const items = [...box.current.querySelectorAll('button, [href], input, select, textarea, [tabindex]:not([tabindex="-1"])')]
          .filter(el => !el.disabled && el.offsetParent !== null)
        if (!items.length) return
        const first = items[0], last = items[items.length - 1]
        if (e.shiftKey && document.activeElement === first) { e.preventDefault(); last.focus() }
        else if (!e.shiftKey && document.activeElement === last) { e.preventDefault(); first.focus() }
      }
    }
    window.addEventListener('keydown', onKey, true)
    return () => {
      clearTimeout(t)
      window.removeEventListener('keydown', onKey, true)
      try { returnTo.current?.focus?.() } catch {}
    }
  }, [open, onClose])

  if (!open) return null
  return createPortal(
    <div className="fixed inset-0 z-[60] flex items-center justify-center confirm-backdrop"
         onMouseDown={(e) => { if (closeOnBackdrop && e.target === e.currentTarget) onClose?.() }}>
      <div ref={box} role="dialog" aria-modal="true" aria-label={label}
           className="glass-card border border-rone-border-2 rounded-2xl p-5 max-w-[92vw] max-h-[86vh] overflow-y-auto scroll-y shadow-2xl"
           style={{ width }}>
        {children}
      </div>
    </div>,
    document.body
  )
}

// ---- A menu that floats over the page, opens upward when there is no room,
// and works with the keyboard (arrows, Enter, Esc). The root CSS zoom (text
// size) is taken into account when it is positioned. ----
export function useFloatingMenu () {
  const anchor = useRef(null)
  const [state, setState] = React.useState(null)   // { top, right, up } | null

  const open = () => {
    const r = anchor.current?.getBoundingClientRect()
    if (!r) return
    const zoom = parseFloat(getComputedStyle(document.documentElement).zoom) || 1
    setState({ top: r.bottom / zoom + 6, right: (window.innerWidth - r.right) / zoom, anchorTop: r.top / zoom, up: false, zoom })
  }
  const close = () => setState(null)
  return { anchor, state, setState, open, close, isOpen: !!state }
}

export function FloatingMenu ({ menu, items, label }) {
  const ref = useRef(null)
  useEffect(() => {
    if (!menu.state) return
    const el = ref.current
    if (el && !menu.state.up) {
      const h = el.offsetHeight
      const vh = window.innerHeight / (menu.state.zoom || 1)
      if (menu.state.top + h > vh - 8) menu.setState(s => s && ({ ...s, top: Math.max(8, s.anchorTop - 6 - h), up: true }))
    }
    el?.querySelector('[role="menuitem"]')?.focus()
    const onDown = (e) => { if (!el?.contains(e.target) && !menu.anchor.current?.contains(e.target)) menu.close() }
    const onScroll = () => menu.close()
    window.addEventListener('mousedown', onDown)
    window.addEventListener('resize', onScroll)
    window.addEventListener('scroll', onScroll, true)
    return () => {
      window.removeEventListener('mousedown', onDown)
      window.removeEventListener('resize', onScroll)
      window.removeEventListener('scroll', onScroll, true)
    }
  }, [!!menu.state, menu.state?.up])

  if (!menu.state) return null
  const onKey = (e) => {
    const all = [...ref.current.querySelectorAll('[role="menuitem"]')]
    const i = all.indexOf(document.activeElement)
    if (e.key === 'ArrowDown') { e.preventDefault(); all[(i + 1) % all.length]?.focus() }
    if (e.key === 'ArrowUp') { e.preventDefault(); all[(i - 1 + all.length) % all.length]?.focus() }
    if (e.key === 'Escape' || e.key === 'Tab') { e.preventDefault(); menu.close(); menu.anchor.current?.focus() }
  }
  return createPortal(
    <div ref={ref} role="menu" aria-label={label} onKeyDown={onKey}
         style={{ position: 'fixed', top: menu.state.top, right: menu.state.right }}
         className="z-[70] min-w-[180px] rounded-xl border border-rone-border-2 bg-rone-drawer shadow-xl shadow-black/50 py-1">
      {items.filter(Boolean).map((it, i) => it === '-' ? (
        <div key={i} className="my-1 border-t border-rone-border" />
      ) : (
        <button key={it.label} role="menuitem" type="button"
                onClick={() => { menu.close(); it.onSelect() }}
                className={`w-full flex items-center gap-2.5 text-left px-3 py-2 text-[12.5px] font-semibold focus:bg-white/[0.06] hover:bg-white/[0.05]
                            ${it.danger ? 'text-rone-error hover:text-rone-error' : 'text-rone-text-secondary hover:text-rone-text-primary'}`}>
          {it.icon && <span className="w-4 h-4 flex-none opacity-80">{it.icon}</span>}
          {it.label}
        </button>
      ))}
    </div>,
    document.body
  )
}

// "3 min ago" in the page's language
export function timeAgo (ms, t) {
  if (!ms) return ''
  const s = Math.max(0, Math.floor((Date.now() - ms) / 1000))
  if (s < 60) return t('ago.now')
  const m = Math.floor(s / 60)
  if (m < 60) return t('ago.m', { n: m })
  const h = Math.floor(m / 60)
  if (h < 24) return t('ago.h', { n: h })
  return t('ago.d', { n: Math.floor(h / 24) })
}
