import React from 'react'
import { Icon } from './ui'
import { accentOf, shortName } from '../catalog'

// One queue for everything in flight, at the bottom of the page: what runs now,
// what follows, the promise of one permission prompt for the batch, and Cancel.
const ORDER = { installing: 0, downloading: 1, waiting: 2, ready: 3, queued: 4 }

export default function InstallDock ({ t, plugins, platform, onCancelAll }) {
  const active = plugins.filter(p => ORDER[p.status] !== undefined)
                        .sort((a, b) => ORDER[a.status] - ORDER[b.status])
  if (!active.length) return null

  const head = active[0]
  const rest = active.slice(1)
  const name = 'RONE ' + shortName(head)
  const line = head.status === 'installing' ? t('dock.installing', { name })
             : head.status === 'downloading' ? t('dock.downloading', { name })
             : head.status === 'waiting' ? t('dock.waiting', { name, host: head.waitingFor || name })
             : head.status === 'ready' ? t('dock.ready', { name })
             : t('dock.queued', { name })
  const pct = head.status === 'downloading' ? Math.round((head.downloadProgress || 0) * 100) : null
  const cancellable = active.some(p => p.status !== 'installing')

  return (
    <div role="status" aria-live="polite"
         className="absolute left-6 right-6 bottom-4 z-20 flex items-center gap-3.5 pl-4 pr-3 py-2.5 rounded-[12px] border border-rone-border-2 shadow-[0_18px_40px_-12px_rgba(0,0,0,.75)]"
         style={{ background: 'rgba(27,30,35,.97)', '--acc': accentOf(head) }}>
      <img src={head.logoUrl} alt="" className="w-[30px] h-[30px] rounded-[8px]" />
      <div className="flex-1 min-w-0 flex flex-col gap-1.5">
        <div className="flex items-center gap-2 text-[13px] font-bold min-w-0">
          <span className="truncate">{line}</span>
          {rest.length > 0 && (
            <span className="text-rone-text-dim font-semibold truncate">
              · {t('dock.next', { names: rest.map(p => shortName(p)).join(', ') })}
            </span>
          )}
          {pct !== null && <span className="ml-auto font-mono text-[12px] text-rone-text-secondary num">{pct}%</span>}
        </div>
        <div className={`prog ${pct === null ? 'indet' : ''}`} aria-hidden="true"><i style={{ width: `${pct || 0}%` }} /></div>
      </div>
      <span className="hidden xl:flex items-center gap-1.5 text-[11.5px] text-rone-text-dim whitespace-nowrap">
        <Icon.shield className="w-3.5 h-3.5 text-rone-green" />{platform === 'mac' ? t('dock.shieldMac') : t('dock.shieldWin')}
      </span>
      {cancellable && <button className="btn btn-ghost btn-sm" onClick={onCancelAll}>{t('dock.cancel')}</button>}
    </div>
  )
}
