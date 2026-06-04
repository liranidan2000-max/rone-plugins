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
      const op = Math.max(0.05, 0.5 - r * 0.07) * (0.4 + (c / cols) * 0.6)
      dots.push(<circle key={`${c}-${r}`} cx={6 + c * 7} cy={y} r={1.3} fill="#A855F7" opacity={op} />)
    }
  }
  return (
    <svg className="absolute right-0 top-0 h-full w-[55%] pointer-events-none" viewBox="0 0 190 96" preserveAspectRatio="xMaxYMid slice">
      {dots}
    </svg>
  )
}

export default function FeaturedSection({ plugins, onUpdateAll, onRefresh, licensed }) {
  const updatable = plugins.filter(p => p.status === 'update_available')
  const notInstalled = plugins.filter(p => p.status === 'not_installed')
  const installed = plugins.filter(p => p.status === 'up_to_date' || p.status === 'update_available')
  const pending = updatable.length + notInstalled.length

  const allUpToDate = plugins.length > 0 && pending === 0

  const title = allUpToDate
    ? 'All plugins are up to date'
    : `${pending} ${pending === 1 ? 'update' : 'updates'} available`
  const subtitle = allUpToDate
    ? `${installed.length} plugin${installed.length !== 1 ? 's' : ''} installed and ready to use`
    : `${updatable.length} to update${notInstalled.length ? ` · ${notInstalled.length} new to install` : ''}`

  return (
    <motion.div
      className="relative mx-6 mt-5 rounded-2xl banner-gradient overflow-hidden"
      initial={{ opacity: 0, y: -10 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ duration: 0.4, ease: 'easeOut' }}
    >
      <WaveArt />
      <div className="relative flex items-center gap-5 px-6 py-6">
        {/* Status ring icon */}
        <div className="flex-shrink-0 w-16 h-16 rounded-full flex items-center justify-center
                        border-2 border-rone-purple/40 bg-rone-purple/5">
          {allUpToDate ? (
            <svg className="w-7 h-7 text-rone-purple" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2.4} d="M5 13l4 4L19 7" />
            </svg>
          ) : (
            <svg className="w-7 h-7 text-rone-purple" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 4v9m0 0l-3.5-3.5M12 13l3.5-3.5M5 18h14" />
            </svg>
          )}
        </div>

        {/* Text */}
        <div className="flex-1 min-w-0">
          <h2 className="text-[19px] font-bold text-rone-text-primary tracking-tight">{title}</h2>
          <p className="text-[13px] text-rone-text-secondary mt-1">{subtitle}</p>
        </div>

        {/* CTA */}
        {allUpToDate ? (
          <button
            onClick={onRefresh}
            className="flex-shrink-0 flex items-center gap-2 px-5 py-2.5 rounded-xl text-[13px] font-semibold
                       text-rone-text-primary border border-rone-border/80 bg-rone-surface-2
                       hover:bg-rone-purple/10 hover:border-rone-purple/40 transition-all"
          >
            <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
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
            className="flex-shrink-0 flex items-center gap-2 px-5 py-2.5 rounded-xl text-[13px] font-bold text-white
                       btn-gradient disabled:opacity-40 disabled:cursor-not-allowed"
          >
            <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 4v9m0 0l-3.5-3.5M12 13l3.5-3.5M5 18h14" />
            </svg>
            {updatable.length > 0 ? 'Update All' : 'Install New'}
          </motion.button>
        )}
      </div>
    </motion.div>
  )
}
