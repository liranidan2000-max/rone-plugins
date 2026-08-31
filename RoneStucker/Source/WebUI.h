#pragma once

#include <string>

// ============================================================================
// WebUI.h — Rone Stucker UI, RONE graphite design language (Xvox-inspired).
//
// Language tokens (shared across the RONE bundle):
//   ground #14161A→#17191E · card #101216 · border #23262C · text #E8EAED
//   dim #7A7F88 · neon per-plugin (Stucker: #9D6BFF purple)
//   Knobs: charcoal radial gradient + white position dot. Values in dark chips.
//   Fonts: Sora (display/values), Manrope (labels) — Google Fonts w/ fallbacks.
//
// Split into multiple string literals to avoid MSVC C2026 (string too big).
// Each part is kept under 16000 bytes.
// ============================================================================

namespace WebUI
{

inline std::string getPart1()
{
    return std::string(R"rawhtml(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Rone Stucker</title>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Sora:wght@600;700;800&family=Manrope:wght@500;600;700;800&display=swap">
<style>
*, *::before, *::after { margin: 0; padding: 0; box-sizing: border-box; }

:root {
    --ground:  #14161A;
    --panel:   #17191E;
    --card:    #101216;
    --line:    #23262C;
    --line2:   #2A2E35;
    --text:    #E8EAED;
    --dim:     #7A7F88;
    --faint:   #4E535B;
    --neon:    #9D6BFF;
}

html, body {
    width: 100%; height: 100%;
    overflow: hidden;
    font-family: 'Manrope', 'Segoe UI', -apple-system, sans-serif;
    background: linear-gradient(180deg, var(--panel) 0%, var(--ground) 100%);
    color: var(--text);
    user-select: none;
    -webkit-user-select: none;
    cursor: default;
}

/* ---- Header ---- */
.p-head {
    height: 54px;
    display: flex; align-items: center; justify-content: space-between;
    padding: 0 16px;
    border-bottom: 1px solid var(--line);
    background: var(--panel);
}
.p-logo { display: flex; align-items: center; gap: 9px; }
.p-glyph {
    width: 22px; height: 22px; border-radius: 50%;
    border: 2px solid var(--neon);
    position: relative;
    box-shadow: 0 0 10px rgba(157,107,255,0.5);
}
.p-glyph::after {
    content: ''; position: absolute; left: 50%; top: 2px;
    width: 2px; height: 7px; margin-left: -1px;
    background: var(--neon); border-radius: 2px;
}
.p-name {
    font-family: 'Sora', 'Segoe UI', sans-serif;
    font-weight: 800; font-size: 16px; letter-spacing: 0.16em;
}
.p-name i {
    font-style: normal; color: var(--neon);
    text-shadow: 0 0 12px rgba(157,107,255,0.55);
}
.p-bypass {
    font-weight: 700; font-size: 11px; letter-spacing: 0.14em;
    color: #B9BDC4;
    border: 1px solid #383D45; border-radius: 7px;
    padding: 7px 14px;
    cursor: pointer;
    transition: all 0.15s;
}
.p-bypass:hover { border-color: var(--dim); color: var(--text); }
.p-bypass.active {
    color: var(--neon); border-color: var(--neon);
    box-shadow: 0 0 12px rgba(157,107,255,0.35);
    text-shadow: 0 0 8px rgba(157,107,255,0.6);
}

/* ---- Sub row ---- */
.p-sub {
    display: flex; align-items: center; justify-content: space-between;
    padding: 12px 16px 0;
}
.p-seg {
    display: flex;
    background: var(--card);
    border: 1px solid var(--line2);
    border-radius: 8px;
    overflow: hidden;
}
.p-seg span {
    font-size: 10px; font-weight: 800; letter-spacing: 0.16em;
    padding: 6px 13px;
    color: var(--dim);
    cursor: pointer;
    transition: all 0.15s;
}
.p-seg span:hover { color: var(--text); }
.p-seg span.on {
    color: #0F1114;
    background: var(--neon);
    box-shadow: 0 0 14px rgba(157,107,255,0.5);
}
.p-preset {
    font-size: 10px; font-weight: 700; letter-spacing: 0.14em;
    color: var(--dim);
}
.p-preset b { color: var(--text); font-weight: 700; }

/* ---- Knob stage ---- */
.p-stage {
    position: relative;
    width: 280px; height: 280px;
    margin: 14px auto 0;
    transition: opacity 0.25s, filter 0.25s;
}
.p-stage.bypassed { opacity: 0.35; filter: saturate(0.4); }
.p-arc { position: absolute; inset: 0; pointer-events: none; }
.p-arc .track { fill: none; stroke: var(--line); stroke-width: 6; stroke-linecap: round; }
.p-arc .lit {
    fill: none; stroke: var(--neon); stroke-width: 6; stroke-linecap: round;
    filter: drop-shadow(0 0 6px rgba(157,107,255,0.75));
}
.p-knob {
    position: absolute; left: 50%; top: 50%; transform: translate(-50%,-50%);
    width: 204px; height: 204px; border-radius: 50%;
    background: radial-gradient(circle at 38% 30%, #363B42 0%, #26292F 52%, #1A1C21 100%);
    box-shadow:
        0 18px 34px rgba(0,0,0,0.55),
        inset 0 2px 3px rgba(255,255,255,0.07),
        inset 0 -10px 22px rgba(0,0,0,0.45);
    cursor: grab;
}
.p-knob:active { cursor: grabbing; }
.p-knob::before {
    content: ''; position: absolute; inset: 14px; border-radius: 50%;
    background: radial-gradient(circle at 40% 32%, #2C3037 0%, #1D2025 60%, #16181C 100%);
    box-shadow: inset 0 1px 2px rgba(255,255,255,0.05), 0 2px 8px rgba(0,0,0,0.4);
}
.p-dot {
    position: absolute; left: 50%; top: 24px;
    width: 9px; height: 9px; margin-left: -4.5px;
    border-radius: 50%;
    background: #F2F4F6;
    box-shadow: 0 0 8px rgba(255,255,255,0.5);
    transform-origin: 50% 78px;
    transform: rotate(-135deg);
}
.p-center {
    position: absolute; inset: 0;
    display: flex; flex-direction: column;
    align-items: center; justify-content: center;
    pointer-events: none;
}
.p-val {
    font-family: 'Sora', 'Segoe UI', sans-serif;
    font-weight: 700; font-size: 30px; letter-spacing: 0.02em;
}
.p-val small { font-size: 18px; }
.p-val-label {
    font-size: 9px; font-weight: 800; letter-spacing: 0.34em;
    color: var(--dim); margin-top: 4px;
}
</style>
)rawhtml");
}

inline std::string getPart2()
{
    return std::string(R"rawhtml(<style>
/* ---- Data row: loop chip + LED meter ---- */
.p-datarow {
    display: flex; align-items: center; justify-content: center;
    gap: 10px; margin-top: 14px;
}
.p-chip {
    background: var(--card);
    border: 1px solid var(--line2);
    border-radius: 8px;
    font-size: 11px; font-weight: 700; letter-spacing: 0.08em;
    padding: 7px 13px;
    color: #B9BDC4;
    min-width: 96px; text-align: center;
}
.p-chip b { color: var(--text); font-variant-numeric: tabular-nums; }
.p-leds { display: flex; gap: 3px; align-items: center; }
.p-led {
    width: 4px; height: 4px; border-radius: 50%;
    background: var(--line2);
    transition: background 0.1s;
}
.p-led.on { background: var(--neon); box-shadow: 0 0 5px rgba(157,107,255,0.8); }

/* ---- Footer ---- */
.p-foot {
    position: absolute; bottom: 0; left: 0; right: 0; height: 44px;
    display: flex; align-items: center; justify-content: space-between;
    padding: 0 16px;
    border-top: 1px solid var(--line);
    background: #15171B;
}
.p-adv {
    font-size: 10px; font-weight: 800; letter-spacing: 0.22em;
    color: var(--dim); cursor: pointer;
    transition: color 0.15s;
}
.p-adv:hover { color: var(--text); }
.p-adv i { font-style: normal; color: var(--neon); }
.p-adv.open { color: var(--text); }
.p-brand { font-size: 10px; font-weight: 700; letter-spacing: 0.3em; color: var(--faint); }

/* ---- Advanced drawer ---- */
.p-drawer {
    position: absolute; bottom: 44px; left: 14px; right: 14px;
    background: #1B1E23;
    border: 1px solid var(--line2);
    border-radius: 10px;
    padding: 16px 18px 14px;
    box-shadow: 0 -8px 30px rgba(0,0,0,0.45);
    transform: translateY(12px); opacity: 0; pointer-events: none;
    transition: transform 0.22s ease, opacity 0.22s ease;
}
.p-drawer.open { transform: translateY(0); opacity: 1; pointer-events: auto; }
.p-drawer-grid {
    display: flex; justify-content: space-between; align-items: flex-start;
}
.p-mini { display: flex; flex-direction: column; align-items: center; gap: 7px; }
.p-miniknob {
    width: 54px; height: 54px; border-radius: 50%;
    background: radial-gradient(circle at 38% 30%, #363B42, #23262B 55%, #191B1F);
    box-shadow: 0 6px 14px rgba(0,0,0,0.5), inset 0 1px 2px rgba(255,255,255,0.07);
    position: relative;
    cursor: grab;
}
.p-miniknob:active { cursor: grabbing; }
.p-minidot {
    position: absolute; left: 50%; top: 7px;
    width: 5px; height: 5px; margin-left: -2.5px;
    border-radius: 50%; background: #F2F4F6;
    transform-origin: 50% 20px;
    transform: rotate(-135deg);
}
.p-minilabel {
    font-size: 9px; font-weight: 800; letter-spacing: 0.2em;
    color: var(--dim);
}
.p-minival {
    font-size: 10px; font-weight: 700; color: var(--neon);
    font-variant-numeric: tabular-nums;
}
.p-drawer-col { display: flex; flex-direction: column; gap: 8px; align-items: center; padding-top: 4px; }
.p-drawer .p-seg span { padding: 5px 9px; }
</style>
</head>
<body>

<div class="p-head">
    <div class="p-logo">
        <div class="p-glyph"></div>
        <div class="p-name">STU<i>CKER</i></div>
    </div>
    <div class="p-bypass" id="bypassBtn">BYPASS</div>
</div>

<div class="p-sub">
    <div class="p-seg" id="syncSeg">
        <span data-sync="1">SYNC</span><span data-sync="0">FREE</span>
    </div>
    <div class="p-preset">GRID <b id="gridLabel">1/4</b></div>
</div>

<div class="p-stage" id="stage">
    <svg class="p-arc" viewBox="0 0 280 280">
        <circle class="track" cx="140" cy="140" r="124"
            stroke-dasharray="584 195" transform="rotate(135 140 140)"/>
        <circle class="lit" id="arc" cx="140" cy="140" r="124"
            stroke-dasharray="0 779" transform="rotate(135 140 140)" style="opacity:0"/>
    </svg>
    <div class="p-knob" id="knob"><div class="p-dot" id="dot"></div></div>
    <div class="p-center">
        <div class="p-val" id="valueNum">OFF</div>
        <div class="p-val-label">STUCK</div>
    </div>
</div>

<div class="p-datarow">
    <div class="p-chip">LOOP <b id="loopMs">&mdash;</b></div>
    <div class="p-leds" id="leds"></div>
</div>

<div class="p-drawer" id="advPanel">
    <div class="p-drawer-grid">
        <div class="p-mini">
            <div class="p-miniknob" data-param="smooth" data-min="0" data-max="1"><div class="p-minidot"></div></div>
            <div class="p-minilabel">SMOOTH</div>
            <div class="p-minival" id="val-smooth">35%</div>
        </div>
        <div class="p-mini">
            <div class="p-miniknob" data-param="range" data-min="3" data-max="10"><div class="p-minidot"></div></div>
            <div class="p-minilabel">RANGE</div>
            <div class="p-minival" id="val-range">8.0</div>
        </div>
        <div class="p-mini">
            <div class="p-miniknob" data-param="lowcut" data-min="20" data-max="500"><div class="p-minidot"></div></div>
            <div class="p-minilabel">LOW CUT</div>
            <div class="p-minival" id="val-lowcut">20 Hz</div>
        </div>
        <div class="p-drawer-col">
            <div class="p-minilabel">LENGTH</div>
            <div class="p-seg" id="lenGroup">
                <span data-index="0">1/1</span><span data-index="1">1/2</span><span data-index="2" class="on">1/4</span><span data-index="3">1/8</span>
            </div>
        </div>
    </div>
</div>

<div class="p-foot">
    <div class="p-adv" id="advToggle"><i>&#8801;</i> ADVANCED</div>
    <div class="p-brand">RONE PLUGINS</div>
</div>
)rawhtml");
}

inline std::string getPart3()
{
    return std::string(R"rawhtml(<script>
// ─────────────────────────────────────────────────────────────────
// Bridge helpers
// ─────────────────────────────────────────────────────────────────
function emit(name, payload) {
    if (window.__JUCE__ && window.__JUCE__.backend)
        window.__JUCE__.backend.emitEvent(name, payload);
}

const state = { amount: 0, smooth: 0.35, range: 8, lowcut: 20, length: 2, bypass: false, sync: true };
let engineDepth = 0, engineEngaged = false, engineLoopMs = 0;

const ARC_LEN = 584; // 270deg of r=124 circle

const knob = document.getElementById('knob');
const dot = document.getElementById('dot');
const arc = document.getElementById('arc');
const valueNum = document.getElementById('valueNum');
const stage = document.getElementById('stage');
const loopMsEl = document.getElementById('loopMs');
const gridLabel = document.getElementById('gridLabel');
const bypassBtn = document.getElementById('bypassBtn');
const LEN_NAMES = ['1/1', '1/2', '1/4', '1/8'];

// LED meter
const ledsHolder = document.getElementById('leds');
for (let i = 0; i < 12; i++) {
    const d = document.createElement('div');
    d.className = 'p-led';
    ledsHolder.appendChild(d);
}
const leds = ledsHolder.children;

// ─────────────────────────────────────────────────────────────────
// Renders
// ─────────────────────────────────────────────────────────────────
function renderKnob() {
    const v = state.amount;
    dot.style.transform = 'rotate(' + (-135 + v * 270) + 'deg)';
    arc.setAttribute('stroke-dasharray', (v * ARC_LEN) + ' 779');
    arc.style.opacity = v <= 0.001 ? '0' : '1';
    valueNum.innerHTML = v <= 0.001 ? 'OFF'
        : Math.round(v * 100) + '<small>%</small>';
}

function renderEngine() {
    const lit = engineEngaged ? Math.round(engineDepth * 12) : 0;
    for (let i = 0; i < 12; i++)
        leds[i].classList.toggle('on', i < lit);
    if (engineEngaged && engineLoopMs > 0) {
        loopMsEl.textContent = (engineLoopMs < 10 ? engineLoopMs.toFixed(1) : Math.round(engineLoopMs)) + ' ms';
    } else {
        loopMsEl.innerHTML = '&mdash;';
    }
}

function fmtAdv(param, v) {
    if (param === 'range')  return v.toFixed(1);
    if (param === 'lowcut') return Math.round(v) + ' Hz';
    return Math.round(v * 100) + '%';
}

function renderMini(el) {
    const param = el.dataset.param;
    const min = parseFloat(el.dataset.min), max = parseFloat(el.dataset.max);
    const norm = (state[param] - min) / (max - min);
    el.querySelector('.p-minidot').style.transform = 'rotate(' + (-135 + norm * 270) + 'deg)';
    const label = document.getElementById('val-' + param);
    if (label) label.textContent = fmtAdv(param, state[param]);
}

function renderLength() {
    document.querySelectorAll('#lenGroup span').forEach(b =>
        b.classList.toggle('on', parseInt(b.dataset.index, 10) === state.length));
    gridLabel.textContent = LEN_NAMES[state.length] || '1/4';
}

function renderBypass() {
    bypassBtn.classList.toggle('active', state.bypass);
    stage.classList.toggle('bypassed', state.bypass);
}

function renderSync() {
    document.querySelectorAll('#syncSeg span').forEach(o =>
        o.classList.toggle('on', (o.dataset.sync === '1') === state.sync));
}

// ─────────────────────────────────────────────────────────────────
// Main knob drag  (vertical; shift = fine; dblclick = release)
// ─────────────────────────────────────────────────────────────────
let dragging = false, startY = 0, startVal = 0;

knob.addEventListener('mousedown', e => {
    dragging = true; startY = e.clientY; startVal = state.amount;
    emit('paramGesture', { name: 'amount', begin: true });
    e.preventDefault();
});
document.addEventListener('mousemove', e => {
    if (!dragging) return;
    const scale = e.shiftKey ? 900 : 240;
    state.amount = Math.max(0, Math.min(1, startVal + (startY - e.clientY) / scale));
    emit('setParameter', { name: 'amount', value: state.amount });
    renderKnob();
});
document.addEventListener('mouseup', () => {
    if (!dragging) return;
    dragging = false;
    emit('paramGesture', { name: 'amount', begin: false });
});
knob.addEventListener('dblclick', () => {
    state.amount = 0;
    emit('setParameter', { name: 'amount', value: 0 });
    renderKnob();
});

// ─────────────────────────────────────────────────────────────────
// Mini knobs drag
// ─────────────────────────────────────────────────────────────────
document.querySelectorAll('.p-miniknob').forEach(el => {
    const param = el.dataset.param;
    const min = parseFloat(el.dataset.min), max = parseFloat(el.dataset.max);
    let mDrag = false, mY = 0, mVal = 0;

    el.addEventListener('mousedown', e => {
        mDrag = true; mY = e.clientY; mVal = state[param];
        emit('paramGesture', { name: param, begin: true });
        e.preventDefault(); e.stopPropagation();
    });
    document.addEventListener('mousemove', e => {
        if (!mDrag) return;
        const norm = (mVal - min) / (max - min) + (mY - e.clientY) / (e.shiftKey ? 700 : 180);
        state[param] = min + Math.max(0, Math.min(1, norm)) * (max - min);
        emit('setParameter', { name: param, value: state[param] });
        renderMini(el);
    });
    document.addEventListener('mouseup', () => {
        if (!mDrag) return;
        mDrag = false;
        emit('paramGesture', { name: param, begin: false });
    });
});

// ─────────────────────────────────────────────────────────────────
// Toggles
// ─────────────────────────────────────────────────────────────────
bypassBtn.addEventListener('click', () => {
    state.bypass = !state.bypass;
    emit('setParameter', { name: 'bypass', value: state.bypass ? 1 : 0 });
    renderBypass();
});

document.querySelectorAll('#syncSeg span').forEach(o => {
    o.addEventListener('click', () => {
        state.sync = o.dataset.sync === '1';
        emit('setParameter', { name: 'sync', value: state.sync ? 1 : 0 });
        renderSync();
    });
});

document.querySelectorAll('#lenGroup span').forEach(btn => {
    btn.addEventListener('click', () => {
        state.length = parseInt(btn.dataset.index, 10);
        emit('setParameter', { name: 'length', value: state.length });
        renderLength();
    });
});

const advToggle = document.getElementById('advToggle');
const advPanel = document.getElementById('advPanel');
advToggle.addEventListener('click', () => {
    advPanel.classList.toggle('open');
    advToggle.classList.toggle('open');
});

// ─────────────────────────────────────────────────────────────────
// C++ -> JS
// ─────────────────────────────────────────────────────────────────
function whenBridgeReady(fn) {
    if (window.__JUCE__ && window.__JUCE__.backend) { fn(); return; }
    const iv = setInterval(() => {
        if (window.__JUCE__ && window.__JUCE__.backend) { clearInterval(iv); fn(); }
    }, 50);
}

whenBridgeReady(() => {
    window.__JUCE__.backend.addEventListener('paramState', p => {
        if (dragging) {
            ['smooth','range','lowcut','length','bypass','sync'].forEach(k => { if (p[k] !== undefined) state[k] = p[k]; });
        } else {
            Object.keys(state).forEach(k => { if (p[k] !== undefined) state[k] = p[k]; });
            renderKnob();
        }
        document.querySelectorAll('.p-miniknob').forEach(renderMini);
        renderLength(); renderBypass(); renderSync();
    });

    window.__JUCE__.backend.addEventListener('engineState', p => {
        engineDepth = p.depth || 0;
        engineEngaged = !!p.engaged;
        engineLoopMs = p.loopMs || 0;
        renderEngine();
    });

    emit('requestState', {});
});

renderKnob();
renderEngine();
document.querySelectorAll('.p-miniknob').forEach(renderMini);
renderLength();
renderBypass();
renderSync();
</script>
</body>
</html>
)rawhtml");
}

inline std::string getIndexHTML()
{
    return getPart1() + getPart2() + getPart3();
}

} // namespace WebUI
