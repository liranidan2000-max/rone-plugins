import React, { useEffect, useRef, useState } from 'react'
import { Icon, FloatingMenu, useFloatingMenu } from './ui'
import { accentOf, categoriesOf, descriptionOf, isBusy, isInstalled, isNew, isUnlocked, priceOf, shortName, usd } from '../catalog'

const LED = {
  up_to_date: 'led-ok', update_available: 'led-upd', error: 'led-err',
  queued: 'led-busy', downloading: 'led-busy', ready: 'led-busy', installing: 'led-busy', waiting: 'led-busy', uninstalling: 'led-busy',
}

function Tile ({ plugin, size = 56 }) {
  const [broken, setBroken] = useState(false)
  if (!broken && plugin.logoUrl) {
    return <img className="card-icon flex-none rounded-[14px]" src={plugin.logoUrl} alt="" width={size} height={size}
                style={{ width: size, height: size, boxShadow: '0 8px 18px rgba(0,0,0,.45)' }} onError={() => setBroken(true)} />
  }
  return (
    <div className="card-icon flex-none rounded-[14px] grid place-items-center border border-rone-border-2 font-display font-bold text-white/85"
         style={{ width: size, height: size, background: 'radial-gradient(circle at 38% 30%, #2A2E35, #1B1E23 55%, #14161A)' }}>
      {shortName(plugin).substring(0, 2).toUpperCase()}
    </div>
  )
}

export function PlayButton ({ t, plugin, preview, big = false }) {
  if (!plugin.preview) return null
  const on = preview.id === plugin.id && preview.status !== 'error'
  const name = 'RONE ' + shortName(plugin)
  return (
    <button onClick={(e) => { e.stopPropagation(); preview.toggle(plugin) }}
            aria-label={on ? t('card.stopHear', { name }) : t('card.hear', { name })} aria-pressed={on}
            className={`grid place-items-center rounded-full border flex-none transition-all
                        ${big ? 'w-[52px] h-[52px]' : 'w-[34px] h-[34px]'}
                        ${on ? 'text-[#101216]' : 'border-rone-border-3 text-rone-text-secondary hover:text-[var(--acc)] hover:border-[var(--acc)]'}`}
            style={on ? { background: 'var(--acc)', borderColor: 'var(--acc)', boxShadow: '0 0 14px color-mix(in srgb, var(--acc) 50%, transparent)' } : undefined}>
      {on ? <Icon.stop className={big ? 'w-4 h-4' : 'w-3 h-3'} /> : <Icon.play className={big ? 'w-4 h-4 ml-0.5' : 'w-3 h-3 ml-0.5'} />}
    </button>
  )
}

// What the card's footer says and does, from the plugin's state and access.
export function cardAction (plugin, access, t) {
  const st = plugin.status
  const un = isUnlocked(plugin, access)
  if (st === 'queued') return { status: t('card.queued'), busy: true, cancel: true }
  if (st === 'downloading') return { status: t('card.downloading'), busy: true, cancel: true, progress: plugin.downloadProgress }
  if (st === 'ready') return { status: t('card.ready'), busy: true, cancel: true }
  if (st === 'installing') return { status: t('card.installing'), busy: true }
  if (st === 'waiting') return { status: plugin.waitingFor ? t('card.waitingHost', { host: plugin.waitingFor }) : t('card.waitingOwn'), busy: true, cancel: true, waiting: true }
  if (st === 'uninstalling') return { status: t('card.uninstalling'), busy: true }
  if (!un) {
    if (plugin.free) return { status: t('card.freeLine'), primary: { label: t('card.getFree'), kind: 'signin' } }
    const price = priceOf(plugin)
    return { price, primary: { label: t('card.getIt'), kind: 'buy' } }
  }
  if (st === 'update_available') return { version: true, primary: { label: t('card.update'), kind: 'install', icon: Icon.download, amber: true } }
  if (st === 'error') return { status: t('card.error'), error: true, primary: { label: t('card.retry'), kind: 'install', icon: Icon.retry } }
  if (st === 'up_to_date') return { status: t('card.installed'), ok: true, secondary: plugin.hasStandalone ? { label: t('card.open'), kind: 'open', icon: Icon.open } : null }
  return { status: t('card.notInstalled'), primary: { label: t('card.install'), kind: 'install', icon: Icon.download } }
}

function PluginCard ({ t, plugin, access, preview, catLabel, onInstall, onCancel, onOpen, onDetail, onBuy, onSignIn, onManual, onOpenFolder, onUninstall }) {
  const acc = accentOf(plugin)
  const un = isUnlocked(plugin, access)
  const a = cardAction(plugin, access, t)
  const menu = useFloatingMenu()
  const name = shortName(plugin)

  // The neon flickers on when an install has just finished.
  const prev = useRef(plugin.status)
  const [power, setPower] = useState(false)
  useEffect(() => {
    if (['installing', 'downloading', 'ready', 'queued', 'waiting'].includes(prev.current) && plugin.status === 'up_to_date') {
      setPower(true)
      const id = setTimeout(() => setPower(false), 1600)
      prev.current = plugin.status
      return () => clearTimeout(id)
    }
    prev.current = plugin.status
  }, [plugin.status])

  const act = (kind) => {
    if (kind === 'install') onInstall(plugin.id)
    else if (kind === 'open') onOpen(plugin.id)
    else if (kind === 'buy') onBuy(plugin)
    else if (kind === 'signin') onSignIn()
  }

  const items = [
    { label: t('menu.details'), onSelect: () => onDetail(plugin.id) },
    (plugin.hasManual || plugin.videoUrl) && { label: plugin.videoUrl ? t('menu.manual') + ' / ' + t('menu.video') : t('menu.manual'), onSelect: () => onManual(plugin) },
    isInstalled(plugin) && { label: t('menu.openFolder'), onSelect: () => onOpenFolder(plugin.id) },
    a.cancel && { label: t('menu.cancel'), onSelect: () => onCancel(plugin.id) },
    un && plugin.status === 'up_to_date' && { label: t('menu.reinstall'), onSelect: () => onInstall(plugin.id) },
    un && isInstalled(plugin) && !isBusy(plugin) && '-',
    un && isInstalled(plugin) && !isBusy(plugin) && { label: t('menu.uninstall'), danger: true, onSelect: () => onUninstall(plugin) },
  ]

  return (
    <article className={`card ${un ? '' : 'locked'} ${power ? 'power-on' : ''} relative flex flex-col gap-2.5 p-3.5 rounded-[14px] min-w-0 cursor-pointer`}
             style={{ '--acc': acc }} onClick={() => onDetail(plugin.id)} data-card={plugin.id}
             aria-label={'RONE ' + name}>
      <div className="flex items-start gap-3">
        <Tile plugin={plugin} />
        <div className="flex-1 min-w-0 flex flex-col gap-1">
          <h3 className="m-0 font-display font-bold text-[15px] text-rone-text-primary truncate">
            {/* The name is the keyboard's way into the plugin's page */}
            <button className="text-left max-w-full truncate" onClick={(e) => { e.stopPropagation(); onDetail(plugin.id) }}>
              <span className="font-sans text-[11px] font-bold text-rone-text-faint mr-1">RONE</span>{name}
            </button>
          </h3>
          <div className="flex items-center gap-1.5 flex-wrap text-[11.5px] text-rone-text-dim">
            <span className="font-mono text-[11px]">v{(plugin.remoteVersion || '').split('.').slice(0, 3).join('.')}</span>
            {categoriesOf(plugin)[0] && <><span aria-hidden="true">·</span><span>{catLabel(categoriesOf(plugin)[0])}</span></>}
            {isNew(plugin) && <span className="pill pill-new">{t('card.new')}</span>}
            {plugin.free && <span className="pill pill-free">{t('card.free')}</span>}
            {plugin.owned && !plugin.free && <span className="pill pill-own">{t('card.lifetime')}</span>}
          </div>
        </div>
        <span className={`led mt-1.5 ${LED[plugin.status] || ''}`} aria-hidden="true" />
        <button ref={menu.anchor} onClick={(e) => { e.stopPropagation(); menu.isOpen ? menu.close() : menu.open() }}
                aria-label={t('card.menu', { name: 'RONE ' + name })} aria-haspopup="menu" aria-expanded={menu.isOpen}
                className="-mr-1 -mt-0.5 p-1 rounded-md text-rone-text-faint hover:text-rone-text-primary">
          <Icon.dots className="w-4 h-4" />
        </button>
        <FloatingMenu menu={menu} items={items} label={t('card.menu', { name: 'RONE ' + name })} />
      </div>

      <p className="m-0 text-[12.5px] leading-[1.45] text-rone-text-secondary line-clamp-2 min-h-[36px]">
        {descriptionOf(plugin)}
      </p>

      {(plugin.status === 'downloading' || plugin.status === 'installing' || plugin.status === 'queued' || plugin.status === 'ready') && (
        <div className={`prog ${plugin.status !== 'downloading' ? 'indet' : ''}`} aria-hidden="true">
          <i style={{ width: `${Math.round((plugin.downloadProgress || 0) * 100)}%` }} />
        </div>
      )}

      <div className="mt-auto pt-2.5 border-t border-rone-border flex items-center gap-2 min-h-[44px]" onClick={(e) => e.stopPropagation()}>
        {a.price ? (
          <span className="flex items-baseline gap-1.5 whitespace-nowrap">
            <b className="font-display text-[16px] text-rone-text-primary num">{usd(a.price.live)}</b>
            {a.price.onSale && <s className="text-[11.5px] text-rone-text-faint num">{usd(a.price.regular)}</s>}
            <span className="text-[10.5px] font-extrabold uppercase tracking-[0.12em] text-rone-text-faint">{t('card.lifetimeShort')}</span>
          </span>
        ) : a.version ? (
          <span className="font-mono text-[11.5px] text-rone-text-dim whitespace-nowrap">
            v{(plugin.installedVersion || '?').split('.').slice(0, 3).join('.')} <b className="text-rone-amber font-semibold">→ {(plugin.remoteVersion || '').split('.').slice(0, 3).join('.')}</b>
          </span>
        ) : (
          <span className={`text-[11px] font-extrabold uppercase tracking-[0.08em] truncate
                            ${a.ok ? 'text-rone-green' : a.error ? 'text-rone-error' : a.busy ? 'text-rone-text-secondary normal-case tracking-normal text-[12px] font-bold' : 'text-rone-text-faint'}`}
                title={a.status}>
            {a.ok && '✓ '}{a.status}
            {plugin.status === 'downloading' && plugin.downloadProgress > 0 && <span className="num"> · {Math.round(plugin.downloadProgress * 100)}%</span>}
          </span>
        )}
        <span className="flex-1" />
        <PlayButton t={t} plugin={plugin} preview={preview} />
        {a.cancel && (
          <button className="btn btn-ghost btn-sm" onClick={() => onCancel(plugin.id)}>{t('card.cancel')}</button>
        )}
        {a.secondary && (
          <button className="btn btn-out btn-sm" onClick={() => act(a.secondary.kind)}>
            {a.secondary.icon && <a.secondary.icon className="w-3 h-3" />}{a.secondary.label}
          </button>
        )}
        {a.primary && (
          <button className="btn btn-pri btn-sm" onClick={() => act(a.primary.kind)}
                  style={a.primary.amber ? { '--acc': '#FFD02B' } : undefined}>
            {a.primary.icon && <a.primary.icon className="w-3 h-3" />}{a.primary.label}
          </button>
        )}
      </div>

      {plugin.downloaded && plugin.status === 'update_available' && (
        <span className="absolute top-2 right-12 text-[10px] font-bold text-rone-green" title={t('card.downloaded')}>●</span>
      )}
    </article>
  )
}

export default React.memo(PluginCard)
