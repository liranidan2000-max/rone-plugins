# -*- coding: utf-8 -*-
"""RONE Rise 1.2.0: the round-2 controls go into the real page (Source/ui/index.html).
One-off, exact replacements; fails loudly if the page is not the 1.1.1 one."""
import io, re, sys

path = sys.argv[1]
s = io.open(path, encoding="utf-8").read()


def rep(old, new, count=1):
    global s
    assert s.count(old) == count, f"expected {count} x, found {s.count(old)}: {old[:80]!r}"
    s = s.replace(old, new)


# ------------------------------------------------------------------ CSS
rep(".p-gopill{position:absolute;left:62px;width:340px;top:392px;display:none;align-items:center;justify-content:center;gap:14px}\n.p-gopill.show{display:flex}\n",
    ".p-gopill{position:absolute;left:62px;width:340px;top:392px;display:flex;align-items:center;justify-content:center;gap:14px}\n"
    "#app .hid{display:none !important}\n.p-aud{padding:9px 16px}\n")
rep("/* ---- GO pill under the knob: only when LAND ON BAR is armed ---- */",
    "/* ---- Under the knob: AUDITION always (1.2.0); GO + the bar clock when LAND ON BAR is armed ---- */")
rep(".p-tailbar{flex:1;max-width:200px;height:8px;border-radius:4px;background:var(--card);box-shadow:inset 0 1px 3px rgba(0,0,0,.6),0 1px 0 rgba(255,255,255,.04);position:relative;overflow:hidden;cursor:pointer}\n"
    ".p-tailbar i{position:absolute;left:0;top:0;bottom:0;width:0;background:var(--hot);box-shadow:0 0 8px rgba(var(--hot-rgb),.8)}\n"
    ".p-tailval{min-width:72px;justify-content:center}\n"
    ".p-tailval .v{font-size:11px;letter-spacing:.1em}\n", "")
rep(".p-flash.on{opacity:.45;transition:none}\n",
    ".p-flash.on{opacity:.45;transition:none}\n"
    "/* ---- 1.2.0: DICE in the preset bar, a typed value on any knob ---- */\n"
    ".p-dice svg{width:13px;height:13px;display:block}\n"
    ".p-kval b{pointer-events:auto;cursor:text}\n"
    ".p-type{position:absolute;left:50%;top:50%;transform:translate(-50%,-50%);z-index:6;width:132px;text-align:center;font-family:var(--font-display);font-weight:800;font-size:40px;color:#fff;"
    "background:var(--card);border:0;border-radius:8px;padding:4px 6px;outline:none;pointer-events:auto;user-select:text;-webkit-user-select:text;"
    "box-shadow:inset 0 1px 3px rgba(0,0,0,.6),0 0 0 1px rgba(var(--neon-rgb),.7)}\n"
    ".p-miniarc .p-type{width:52px;font-size:12px;padding:4px 2px;border-radius:5px}\n"
    ".p-kval.typing > b,.p-kval.typing > span,.p-miniarc.typing > .cval{visibility:hidden}\n")

# ------------------------------------------------------------------ markup
rep('        <button class="rone-btn arrow" id="pNext" title="Next preset">&#9654;</button>\n',
    '        <button class="rone-btn arrow" id="pNext" title="Next preset">&#9654;</button>\n'
    '        <button class="rone-btn arrow p-dice" id="pDice" title="DICE: a new build-up, inside the limits of taste">'
    '<svg viewBox="0 0 16 16"><rect x="2" y="2" width="12" height="12" rx="3" fill="none" stroke="currentColor" stroke-width="1.5"/>'
    '<circle cx="5.5" cy="5.5" r="1.15" fill="currentColor"/><circle cx="8" cy="8" r="1.15" fill="currentColor"/><circle cx="10.5" cy="10.5" r="1.15" fill="currentColor"/></svg></button>\n')
rep('title="INTENSITY 0..100 - drag, wheel (shift = fine), double-click = 0"', 'title="INTENSITY 0..100 - drag, wheel (shift = fine), double-click = 0, click the number to type"')
rep('    <div class="p-gopill" id="gopill">\n'
    '      <button class="p-go rone-btn act wait" id="goBtn">GO</button>\n'
    '      <div class="p-clock"><span id="clockTxt">BAR &mdash; / 8</span><div class="p-beats" id="beats"><i></i><i></i><i></i><i></i></div></div>\n'
    '    </div>\n',
    '    <div class="p-gopill" id="gopill">\n'
    '      <button class="p-aud rone-btn act wait" id="audBtn" title="AUDITION: the rise runs in a loop, so you hear the preset while you browse">AUDITION</button>\n'
    '      <button class="p-go rone-btn act wait hid" id="goBtn">GO</button>\n'
    '      <div class="p-clock hid" id="pClock"><span id="clockTxt">BAR &mdash; / 8</span><div class="p-beats" id="beats"><i></i><i></i><i></i><i></i></div></div>\n'
    '    </div>\n')
# LAND's choice list was extended at its END (projects store the index): data-i is the parameter's index, the order on screen is musical
rows = '''
      <div class="p-drow">
        <div class="p-lbl rone-label">LAND ON BAR</div>
        <div class="p-seg rone-seg" id="landSeg"><span data-i="0" class="on">MANUAL</span><span data-i="5">1</span><span data-i="6">2</span><span data-i="1">4</span><span data-i="2">8</span><span data-i="3">16</span><span data-i="4">32</span></div>
        <div class="p-seg rone-seg" id="dirSeg" title="RISE climbs and lands on the drop. FALL starts at 100 on the next bar line and comes down - the way into a breakdown"><span data-i="0" class="on">RISE</span><span data-i="1">FALL</span></div>
        <div class="p-hint">GO runs it and lands it &middot; MIDI C1 = GO</div>
      </div>
      <div class="p-drow">
        <div class="p-lbl rone-label">CURVE</div>
        <div class="p-seg rone-seg" id="curveSeg" title="How the LAND run climbs: straight, soft (default), quiet-then-everything, or with a last-stretch push"><span data-i="0">LIN</span><span data-i="1" class="on">SOFT</span><span data-i="2">EXP</span><span data-i="3">S</span></div>
        <div class="p-lbl rone-label" style="width:auto;margin-left:18px">GAP</div>
        <div class="p-seg rone-seg" id="gapSeg" title="GAP: beats of silence right before the drop. The reverb and delay tails keep ringing through it"><span data-i="0" class="on">OFF</span><span data-i="1">&frac12;</span><span data-i="2">1</span><span data-i="3">2</span></div>
        <div class="p-hint">beats of silence before the drop &middot; the tail rings through</div>
      </div>
      '''
s, n = re.subn(r'(<div class="p-drawer" id="drawer">).*?(<div class="p-macros" id="macros"></div>)', lambda m: m.group(1) + rows + m.group(2), s, flags=re.S)
assert n == 1

# ------------------------------------------------------------------ script: state, macros, presets
rep("tail: 0, land: 0, curve: 1, bypass: false, presetName: '' };",
    "tail: 0, land: 0, curve: 1, bypass: false, presetName: '', drive: .15, dir: 0, gap: 0 };")
rep("gate: 0, pitch: 0, wet: 0 };", "gate: 0, pitch: 0, wet: 0, audition: false, armed: false, hold: false, prog: 0 };")
rep("// The card macros in use:",
    "// DRIVE is a macro since 1.2.0 (it was fixed per mode): the cards and the factory presets carry the mode's old amount\n"
    "const DRIVE_W = [.15, .2, .4, .3, .8]; PRESETS.forEach((p, i) => { p.m.drive = DRIVE_W[i]; });\n"
    "// The card macros in use:")
rep("let userPresets = [];", "FACTORY.forEach(p => { if (p.m.drive === undefined) p.m.drive = DRIVE_W[p.c]; });\nlet userPresets = [];")
rep("{ k: 'pitch', n: 'PITCH' }];", "{ k: 'pitch', n: 'PITCH' }, { k: 'drive', n: 'DRIVE' }];")
rep("function refMacros() { const p = findPreset(state.presetName);",
    "// DICE (1.2.0): a rolled build-up is its own reference until it is saved or another preset is loaded\n"
    "let diceRef = null, diceN = 0;\nfunction isDice() { return /^DICE /.test(state.presetName || ''); }\n"
    "function refMacros() { if (diceRef && isDice()) return diceRef; const p = findPreset(state.presetName);")
rep("  const html = p ? `", "  const html = isDice() ? `<small>DICE</small>${PRESETS[state.preset].n} ${state.presetName.slice(5)}` : p ? `")
rep("$('#pNext').addEventListener('click', () => stepPreset(1));\n",
    "$('#pNext').addEventListener('click', () => stepPreset(1));\n"
    "// One in five rolls is an underwater (DIVE) build; the reverb is always there, the dry always sinks, the loud stages come in by chance\n"
    "$('#pDice').addEventListener('click', e => {\n"
    "  e.stopPropagation(); RoneUI.closeAll(); pSave.classList.remove('open');\n"
    "  const r = Math.random, pick = (a, b) => Math.round((a + (b - a) * r()) * 100) / 100, card = Math.floor(r() * 5), under = r() < 0.2;\n"
    "  const m = { kill: under ? pick(0, .3) : pick(.3, 1), dive: under ? pick(.6, 1) : 0, space: pick(.45, 1), delay: r() < .5 ? pick(.2, .9) : 0, noise: r() < .6 ? pick(.15, .9) : 0,\n"
    "              rise: r() < .4 ? pick(.2, 1) : 0, gate: r() < .35 ? pick(.3, 1) : 0, pitch: r() < .4 ? pick(.2, 1) : 0, sink: pick(.4, .8), drive: pick(.1, .8) };\n"
    "  diceRef = m; loadPreset(card, m, 'DICE ' + (++diceN));\n"
    "});\n")
# the dice button sits above the SAVE code that declares pSave: move the declaration up
rep("const pSave = $('#pSave'), pSaveName = $('#pSaveName');\n", "")
rep("// ---- the preset bar: the name opens the list, the arrows step through it ----\n",
    "// ---- the preset bar: the name opens the list, the arrows step through it ----\n"
    "const pSave = $('#pSave'), pSaveName = $('#pSaveName');\n")

# ------------------------------------------------------------------ script: the knob, LAND, TAIL
new_knob = r'''const LAND_BARS = [0, 4, 8, 16, 32, 1, 2];   // by the LAND parameter's index (1 and 2 bars were appended in 1.2.0)
function renderKnob() {
  const v = shownT() * 100, a = A0 + 270 * (v / 100), hot = false;
  setAttr($('#lit'), 'd', v > 0.3 ? arcPath(CX, CY, R, A0, a) : ''); setCls($('#lit'), 'hot', hot);
  setStyle($('#dot'), 'transform', `rotate(${a.toFixed(2)}deg)`);
  const kv = $('#kval'); setText(kv, Math.round(v) + '%'); setCls(kv, 'hot', hot);
  // the ring counts the bars of a LAND run - and of an AUDITION loop (4 bars when LAND is MANUAL)
  const circ = 2 * Math.PI * 162, land = LAND_BARS[state.land] || 0, bars = frame.audition ? (frame.bars || 4) : land;
  if (bars > 0) {
    setAttr($('#barsTrack'), 'stroke-dasharray', `${(circ / bars - 5).toFixed(2)} 5`); setAttr($('#barsTrack'), 'transform', 'rotate(-90 170 170)');
    const done = frame.running ? clamp(frame.prog, 0, 1) : 0;
    setAttr($('#barsLit'), 'stroke-dasharray', `${(circ * done).toFixed(2)} ${circ.toFixed(2)}`); setAttr($('#barsLit'), 'transform', 'rotate(-90 170 170)');
  } else { setAttr($('#barsTrack'), 'stroke-dasharray', '0 2000'); setAttr($('#barsLit'), 'stroke-dasharray', '0 2000'); }
  setCls($('#pClock'), 'hid', bars === 0);
  setText($('#clockTxt'), frame.hold ? 'GAP' : frame.armed ? 'NEXT BAR' : frame.running ? `BAR ${frame.bar + 1} / ${bars}` : `BAR — / ${bars}`);
  $('#beats').querySelectorAll('i').forEach((b, i) => setCls(b, 'on', frame.running && i <= frame.beat));
  const go = $('#goBtn'), hid = land === 0 ? ' hid' : '';
  let gt = 'GO', gc = 'p-go rone-btn act wait';
  if (!frame.audition) {
    if (frame.armed) { gt = 'ARMED'; gc = 'p-go rone-btn act running'; }
    else if (frame.running) { gt = 'RUNNING'; gc = 'p-go rone-btn act running'; }
    else if (frame.tail > 0.001) { gt = 'TAIL'; gc = 'p-go rone-btn hot on tail'; }
  }
  setText(go, gt); if (go.className !== gc + hid) go.className = gc + hid;
  const aud = $('#audBtn'), ac = 'p-aud rone-btn act' + (frame.audition ? '' : ' wait'); if (aud.className !== ac) aud.className = ac;
  const m = $('#mood'), t = shownT();
  setText(m.querySelector('.v'), sceneName(t));
  const mc = 'p-mood rone-chip' + (t >= 0.6 ? ' up' : ''); if (m.className !== mc) m.className = mc;
  CH.target = t;
}'''
s, n = re.subn(r"function renderKnob\(\) \{.*?\n  CH\.target = t;\n\}", lambda m: new_knob, s, flags=re.S)
assert n == 1
rep("if (!s) return; if (frame.running) emit('cancelRun', {}); gestured('land',", "if (!s) return; if (frame.running && !frame.audition) emit('cancelRun', {}); gestured('land',")
rep("setCls(s, 'on', parseInt(s.dataset.i, 10) === state.curve)); }",
    "setCls(s, 'on', parseInt(s.dataset.i, 10) === state.curve));\n"
    "  ['dir', 'gap'].forEach(k => $('#' + k + 'Seg').querySelectorAll('span').forEach(s => setCls(s, 'on', parseInt(s.dataset.i, 10) === state[k]))); }")
rep("RoneUI.seg($('#landSeg'), { onClass: 'on' }); RoneUI.seg($('#curveSeg'), { onClass: 'on' });\n",
    "RoneUI.seg($('#landSeg'), { onClass: 'on' }); RoneUI.seg($('#curveSeg'), { onClass: 'on' });\n"
    "// RISE / FALL and GAP (1.2.0). A new direction cancels a run in flight; GAP can change while it runs\n"
    "['dir', 'gap'].forEach(k => { const sg = $('#' + k + 'Seg');\n"
    "  sg.addEventListener('click', e => { const t = e.target.closest('span'); if (!t) return; if (k === 'dir' && frame.running && !frame.audition) emit('cancelRun', {}); gestured(k, parseInt(t.dataset.i, 10)); renderLand(); renderKnob(); });\n"
    "  RoneUI.seg(sg, { onClass: 'on' }); });\n")
# TAIL lives in the header only (1.0.15 put it there; 1.2.0 takes the second copy out of ADVANCED)
rep("$('#tailBar').addEventListener('click', e => { const r = e.currentTarget.getBoundingClientRect(); gestured('tail', Math.round(clamp((e.clientX - r.left) / r.width, 0, 1) * 4)); renderTail(); });\n"
    "$('#tailBar').addEventListener('wheel', e => { e.preventDefault(); gestured('tail', clamp(state.tail + (e.deltaY < 0 ? 1 : -1), 0, 4)); renderTail(); }, { passive: false });\n"
    "function renderTail() { setText($('#tailVal'), state.tail === 0 ? 'CUT' : state.tail === 1 ? '1 BAR' : state.tail + ' BARS'); setStyle($('#tailFill'), 'width', (state.tail / 4 * 100) + '%');\n"
    "  $('#tailSeg').querySelectorAll('span').forEach(s => setCls(s, 'on', parseInt(s.dataset.i, 10) === state.tail)); }\n"
    "// the quick TAIL in the header (1.0.15, Liran: not only in ADVANCED)\n",
    "// TAIL lives in the header (1.0.15); its second copy in ADVANCED left in 1.2.0 (Liran: it is already out front)\n"
    "function renderTail() { $('#tailSeg').querySelectorAll('span').forEach(s => setCls(s, 'on', parseInt(s.dataset.i, 10) === state.tail)); }\n")
rep("$('#goBtn').addEventListener('click', () => { if (!frame.running) gestured('intensity', 0); emit('go', {}); });\n",
    "$('#goBtn').addEventListener('click', () => { if (!frame.running) gestured('intensity', 0); emit('go', {}); });\n"
    "$('#audBtn').addEventListener('click', () => emit('audition', { on: !frame.audition }));\n")
rep("function renderBypass() { setCls(bypassBtn, 'active', state.bypass); setCls($('#app'), 'bypassed', state.bypass); }\n",
    "function renderBypass() { setCls(bypassBtn, 'active', state.bypass); setCls($('#app'), 'bypassed', state.bypass); }\n"
    "\n"
    "// ---- Type a value (1.2.0): a plain click on the number, Enter applies, Esc leaves it. Double-click still resets ----\n"
    "let clickDown = null; document.addEventListener('mousedown', e => { clickDown = [e.clientX, e.clientY]; }, true);\n"
    "function typeIn(host, id, shown, toParam) {\n"
    "  if (host.querySelector('.p-type')) return;\n"
    "  const inp = document.createElement('input'); inp.className = 'p-type'; inp.value = shown; inp.inputMode = 'decimal'; inp.setAttribute('aria-label', id);\n"
    "  host.appendChild(inp); host.classList.add('typing'); inp.focus(); inp.select();\n"
    "  let done = false;\n"
    "  const close = apply => { if (done) return; done = true;\n"
    "    if (apply) { const x = parseFloat(String(inp.value).replace(',', '.')); if (isFinite(x)) { if (id === 'intensity' && frame.running) emit('cancelRun', {}); gestured(id, clamp(toParam(x), 0, 1)); } }\n"
    "    host.classList.remove('typing'); inp.remove(); renderKnob(); renderMacros(); };\n"
    "  inp.addEventListener('keydown', e => { e.stopPropagation(); if (e.key === 'Enter') close(true); else if (e.key === 'Escape') close(false); });\n"
    "  inp.addEventListener('blur', () => close(true));\n"
    "  ['mousedown', 'click', 'dblclick', 'wheel', 'contextmenu'].forEach(t => inp.addEventListener(t, e => e.stopPropagation()));\n"
    "}\n"
    "function plainClick(el, fn) { let timer = null;   // a click that did not drag, and is not the first half of a double-click\n"
    "  el.addEventListener('click', e => { if (e.target.closest('.p-type')) return; if (!clickDown || Math.hypot(e.clientX - clickDown[0], e.clientY - clickDown[1]) > 3) return; clearTimeout(timer); timer = setTimeout(fn, 240); });\n"
    "  el.addEventListener('dblclick', () => clearTimeout(timer)); }\n"
    "plainClick($('#kval'), () => typeIn($('.p-kval'), 'intensity', Math.round(shownT() * 100), x => x / 100));\n"
    "macrosEl.querySelectorAll('.p-miniarc').forEach(arc => { const id = arc.dataset.k, k = id === 'pitch' ? 200 : 100;\n"
    "  plainClick(arc, () => typeIn(arc, id, Math.round(state[id] * k), x => x / k)); });\n")

# ------------------------------------------------------------------ bridge
rep("m: Object.assign(FM({}), p.m || {}) }));",
    "m: Object.assign(FM({}), p.m || {}) })).map(p => { if (p.m.drive === undefined) p.m.drive = DRIVE_W[p.card] !== undefined ? DRIVE_W[p.card] : .15; return p; });")
rep("bind($('#curveSeg'), 'curve'); bind($('#tailBar'), 'tail'); bind($('#tailSeg'), 'tail');",
    "bind($('#curveSeg'), 'curve'); bind($('#dirSeg'), 'dir'); bind($('#gapSeg'), 'gap'); bind($('#tailSeg'), 'tail');")

io.open(path, "w", encoding="utf-8", newline="\n").write(s)
print("page patched:", len(s) // 1024, "KB")
