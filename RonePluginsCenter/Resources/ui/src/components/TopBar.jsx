import React, { useEffect, useRef, useState } from 'react'
import { Icon, timeAgo } from './ui'

function initials (name) {
  if (!name) return ''
  const parts = name.trim().split(/\s+/)
  return (parts.length === 1 ? parts[0].substring(0, 2) : parts[0][0] + parts[parts.length - 1][0]).toUpperCase()
}

export default function TopBar ({ t, searchQuery, onSearchChange, onRefresh, manifest, account, license, daws, dawId, onPickDaw, onAccount, onSettings }) {
  const input = useRef(null)
  const [spinning, setSpinning] = useState(false)

  // Ctrl+K (Cmd+K) jumps to the search from anywhere; Esc clears it.
  useEffect(() => {
    const onKey = (e) => {
      if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'k') { e.preventDefault(); input.current?.focus(); input.current?.select() }
    }
    window.addEventListener('keydown', onKey)
    return () => window.removeEventListener('keydown', onKey)
  }, [])

  const refresh = () => { setSpinning(true); onRefresh(); setTimeout(() => setSpinning(false), 800) }
  const name = account.signedIn ? (account.name || account.email) : license.customerName
  const daw = daws.find(d => d.id === dawId) || daws[0]
  const offline = manifest.offline || (!manifest.loaded && manifest.syncedAt === 0 && manifest.error)

  return (
    <div className="flex-shrink-0 h-[60px] flex items-center gap-3 px-6 border-b border-rone-border">
      <label className="relative flex-1 max-w-[440px] flex items-center gap-2.5 h-[38px] pl-3 pr-2 rounded-[10px] border border-rone-border bg-rone-card focus-within:border-rone-purple/60">
        <Icon.search className="w-4 h-4 text-rone-text-faint flex-none" />
        <input ref={input} type="search" value={searchQuery} onChange={(e) => onSearchChange(e.target.value)}
               onKeyDown={(e) => { if (e.key === 'Escape') { onSearchChange(''); e.currentTarget.blur() } }}
               placeholder={t('top.search')} aria-label={t('top.searchAria')}
               className="flex-1 min-w-0 bg-transparent text-[13px] text-rone-text-primary placeholder:text-rone-text-faint outline-none" />
        <kbd className="font-mono text-[10.5px] text-rone-text-dim border border-rone-border-2 rounded-[5px] px-1.5 py-[1px] whitespace-nowrap">Ctrl K</kbd>
      </label>

      <div className="flex-1" />

      {/* The DAW the install steps are written for: found on this computer, or picked in Settings */}
      <button onClick={onPickDaw}
              className="flex items-center gap-2 h-[36px] px-3 rounded-[10px] border border-rone-border bg-rone-card text-[12.5px] font-bold text-rone-text-secondary hover:border-rone-border-3 whitespace-nowrap">
        <span className={`w-[7px] h-[7px] rounded-full ${daw ? 'bg-rone-green' : 'bg-rone-text-faint'}`}
              style={daw ? { boxShadow: '0 0 8px rgba(62,255,139,0.6)' } : undefined} />
        {daw ? daw.name : t('top.pickDaw')}
        {daws.length > 1 && <span className="text-rone-text-faint font-semibold">+{daws.length - 1}</span>}
      </button>

      <button onClick={refresh} title={t('top.refresh')} aria-label={t('top.refresh')}
              className="flex items-center gap-2 h-[36px] px-2.5 rounded-[10px] text-[12px] text-rone-text-dim hover:text-rone-text-primary whitespace-nowrap">
        {offline
          ? <Icon.offline className="w-4 h-4 text-rone-amber" />
          : <Icon.refresh className={`w-4 h-4 text-rone-green ${spinning ? 'animate-spin' : ''}`} />}
        <span className="num">
          {offline ? (manifest.syncedAt ? t('top.offlineSince', { ago: timeAgo(manifest.syncedAt, t) }) : t('top.offline'))
                   : manifest.syncedAt ? t('top.synced', { ago: timeAgo(manifest.syncedAt, t) }) : ''}
        </span>
      </button>

      {name ? (
        <button onClick={onAccount} title={name} aria-label={t('nav.account')}
                className="w-[38px] h-[38px] rounded-full border border-rone-border-2 grid place-items-center font-display text-[12px] font-bold text-rone-purple flex-none"
                style={{ background: 'radial-gradient(circle at 38% 30%, #363B42, #26292F 52%, #1A1C21)', boxShadow: '0 6px 14px rgba(0,0,0,0.4)' }}>
          {initials(name)}
        </button>
      ) : (
        <button onClick={onAccount} className="btn btn-pri">{t('top.signIn')}</button>
      )}
    </div>
  )
}
