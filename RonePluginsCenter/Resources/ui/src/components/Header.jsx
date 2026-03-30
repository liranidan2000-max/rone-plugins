import React, { useState } from 'react'
import { motion } from 'framer-motion'

export default function Header({ updatesCount, searchQuery, onSearchChange, onRefresh }) {
  const [spinning, setSpinning] = useState(false)

  const handleRefresh = () => {
    setSpinning(true)
    onRefresh()
    setTimeout(() => setSpinning(false), 600)
  }

  return (
    <div className="flex-shrink-0 h-12 bg-rone-header border-b border-rone-border/40 flex items-center px-5 gap-4">
      {/* Title */}
      <h1 className="text-rone-purple font-bold text-base tracking-wide whitespace-nowrap">
        RONE PLUGINS CENTER
      </h1>

      {/* Spacer */}
      <div className="flex-1" />

      {/* Status pill */}
      {updatesCount > 0 && (
        <div className="flex items-center gap-2 bg-rone-green/10 border border-rone-green/20 rounded-full px-3 py-1">
          <motion.span
            className="w-2 h-2 rounded-full bg-rone-green"
            animate={{ opacity: [0.4, 1, 0.4] }}
            transition={{ repeat: Infinity, duration: 2, ease: 'easeInOut' }}
          />
          <span className="text-rone-green text-xs font-medium">
            {updatesCount} update{updatesCount !== 1 ? 's' : ''} available
          </span>
        </div>
      )}

      {/* Search */}
      <div className="relative">
        <svg
          className="absolute left-2.5 top-1/2 -translate-y-1/2 w-3.5 h-3.5 text-rone-text-dim"
          fill="none" stroke="currentColor" viewBox="0 0 24 24"
        >
          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2}
            d="M21 21l-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z" />
        </svg>
        <input
          type="text"
          placeholder="Search..."
          value={searchQuery}
          onChange={(e) => onSearchChange(e.target.value)}
          className="w-44 pl-8 pr-3 py-1.5 text-xs bg-rone-bg/80 border border-rone-border/50 rounded-lg
                     text-rone-text-primary placeholder:text-rone-text-dim
                     focus:outline-none focus:border-rone-purple/50 transition-colors"
        />
      </div>

      {/* Refresh */}
      <button
        onClick={handleRefresh}
        className="p-1.5 rounded-lg border border-rone-border/50 bg-rone-button/50
                   hover:bg-rone-purple/20 hover:border-rone-purple/30
                   transition-all duration-200 active:scale-95"
        title="Refresh"
      >
        <motion.svg
          className="w-4 h-4 text-rone-text-secondary"
          fill="none"
          stroke="currentColor"
          viewBox="0 0 24 24"
          animate={spinning ? { rotate: 360 } : { rotate: 0 }}
          transition={{ duration: 0.6, ease: 'easeInOut' }}
        >
          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2}
            d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" />
        </motion.svg>
      </button>
    </div>
  )
}
