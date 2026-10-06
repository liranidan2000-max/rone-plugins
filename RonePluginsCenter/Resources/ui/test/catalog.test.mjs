import test from 'node:test'
import assert from 'node:assert/strict'
import {
  accentOf, categoriesOf, parseChangelog, changesSince, matchesSearch, updatable,
  sortPlugins, isNew, priceOf, tipOfTheWeek, compareVersions, formatSize, isUnlocked,
} from '../src/catalog.js'

const RR_WHATS_NEW = '1.1.10 - Mac standalone: OPTIONS opens again and the window can be dragged by its title bar - a click there used to be caught by the file-drop layer. 1.1.9 - On older Macs (Safari before 14.1) the window\'s controls now sit where they belong: labels inside their knobs, rows spaced, nothing in a corner. 1.1.8 - TREMOLO: with RAMP on, the TO box stays inside the tremolo panel.'

test('accent: manifest wins, registry falls back, purple last', () => {
  assert.equal(accentOf({ id: 'RoneThrow', accent: '#123456' }), '#123456')
  assert.equal(accentOf({ id: 'RoneThrow' }), '#D8E4EC')
  assert.equal(accentOf({ id: 'Unknown', accent: 'red' }), '#9D6BFF')
})

test('categories: manifest wins and is filtered to known keys', () => {
  assert.deepEqual(categoriesOf({ id: 'RoneIron', categories: ['space', 'bogus'] }), ['space'])
  assert.deepEqual(categoriesOf({ id: 'RoneIron' }), ['vocal'])
})

test('changelog splits versions and sentences', () => {
  const log = parseChangelog(RR_WHATS_NEW)
  assert.deepEqual(log.map(e => e.version), ['1.1.10', '1.1.9', '1.1.8'])
  assert.ok(log[0].items[0].startsWith('Mac standalone'))
  // the hyphen inside "title bar - a click" is not a version marker
  assert.equal(log[0].items.length, 1)
})

test('changes since the installed version', () => {
  const p = { whatsNew: RR_WHATS_NEW, installedVersion: '1.1.8.243' }
  assert.deepEqual(changesSince(p).map(e => e.version), ['1.1.10', '1.1.9'])
  assert.deepEqual(changesSince({ whatsNew: RR_WHATS_NEW, installedVersion: '?' }).map(e => e.version), ['1.1.10'])
})

test('a changelog without versions is one entry', () => {
  const log = parseChangelog('First public release.')
  assert.equal(log.length, 1)
  assert.equal(log[0].version, '')
})

test('search matches tags and every word', () => {
  const iron = { id: 'RoneIron', name: 'RONE Iron', description: 'Drop a vocal' }
  assert.ok(matchesSearch(iron, 'vocal chop'))
  assert.ok(matchesSearch(iron, 'IRON'))
  assert.ok(!matchesSearch(iron, 'delay'))
  assert.ok(matchesSearch({ id: 'RoneThrow', name: 'RONE Throw' }, 'dub'))
})

test('updates count only installable updates, never not-installed', () => {
  const plugins = [
    { id: 'a', status: 'update_available', owned: true },
    { id: 'b', status: 'not_installed', owned: true },
    { id: 'c', status: 'update_available' },
  ]
  assert.deepEqual(updatable(plugins, { licensed: false, signedIn: true }).map(p => p.id), ['a'])
  assert.deepEqual(updatable(plugins, { licensed: true, signedIn: true }).map(p => p.id), ['a', 'c'])
})

test('free plugins unlock with any account, not for a guest', () => {
  assert.ok(isUnlocked({ free: true }, { licensed: false, signedIn: true }))
  assert.ok(!isUnlocked({ free: true }, { licensed: false, signedIn: false }))
})

test('yours first: updates, then owned and installed, then installable, then new', () => {
  const now = Date.parse('2026-10-06')
  const plugins = [
    { id: 'locked', status: 'not_installed' },
    { id: 'new', status: 'not_installed', released: '2026-10-01' },
    { id: 'mine', status: 'up_to_date', owned: true },
    { id: 'upd', status: 'update_available', owned: true },
  ]
  const order = sortPlugins(plugins, { licensed: false, signedIn: true }, 'yours', now).map(p => p.id)
  assert.deepEqual(order, ['upd', 'mine', 'new', 'locked'])
  const guest = sortPlugins([...plugins, { id: 'free', free: true, status: 'not_installed' }],
                            { licensed: false, signedIn: false }, 'yours', now).map(p => p.id)
  assert.equal(guest[0], 'free')
})

test('new means released within 30 days', () => {
  const now = Date.parse('2026-10-06')
  assert.ok(isNew({ released: '2026-10-04' }, now))
  assert.ok(!isNew({ released: '2026-08-01' }, now))
  assert.ok(!isNew({}, now))
})

test('price: launch price during a sale, none for free', () => {
  assert.deepEqual(priceOf({ price: 39, launch_price: 29 }), { live: 29, regular: 39, onSale: true })
  assert.equal(priceOf({ price: 0, free: true }), null)
})

test('tip of the week is stable within a week', () => {
  const tips = [{ text: 'a' }, { text: 'b' }]
  const t1 = tipOfTheWeek(tips, Date.parse('2026-10-06T00:00:00Z'))
  const t2 = tipOfTheWeek(tips, Date.parse('2026-10-07T00:00:00Z'))
  assert.equal(t1, t2)
  assert.equal(tipOfTheWeek([], Date.now()), null)
})

test('versions and sizes', () => {
  assert.equal(compareVersions('1.1.10', '1.1.9'), 1)
  assert.equal(compareVersions('1.0', '1.0.0'), 0)
  assert.equal(formatSize(48 * 1024 * 1024), '48 MB')
  assert.equal(formatSize(0), '')
})
