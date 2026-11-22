// Every 3D unit must show WHOLE in the Center: on its plugin page and in My Rack, at the smallest
// window and a big one (22.11.2026: Listen and Clipper were cut at the top of their pages).
// The work is in tools/unit-fit-check.mjs. On CI (GitHub runners carry Chrome) a missing browser
// fails the test; on a PC without one it is skipped.
import test from 'node:test'
import assert from 'node:assert/strict'
import { runUnitFit } from '../tools/unit-fit-check.mjs'

test('every 3D unit fits whole inside its frame', { timeout: 180000 }, async (t) => {
  const r = await runUnitFit({ log: () => {} })
  if (r.skipped) {
    if (process.env.CI) assert.fail('unit-fit could not run on CI: ' + r.skipped)
    t.skip(r.skipped); return
  }
  assert.ok(r.rows.length > 0, 'no unit was measured')
  assert.deepEqual(r.bad.map(b => `${b.where} ${b.id}: ${b.why}`), [])
})
