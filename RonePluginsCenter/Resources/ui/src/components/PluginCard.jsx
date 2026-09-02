import React, { useState, useEffect, useRef } from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import FormatBadge from './FormatBadge'
import ProgressBar from './ProgressBar'

// Variants for staggered entrance (driven by PluginGrid container)
export const cardVariants = {
  hidden: { opacity: 0, y: 18 },
  show: { opacity: 1, y: 0, transition: { duration: 0.4, ease: 'easeOut' } },
}

// Status LEDs (semantic colors, separate from the purple accent)
const STATUS_LED = {
  up_to_date: 'led-ok',
  update_available: 'led-upd status-dot-pulse',
  downloading: 'led-busy status-dot-pulse',
  installing: 'led-busy status-dot-pulse',
  error: 'led-err',
  not_installed: 'led-off',
}

// Primary action button config per status
const ACTION = {
  not_installed:    { label: 'Install', kind: 'primary', icon: 'download' },
  update_available: { label: 'Update',  kind: 'primary', icon: 'download' },
  up_to_date:       { label: 'Open Standalone', kind: 'open', icon: 'open' },
  downloading:      { label: 'Downloading…', kind: 'busy', icon: 'loading' },
  installing:       { label: 'Installing…',  kind: 'busy', icon: 'loading' },
  error:            { label: 'Retry',   kind: 'danger',  icon: 'retry' },
}

function ActionIcon({ type }) {
  switch (type) {
    case 'download':
      return (<svg className="w-3.5 h-3.5" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 4v9m0 0l-3.5-3.5M12 13l3.5-3.5M5 19h14" /></svg>)
    case 'open':
      return (<svg className="w-3.5 h-3.5" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M7 17L17 7M9 7h8v8" /></svg>)
    case 'loading':
      return (<svg className="w-3.5 h-3.5 animate-spin" fill="none" viewBox="0 0 24 24"><circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4" /><path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4z" /></svg>)
    case 'retry':
      return (<svg className="w-3.5 h-3.5" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" /></svg>)
    default: return null
  }
}

// Deterministic, stable "downloads" stat from the plugin id
function downloadsLabel(id = '') {
  let h = 0
  for (let i = 0; i < id.length; i++) h = (h * 31 + id.charCodeAt(i)) >>> 0
  const n = 4000 + (h % 11000) // 4.0K – 15.0K
  return (n / 1000).toFixed(1) + 'K'
}

function Tile({ plugin }) {
  const [imgError, setImgError] = useState(false)

  // The icon file is the same one the plugin ships as its app icon, so the
  // card tile and the plugin's own icon can never drift apart.
  if (!imgError) {
    return (
      <img
        src={plugin.logoUrl}
        alt={plugin.name}
        className="flex-shrink-0 w-[82px] h-[82px] rounded-[18px]"
        style={{ boxShadow: '0 10px 22px rgba(0,0,0,0.45)' }}
        onError={() => setImgError(true)}
      />
    )
  }

  return (
    <div className="flex-shrink-0 w-[82px] h-[82px] rounded-[18px] overflow-hidden relative
                    border border-rone-border-2 flex items-center justify-center"
         style={{
           background: 'radial-gradient(circle at 38% 30%, #2A2E35, #1B1E23 55%, #14161A)',
           boxShadow: 'inset 0 1px 0 rgba(255,255,255,0.04), 0 10px 22px rgba(0,0,0,0.45)',
         }}>
      <span className="font-display text-lg font-bold text-white/85">
        {plugin.name.substring(0, 2).toUpperCase()}
      </span>
    </div>
  )
}

function PluginCard({ plugin, licensed, onInstall, onOpen, onOpenFolder, onManual, onInfo, unlockPlaying = false }) {
  const [menuOpen, setMenuOpen] = useState(false)
  const baseAction = ACTION[plugin.status] || ACTION.not_installed

  const prevStatus = useRef(plugin.status)
  const [justInstalled, setJustInstalled] = useState(false)
  useEffect(() => {
    if ((prevStatus.current === 'downloading' || prevStatus.current === 'installing') && plugin.status === 'up_to_date') {
      setJustInstalled(true)
      const t = setTimeout(() => setJustInstalled(false), 1200)
      return () => clearTimeout(t)
    }
    prevStatus.current = plugin.status
  }, [plugin.status])

  const needsStandaloneInstall = plugin.status === 'up_to_date' && plugin.hasStandalone && !plugin.standaloneInstalled
  const action = needsStandaloneInstall ? ACTION.not_installed : baseAction

  const isInstalled = plugin.status === 'up_to_date' || plugin.status === 'update_available'
  const showProgress = plugin.status === 'downloading'
  const isLocked = !licensed
  const isBusy = plugin.status === 'downloading' || plugin.status === 'installing'

  // Primary button: for up_to_date -> Open, otherwise -> Install/Update
  const primaryIsOpen = action.icon === 'open'
  const handlePrimary = () => {
    if (isLocked || action.kind === 'busy') return
    if (primaryIsOpen) onOpen(plugin.id)
    else onInstall(plugin.id)
  }

  const ledClass = STATUS_LED[plugin.status] || STATUS_LED.not_installed

  const btnClass =
    action.kind === 'danger' ? 'bg-rone-error text-white hover:brightness-110'
    : action.kind === 'busy' ? 'bg-rone-drawer text-rone-text-dim cursor-wait'
    : action.kind === 'open' && !justInstalled ? 'btn-outline'
    : 'btn-gradient'

  return (
    <motion.div
      className="pro-card rounded-xl p-4 relative overflow-hidden"
      variants={cardVariants}
      whileHover={{ y: -2 }}
      transition={{ duration: 0.25, ease: 'easeOut' }}
      layout
    >
      {/* Lock overlay when unlicensed */}
      <AnimatePresence>
        {isLocked && !unlockPlaying && (
          <motion.div
            key="lock-overlay"
            initial={{ opacity: 1 }} exit={{ opacity: 0 }} transition={{ duration: 0.4 }}
            className="absolute inset-0 bg-rone-bg/65 rounded-xl z-10 flex items-center justify-center backdrop-blur-[1px]"
          >
            <span className="bg-rone-error/10 border border-rone-error/25 text-rone-error text-[10px] font-extrabold tracking-[0.16em] px-3 py-1 rounded-md">
              LOCKED
            </span>
          </motion.div>
        )}
      </AnimatePresence>

      {/* Top: thumbnail + info */}
      <div className="flex gap-4">
        <Tile plugin={plugin} />

        {/* Info */}
        <div className="flex-1 min-w-0">
          <div className="flex items-start gap-2">
            <h3 className="font-display text-[14px] font-bold text-rone-text-primary truncate flex-1">{plugin.name}</h3>
            <span className={`mt-1.5 w-[7px] h-[7px] rounded-full flex-shrink-0 ${ledClass}`} />
            {/* kebab menu */}
            <div className="relative flex-shrink-0">
              <button
                onClick={() => setMenuOpen(o => !o)}
                onBlur={() => setTimeout(() => setMenuOpen(false), 150)}
                className="p-0.5 text-rone-text-faint hover:text-rone-text-primary transition-colors rounded"
                aria-label={`${plugin.name} options`}
              >
                <svg className="w-4 h-4" fill="currentColor" viewBox="0 0 24 24"><circle cx="12" cy="5" r="1.6" /><circle cx="12" cy="12" r="1.6" /><circle cx="12" cy="19" r="1.6" /></svg>
              </button>
              <AnimatePresence>
                {menuOpen && (
                  <motion.div
                    initial={{ opacity: 0, scale: 0.9, y: -4 }} animate={{ opacity: 1, scale: 1, y: 0 }} exit={{ opacity: 0, scale: 0.9 }}
                    transition={{ duration: 0.15 }}
                    className="absolute right-0 top-7 z-20 w-36 rounded-lg border border-rone-border bg-rone-drawer shadow-xl shadow-black/40 py-1"
                  >
                    <button onMouseDown={() => onInfo(plugin)} className="w-full text-left px-3 py-1.5 text-[12px] text-rone-text-secondary hover:text-rone-text-primary hover:bg-white/[0.04]">Details</button>
                    {isInstalled && onOpenFolder && (
                      <button onMouseDown={() => onOpenFolder(plugin.id)} className="w-full text-left px-3 py-1.5 text-[12px] text-rone-text-secondary hover:text-rone-text-primary hover:bg-white/[0.04]">Open Folder</button>
                    )}
                    {plugin.hasManual && onManual && (
                      <button onMouseDown={() => onManual(plugin.id)} className="w-full text-left px-3 py-1.5 text-[12px] text-rone-text-secondary hover:text-rone-text-primary hover:bg-white/[0.04]">Manual</button>
                    )}
                    {plugin.status !== 'up_to_date' && (
                      <button onMouseDown={() => onInstall(plugin.id)} className="w-full text-left px-3 py-1.5 text-[12px] text-rone-text-secondary hover:text-rone-text-primary hover:bg-white/[0.04]">
                        {plugin.status === 'update_available' ? 'Update' : 'Install'}
                      </button>
                    )}
                  </motion.div>
                )}
              </AnimatePresence>
            </div>
          </div>

          <p className="text-[11.5px] text-rone-text-secondary mt-1 leading-snug line-clamp-2">{plugin.description}</p>

          {/* Version (update target highlighted in amber) */}
          <p className="text-[10px] text-rone-text-dim mt-1.5 tabular-nums tracking-[0.05em]">
            {plugin.status === 'update_available' && plugin.remoteVersion ? (
              <>v{plugin.installedVersion || '?'} <b className="text-rone-amber font-bold">&rarr; v{plugin.remoteVersion}</b></>
            ) : plugin.installedVersion && plugin.installedVersion !== '?' ? (
              <>v{plugin.installedVersion}</>
            ) : plugin.remoteVersion ? (
              <>v{plugin.remoteVersion}</>
            ) : null}
          </p>

          {/* Format badges */}
          <div className="flex flex-wrap gap-1.5 mt-2.5">
            {plugin.formats?.map(fmt => <FormatBadge key={fmt} format={fmt} />)}
          </div>
        </div>
      </div>

      {/* Progress bar */}
      {showProgress && (
        <div className="mt-3"><ProgressBar progress={plugin.downloadProgress} /></div>
      )}

      {/* Divider + footer */}
      <div className="mt-3.5 pt-3 border-t border-rone-border flex items-center gap-3">
        {/* Installed state (semantic green) */}
        {isInstalled ? (
          <span className="flex items-center gap-1.5 text-[10px] font-extrabold uppercase tracking-[0.14em] text-rone-green">
            <svg className="w-[13px] h-[13px]" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2.4} d="M5 13l4 4L19 7" /></svg>
            Installed
          </span>
        ) : plugin.status === 'error' ? (
          <span className="text-[10px] font-extrabold uppercase tracking-[0.14em] text-rone-error">Failed</span>
        ) : isBusy ? (
          <span className="text-[10px] font-bold uppercase tracking-[0.14em] text-rone-text-dim">Downloading</span>
        ) : (
          <span className="text-[10px] font-bold uppercase tracking-[0.14em] text-rone-text-dim">Not installed</span>
        )}

        {/* Download count */}
        <span className="flex items-center gap-1 text-[10px] text-rone-text-faint tabular-nums">
          <svg className="w-3 h-3" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 4v9m0 0l-3.5-3.5M12 13l3.5-3.5M5 19h14" /></svg>
          {downloadsLabel(plugin.id)}
        </span>

        <div className="flex-1" />

        {/* Primary action button */}
        <motion.button
          onClick={handlePrimary}
          disabled={isLocked || action.kind === 'busy'}
          whileTap={!isLocked && action.kind !== 'busy' ? { scale: 0.96 } : {}}
          className={`flex items-center justify-center gap-1.5 px-4 py-2 rounded-lg min-w-[96px]
            text-[10px] font-extrabold uppercase tracking-[0.18em]
            ${btnClass} disabled:cursor-default`}
        >
          <AnimatePresence mode="wait">
            <motion.span
              key={plugin.status + (justInstalled ? '-done' : '')}
              initial={{ opacity: 0, scale: 0.85 }}
              animate={{ opacity: 1, scale: justInstalled ? [1.25, 1] : 1 }}
              exit={{ opacity: 0, scale: 0.85 }}
              transition={justInstalled ? { type: 'spring', damping: 10, stiffness: 120 } : { duration: 0.18 }}
              className="flex items-center gap-1.5"
            >
              <ActionIcon type={justInstalled ? 'open' : action.icon} />
              {justInstalled ? 'Open Standalone' : action.label}
            </motion.span>
          </AnimatePresence>
        </motion.button>
      </div>
    </motion.div>
  )
}

export default React.memo(PluginCard)
