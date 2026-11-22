// unit-fit-check: every 3D unit in Resources/units must sit WHOLE inside the frame that shows it.
//
// 22.11.2026: RONE Listen's unit (1:2, a phone) was cut at the top of its own page in the Center,
// and so was Clipper's (1:2.17). The picture had a % max-height inside a grid row of auto height,
// which the browser ignores, so a tall unit grew past the 560 px box. A wide unit never showed it.
// This check renders the real page (the vite dev server, the real CSS) in a headless Chrome / Edge,
// puts EVERY unit file into every frame marked `data-unit-frame` (the plugin page and My Rack), at
// the smallest Center window (980 x 620) and a big one, and fails when any edge of a unit is outside
// the nearest box that clips it. A new tall or wide unit is therefore checked before it ships.
//
//   node tools/unit-fit-check.mjs          prints a table, exit 1 on any cut unit
//   test/unit-fit.test.mjs                 runs it inside `npm test` (the Center's CI step)
//
// No Playwright: it talks to the browser over the DevTools protocol with Node's own WebSocket.

import { spawn } from 'node:child_process'
import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const here = path.dirname(fileURLToPath(import.meta.url))
const uiRoot = path.resolve(here, '..')
const unitsDir = path.resolve(uiRoot, '..', 'units')

export const VIEWPORTS = [{ w: 980, h: 620 }, { w: 1600, h: 1000 }]

export function findBrowser () {
  const env = process.env.UNIT_FIT_BROWSER
  const list = [
    env,
    'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe',
    'C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe',
    'C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe',
    'C:\\Program Files\\Microsoft\\Edge\\Application\\msedge.exe',
    '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',
    '/Applications/Microsoft Edge.app/Contents/MacOS/Microsoft Edge',
    '/usr/bin/google-chrome', '/usr/bin/google-chrome-stable', '/usr/bin/chromium', '/usr/bin/chromium-browser',
  ]
  return list.find(p => p && fs.existsSync(p)) || null
}

export function unitIds () {
  return fs.readdirSync(unitsDir).map(f => /^unit_(.+)\.webp$/.exec(f)).filter(Boolean).map(m => m[1]).sort()
}

function launch (exe) {
  const profile = fs.mkdtempSync(path.join(os.tmpdir(), 'unit-fit-'))
  const proc = spawn(exe, ['--headless=new', '--remote-debugging-port=0', `--user-data-dir=${profile}`,
    '--no-first-run', '--no-default-browser-check', '--disable-gpu', '--hide-scrollbars', 'about:blank'],
  { stdio: ['ignore', 'ignore', 'pipe'] })
  return new Promise((resolve, reject) => {
    let buf = ''
    const t = setTimeout(() => reject(new Error('the browser did not open its DevTools port in 20 s')), 20000)
    proc.stderr.on('data', d => {
      buf += d
      const m = /DevTools listening on (ws:\/\/\S+)/.exec(buf)
      if (m) { clearTimeout(t); resolve({ proc, ws: m[1], profile }) }
    })
    proc.on('exit', c => { clearTimeout(t); reject(new Error(`the browser quit (${c}) before it was ready`)) })
  })
}

function cdp (url) {
  const ws = new WebSocket(url)
  let id = 0
  const wait = new Map()
  ws.onmessage = e => {
    const m = JSON.parse(e.data)
    if (m.id && wait.has(m.id)) { const w = wait.get(m.id); wait.delete(m.id); m.error ? w.rej(new Error(m.error.message)) : w.res(m.result) }
  }
  const send = (method, params = {}, sessionId) => new Promise((res, rej) => {
    const n = ++id; wait.set(n, { res, rej }); ws.send(JSON.stringify({ id: n, method, params, sessionId }))
  })
  return new Promise((res, rej) => { ws.onopen = () => res({ send, close: () => ws.close() }); ws.onerror = () => rej(new Error('DevTools socket failed')) })
}

// Runs inside the page: put each unit into every [data-unit-frame] picture and measure it against the
// nearest ancestor that clips (overflow not visible). Returns one row per frame x unit.
const MEASURE = async (ids, where) => {
  const sleep = ms => new Promise(r => setTimeout(r, ms))
  const clipBox = el => { for (let a = el.parentElement; a; a = a.parentElement) { const s = getComputedStyle(a); if (s.overflow !== 'visible' || s.overflowX !== 'visible' || s.overflowY !== 'visible') return a } return document.documentElement }
  const frames = [...document.querySelectorAll('img[data-unit-frame]')].slice(0, 1)
  const rows = []
  for (const img of frames) {
    img.loading = 'eager'
    for (const id of ids) {
      await new Promise(res => { img.onload = res; img.onerror = res; img.src = `/units/${id}.webp` })
      await sleep(30)
      if (!img.naturalWidth) { rows.push({ where, id, ok: false, why: 'did not load' }); continue }
      const b = clipBox(img).getBoundingClientRect(); const r = img.getBoundingClientRect()
      const cut = { top: b.top - r.top, bottom: r.bottom - b.bottom, left: b.left - r.left, right: r.right - b.right }
      const over = Object.entries(cut).filter(([, v]) => v > 0.5).map(([k, v]) => `${k} ${Math.round(v)}px`)
      rows.push({ where, id, ok: over.length === 0, size: `${Math.round(r.width)}x${Math.round(r.height)}`, box: `${Math.round(b.width)}x${Math.round(b.height)}`, why: over.join(', ') })
    }
  }
  return rows
}

export async function runUnitFit ({ log = console.log } = {}) {
  const exe = findBrowser()
  if (!exe) return { skipped: 'no Chrome or Edge found (set UNIT_FIT_BROWSER)' }
  const { createServer } = await import('vite')
  const server = await createServer({ root: uiRoot, configFile: path.join(uiRoot, 'vite.config.js'), logLevel: 'silent', server: { port: 0, host: '127.0.0.1' } })
  await server.listen()
  const base = server.resolvedUrls.local[0]
  const ids = unitIds()
  let browser
  const rows = []
  try {
    browser = await launch(exe)
    const c = await cdp(browser.ws)
    const { targetId } = await c.send('Target.createTarget', { url: 'about:blank' })
    const { sessionId: s } = await c.send('Target.attachToTarget', { targetId, flatten: true })
    const ev = async (expr) => {
      const r = await c.send('Runtime.evaluate', { expression: expr, awaitPromise: true, returnByValue: true }, s)
      if (r.exceptionDetails) throw new Error(r.exceptionDetails.exception?.description || r.exceptionDetails.text)
      return r.result.value
    }
    const until = async (expr, what, ms = 20000) => {
      const end = Date.now() + ms
      while (Date.now() < end) { if (await ev(expr)) return; await new Promise(r => setTimeout(r, 150)) }
      throw new Error(`timed out waiting for ${what}`)
    }
    for (const vp of VIEWPORTS) {
      await c.send('Emulation.setDeviceMetricsOverride', { width: vp.w, height: vp.h, deviceScaleFactor: 1, mobile: false }, s)
      await c.send('Page.navigate', { url: base + '?all' }, s)
      await until(`[...document.querySelectorAll('button')].some(b => b.textContent.trim() === 'My Rack')`, 'the Center page')
      // My Rack: the slot pictures, then a slot opens that plugin's own page.
      await ev(`[...document.querySelectorAll('button')].find(b => b.textContent.trim() === 'My Rack').click()`)
      await until(`!!document.querySelector('img[data-unit-frame]')`, 'My Rack')
      rows.push(...await ev(`(${MEASURE})(${JSON.stringify(ids)}, 'My Rack ${vp.w}x${vp.h}')`))
      await ev(`document.querySelector('img[data-unit-frame]').closest('button').click()`)
      await until(`!!document.querySelector('img[data-unit-frame][alt^="RONE"]')`, 'the plugin page')
      rows.push(...await ev(`(${MEASURE})(${JSON.stringify(ids)}, 'plugin page ${vp.w}x${vp.h}')`))
    }
    c.close()
  } finally {
    if (browser) { browser.proc.kill(); try { fs.rmSync(browser.profile, { recursive: true, force: true }) } catch {} }
    await server.close()
  }
  const bad = rows.filter(r => !r.ok)
  for (const r of rows) if (!r.ok) log(`CUT  ${r.where.padEnd(26)} ${r.id.padEnd(16)} ${r.size || ''} in ${r.box || ''}: ${r.why}`)
  log(`unit-fit: ${rows.length - bad.length}/${rows.length} fit (${ids.length} units x ${rows.length / ids.length} frames)`)
  return { rows, bad }
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  runUnitFit().then(r => {
    if (r.skipped) { console.log('unit-fit: skipped -', r.skipped); process.exit(0) }
    process.exit(r.bad.length ? 1 : 0)
  }, e => { console.error('unit-fit:', e.message); process.exit(2) })
}
