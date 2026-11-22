#!/usr/bin/env node
// RONE launch-film: the 15-second Higgsfield "the unit builds itself from its parts" film of a plugin, in 16:9 AND
// 9:16, made on every release. Liran 22.11.2026: "ברגע שאני משחרר פלאגין, זה גם אוטומטית ייצור לו סרטון כזה שוכב
// וגודל סטורי גם", from his Higgsfield PLUS account (the ronemusics Gmail) - NEVER the ULTRA workspace, it is PPU's.
//
// The film is made by Seedance 2.5 through the Higgsfield MCP connector (a consumer account has no API key), so the
// generation itself is a Claude step of the rone-release skill. This script is everything around it:
//
//   launch-film.cmd prepare --plugin RoneReverb [--action]      folder + the real render + both prompts + job.json,
//                                                                and prints the exact generate_video params
//   launch-film.cmd record  --plugin X --aspect 16x9 --job <job id> --workspace <id> [--media <id>] [--credits N]
//                                                                writes the paid job down BEFORE waiting on it
//   launch-film.cmd finish  --plugin X --aspect 16x9 --url <https> | --file <mp4>
//                                                                download, ffprobe, check sheet + last frame
//   launch-film.cmd approve --plugin X --aspect 16x9|9x16|both   after looking at the check sheets
//   launch-film.cmd reject  --plugin X --aspect 16x9 --why "..."  (invented knob, wrong name, melted screen)
//   launch-film.cmd status  [--plugin X]
//   any command: --root <dir> writes somewhere else (a test); default WEBSITE RONE AUDIO\videos\ai-launch-films
//
// Rules that live here, not in memory: at most 2 paid tries per format without Liran's word (--force after his OK),
// the PPU workspace is refused, the real 3D render (graphics/renders/<slug>.jpg, made only by render.py) is image 1,
// and the prompt forbids physical knobs (RULE 12.11: the units have none, every control is on the glass screen).

import { spawnSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';

const HERE = import.meta.dirname;
const MONO = path.resolve(HERE, '..', '..');
const SITE = 'D:/RONE PLUGINS/WEBSITE RONE AUDIO';
const PPU_WORKSPACE = '29423c29-4069-499e-8f4a-5263cf043e02';   // ULTRA = PPU's account. Never spend it on RONE PLUGINS.
const PLUS_WORKSPACE = 'b4e4388c-1b10-4d31-8c36-97c0638b19aa';  // Liran's PLUS (ronemusics Gmail), seen 22.11.2026 = the only one allowed
const MAX_TRIES = 2;
const SETTINGS = { model: 'seedance_2_5', mode: 'omni_reference', duration: 15, resolution: '1080p', generate_audio: true };
const CREDITS_EACH = 240;   // estimate_video_cost 22.11.2026: seedance_2_5, 15 s, 1080p, audio = 240 (720p = 105)
const ASPECTS = { '16x9': '16:9', '9x16': '9:16' };
const FFMPEG = which('ffmpeg');
const FFPROBE = which('ffprobe');

const a = process.argv.slice(2);
const cmd = a[0];
const opt = (k) => { const i = a.indexOf(k); return i >= 0 ? a[i + 1] : null; };
const flag = (k) => a.includes(k);
const ROOT = path.resolve(opt('--root') || path.join(SITE, 'videos/ai-launch-films'));

function which(name) {
  const r = spawnSync('where', [name], { encoding: 'utf8' });
  const hit = r.status === 0 && r.stdout.split(/\r?\n/).find(Boolean);
  if (hit) return hit.trim();
  const winget = path.join(process.env.LOCALAPPDATA || '', 'Microsoft/WinGet/Packages');
  try {
    for (const d of fs.readdirSync(winget).filter((n) => n.startsWith('Gyan.FFmpeg'))) {
      for (const b of fs.readdirSync(path.join(winget, d))) {
        const f = path.join(winget, d, b, 'bin', `${name}.exe`);
        if (fs.existsSync(f)) return f;
      }
    }
  } catch {}
  return name;
}
const die = (m) => { console.error(m); process.exit(2); };
const readJson = (f, fb) => { try { return JSON.parse(fs.readFileSync(f, 'utf8').replace(/^\uFEFF/, '')); } catch { return fb; } };
const writeJson = (f, o) => fs.writeFileSync(f, JSON.stringify(o, null, 2) + '\n');
const today = () => new Date().toLocaleDateString('sv-SE', { timeZone: 'Asia/Jerusalem' });

// ---- which plugin -------------------------------------------------------------------------------------------
function unitOf(want) {
  if (!want) die('צריך --plugin <Id או slug>');
  const versions = readJson(path.join(MONO, 'versions.json'), { plugins: [] }).plugins;
  const units = readJson(path.join(SITE, 'scripts/unit-render/units.json'), {});
  const w = want.toLowerCase();
  const bare = w.replace(/^rone[\s-]*/, '');   // "RoneDMT", "dmt", "RONE DMT" -> the product whose name is RONE DMT (id RoneGrain)
  let p = versions.find((x) => x.id.toLowerCase() === w || (x.store_url || '').toLowerCase().includes(`/${w}.html`))
    || versions.find((x) => (x.name || '').toLowerCase().replace(/^rone\s*/, '') === bare);
  let slug = p ? (p.store_url || '').match(/products\/([^/.]+)/)?.[1] : null;
  if (!p && units[w]) slug = w;                              // a plugin in development: its unit exists, versions.json not yet
  if (!slug || !units[slug]) die(`${want}: אין לו יחידת 3D ב-units.json. קודם render.py (יחידה), אחר כך הסרטון.`);
  const u = units[slug];
  const label = (Array.isArray(u.name) ? u.name.join(' ') : String(u.name || '')).trim() || slug.toUpperCase();
  const render = path.join(SITE, `graphics/renders/${slug}.jpg`);
  if (!fs.existsSync(render)) die(`חסרה התמונה האמיתית ${render}. היא נוצרת רק ב-render.py.`);
  const words = readJson(path.join(HERE, 'launch-film.json'), { units: {} }).units[slug] || {};
  const color = words.color || colorWord(u.accent || '#2EE6C8');
  return { id: p?.id || slug, name: p?.name || label, slug, label, accent: u.accent, color, screen: words.screen || null, render, dir: path.join(ROOT, slug) };
}

function colorWord(hex) {   // only for a new plugin with no row in launch-film.json
  const n = parseInt(hex.replace('#', ''), 16);
  const [r, g, b] = [(n >> 16) & 255, (n >> 8) & 255, n & 255].map((v) => v / 255);
  const mx = Math.max(r, g, b), mn = Math.min(r, g, b), l = (mx + mn) / 2, d = mx - mn;
  const s = d === 0 ? 0 : d / (1 - Math.abs(2 * l - 1));
  if (s < 0.4 && l > 0.8) return 'cool white';
  let h = d === 0 ? 0 : mx === r ? ((g - b) / d) % 6 : mx === g ? (b - r) / d + 2 : (r - g) / d + 4;
  h = (h * 60 + 360) % 360;
  return h < 12 ? 'red' : h < 22 ? 'coral' : h < 38 ? 'orange' : h < 52 ? 'gold' : h < 70 ? 'yellow' : h < 160 ? 'green'
    : h < 185 ? 'teal' : h < 200 ? 'cyan' : h < 250 ? 'blue' : h < 295 ? 'purple' : h < 340 ? 'pink' : 'red';
}

// ---- the prompt -----------------------------------------------------------------------------------------------
// The approved film (DMT, 17-18.11, "הסרטון יצא מושלם") came from Liran's short chat prompt, which Claude in the chat
// broke into 5 shots before calling generate_video. This is that 5-shot form (seedance.py, 13.11: no invented knobs,
// the real screen powers on) with the plugin's own name, colour and screen; 9:16 only changes the framing.
function prompt(u, aspect, action) {
  const C = u.color;
  const screen = u.screen || `its ${C} interface lights up from left to right`;
  const tall = aspect === '9:16';
  return [
    `5 shots. Total 15 seconds - shot 1 runs 0.0-3.0s, shot 2 runs 3.0-6.0s, shot 3 runs 6.0-9.0s, shot 4 runs 9.0-12.0s, shot 5 runs 12.0-15.0s. Hard cuts between them, no transitions, no dissolves. Every cut lands on a downbeat of the music. The camera never stops moving${action ? ', and inside every shot it changes angle again: much more action, cinematic and fast' : ''}.`,
    tall
      ? `Vertical 9:16 film for a phone screen. The unit is wide, so frame it whole in the middle of the tall frame with room above and below, where the parts fly in. Prefer moves that suit a tall frame: rising, dropping, tight orbits and push-ins.`
      : `Wide 16:9 cinematic film.`,
    `Product: the hardware audio unit in image 1, ${u.label}: a dark graphite box whose whole front is one flat glass touch screen in ${C} light, with the name ${u.label} printed on its front. Keep the unit, its screen layout, colours and every label exactly as in image 1.`,
    `IMPORTANT: the unit has NO physical knobs, NO physical buttons and NO switches. Every knob, button and label is part of the picture on its flat glass screen, exactly as in image 1. Never show a metal knob or a loose knob anywhere, not on the table, not over the board, not between the parts.`,
    `Set: a glossy black table in a dark graphite studio, a soft ${C} glow on the horizon, fine dust in the light. Photoreal macro product cinematography, shallow depth of field.`,
    `Shot 1 - Low sweep: the camera skims fast and low across the glossy table as the parts fly in from both sides: chips, capacitors, brass standoffs, small screws and a strip of ${C} LEDs, tumbling and reflecting in the surface.`,
    `Shot 2 - Macro arc: the chips and capacitors drop onto a dark circuit board with tiny solder sparks, the row of ${C} LEDs lights up one after another, the camera arcs tight around the board.`,
    `Shot 3 - Orbit: the board slides into the black steel chassis with its side vents, the black bezel frame closes over it with a spray of fine metal dust, screws spin in at the four corners, the camera orbits a quarter turn.`,
    `Shot 4 - Over the top: the flat glass front screen lands on the front and powers on: ${screen}, exactly the screen of image 1, as the camera rises and looks at the front.`,
    `Shot 5 - Hero: the finished unit stands on the table exactly as in image 1, ${C} light breathing under it, while the camera arcs slowly around its front and the last dust settles.`,
    `Audio: driving upbeat electronic music, tight kick and rolling bass, no vocals, with mechanical clicks, whooshes and one deep impact when the screen powers on. No titles, captions or logos over the picture; the only words are the ones printed on the unit.`,
  ].join('\n\n');
}

// ---- commands ---------------------------------------------------------------------------------------------------
const jobFile = (u) => path.join(u.dir, 'job.json');
const loadJob = (u) => readJson(jobFile(u), null) || die(`${u.name}: עוד אין הכנה. קודם: launch-film.cmd prepare --plugin ${u.id}`);
const aspectArg = () => { const k = (opt('--aspect') || '').replace(':', 'x'); if (!ASPECTS[k] && k !== 'both') die('--aspect 16x9 | 9x16'); return k; };

function prepare() {
  const u = unitOf(opt('--plugin'));
  fs.mkdirSync(u.dir, { recursive: true });
  const ref = path.join(u.dir, `ref-${u.slug}.jpg`);
  fs.copyFileSync(u.render, ref);
  const old = readJson(jobFile(u), null);
  const job = old || { plugin: u.id, name: u.name, slug: u.slug, created: today(), settings: SETTINGS, films: {} };
  job.label = u.label; job.color = u.color; job.ref = path.basename(ref); job.action = flag('--action');
  for (const [k, ar] of Object.entries(ASPECTS)) {
    const f = path.join(u.dir, `prompt-${k}.txt`);
    fs.writeFileSync(f, prompt(u, ar, job.action) + '\n');
    job.films[k] = { aspect: ar, tries: [], status: 'prepared', ...(old?.films?.[k] || {}), prompt: path.basename(f) };   // a re-prepare keeps the paid tries
  }
  writeJson(jobFile(u), job);
  const call = (k) => ({ model: SETTINGS.model, mode: SETTINGS.mode, prompt: `<${path.join(u.dir, job.films[k].prompt)}>`, medias: [{ role: 'image_references', value: '<MEDIA_ID of ref>' }],
    duration: SETTINGS.duration, aspect_ratio: ASPECTS[k], resolution: SETTINGS.resolution, generate_audio: true });
  console.log(`${u.name}: מוכן לסרטון ההשקה (${u.color}${u.screen ? '' : ', מסך כללי'}) ב-${u.dir}`);
  console.log(`  תמונה 1 = הרינדור האמיתי: ${ref}`);
  console.log(`  עלות: ${CREDITS_EACH} קרדיטים לכל פורמט, ${CREDITS_EACH * 2} לשניהם. עד ${MAX_TRIES} ניסיונות לפורמט בלי לשאול את לירן.`);
  console.log(`  רק מחשבון ה-PLUS (ronemusics, סביבת עבודה ${PLUS_WORKSPACE}). ${PPU_WORKSPACE} (ULTRA של PPU) אסורה.`);
  console.log(JSON.stringify({ upload: ref, generate_video: { '16x9': call('16x9'), '9x16': call('9x16') } }, null, 2));
}

function record() {
  const u = unitOf(opt('--plugin'));
  const job = loadJob(u);
  const k = aspectArg();
  if (k === 'both') die('record: פורמט אחד בכל פעם');
  const jobId = opt('--job') || die('צריך --job <job id מ-generate_video>');
  const ws = opt('--workspace') || die('צריך --workspace <id> (מ-list_workspaces): כך מוודאים שזה חשבון ה-PLUS');
  if (ws === PPU_WORKSPACE) die('זו סביבת ה-ULTRA של PPU. סרטוני RONE PLUGINS נוצרים רק מחשבון ה-PLUS. לא נרשם.');
  if (ws !== PLUS_WORKSPACE && !flag('--force')) die(`סביבת עבודה לא מוכרת (${ws}). חשבון ה-PLUS הוא ${PLUS_WORKSPACE}. אחרת רק באישור של לירן (--force).`);
  const f = job.films[k];
  if (f.tries.length >= MAX_TRIES && !flag('--force')) die(`כבר ${f.tries.length} ניסיונות ל-${k}. ניסיון נוסף רק אחרי אישור של לירן (--force).`);
  f.tries.push({ job: jobId, workspace: ws, media: opt('--media') || null, credits: Number(opt('--credits') || CREDITS_EACH), at: new Date().toISOString() });
  f.status = 'generating';
  writeJson(jobFile(u), job);
  const spent = Object.values(job.films).flatMap((x) => x.tries).reduce((s, t) => s + (t.credits || 0), 0);
  console.log(`נרשם: ${u.name} ${k}, job ${jobId} (ניסיון ${f.tries.length}). סך הכל לסרטון הזה: ${spent} קרדיטים.`);
}

async function finish() {
  const u = unitOf(opt('--plugin'));
  const job = loadJob(u);
  const k = aspectArg();
  if (k === 'both') die('finish: פורמט אחד בכל פעם');
  const f = job.films[k];
  const out = path.join(u.dir, `${u.slug}-launch-${k}.mp4`);
  if (opt('--url')) {
    const r = await fetch(opt('--url'));
    if (!r.ok) die(`ההורדה נכשלה: HTTP ${r.status}`);
    fs.writeFileSync(out, Buffer.from(await r.arrayBuffer()));
    f.url = opt('--url');
  } else if (opt('--file')) fs.copyFileSync(path.resolve(opt('--file')), out);
  else die('צריך --url <https> (מ-jobs_wait) או --file <mp4>');
  const pr = spawnSync(FFPROBE, ['-v', 'error', '-show_entries', 'stream=codec_type,width,height:format=duration', '-of', 'json', out], { encoding: 'utf8' });
  let info = {};
  try { info = JSON.parse(pr.stdout || '{}'); } catch {}
  const v = (info.streams || []).find((s) => s.codec_type === 'video') || {};
  const dur = Number(info.format?.duration || 0);
  const audio = (info.streams || []).some((s) => s.codec_type === 'audio');
  // One frame from the middle of every 3 s shot + the very end, side by side: the knob / name / screen check.
  const times = [1.5, 4.5, 7.5, 10.5, 13.5, Math.max(0, dur - 0.15)];
  const tmp = fs.mkdtempSync(path.join(u.dir, '.frames-'));
  const w = k === '16x9' ? 640 : 360;
  times.forEach((t, i) => spawnSync(FFMPEG, ['-v', 'error', '-y', '-ss', String(t), '-i', out, '-frames:v', '1', '-vf', `scale=${w}:-2`, path.join(tmp, `${i}.png`)]));
  const sheet = path.join(u.dir, `check-${k}.jpg`);
  const ins = times.flatMap((_, i) => ['-i', path.join(tmp, `${i}.png`)]);
  const layout = k === '16x9' ? `0_0|w0_0|w0+w1_0|0_h0|w0_h0|w0+w1_h0` : `0_0|w0_0|w0+w1_0|w0+w1+w2_0|w0+w1+w2+w3_0|w0+w1+w2+w3+w4_0`;
  spawnSync(FFMPEG, ['-v', 'error', '-y', ...ins, '-filter_complex', `xstack=inputs=6:layout=${layout}`, '-q:v', '3', sheet]);
  const last = path.join(u.dir, `last-${k}.jpg`);
  spawnSync(FFMPEG, ['-v', 'error', '-y', '-ss', String(Math.max(0, dur - 0.15)), '-i', out, '-frames:v', '1', '-q:v', '2', last]);
  fs.rmSync(tmp, { recursive: true, force: true });
  const problems = [];
  if (Math.abs(dur - SETTINGS.duration) > 1) problems.push(`אורך ${dur.toFixed(1)} שניות`);
  if (!audio) problems.push('אין סאונד');
  if (k === '16x9' && !(v.width > v.height)) problems.push(`לא שוכב (${v.width}x${v.height})`);
  if (k === '9x16' && !(v.height > v.width)) problems.push(`לא עומד (${v.width}x${v.height})`);
  if ((Math.min(v.width, v.height) || 0) < 1000) problems.push(`רזולוציה ${v.width}x${v.height}, לא 1080p`);
  Object.assign(f, { file: path.basename(out), status: 'check', width: v.width, height: v.height, seconds: Math.round(dur * 10) / 10, audio, problems,
    sheet: path.basename(sheet), at: new Date().toISOString() });
  writeJson(jobFile(u), job);
  console.log(`${u.name} ${k}: ${out} (${v.width}x${v.height}, ${dur.toFixed(1)} שניות${audio ? ', עם סאונד' : ''})`);
  console.log(`  לבדוק בעיניים: ${sheet} ו-${last}: אין כפתור פיזי מומצא, השם ${u.label} נכון, המסך לא נמס.`);
  if (problems.length) console.log(`  שים לב: ${problems.join(', ')}`);
}

function approve(ok) {
  const u = unitOf(opt('--plugin'));
  const job = loadJob(u);
  const k = aspectArg();
  for (const x of k === 'both' ? Object.keys(ASPECTS) : [k]) {
    const f = job.films[x];
    if (ok && !f.file) die(`${x}: אין עדיין קובץ (finish קודם)`);
    if (ok && f.problems?.length && !flag('--force')) die(`${x}: ${f.problems.join(', ')}. לא מאשר סרטון כזה (רק --force, באישור של לירן).`);
    f.status = ok ? 'ok' : 'rejected';
    if (!ok) f.why = opt('--why') || 'נפסל בבדיקה';
  }
  writeJson(jobFile(u), job);
  const done = Object.values(job.films).every((f) => f.status === 'ok');
  console.log(`${u.name}: ${Object.entries(job.films).map(([x, f]) => `${x} ${f.status}`).join(', ')}`);
  if (!ok) { console.log(`  ניסיון חוזר: generate_video שוב עם אותו פרומפט, ואז record. נשארו ${MAX_TRIES - job.films[k].tries.length} בלי לשאול.`); return; }
  if (!done) return;
  // Both formats approved: the release-check station turns green and the board card gets the film as its picture.
  if (!opt('--root')) ledgerSet(u.id, 'film', ['ok', `16:9 + 9:16, ${today()}`]);
  const card = `rel-${u.id}-film`;
  const r = spawnSync(process.execPath, [path.join(SITE, 'scripts/schedule.mjs'), 'set', card, '--thumb', path.join(u.dir, job.films['9x16'].file),
    '--note', `מוכן: ${path.join(u.dir, job.films['9x16'].file)} (9:16) ו-${job.films['16x9'].file} (16:9). עולה רק באישור של לירן.`], { encoding: 'utf8' });
  console.log(r.status === 0 ? `  הכרטיס ${card} בלוח קיבל את הסרטון.` : `  אין כרטיס ${card} בלוח (release-media.cmd --plugin ${u.id} --only film פותח אותו).`);
}

// ledger.json keeps one product per line; change only that line (the rest of the file stays byte for byte).
function ledgerSet(id, key, value) {
  const file = path.join(HERE, 'ledger.json');
  const text = fs.readFileSync(file, 'utf8');
  const row = (o) => `{ ${Object.entries(o).map(([k, v]) => `"${k}": ${JSON.stringify(v).replace(/","/g, '", "')}`).join(', ')} }`;
  const re = new RegExp(`^([ \\t]*"${id}":[ \\t]*)(\\{.*\\})(,?)[ \\t\\r]*$`, 'm');
  const m = text.match(re);
  let next;
  if (m) {
    const o = JSON.parse(m[2]);
    o[key] = value;
    next = text.replace(re, `${m[1]}${row(o)}${m[3]}`);
  } else {
    next = text.replace(/("products":\s*\{\s*\n)/, `$1    "${id}": ${row({ [key]: value })},\n`);
  }
  JSON.parse(next);   // never write a broken file
  fs.writeFileSync(file, next);
}

function status() {
  const want = opt('--plugin');
  const slugs = want ? [unitOf(want).slug] : (fs.existsSync(ROOT) ? fs.readdirSync(ROOT) : []);
  if (!slugs.length) { console.log('עוד אין סרטוני השקה.'); return; }
  for (const s of slugs) {
    const job = readJson(path.join(ROOT, s, 'job.json'), null);
    if (!job) continue;
    const spent = Object.values(job.films).flatMap((x) => x.tries).reduce((t, x) => t + (x.credits || 0), 0);
    console.log(`${job.name}: ${Object.entries(job.films).map(([x, f]) => `${x} ${f.status} (${f.tries.length} ניסיונות)`).join(' · ')} · ${spent} קרדיטים`);
  }
}

const RUN = { prepare, record, finish, approve: () => approve(true), reject: () => approve(false), status };
if (!RUN[cmd]) die('launch-film.cmd prepare|record|finish|approve|reject|status --plugin <Id>   (ראה את ראש הקובץ)');
await RUN[cmd]();
