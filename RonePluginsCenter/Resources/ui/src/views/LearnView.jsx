import React from 'react'
import { Icon } from '../components/ui'
import { accentOf, shortName, tint, tipOfTheWeek } from '../catalog'
import { openExternal } from '../bridge'

// Every video guide and manual in one place, and the tip of the week.
export default function LearnView ({ t, plugins, tips, onManualPdf, onDetail }) {
  const tip = tipOfTheWeek(tips)
  const tipPlugin = tip && plugins.find(p => p.id === tip.plugin)

  return (
    <div>
      <h1 className="m-0 font-display font-extrabold text-[24px] text-rone-text-primary">{t('learn.title')}</h1>
      <p className="m-0 mt-1 mb-4 text-[13px] text-rone-text-dim">{t('learn.sub')}</p>

      {tip && (
        <div className="flex items-center gap-4 p-4 mb-4 rounded-[14px] border border-rone-border-2"
             style={{ background: `linear-gradient(120deg, ${tint(tipPlugin ? accentOf(tipPlugin) : '#D8E4EC', 0.1)}, transparent 60%), rgb(var(--card))` }}>
          {tipPlugin && <img src={tipPlugin.logoUrl} alt="" className="w-[52px] h-[52px] rounded-[12px]" />}
          <div className="flex-1 min-w-0">
            <div className="text-[11px] font-extrabold tracking-[0.18em] uppercase text-rone-text-dim">{t('hero.tip')}</div>
            <p className="m-0 mt-1 text-[14.5px] text-rone-text-primary">{tip.text}</p>
          </div>
          {(tip.url || tipPlugin?.videoUrl) && (
            <button className="btn btn-out" onClick={() => openExternal(tip.url || tipPlugin.videoUrl)}><Icon.play className="w-3 h-3" />{t('hero.watch')}</button>
          )}
        </div>
      )}

      <div className="grid gap-3.5" style={{ gridTemplateColumns: 'repeat(auto-fill, minmax(260px, 1fr))' }}>
        {plugins.map(p => {
          const acc = accentOf(p)
          return (
            <div key={p.id} className="flex flex-col gap-2.5 p-3 rounded-[14px] border border-rone-border bg-rone-card" style={{ '--acc': acc }}>
              <button onClick={() => p.videoUrl ? openExternal(p.videoUrl) : onDetail(p.id)}
                      className="relative h-[120px] rounded-[10px] overflow-hidden text-left group"
                      style={{ background: `radial-gradient(80% 90% at 70% 40%, ${tint(acc, 0.4)}, #0F1114)` }}
                      aria-label={p.videoUrl ? t('detail.video') + ' · RONE ' + shortName(p) : 'RONE ' + shortName(p)}>
                {p.unitUrl && <img src={p.unitUrl} alt="" loading="lazy" className="absolute right-[-6px] top-[8px] h-[170%]" />}
                {p.videoUrl && (
                  <span className="absolute left-3 top-1/2 -translate-y-1/2 w-[38px] h-[38px] rounded-full bg-white/90 grid place-items-center text-[#101216] group-hover:scale-110 transition-transform">
                    <Icon.play className="w-3.5 h-3.5 ml-0.5" />
                  </span>
                )}
              </button>
              <b className="font-display text-[14px] text-rone-text-primary">RONE {shortName(p)}</b>
              <div className="flex gap-2">
                {p.videoUrl
                  ? <button className="btn btn-out btn-sm" onClick={() => openExternal(p.videoUrl)}><Icon.play className="w-3 h-3" />{t('detail.video')}</button>
                  : <span className="btn btn-busy btn-sm">{t('detail.videoSoon')}</span>}
                {p.hasManual && <button className="btn btn-ghost btn-sm" onClick={() => onManualPdf(p)}><Icon.pdf className="w-3.5 h-3.5" />PDF</button>}
              </div>
            </div>
          )
        })}
      </div>
    </div>
  )
}
