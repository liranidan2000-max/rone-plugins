// End-to-end check of RONE Rise 1.2.0 dev in its Standalone: the real page talking to the real processor.
// Launches the exe with WebView2's debugger on, drives the page over CDP, prints what crossed the bridge.
// The input is muted for the run (the saved settings had it live) and the settings file is put back afterwards.
import { spawn } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';

const EXE = process.argv[2], OUT = process.argv[3], PORT = 9222;
const settings = path.join(process.env.APPDATA, 'RONE Rise', 'RONE Rise.settings');
const original = fs.existsSync(settings) ? fs.readFileSync(settings, 'utf8') : null;
if (original !== null) fs.writeFileSync(settings, original.replace('name="shouldMuteInput" val="0"', 'name="shouldMuteInput" val="1"'));

const sleep = ms => new Promise(r => setTimeout(r, ms));
const results = []; let fails = 0;
const ok = (c, msg) => { results.push((c ? 'PASS ' : 'FAIL ') + msg); if (!c) fails++; };

const child = spawn(EXE, [], { env: { ...process.env, WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS: `--remote-debugging-port=${PORT}`, WEBVIEW2_USER_DATA_FOLDER: path.join(OUT, 'wv2-rise') }, stdio: 'ignore' });
let ws = null, id = 0; const pending = new Map();
const send = (method, params = {}) => new Promise((res, rej) => { const i = ++id; pending.set(i, { res, rej }); ws.send(JSON.stringify({ id: i, method, params })); });
const js = async expr => { const r = await send('Runtime.evaluate', { expression: expr, awaitPromise: true, returnByValue: true }); if (r.exceptionDetails) throw new Error(r.exceptionDetails.exception?.description || r.exceptionDetails.text); return r.result.value; };

try {
  let target = null;
  for (let i = 0; i < 80 && !target; i++) { await sleep(250); try { const list = await (await fetch(`http://127.0.0.1:${PORT}/json`)).json(); target = list.find(t => t.type === 'page' && /juce/i.test(t.url)); } catch (e) { /* not up yet */ } }
  if (!target) throw new Error('the WebView never came up on port ' + PORT);
  ws = new WebSocket(target.webSocketDebuggerUrl);
  await new Promise((res, rej) => { ws.onopen = res; ws.onerror = () => rej(new Error('websocket')); });
  ws.onmessage = e => { const m = JSON.parse(e.data); const p = pending.get(m.id); if (p) { pending.delete(m.id); m.error ? p.rej(new Error(m.error.message)) : p.res(m.result); } };
  await sleep(2500);

  // ---- the page is the 1.2 page, and the processor's parameters reached it ----
  const boot = JSON.parse(await js(`JSON.stringify({ url: location.href, sun: typeof poseFor, aud: !!document.querySelector('#audBtn'), tailRow: !!document.querySelector('#tailBar'), land: [...document.querySelectorAll('#landSeg span')].map(s => s.dataset.i).join(','),
    ver: (document.querySelector('.bp-verline') || {}).textContent || '', locked: (() => { const o = document.getElementById('rlOverlay'); return !!o && !o.classList.contains('rl-hidden') && o.style.display !== 'none'; })(),
    state: { drive: state.drive, dir: state.dir, gap: state.gap, land: state.land, preset: state.preset, intensity: state.intensity }, bpm: frame.bpm, playing: frame.playing })`));
  console.log('boot:', JSON.stringify(boot));
  ok(boot.sun === 'function' && boot.aud && !boot.tailRow && boot.land === '0,5,6,1,2,3,4', 'the 1.2 page is in the plugin (sun, AUDITION, no TAIL row, LAND 1/2)');
  ok(/1\.2\.0/.test(boot.ver), 'the About line says 1.2.0: "' + boot.ver.trim().slice(0, 40) + '"');
  ok(typeof boot.state.drive === 'number' && boot.state.dir === 0 && boot.state.gap === 0, 'paramState carries drive / dir / gap (drive ' + boot.state.drive + ')');

  // start clean: LIFTOFF card, knob at zero, LAND 1 bar, GAP 1 beat
  await js(`loadPreset(0, cardMacros[0], ''); gestured('intensity', 0); document.querySelector('#landSeg span[data-i="5"]').click(); document.querySelector('#gapSeg span[data-i="2"]').click(); 0`);
  await sleep(700);
  const st = JSON.parse(await js(`emit('requestState', {}); new Promise(r => setTimeout(() => r(JSON.stringify({ land: state.land, gap: state.gap, bars: frame.bars })), 300))`));
  ok(st.land === 5 && st.gap === 2 && st.bars === 1, 'LAND "1" = index 5 = one bar, GAP 1 beat - read back from the processor (' + JSON.stringify(st) + ')');

  // ---- AUDITION: on, two full cycles (1 bar up, 1 bar rest), the GAP holds, the drop is seen, off ----
  const aud = JSON.parse(await js(`(async () => { document.querySelector('#audBtn').click(); await new Promise(r => setTimeout(r, 500));
    const o = { on: frame.audition, bars: frame.bars, runs: 0, lands: 0, hold: false, drop: false, maxT: 0, maxProg: 0 }; let was = frame.running; const t0 = performance.now();
    while (performance.now() - t0 < 9000) { if (frame.running && !was) o.runs++; if (!frame.running && was) o.lands++; was = frame.running; if (frame.hold) o.hold = true; if (CH.cool > 0) o.drop = true;
      o.maxT = Math.max(o.maxT, frame.t); o.maxProg = Math.max(o.maxProg, frame.prog); await new Promise(r => setTimeout(r, 12)); }
    o.led = document.querySelector('#audBtn').className; document.querySelector('#audBtn').click(); await new Promise(r => setTimeout(r, 500)); o.off = frame.audition === false && !frame.running; return JSON.stringify(o); })()`));
  console.log('audition:', JSON.stringify(aud));
  ok(aud.on && aud.bars === 1, 'AUDITION switches on in the processor (the frame says audition, 1 bar)');
  ok(aud.lands >= 2 && aud.runs >= 1, 'it loops: ' + aud.runs + ' new runs and ' + aud.lands + ' landings in 9 s at ' + boot.bpm + ' BPM');
  ok(aud.maxT > 0.85 && aud.maxProg > 0.9, 'a run climbs to the top (max intensity ' + aud.maxT.toFixed(2) + ', progress ' + aud.maxProg.toFixed(2) + ')');
  ok(aud.hold, 'the GAP shows up in the frame (hold) before the line');
  ok(aud.drop, 'the page sees the drop (shades on)');
  ok(aud.off, 'AUDITION off stops it');

  // ---- GO, RISE then FALL ----
  const go = JSON.parse(await js(`(async () => { document.querySelector('#gapSeg span[data-i="0"]').click(); document.querySelector('#goBtn').click(); await new Promise(r => setTimeout(r, 300));
    const o = { running: frame.running, txt: document.querySelector('#goBtn').textContent, maxT: 0 }; const t0 = performance.now(); while (frame.running && performance.now() - t0 < 6000) { o.maxT = Math.max(o.maxT, frame.t); await new Promise(r => setTimeout(r, 15)); }
    await new Promise(r => setTimeout(r, 200)); o.endT = frame.t; o.latched = frame.latched;
    document.querySelector('#dirSeg span[data-i="1"]').click(); await new Promise(r => setTimeout(r, 300)); document.querySelector('#goBtn').click(); await new Promise(r => setTimeout(r, 120));
    o.armed = frame.armed || frame.running; o.armTxt = document.querySelector('#goBtn').textContent; let first = -1, last = -1; const t1 = performance.now();
    while (performance.now() - t1 < 6500) { if (frame.running) { if (first < 0) first = frame.t; last = frame.t; } else if (first >= 0) break; await new Promise(r => setTimeout(r, 15)); }
    o.fallFirst = first; o.fallLast = last; await new Promise(r => setTimeout(r, 200)); o.fallEnd = frame.t; o.dropOnFall = CH.cool > 0;
    document.querySelector('#dirSeg span[data-i="0"]').click(); return JSON.stringify(o); })()`));
  console.log('go:', JSON.stringify(go));
  ok(go.running && go.maxT > 0.85 && go.endT === 0 && go.latched, 'GO runs one bar up and lands at zero, latched (max ' + go.maxT.toFixed(2) + ')');
  ok(go.armed && go.fallFirst > 0.85 && go.fallLast < 0.2 && go.fallEnd === 0, 'FALL: armed, starts at ' + go.fallFirst.toFixed(2) + ', comes down to ' + go.fallLast.toFixed(2) + ', ends at 0');
  ok(!go.dropOnFall, 'a FALL ends without the drop celebration');

  // ---- DICE and DRIVE go through the processor and come back ----
  const dice = JSON.parse(await js(`(async () => { document.querySelector('#pDice').click(); const want = Object.assign({}, diceRef), card = state.preset; await new Promise(r => setTimeout(r, 300)); emit('requestState', {}); await new Promise(r => setTimeout(r, 400));
    const got = {}; Object.keys(want).forEach(k => { got[k] = state[k]; }); const worst = Math.max(...Object.keys(want).map(k => Math.abs(want[k] - got[k])));
    return JSON.stringify({ name: state.presetName, card, backCard: state.preset, worst, drive: [want.drive, got.drive], label: document.querySelector('#pName').textContent }); })()`));
  console.log('dice:', JSON.stringify(dice));
  ok(/^DICE /.test(dice.name) && dice.card === dice.backCard && dice.worst < 0.002, 'DICE: the ten macros and the mode come back from the processor as rolled (worst difference ' + dice.worst.toFixed(4) + ')');

  // ---- a picture of each mode at the top, from inside the plugin ----
  await js(`document.querySelector('#landSeg span[data-i="0"]').click(); 0`);
  for (const [card, name] of [[0, 'liftoff'], [3, 'undertow'], [4, 'meltdown']]) {
    await js(`loadPreset(${card}, cardMacros[${card}], ''); gestured('intensity', 1); 0`); await sleep(1500);
    const shot = await send('Page.captureScreenshot', { format: 'png' }); fs.writeFileSync(path.join(OUT, `plugin-${name}.png`), Buffer.from(shot.data, 'base64'));
  }
  const errs = await js(`gestured('intensity', 0); loadPreset(0, cardMacros[0], ''); JSON.stringify({ scene: document.querySelector('#mood .v').textContent })`);
  console.log('end:', errs);
} catch (e) { fails++; results.push('FAIL the check itself broke: ' + e.message); }
finally {
  try { if (ws) ws.close(); } catch (e) { /* closing */ }
  child.kill(); await sleep(1500);
  if (original !== null) fs.writeFileSync(settings, original);
  console.log(results.join('\n')); console.log(fails ? fails + ' FAILED' : 'all good');
  process.exit(fails ? 1 : 0);
}
