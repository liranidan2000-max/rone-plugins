import React, { useRef } from 'react'
import { Icon } from '../components/ui'
import { PlayButton, cardAction } from '../components/PluginCard'
import { useBars } from '../usePreview'
import { accentOf, categoriesOf, descriptionOf, formatSize, isInstalled, isUnlocked, parseChangelog, priceOf, shortName, tagsOf, usd } from '../catalog'
import { openExternal } from '../bridge'

// A plugin's own page: the unit, a dry/wet player, what is new, where it shows
// up in the user's DAW, the guide and the manual - everything that used to hide
// in a small Details window and a ⋮ menu.
export default function DetailView ({ t, plugin, access, preview, reduced, daws, dawId, onDaw, catLabel,
                                      onBack, onInstall, onCancel, onOpen, onBuy, onSignIn, onManualPdf, onOpenFolder, onUninstall }) {
  const canvas = useRef(null)
  const acc = accentOf(plugin)
  const playing = preview.id === plugin.id && preview.status === 'playing'
  useBars(canvas, { color: acc, bands: 48, active: playing, reduced })

  const a = cardAction(plugin, access, t)
  const un = isUnlocked(plugin, access)
  const latest = parseChangelog(plugin.whatsNew)[0]
  const name = shortName(plugin)
  const steps = stepsFor(t, dawId, name)
  const price = priceOf(plugin)
  const version = (plugin.remoteVersion || '').split('.').slice(0, 3).join('.')
  const dawChoices = daws.length ? daws : [{ id: 'fl', name: 'FL Studio' }, { id: 'ableton', name: 'Ableton Live' }, { id: 'cubase', name: 'Cubase' }]

  const act = (kind) => kind === 'install' ? onInstall(plugin.id) : kind === 'open' ? onOpen(plugin.id) : kind === 'buy' ? onBuy(plugin) : onSignIn()

  return (
    <div style={{ '--acc': acc }}>
      <button onClick={onBack} className="inline-flex items-center gap-1.5 text-[12.5px] font-bold text-rone-text-dim hover:text-rone-text-primary mb-3">
        <Icon.back className="w-3.5 h-3.5" />{t('detail.back')}
      </button>

      <div className="grid grid-cols-[minmax(260px,340px)_1fr] gap-7 items-start">
        <div className="relative h-[560px] rounded-[16px] border border-rone-border grid place-items-center overflow-hidden"
             style={{ background: `radial-gradient(70% 55% at 50% 60%, color-mix(in srgb, ${acc} 22%, transparent), transparent 70%), linear-gradient(180deg,#16181C,#0F1114)` }}>
          <div className="absolute left-[12%] right-[12%] bottom-5 h-3.5 rounded-full blur-[14px]" style={{ background: `color-mix(in srgb, ${acc} 40%, transparent)` }} />
          <img src={plugin.unitUrl || plugin.logoUrl} alt={'RONE ' + name}
               className={plugin.unitUrl ? 'max-w-[88%] max-h-[92%]' : 'w-[140px] h-[140px] rounded-[30px]'}
               style={{ filter: 'drop-shadow(0 26px 40px rgba(0,0,0,.65))' }} />
        </div>

        <div className="min-w-0 flex flex-col gap-4">
          <div>
            <div className="text-[11.5px] font-extrabold tracking-[0.16em] uppercase" style={{ color: acc }}>
              {[...categoriesOf(plugin).map(catLabel), ...tagsOf(plugin).slice(0, 2)].join(' · ')}
            </div>
            <h1 className="m-0 mt-1 font-display font-extrabold text-[34px] leading-[1.05] text-rone-text-primary">
              <span className="block font-sans text-[12px] tracking-[0.2em] text-rone-text-faint font-extrabold mb-1">RONE</span>{name}
            </h1>
          </div>
          <p className="m-0 text-[14px] leading-relaxed text-rone-text-secondary max-w-[62ch]">{descriptionOf(plugin)}</p>

          <div className="flex items-center gap-2.5 flex-wrap">
            {a.primary && (
              <button className="btn btn-pri" onClick={() => act(a.primary.kind)} style={a.primary.amber ? { '--acc': '#FFD02B' } : undefined}>
                {a.primary.icon && <a.primary.icon className="w-3.5 h-3.5" />}
                {a.primary.kind === 'buy' && price ? `${t('card.getIt')} · ${usd(price.live)}` : a.primary.label}
                {a.primary.kind === 'install' && plugin.status === 'update_available' ? ` → ${version}` : ''}
              </button>
            )}
            {a.secondary && <button className="btn btn-out" onClick={() => act(a.secondary.kind)}><Icon.open className="w-3.5 h-3.5" />{a.secondary.label}</button>}
            {a.busy && <span className="text-[13px] font-bold text-rone-text-secondary">{a.status}{plugin.status === 'downloading' ? ` · ${Math.round((plugin.downloadProgress || 0) * 100)}%` : ''}</span>}
            {a.cancel && <button className="btn btn-ghost" onClick={() => onCancel(plugin.id)}>{t('card.cancel')}</button>}
            {a.ok && <span className="text-[12px] font-extrabold uppercase tracking-[0.08em] text-rone-green">✓ {t('card.installed')}</span>}
            {!un && price && (
              <span className="text-[12px] text-rone-text-dim">
                {price.onSale && <>{t('card.lifetime')} · <s>{usd(price.regular)}</s> · </>}{t('card.trial')}
              </span>
            )}
            <span className="flex-1" />
            {un && isInstalled(plugin) && <button className="btn btn-ghost btn-sm" onClick={() => onOpenFolder(plugin.id)}><Icon.folder className="w-3.5 h-3.5" />{t('menu.openFolder')}</button>}
            {un && plugin.status === 'up_to_date' && <button className="btn btn-ghost btn-sm" onClick={() => onInstall(plugin.id)}>{t('menu.reinstall')}</button>}
            {un && isInstalled(plugin) && <button className="btn btn-ghost btn-sm text-rone-error/80 hover:text-rone-error" onClick={() => onUninstall(plugin)}>{t('menu.uninstall')}</button>}
          </div>

          {/* Dry / wet: the same loop, one switch */}
          <div className="grid grid-cols-[auto_1fr] gap-4 items-center p-3.5 rounded-[14px] border border-rone-border bg-rone-card">
            {plugin.preview ? <PlayButton t={t} plugin={plugin} preview={preview} big /> :
              <span className="w-[52px] h-[52px] rounded-full border border-dashed border-rone-border-3 grid place-items-center text-rone-text-faint"><Icon.play className="w-4 h-4 ml-0.5" /></span>}
            <canvas ref={canvas} width="1100" height="140" className="w-full h-[70px] block" aria-hidden="true" />
            <div className="col-span-2 flex items-center gap-3 text-[12px] text-rone-text-dim">
              {plugin.preview ? (
                <>
                  <span className="inline-flex border border-rone-border-2 rounded-[7px] p-[2px] gap-[2px]" role="radiogroup" aria-label="Dry / wet">
                    {[[false, t('pv.dry')], [true, t('pv.wet')]].map(([w, label]) => (
                      <button key={label} type="button" role="radio" aria-checked={preview.wet === w} onClick={() => preview.setWet(w)}
                              className={`px-2.5 h-[22px] rounded-[5px] text-[10.5px] font-extrabold tracking-[0.08em] ${preview.wet === w ? 'text-[#101216]' : 'text-rone-text-dim'}`}
                              style={preview.wet === w ? { background: acc } : undefined}>{label}</button>
                    ))}
                  </span>
                  <span>{preview.status === 'error' && preview.id === plugin.id ? t('pv.error') : t('detail.ab', { name: 'RONE ' + name })}</span>
                </>
              ) : <span>{t('detail.noPreview')}</span>}
            </div>
          </div>

          <div className="grid grid-cols-2 gap-3">
            <div className="p-4 rounded-[14px] border border-rone-border bg-rone-card min-w-0">
              <h3 className="m-0 mb-2 text-[11px] font-extrabold tracking-[0.14em] uppercase text-rone-text-dim">{t('detail.whatsNew', { version: latest?.version || version })}</h3>
              <ul className="m-0 pl-4 text-[12.5px] text-rone-text-secondary space-y-1 list-disc marker:text-[var(--acc)]">
                {(latest?.items || []).slice(0, 5).map((x, i) => <li key={i}>{x}</li>)}
              </ul>
            </div>
            <div className="p-4 rounded-[14px] border border-rone-border bg-rone-card min-w-0">
              <h3 className="m-0 mb-2 text-[11px] font-extrabold tracking-[0.14em] uppercase text-rone-text-dim">{t('detail.findIt')}</h3>
              <div className="flex gap-1 mb-2 flex-wrap" role="radiogroup" aria-label={t('set.dawMain')}>
                {dawChoices.map(d => (
                  <button key={d.id} role="radio" aria-checked={(dawId || dawChoices[0].id) === d.id} onClick={() => onDaw(d.id)}
                          className={`text-[11.5px] font-bold px-2.5 py-1 rounded-[7px] border ${(dawId || dawChoices[0].id) === d.id ? 'border-[var(--acc)] text-rone-text-primary' : 'border-rone-border-2 text-rone-text-dim'}`}>
                    {d.name}
                  </button>
                ))}
              </div>
              <ol className="m-0 pl-4 text-[12.5px] text-rone-text-secondary space-y-1 list-decimal">
                {steps.map((s, i) => <li key={i}>{s}</li>)}
              </ol>
            </div>
          </div>

          <div className="p-4 rounded-[14px] border border-rone-border bg-rone-card flex items-center gap-4 flex-wrap">
            {plugin.videoUrl ? (
              <button onClick={() => openExternal(plugin.videoUrl)} className="flex items-center gap-3 text-left group">
                <span className="relative w-[150px] h-[84px] rounded-[10px] overflow-hidden flex-none"
                      style={{ background: `radial-gradient(80% 90% at 70% 40%, color-mix(in srgb, ${acc} 40%, transparent), #0F1114)` }}>
                  {plugin.unitUrl && <img src={plugin.unitUrl} alt="" className="absolute right-[-6px] top-[6px] h-[150%]" />}
                  <span className="absolute left-3 top-1/2 -translate-y-1/2 w-[34px] h-[34px] rounded-full bg-white/90 grid place-items-center text-[#101216] group-hover:scale-110 transition-transform">
                    <Icon.play className="w-3 h-3 ml-0.5" />
                  </span>
                </span>
                <span>
                  <b className="block text-[13.5px] text-rone-text-primary">{t('detail.video')} · RONE {name}</b>
                  <span className="text-[12px] text-rone-text-dim">{t('detail.watchOn')} · @RONEPLUGINS</span>
                </span>
              </button>
            ) : (
              <span className="text-[13px] text-rone-text-dim">{t('detail.videoSoon')}</span>
            )}
            <span className="flex-1" />
            {plugin.hasManual && <button className="btn btn-out" onClick={() => onManualPdf(plugin)}><Icon.pdf className="w-3.5 h-3.5" />{t('detail.pdf')}</button>}
            <span className="text-[12px] text-rone-text-faint">
              {(plugin.formats || []).join(' · ')}{plugin.sizeBytes ? ' · ' + formatSize(plugin.sizeBytes) : ''}
            </span>
          </div>
        </div>
      </div>
    </div>
  )
}

export function stepsFor (t, dawId, name) {
  const id = ['fl', 'ableton', 'cubase', 'studioone', 'reaper', 'bitwig', 'logic'].includes(dawId) ? dawId : 'generic'
  const steps = t('steps.' + id, { name: 'RONE ' + name })
  return Array.isArray(steps) ? steps : [steps]
}
