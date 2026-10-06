import React, { useEffect, useMemo, useState } from 'react'
import { Icon } from './ui'
import { accentOf, descriptionOf, isNew, isUnlocked, priceOf, shortName, tipOfTheWeek, usd } from '../catalog'
import { openExternal } from '../bridge'

// The banner: the newest plugin with its real 3D unit, the free plugin, and the
// tip of the week - instead of a status banner. A guest or a lapsed pass sees
// what unlocks things first. Slides change every 8 s unless motion is reduced.
export default function Hero ({ t, plugins, access, account, license, preview, tips, reduced, onInstall, onDetail, onAccount }) {
  const slides = useMemo(() => {
    const out = []
    const signedIn = !!account.signedIn

    if (!signedIn && !license.licensed) {
      out.push({ key: 'signin', color: '#9D6BFF', eyebrow: 'RONE PLUGINS', title: t('hero.signInTitle'), body: t('hero.signInBody'),
                 unit: plugins.find(p => p.free)?.unitUrl, actions: [{ label: t('top.signIn'), primary: true, onClick: onAccount }] })
    }

    const newest = [...plugins].filter(p => isNew(p)).sort((a, b) => Date.parse(b.released) - Date.parse(a.released))[0]
    if (newest) {
      const un = isUnlocked(newest, access)
      const price = priceOf(newest)
      const actions = []
      if (newest.preview) actions.push({ label: preview.id === newest.id && preview.status !== 'error' ? t('hero.stop') : t('hero.hear'), icon: Icon.play, onClick: () => preview.toggle(newest) })
      if (un && newest.status === 'not_installed') actions.push({ label: t('hero.install'), primary: true, icon: Icon.download, onClick: () => onInstall(newest.id) })
      else if (!un && price) actions.push({ label: t('hero.getIt', { price: usd(price.live) }), primary: true, onClick: () => onDetail(newest.id) })
      actions.push({ label: t('hero.details'), ghost: true, onClick: () => onDetail(newest.id) })
      out.push({ key: 'new', color: accentOf(newest), eyebrow: t('hero.newDrop'),
                 title: 'RONE ' + shortName(newest), body: descriptionOf(newest), unit: newest.unitUrl || newest.logoUrl, actions })
    }

    const free = plugins.find(p => p.free)
    if (free && signedIn && free.status === 'not_installed') {
      out.push({ key: 'free', color: accentOf(free), eyebrow: t('hero.free'), title: 'RONE ' + shortName(free),
                 body: descriptionOf(free), unit: free.unitUrl || free.logoUrl,
                 actions: [
                   free.preview && { label: t('hero.hear'), icon: Icon.play, onClick: () => preview.toggle(free) },
                   { label: t('hero.install'), primary: true, icon: Icon.download, onClick: () => onInstall(free.id) },
                 ].filter(Boolean) })
    }

    const tip = tipOfTheWeek(tips)
    if (tip) {
      const p = plugins.find(x => x.id === tip.plugin)
      out.push({ key: 'tip', color: p ? accentOf(p) : '#D8E4EC', eyebrow: t('hero.tip'), title: '', body: tip.text,
                 unit: p?.unitUrl || p?.logoUrl,
                 actions: [(tip.url || p?.videoUrl) && { label: t('hero.watch'), icon: Icon.play, onClick: () => openExternal(tip.url || p.videoUrl) },
                           p && { label: t('hero.details'), ghost: true, onClick: () => onDetail(p.id) }].filter(Boolean) })
    }

    if (signedIn && !license.licensed && account.plan === 'all-access') {
      out.unshift({ key: 'pass', color: '#9D6BFF', eyebrow: 'ALL ACCESS', title: t('hero.passTitle'), body: t('hero.passBody'),
                    actions: [{ label: t('plan.accountGo'), primary: true, onClick: onAccount }] })
    }
    return out
  }, [plugins, access, account, license, preview.id, preview.status, tips, t])

  const [i, setI] = useState(0)
  useEffect(() => { if (i >= slides.length) setI(0) }, [slides.length])
  useEffect(() => {
    if (reduced || slides.length < 2) return
    const id = setInterval(() => setI(x => (x + 1) % slides.length), 8000)
    return () => clearInterval(id)
  }, [reduced, slides.length])

  if (!slides.length) return null
  const s = slides[Math.min(i, slides.length - 1)]

  return (
    <section className="relative h-[184px] rounded-[16px] overflow-hidden border border-rone-border-2 flex-shrink-0"
             style={{ background: 'linear-gradient(160deg, #1B1E23, #0F1114)' }} aria-roledescription="carousel">
      <div className="absolute inset-0" style={{ background: `radial-gradient(60% 120% at 82% 50%, ${s.color} 0%, transparent 60%)`, opacity: 0.3 }} />
      {s.unit && (
        <img key={s.key + s.unit} src={s.unit} alt="" aria-hidden="true"
             className="absolute right-[4%] top-[12px] h-[300px] w-auto -rotate-2 pointer-events-none"
             style={{ filter: 'drop-shadow(0 18px 30px rgba(0,0,0,.6))' }} />
      )}
      <div className="relative h-full px-7 py-6 flex flex-col gap-1.5 max-w-[60%]" aria-live="polite">
        <div className="text-[11px] font-extrabold tracking-[0.2em] uppercase" style={{ color: s.color }}>{s.eyebrow}</div>
        {s.title && <h2 className="m-0 font-display font-extrabold text-[26px] leading-[1.12] text-rone-text-primary text-balance">{s.title}</h2>}
        <p className={`m-0 text-rone-text-secondary ${s.title ? 'text-[13.5px] line-clamp-2' : 'text-[16px] font-semibold leading-snug text-rone-text-primary line-clamp-3'}`}>{s.body}</p>
        <div className="mt-auto flex items-center gap-2" style={{ '--acc': s.color }}>
          {s.actions.map(a => (
            <button key={a.label} onClick={a.onClick} className={`btn ${a.primary ? 'btn-pri' : a.ghost ? 'btn-ghost' : 'btn-out'}`}>
              {a.icon && <a.icon className="w-3.5 h-3.5" />}{a.label}
            </button>
          ))}
        </div>
      </div>
      {slides.length > 1 && (
        <div className="absolute left-7 bottom-3 flex gap-1.5">
          {slides.map((x, k) => (
            <button key={x.key} onClick={() => setI(k)} aria-label={t('hero.slide', { n: k + 1 })} aria-current={k === i}
                    className="h-[4px] rounded-[2px] transition-all" style={{ width: k === i ? 28 : 16, background: k === i ? s.color : '#383D45' }} />
          ))}
        </div>
      )}
    </section>
  )
}
