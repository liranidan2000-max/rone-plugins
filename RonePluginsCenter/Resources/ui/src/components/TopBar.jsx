import React, { useState } from 'react'
import { motion } from 'framer-motion'

function initials(name) {
  if (!name) return 'U'
  const parts = name.trim().split(/\s+/)
  if (parts.length === 1) return parts[0].substring(0, 2).toUpperCase()
  return (parts[0][0] + parts[parts.length - 1][0]).toUpperCase()
}

function timeAgo(date) {
  if (!date) return null
  const seconds = Math.floor((Date.now() - date.getTime()) / 1000)
  if (seconds < 60) return 'Just now'
  const minutes = Math.floor(seconds / 60)
  if (minutes < 60) return `${minutes}m ago`
  const hours = Math.floor(minutes / 60)
  if (hours < 24) return `${hours}h ago`
  return `${Math.floor(hours / 24)}d ago`
}

export default function TopBar({
  license, searchQuery, onSearchChange, onRefresh, lastSync, onSettings,
}) {
  const [spinning, setSpinning] = useState(false)
  const handleRefresh = () => {
    setSpinning(true)
    onRefresh()
    setTimeout(() => setSpinning(false), 700)
  }

  return (
    <div className="flex-shrink-0 h-[62px] flex items-center gap-4 px-6 border-b border-rone-border">
      {/* Avatar + licensed-to */}
      <div className="flex items-center gap-3 min-w-0">
        <div className="w-[38px] h-[38px] rounded-full border border-rone-border-2
                        flex items-center justify-center font-display text-[12px] font-bold text-rone-purple"
             style={{
               background: 'radial-gradient(circle at 38% 30%, #363B42, #26292F 52%, #1A1C21)',
               boxShadow: '0 6px 14px rgba(0,0,0,0.4)',
             }}>
          {initials(license?.customerName)}
        </div>
        <div className="min-w-0">
          <p className="text-[9px] font-bold uppercase tracking-[0.22em] text-rone-text-faint leading-tight">Licensed to</p>
          <div className="flex items-center gap-2 mt-0.5">
            <p className="text-[13px] font-bold text-rone-text-primary truncate max-w-[160px]">
              {license?.customerName || 'Guest'}
            </p>
            {license?.licensed && (
              <span className="px-2 py-0.5 text-[8px] font-extrabold tracking-[0.18em] rounded
                               bg-rone-purple/[0.07] text-rone-purple border border-rone-purple/35">
                PRO
              </span>
            )}
          </div>
        </div>
      </div>

      <div className="flex-1" />

      {/* Synced status (semantic green) */}
      {lastSync && (
        <div className="hidden sm:flex items-center gap-2">
          <svg className="w-[15px] h-[15px] text-rone-green" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <circle cx="12" cy="12" r="9" strokeWidth={1.8} />
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M8.5 12.5l2.2 2.2 4.8-5" />
          </svg>
          <div className="leading-tight">
            <p className="text-[10px] font-bold uppercase tracking-[0.14em] text-rone-text-secondary">Synced</p>
            <p className="text-[10px] text-rone-text-dim tabular-nums">{timeAgo(lastSync)}</p>
          </div>
        </div>
      )}

      {/* Search */}
      <div className="relative">
        <svg className="absolute left-3 top-1/2 -translate-y-1/2 w-3.5 h-3.5 text-rone-text-dim"
             fill="none" stroke="currentColor" viewBox="0 0 24 24">
          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2}
            d="M21 21l-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z" />
        </svg>
        <input
          type="text"
          placeholder="Search plugins..."
          value={searchQuery}
          onChange={(e) => onSearchChange(e.target.value)}
          className="w-56 pl-9 pr-3 py-2 text-[12px] bg-rone-card border border-rone-border rounded-lg
                     text-rone-text-primary placeholder:text-rone-text-dim
                     focus:outline-none focus:border-rone-purple/50 transition-colors"
        />
      </div>

      {/* Refresh */}
      <button
        onClick={handleRefresh}
        className="w-[34px] h-[34px] flex items-center justify-center rounded-lg border border-rone-border bg-rone-card
                   text-rone-text-secondary hover:border-rone-purple/40 hover:text-rone-purple transition-all active:scale-95"
        title="Refresh"
        aria-label="Refresh plugin list"
      >
        <motion.svg className="w-[15px] h-[15px]" fill="none" stroke="currentColor" viewBox="0 0 24 24"
          animate={spinning ? { rotate: 360 } : { rotate: 0 }} transition={{ duration: 0.7, ease: 'easeInOut' }}>
          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2}
            d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" />
        </motion.svg>
      </button>

      {/* Settings */}
      <button
        onClick={onSettings}
        className="w-[34px] h-[34px] flex items-center justify-center rounded-lg border border-rone-border bg-rone-card
                   text-rone-text-secondary hover:border-rone-purple/40 hover:text-rone-purple transition-all active:scale-95"
        title="Settings"
        aria-label="Settings"
      >
        <svg className="w-[15px] h-[15px]" fill="none" stroke="currentColor" viewBox="0 0 24 24">
          <circle cx="12" cy="12" r="3" strokeWidth={1.8} />
          <path strokeWidth={1.8} strokeLinecap="round" d="M12 2v2.5M12 19.5V22M4.2 4.2l1.8 1.8M18 18l1.8 1.8M2 12h2.5M19.5 12H22M4.2 19.8L6 18M18 6l1.8-1.8" />
        </svg>
      </button>
    </div>
  )
}
