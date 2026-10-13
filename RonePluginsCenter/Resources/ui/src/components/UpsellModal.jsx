import React, { useEffect, useRef, useState } from 'react'
import { motion } from 'framer-motion'
import { usd } from '../catalog'

// "One more for $19" (2026-10-13): after a purchase, once the bought plugin is installed, the Center shows the one
// plugin picked for it - its tutorial Short on the left, MUTED until the speaker is tapped (Liran: our Shorts, never
// the Instagram reels, and no sound by itself) - with the personal price and its clock. The button opens the signed
// roneaudio.com/owner link, where checkout applies the price. App.jsx decides when (once per offer).

const SITE = 'https://roneaudio.com'

function Title ({ text, accent }) {
  const m = /^(RONE)\s+(.+)$/.exec(text || '')
  if (!m) return <>{text}</>
  return <>{m[1]} <span style={{ color: accent, textShadow: `0 0 26px ${accent}66` }}>{m[2]}</span></>
}

function left (endsAt) {
  const ms = Date.parse(endsAt) - Date.now()
  if (!(ms > 0)) return '00:00:00'
  const pad = (n) => String(n).padStart(2, '0')
  return pad(Math.floor(ms / 3600000)) + ':' + pad(Math.floor(ms % 3600000 / 60000)) + ':' + pad(Math.floor(ms % 60000 / 1000))
}

export default function UpsellModal ({ t, offer, plugin, accent, reduced, onBuy, onClose }) {
  const video = useRef(null)
  const [muted, setMuted] = useState(true)
  const [clock, setClock] = useState(left(offer.endsAt))
  const [videoOk, setVideoOk] = useState(!!offer.video)

  useEffect(() => {
    const id = setInterval(() => setClock(left(offer.endsAt)), 1000)
    const onKey = (e) => { if (e.key === 'Escape') onClose() }
    window.addEventListener('keydown', onKey)
    return () => { clearInterval(id); window.removeEventListener('keydown', onKey) }
  }, [offer.endsAt, onClose])

  const toggleSound = () => {
    const v = video.current
    if (!v) return
    v.muted = !v.muted
    if (!v.muted) { try { v.currentTime = 0 } catch {} }   // they chose to watch: from the start, with sound
    v.play?.()?.catch?.(() => {})
    setMuted(v.muted)
  }

  return (
    <motion.div
      className="fixed inset-0 z-50 flex items-center justify-center bg-black/65 backdrop-blur-sm"
      role="dialog" aria-modal="true" aria-label={offer.name}
      onClick={(e) => { if (e.target === e.currentTarget) onClose() }}
      initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} transition={{ duration: 0.25 }}
    >
      <motion.div
        className="relative flex w-[680px] max-w-[92vw] rounded-2xl overflow-hidden border bg-[#101216]"
        style={{ borderColor: accent + '40', boxShadow: `0 40px 90px -34px ${accent}80, 0 24px 48px -24px rgba(0,0,0,0.85)` }}
        initial={{ opacity: 0, y: 18, scale: 0.97 }} animate={{ opacity: 1, y: 0, scale: 1 }}
        exit={{ opacity: 0, y: 12, scale: 0.98 }} transition={{ duration: 0.35, ease: 'easeOut' }}
      >
        {/* The Short (9:16), or the unit when the plugin has none yet */}
        <div className="relative w-[230px] flex-shrink-0 min-h-[408px] bg-black overflow-hidden border-r" style={{ borderColor: accent + '26' }}>
          {videoOk ? (
            <>
              <video ref={video} src={SITE + offer.video} poster={offer.poster ? SITE + offer.poster : undefined}
                     muted playsInline loop autoPlay={!reduced} preload="metadata" onError={() => setVideoOk(false)}
                     aria-label={offer.name + ' tutorial short'} className="absolute inset-0 w-full h-full object-cover" />
              <button onClick={toggleSound}
                      className="absolute left-3 bottom-3 px-3 py-1.5 rounded-full text-[11px] font-bold text-white bg-black/60 border border-white/30 focus:outline-none focus:ring-2 focus:ring-white/40">
                {muted ? '🔇 ' + t('up.tapSound') : '🔊 ' + t('up.soundOn')}
              </button>
            </>
          ) : plugin?.unitUrl ? (
            <div className="absolute inset-0" style={{ background: `radial-gradient(75% 55% at 50% 55%, ${accent}5c 0%, ${accent}1f 45%, transparent 80%), linear-gradient(180deg, #0E0F12 0%, #17191E 100%)` }}>
              <img src={plugin.unitUrl} alt="" draggable={false} className="absolute inset-x-0 mx-auto top-[30px] h-[calc(100%-60px)] max-w-[86%] object-contain"
                   style={{ filter: `drop-shadow(0 18px 24px rgba(0,0,0,0.6)) drop-shadow(0 0 22px ${accent}33)` }} />
            </div>
          ) : null}
        </div>

        <div className="flex-1 min-w-0 flex flex-col px-8 py-8">
          <div className="self-start flex items-center gap-2 pl-2.5 pr-3 py-1.5 rounded-full bg-white/[0.04] border border-white/10">
            <span className="w-[7px] h-[7px] rounded-full" style={{ background: accent, boxShadow: `0 0 10px ${accent}` }} />
            <span className="text-[10px] font-extrabold uppercase tracking-[0.2em] text-rone-text-primary">{t('hero.because', { name: offer.becauseName || '' })}</span>
          </div>
          <h2 className="mt-5 font-display text-[32px] leading-[1.1] font-extrabold tracking-tight text-rone-text-primary">
            <Title text={offer.name} accent={accent} />
          </h2>
          {offer.line && <p className="mt-3 text-[14px] leading-relaxed text-rone-text-secondary">{offer.line}.</p>}

          <div className="mt-5 flex items-baseline gap-3 num">
            <b className="font-display text-[30px]" style={{ color: accent }}>{usd(offer.price)}</b>
            <s className="text-[15px] text-rone-text-faint">{usd(offer.regular)}</s>
          </div>
          <div className="mt-1 text-[11px] font-bold uppercase tracking-[0.14em] text-rone-text-dim">
            {t('up.ends', { time: '' })}<span className="font-mono num normal-case tracking-normal" style={{ color: accent }}>{clock}</span>
          </div>

          <div className="flex-1 min-h-[16px]" />

          <div className="mt-6 flex items-center gap-3">
            <motion.button onClick={onBuy} whileTap={{ scale: 0.96 }} autoFocus
                           className="px-6 py-3 rounded-lg text-[11px] font-extrabold uppercase tracking-[0.18em] focus:outline-none focus:ring-2 focus:ring-white/30"
                           style={{ background: accent, color: '#0B0C0E', boxShadow: `0 12px 28px -12px ${accent}` }}>
              {t('hero.getIt', { price: usd(offer.price) })}
            </motion.button>
            <button onClick={onClose}
                    className="px-4 py-3 rounded-lg text-[11px] font-extrabold uppercase tracking-[0.18em] text-rone-text-dim border border-rone-border/60 hover:text-rone-text-primary transition-colors">
              {t('up.later')}
            </button>
          </div>
          <p className="mt-3 mb-0 text-[11.5px] text-rone-text-faint leading-relaxed">{t('up.note')}</p>
        </div>

        <button onClick={onClose} aria-label="Close"
                className="absolute right-4 top-4 p-1.5 rounded-lg text-rone-text-dim hover:text-rone-text-primary transition-colors focus:outline-none focus:ring-2 focus:ring-white/20">
          <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" />
          </svg>
        </button>
      </motion.div>
    </motion.div>
  )
}
