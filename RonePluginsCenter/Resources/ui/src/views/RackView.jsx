import React from 'react'
import { accentOf, isUnlocked, priceOf, shortName, usd } from '../catalog'

// Every plugin as its hardware unit on a shelf. What you own is lit, with its
// lamp on; the rest stands dark with a small price. Someone holding the pass
// (a gift included) owns everything, so for them the whole rack is lit and
// nothing is for sale.
export default function RackView ({ t, plugins, access, onDetail }) {
  const owned = plugins.filter(p => isUnlocked(p, access))

  return (
    <div>
      <div className="flex items-end justify-between gap-4 mb-4">
        <div>
          <h1 className="m-0 font-display font-extrabold text-[24px] text-rone-text-primary">{t('rack.title')}</h1>
          <p className="m-0 mt-1 text-[13px] text-rone-text-dim">{t('rack.sub')}</p>
        </div>
        <div className="flex items-center gap-2.5">
          <span className="flex gap-[3px]" aria-hidden="true">
            {plugins.map(p => {
              const on = isUnlocked(p, access)
              const c = accentOf(p)
              return <i key={p.id} className="block w-4 h-2 rounded-[2px]" style={on ? { background: c, boxShadow: `0 0 8px ${c}` } : { background: '#25282E' }} />
            })}
          </span>
          <b className="font-mono text-[14px] text-rone-text-primary">{t('rack.count', { n: owned.length, total: plugins.length })}</b>
        </div>
      </div>

      <div className="rack-shelf grid gap-3 p-3.5 rounded-[16px] border border-rone-border"
           style={{ gridTemplateColumns: 'repeat(auto-fill, minmax(170px, 1fr))' }}>
        {plugins.map(p => {
          const on = isUnlocked(p, access)
          const acc = accentOf(p)
          const price = priceOf(p)
          const status = on
            ? (p.status === 'up_to_date' ? t('rack.installed') : p.status === 'update_available'
                ? t('rack.update', { v: (p.remoteVersion || '').split('.').slice(0, 3).join('.') }) : t('rack.notInstalled'))
            : p.free ? t('card.free') : price ? t('rack.price', { price: usd(price.live) }) : ''
          return (
            <button key={p.id} onClick={() => onDetail(p.id)}
                    className={`relative h-[240px] rounded-[12px] border border-[#1E2126] flex flex-col items-center justify-end px-2 pb-2.5 overflow-hidden text-center ${on ? '' : 'slot-off'}`}
                    style={{ background: 'linear-gradient(180deg,#15171B,#0E1013)' }} aria-label={'RONE ' + shortName(p) + ' · ' + status}>
              {on && <span className="absolute left-[20%] right-[20%] top-[150px] h-[22px] rounded-full blur-[16px] opacity-45" style={{ background: acc }} />}
              <img src={p.unitUrl || p.logoUrl} alt="" loading="lazy"
                   className={`absolute left-1/2 -translate-x-1/2 top-3 max-w-[86%] max-h-[165px] ${p.unitUrl ? '' : 'w-[96px] h-[96px] top-10 rounded-[22px]'}`}
                   style={{ filter: on ? 'drop-shadow(0 14px 20px rgba(0,0,0,.6))' : undefined, transition: 'filter .4s, opacity .4s' }} />
              <span className="relative flex items-center gap-1.5 font-display font-bold text-[13px] text-rone-text-primary">
                <i className="block w-[7px] h-[7px] rounded-full" style={on ? { background: acc, boxShadow: `0 0 8px ${acc}` } : { background: '#3A3E45' }} />
                {shortName(p)}
              </span>
              <span className="relative text-[11.5px] text-rone-text-faint mt-0.5">{status}</span>
            </button>
          )
        })}
      </div>
    </div>
  )
}
