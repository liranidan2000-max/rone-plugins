# -*- coding: utf-8 -*-
"""Mockup of RONE Rise 1.2.

Takes the REAL page of the dev branch (RoneRise/Source/ui/index.html) as it is, scopes its CSS into a
frame, and puts the browser port of the engine behind the page's own bridge. Nothing of the plugin's
screen is written here: what you see is the file that goes into the plugin.
  python build_mockup.py <out_local.html> [<page index.html>]
Writes two files:
  out_artifact  - page content only (the Artifact tool adds the document skeleton)
  out_local     - the same wrapped as a full document, to open from the folder
"""
import pathlib, re, sys

HERE = pathlib.Path(__file__).resolve().parent
PAGE = pathlib.Path(sys.argv[2]) if len(sys.argv) > 2 else pathlib.Path(r"D:\RONE PLUGINS\rone-plugins\RoneRise\Source\ui\index.html")
page = PAGE.read_text(encoding="utf-8")
engine = (HERE / "engine.js").read_text(encoding="utf-8")
out_artifact = HERE / "rise-round2-artifact.html"
out_local = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else HERE / "rise-round2.html"


def rep(text, old, new, count=1):
    assert text.count(old) == count, f"expected {count} x, found {text.count(old)}: {old[:70]!r}"
    return text.replace(old, new)


# ------------------------------------------------------------------ split the released page
styles = re.findall(r"<style>(.*?)</style>", page, re.S)
assert len(styles) == 3
kit_css, page_css = styles[1], styles[2]
scripts = re.findall(r"<script>(.*?)</script>", page, re.S)
assert len(scripts) == 4
kit_js, main_js, sun_js, bridge_js = scripts
body = re.search(r"<body>(.*?)<script>", page, re.S).group(1)

# ------------------------------------------------------------------ CSS: the page lives inside .plug
page_css = rep(page_css, "html, body { width:100%; height:100%; overflow:hidden;", ".plug { overflow:hidden;")
page_css = rep(page_css, "body { display:flex; flex-direction:column; }", ".plug { display:flex; flex-direction:column; }")
page_css = rep(page_css, "#app-viewport { position:relative; flex:1; overflow:hidden; }",
               "#app-viewport { position:relative; width:100%; aspect-ratio:860/580; overflow:hidden; }")

# ------------------------------------------------------------------ the mockup's own parts
shell_css = r'''
/* layout: one column - a DAW-transport strip, the plugin at its real proportions, then the notes */
:root { color-scheme:dark; --page:#0B0C0F; --strip:#111318; --ink:#E8EAED; --ink2:#A9AEB6; --rule:#23262C; --pink:#FF5FB8;
        --font-he:'Heebo','Segoe UI',Arial,sans-serif; }
body { background:var(--page); color:var(--ink); font-family:var(--font-he); }
.mk { max-width:1080px; margin:0 auto; padding-block:22px 56px; padding-left:max(16px, env(safe-area-inset-left, 0px)); padding-right:max(16px, env(safe-area-inset-right, 0px));
      display:flex; flex-direction:column; gap:16px; }
.mk-head { direction:rtl; display:flex; flex-direction:column; gap:6px; }
.mk-head h1 { font-family:'Sora',var(--font-he); font-size:22px; font-weight:800; letter-spacing:.01em; text-wrap:balance; }
.mk-head h1 i { font-style:normal; color:var(--pink); }
.mk-head p { color:var(--ink2); font-size:14px; line-height:1.65; max-width:68ch; }
.mk-bar { direction:ltr; display:flex; flex-wrap:wrap; align-items:center; gap:10px 20px; background:var(--strip); border-radius:12px; padding:12px 14px; box-shadow:inset 0 1px 0 rgba(255,255,255,.04); }
.mk-grp { display:flex; align-items:center; gap:8px; min-width:0; }
.mk-grp .k { font-family:var(--font-he); font-size:12.5px; font-weight:700; color:var(--ink2); white-space:nowrap; }
.mk-play { min-width:96px; padding:11px 16px; font-size:11px; }
.mk-num { width:64px; font-family:var(--font-mono); font-size:13px; color:var(--ink); background:var(--card); border:0; border-radius:6px; padding:8px; text-align:center; outline:none; box-shadow:inset 0 1px 3px rgba(0,0,0,.6); }
.mk-num:focus-visible, .mk-vol:focus-visible { box-shadow:inset 0 1px 3px rgba(0,0,0,.6), 0 0 0 1px rgba(255,95,184,.7); }
.mk-pos { font-family:var(--font-mono); font-size:12px; color:var(--ink2); min-width:112px; font-variant-numeric:tabular-nums; }
.mk-bar .rone-seg > :not(.pill), .mk-file { font-family:var(--font-he); font-size:12.5px; letter-spacing:0; text-transform:none; }
.mk-bar .rone-seg > :not(.pill) { padding:6px 12px; }
.mk-file { padding:8px 12px; }
.mk-vol { width:90px; accent-color:var(--pink); }
.mk-msg { font-family:var(--font-he); font-size:12.5px; color:var(--ink2); direction:rtl; }
.plug { border-radius:14px; box-shadow:0 0 0 1px #000, 0 24px 60px rgba(0,0,0,.55); }
.mk-notes { direction:rtl; display:grid; grid-template-columns:repeat(auto-fit, minmax(min(100%, 300px), 1fr)); gap:22px 36px; padding-top:6px; }
.mk-notes > div { min-width:0; }
.mk-notes h2 { font-size:15px; font-weight:800; margin-bottom:10px; text-wrap:balance; }
.mk-notes dl, .mk-notes ol { display:grid; gap:10px; }
.mk-notes dt { font-family:'Sora',var(--font-he); font-size:12px; font-weight:800; letter-spacing:.08em; color:var(--ink); }
.mk-notes dt small { font-family:var(--font-he); font-size:12px; font-weight:500; letter-spacing:0; color:var(--ink2); margin-inline-start:8px; }
.mk-notes dd, .mk-notes li, .mk-notes p { color:var(--ink2); font-size:14px; line-height:1.65; }
.mk-notes ol { padding-inline-start:20px; }
.mk-notes b { color:var(--ink); font-weight:700; }
.mk-notes .ltr { direction:ltr; unicode-bidi:isolate; font-family:'Sora',var(--font-he); font-size:12px; font-weight:700; letter-spacing:.06em; color:var(--ink); }

/* ---- the page inside the frame ---- */
#resize-handle { display:none; }
#app, #app-viewport { overflow:clip; }   /* clip, not hidden: a hidden box can still be scrolled by focus */   /* the mockup has no host window to resize */
@media (prefers-reduced-motion: reduce) { .rone-seg .pill, .p-drawer, .rone-menu { transition:none !important; animation:none !important; } }
'''

shell_top = r'''
<div class="mk">
  <header class="mk-head">
    <h1>RONE <i>Rise</i> 1.2 · הדמיה</h1>
    <p>זה העמוד האמיתי של הגרסה הבאה, אותו קובץ שנכנס לפלאגין. הסאונד הוא המנוע של הפלאגין, מועתק שורה-שורה ורץ כאן בדפדפן, אז מה שאתה רואה ושומע הוא ההתנהגות האמיתית ולא חיקוי.</p>
  </header>

  <section class="mk-bar" aria-label="Transport">
    <div class="mk-grp"><button class="rone-btn act wait mk-play" id="mkPlay">PLAY</button><span class="mk-pos" id="mkPos">BAR 1 · BEAT 1</span></div>
    <div class="mk-grp"><label class="k" for="mkBpm">BPM</label><input class="mk-num" id="mkBpm" type="number" min="60" max="200" step="1" value="145" inputmode="numeric"></div>
    <div class="mk-grp"><span class="k">מקור</span>
      <div class="rone-seg" id="mkSrc"><span data-i="0" class="on">לופ מובנה</span><span data-i="1">קובץ שלי</span></div>
      <button class="rone-btn mk-file" id="mkFileBtn" hidden>בחר קובץ</button><input type="file" id="mkFile" accept="audio/*" hidden>
    </div>
    <div class="mk-grp"><span class="k">הדרופ</span>
      <div class="rone-seg" id="mkDrop"><span data-i="0">1.1.1</span><span data-i="1" class="on">1.2</span></div>
    </div>
    <div class="mk-grp"><label class="k" for="mkVol">עוצמה</label><input class="mk-vol" id="mkVol" type="range" min="0" max="1" step="0.01" value="0.8"></div>
    <div class="mk-msg" id="mkMsg" hidden></div>
  </section>

  <div class="plug">
'''

shell_bottom = r'''
  </div><!-- /.plug -->

  <section class="mk-notes">
    <div>
      <h2>השמש: דמות לכל מצב</h2>
      <dl>
        <div><dt>LIFTOFF<small>ורודה</small></dt><dd>השמש שאתה מכיר, מלוטשת: קפיץ בתנועה, קוצים עגולים שקופצים עם הביט.</dd></div>
        <div><dt>ORBIT<small>סגולה</small></dt><dd>עיניים גדולות ונוצצות, פה של "וואו" למעלה, טבעת עם שני ירחים שמקיפים אותה.</dd></div>
        <div><dt>PULSAR<small>זהובה</small></dt><dd>מחושמלת: האור נחתך עם ה-GATE, שתי אלומות מסתובבות פעם בתיבה, ובשיא העיניים נעצמות מצחוק.</dd></div>
        <div><dt>UNDERTOW<small>כחולה</small></dt><dd>הים עולה ומכסה אותה ככל ש-DIVE נסגר. היא שמה מסכה ושנורקל, מנפחת לחיים ומוציאה בועות.</dd></div>
        <div><dt>MELTDOWN<small>אדומה</small></dt><dd>כועסת: גבות יורדות, שיניים חשוקות, סימן עצבים, אדים מהאוזניים, רועדת ונמסה. ב-100 היא שואגת.</dd></div>
        <div><dt>בכל המצבים</dt><dd>הדיליי זורק טבעות, הריוורב מרחיב את ההילה, הרעש מעיף רסס, RISE שולח חצים למעלה. העיניים עוקבות אחרי העכבר, והיא נרדמת כשעוזבים אותה. בדרופ: קפיצה ומשקפי שמש. ב-GAP: עוצרת נשימה והאור יורד.</dd></div>
      </dl>
    </div>
    <div>
      <h2>מה חדש במסך</h2>
      <dl>
        <div><dt>AUDITION<small>מתחת לכפתור הגדול</small></dt><dd>מריץ את העלייה בלופ: עולה, נוחת, נותן לדרופ לנגן תיבה או שתיים, ושוב. מחליפים פריסט תוך כדי ושומעים מיד.</dd></div>
        <div><dt>DICE<small>הקובייה ליד שם הפריסט</small></dt><dd>בילד חדש בכל לחיצה, בתוך גבולות טעם. SAVE שומר אותו כפריסט שלך.</dd></div>
        <div><dt>ADVANCED</dt><dd>TAIL ירד מכאן (הוא למעלה, ליד BYPASS). נוספו: 1 ו-2 תיבות ב-LAND ON BAR, <span class="ltr">RISE / FALL</span>, <span class="ltr">GAP</span> (שקט לפני הדרופ), וכפתור <span class="ltr">DRIVE</span>.</dd></div>
        <div><dt>הקלדת ערך</dt><dd>קליק על המספר (בכפתור הגדול או בכפתורי ADVANCED), מקלידים, Enter. דאבל-קליק עדיין מאפס.</dd></div>
      </dl>
    </div>
    <div>
      <h2>איך לבדוק</h2>
      <ol>
        <li>לחץ <span class="ltr">AUDITION</span>. הלופ מתחיל לנגן והעלייה רצה לבד.</li>
        <li>עבור בין חמשת המצבים בשורה התחתונה וראה את השמש מתחלפת. <span class="ltr">MELTDOWN</span> ו-<span class="ltr">UNDERTOW</span> הכי שונים.</li>
        <li>חכה לנחיתה: קונפטי, קפיצה ומשקפי שמש. אחר כך שים <span class="ltr">GAP</span> על 1 ב-ADVANCED וראה אותה עוצרת נשימה.</li>
        <li>עצור את הנגינה והזז את העכבר מעל השמש. עזוב אותה כמה שניות והיא נרדמת.</li>
        <li>המתג "הדרופ" למעלה משווה את הנחיתה של 1.1.1 לזו של 1.2. הקשב לקיק הראשון עם <span class="ltr">KILL LOW</span> על 100.</li>
      </ol>
    </div>
    <div>
      <h2>מה לא אמיתי כאן</h2>
      <p>אין DAW: ה-PLAY וה-BPM שלמעלה מחליפים אותו, והלופ המובנה הוא קיק, בס ולִיד שנוצרים בדפדפן ב-145 BPM. המתג "הדרופ" הוא לא חלק מהפלאגין.</p>
      <p style="margin-top:10px">המסך עצמו אמיתי לגמרי. המנוע שמאחוריו הוא העתק לדפדפן שנבדק במספרים; את הפלאגין עצמו בודקים ב-DAW.</p>
    </div>
  </section>
</div>
'''

host_js = r'''
// ============================================================================
// The "DAW" behind the page: the page talks to window.__JUCE__.backend exactly as it does in the
// plugin; here the other end is the engine port, a transport, and a loop to play through it.
// ============================================================================
(function () {
  'use strict';
  const D = window.RiseDSP, listeners = {};
  const CARDS = [
    { rise: 0,  noise: 0,   space: .73, delay: 0,    kill: .35,  dive: 0, gate: 0,  sink: .55, pitch: 0, drive: .15 },
    { rise: 0,  noise: .25, space: .82, delay: .656, kill: .35,  dive: 0, gate: 0,  sink: .55, pitch: 0, drive: .2 },
    { rise: .4, noise: .5,  space: .76, delay: .304, kill: .8,   dive: 0, gate: .7, sink: .6,  pitch: .5, drive: .4 },
    { rise: 0,  noise: 0,   space: .91, delay: .637, kill: .125, dive: 1, gate: 0,  sink: .7,  pitch: 0, drive: .3 },
    { rise: 1,  noise: 1,   space: .88, delay: .704, kill: 1,    dive: 0, gate: .5, sink: .7,  pitch: 1, drive: .8 } ];
  const H = window.RiseHost = { proc: new D.RiseProc(48000), ctx: null, node: null, gain: null, playing: false, presetName: '', src: 0, loop: null, user: null, loopBpm: 0, vol: 0.8, onChange: null };
  H.proc.fixed = true;   // the mockup opens on the 1.2 drop
  const send = (n, p) => (listeners[n] || []).forEach(cb => { try { cb(p); } catch (e) { console.error(e); } });
  window.__JUCE__ = { backend: { addEventListener(n, cb) { (listeners[n] = listeners[n] || []).push(cb); }, emitEvent(n, p) { onEvent(n, p || {}); } } };

  function pushState() { const P = H.proc.P; send('paramState', Object.assign({}, P, { bypass: P.bypass > 0.5, presetName: H.presetName })); }
  function ensureAudio() {
    if (H.ctx) return true;
    const AC = window.AudioContext || window.webkitAudioContext; if (!AC) { H.fail = true; return false; }
    try {
      H.ctx = new AC({ latencyHint: 'interactive' });
      const old = H.proc; H.proc = new D.RiseProc(H.ctx.sampleRate); Object.assign(H.proc.P, old.P); H.proc.bpm = old.bpm; H.proc.fixed = old.fixed;
      H.gain = H.ctx.createGain(); H.gain.gain.value = H.vol; H.gain.connect(H.ctx.destination);
      H.node = H.ctx.createScriptProcessor(1024, 1, 2);
      H.node.onaudioprocess = e => {
        const L = e.outputBuffer.getChannelData(0), R = e.outputBuffer.getChannelData(1), n = L.length, pr = H.proc;
        const s = H.playing ? source() : null;
        if (s) { let k = Math.round(pr.ppq * pr.spb()) % s.len; for (let i = 0; i < n; i++) { L[i] = s.L[k]; R[i] = s.R[k]; if (++k >= s.len) k = 0; } }
        else { L.fill(0); R.fill(0); }
        pr.playing = H.playing; pr.process(L, R, n);
        for (let i = 0; i < n; i++) { const a = L[i], b = R[i]; L[i] = a > 1 ? 1 : a < -1 ? -1 : a; R[i] = b > 1 ? 1 : b < -1 ? -1 : b; }
      };
      H.node.connect(H.gain);
    } catch (err) { console.error(err); H.fail = true; return false; }
    return true;
  }
  function source() {
    if (H.src === 1 && H.user) return H.user;
    if (!H.loop || H.loopBpm !== H.proc.bpm || H.loop.sr !== H.ctx.sampleRate) { H.loop = D.makeLoop(H.ctx.sampleRate, H.proc.bpm); H.loop.sr = H.ctx.sampleRate; H.loopBpm = H.proc.bpm; }
    return H.loop;
  }
  H.play = () => { if (!ensureAudio()) { if (H.onChange) H.onChange(); return; } if (H.ctx.state !== 'running') H.ctx.resume(); source(); H.proc.ppq = 0; H.playing = true; if (H.onChange) H.onChange(); };
  H.stop = () => { H.playing = false; if (H.onChange) H.onChange(); };
  H.setBpm = b => { H.proc.bpm = Math.max(60, Math.min(200, b || 145)); };
  H.setFixed = f => { H.proc.fixed = !!f; };
  H.setVol = v => { H.vol = v; if (H.gain) H.gain.gain.value = v; };
  H.loadFile = (file, done) => {
    if (!ensureAudio()) { done(false); return; }
    const fr = new FileReader();
    fr.onload = () => { H.ctx.decodeAudioData(fr.result, buf => { const L = buf.getChannelData(0), R = buf.numberOfChannels > 1 ? buf.getChannelData(1) : L; H.user = { L, R, len: buf.length }; done(true); }, () => done(false)); };
    fr.onerror = () => done(false); fr.readAsArrayBuffer(file);
  };

  function onEvent(n, p) {
    if ((n === 'go' || (n === 'audition' && p.on)) && !H.playing) H.play();
    const pr = H.proc, P = pr.P;
    switch (n) {
      case 'setParameter': if (p.name in P) { P[p.name] = typeof p.value === 'boolean' ? (p.value ? 1 : 0) : +p.value; if ((p.name === 'land' || p.name === 'dir') && pr.audition) pr.setAudition(true); } break;
      case 'loadPreset': P.preset = p.card | 0; Object.keys(p.m || {}).forEach(k => { if (k in P) P[k] = +p.m[k]; }); if (!p.m || p.m.drive === undefined) P.drive = CARDS[P.preset].drive; H.presetName = p.name || ''; pushState(); break;
      case 'savePreset': H.presetName = p.name || ''; break;
      case 'saveCardMacros': case 'deletePreset': H.presetName = ''; break;
      case 'resetCardMacros': Object.assign(P, CARDS[P.preset | 0]); H.presetName = ''; pushState(); break;
      case 'go': pr.goReq = true; break;
      case 'cancelRun': pr.cancelReq = true; break;
      case 'audition': pr.setAudition(!!p.on && H.playing); break;
      case 'requestState': pushState(); send('licenseStatus', { licensed: true }); break;
      case 'requestUiConfig': send('uiConfig', { version: '1.2.0 dev' }); break;
    }
  }
  setInterval(() => {
    const pr = H.proc, u = pr.ui, e = pr.engine.ui;
    send('frame', { t: u.t, latched: u.latched, running: u.running, bar: u.bar, beat: u.beat, bars: u.bars, tail: e.tail, bpm: pr.bpm, playing: H.playing, phase: u.phase, bpb: u.bpb,
      hpf: e.hpf, rev: e.rev, del: e.del, noise: e.noise, shep: e.shep, drive: e.drive, lpf: e.lpf, gate: e.gate, pitch: e.pitch, wet: e.wet,
      audition: pr.audition, armed: u.armed, hold: u.hold, prog: u.prog });
  }, 33);
})();
'''

extra_js = r'''
// Touch: the page listens to the mouse; a finger drives the same handlers
(function () {
  const plug = document.querySelector('.plug'); let live = null;
  const fire = (type, t, target) => target.dispatchEvent(new MouseEvent(type, { bubbles: true, cancelable: true, clientX: t.clientX, clientY: t.clientY, screenX: t.screenX, screenY: t.screenY, button: 0 }));
  plug.addEventListener('touchstart', e => { const k = e.target.closest('.arck'); if (!k || e.target.closest('.p-type')) return; live = e.changedTouches[0].identifier; fire('mousedown', e.changedTouches[0], e.target); e.preventDefault(); }, { passive: false });
  plug.addEventListener('touchmove', e => { if (live === null) return; for (const t of e.changedTouches) if (t.identifier === live) { fire('mousemove', t, document); e.preventDefault(); } }, { passive: false });
  const end = e => { if (live === null) return; for (const t of e.changedTouches) if (t.identifier === live) { fire('mouseup', t, document); live = null; } };
  plug.addEventListener('touchend', end); plug.addEventListener('touchcancel', end);
})();

// ============================================================================
// The transport strip above the plugin
// ============================================================================
(function () {
  const H = window.RiseHost, play = $('#mkPlay'), msg = $('#mkMsg');
  const say = t => { msg.hidden = !t; msg.textContent = t || ''; };
  function paint() {
    play.textContent = H.playing ? 'STOP' : 'PLAY'; play.className = 'rone-btn act mk-play' + (H.playing ? '' : ' wait');
    if (H.fail) say('הדפדפן הזה לא נתן לפתוח אודיו. נסה לפתוח את הדף ב-Chrome או ב-Edge.');
  }
  H.onChange = paint;
  play.addEventListener('click', () => { H.playing ? H.stop() : H.play(); });
  document.addEventListener('keydown', e => { if (e.code !== 'Space' || e.target.closest('input, button')) return; e.preventDefault(); H.playing ? H.stop() : H.play(); });
  $('#mkBpm').addEventListener('change', e => { H.setBpm(parseFloat(e.target.value)); e.target.value = H.proc.bpm; });
  $('#mkVol').addEventListener('input', e => H.setVol(parseFloat(e.target.value)));
  const seg = (id, fn) => { const s = $(id); s.addEventListener('click', e => { const t = e.target.closest('span'); if (!t) return; s.querySelectorAll('span').forEach(x => x.classList.toggle('on', x === t)); fn(parseInt(t.dataset.i, 10)); }); RoneUI.seg(s, { onClass: 'on' }); };
  seg('#mkDrop', i => H.setFixed(i === 1));
  const fileBtn = $('#mkFileBtn'), file = $('#mkFile');
  seg('#mkSrc', i => { H.src = i; fileBtn.hidden = i !== 1; say(i === 1 && !H.user ? 'בחר קובץ אודיו. עד אז מתנגן הלופ המובנה.' : ''); });
  fileBtn.addEventListener('click', () => file.click());
  file.addEventListener('change', () => { const f = file.files && file.files[0]; if (!f) return; say('טוען את ' + f.name + '…');
    H.loadFile(f, ok => { say(ok ? 'מנגן את ' + f.name + '. כתוב את ה-BPM שלו כדי שהתיבות יתיישרו.' : 'לא הצלחתי לקרוא את הקובץ הזה. נסה WAV או MP3.'); if (ok) fileBtn.textContent = f.name.length > 22 ? f.name.slice(0, 20) + '…' : f.name; }); });
  setInterval(() => { const pr = H.proc, b = pr.ppq; $('#mkPos').textContent = H.playing ? 'BAR ' + (Math.floor(b / 4) + 1) + ' · BEAT ' + (Math.floor(b % 4) + 1) : 'BAR 1 · BEAT 1'; }, 60);
  paint();
})();
renderKnob(); renderLand();
'''

head = ('<title>RONE Rise סבב 2</title>\n'
        '<meta name="claude:viewport-fit-sides" content="cover">\n'
        '<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Sora:wght@400;600;700;800&family=Manrope:wght@500;600;700;800'
        '&family=IBM+Plex+Mono:wght@400;500;600&family=Heebo:wght@400;500;700;800&display=swap">\n')
content = (head
    + "<style>" + kit_css + "</style>\n<style>" + page_css + "</style>\n<style>" + shell_css + "</style>\n"
    + shell_top + body + shell_bottom
    + "<script>" + engine + "</script>\n<script>" + host_js + "</script>\n"
    + "<script>" + kit_js + "</script>\n<script>" + main_js + "</script>\n<script>" + sun_js + "</script>\n<script>" + bridge_js + "</script>\n"
    + "<script>" + extra_js + "</script>\n")
assert "</script>" not in engine and "fonts/sora" not in content
out_artifact.write_text(content, encoding="utf-8")
doc = ('<!DOCTYPE html>\n<html lang="he">\n<head>\n<meta charset="UTF-8">\n<meta name="viewport" content="width=device-width, initial-scale=1.0, viewport-fit=cover">\n'
       '<style>:root{color-scheme:light}body{margin:0;font:14px system-ui,sans-serif}img{max-width:100%}[hidden]{display:none!important}</style>\n</head>\n<body>\n'
       + content + '</body>\n</html>\n')
out_local.write_text(doc, encoding="utf-8")
print("artifact:", out_artifact, len(content) // 1024, "KB")
print("local:   ", out_local)
