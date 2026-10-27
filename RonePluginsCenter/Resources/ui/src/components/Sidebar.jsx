import React from 'react'
import { Icon } from './ui'
import { openExternal } from '../bridge'
import { shortName } from '../catalog'
import { isGiftPass } from '../announcements'

const NAV = [
  ['library', Icon.library], ['updates', Icon.updates], ['rack', Icon.rack], ['learn', Icon.learn], ['settings', Icon.settings],
]

export default function Sidebar ({ t, view, onNavigate, updatesCount, account, license, ownedPlugins, locale }) {
  const navView = view === 'detail' ? 'library' : view
  const signedIn = !!account.signedIn
  const pass = !!license.licensed
  const owned = ownedPlugins.filter(p => !p.free)
  const gift = isGiftPass(account.passSource)

  // The plan card says what is true for this person - before 2.0 a guest read
  // "PRO · Professional Plan · Not activated".
  let plan
  if (pass) {
    const renews = !gift && account.renewsAt ? new Date(account.renewsAt).toLocaleDateString(locale, { day: 'numeric', month: 'short' }) : ''
    plan = { k: t('plan.all'), body: renews ? t('plan.allRenews', { date: renews }) : t('plan.allBody'), go: t('plan.allGo'), to: 'account' }
  } else if (owned.length) {
    const esc = (s) => String(s).replace(/[&<>"]/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]))
    const names = owned.length <= 2 ? owned.map(p => esc(shortName(p))).join(', ') : t('plan.lifetimeCount', { n: owned.length })
    plan = { k: t('plan.lifetime'), body: t('plan.lifetimeBody', { names }), go: t('plan.lifetimeGo'), to: 'account' }
  } else if (signedIn) {
    plan = { k: t('plan.free'), body: t('plan.freeBody'), go: t('plan.accountGo'), to: 'account' }
  } else {
    plan = { k: t('plan.guest'), body: t('plan.guestBody'), go: t('plan.guestGo'), to: 'signup' }
  }

  return (
    <aside className="sidebar-panel flex-shrink-0 w-[224px] h-full flex flex-col" aria-label="RONE Plugins Center">
      <button onClick={() => onNavigate('library')} className="group px-5 pt-5 pb-4 text-left" aria-label={t('nav.library')}>
        <div className="font-display font-extrabold text-[19px] tracking-[0.01em] text-rone-text-primary whitespace-nowrap transition-transform duration-200 group-hover:scale-[1.03] origin-left">
          <span style={{ textShadow: '0 0 16px rgba(255,255,255,0.35)' }}>RONE</span><span className="ml-1.5 text-rone-text-dim">PLUGINS</span>
        </div>
        <div className="mt-0.5 text-[10px] font-extrabold tracking-[0.32em] text-rone-text-dim uppercase">Center</div>
      </button>

      <nav className="px-2.5 flex flex-col gap-0.5" aria-label="Main">
        {NAV.map(([key, I]) => {
          const active = navView === key
          const badge = key === 'updates' ? updatesCount : 0
          return (
            <button key={key} onClick={() => onNavigate(key)} aria-current={active ? 'page' : undefined}
                    className={`relative flex items-center gap-3 px-3 py-[9px] rounded-[10px] text-[13px] font-bold transition-colors
                                ${active ? 'nav-active' : 'text-rone-text-dim hover:text-rone-text-secondary hover:bg-white/[0.03]'}`}>
              <I className={`w-[17px] h-[17px] ${active ? 'text-rone-purple' : 'text-rone-text-faint'}`} />
              <span className="flex-1 text-left">{t('nav.' + key)}</span>
              {badge > 0 && (
                <span className="min-w-[20px] h-[20px] px-1.5 rounded-full bg-white text-[#101216] text-[11px] font-extrabold grid place-items-center shadow-[0_0_12px_rgba(255,255,255,0.4)]"
                      aria-label={String(badge)}>{badge}</span>
              )}
            </button>
          )
        })}
        <button onClick={() => openExternal('https://roneaudio.com/?utm_source=plugins_center&utm_medium=sidebar')}
                className="mt-1.5 flex items-center gap-3 px-3 py-[9px] rounded-[10px] text-[12.5px] font-semibold text-rone-text-dim hover:text-rone-purple hover:bg-white/[0.03]">
          <Icon.globe className="w-[17px] h-[17px] text-rone-text-faint" />
          <span className="flex-1 text-left">roneaudio.com</span>
          <Icon.open className="w-3 h-3" />
        </button>
      </nav>

      <div className="m-3 mt-auto rounded-xl border border-rone-border p-3.5" style={{ background: 'linear-gradient(160deg, #1B1E23 0%, #101216 100%)' }}>
        <div className="flex items-center gap-2 font-display text-[12px] font-extrabold tracking-[0.16em] text-rone-text-primary">
          <Icon.bolt className="w-3.5 h-3.5 text-rone-purple" />{plan.k}
        </div>
        <p className="mt-1.5 text-[12px] leading-snug text-rone-text-secondary [&_b]:text-rone-green [&_b]:font-bold"
           dangerouslySetInnerHTML={{ __html: plan.body }} />
        <button onClick={() => plan.to === 'signup' ? onNavigate('account') : onNavigate(plan.to)}
                className="mt-2.5 text-[11px] font-extrabold uppercase tracking-[0.12em] text-rone-purple hover:text-rone-light-purple">
          {plan.go} ›
        </button>
      </div>
    </aside>
  )
}
