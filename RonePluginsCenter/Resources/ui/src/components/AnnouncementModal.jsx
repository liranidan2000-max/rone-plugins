import React, { useEffect, useState } from 'react'
import { motion } from 'framer-motion'
import FormatBadge from './FormatBadge'

// One of the website's popups (roneaudio.com/api/v1/popup), shown in the Center:
// the plugin's Hardware Series unit standing on its own neon (the units are tall,
// so the hero is a column, not a strip), the eyebrow ("New free plugin"), one line
// on what it does, and one button that does the thing here - Install now / Update
// now - or opens the site when it has to. App.jsx decides which popup, when, and
// what the button does.

const SITE = 'https://roneaudio.com/'

// "RONE Clipper" -> RONE + the product word in the accent (the one glowing word)
function Title({ text, accent }) {
  const m = /^(RONE)\s+(.+)$/.exec(text || '')
  if (!m) return <>{text}</>
  return <>{m[1]} <span style={{ color: accent, textShadow: `0 0 26px ${accent}66` }}>{m[2]}</span></>
}

export default function AnnouncementModal({ item, onPrimary, onClose }) {
  const { popup, plugin, primary, priceLine } = item
  const accent = /^#[0-9a-f]{6}$/i.test(popup.accent || '') ? popup.accent : '#9D6BFF'
  const image = popup.image
    ? (/^https:\/\//.test(popup.image) ? popup.image : SITE + String(popup.image).replace(/^\/+/, ''))
    : null
  const [imageOk, setImageOk] = useState(!!image)

  useEffect(() => {
    const onKey = (e) => { if (e.key === 'Escape') onClose() }
    window.addEventListener('keydown', onKey)
    return () => window.removeEventListener('keydown', onKey)
  }, [onClose])

  return (
    <motion.div
      className="fixed inset-0 z-50 flex items-center justify-center bg-black/65 backdrop-blur-sm"
      role="dialog" aria-modal="true" aria-label={popup.title}
      onClick={(e) => { if (e.target === e.currentTarget) onClose() }}
      initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} transition={{ duration: 0.25 }}
    >
      <motion.div
        className="relative flex w-[680px] max-w-[92vw] rounded-2xl overflow-hidden border bg-[#101216]"
        style={{ borderColor: accent + '40', boxShadow: `0 40px 90px -34px ${accent}80, 0 24px 48px -24px rgba(0,0,0,0.85)` }}
        initial={{ opacity: 0, y: 18, scale: 0.97 }} animate={{ opacity: 1, y: 0, scale: 1 }}
        exit={{ opacity: 0, y: 12, scale: 0.98 }} transition={{ duration: 0.35, ease: 'easeOut' }}
      >
        {/* Hero column: the unit on its neon */}
        <div
          className="relative w-[250px] flex-shrink-0 min-h-[360px] overflow-hidden border-r"
          style={{
            borderColor: accent + '26',
            background: `radial-gradient(75% 55% at 50% 55%, ${accent}5c 0%, ${accent}1f 45%, transparent 80%), linear-gradient(180deg, #0E0F12 0%, #17191E 100%)`,
          }}
        >
          <div
            className="absolute inset-0"
            style={{
              backgroundImage: 'linear-gradient(rgba(255,255,255,0.04) 1px, transparent 1px), linear-gradient(90deg, rgba(255,255,255,0.04) 1px, transparent 1px)',
              backgroundSize: '22px 22px',
              WebkitMaskImage: 'radial-gradient(ellipse at 50% 55%, #000 30%, transparent 75%)',
              maskImage: 'radial-gradient(ellipse at 50% 55%, #000 30%, transparent 75%)',
            }}
          />
          {/* the floor it stands on */}
          <div
            className="absolute left-0 right-0 mx-auto bottom-[20px] w-[170px] h-[22px] rounded-[50%]"
            style={{ background: 'radial-gradient(closest-side, rgba(0,0,0,0.75), transparent)' }}
          />
          {imageOk ? (
            <motion.img
              src={image} alt="" draggable={false} onError={() => setImageOk(false)}
              className="absolute left-0 right-0 mx-auto top-[22px] bottom-[28px] h-[calc(100%-50px)] max-w-[86%] object-contain select-none"
              style={{ filter: `drop-shadow(0 18px 24px rgba(0,0,0,0.6)) drop-shadow(0 0 22px ${accent}33)` }}
              initial={{ opacity: 0, y: 16 }} animate={{ opacity: 1, y: 0 }}
              transition={{ duration: 0.6, delay: 0.1, ease: 'easeOut' }}
            />
          ) : plugin?.logoUrl ? (
            <img src={plugin.logoUrl} alt="" className="absolute inset-0 m-auto w-24 h-24 rounded-2xl"
                 onError={(e) => { e.currentTarget.style.display = 'none' }} />
          ) : null}
        </div>

        {/* Words and the one action */}
        <div className="flex-1 min-w-0 flex flex-col px-8 py-8">
          <div className="self-start flex items-center gap-2 pl-2.5 pr-3 py-1.5 rounded-full bg-white/[0.04] border border-white/10">
            <span className="w-[7px] h-[7px] rounded-full" style={{ background: accent, boxShadow: `0 0 10px ${accent}` }} />
            <span className="text-[10px] font-extrabold uppercase tracking-[0.2em] text-rone-text-primary">
              {popup.eyebrow || 'New'}
            </span>
          </div>

          <h2 className="mt-5 font-display text-[32px] leading-[1.1] font-extrabold tracking-tight text-rone-text-primary">
            <Title text={popup.title} accent={accent} />
          </h2>
          {popup.body && (
            <p className="mt-3 text-[14px] leading-relaxed text-rone-text-secondary">{popup.body}</p>
          )}

          {(priceLine || plugin?.formats?.length > 0) && (
            <div className="mt-5 flex flex-wrap items-center gap-2">
              {priceLine && (
                <span className="w-full text-[11px] font-bold uppercase tracking-[0.14em] mb-1" style={{ color: accent }}>
                  {priceLine}
                </span>
              )}
              {plugin?.formats?.map((f) => <FormatBadge key={f} format={f} />)}
            </div>
          )}

          <div className="flex-1 min-h-[16px]" />

          <div className="mt-6 flex items-center gap-3">
            <motion.button
              onClick={onPrimary} whileTap={{ scale: 0.96 }} autoFocus
              className="px-6 py-3 rounded-lg text-[11px] font-extrabold uppercase tracking-[0.18em]
                         focus:outline-none focus:ring-2 focus:ring-white/30"
              style={{ background: accent, color: '#0B0C0E', boxShadow: `0 12px 28px -12px ${accent}` }}
            >
              {primary.label}
            </motion.button>
            <button
              onClick={onClose}
              className="px-4 py-3 rounded-lg text-[11px] font-extrabold uppercase tracking-[0.18em] text-rone-text-dim
                         border border-rone-border/60 hover:text-rone-text-primary transition-colors"
            >
              Later
            </button>
          </div>
        </div>

        <button
          onClick={onClose} aria-label="Close"
          className="absolute right-4 top-4 p-1.5 rounded-lg text-rone-text-dim hover:text-rone-text-primary
                     transition-colors focus:outline-none focus:ring-2 focus:ring-white/20"
        >
          <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" />
          </svg>
        </button>
      </motion.div>
    </motion.div>
  )
}
