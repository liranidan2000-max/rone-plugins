import React, { useEffect } from 'react'
import { motion } from 'framer-motion'
import FormatBadge from './FormatBadge'

export default function InfoModal({ plugin, onClose }) {
  // Close on Escape
  useEffect(() => {
    const handleKey = (e) => {
      if (e.key === 'Escape') onClose()
    }
    window.addEventListener('keydown', handleKey)
    return () => window.removeEventListener('keydown', handleKey)
  }, [onClose])

  return (
    <motion.div
      className="fixed inset-0 z-50 flex items-center justify-center bg-black/60 backdrop-blur-sm"
      role="dialog"
      aria-modal="true"
      aria-label={`${plugin.name} details`}
      onClick={(e) => { if (e.target === e.currentTarget) onClose() }}
      initial={{ opacity: 0 }}
      animate={{ opacity: 1 }}
      exit={{ opacity: 0 }}
      transition={{ duration: 0.25 }}
    >
      <motion.div
        className="glass-card border border-rone-border/60 rounded-2xl p-6 w-[420px] max-w-[90vw] max-h-[80vh] overflow-y-auto
                    shadow-2xl shadow-rone-purple/10"
        initial={{ opacity: 0, y: 15, scale: 0.97 }}
        animate={{ opacity: 1, y: 0, scale: 1 }}
        exit={{ opacity: 0, y: 15, scale: 0.97 }}
        transition={{ duration: 0.3, ease: 'easeOut' }}
      >
        {/* Header */}
        <div className="flex items-start justify-between mb-4">
          <div className="flex items-center gap-3">
            <img
              src={plugin.logoUrl}
              alt={plugin.name}
              className="w-12 h-12 rounded-xl"
              onError={(e) => { e.target.style.display = 'none' }}
            />
            <div>
              <h2 className="text-lg font-bold text-rone-text-primary">{plugin.name}</h2>
              <p className="text-xs text-rone-text-dim">v{plugin.remoteVersion}</p>
            </div>
          </div>
          <button
            onClick={onClose}
            className="p-1 text-rone-text-dim hover:text-rone-text-primary transition-colors
                       focus:outline-none focus:ring-2 focus:ring-rone-purple/50 rounded"
            aria-label="Close dialog"
          >
            <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" />
            </svg>
          </button>
        </div>

        {/* Description */}
        <p className="text-sm text-rone-text-secondary mb-4">{plugin.description}</p>

        {/* What's New */}
        {plugin.whatsNew && (
          <div className="mb-4">
            <h3 className="text-xs font-bold text-rone-purple uppercase tracking-wider mb-1.5">
              What's New
            </h3>
            <p className="text-sm text-rone-text-secondary bg-rone-bg/40 rounded-lg p-3 border border-rone-border/30">
              {plugin.whatsNew}
            </p>
          </div>
        )}

        {/* Formats */}
        <div className="mb-4">
          <h3 className="text-xs font-bold text-rone-purple uppercase tracking-wider mb-1.5">
            Formats
          </h3>
          <div className="flex gap-2">
            {plugin.formats?.map(fmt => (
              <FormatBadge key={fmt} format={fmt} />
            ))}
          </div>
        </div>

        {/* Details */}
        <div className="space-y-1.5 text-xs text-rone-text-dim">
          <div className="flex justify-between">
            <span>Type</span>
            <span className="text-rone-text-secondary capitalize">{plugin.type}</span>
          </div>
          {plugin.installedVersion && (
            <div className="flex justify-between">
              <span>Installed</span>
              <span className="text-rone-text-secondary">v{plugin.installedVersion}</span>
            </div>
          )}
          <div className="flex justify-between">
            <span>Latest</span>
            <span className="text-rone-text-secondary">v{plugin.remoteVersion}</span>
          </div>
          <div className="flex justify-between">
            <span>Status</span>
            <span className={`capitalize ${
              plugin.status === 'up_to_date' ? 'text-rone-green' :
              plugin.status === 'update_available' ? 'text-rone-pink' :
              plugin.status === 'error' ? 'text-rone-error' :
              'text-rone-text-secondary'
            }`}>
              {plugin.status.replace(/_/g, ' ')}
            </span>
          </div>
        </div>
      </motion.div>
    </motion.div>
  )
}
