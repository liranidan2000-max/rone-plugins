import React, { useState } from 'react'
import { Icon } from './ui'
import { cardAction } from './PluginCard'
import { descriptionOf } from '../catalog'

// RONE VST MANAGER (29.10.2026): RONE's own app, a gift for every customer. It
// comes from the manifest's "apps" and gets this one wide card above the
// library, never a library card - nothing here is for sale. App.jsx shows it
// only when the backend says this account is a customer (`unlocked`).
export default function GiftAppCard ({ t, app, onInstall, onCancel, onOpen }) {
  // A customer can always act on it: the card's footer rules with the lock taken off
  const a = cardAction(app, { licensed: true, signedIn: true }, t)
  const act = (kind) => (kind === 'open' ? onOpen(app.id) : onInstall(app.id))
  const v = (s) => (s || '').split('.').slice(0, 3).join('.')
  const [broken, setBroken] = useState(false)
  const busy = ['downloading', 'installing', 'queued', 'ready'].includes(app.status)

  return (
    <section className="mt-3.5 relative overflow-hidden flex items-center gap-4 pl-4 pr-3 py-3 rounded-[13px] border border-white/[0.14]"
             style={{ background: 'linear-gradient(90deg, rgba(255,255,255,.07) 0%, rgb(var(--card)) 55%)' }}
             data-card={app.id} aria-label={app.name}>
      {app.logoUrl && !broken
        ? <img src={app.logoUrl} alt="" width={48} height={48} onError={() => setBroken(true)}
               className="flex-none w-12 h-12 rounded-[12px]" style={{ boxShadow: '0 8px 18px rgba(0,0,0,.45)' }} />
        : <div className="flex-none w-12 h-12 rounded-[12px] grid place-items-center border border-white/15 text-white"
               style={{ background: 'radial-gradient(circle at 38% 30%, #2A2E35, #1B1E23 55%, #14161A)', boxShadow: '0 8px 18px rgba(0,0,0,.45)' }}>
            <Icon.gift className="w-6 h-6" />
          </div>}

      <div className="flex-1 min-w-0 flex flex-col gap-1">
        <div className="flex items-center gap-2 flex-wrap">
          <h3 className="m-0 font-display font-bold text-[15px] text-rone-text-primary truncate">{app.name}</h3>
          <span className="font-mono text-[11px] text-rone-text-dim">v{v(app.remoteVersion)}</span>
          <span className="pill pill-free">{t('gift.free')}</span>
          <span className="text-[10.5px] font-extrabold uppercase tracking-[0.12em] text-rone-text-faint">{t('gift.badge')}</span>
        </div>
        <p className="m-0 text-[12.5px] leading-[1.45] text-rone-text-secondary line-clamp-2">{descriptionOf(app)}</p>
        {busy && (
          <div className={`prog mt-1 ${app.status !== 'downloading' ? 'indet' : ''}`} aria-hidden="true">
            <i style={{ width: `${Math.round((app.downloadProgress || 0) * 100)}%` }} />
          </div>
        )}
      </div>

      <div className="flex-none flex items-center gap-2">
        {a.version ? (
          <span className="font-mono text-[11.5px] text-rone-text-dim whitespace-nowrap">
            v{v(app.installedVersion) || '?'} <b className="text-rone-amber font-semibold">→ {v(app.remoteVersion)}</b>
          </span>
        ) : a.status && (
          <span className={`text-[11px] font-extrabold uppercase tracking-[0.08em] whitespace-nowrap
                            ${a.ok ? 'text-rone-green' : a.error ? 'text-rone-error' : a.busy ? 'text-rone-text-secondary normal-case tracking-normal text-[12px] font-bold' : 'text-rone-text-faint'}`}>
            {a.ok && '✓ '}{a.status}
            {app.status === 'downloading' && app.downloadProgress > 0 && <span className="num"> · {Math.round(app.downloadProgress * 100)}%</span>}
          </span>
        )}
        {a.cancel && <button className="btn btn-ghost btn-sm" onClick={() => onCancel(app.id)}>{t('card.cancel')}</button>}
        {a.secondary && (
          <button className="btn btn-pri btn-white btn-sm" onClick={() => act(a.secondary.kind)}>
            {a.secondary.icon && <a.secondary.icon className="w-3 h-3" />}{a.secondary.label}
          </button>
        )}
        {a.primary && (
          <button className="btn btn-pri btn-white btn-sm" onClick={() => act(a.primary.kind)}>
            {a.primary.icon && <a.primary.icon className="w-3 h-3" />}{a.primary.label}
          </button>
        )}
      </div>
    </section>
  )
}
