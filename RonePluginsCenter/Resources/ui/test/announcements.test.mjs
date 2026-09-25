// node --test  (npm test) - the Center's popup choice, without React or a WebView.
// Liran's rule (2026-09-25): a pass he gave by hand (passSource 'comp') is never
// shown a deal popup; everyone else sees exactly what 1.5.0 showed them.
import { test, beforeEach } from 'node:test'
import assert from 'node:assert/strict'
import { pickAnnouncement, rememberAnnouncement, isGiftPass } from '../src/announcements.js'

// The page's localStorage, in memory (node has none).
const store = new Map()
globalThis.localStorage = {
  getItem: (k) => (store.has(k) ? store.get(k) : null),
  setItem: (k, v) => { store.set(k, String(v)) },
  removeItem: (k) => { store.delete(k) },
}
beforeEach(() => store.clear())

// Shaped like GET /api/v1/popup?surface=center: FOUNDERS 100 is a 'deal'.
const deal = {
  id: 'deal:1fbd2e0d-test', kind: 'deal', eyebrow: 'FOUNDERS 100', title: 'ALL ACCESS for life',
  price: 'From $79', cta: { label: 'Claim a founder spot', url: '/founders' },
}
const free = {
  id: 'free:RoneClipper:1790188198985', kind: 'free', eyebrow: 'New free plugin', title: 'RONE Clipper',
  price: 'Free · no card needed', cta: { label: 'Get it free', url: '/products/rone-clipper' },
}
const plugin = {
  id: 'plugin:RoneRise:1790188200069', kind: 'plugin', eyebrow: 'New plugin', title: 'RONE Rise',
  price: '$29 lifetime', cta: { label: 'See RONE Rise', url: '/products/rone-rise' },
}
const plugins = [
  { id: 'RoneClipper', name: 'RONE Clipper', status: 'not_installed' },
  { id: 'RoneRise', name: 'RONE Rise', status: 'not_installed' },
]
const pass = (passSource) => ({ plugins, ownedKeys: new Set(), licensed: true, passSource, ignoreMemory: true })
const noPassSourceField = { plugins, ownedKeys: new Set(), licensed: true, ignoreMemory: true }

test('a gift pass (comp) never gets the deal popup', () => {
  assert.equal(pickAnnouncement([deal], pass('comp')), null)
})

test('a gift pass skips the deal and still gets the next popup', () => {
  const item = pickAnnouncement([deal, free], pass('comp'))
  assert.equal(item.popup.id, free.id)
  assert.equal(item.primary.kind, 'install')
})

test('a gift pass still sees plugin popups', () => {
  const item = pickAnnouncement([deal, plugin], pass('comp'))
  assert.equal(item.popup.id, plugin.id)
  assert.equal(item.priceLine, 'Included in your ALL ACCESS pass')
})

test('the compare is the ownership one: casing and padding do not hide a gift', () => {
  assert.equal(pickAnnouncement([deal], pass(' Comp ')), null)
  assert.equal(isGiftPass('COMP'), true)
})

for (const [label, ctx] of [
  ["'paddle' (a paying subscriber)", pass('paddle')],
  ["'lemonsqueezy'", pass('lemonsqueezy')],
  ['null (a giveaway trial)', pass(null)],
  ["'' (no live pass)", pass('')],
  ['no passSource field (a 1.5.0 account file)', noPassSourceField],
]) {
  test(`passSource ${label} still gets the deal popup`, () => {
    const item = pickAnnouncement([deal, free], ctx)
    assert.equal(item.popup.id, deal.id)
    assert.equal(item.primary.kind, 'url')
    assert.equal(item.primary.label, 'Claim a founder spot')
    assert.match(item.primary.url, /^https:\/\/roneaudio\.com\/founders\?utm_source=plugins_center&utm_medium=popup&utm_campaign=center_1fbd2e0d-test$/)
  })
}

test('a skipped deal is not remembered: it can greet the same person once the gift ends', () => {
  const now = Date.now
  try {
    Date.now = () => 1_000_000_000_000
    assert.equal(pickAnnouncement([deal], { ...pass('comp'), ignoreMemory: false }), null)
    assert.equal(store.size, 0)                       // nothing written for the gift
    const item = pickAnnouncement([deal], { ...pass('paddle'), ignoreMemory: false })
    assert.equal(item.popup.id, deal.id)
  } finally { Date.now = now }
})

test('the once-a-day and once-per-id memory is unchanged', () => {
  const now = Date.now
  try {
    Date.now = () => 1_000_000_000_000
    rememberAnnouncement(deal.id)
    assert.equal(pickAnnouncement([deal, free], { ...pass('paddle'), ignoreMemory: false }), null)   // same day
    Date.now = () => 1_000_000_000_000 + 21 * 3600 * 1000
    const item = pickAnnouncement([deal, free], { ...pass('paddle'), ignoreMemory: false })
    assert.equal(item.popup.id, free.id)                                                            // deal already seen
  } finally { Date.now = now }
})
