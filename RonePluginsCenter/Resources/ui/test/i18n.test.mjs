import test from 'node:test'
import assert from 'node:assert/strict'
import { _tables, makeT } from '../src/i18n.js'

const placeholders = (s) => (Array.isArray(s) ? s.join(' ') : String(s)).match(/\{\w+\}/g)?.sort() || []

test('every English string exists in Portuguese and Spanish, with the same placeholders', () => {
  const en = _tables.en
  for (const lang of ['pt', 'es']) {
    const table = _tables[lang]
    const missing = Object.keys(en).filter(k => !(k in table))
    assert.deepEqual(missing, [], `${lang} is missing: ${missing.join(', ')}`)
    for (const k of Object.keys(en)) {
      assert.deepEqual(placeholders(table[k]), placeholders(en[k]), `${lang} ${k} placeholders`)
      assert.equal(Array.isArray(table[k]), Array.isArray(en[k]), `${lang} ${k} shape`)
    }
  }
})

test('t fills placeholders and falls back to English, then to the key', () => {
  const t = makeT('pt')
  assert.equal(t('upd.button', { n: 3 }), 'Atualizar 3')
  assert.equal(makeT('xx')('nav.library'), 'Library')
  assert.equal(t('no.such.key'), 'no.such.key')
  assert.deepEqual(makeT('en')('steps.cubase', { name: 'RONE Throw' })[1], 'Insert slot › RONE › RONE Throw')
})
