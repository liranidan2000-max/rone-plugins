// node --test (npm test): the committed dist/ is the build of the committed src/.
// See tools/src-hash.mjs for why (audit R032, R197).
import test from 'node:test'
import assert from 'node:assert/strict'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'
import { srcHash } from '../tools/src-hash.mjs'

const ui = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..')

test('dist/ was built from this src/ (else: npm run build, and commit dist/ with it)', () => {
  const stamp = path.join(ui, 'dist', 'src-hash.txt')
  assert.ok(fs.existsSync(stamp), 'dist/src-hash.txt is missing - run npm run build')
  assert.equal(fs.readFileSync(stamp, 'utf8').trim(), srcHash(ui),
               'Resources/ui/dist is older than src/ - run npm run build and commit dist/')
})
