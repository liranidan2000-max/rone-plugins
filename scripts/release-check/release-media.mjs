#!/usr/bin/env node
// RONE release-media: the moment a plugin is released, open its whole video + launch package on the broadcast
// board (לוח שידורים), dated from the release day, plus the T+7 re-check. Approved by Liran 22.11.2026
// (release-machine proposals "media" and "week"): "the only thing asked of you is a 30-second studio clip".
//
//   release-media.cmd --plugin RoneListen [--t0 2026-11-25] [--dry-run] [--only tutorial,short1,week]
//   keys: reel studio film carousel tutorial hero short1 tiktok presets short2 week
//   (film = the Higgsfield 3D launch film, launch-film.mjs; its approve step puts the 9:16 on this card)
//
// What it does (nothing here publishes anything; every card is a PLAN that waits for its maker):
//   1. reads the board for T0..T0+8 (schedule.mjs read --json) and picks FREE Instagram prime hours for the
//      feed cards (nothing within 90 min, same rule as the board itself);
//   2. adds the cards with `schedule.mjs add` (stable ids rel-<id>-<key>, so a rerun skips what is there),
//      each with the plugin's 3D render as its picture and the right --channel;
//   3. registers the one-off Windows task "RONE Release Week <Name>" at T0+7 10:00, which runs
//      release-week.mjs (the check again + the board cards + the first week's licences) and sends it to Liran
//      on Telegram. The board shows that task by itself (one-off "RONE ..." tasks are read from the PC).
// The tutorial follows the tutorial kit (Kokoro voice + take host, memory tutorials-second-wave / tutorial-take-host).

import { spawnSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';

const HERE = import.meta.dirname;
const MONO = path.resolve(HERE, '..', '..');
const SITE = 'D:/RONE PLUGINS/WEBSITE RONE AUDIO';
const NODE = process.execPath;
const SCHEDULE = path.join(SITE, 'scripts/schedule.mjs');
const a = process.argv.slice(2);
const opt = (k) => { const i = a.indexOf(k); return i >= 0 ? a[i + 1] : null; };
const DRY = a.includes('--dry-run');
const want = opt('--plugin');
if (!want) { console.error('צריך --plugin <Id>'); process.exit(2); }

const versions = JSON.parse(fs.readFileSync(path.join(MONO, 'versions.json'), 'utf8'));
const p = versions.plugins.find((x) => x.id.toLowerCase() === want.toLowerCase());
if (!p) { console.error(`${want} לא נמצא ב-versions.json`); process.exit(2); }
const slug = (p.store_url || '').match(/products\/([^/.]+)/)?.[1];
const thumb = [`graphics/renders/${slug}.jpg`, `graphics/og/${slug}.jpg`].map((f) => path.join(SITE, f)).find((f) => fs.existsSync(f));
const t0 = opt('--t0') || new Date().toLocaleDateString('sv-SE', { timeZone: 'Asia/Jerusalem' });
const day = (n) => { const d = new Date(`${t0}T12:00:00Z`); d.setUTCDate(d.getUTCDate() + n); return d.toISOString().slice(0, 10); };

// [key, day offset, preferred hours, channel, kind, title, note, feed slot?]
const CARDS = [
  ['reel', 0, ['20:00', '21:00', '18:00'], 'instagram', 'reel', 'ריל השקה', 'נוסחת ריל ההצעה (offer-reel-formula). תמונת השער מהיחידה.', true],
  ['studio', 0, ['13:00'], 'instagram', 'trial', 'ריל ניסיון מקליפ סטודיו', 'הבקשה היחידה מלירן: קליפ סטודיו של 30 שניות. מכונת הטריאלים בונה ממנו.', false],
  ['film', 1, ['18:00', '21:00', '20:00'], 'instagram', 'reel', 'סרטון השקה 3D (9:16)', 'launch-film.cmd: היחידה מתרכבת מהחלקים (Seedance 2.5 מחשבון ה-PLUS). ה-16:9 ליוטיוב ולעמוד המוצר. עולה רק באישור של לירן.', true],
  ['carousel', 1, ['20:00', '21:00', '18:00', '13:00'], 'instagram', 'carousel', 'קרוסלת NEW UPDATE', 'update-carousels.mjs (צריך שורה למוצר).', true],
  ['tutorial', 2, ['18:00'], 'youtube', 'reel', 'YouTube: מדריך', 'ערכת המדריכים: קריינות Kokoro + מארח הצילום. אחרי שעולה: השדה video ב-versions.json ובעמוד המוצר, ושורת ledger.', false],
  ['hero', 2, ['12:00'], 'site', 'release', 'קליפ גיבור לעמוד המוצר', 'קליפ קצר מהמדריך לראש עמוד המוצר באתר.', false],
  ['short1', 3, ['18:00'], 'youtube', 'reel', 'YouTube Short 1 מהמדריך', 'הקטע החזק ביותר מהמדריך, 9:16.', false],
  ['tiktok', 3, ['19:00'], 'tiktok', 'reel', 'TikTok: הדגמה', 'העלאה באיכות מלאה (tiktok-upload-full-quality).', false],
  ['presets', 4, ['18:00'], 'youtube', 'reel', 'YouTube: סרטון פריסטים', 'רק הצלילים של הפלאגין עצמו (plugin-sessions-series).', false],
  ['short2', 5, ['18:00'], 'youtube', 'reel', 'YouTube Short 2 מהמדריך', 'קטע שני מהמדריך, 9:16.', false],
];

function sched(...args) {
  const r = spawnSync(NODE, [SCHEDULE, ...args], { encoding: 'utf8', maxBuffer: 32 << 20 });
  if (r.status !== 0) throw new Error(`schedule.mjs ${args[0]}: ${(r.stderr || r.stdout).slice(0, 400)}`);
  return r.stdout;
}

const board = JSON.parse(sched('read', '--json', '--from', t0, '--days', '9'));
const ilMs = (d, t) => {   // Israel wall time -> epoch ms, summer and winter
  const guess = Date.parse(`${d}T${t}:00Z`);
  const wall = Date.parse(new Date(guess).toLocaleString('sv-SE', { timeZone: 'Asia/Jerusalem' }).replace(' ', 'T') + 'Z');
  return guess - (wall - guess);
};
const taken = board.items.filter((i) => i.channel === 'instagram' && i.kind !== 'story' && !['cancelled', 'idea'].includes(i.status)).map((i) => i.at);
const have = new Set(board.items.map((i) => String(i.id).replace(/^item:/, '')));

const only = opt('--only')?.split(',');            // backfill of an already released plugin: just the missing ones
const planned = [];
for (const [key, off, hours, channel, kind, title, note, feed] of CARDS) {
  if (only && !only.includes(key)) continue;
  const id = `rel-${p.id}-${key}`;
  if (have.has(id) || board.items.some((i) => String(i.id).endsWith(id))) { console.log(`· כבר בלוח: ${title}`); continue; }
  let at = null;
  for (let extra = 0; extra < 4 && !at; extra++) {           // a full day moves the card one day later
    const d = day(off + extra);
    const hit = hours.find((h) => !feed || !taken.some((x) => Math.abs(x - ilMs(d, h)) < 90 * 60_000));
    if (hit) at = `${d} ${hit}`;
  }
  if (feed) taken.push(ilMs(...at.split(' ')));
  const args = ['add', '--id', id, '--title', `${p.name} · ${title}`, '--at', at, '--kind', kind, '--status', 'planned',
    '--product', p.id, '--note', note];
  if (channel !== 'instagram') args.push('--channel', channel);
  if (thumb) args.push('--thumb', thumb);
  planned.push({ title, at, args });
}

for (const c of planned) {
  console.log(`${DRY ? '[ניסיון] ' : ''}+ ${c.at}  ${c.title}`);
  if (!DRY) sched(...c.args);
}

// T+7: one-off Windows task that the board reads by itself.
const weekAt = `${day(7)} 10:00`;
if (only && !only.includes('week')) { console.log(DRY ? '\nניסיון בלבד: שום דבר לא נכתב.' : `\nנפתחו ${planned.length} כרטיסים.`); process.exit(0); }
const taskName = `RONE Release Week ${p.name.replace(/^RONE /, '')}`;
const cmdDir = path.join(MONO, 'scripts/release-check/week-tasks');
const cmdFile = path.join(cmdDir, `${p.id}.cmd`);
const cmd = [
  '@echo off',
  `rem board: media=${thumb || ''} at=${weekAt} kind=release`,
  `rem ${taskName}: T+7 re-check of ${p.name} (release-machine "week"). Deletes its own task when done.`,
  `"${NODE}" "${path.join(HERE, 'release-week.mjs')}" --plugin ${p.id} --t0 ${t0}`,
  `schtasks /Delete /TN "${taskName}" /F >nul 2>&1`,
  '',
].join('\r\n');
console.log(`${DRY ? '[ניסיון] ' : ''}+ ${weekAt}  בדיקה חוזרת אחרי שבוע (משימת Windows "${taskName}")`);
if (!DRY) {
  fs.mkdirSync(cmdDir, { recursive: true });
  fs.writeFileSync(cmdFile, cmd);
  const [d, t] = weekAt.split(' ');
  const ps = `$a=New-ScheduledTaskAction -Execute 'cmd.exe' -Argument '/c ""${cmdFile}""'; ` +
    `$t=New-ScheduledTaskTrigger -Once -At ([datetime]'${d}T${t}:00'); ` +
    `$s=New-ScheduledTaskSettingsSet -StartWhenAvailable -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries; ` +
    `Register-ScheduledTask -TaskName '${taskName}' -Action $a -Trigger $t -Settings $s -Force | Out-Null`;
  const r = spawnSync('powershell.exe', ['-NoProfile', '-Command', ps], { encoding: 'utf8' });
  if (r.status !== 0) console.error(`המשימה לא נרשמה: ${r.stderr}`);
}
console.log(DRY ? '\nניסיון בלבד: שום דבר לא נכתב.' : `\nנפתחו ${planned.length} כרטיסים ובדיקה אחרי שבוע. הכל ממתין לך או לי בלוח השידורים.`);
