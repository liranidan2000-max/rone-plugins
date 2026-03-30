import React, { useState, useEffect, useRef } from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import FormatBadge from './FormatBadge'
import ProgressBar from './ProgressBar'

const STATUS_CONFIG = {
  not_installed: {
    label: 'INSTALL',
    icon: 'download',
    className: 'bg-rone-purple hover:brightness-110',
  },
  update_available: {
    label: 'UPDATE',
    icon: 'download',
    className: 'bg-rone-pink hover:brightness-110',
  },
  up_to_date: {
    label: 'Installed',
    icon: 'check',
    className: 'bg-rone-green/15 text-rone-green cursor-default',
    disabled: true,
  },
  downloading: {
    label: 'DOWNLOADING...',
    icon: 'loading',
    className: 'bg-rone-purple/40 cursor-wait',
    disabled: true,
  },
  installing: {
    label: 'INSTALLING...',
    icon: 'loading',
    className: 'bg-rone-purple/40 cursor-wait',
    disabled: true,
  },
  error: {
    label: 'RETRY',
    icon: 'retry',
    className: 'bg-rone-error hover:brightness-110',
  },
}

// Variants for staggered entrance (driven by PluginGrid container)
export const cardVariants = {
  hidden: { opacity: 0, y: 20 },
  show: {
    opacity: 1,
    y: 0,
    transition: { duration: 0.4, ease: 'easeOut' },
  },
}

function ActionIcon({ type }) {
  if (type === 'download') {
    return (
      <svg className="w-3.5 h-3.5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
        <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2}
          d="M7 16a4 4 0 01-.88-7.903A5 5 0 1115.9 6L16 6a5 5 0 011 9.9M9 19l3 3m0 0l3-3m-3 3V10" />
      </svg>
    )
  }
  if (type === 'check') {
    return (
      <svg className="w-3.5 h-3.5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
        <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2.5} d="M5 13l4 4L19 7" />
      </svg>
    )
  }
  if (type === 'loading') {
    return (
      <svg className="w-3.5 h-3.5 animate-spin" fill="none" viewBox="0 0 24 24">
        <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4" />
        <path className="opacity-75" fill="currentColor"
          d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z" />
      </svg>
    )
  }
  if (type === 'retry') {
    return (
      <svg className="w-3.5 h-3.5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
        <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2}
          d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" />
      </svg>
    )
  }
  return null
}

export default function PluginCard({ plugin, licensed, onInstall, onOpen, onInfo, unlockPlaying = false }) {
  const [imgError, setImgError] = useState(false)
  const baseConfig = STATUS_CONFIG[plugin.status] || STATUS_CONFIG.not_installed

  // Track status transition for "just installed" pop effect
  const prevStatus = useRef(plugin.status)
  const [justInstalled, setJustInstalled] = useState(false)

  useEffect(() => {
    if (
      (prevStatus.current === 'downloading' || prevStatus.current === 'installing') &&
      plugin.status === 'up_to_date'
    ) {
      setJustInstalled(true)
      const timer = setTimeout(() => setJustInstalled(false), 1200)
      return () => clearTimeout(timer)
    }
    prevStatus.current = plugin.status
  }, [plugin.status])

  // Show INSTALL button when standalone is missing even if VST3/AU are installed
  const needsStandaloneInstall = plugin.status === 'up_to_date'
    && plugin.hasStandalone && !plugin.standaloneInstalled
  const config = needsStandaloneInstall ? STATUS_CONFIG.not_installed : baseConfig

  const isInstalled = plugin.status === 'up_to_date' || plugin.status === 'update_available'
  const showOpen = isInstalled && plugin.hasStandalone
  const showProgress = plugin.status === 'downloading'
  const isLocked = !licensed
  const isDownloading = plugin.status === 'downloading' || plugin.status === 'installing'

  // Version text
  let versionText = ''
  if (plugin.installedVersion && plugin.installedVersion !== '?')
    versionText = `v${plugin.installedVersion}`
  else if (plugin.remoteVersion)
    versionText = `v${plugin.remoteVersion}`

  if (plugin.status === 'update_available' && plugin.remoteVersion)
    versionText += ` \u2192 v${plugin.remoteVersion}`

  // Border color based on status
  const borderColor = {
    up_to_date: 'border-rone-green/20',
    update_available: 'border-rone-pink/25',
    downloading: 'border-rone-purple/35',
    error: 'border-rone-error/30',
  }[plugin.status] || 'border-rone-border/60'

  return (
    <motion.div
      className={`glass-card rounded-xl border ${borderColor} p-4 relative`}
      variants={cardVariants}
      whileHover={{
        scale: 1.02,
        y: -4,
        boxShadow: '0 8px 30px rgba(181,55,242,0.25)',
      }}
      transition={{ duration: 0.3, ease: 'easeOut' }}
      layout
    >
      {/* Lock overlay when unlicensed */}
      <AnimatePresence>
        {isLocked && !unlockPlaying && (
          <motion.div
            key="lock-overlay"
            initial={{ opacity: 1 }}
            exit={{ opacity: 0 }}
            transition={{ duration: 0.4 }}
            className="absolute inset-0 bg-rone-bg/60 rounded-xl z-10 flex items-center justify-center backdrop-blur-[1px]"
          >
            <motion.span
              className="bg-rone-error/12 border border-rone-error/20 text-rone-error text-[10px] font-bold px-3 py-1 rounded-md"
              exit={{ rotate: 15, opacity: 0, scale: 0.8 }}
              transition={{ duration: 0.3 }}
            >
              LOCKED
            </motion.span>
          </motion.div>
        )}
      </AnimatePresence>

      <div className="flex gap-3">
        {/* Logo */}
        <div className="flex-shrink-0 w-12 h-12 rounded-xl bg-rone-purple/8 flex items-center justify-center overflow-hidden">
          {!imgError ? (
            <img
              src={plugin.logoUrl}
              alt={plugin.name}
              className="w-10 h-10 object-contain"
              onError={() => setImgError(true)}
            />
          ) : (
            <span className="text-lg font-bold text-white/80">
              {plugin.name.substring(0, 2).toUpperCase()}
            </span>
          )}
        </div>

        {/* Content */}
        <div className="flex-1 min-w-0">
          <div className="flex items-start justify-between">
            <h3 className="text-sm font-semibold text-rone-text-primary truncate">
              {plugin.name}
            </h3>
            <button
              onClick={() => onInfo(plugin)}
              className="flex-shrink-0 ml-2 p-0.5 text-rone-text-dim hover:text-rone-purple transition-colors"
              title="Info"
            >
              <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2}
                  d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
              </svg>
            </button>
          </div>

          <p className="text-[11px] text-rone-text-secondary truncate mt-0.5">
            {plugin.description}
          </p>

          <p className="text-[10px] text-rone-text-dim mt-1">
            {versionText}
          </p>
        </div>
      </div>

      {/* Format badges */}
      <div className="flex gap-1.5 mt-2.5">
        {plugin.formats?.map(fmt => (
          <FormatBadge key={fmt} format={fmt} />
        ))}
      </div>

      {/* Progress bar */}
      {showProgress && (
        <div className="mt-2.5">
          <ProgressBar progress={plugin.downloadProgress} />
        </div>
      )}

      {/* Buttons */}
      <div className="flex items-center gap-2 mt-3">
        {/* Action button (Install/Update/Installed/Retry) */}
        <motion.button
          onClick={() => !config.disabled && !isLocked && onInstall(plugin.id)}
          disabled={config.disabled || isLocked}
          className={`flex items-center justify-center gap-1.5 px-3.5 py-1.5 rounded-lg text-[11px] font-bold
                      text-white transition-colors duration-200
                      disabled:cursor-default ${config.className}`}
          whileTap={!config.disabled && !isLocked ? { scale: 0.95 } : {}}
          animate={isDownloading ? {
            boxShadow: [
              '0 0 0px rgba(181,55,242,0)',
              '0 0 15px rgba(181,55,242,0.4)',
              '0 0 0px rgba(181,55,242,0)',
            ],
          } : {}}
          transition={isDownloading ? { repeat: Infinity, duration: 1.5, ease: 'easeInOut' } : {}}
        >
          <AnimatePresence mode="wait">
            <motion.span
              key={plugin.status + (justInstalled ? '-done' : '')}
              initial={{ opacity: 0, scale: 0.8 }}
              animate={{ opacity: 1, scale: justInstalled ? [1.3, 1] : 1 }}
              exit={{ opacity: 0, scale: 0.8 }}
              transition={justInstalled
                ? { type: 'spring', damping: 10, stiffness: 100 }
                : { duration: 0.2 }}
              className="flex items-center gap-1.5"
            >
              <ActionIcon type={justInstalled ? 'check' : config.icon} />
              {justInstalled ? 'INSTALLED' : config.label}
            </motion.span>
          </AnimatePresence>
        </motion.button>

        {/* Open button */}
        {showOpen && (
          <motion.button
            onClick={() => !isLocked && onOpen(plugin.id)}
            disabled={isLocked}
            className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg text-[11px] font-bold
                       bg-rone-button text-white border border-rone-border/30
                       hover:bg-rone-purple/30 hover:border-rone-purple/30
                       transition-colors duration-200
                       disabled:opacity-40"
            whileTap={!isLocked ? { scale: 0.95 } : {}}
          >
            <svg className="w-3 h-3" fill="currentColor" viewBox="0 0 24 24">
              <path d="M8 5v14l11-7z" />
            </svg>
            OPEN
          </motion.button>
        )}
      </div>
    </motion.div>
  )
}
