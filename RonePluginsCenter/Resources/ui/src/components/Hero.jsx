import React, { useEffect, useMemo, useState } from 'react'
import { Icon } from './ui'
import { accentOf, descriptionOf, isNew, isUnlocked, priceOf, shortName, tipOfTheWeek, usd } from '../catalog'
import { openExternal } from '../bridge'

// The banner: the newest plugin with its real 3D unit, the free plugin, and the
// tip of the week - instead of a status banner. A guest or a lapsed pass sees
// what unlocks things first. Slides change every 8 s unless motion is reduced.
// The unit turns slowly in perspective with a light sweeping across it (index.css .hero-*).
export default function Hero ({ t, plugins, access, account, license, personal, optinAsk, onBuy, tips, reduced, onInstall, onDetail, onAccount }) {
  const slides = useMemo(() => {
    const out = []
    const signedIn = !!account.signedIn

    if (!signedIn && !license.licensed) {
      out.push({ key: 'signin', color: '#FFFFFF', eyebrow: 'RONE PLUGINS', title: t('hero.signInTitle'), body: t('hero.signInBody'),
                 unit: plugins.find(p => p.free)?.unitUrl, actions: [{ label: t('top.signIn'), primary: true, onClick: onAccount }] })
    }

    const newest = [...plugins].filter(p => isNew(p)).sort((a, b) => Date.parse(b.released) - Date.parse(a.released))[0]
    if (newest) {
      const un = isUnlocked(newest, access)
      const price = priceOf(newest)
      const actions = []
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

    // An owner's own slide first: "Because you have X" - the $19 plugin while that day runs, else the owner pick.
    if (personal?.plugin && onBuy) {
      const p = personal.plugin
      const price = priceOf(p)
      const hours = personal.kind === 'first' ? Math.max(1, Math.ceil((Date.parse(personal.endsAt) - Date.now()) / 3600000)) : 0
      out.unshift({ key: 'mine', color: accentOf(p),
                    eyebrow: personal.kind === 'first' ? t('hero.first') : t('hero.because', { name: personal.becauseName }),
                    title: 'RONE ' + shortName(p),
                    body: (personal.line ? personal.line + '.' : descriptionOf(p)) + (hours ? ' ' + t('hero.hoursLeft', { n: hours }) + '.' : ''),
                    unit: p.unitUrl || p.logoUrl,
                    actions: [
                      price && { label: price.kind === 'owner' ? t('hero.ownerPrice', { price: usd(price.live) }) : t('hero.getIt', { price: usd(price.live) }),
                                 primary: true, onClick: () => onBuy(p, 'hero') },
                      { label: t('hero.details'), ghost: true, onClick: () => onDetail(p.id) },
                    ].filter(Boolean) })
    }

    // The deals list, asked once (App.jsx decides who: signed in, not on it, never a gift pass).
    if (optinAsk) {
      out.splice(Math.min(1, out.length), 0, { key: 'optin', color: '#E8EAED', eyebrow: t('hero.optinEyebrow'), title: t('hero.optinTitle'), body: t('hero.optinBody'),
        actions: [{ label: t('hero.optinYes'), primary: true, onClick: () => optinAsk(true) }, { label: t('hero.optinNo'), ghost: true, onClick: () => optinAsk(false) }] })
    }

    if (signedIn && !license.licensed && account.plan === 'all-access') {
      out.unshift({ key: 'pass', color: '#FFFFFF', eyebrow: 'ALL ACCESS', title: t('hero.passTitle'), body: t('hero.passBody'),
                    actions: [{ label: t('plan.accountGo'), primary: true, onClick: onAccount }] })
    }
    return out
  }, [plugins, access, account, license, tips, t, personal, optinAsk, onBuy])

  const [i, setI] = useState(0)
  useEffect(() => { if (i >= slides.length) setI(0) }, [slides.length])
  useEffect(() => {
    if (reduced || slides.length < 2) return
    const id = setInterval(() => setI(x => (x + 1) % slides.length), 8000)
    return () => clearInterval(id)
  }, [reduced, slides.length])

  const [broken, setBroken] = useState({})
  if (!slides.length) return null
  const s = slides[Math.min(i, slides.length - 1)]
  const unit = s.unit && !broken[s.unit] ? s.unit : null

  return (
    <section className="relative h-[220px] rounded-[18px] overflow-hidden border border-rone-border-2 flex-shrink-0"
             style={{ background: 'linear-gradient(160deg, #1B1E23, #0D0F12)' }} aria-roledescription="carousel">
      <div className="hero-glow absolute inset-0" style={{ background: `radial-gradient(55% 110% at 78% 50%, ${s.color} 0%, transparent 62%)` }} />
      <div className="absolute inset-0" style={{ background: 'radial-gradient(90% 160% at 85% 50%, rgba(255,255,255,.06) 0%, transparent 55%)' }} />
      {unit && (
        <div className="hero-stage absolute right-[3%] top-0 bottom-0 w-[44%] flex items-center justify-center pointer-events-none" aria-hidden="true">
          <div key={s.key + unit} className={reduced ? 'relative' : 'hero-unit-wrap'} style={{ '--unit-mask': `url("${unit}")` }}>
            <img src={unit} alt="" className={reduced ? 'block max-h-[190px] w-auto' : 'hero-unit max-h-[190px] w-auto'}
                 style={reduced ? { filter: 'drop-shadow(0 22px 34px rgba(0,0,0,.65))' } : undefined}
                 onError={() => setBroken(b => ({ ...b, [unit]: true }))} />
            {!reduced && <span className="hero-sheen" />}
            <span className="hero-reflect" style={{ backgroundImage: `url("${unit}")` }} />
          </div>
        </div>
      )}
      <div className={`relative h-full px-8 py-7 flex flex-col gap-2 ${unit ? 'max-w-[58%]' : 'max-w-[86%]'}`} aria-live="polite">
        <div className="text-[11px] font-extrabold tracking-[0.24em] uppercase" style={{ color: s.color, textShadow: `0 0 14px ${s.color}` }}>{s.eyebrow}</div>
        {s.title && <h2 className="m-0 font-display font-extrabold text-[32px] leading-[1.08] tracking-[-0.01em] text-white text-balance">{s.title}</h2>}
        <p className={`m-0 text-rone-text-secondary ${s.title ? 'text-[13.5px] leading-[1.5] line-clamp-2' : 'text-[16px] font-semibold leading-snug text-rone-text-primary line-clamp-3'}`}>{s.body}</p>
        <div className="mt-auto flex items-center gap-2">
          {s.actions.map(a => (
            <button key={a.label} onClick={a.onClick} className={`btn ${a.primary ? 'btn-pri btn-white h-[38px] px-5' : a.ghost ? 'btn-ghost' : 'btn-out'}`}>
              {a.icon && <a.icon className="w-3.5 h-3.5" />}{a.label}
            </button>
          ))}
        </div>
      </div>
      {slides.length > 1 && (
        <div className="absolute left-8 bottom-3 flex gap-1.5">
          {slides.map((x, k) => (
            <button key={x.key} onClick={() => setI(k)} aria-label={t('hero.slide', { n: k + 1 })} aria-current={k === i}
                    className="h-[4px] rounded-[2px] transition-all"
                    style={{ width: k === i ? 28 : 16, background: k === i ? s.color : '#383D45', boxShadow: k === i ? `0 0 8px ${s.color}` : 'none' }} />
          ))}
        </div>
      )}
    </section>
  )
}
