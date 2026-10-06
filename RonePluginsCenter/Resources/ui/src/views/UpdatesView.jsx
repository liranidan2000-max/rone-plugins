import React from 'react'
import { Icon } from '../components/ui'
import { accentOf, changesSince, isBusy, isUnlocked, shortName } from '../catalog'
import { cardAction } from '../components/PluginCard'

// What changes, before you update: every update with its changelog, newest first.
export default function UpdatesView ({ t, plugins, access, onInstall, onUpdateAll, onCancel, onRefresh, onDetail }) {
  const list = plugins.filter(p => isUnlocked(p, access) && (p.status === 'update_available' || (isBusy(p) && p.installedVersion)))
  const ready = list.filter(p => p.status === 'update_available')

  return (
    <div className="max-w-[920px]">
      <div className="flex items-end justify-between gap-4 mb-4">
        <div>
          <h1 className="m-0 font-display font-extrabold text-[24px] text-rone-text-primary">{t('updates.title')}</h1>
          <p className="m-0 mt-1 text-[13px] text-rone-text-dim">{list.length ? t('updates.sub') : t('updates.none')}</p>
        </div>
        {ready.length > 0 ? (
          <button className="btn btn-pri" style={{ '--acc': '#FFD02B' }} onClick={onUpdateAll}>
            <Icon.download className="w-3.5 h-3.5" />{t('updates.all', { n: ready.length })}
          </button>
        ) : (
          <button className="btn btn-out" onClick={onRefresh}><Icon.refresh className="w-3.5 h-3.5" />{t('updates.check')}</button>
        )}
      </div>

      {list.map(p => {
        const a = cardAction(p, access, t)
        const changes = changesSince(p)
        return (
          <div key={p.id} className="grid grid-cols-[56px_1fr_auto] gap-4 items-start p-4 mb-2.5 rounded-[14px] border border-rone-border bg-rone-card"
               style={{ '--acc': accentOf(p) }}>
            <img src={p.logoUrl} alt="" className="w-14 h-14 rounded-[14px]" />
            <div className="min-w-0">
              <h2 className="m-0 font-display text-[16px] font-bold text-rone-text-primary">
                <button className="hover:underline" onClick={() => onDetail(p.id)}>RONE {shortName(p)}</button>
                <span className="ml-2 font-mono text-[12px] font-medium text-rone-text-dim">
                  v{(p.installedVersion || '?').split('.').slice(0, 3).join('.')} <b className="text-rone-amber font-semibold">→ {(p.remoteVersion || '').split('.').slice(0, 3).join('.')}</b>
                </span>
              </h2>
              {changes.map(c => (
                <div key={c.version} className="mt-2">
                  {c.version && <div className="text-[11px] font-extrabold tracking-[0.1em] text-rone-text-faint">v{c.version}</div>}
                  <ul className="m-0 mt-1 pl-4 text-[12.5px] text-rone-text-secondary space-y-1 marker:text-[var(--acc)] list-disc">
                    {c.items.map((it, i) => <li key={i}>{it}</li>)}
                  </ul>
                </div>
              ))}
              {(p.status === 'downloading' || p.status === 'installing' || p.status === 'queued' || p.status === 'ready') && (
                <div className={`prog mt-3 ${p.status !== 'downloading' ? 'indet' : ''}`}><i style={{ width: `${Math.round((p.downloadProgress || 0) * 100)}%` }} /></div>
              )}
            </div>
            <div className="flex flex-col items-end gap-1.5">
              {a.primary ? (
                <button className="btn btn-pri btn-sm" style={{ '--acc': '#FFD02B' }} onClick={() => onInstall(p.id)}>
                  <Icon.download className="w-3 h-3" />{t('card.update')}
                </button>
              ) : (
                <span className="text-[12px] font-bold text-rone-text-secondary text-right max-w-[180px]">{a.status}</span>
              )}
              {a.cancel && <button className="btn btn-ghost btn-sm" onClick={() => onCancel(p.id)}>{t('card.cancel')}</button>}
              {p.downloaded && p.status === 'update_available' && <span className="text-[11px] text-rone-green">{t('card.downloaded')}</span>}
            </div>
          </div>
        )
      })}

      {!list.length && (
        <div className="p-12 rounded-[14px] border border-dashed border-rone-border-2 text-center text-rone-text-dim">
          <Icon.checkCircle className="w-8 h-8 mx-auto mb-2 text-rone-green" />
          {t('updates.none')}
        </div>
      )}
    </div>
  )
}
