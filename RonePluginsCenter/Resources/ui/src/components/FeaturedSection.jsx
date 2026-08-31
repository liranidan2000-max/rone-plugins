import React from 'react'
import { motion } from 'framer-motion'

// Decorative particle-wave graphic shown on the right side of the banner
function WaveArt() {
  const dots = []
  const cols = 26, rows = 6
  for (let c = 0; c < cols; c++) {
    for (let r = 0; r < rows; r++) {
      const wave = Math.sin(c * 0.5) * 8
      const y = 14 + r * 9 + wave
      const op = Math.max(0.04, 0.4 - r * 0.06) * (0.35 + (c / cols) * 0.65)
      dots.push(<circle key={`${c}-${r}`} cx={6 + c * 7} cy={y} r={1.3} fill="#9D6BFF" opacity={op} />)
    }
  }
  return (
    <svg className="absolute right-0 top-0 h-full w-[52%] pointer-events-none" viewBox="0 0 190 96" preserveAspectRatio="xMaxYMid slice">
      {dots}
    </svg>
  )
}

function Chip({ label, value }) {
  return (
    <span className="px-2.5 py-1 rounded-lg bg-rone-card border border-rone-border
                     text-[9px] font-bold uppercase tracking-[0.16em] text-rone-text-dim tabular-nums">
      {label} <b className="ml-1 text-rone-text-secondary font-extrabold tracking-[0.06em]">{value}</b>
    </span>
  )
}

export default function FeaturedSection({ plugins, onUpdateAll, onRefresh, licensed }) {
  const updatable = plugins.filter(p => p.status === 'update_available')
  const notInstalled = plugins.filter(p => p.status === 'not_installed')
  const installed = plugins.filter(p => p.status === 'up_to_date' || p.status === 'update_available')
  const pending = updatable.length + notInstalled.length

  const allUpToDate = plugins.length > 0 && pending === 0

  const subtitle = allUpToDate
    ? `${installed.length} plugin${installed.length !== 1 ? 's' : ''} installed and ready to use`
    : `${updatable.length} to update${notInstalled.length ? ` · ${notInstalled.length} new to install` : ''}`

  return (
    <motion.div
      className="relative mx-6 mt-[18px] rounded-xl banner-gradient overflow-hidden"
      initial={{ opacity: 0, y: -10 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ duration: 0.4, ease: 'easeOut' }}
    >
      <WaveArt />
      <div className="relative flex items-center gap-[18px] px-[22px] py-5">
        {/* Status ring icon */}
        <div className="flex-shrink-0 w-[58px] h-[58px] rounded-full flex items-center justify-center
                        border-2 border-rone-purple/40 bg-rone-purple/5"
             style={{ boxShadow: '0 0 14px rgba(157,107,255,0.15)' }}>
          {allUpToDate ? (
            <svg className="w-6 h-6 text-rone-purple" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2.4} d="M5 13l4 4L19 7" />
            </svg>
          ) : (
            <svg className="w-6 h-6 text-rone-purple" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 4v9m0 0l-3.5-3.5M12 13l3.5-3.5M5 18h14" />
            </svg>
          )}
        </div>

        {/* Text */}
        <div className="flex-1 min-w-0">
          <h2 className="font-display text-[19px] font-bold text-rone-text-primary tracking-tight">
            {allUpToDate
              ? 'All plugins are up to date'
              : (<><b className="font-extrabold text-rone-purple">{pending} {pending === 1 ? 'update' : 'updates'}</b> available</>)}
          </h2>
          <p className="text-[12px] text-rone-text-secondary mt-0.5">{subtitle}</p>
          <div className="flex gap-2 mt-2.5">
            <Chip label="Plugins" value={plugins.length} />
            {updatable.length > 0 && <Chip label="Updates" value={updatable.length} />}
          </div>
        </div>

        {/* CTA */}
        {allUpToDate ? (
          <button
            onClick={onRefresh}
            className="btn-outline flex-shrink-0 flex items-center gap-2 px-5 py-[11px] rounded-lg
                       text-[11px] font-extrabold uppercase tracking-[0.18em]"
          >
            <svg className="w-3.5 h-3.5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2}
                d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" />
            </svg>
            Check for Updates
          </button>
        ) : (
          <motion.button
            onClick={onUpdateAll}
            disabled={!licensed || updatable.length === 0}
            whileTap={licensed ? { scale: 0.96 } : {}}
            className="btn-gradient flex-shrink-0 flex items-center gap-2 px-5 py-[11px] rounded-lg
                       text-[11px] font-extrabold uppercase tracking-[0.18em]
                       disabled:opacity-40 disabled:cursor-not-allowed"
          >
            <svg className="w-3.5 h-3.5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2.2} d="M12 4v9m0 0l-3.5-3.5M12 13l3.5-3.5M5 18h14" />
            </svg>
            {updatable.length > 0 ? 'Update All' : 'Install New'}
          </motion.button>
        )}
      </div>
    </motion.div>
  )
}
