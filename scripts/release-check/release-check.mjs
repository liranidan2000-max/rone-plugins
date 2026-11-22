#!/usr/bin/env node
// RONE release-check: ONE command that says, per plugin, what is missing for "it is out, everywhere".
// Born from the release-machine audit (22.11.2026, docs/mockups/release-machine.html): RONE Reverb, Hook and
// Listen shipped without RONE Control and nobody noticed for days, because nothing checked. Now this does.
//
//   release-check.cmd                         every product in versions.json, every station
//   release-check.cmd --plugin RoneListen     one product (id or folder; a folder not in versions.json = a plugin
//                                             in development, read from its working copy)
//   release-check.cmd --stage build|site|media|all   only that part of the line (default all)
//   release-check.cmd --json out.json         machine copy
//   release-check.cmd --nightly               the Windows task "RONE Release Check": Telegram to Liran when a red
//                                             station appears that was not red last night (first run: full picture)
//
// Exit 1 when a selected station is red, so it can gate a release ("משחררים" ends only when this is green).
// Read-only: it never edits, builds, deploys or publishes anything. Shipped code is read at the monorepo's
// submodule POINTER (`git show <pointer>:path`), never the working copy: most working copies sit on old
// rack-dev branches 11-25 commits behind what customers have.
// Stations no tool can see (YouTube, Instagram, TikTok, factory presets) come from ledger.json next to this file.

import { spawnSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import { pathToFileURL } from 'node:url';

const HERE = import.meta.dirname;
const MONO = path.resolve(HERE, '..', '..');
const ARGS = parseArgs(process.argv.slice(2));
const SITE = path.resolve(ARGS.site || 'D:/RONE PLUGINS/WEBSITE RONE AUDIO');
const BENCH_PLUGINS = 'D:/RONE PLUGINS/_daw-lab/test-bench/config/plugins.json';
const BENCH_RUNS = 'D:/RONE PLUGINS/_daw-lab/test-bench/runs';
const TELEGRAM = 'D:/RONE PLUGINS/_daw-lab/test-bench/bench/lib/telegram.mjs';
const STATE_DIR = path.join(process.env.LOCALAPPDATA || os.tmpdir(), 'RONE', 'release-check');

// Products that are allowed to be without a station, and why (shown as "לא רלוונטי").
const NO_RACK = {
  RONEAnalyzer: 'Analyzer הוא כלי מדידה, בלי Control בכוונה',
  RoneListen: 'ל-Listen אין אף פרמטר (בלי APVTS): אין ל-Control מה להזיז',   // 22.11.2026
  // Liran 22.11.2026: RONE Control goes only into EFFECTS, whose knobs are ridden by hand for automation (1.1.1 took it out)
  RoneHook: 'HOOK תופס קול, הוא לא אפקט: את הכפתורים שלו לא מזיזים ביד לאוטומציה',
};
const FOLDER = { ReverseReverb: 'ReverseReverbVST', RoneFlanger: 'rone-flanger-', RoneAfterspace: 'RONE AFTER SPACE' };

const STATIONS = [
  // key, Hebrew label, stage, group
  ['versions', 'רשומה בסנטר (versions.json)', 'build', 'הפלאגין'],
  ['rack', 'RONE Control בתוך הפלאגין', 'build', 'הפלאגין'],
  ['maps', 'מפות Control מוכנות (2 לפחות)', 'build', 'הפלאגין'],
  ['crash', 'דיווח קריסות', 'build', 'הפלאגין'],
  ['words', 'מסך הנעילה: ניסיון 14 יום כמו באתר', 'build', 'הפלאגין'],
  ['installer', 'קובץ התקנה ו-AppId', 'build', 'הפלאגין'],
  ['hands', 'מודל הידיים בהתקנה', 'build', 'הפלאגין'],
  ['ci', 'מסנן CI כולל Control', 'build', 'הפלאגין'],
  ['center', 'אייקון ושורה בסנטר', 'build', 'הפלאגין'],
  ['bench', 'שורה בספסל הבדיקות', 'build', 'הפלאגין'],
  ['presets', 'פריסטים של המפעל', 'build', 'הפלאגין'],
  ['manual', 'מדריך PDF', 'site', 'מדריך ויחידה'],
  ['manualControl', 'פרק Control במדריך', 'site', 'מדריך ויחידה'],
  ['unit', 'יחידת 3D (4 קבצים)', 'site', 'מדריך ויחידה'],
  ['page', 'עמוד מוצר', 'site', 'האתר'],
  ['price', 'מחיר Paddle כולל _REGULAR', 'site', 'האתר'],
  ['home', 'דף הבית והתצוגה', 'site', 'האתר'],
  ['demo', 'הדגמה חיה בעמוד', 'site', 'האתר'],
  ['lists', 'רשימות פנימיות באתר', 'site', 'האתר'],
  ['tutorial', 'מדריך יוטיוב בסנטר ובעמוד', 'media', 'וידאו'],
  ['pvid', 'סרטון פריסטים', 'media', 'וידאו'],
  ['shorts', 'Shorts מהמדריך', 'media', 'וידאו'],
  ['tiktok', 'TikTok', 'media', 'וידאו'],
  ['film', 'סרטון השקה 3D (16:9 + 9:16)', 'media', 'השקה'],
  ['reel', 'ריל השקה', 'media', 'השקה'],
  ['carousel', 'קרוסלת עדכון', 'media', 'השקה'],
  ['trial', 'ריל ניסיון (סטודיו)', 'media', 'השקה'],
];
const LABEL = Object.fromEntries(STATIONS.map((s) => [s[0], s[1]]));

// ---------------------------------------------------------------------------------------------------------
function parseArgs(a) {
  const o = { plugin: [], stage: 'all' };
  for (let i = 0; i < a.length; i++) {
    const k = a[i];
    if (k === '--plugin') o.plugin.push(...String(a[++i]).split(','));
    else if (k === '--stage') o.stage = a[++i];
    else if (k === '--json') o.json = a[++i];
    else if (k === '--site') o.site = a[++i];
    else if (k === '--slug') o.slug = a[++i];
    else if (k === '--nightly') o.nightly = true;
    else if (k === '--quiet') o.quiet = true;
    else if (k === '-h' || k === '--help') { console.log(fs.readFileSync(import.meta.filename, 'utf8').split('\n').slice(0, 20).join('\n')); process.exit(0); }
  }
  return o;
}
function git(cwd, ...args) {
  const r = spawnSync('git', args, { cwd, encoding: 'utf8', maxBuffer: 64 << 20 });
  return r.status === 0 ? r.stdout : null;
}
const read = (p) => { try { return fs.readFileSync(p, 'utf8'); } catch { return null; } };
const exists = (p) => fs.existsSync(p);
const ok = (why = '', where = '') => ({ s: 'ok', why, where });
const warn = (why, where = '') => ({ s: 'warn', why, where });
const bad = (why, where = '') => ({ s: 'missing', why, where });
const na = (why = '') => ({ s: 'na', why });
const unknown = (why) => ({ s: 'unknown', why });
const readJsonSafe = (p) => { try { return JSON.parse((read(p) || '').replace(/^﻿/, '')); } catch { return null; } };
const FILM_STATUS = { prepared: 'הוכן, עוד לא נוצר', generating: 'בהיגספילד', check: 'מחכה לבדיקה בעיניים', rejected: 'נפסל', ok: 'מוכן' };

const SUBMODULES = new Set((read(path.join(MONO, '.gitmodules')) || '').split('\n')
  .map((l) => l.match(/^\s*path\s*=\s*(.+?)\s*$/)?.[1]).filter(Boolean));

// The code customers have: the monorepo pointer of a submodule, HEAD for vendored folders, the working copy for
// a plugin in development (not in versions.json yet).
function shipped(prod) {
  const folder = prod.folder;
  if (prod.dev) return { label: 'working copy', show: (rel) => read(path.join(MONO, folder, rel)), grep: (re) => grepTree(path.join(MONO, folder, 'Source'), re) };
  if (SUBMODULES.has(folder)) {
    const ptr = (git(MONO, 'ls-tree', 'HEAD', folder) || '').split(/\s+/)[2];
    const there = ptr && git(path.join(MONO, folder), 'cat-file', '-e', ptr) !== null;
    if (!there) return { label: 'pointer missing locally', missing: true, show: () => null, grep: () => null };
    return {
      label: ptr.slice(0, 8),
      show: (rel) => git(path.join(MONO, folder), 'show', `${ptr}:${rel}`),
      grep: (re) => git(path.join(MONO, folder), 'grep', '-l', '-E', re, ptr, '--', '.', ':(exclude)JUCE', ':(exclude)**/JUCE/**') !== null,
    };
  }
  return {
    label: 'HEAD',
    show: (rel) => git(MONO, 'show', `HEAD:${folder}/${rel}`),
    grep: (re) => git(MONO, 'grep', '-l', '-E', re, 'HEAD', '--', folder, `:(exclude)${folder}/JUCE`) !== null,
  };
}
function grepTree(dir, re) {
  const rx = new RegExp(re);
  const walk = (d) => (fs.existsSync(d) ? fs.readdirSync(d, { withFileTypes: true }) : []).some((e) => {
    const p = path.join(d, e.name);
    return e.isDirectory() ? walk(p) : /\.(cpp|h|txt|json)$/.test(e.name) && rx.test(read(p) || '');
  });
  return walk(dir) || rx.test(read(path.join(dir, '..', 'CMakeLists.txt')) || '');
}

// main.yml paths-filter blocks: { key: [items] } - the block whose first item is the plugin's folder.
function ciFilters() {
  const yml = read(path.join(MONO, '.github/workflows/main.yml')) || '';
  const lines = yml.split('\n');
  const start = lines.findIndex((l) => /^\s*filters:\s*\|/.test(l));
  const blocks = {};
  if (start < 0) return blocks;
  const base = lines[start].match(/^\s*/)[0].length;
  let cur = null;
  for (let i = start + 1; i < lines.length; i++) {
    const l = lines[i];
    if (!l.trim()) continue;
    const ind = l.match(/^\s*/)[0].length;
    if (ind <= base) break;
    const key = l.match(/^\s*([\w-]+):\s*$/);
    if (key) { cur = blocks[key[1]] = []; continue; }
    const item = l.match(/^\s*-\s*'([^']+)'/);
    if (item && cur) cur.push(item[1]);
  }
  return blocks;
}

async function loadCatalog() {
  const f = path.join(SITE, 'worker/_lib/catalog.js');
  if (!exists(f)) return null;
  try { return (await import(pathToFileURL(f).href)).CATALOG; } catch (e) { return null; }
}
function wranglerVars() {
  const t = read(path.join(SITE, 'wrangler.toml')) || '';
  const v = {};
  for (const m of t.matchAll(/^\s*(PADDLE_PRICE_[A-Z0-9_]+)\s*=\s*"([^"]*)"/gm)) v[m[1]] = m[2];
  return v;
}
function siteCoverage() {
  const script = path.join(SITE, 'scripts/site-counts.mjs');
  if (!exists(script)) return null;
  const r = spawnSync(process.execPath, [script, '--check', SITE, '--json'], { cwd: SITE, encoding: 'utf8', maxBuffer: 16 << 20 });
  try { return JSON.parse(r.stdout); } catch { return { ok: false, error: (r.stderr || r.stdout || '').slice(0, 300) }; }
}
function manualTexts(files) {
  // One python run reads every PDF that is asked about (pymupdf is installed for the manuals pipeline).
  const list = files.filter(exists);
  if (!list.length) return {};
  const code = 'import sys,json\nimport pymupdf\nout={}\nfor f in sys.argv[1:]:\n  try:\n    d=pymupdf.open(f); t="".join(p.get_text() for p in d); out[f]={"pages":d.page_count,"control":("RONE Control" in t)}\n  except Exception as e: out[f]={"error":str(e)}\nprint(json.dumps(out))';
  const r = spawnSync('python', ['-c', code, ...list], { encoding: 'utf8', maxBuffer: 16 << 20 });
  try { return JSON.parse(r.stdout); } catch { return {}; }
}

// ---------------------------------------------------------------------------------------------------------
async function main() {
  const versions = JSON.parse(read(path.join(MONO, 'versions.json')));
  const catalog = await loadCatalog();
  const vars = wranglerVars();
  const ledger = JSON.parse(read(path.join(HERE, 'ledger.json')) || '{"products":{}}').products;
  const filters = ciFilters();
  let bench = null;
  try { bench = JSON.parse((read(BENCH_PLUGINS) || '').replace(/^﻿/, '')).plugins; } catch {}
  let benchRun = null;   // the latest run that tested plugins (a nightly that a DAW skipped has no folders)
  try {
    for (const name of fs.readdirSync(BENCH_RUNS).filter((n) => /^\d{4}-\d\d-\d\d/.test(n)).sort().reverse()) {
      const dirs = fs.readdirSync(path.join(BENCH_RUNS, name), { withFileTypes: true }).filter((e) => e.isDirectory()).map((e) => e.name);
      if (dirs.length >= 5) { benchRun = { name, dirs }; break; }
    }
  } catch {}
  const centerCmake = read(path.join(MONO, 'RonePluginsCenter/CMakeLists.txt')) || '';
  const versionChecker = read(path.join(MONO, 'RonePluginsCenter/Source/VersionChecker.cpp')) || '';
  const units = JSON.parse(read(path.join(SITE, 'scripts/unit-render/units.json')) || '{}');
  const siteFile = (rel) => read(path.join(SITE, rel)) || '';
  const index = siteFile('index.html'), fx = siteFile('assets/fx.js'), sitemap = siteFile('sitemap.xml');
  const llms = siteFile('llms.txt'), account = siteFile('assets/account.js'), pricing = siteFile('pricing.html');
  const coverage = ARGS.stage === 'build' || ARGS.stage === 'media' ? null : siteCoverage();

  // Which products.
  let prods = versions.plugins.map((p) => ({ id: p.id, v: p, folder: FOLDER[p.id] || p.id }));
  if (ARGS.plugin.length) {
    prods = ARGS.plugin.map((want) => {
      const hit = prods.find((p) => p.id.toLowerCase() === want.toLowerCase() || p.folder.toLowerCase() === want.toLowerCase());
      if (hit) return hit;
      if (!exists(path.join(MONO, want, 'CMakeLists.txt'))) { console.error(`לא מצאתי פלאגין בשם ${want}`); process.exit(2); }
      return { id: want, v: null, folder: want, dev: true }; // in development: not in versions.json yet
    });
  }

  const manualFiles = prods.filter((p) => p.v?.manual).map((p) => path.join(MONO, 'docs/manuals', p.v.manual));
  const manuals = ARGS.stage === 'build' || ARGS.stage === 'media' ? {} : manualTexts(manualFiles);

  const results = [];
  for (const prod of prods) {
    const v = prod.v || {};
    const id = prod.id;
    const code = shipped(prod);
    const row = catalog?.find((c) => c.id === id);
    const slug = ARGS.slug || (v.store_url || '').match(/products\/([^/.]+)/)?.[1] || row?.slug || '';
    const needsRack = !NO_RACK[id];
    const st = {};

    // --- the plugin -------------------------------------------------------------------------------------
    if (prod.dev) st.versions = bad('עוד לא ב-versions.json (פלאגין בפיתוח)', 'versions.json');
    else {
      const holes = [];
      if (!v.version || /^1\.0\.0\.0$/.test(v.version)) holes.push('גרסה');
      if (!v.sha256) holes.push('sha256 של Windows');
      if ((v.formats || []).some((f) => /AU/i.test(f)) && !v.sha256_mac) holes.push('sha256 של Mac');
      for (const k of ['download_url', 'store_url', 'manual', 'whats_new', 'inno_app_id', 'accent', 'category', 'description']) if (!v[k] || (Array.isArray(v[k]) && !v[k].length)) holes.push(k);
      st.versions = holes.length ? bad(`חסר: ${holes.join(', ')}`, 'versions.json') : ok(v.version);
    }

    const cmake = code.show('CMakeLists.txt');
    const rackJson = code.show('Source/rack.json');
    if (code.missing) { st.rack = st.maps = st.crash = unknown(`הקוד של ${prod.folder} לא נמצא כאן (git submodule update --init ${prod.folder})`); }
    else if (!needsRack) { st.rack = na(NO_RACK[id]); st.maps = na(NO_RACK[id]); }
    else {
      const linked = /RoneControlRack/.test(cmake || '');
      st.rack = rackJson && linked ? ok(`בקוד ששוחרר (${code.label})`)
        : bad(!rackJson ? 'אין Source/rack.json בקוד ששוחרר' : 'CMakeLists לא מקשר את RoneControlRack', `${prod.folder}/Source/rack.json`);
      let maps = [];
      try { maps = JSON.parse(rackJson || '{}').maps || []; } catch { maps = null; }
      st.maps = maps === null ? bad('rack.json לא תקין', `${prod.folder}/Source/rack.json`)
        : maps.length >= 2 ? ok(`${maps.length} מפות: ${maps.map((m) => m.name || m.id).join(', ')}`)
        : bad(`${maps.length} מפות (צריך 2 לפחות)`, `${prod.folder}/Source/rack.json "maps"`);
    }
    if (!code.missing) st.crash = code.grep('RoneCrashReporter') ? ok() : bad('אין RoneCrashReporter בקוד ששוחרר', `${prod.folder}/CMakeLists.txt`);
    // Finish line 22.11.2026: twelve lock screens said "7-day free trial" while the site, the Center and Paddle give 14.
    // Fixed in the plugin's own UI (ui/index.html + WebUI.h, or Resources/ui) on its next release.
    if (!code.missing) st.words = code.grep('[0-9]+-day free trial') && !code.grep('14-day free trial')
      ? bad('מסך הנעילה כותב ניסיון של מספר ימים אחר מ-14 (באתר, בסנטר וב-Paddle: 14). לתקן ל-"14-day free trial"', `${prod.folder}: "No account yet? 7-day free trial"`)
      : ok();

    const iss = read(path.join(MONO, 'installer', `${id}.iss`));
    if (!iss) st.installer = bad('אין קובץ התקנה', `installer/${id}.iss`);
    else {
      const appId = iss.match(/^AppId=\{\{?([^}]+)\}/m)?.[1];
      const want = String(v.inno_app_id || '').replace(/[{}]/g, '');
      st.installer = prod.dev || !want ? (appId ? ok(appId) : bad('אין AppId', `installer/${id}.iss`))
        : appId?.toUpperCase() === want.toUpperCase() ? ok() : bad(`AppId בהתקנה (${appId}) שונה מ-versions.json (${want})`, `installer/${id}.iss`);
    }
    st.hands = !needsRack ? na(NO_RACK[id]) : !iss ? bad('אין קובץ התקנה', `installer/${id}.iss`)
      : /hands\\mp/i.test(iss) ? ok() : bad('בלי בלוק hands: המצלמה תראה HAND MODEL MISSING', `installer/${id}.iss`);

    const block = Object.entries(filters).find(([, items]) => items[0] === prod.folder || items[0] === `${prod.folder}/**`);
    st.ci = !block ? bad('אין מסנן נתיבים ב-CI: הפלאגין לא ייבנה', '.github/workflows/main.yml')
      : needsRack && !block[1].includes('Shared/RoneControlRack/**') ? bad(`המסנן ${block[0]} בלי Shared/RoneControlRack/**`, '.github/workflows/main.yml')
      : ok(block[0]);

    const icon = `${id}_icon.png`;
    const cHoles = [];
    if (!exists(path.join(MONO, 'RonePluginsCenter/Resources', icon))) cHoles.push('קובץ האייקון');
    if (!centerCmake.includes(icon) && !/GLOB[^\n]*_icon\.png/.test(centerCmake)) cHoles.push('האייקון ברשימת BinaryData');
    if (!new RegExp(`\\{\\s*"${id}"\\s*,`).test(versionChecker)) cHoles.push('שורת AppId ב-VersionChecker.cpp');
    st.center = cHoles.length ? bad(`חסר: ${cHoles.join(', ')}`, 'RonePluginsCenter') : ok();

    // The bench tests every INSTALLED plugin; plugins.json only adds the expected defaults. So: was it in the last
    // real run (a folder per slug), and does it have its row?
    const benchKey = slug.replace(/^rone-/, '');
    const isKey = (k) => k === benchKey || k.startsWith(`${benchKey}-`) || k === id;
    const benchRow = bench && Object.keys(bench).some(isKey);
    const inRun = benchRun && benchRun.dirs.some(isKey);
    st.bench = !benchRun ? unknown('לא מצאתי הרצה של הספסל')
      : !inRun ? bad(`לא נבדק בהרצה האחרונה (${benchRun.name}): לא מותקן במחשב?`, BENCH_RUNS)
      : !benchRow ? warn('נבדק בלילה, אבל בלי שורת ברירות מחדל ב-plugins.json', BENCH_PLUGINS) : ok(benchRun.name);

    // --- manual + unit --------------------------------------------------------------------------------
    const pdf = v.manual ? path.join(MONO, 'docs/manuals', v.manual) : null;
    st.manual = !pdf ? bad('אין שדה manual', 'versions.json') : exists(pdf) ? ok(manuals[pdf]?.pages ? `${manuals[pdf].pages} עמודים` : '') : bad('אין PDF', `docs/manuals/${v.manual}`);
    st.manualControl = !needsRack ? na(NO_RACK[id]) : !pdf || !exists(pdf) ? bad('אין מדריך') : manuals[pdf]?.control === undefined ? unknown('לא הצלחתי לקרוא את ה-PDF')
      : manuals[pdf].control ? ok() : bad('המדריך לא מזכיר את RONE Control', `docs/manuals/${v.manual}`);
    const uHoles = [];
    if (!units[slug]) uHoles.push('מתכון ב-units.json');
    for (const f of [`graphics/renders/${slug}.jpg`, `graphics/cutouts/${slug}.webp`, `graphics/og/${slug}.jpg`, `graphics/icons/${slug}.png`]) if (!exists(path.join(SITE, f))) uHoles.push(f);
    st.unit = !slug ? bad('אין slug') : uHoles.length ? bad(`חסר: ${uHoles.join(', ')}`, 'scripts/unit-render/render.py --unit ' + slug) : ok();

    // --- site --------------------------------------------------------------------------------------------
    const lifetime = row && row.forSale !== false && !row.free && !row.subscriptionOnly;
    const pageFile = `products/${slug}.html`;
    const page = slug ? read(path.join(SITE, pageFile)) : null;
    st.page = !page ? bad('אין עמוד מוצר', pageFile)
      : lifetime && !page.includes(`data-paddle-product="${id}"`) ? bad('כפתור הקנייה לא מחובר (data-paddle-product)', pageFile) : ok();
    if (!catalog) st.price = unknown('לא הצלחתי לקרוא את worker/_lib/catalog.js');
    else if (!row) st.price = bad('אין שורה בקטלוג', 'worker/_lib/catalog.js');
    else if (!lifetime) st.price = na(row.free ? 'חינם' : row.subscriptionOnly ? 'במנוי בלבד' : 'לא למכירה');
    else {
      const key = row.envKey || `PADDLE_PRICE_${id.toUpperCase()}`;
      const miss = [key, `${key}_REGULAR`].filter((k) => !vars[k]);
      const sameId = vars[key] && miss.length === 1 && miss[0].endsWith('_REGULAR') && row.price === row.launchPrice;
      st.price = !miss.length ? ok()
        : sameId ? bad(`חסר ${miss[0]}: אין מבצע השקה, אז הוא אותו מזהה כמו ${key} (שורה ב-wrangler.toml, בלי Paddle)`, 'wrangler.toml')
        : bad(`חסר ${miss.join(' ו-')} (צעד שלך ב-Paddle)`, 'wrangler.toml');
    }
    const homeHoles = [];
    if (slug && !index.includes(`products/${slug}`)) homeHoles.push('כרטיס בדף הבית');
    if (slug && !new RegExp(`['"]${slug}['"]`).test(fx)) homeHoles.push('התצוגה (assets/fx.js)');
    st.home = homeHoles.length ? bad(`חסר: ${homeHoles.join(', ')}`, 'index.html, assets/fx.js') : ok();
    st.demo = slug && exists(path.join(SITE, 'demo', slug)) ? ok() : bad('אין הדגמה חיה', `demo/${slug}`);
    const lHoles = [];
    if (slug && !sitemap.includes(`/products/${slug}`)) lHoles.push('sitemap.xml');
    if (v.name && !llms.includes(v.name)) lHoles.push('llms.txt');
    if (!account.includes(id)) lHoles.push('assets/account.js');
    if (lifetime && slug && !pricing.includes(slug)) lHoles.push('pricing.html');
    for (const f of coverage?.coverage?.[id] || []) lHoles.push(f);
    st.lists = lHoles.length ? bad(`חסר ב: ${[...new Set(lHoles)].join(', ')}`) : ok(coverage ? '' : 'נבדקו 4 רשימות; הבדיקה המלאה מגיעה מ-site-counts.mjs');

    // --- video + launch (ledger) --------------------------------------------------------------------------
    const led = ledger[id] || {};
    const fromLedger = (k, fallback) => {
      const e = led[k];
      if (!e) return fallback || unknown('אין רישום ב-ledger.json');
      const [s, note = ''] = e;
      return s === 'ok' ? ok(note) : s === 'na' ? na(note) : s === 'warn' ? warn(note || 'חלקי', 'ledger.json') : bad(note || 'לא קיים', 'ledger.json');
    };
    st.tutorial = !v.video ? bad('אין מדריך: בסנטר כתוב Coming soon', 'versions.json "video"') : led.tutorial ? fromLedger('tutorial') : ok(v.video);
    st.presets = fromLedger('presets');
    for (const k of ['pvid', 'shorts', 'tiktok', 'reel', 'carousel', 'trial']) st[k] = fromLedger(k);
    // The Higgsfield 3D launch film (launch-film.mjs, 22.11.2026): its job.json is the truth once it exists.
    const filmJob = slug ? readJsonSafe(path.join(SITE, 'videos/ai-launch-films', slug, 'job.json')) : null;
    if (filmJob?.films) {
      const fs_ = Object.entries(filmJob.films);
      const okN = fs_.filter(([, f]) => f.status === 'ok').length;
      st.film = okN === fs_.length ? ok(fs_.map(([k]) => k.replace('x', ':')).join(' + '))
        : okN ? warn(fs_.map(([k, f]) => `${k.replace('x', ':')} ${FILM_STATUS[f.status] || f.status}`).join(', '), 'launch-film.cmd status')
        : bad(fs_.map(([k, f]) => `${k.replace('x', ':')} ${FILM_STATUS[f.status] || f.status}`).join(', '), `launch-film.cmd status --plugin ${id}`);
    } else st.film = fromLedger('film', bad('אין סרטון השקה 3D', `launch-film.cmd prepare --plugin ${id}`));

    results.push({ id, name: v.name || id, folder: prod.folder, slug, dev: !!prod.dev, code: code.label, stations: st });
  }

  // Global: the counts on the site.
  const global = {};
  if (ARGS.stage === 'all' || ARGS.stage === 'site') {
    if (!coverage) global.counts = unknown('אין עדיין scripts/site-counts.mjs באתר');
    else if (coverage.error) global.counts = unknown(`site-counts.mjs נכשל: ${coverage.error}`);
    else {
      const wrong = (coverage.counts || []).map((c) => `${c.file}:${c.line} "${c.found}" (צריך ${c.expected})`);
      const left = (coverage.placeholdersLeft || []).map((c) => `${c.file} ${c.token}`);
      global.counts = wrong.length || left.length ? bad([...wrong, ...left].join(' · '), 'scripts/site-counts.mjs --check') : ok('כל המספרים באתר תואמים לקטלוג');
    }
  }

  const stageKeys = STATIONS.filter((s) => ARGS.stage === 'all' || s[2] === ARGS.stage).map((s) => s[0]);
  const report = { at: new Date().toISOString(), stage: ARGS.stage, stations: stageKeys, labels: LABEL, products: results, global };
  if (ARGS.json) fs.writeFileSync(ARGS.json, JSON.stringify(report, null, 2));

  const text = render(report);
  if (!ARGS.quiet) console.log(text);
  if (ARGS.nightly) await nightly(report);
  const red = results.some((p) => stageKeys.some((k) => p.stations[k]?.s === 'missing')) || Object.values(global).some((g) => g.s === 'missing');
  process.exitCode = red ? 1 : 0;
}

const MARK = { ok: '✔', warn: '⚠', missing: '✖', na: '·', unknown: '?' };
function tally(report) {
  const t = { ok: 0, warn: 0, missing: 0, na: 0, unknown: 0 };
  for (const p of report.products) for (const k of report.stations) t[p.stations[k]?.s || 'unknown']++;
  for (const g of Object.values(report.global)) t[g.s]++;
  return t;
}
function render(report) {
  const t = tally(report);
  const out = [];
  out.push(`בדיקת שלמות · ${new Date(report.at).toLocaleString('he-IL', { timeZone: 'Asia/Jerusalem' })}`);
  out.push(`${report.products.length} פלאגינים × ${report.stations.length} תחנות: ✔ ${t.ok} תקין · ⚠ ${t.warn} חלקי · ✖ ${t.missing} חסר${t.unknown ? ` · ? ${t.unknown} לא ידוע` : ''}`);
  for (const [k, g] of Object.entries(report.global)) out.push(`${MARK[g.s]} המספרים באתר: ${g.why}`);
  out.push('');
  for (const p of report.products) {
    const lines = report.stations.map((k) => [k, p.stations[k]]).filter(([, r]) => r && r.s !== 'ok' && r.s !== 'na');
    const reds = lines.filter(([, r]) => r.s === 'missing').length;
    out.push(`${reds ? '✖' : lines.length ? '⚠' : '✔'} ${p.name}${p.dev ? ' (בפיתוח)' : ''}${lines.length ? ` · ${reds} חסר, ${lines.length - reds} חלקי` : ' · הכל במקום'}`);
    for (const [k, r] of lines) out.push(`   ${MARK[r.s]} ${LABEL[k]}${r.why ? `: ${r.why}` : ''}${r.where && r.s === 'missing' ? `  [${r.where}]` : ''}`);
  }
  return out.join('\n');
}

async function nightly(report) {
  fs.mkdirSync(STATE_DIR, { recursive: true });
  const stateFile = path.join(STATE_DIR, 'last.json');
  const reds = new Map();
  for (const p of report.products) for (const k of report.stations) if (p.stations[k]?.s === 'missing') reds.set(`${p.id}:${k}`, `${p.name} · ${LABEL[k]}: ${p.stations[k].why}`);
  for (const [k, g] of Object.entries(report.global)) if (g.s === 'missing') reds.set(`global:${k}`, `המספרים באתר: ${g.why}`);
  let prev = null;
  try { prev = JSON.parse(fs.readFileSync(stateFile, 'utf8')); } catch {}
  fs.writeFileSync(stateFile, JSON.stringify({ at: report.at, reds: [...reds.keys()] }, null, 1));
  fs.writeFileSync(path.join(STATE_DIR, 'last-report.txt'), render(report));
  const fresh = prev ? [...reds].filter(([k]) => !prev.reds.includes(k)) : null;
  const fixed = prev ? prev.reds.filter((k) => !reds.has(k)).length : 0;
  let msg;
  if (!prev) msg = `${render(report).split('\n').slice(0, 2).join('\n')}\nזו הבדיקה הלילית הראשונה. מעכשיו תגיע הודעה רק כשמשהו חדש נהיה אדום.`;
  else if (fresh.length) msg = `בדיקת שלמות: ${fresh.length} חדשים באדום מאז הבדיקה הקודמת${fixed ? `, ו-${fixed} נסגרו` : ''}\n${fresh.slice(0, 25).map(([, l]) => `• ${l}`).join('\n')}\n\nהפירוט המלא: release-check.cmd`;
  else return; // nothing new is red: stay quiet
  try { const { sendText } = await import(pathToFileURL(TELEGRAM).href); await sendText(msg); }
  catch (e) { fs.appendFileSync(path.join(STATE_DIR, 'notify-errors.log'), `${new Date().toISOString()} ${e.message}\n`); }
}

main().catch((e) => { console.error(e.stack || e.message); process.exit(2); });
