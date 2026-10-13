// node --test (npm test): what a message from the Center's backend says on the
// page (src/messages.js) - audit R032, R195, R246, R281, R120.
import test from 'node:test'
import assert from 'node:assert/strict'
import { translateStatus, installErrorText, isSticky } from '../src/messages.js'
import { makeT } from '../src/i18n.js'

const en = makeT('en')
const pt = makeT('pt')

test('an install that finished is announced by the page itself', () => {
  assert.equal(translateStatus({ text: 'x installed', type: 'success', code: 'installed', params: { id: 'RoneRise' } }, en), null)
})

test("'failed' with no installer exit code shows the Center's own words (not 'installer code 0')", () => {
  const m = { text: 'Install failed - could not write to ~/Library. Nothing was changed.', type: 'error', code: 'failed',
              params: { name: 'RONE Rise', exitCode: 0 } }
  assert.equal(translateStatus(m, en).text, m.text)
  // Windows' own codes are below zero and say why too (-3 copy, -5 start, -6 hung before it)
  const w = { text: 'RONE Rise: Windows could not start the installer (often antivirus).', type: 'error', code: 'failed',
              params: { name: 'RONE Rise', exitCode: -5 } }
  assert.equal(translateStatus(w, pt).text, w.text)
})

test("'failed' with a real exit code is translated, and the installer's reason follows it", () => {
  const m = { text: 'Install failed (installer code 1). (no disk)', type: 'error', code: 'failed',
              params: { name: 'RONE Rise', exitCode: 1, reason: 'installer: Cannot install on this volume.' } }
  assert.equal(translateStatus(m, en).text, 'RONE Rise could not be installed (installer code 1). installer: Cannot install on this volume.')
  const plain = { ...m, params: { name: 'RONE Rise', exitCode: 1 } }
  assert.equal(translateStatus(plain, en).text, 'RONE Rise could not be installed (installer code 1).')
})

test('the new install outcomes speak the user language (restart, aborted, sign in)', () => {
  assert.match(translateStatus({ text: 'x', type: 'error', code: 'restart_needed', params: { name: 'RONE Iron' } }, pt).text, /reiniciar/)
  assert.match(translateStatus({ text: 'x', type: 'error', code: 'install_aborted', params: { name: 'RONE Iron' } }, pt).text, /^RONE Iron: o instalador parou/)
  assert.equal(translateStatus({ text: 'x', type: 'error', code: 'sign_in_required', params: { name: 'RONE Iron' } }, en).text,
               'Sign in to install RONE Iron.')
})

test('many installs while the window was closed read as one line', () => {
  const x = translateStatus({ text: '3 plugins installed: A, B, C.', type: 'success', code: 'installed_many',
                              params: { count: 3, names: 'A, B, C' } }, en)
  assert.equal(x.type, 'success')
  assert.match(x.text, /^3 plugins installed: A, B, C\./)
})

test('a code the page does not know, and download_failed, keep the backend text', () => {
  assert.equal(translateStatus({ text: 'Something new', type: 'error', code: 'no_such_code' }, en).text, 'Something new')
  assert.equal(translateStatus({ text: 'Download failed - x', type: 'error', code: 'download_failed', params: { name: 'n' } }, en).text,
               'Download failed - x')
  assert.deepEqual(translateStatus({ text: 'plain', type: 'info' }, en), { text: 'plain', type: 'info', sticky: false })
})

test('declined is information, not an error', () => {
  assert.equal(translateStatus({ text: 'x', type: 'error', code: 'declined', params: {} }, en).type, 'info')
})

test("a 'To fix it' line stays until it is closed", () => {
  assert.equal(isSticky('Install failed - x. To fix it: in Finder, Get Info on the folder'), true)
  assert.equal(isSticky('Install failed'), false)
  const m = { text: 'Install failed - x. Nothing was changed. To fix it: in Finder ...', type: 'error', code: 'failed', params: { exitCode: 0 } }
  assert.equal(translateStatus(m, en).sticky, true)
})

test('installPlugin errors: licence vs sign in', () => {
  assert.match(installErrorText('License required', 'RONE Throw', en), /not unlocked on this account/)
  assert.equal(installErrorText('Sign in required', 'RONE Throw', en), 'Sign in to install RONE Throw.')
  assert.equal(installErrorText('Already on its way', 'RONE Throw', en), 'Already on its way')
})
