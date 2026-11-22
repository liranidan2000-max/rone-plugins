#!/usr/bin/env node
// RONE release-week: the T+7 re-check of a released plugin (release-machine "week", approved 22.11.2026).
// Started by the one-off Windows task "RONE Release Week <Name>" that release-media.mjs registers.
//   release-week.mjs --plugin RoneListen --t0 2026-11-25 [--no-send]
// Sends Liran ONE Telegram message: what release-check still finds missing, which launch cards went up and which
// are stuck on the board, and the first week's licences of the plugin (D1, read-only). Changes nothing.

import { spawnSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import { pathToFileURL } from 'node:url';

const HERE = import.meta.dirname;
const MONO = path.resolve(HERE, '..', '..');
const SITE = 'D:/RONE PLUGINS/WEBSITE RONE AUDIO';
const NODE = process.execPath;
const TELEGRAM = 'D:/RONE PLUGINS/_daw-lab/test-bench/bench/lib/telegram.mjs';
const a = process.argv.slice(2);
const opt = (k) => { const i = a.indexOf(k); return i >= 0 ? a[i + 1] : null; };
const id = opt('--plugin');
const t0 = opt('--t0');
if (!id || !t0) { console.error('צריך --plugin <Id> --t0 YYYY-MM-DD'); process.exit(2); }
const versions = JSON.parse(fs.readFileSync(path.join(MONO, 'versions.json'), 'utf8'));
const p = versions.plugins.find((x) => x.id === id) || { id, name: id };
const run = (args, cwd) => spawnSync(NODE, args, { cwd, encoding: 'utf8', maxBuffer: 32 << 20 });
const L = [`${p.name}: שבוע אחרי השחרור (${t0})`];

// 1. the check again
const jsonOut = path.join(os.tmpdir(), `release-week-${id}.json`);
run([path.join(HERE, 'release-check.mjs'), '--plugin', id, '--json', jsonOut, '--quiet'], MONO);
try {
  const r = JSON.parse(fs.readFileSync(jsonOut, 'utf8'));
  const st = r.products[0].stations;
  const miss = Object.entries(st).filter(([, v]) => v.s === 'missing');
  const part = Object.entries(st).filter(([, v]) => v.s === 'warn');
  L.push(miss.length ? `✖ עדיין חסר (${miss.length}):` : '✔ הבדיקה ירוקה: כל התחנות במקום.');
  const name = (k) => r.labels?.[k] || k;
  for (const [k, v] of miss) L.push(`  • ${name(k)}: ${v.why}`);
  if (part.length) L.push(`⚠ חלקי: ${part.map(([k]) => name(k)).join(', ')}`);
} catch { L.push('? הבדיקה לא רצה'); }

// 2. the launch cards on the board
const s = run([path.join(SITE, 'scripts/schedule.mjs'), 'read', '--json', '--from', t0, '--days', '9'], SITE);
try {
  const items = JSON.parse(s.stdout).items.filter((i) => i.product === id || String(i.id).includes(`rel-${id}-`));
  const up = items.filter((i) => i.status === 'published');
  const stuck = items.filter((i) => i.status !== 'published' && i.status !== 'cancelled' && i.at < Date.now());
  L.push(`לוח השידורים: ${up.length} עלו, ${stuck.length} תקועים${stuck.length ? `: ${stuck.map((i) => i.title.replace(`${p.name} · `, '')).join(', ')}` : ''}`);
} catch { L.push('? לא הצלחתי לקרוא את לוח השידורים'); }

// 3. the first week's licences (read-only D1)
const since = Math.floor(Date.parse(`${t0}T00:00:00+02:00`) / 1000);
const sql = `SELECT source, COUNT(*) AS n FROM entitlements WHERE product='${id.replace(/'/g, '')}' AND (CASE WHEN created_at > 100000000000 THEN created_at/1000 ELSE created_at END) >= ${since} GROUP BY source`;
const d1 = run([path.join(SITE, 'node_modules/wrangler/bin/wrangler.js'), 'd1', 'execute', 'rone-accounts', '--remote', '--json', '--command', sql], SITE);
try {
  const rows = JSON.parse(d1.stdout)[0].results;
  const total = rows.reduce((n, r) => n + r.n, 0);
  const SRC = { paddle: 'קנייה', giveaway: 'מתנה מהגרלה', comp: 'הענקה מאיתנו', gift: 'מתנה', trial: 'ניסיון' };
  L.push(`רישיונות חדשים בשבוע: ${total}${rows.length ? ` (${rows.map((r) => `${SRC[r.source] || r.source} ${r.n}`).join(', ')})` : ''}`);
} catch { L.push('? לא הצלחתי לקרוא מכירות מ-D1'); }

L.push('', 'הפירוט המלא: release-check.cmd --plugin ' + id);
const msg = L.join('\n');
console.log(msg);
if (!a.includes('--no-send')) {
  const { sendText } = await import(pathToFileURL(TELEGRAM).href);
  await sendText(msg);
}
