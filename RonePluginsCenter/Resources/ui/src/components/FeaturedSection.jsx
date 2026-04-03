import React from 'react'
import { motion } from 'framer-motion'
import FormatBadge from './FormatBadge'

export default function FeaturedSection({ plugins, onInstall, onUpdateAll, licensed }) {
  const updatable = plugins.filter(p => p.status === 'update_available')
  const notInstalled = plugins.filter(p => p.status === 'not_installed')
  const allUpToDate = plugins.length > 0 && updatable.length === 0 && notInstalled.length === 0

  // If everything is up to date, show a celebratory state
  if (allUpToDate) {
    return (
      <motion.div
        className="mx-4 mt-3 mb-1 rounded-xl border border-rone-green/15 p-4 hero-gradient"
        initial={{ opacity: 0, y: -10 }}
        animate={{ opacity: 1, y: 0 }}
        transition={{ duration: 0.4, ease: 'easeOut' }}
      >
        <div className="flex items-center gap-3">
          <div className="w-10 h-10 rounded-xl bg-rone-green/10 flex items-center justify-center">
            <svg className="w-5 h-5 text-rone-green" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M5 13l4 4L19 7" />
            </svg>
          </div>
          <div>
            <p className="text-sm font-semibold text-rone-text-primary">All plugins up to date</p>
            <p className="text-xs text-rone-text-dim mt-0.5">
              {plugins.length} plugin{plugins.length !== 1 ? 's' : ''} installed and ready to use
            </p>
          </div>
        </div>
      </motion.div>
    )
  }

  // Show updates available
  if (updatable.length > 0) {
    const featured = updatable[0]
    return (
      <motion.div
        className="mx-4 mt-3 mb-1 rounded-xl border border-rone-pink/20 p-4 hero-gradient overflow-hidden"
        initial={{ opacity: 0, y: -10 }}
        animate={{ opacity: 1, y: 0 }}
        transition={{ duration: 0.4, ease: 'easeOut' }}
      >
        <div className="flex items-center gap-4">
          {/* Featured plugin logo */}
          <div className="flex-shrink-0 w-14 h-14 rounded-xl bg-rone-purple/10 flex items-center justify-center overflow-hidden">
            <img
              src={featured.logoUrl}
              alt={featured.name}
              className="w-12 h-12 object-contain"
              onError={(e) => { e.target.style.display = 'none' }}
            />
          </div>

          {/* Info */}
          <div className="flex-1 min-w-0">
            <div className="flex items-center gap-2">
              <span className="text-xs font-bold text-rone-pink uppercase tracking-wider">
                {updatable.length} Update{updatable.length !== 1 ? 's' : ''} Available
              </span>
            </div>
            <p className="text-sm font-semibold text-rone-text-primary mt-0.5 truncate">
              {featured.name} v{featured.remoteVersion}
              {updatable.length > 1 && (
                <span className="text-rone-text-dim font-normal"> and {updatable.length - 1} more</span>
              )}
            </p>
            {featured.whatsNew && (
              <p className="text-[11px] text-rone-text-secondary mt-0.5 truncate">{featured.whatsNew}</p>
            )}
          </div>

          {/* Update All CTA */}
          <motion.button
            onClick={onUpdateAll}
            disabled={!licensed}
            className="flex-shrink-0 px-4 py-2 rounded-lg text-xs font-bold text-white
                       bg-rone-pink hover:brightness-110 transition-colors
                       disabled:opacity-40 disabled:cursor-not-allowed"
            whileTap={licensed ? { scale: 0.95 } : {}}
          >
            UPDATE ALL
          </motion.button>
        </div>
      </motion.div>
    )
  }

  // Show "new plugins available" if there are not-installed ones
  if (notInstalled.length > 0) {
    const featured = notInstalled[0]
    return (
      <motion.div
        className="mx-4 mt-3 mb-1 rounded-xl border border-rone-purple/20 p-4 hero-gradient overflow-hidden"
        initial={{ opacity: 0, y: -10 }}
        animate={{ opacity: 1, y: 0 }}
        transition={{ duration: 0.4, ease: 'easeOut' }}
      >
        <div className="flex items-center gap-4">
          <div className="flex-shrink-0 w-14 h-14 rounded-xl bg-rone-purple/10 flex items-center justify-center overflow-hidden">
            <img
              src={featured.logoUrl}
              alt={featured.name}
              className="w-12 h-12 object-contain"
              onError={(e) => { e.target.style.display = 'none' }}
            />
          </div>

          <div className="flex-1 min-w-0">
            <span className="text-xs font-bold text-rone-purple uppercase tracking-wider">
              New Plugin Available
            </span>
            <p className="text-sm font-semibold text-rone-text-primary mt-0.5 truncate">
              {featured.name}
            </p>
            <p className="text-[11px] text-rone-text-secondary mt-0.5 truncate">{featured.description}</p>
          </div>

          <motion.button
            onClick={() => onInstall(featured.id)}
            disabled={!licensed}
            className="flex-shrink-0 px-4 py-2 rounded-lg text-xs font-bold text-white
                       bg-rone-purple hover:brightness-110 transition-colors
                       disabled:opacity-40 disabled:cursor-not-allowed"
            whileTap={licensed ? { scale: 0.95 } : {}}
          >
            INSTALL
          </motion.button>
        </div>
      </motion.div>
    )
  }

  return null
}
