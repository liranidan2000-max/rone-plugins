// The banner's slides (Liran 22.11.2026, "the Center is our ad page"): who sees what, never selling what the person
// can already use, at most 8, hidden for 14 days, deals never to a gift pass, our own links counted.
import test from 'node:test'
import assert from 'node:assert/strict'
import { makeT } from '../src/i18n.js'
import { audienceOf, buildSlides, serverSlide, siteLink, clockOf, MAX } from '../src/heroSlides.js'
import { mockBanners } from '../src/mockBanners.js'

const t = makeT('en')
const NOW = Date.parse('2026-11-23T12:00:00Z')
const P = (id, extra = {}) => ({ id, name: id, kind: 'plugin', status: 'not_installed', unitUrl: `/units/${id}.webp`, price: 39, ...extra })
const catalog = () => [
  P('RoneClipper', { free: true, price: 0 }), P('RoneSidechain', { free: true, price: 0 }), P('RoneStutter', { price: 49 }),
  P('RoneStucker', { price: 29 }), P('Rone1176', { price: undefined }), P('RoneReverb', { price: 29, videoUrl: 'https://youtu.be/x' }),
  P('RoneHook', { gift: true, price: undefined, videoUrl: 'https://youtu.be/h' }), P('RoneThrow'),
]
const ctxFor = (who, over = {}) => {
  const plugins = catalog()
  if (who === 'buyer') for (const p of plugins) if (['RoneStutter', 'Rone1176', 'RoneClipper', 'RoneSidechain'].includes(p.id)) p.owned = true
  if (who === 'free') for (const p of plugins) if (p.free) p.owned = true
  const signedIn = who !== 'guest'
  const licensed = who === 'pass'
  return { t, plugins, app: null, access: { licensed, signedIn }, account: { signedIn }, license: { licensed }, tips: [],
    banners: mockBanners, hidden: {}, ups: [], hasTrial: false, optinOn: false, now: NOW, ...over }
}
const keys = (ctx) => buildSlides(ctx).map(s => s.key)

test('audienceOf: guest / free / buyer / pass (a free plugin or a gift does not make a buyer)', () => {
  assert.equal(audienceOf({ plugins: [], signedIn: false, licensed: false }), 'guest')
  assert.equal(audienceOf({ plugins: [P('A', { free: true, owned: true }), P('H', { gift: true, owned: true })], signedIn: true }), 'free')
  assert.equal(audienceOf({ plugins: [P('A', { owned: true })], signedIn: true }), 'buyer')
  assert.equal(audienceOf({ plugins: [], signedIn: true, licensed: true }), 'pass')
})

test('a free account sees the deal, the films and the clips of what it lacks; never a tip for a plugin it lacks', () => {
  const k = keys(ctxFor('free'))
  assert.ok(k.length <= MAX)
  for (const want of ['b:deal:bf2026', 'b:film:hook', 'b:film:reverb', 'b:action:stutter']) assert.ok(k.includes(want), want)
  assert.ok(!k.includes('b:tip:1176-scope'))
  assert.equal(k[0], 'b:deal:bf2026', 'the deal leads (priority 90)')
})

test('never sell what the person can already use', () => {
  const buyer = keys(ctxFor('buyer'))
  assert.ok(!buyer.includes('b:action:stutter'), 'owns Stutter: no "see it work" ad for it')
  assert.ok(buyer.includes('b:tip:stutter-filter'), 'owns Stutter: the tip instead')
  assert.ok(buyer.includes('b:tip:1176-scope'))
  const pass = keys(ctxFor('pass'))
  assert.ok(!pass.includes('b:deal:bf2026'), 'ALL ACCESS gets no deal')
  assert.ok(!pass.some(k => k.startsWith('b:action:')))
  assert.ok(pass.includes('b:tip:1176-scope'))
})

test('installed = no "missing" banner; a plugin this Center does not list = no banner', () => {
  const ctx = ctxFor('free')
  ctx.plugins.find(p => p.id === 'RoneSidechain').status = 'up_to_date'
  assert.ok(!keys(ctx).includes('b:action:sidechain'))
  ctx.plugins = ctx.plugins.filter(p => p.id !== 'RoneReverb')
  assert.ok(!keys(ctx).includes('b:film:reverb'))
})

test('a deal never reaches a gift pass, an ended banner never shows, unknown kinds are ignored', () => {
  const ctx = ctxFor('buyer', { account: { signedIn: true, passSource: 'comp' } })
  assert.ok(!keys(ctx).includes('b:deal:bf2026'))
  assert.equal(serverSlide({ ...mockBanners[0], ends_at: NOW - 1 }, { ...ctxFor('free'), audience: 'free' }), null)
  assert.equal(serverSlide({ ...mockBanners[0], kind: 'popunder' }, { ...ctxFor('free'), audience: 'free' }), null)
})

test('hidden for 14 days stays out; the sign-in slide cannot be hidden', () => {
  const ctx = ctxFor('guest', { hidden: { 'b:deal:bf2026': NOW + 1000, signin: NOW + 1000 } })
  const k = keys(ctx)
  assert.ok(!k.includes('b:deal:bf2026'))
  assert.equal(k[0], 'signin')
})

test('the "plugin" button says what fits this person now', () => {
  const free = buildSlides(ctxFor('free')).find(s => s.key === 'b:film:reverb')
  assert.equal(free.acts[0].label, 'Get it · $29')
  assert.deepEqual(free.acts[0].act, { type: 'buy', id: 'RoneReverb' })
  const pass = buildSlides(ctxFor('pass')).find(s => s.key === 'b:film:reverb')
  assert.equal(pass.acts[0].label, 'Install')
  const side = buildSlides(ctxFor('free')).find(s => s.key === 'b:action:sidechain')
  assert.equal(side.acts[0].act.type, 'install', 'a free plugin a signed-in person lacks installs at once')
})

test('optin: "Tell me first" until on the list, then a quiet check mark', () => {
  const off = buildSlides(ctxFor('free')).find(s => s.key === 'b:soon:bench')
  assert.equal(off?.acts[0].act.type, 'optin')
  const on = buildSlides(ctxFor('free', { optinOn: true })).find(s => s.key === 'b:soon:bench')
  assert.ok(on.acts[0].done)
})

test('the rack lights what the person can use', () => {
  const s = buildSlides(ctxFor('buyer')).find(x => x.key === 'coll')
  assert.ok(s, 'a buyer sees the rack')
  assert.equal(s.lit, 4)
  assert.ok(!buildSlides(ctxFor('pass')).some(x => x.key === 'coll'))
})

test('siteLink: our links counted, other hosts sent home (YouTube allowed as is)', () => {
  assert.match(siteLink('/black-friday', 'center_banner_deal:bf2026'), /^https:\/\/roneaudio\.com\/black-friday\?utm_source=plugins_center&utm_medium=banner&utm_campaign=center_banner_deal_bf2026$/)
  assert.equal(siteLink('https://evil.example/x'), 'https://roneaudio.com/')
  assert.equal(siteLink('https://youtu.be/abc'), 'https://youtu.be/abc')
})

test('clock: "opens in" before the open, "ends in" after, nothing once over', () => {
  const s = { opensAt: NOW + 3600000, endsAt: NOW + 5 * 86400000 }
  assert.deepEqual(clockOf(s, NOW), { before: true, parts: [0, 1, 0, 0] })
  assert.equal(clockOf(s, NOW + 7200000).before, false)
  assert.equal(clockOf(s, NOW + 6 * 86400000), null)
})

test('a tip whose plugin has no guide video: the manual button left becomes the main one, never a lone grey link', () => {
  const tip = mockBanners.find(b => b.id === 'tip:1176-scope')
  const s = serverSlide(tip, { ...ctxFor('buyer'), audience: 'buyer' })
  assert.equal(s.acts.length, 1)
  assert.equal(s.acts[0].act.type, 'manual')
  assert.equal(s.acts[0].primary, true)
  assert.ok(!s.acts[0].ghost)
})
