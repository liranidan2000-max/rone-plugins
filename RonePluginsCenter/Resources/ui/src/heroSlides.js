// ---- The banner's slides (pure: node tests them, test/heroSlides.test.mjs) ----
// Liran 22.11.2026: "the Center is our official ad page - everyone who got a free plugin keeps it. There should be
// many more banners, ads that do not bother, a level up, with video." Mockup ticked in full the same day
// (docs/mockups/center-hero). The slides come from two places:
//   * the Center itself: sign in, the pass to renew, the ALL ACCESS trial rack, "because you have X", the deals list
//     ask, an update waiting, the newest plugin, the free plugin, the rack this person owns, RONE VST MANAGER, the
//     tip of the week;
//   * the console (roneaudio.com/admin#banners, inside the popup answer as `banners`): launch films, the plugin
//     working (a short clip), deals with a clock, tips in video, "on the bench".
// Rules: at most MAX slides, never sell what the person can already use, a slide they hid stays away HIDE_DAYS,
// a deal never to a gift pass. Each slide is plain data; Hero.jsx draws it and runs its actions.

import { accentOf, descriptionOf, isInstalled, isNew, isUnlocked, priceOf, shortName, tipOfTheWeek, usd, changesSince } from './catalog.js'
import { productKey } from './ownership.js'

export const MAX = 8
export const HIDE_DAYS = 14
export const DURATION = { film: 9000, action: 8000, tip: 8000 }
export const durationOf = (s) => DURATION[s?.kind] || 7000
const KINDS = new Set(['film', 'action', 'tip', 'deal', 'unit', 'soon'])
const HEX = /^#[0-9a-f]{6}$/i
const SITE = 'https://roneaudio.com'

/** guest (not signed in) / free (signed in, no paid plugin) / buyer (owns a paid plugin) / pass (ALL ACCESS). */
export function audienceOf ({ plugins, signedIn, licensed }) {
  if (licensed) return 'pass'
  if (!signedIn) return 'guest'
  return (plugins || []).some(p => p.owned === true && !p.free && p.kind !== 'app' && !p.gift) ? 'buyer' : 'free'
}

// ---- "hide for 14 days" (kept in the page's localStorage, which lives in AppData) ----
const HIDE_KEY = 'rone_center_banner_hidden'
export function readHidden (now = Date.now()) {
  try {
    const v = JSON.parse(localStorage.getItem(HIDE_KEY) || '{}')
    const out = {}
    for (const [k, until] of Object.entries(v && typeof v === 'object' ? v : {})) if (Number(until) > now) out[k] = Number(until)
    return out
  } catch { return {} }
}
export function hideSlide (key, now = Date.now()) {
  const v = readHidden(now)
  v[key] = now + HIDE_DAYS * 86400000
  try { localStorage.setItem(HIDE_KEY, JSON.stringify(v)) } catch {}
  return v
}

/** A link of ours, counted in the console: roneaudio.com only (anything else goes home). */
export function siteLink (url, campaign) {
  try {
    const u = new URL(url, SITE + '/')
    const ours = u.origin === SITE
    if (!ours && !/^https:\/\/(www\.)?(youtube\.com|youtu\.be)\//.test(u.href)) return SITE + '/'
    if (ours) {
      u.searchParams.set('utm_source', 'plugins_center')
      u.searchParams.set('utm_medium', 'banner')
      u.searchParams.set('utm_campaign', String(campaign || 'banner').toLowerCase().replace(/[^a-z0-9_-]+/g, '_').slice(0, 60))
    }
    return u.toString()
  } catch { return SITE + '/' }
}

const findPlugin = (plugins, id) => (plugins || []).find(p => productKey(p.id) === productKey(id)) || null

/** The words and the act of the one button that means "this plugin", for this person now. */
export function pluginAction (p, access, t, { primary = true } = {}) {
  if (!p) return null
  const un = isUnlocked(p, access)
  if (un && p.status === 'not_installed') return { label: t('hero.install'), primary, icon: 'download', act: { type: 'install', id: p.id } }
  if (un) return { label: t('hero.details'), primary: false, ghost: !primary, act: { type: 'detail', id: p.id } }
  const price = priceOf(p)
  if (price) return { label: t('hero.getIt', { price: usd(price.live) }), primary, act: { type: 'buy', id: p.id } }
  return { label: t('hero.details'), primary: false, ghost: !primary, act: { type: 'detail', id: p.id } }
}

/** One console banner -> a slide, or null when it is not for this person. */
export function serverSlide (b, ctx) {
  const { plugins, access, audience, t, passSource, optinOn, hasTrial } = ctx
  if (!b || typeof b.id !== 'string' || !KINDS.has(b.kind)) return null
  if (Array.isArray(b.audiences) && b.audiences.length && !b.audiences.includes(audience)) return null
  const now = ctx.now || Date.now()
  if (b.ends_at && Number(b.ends_at) <= now) return null
  if (b.kind === 'deal' && productKey(passSource) === 'comp') return null      // never a deal to a gift pass
  const p = b.product ? findPlugin(plugins, b.product) : null
  if (b.product && !p) return null                                             // not in this Center (e.g. not on this OS)
  if (p) {
    const un = isUnlocked(p, access)
    if (b.show_if === 'missing' && (isInstalled(p) || p.status !== 'not_installed')) return null
    if (b.show_if === 'locked' && un) return null
    if (b.show_if === 'owned' && !un) return null
  }
  const https = (u) => (typeof u === 'string' && /^https:\/\//.test(u) ? u : null)
  const video = https(b.video)
  if ((b.kind === 'film' || b.kind === 'action' || b.kind === 'tip') && !video && !p?.unitUrl) return null

  const action = (spec, label, primary) => {
    if (!spec) return null
    const tag = { primary, ghost: !primary }
    if (spec === 'plugin') return p ? { ...pluginAction(p, access, t, { primary }), server: true } : null
    if (spec === 'video') return p?.videoUrl ? { label: label || t('hero.watch'), icon: 'play', ...tag, primary, ghost: false, act: { type: 'url', url: p.videoUrl }, server: true } : null
    if (spec === 'manual') return p ? { label: label || t('hero.manual'), ...tag, act: { type: 'manual', id: p.id }, server: true } : null
    if (spec === 'optin') return optinOn ? { label: t('hero.onList'), done: true, ...tag, act: null } : { label, ...tag, act: { type: 'optin' }, server: true }
    if (spec === 'trial') return hasTrial ? { label, ...tag, act: { type: 'trial' }, server: true } : null
    if (spec === 'updates') return { label, ...tag, act: { type: 'updates' }, server: true }
    if (spec.startsWith('url:')) return { label, ...tag, act: { type: 'url', url: siteLink(spec.slice(4), 'center_banner_' + b.id) }, server: true }
    return null
  }
  const acts = [action(b.cta, b.cta_label, true), action(b.cta2, b.cta2_label, false)].filter(a => a && a.label)
  return {
    key: 'b:' + b.id, bannerId: b.id, server: true, kind: b.kind, priority: Number(b.priority) || 50,
    color: HEX.test(b.accent || '') ? b.accent : (p ? accentOf(p) : '#E8EAED'),
    eyebrow: String(b.eyebrow || ''), title: String(b.title || ''), body: String(b.body || ''),
    price: String(b.price || ''), was: String(b.was || ''), word: String(b.word || ''),
    video, poster: https(b.poster), unit: p?.unitUrl || p?.logoUrl || null, product: p?.id || null,
    opensAt: Number(b.opens_at) || null, endsAt: Number(b.ends_at) || null, acts,
  }
}

/**
 * Every slide for this person, in order, at most MAX. ctx: { t, plugins, app, access, account, license, personal,
 * optinAsk, tips, banners, hidden, ups, hasTrial, optinOn, now }.
 */
export function buildSlides (ctx) {
  const { t, plugins = [], app, access, account = {}, license = {}, personal, optinAsk, tips, banners = [], hidden = {}, ups = [], hasTrial } = ctx
  const now = ctx.now || Date.now()
  const signedIn = !!account.signedIn
  const audience = audienceOf({ plugins, signedIn, licensed: !!license.licensed })
  const top = []      // always first, in this order
  let optinSlide = null
  const rest = []     // sorted by priority with the console's

  if (!signedIn && !license.licensed) {
    top.push({ key: 'signin', kind: 'unit', color: '#FFFFFF', eyebrow: 'RONE PLUGINS', title: t('hero.signInTitle'), body: t('hero.signInBody'),
      unit: plugins.find(p => p.free)?.unitUrl, acts: [{ label: t('top.signIn'), primary: true, act: { type: 'account' } }], fixed: true })
  }
  if (signedIn && !license.licensed && account.plan === 'all-access') {
    top.unshift({ key: 'pass', kind: 'unit', color: '#FFFFFF', eyebrow: 'ALL ACCESS', title: t('hero.passTitle'), body: t('hero.passBody'),
      acts: [{ label: t('plan.accountGo'), primary: true, act: { type: 'account' } }], fixed: true })
  }
  // The ALL ACCESS trial: every plugin's unit in one animated rack, for a signed-in account that never had a pass.
  if (hasTrial && signedIn && !license.licensed) {
    const rack = plugins.filter(p => !p.free && p.kind !== 'app' && p.unitUrl)
    const locked = rack.filter(p => !isUnlocked(p, access)).length
    if (rack.length >= 3) {
      top.push({ key: 'trial', kind: 'rack', color: '#B07CFF', eyebrow: t('hero.trialEyebrow'), title: t('hero.trialTitle'),
        body: locked < rack.length ? t('hero.trialBodyOwner', { n: locked }) : t('hero.trialBody', { n: rack.length }),
        rack: rack.map(p => ({ id: p.id, url: p.unitUrl, color: accentOf(p) })),
        acts: [{ label: t('hero.trialGo'), primary: true, act: { type: 'trial' } },
               { label: t('hero.trialMore'), ghost: true, act: { type: 'url', url: siteLink('/pricing#plan', 'hero_trial') } }] })
    }
  }
  // An owner's own slide: "Because you have X" - the $19 plugin while that day runs, else the owner pick.
  if (personal?.plugin) {
    const p = personal.plugin
    const price = priceOf(p)
    const hours = personal.kind === 'first' ? Math.max(1, Math.ceil((Date.parse(personal.endsAt) - now) / 3600000)) : 0
    top.push({ key: 'mine', kind: 'unit', color: accentOf(p),
      eyebrow: personal.kind === 'first' && !personal.becauseName ? t('hero.first') : t('hero.because', { name: personal.becauseName }),
      title: 'RONE ' + shortName(p),
      body: (personal.line ? personal.line + '.' : descriptionOf(p)) + (hours ? ' ' + t('hero.hoursLeft', { n: hours }) + '.' : ''),
      unit: p.unitUrl || p.logoUrl,
      acts: [price && { label: price.kind === 'owner' ? t('hero.ownerPrice', { price: usd(price.live) }) : t('hero.getIt', { price: usd(price.live) }), primary: true, act: { type: 'buy', id: p.id, src: 'hero' } },
             { label: t('hero.details'), ghost: true, act: { type: 'detail', id: p.id } }].filter(Boolean) })
  }
  if (optinAsk) {
    optinSlide = ({ key: 'optin', kind: 'unit', color: '#E8EAED', eyebrow: t('hero.optinEyebrow'), title: t('hero.optinTitle'), body: t('hero.optinBody'),
      acts: [{ label: t('hero.optinYes'), primary: true, act: { type: 'optinAnswer', yes: true } }, { label: t('hero.optinNo'), ghost: true, act: { type: 'optinAnswer', yes: false } }] })
  }

  // ---- the console's banners ----
  const server = []
  for (const b of Array.isArray(banners) ? banners : []) {
    const s = serverSlide(b, { plugins, access, audience, t, passSource: account.passSource, optinOn: !!ctx.optinOn, hasTrial, now })
    if (s && !server.some(x => x.key === s.key)) server.push(s)
  }
  const shownProducts = new Set(server.filter(s => s.product && (s.kind === 'film' || s.kind === 'unit')).map(s => productKey(s.product)))

  // ---- the Center's own ----
  // An update that brings something new: what it brings, in one line (for everyone who has updates waiting).
  const fresh = (ups || []).filter(p => p.kind !== 'app' && changesSince(p).some(e => e.items.length))
  if (fresh.length) {
    const p = fresh[0]
    const line = changesSince(p)[0]?.items[0] || ''
    rest.push({ key: 'update', kind: 'unit', priority: 70, color: accentOf(p), badge: String(p.remoteVersion || '').split('.').slice(0, 3).join('.'),
      eyebrow: t('hero.updEyebrow', { n: ups.length }),
      title: fresh.length === 1 ? 'RONE ' + shortName(p) + ' ' + String(p.remoteVersion || '').split('.').slice(0, 3).join('.') : t('hero.updTitle', { n: ups.length }),
      body: line, unit: p.unitUrl || p.logoUrl,
      acts: [{ label: t('hero.updGo', { n: ups.length }), primary: true, icon: 'download', act: { type: 'updateAll' } },
             { label: t('hero.updWhat'), ghost: true, act: { type: 'updates' } }] })
  }
  // RONE VST MANAGER, for everyone who does not have it yet.
  if (app && app.status !== 'up_to_date' && app.status !== 'update_available') {
    const canInstall = app.unlocked && app.installable !== false
    rest.push({ key: 'vstm', kind: 'unit', priority: 35, color: '#5AA9FF', eyebrow: t('hero.newApp'), title: 'RONE VST MANAGER', body: t('hero.vstmBody'), unit: app.unitUrl || app.logoUrl,
      acts: [canInstall ? { label: t('hero.install'), primary: true, icon: 'download', act: { type: 'install', id: app.id } }
                        : { label: t('hero.tryFree'), primary: true, act: { type: 'url', url: siteLink('/vst-manager?src=center', 'vstm') } },
             { label: t('hero.watch'), icon: 'play', act: { type: 'url', url: 'https://youtu.be/5O7U35jwAzI' } }] })
  }
  const newest = [...plugins].filter(p => isNew(p, now) && p.kind !== 'app').sort((a, b) => Date.parse(b.released) - Date.parse(a.released))[0]
  if (newest && !shownProducts.has(productKey(newest.id))) {
    const a = pluginAction(newest, access, t)
    rest.push({ key: 'new', kind: 'unit', priority: 60, color: accentOf(newest), eyebrow: t('hero.newDrop'), title: 'RONE ' + shortName(newest),
      body: descriptionOf(newest), unit: newest.unitUrl || newest.logoUrl,
      acts: [a, a?.act?.type !== 'detail' && { label: t('hero.details'), ghost: true, act: { type: 'detail', id: newest.id } }].filter(Boolean) })
  }
  const free = plugins.find(p => p.free)
  if (free && signedIn && free.status === 'not_installed') {
    rest.push({ key: 'free', kind: 'unit', priority: 40, color: accentOf(free), eyebrow: t('hero.free'), title: 'RONE ' + shortName(free),
      body: descriptionOf(free), unit: free.unitUrl || free.logoUrl, acts: [{ label: t('hero.install'), primary: true, icon: 'download', act: { type: 'install', id: free.id } }] })
  }
  // The rack this person owns: what they have lights up, the rest waits dark.
  if (signedIn && !license.licensed) {
    const units = plugins.filter(p => p.kind !== 'app' && p.unitUrl)
    const lit = units.filter(p => isUnlocked(p, access))
    if (lit.length && lit.length < units.length && units.length >= 6) {
      rest.push({ key: 'coll', kind: 'coll', priority: 45, color: '#3EFF8B', eyebrow: t('hero.collEyebrow'),
        title: lit.length === 1 ? t('hero.collTitleOne') : t('hero.collTitle', { n: lit.length, total: units.length }), body: t('hero.collBody'),
        units: units.map(p => ({ id: p.id, url: p.unitUrl, color: accentOf(p), lit: isUnlocked(p, access) })), lit: lit.length,
        acts: [hasTrial ? { label: t('hero.collTry'), primary: true, act: { type: 'trial' } } : { label: t('hero.collGo'), primary: true, act: { type: 'url', url: siteLink('/pricing', 'hero_rack') } }] })
    }
  }
  const tip = tipOfTheWeek(tips, now)
  if (tip) {
    const p = findPlugin(plugins, tip.plugin)
    rest.push({ key: 'tip', kind: 'unit', priority: 30, color: p ? accentOf(p) : '#D8E4EC', eyebrow: t('hero.tip'), title: '', body: tip.text, unit: p?.unitUrl || p?.logoUrl,
      acts: [(tip.url || p?.videoUrl) && { label: t('hero.watch'), icon: 'play', act: { type: 'url', url: tip.url || p.videoUrl } },
             p && { label: t('hero.details'), ghost: true, act: { type: 'detail', id: p.id } }].filter(Boolean) })
  }

  // Console banners first among equals; then by priority.
  const ordered = [...server, ...rest].sort((a, b) => (b.priority || 0) - (a.priority || 0) || ((b.server ? 1 : 0) - (a.server ? 1 : 0)))
  const all = [...top, ...ordered].filter(s => s.fixed || !hidden[s.key])
  // The deals-list ask is asked once: second in line, as before 22.11.
  if (optinSlide) all.splice(Math.min(1, all.length), 0, optinSlide)
  return all.slice(0, MAX)
}

/** "opens in" before opensAt, "ends in" until endsAt, null after; parts for the clock. */
export function clockOf (slide, now = Date.now()) {
  const before = slide.opensAt && now < slide.opensAt
  const to = before ? slide.opensAt : slide.endsAt
  if (!to || to <= now) return null
  let d = to - now
  const parts = [86400000, 3600000, 60000, 1000].map(ms => { const v = Math.floor(d / ms); d -= v * ms; return v })
  return { before, parts }
}
