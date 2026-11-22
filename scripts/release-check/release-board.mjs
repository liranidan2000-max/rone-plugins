#!/usr/bin/env node
// RONE release board: the release-check, drawn as ONE page Liran reads (Hebrew, RTL) - per plugin, every station as a
// dot: green = done, red = missing and Claude completes it, amber = waits for Liran, grey = not relevant.
// Liran 22.11.2026: "מהיום שאנחנו משחררים משהו, אתה פשוט תעשה הדמיה, שמציגה באדום, ירוק, נקודות, על הפלאגין
// ספציפית, מה בוצע, מה לא בוצע ומה נשאר לי". Every release ends with this page published as an Artifact
// (skill rone-release, step T0.8), and the retro board of all products is the same page without --plugin.
//
//   node release-board.mjs                         every product in versions.json
//   node release-board.mjs --plugin RoneFilter     one plugin (id or folder; a folder not in versions.json = in DEV)
//   node release-board.mjs --json rc.json          draw an existing release-check --json instead of running it
//   node release-board.mjs --out board.html        where to write (default: release-board.html next to the cwd)
//
// Read-only like release-check: it runs it, reads its JSON and writes one HTML file. Nothing else.
import { spawnSync } from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const args = process.argv.slice(2);
const opt = (k) => { const i = args.indexOf(k); return i >= 0 ? args[i + 1] : null; };
const plugin = opt('--plugin');
let jsonPath = opt('--json');
const out = path.resolve(opt('--out') || 'release-board.html');

if (!jsonPath) {
  jsonPath = path.join(os.tmpdir(), `rone-release-board-${process.pid}.json`);
  const rc = ['release-check.mjs', '--json', jsonPath, ...(plugin ? ['--plugin', plugin] : [])];
  const r = spawnSync(process.execPath, rc, { cwd: HERE, encoding: 'utf8', stdio: ['ignore', 'pipe', 'inherit'] });
  if (!fs.existsSync(jsonPath)) { console.error('release-check gave no JSON (exit ' + r.status + ')'); process.exit(2); }
}
const rep = JSON.parse(fs.readFileSync(jsonPath, 'utf8'));
const LABEL = rep.labels;

// The three parts of the line, in the order a release walks them (skill rone-release 1-3).
const GROUPS = [
  ['בנייה', ['versions', 'rack', 'maps', 'crash', 'words', 'installer', 'hands', 'ci', 'center', 'bench', 'presets', 'manual', 'manualControl']],
  ['אתר', ['unit', 'page', 'price', 'home', 'demo', 'lists']],
  ['סרטונים והשקה', ['tutorial', 'pvid', 'shorts', 'tiktok', 'film', 'reel', 'carousel', 'trial']],
];

// Who closes a station that is not green. Liran's: what only he can do (Paddle, his own audio / studio clip, his word
// to make a video public, Higgsfield PLUS credits). Everything else Claude completes - posting still with his OK.
function owner(key, r) {
  const why = r.why || '';
  if (key === 'price') return ['you', 'המחיר נקבע ב-Paddle רק על ידך (פקודה מוכנה ממני)'];
  if (/ממך/.test(why)) return ['you', ''];
  if (key === 'tutorial' && /לא-רשום/.test(why)) return ['you', 'מחכה למילה שלך לפרסם לכולם'];
  if (key === 'reel' && /קליפ סטודיו/.test(why)) return ['you', 'קליפ של 30 שניות מהאולפן'];
  if (key === 'film') return ['you', 'נוצר מחשבון ה-PLUS שלך ב-Higgsfield: צריך קרדיטים שם'];
  return ['me', ['tiktok', 'shorts', 'carousel', 'trial', 'reel', 'pvid'].includes(key) ? 'אני מכין, עולה רק באישור שלך' : ''];
}
const state = (key, r) => {
  if (r.s === 'ok') return 'ok';
  if (r.s === 'na') return 'na';
  return owner(key, r)[0];
};

const esc = (s) => String(s ?? '').replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' })[c]);
const dateHe = new Date(rep.at).toLocaleString('he-IL', { timeZone: 'Asia/Jerusalem', dateStyle: 'medium', timeStyle: 'short' });

const products = rep.products.map((p) => {
  const rows = rep.stations.filter((k) => p.stations[k]).map((k) => {
    const r = p.stations[k];
    const st = state(k, r);
    const [, hint] = r.s === 'ok' || r.s === 'na' ? [null, ''] : owner(k, r);
    return { k, label: LABEL[k] || k, st, why: r.why || '', where: r.where || '', hint, raw: r.s };
  });
  const n = (s) => rows.filter((x) => x.st === s).length;
  return { ...p, rows, ok: n('ok'), me: n('me'), you: n('you'), na: n('na') };
}).sort((a, b) => (b.me + b.you) - (a.me + a.you) || (b.me - a.me));

const T = { ok: 0, me: 0, you: 0, na: 0 };
for (const p of products) for (const s of Object.keys(T)) T[s] += p[s];
const single = products.length === 1 ? products[0] : null;
const title = single ? `שחרור ${single.name}` : 'לוח השחרור';
const globalBad = Object.entries(rep.global || {}).filter(([, g]) => g.s !== 'ok');

const dot = (x) => `<i class="dot ${x.st}" title="${esc(x.label)}${x.why ? ': ' + esc(x.why) : ''}"></i>`;
const item = (x, p) => `<li class="it ${x.st}">${p ? `<b class="pn">${esc(p.name.replace(/^RONE /, ''))}</b>` : ''}<span class="lb">${esc(x.label)}</span>${x.why && x.raw !== 'ok' ? `<span class="wy">${esc(x.why)}</span>` : ''}${x.hint ? `<span class="hn">${esc(x.hint)}</span>` : ''}</li>`;

function card(p) {
  const strip = GROUPS.map(([g, keys]) => {
    const xs = keys.map((k) => p.rows.find((x) => x.k === k)).filter(Boolean);
    return `<div class="grp"><span class="gl">${g}</span><span class="dots">${xs.map(dot).join('')}</span></div>`;
  }).join('');
  const list = (s) => p.rows.filter((x) => x.st === s);
  const you = list('you'), me = list('me'), done = list('ok'), na = list('na');
  const verdict = !you.length && !me.length ? '<span class="pill ok">הכל בוצע</span>'
    : `${me.length ? `<span class="pill me">${me.length} חסר</span>` : ''}${you.length ? `<span class="pill you">${you.length} מחכה לך</span>` : ''}`;
  return `<article class="card" id="${esc(p.id)}">
  <header><h2>${esc(p.name)}${p.dev ? ' <small class="dev">בפיתוח</small>' : ''}</h2><span class="ver">${esc((p.rows.find((x) => x.k === 'versions') || {}).why || '')}</span><span class="verdict">${verdict}</span></header>
  <div class="strip">${strip}</div>
  ${you.length ? `<section class="sec you"><h3>מחכה לך</h3><ul>${you.map((x) => item(x)).join('')}</ul></section>` : ''}
  ${me.length ? `<section class="sec me"><h3>חסר: אני משלים</h3><ul>${me.map((x) => item(x)).join('')}</ul></section>` : ''}
  <details class="sec ok"><summary>בוצע (${done.length})${na.length ? ` · לא רלוונטי (${na.length})` : ''}</summary><ul>${done.map((x) => item(x)).join('')}${na.map((x) => item(x)).join('')}</ul></details>
</article>`;
}

const youAll = products.flatMap((p) => p.rows.filter((x) => x.st === 'you').map((x) => item(x, p)));

const html = `<title>${esc(title)}</title>
<meta name="claude:viewport-fit-sides" content="cover">
<link rel="preconnect" href="https://fonts.googleapis.com"><link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Heebo:wght@400;500;700&family=Rubik:wght@500;700&family=IBM+Plex+Mono:wght@500&display=swap">
<style>
/* Layout: a summary band, the "waits for you" list, then one card per plugin, worst first; every station is a dot. */
:root{--bg:#F3F4F6;--panel:#FFFFFF;--fg:#15171C;--muted:#5D6370;--line:#DFE2E8;--ok:#1E9E5A;--me:#D7263D;--you:#D98A00;--na:#B9BEC8;--accent:#FF2E4D;
  --display:'Rubik','Heebo',system-ui,sans-serif;--text:'Heebo',system-ui,sans-serif;--mono:'IBM Plex Mono',ui-monospace,monospace}
@media (prefers-color-scheme:dark){:root:not([data-theme="light"]){--bg:#0D0F13;--panel:#161920;--fg:#EEF0F4;--muted:#9AA1AE;--line:#272B35;--ok:#3CCB7F;--me:#FF5468;--you:#FFB534;--na:#4A505C;color-scheme:dark}}
:root[data-theme="dark"]{--bg:#0D0F13;--panel:#161920;--fg:#EEF0F4;--muted:#9AA1AE;--line:#272B35;--ok:#3CCB7F;--me:#FF5468;--you:#FFB534;--na:#4A505C;color-scheme:dark}
body{background:var(--bg);color:var(--fg);font:15px/1.55 var(--text)}
.wrap{max-width:1080px;margin:0 auto;padding-block:28px 60px;padding-left:max(16px,env(safe-area-inset-left,0px));padding-right:max(16px,env(safe-area-inset-right,0px));display:grid;gap:22px}
h1{font:700 clamp(26px,4vw,36px)/1.15 var(--display);margin:0;text-wrap:balance}
.sub{color:var(--muted);margin:4px 0 0}
.sum{display:flex;flex-wrap:wrap;gap:10px}
.tot{display:flex;align-items:center;gap:8px;background:var(--panel);border:1px solid var(--line);border-radius:10px;padding:10px 14px;font-weight:500}
.tot b{font:700 22px var(--display);font-variant-numeric:tabular-nums}
.legend{display:flex;flex-wrap:wrap;gap:14px;color:var(--muted);font-size:13px}
.legend span{display:inline-flex;align-items:center;gap:6px}
.dot{display:inline-block;width:12px;height:12px;border-radius:50%;background:var(--na);flex:none}
.dot.ok{background:var(--ok)}.dot.me{background:var(--me)}.dot.you{background:var(--you)}.dot.na{background:transparent;box-shadow:inset 0 0 0 2px var(--na)}
.youall{background:var(--panel);border:1px solid var(--line);border-inline-start:4px solid var(--you);border-radius:12px;padding:16px 18px}
.youall h2{font:700 19px var(--display);margin:0 0 8px}
ul{list-style:none;margin:0;padding:0;display:grid;gap:6px}
.it{display:flex;flex-wrap:wrap;align-items:baseline;gap:4px 10px;padding-inline-start:20px;position:relative}
.it::before{content:"";position:absolute;inset-inline-start:0;top:.45em;width:10px;height:10px;border-radius:50%;background:var(--na)}
.it.ok::before{background:var(--ok)}.it.me::before{background:var(--me)}.it.you::before{background:var(--you)}.it.na::before{background:transparent;box-shadow:inset 0 0 0 2px var(--na)}
.pn{font-weight:700}.lb{font-weight:500}.wy{color:var(--muted);font-size:14px;min-width:0}.hn{font-size:13px;color:var(--you);font-weight:500}
.it.me .hn{color:var(--muted)}
.cards{display:grid;grid-template-columns:repeat(auto-fill,minmax(min(100%,500px),1fr));gap:16px;align-items:start}
.card{background:var(--panel);border:1px solid var(--line);border-radius:12px;padding:16px 18px;display:grid;gap:12px;min-width:0}
.card header{display:flex;flex-wrap:wrap;align-items:baseline;gap:6px 12px}
.card h2{font:700 19px var(--display);margin:0}
.dev{font:500 12px var(--text);color:var(--you);border:1px solid var(--you);border-radius:20px;padding:1px 8px;vertical-align:middle}
.ver{font:500 13px var(--mono);color:var(--muted);direction:ltr}
.verdict{margin-inline-start:auto;display:flex;gap:6px}
.pill{font-size:12.5px;font-weight:700;border-radius:20px;padding:2px 10px;color:#fff}
.pill.ok{background:var(--ok)}.pill.me{background:var(--me)}.pill.you{background:var(--you);color:#1d1400}
.strip{display:grid;gap:6px}
.grp{display:flex;align-items:center;gap:10px}
.gl{width:92px;flex:none;font-size:12px;letter-spacing:.02em;color:var(--muted)}
.dots{display:flex;flex-wrap:wrap;gap:5px}
.sec h3{font:700 14px var(--display);margin:0 0 6px}
.sec.you h3{color:var(--you)}.sec.me h3{color:var(--me)}
details.sec summary{cursor:pointer;color:var(--muted);font-size:14px;font-weight:500}
details.sec ul{margin-top:8px}
summary:focus-visible,a:focus-visible{outline:2px solid var(--accent);outline-offset:2px}
.foot{color:var(--muted);font-size:13px}
code{font-family:var(--mono);font-size:12.5px;direction:ltr;unicode-bidi:embed}
</style>
<div class="wrap" dir="rtl" lang="he">
  <header>
    <h1>${esc(title)}</h1>
    <p class="sub">בדיקת השלמות של מכונת השחרור · ${esc(dateHe)} · ${products.length} ${products.length === 1 ? 'פלאגין' : 'פלאגינים'} × ${rep.stations.length} תחנות</p>
  </header>
  <div class="sum">
    <span class="tot"><i class="dot ok"></i><b>${T.ok}</b> בוצע</span>
    <span class="tot"><i class="dot me"></i><b>${T.me}</b> חסר, אני משלים</span>
    <span class="tot"><i class="dot you"></i><b>${T.you}</b> מחכה לך</span>
    <span class="tot"><i class="dot na"></i><b>${T.na}</b> לא רלוונטי</span>
  </div>
  <div class="legend"><span><i class="dot ok"></i>בוצע</span><span><i class="dot me"></i>לא בוצע, אני משלים</span><span><i class="dot you"></i>נשאר לך</span><span><i class="dot na"></i>לא רלוונטי לפלאגין הזה</span></div>
  ${globalBad.length ? `<section class="youall" style="border-inline-start-color:var(--me)"><h2>באתר כולו</h2><ul>${globalBad.map(([k, g]) => `<li class="it me"><span class="lb">המספרים באתר</span><span class="wy">${esc(g.why)}</span></li>`).join('')}</ul></section>` : ''}
  ${youAll.length && !single ? `<section class="youall"><h2>מה מחכה לך (${youAll.length})</h2><ul>${youAll.join('')}</ul></section>` : ''}
  <div class="cards">${products.map(card).join('')}</div>
  <p class="foot">נבנה מ-<code>release-check.mjs</code> (קריאה בלבד). הנקודות מתעדכנות בכל הרצה: <code>node scripts/release-check/release-board.mjs${single ? ' --plugin ' + esc(single.id) : ''}</code></p>
</div>`;
fs.writeFileSync(out, html, 'utf8');
console.log(`wrote ${out}: ${products.length} plugins, ${T.ok} ok, ${T.me} missing (Claude), ${T.you} waits for Liran, ${T.na} n/a`);
