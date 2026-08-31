#pragma once

#include <string>

// =============================================================================
// WebUI — RONE AFTERSPACE interface
// RONE graphite design language — orange neon (#FF8A3D)
// Identical token set / components to ReverseReverb (cyan) and Flanger
// (magenta): arc knobs with the value inside, chips over the display card,
// neon-glow active states, Sora/Manrope type.
// Split into multiple raw-string chunks to stay under MSVC's literal limit.
// =============================================================================

namespace WebUI
{
inline const std::string& getIndexHTML()
{
    static const std::string html = std::string (R"rawhtml(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>RONE AFTERSPACE</title>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Sora:wght@600;700;800&family=Manrope:wght@500;600;700;800&display=swap">
<style>
/* ============================================
   AFTERSPACE — RONE graphite design language - violet neon
   ============================================ */
:root {
    --ground:  #14161A;
    --panel:   #17191E;
    --card:    #101216;
    --drawer:  #1B1E23;
    --line:    #23262C;
    --line2:   #2A2E35;
    --text:    #E8EAED;
    --text2:   #B9BDC4;
    --dim:     #7A7F88;
    --faint:   #4E535B;
    --neon:       #FF8A3D;
    --neon-glow:  rgba(255,138,61,0.5);
    --neon-soft:  rgba(255,138,61,0.14);
}
* { margin: 0; padding: 0; box-sizing: border-box; }
html, body {
    background: var(--ground);
    overflow: hidden;
    width: 100vw; height: 100vh;
}
body {
    font-family: 'Manrope', 'Segoe UI', sans-serif;
    color: var(--text);
    user-select: none;
    -webkit-user-select: none;
}
#app {
    width: 1150px; height: 780px;
    display: flex;
    flex-direction: column;
    background: linear-gradient(180deg, var(--panel) 0%, var(--ground) 100%);
    position: relative;
    transform-origin: top left;
}

/* ---- Resize handle (family pattern — the WebView owns the corner) ---- */
#resize-handle {
    position: fixed;
    bottom: 0; right: 0;
    width: 18px; height: 18px;
    cursor: nwse-resize;
    z-index: 90;
    background: transparent;
}
#resize-handle::before,
#resize-handle::after {
    content: "";
    position: absolute;
    background: var(--dim);
    border-radius: 1px;
    transform: rotate(-45deg);
    transform-origin: bottom right;
}
#resize-handle::before { width: 10px; height: 1.5px; bottom: 7px; right: 2px; }
#resize-handle::after  { width: 6px;  height: 1.5px; bottom: 4px; right: 2px; }
#resize-handle:hover::before, #resize-handle:hover::after { background: var(--neon); }

/* ---- Header ---- */
#logo-banner {
    display: flex;
    align-items: center;
    gap: 10px;
    height: 54px; min-height: 54px;
    padding: 0 16px;
    background: var(--panel);
    border-bottom: 1px solid var(--line);
    flex-shrink: 0;
}
#glyph {
    width: 22px; height: 22px;
    border-radius: 50%;
    border: 2px solid var(--neon);
    position: relative;
    box-shadow: 0 0 10px var(--neon-glow);
    flex-shrink: 0;
}
#glyph::after {
    content: '';
    position: absolute;
    left: 50%; top: 2px;
    width: 2px; height: 7px;
    margin-left: -1px;
    background: var(--neon);
    border-radius: 2px;
}
#logo-banner h1 {
    font-family: 'Sora', 'Segoe UI', sans-serif;
    font-size: 15px;
    font-weight: 800;
    letter-spacing: 0.14em;
    color: var(--text);
}
#logo-banner h1 i {
    font-style: normal;
    color: var(--neon);
    text-shadow: 0 0 12px var(--neon-glow);
}
#logo-banner .subtitle {
    font-size: 9px;
    font-weight: 700;
    color: var(--faint);
    letter-spacing: 0.3em;
    margin-top: 2px;
}
.preset {
    margin-left: auto;
    display: flex;
    align-items: center;
    height: 28px;
    background: var(--card);
    border: 1px solid var(--line);
    border-radius: 8px;
    position: relative;
}
.preset button {
    border-radius: 8px;
    width: 26px; height: 100%;
    border: none;
    background: transparent;
    color: var(--dim);
    font-size: 11px;
    cursor: pointer;
    transition: color 0.15s;
    font-family: inherit;
}
.preset button:hover { color: var(--neon); }
#preset-name {
    min-width: 190px;
    display: flex;
    align-items: center;
    justify-content: center;
    text-align: center;
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 0.08em;
    color: var(--text2);
    text-transform: uppercase;
    cursor: pointer;
    padding: 0 8px;
    border-left: 1px solid var(--line);
    border-right: 1px solid var(--line);
    height: 100%;
    white-space: nowrap;
    overflow: hidden;
}
#preset-name:hover { color: var(--text); }
#preset-menu {
    position: absolute;
    top: 34px;
    left: 50%;
    transform: translateX(-50%);
    z-index: 60;
    background: var(--drawer);
    border: 1px solid var(--line2);
    border-radius: 8px;
    box-shadow: 0 18px 40px rgba(0,0,0,0.6);
    padding: 5px;
    display: none;
    min-width: 240px;
    max-height: 520px;
    overflow-y: auto;
}
#preset-menu.open { display: block; }
#preset-menu .p-cat {
    font-size: 8px;
    font-weight: 800;
    letter-spacing: 0.22em;
    color: var(--neon);
    padding: 8px 10px 3px;
}
#preset-menu .p-item {
    display: flex;
    align-items: center;
    padding: 6px 10px;
    border-radius: 5px;
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 0.06em;
    color: var(--text2);
    text-transform: uppercase;
    cursor: pointer;
    gap: 8px;
}
#preset-menu .p-item:hover { background: var(--neon-soft); color: var(--neon); }
#preset-menu .p-item.current { color: var(--neon); }
#preset-menu .p-sep { height: 1px; background: var(--line2); margin: 5px 4px; }
#preset-menu .p-lock {
    display: flex;
    align-items: center;
    gap: 7px;
    padding: 7px 10px;
    font-size: 9px;
    font-weight: 800;
    letter-spacing: 0.12em;
    color: var(--dim);
    cursor: pointer;
}
#preset-menu .p-lock:hover { color: var(--text2); }
#preset-menu .p-lock input[type="checkbox"] {
    accent-color: var(--neon);
    cursor: pointer;
    width: 13px; height: 13px;
}
#about-btn {
    width: 26px; height: 26px;
    border-radius: 50%;
    border: 1px solid #383D45;
    background: transparent;
    color: var(--text2);
    font-size: 12px;
    font-weight: bold;
    cursor: pointer;
    transition: all 0.2s;
    margin-left: 10px;
    flex-shrink: 0;
}
#about-btn:hover {
    border-color: var(--neon);
    color: var(--neon);
    box-shadow: 0 0 10px var(--neon-glow);
}
#bypass {
    height: 28px;
    padding: 0 14px;
    margin-left: 10px;
    border-radius: 8px;
    border: 1px solid #383D45;
    background: transparent;
    color: var(--dim);
    font-family: 'Manrope', 'Segoe UI', sans-serif;
    font-size: 9px;
    font-weight: 800;
    letter-spacing: 0.22em;
    cursor: pointer;
    transition: all 0.18s;
}
#bypass:hover { border-color: var(--text2); color: var(--text2); }
#bypass.on {
    border-color: var(--neon);
    color: var(--neon);
    box-shadow: 0 0 12px var(--neon-glow), inset 0 0 10px var(--neon-soft);
}
</style>
)rawhtml")

    + R"rawhtml(<style>
/* ---- Space display ---- */
#viz-wrap {
    margin: 12px 16px 0;
    position: relative;
    height: 306px; min-height: 306px;
    background: var(--card);
    border: 1px solid var(--line);
    border-radius: 8px;
    overflow: hidden;
}
#viz { position: absolute; inset: 0; width: 100%; height: 100%; }
.chip {
    position: absolute;
    display: flex;
    align-items: baseline;
    gap: 6px;
    padding: 5px 10px;
    background: rgba(16,18,22,0.85);
    border: 1px solid var(--line2);
    border-radius: 8px;
    font-size: 9px;
    font-weight: 800;
    letter-spacing: 0.18em;
    color: var(--dim);
    font-variant-numeric: tabular-nums;
}
.chip b {
    font-family: 'Sora', 'Segoe UI', sans-serif;
    font-size: 13px;
    font-weight: 700;
    color: var(--neon);
    text-shadow: 0 0 10px var(--neon-glow);
}
.chip small { font-size: 8px; color: var(--dim); }
#chip-spatial { top: 10px; left: 10px; }
#chip-bpm     { top: 10px; right: 10px; }
#chip-engine  { bottom: 10px; right: 10px; }
#chip-duck    { bottom: 10px; left: 10px; }

/* ---- Knob groups ---- */
.knob-group {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 5px;
}
.knob-group label {
    font-size: 9px;
    font-weight: 800;
    text-transform: uppercase;
    letter-spacing: 0.18em;
    color: var(--dim);
}
.knob { cursor: grab; }
.knob:active { cursor: grabbing; }

/* ---- Arc knob (family component, identical to ReverseReverb/Flanger) ---- */
.arck { position: relative; flex-shrink: 0; }
.arck svg { position: absolute; inset: 0; pointer-events: none; }
.arck .track { fill: none; stroke: var(--line); stroke-linecap: round; }
.arck .lit {
    fill: none; stroke: var(--neon); stroke-linecap: round;
    filter: drop-shadow(0 0 5px var(--neon-glow));
    transition: opacity 0.1s;
}
.arck .kbody {
    position: absolute; border-radius: 50%;
    background: radial-gradient(circle at 38% 30%, #363B42 0%, #26292F 52%, #1A1C21 100%);
    box-shadow:
        0 12px 24px rgba(0,0,0,0.55),
        inset 0 2px 3px rgba(255,255,255,0.07),
        inset 0 -7px 16px rgba(0,0,0,0.45);
}
.arck .kbody::before {
    content: ''; position: absolute; inset: 9%; border-radius: 50%;
    background: radial-gradient(circle at 40% 32%, #2C3037 0%, #1D2025 60%, #16181C 100%);
    box-shadow: inset 0 1px 2px rgba(255,255,255,0.05), 0 2px 6px rgba(0,0,0,0.4);
}
.arck .kdot {
    position: absolute; left: 50%; border-radius: 50%;
    background: #F2F4F6;
    box-shadow: 0 0 7px rgba(255,255,255,0.5);
    transform: rotate(-135deg);
    z-index: 2;
    pointer-events: none;
}
.arck .cval {
    position: absolute; inset: 0;
    display: flex; align-items: center; justify-content: center;
    pointer-events: none;
    font-family: 'Sora', 'Segoe UI', sans-serif;
    font-weight: 700;
    color: var(--text);
    text-transform: uppercase;
    z-index: 3;
    font-variant-numeric: tabular-nums;
}
.arck .cval small { font-size: 0.55em; font-weight: 700; margin-left: 2px; }

.arck.size-l { width: 124px; height: 124px; }
.arck.size-l .track, .arck.size-l .lit { stroke-width: 5; }
.arck.size-l .kbody { inset: 15px; }
.arck.size-l .kdot { top: 23px; width: 7px; height: 7px; margin-left: -3.5px; transform-origin: 50% 39px; }
.arck.size-l .cval { font-size: 18px; }

.arck.size-m { width: 104px; height: 104px; }
.arck.size-m .track, .arck.size-m .lit { stroke-width: 4.5; }
.arck.size-m .kbody { inset: 13px; }
.arck.size-m .kdot { top: 21px; width: 6px; height: 6px; margin-left: -3px; transform-origin: 50% 31px; }
.arck.size-m .cval { font-size: 15px; }

.arck.size-s { width: 58px; height: 58px; }
.arck.size-s .track, .arck.size-s .lit { stroke-width: 3.5; }
.arck.size-s .kbody { inset: 8px; }
.arck.size-s .kdot { top: 11px; width: 4px; height: 4px; margin-left: -2px; transform-origin: 50% 18px; }
.arck.size-s .cval { font-size: 10px; }

/* ---- Main knob row ---- */
#main-knobs {
    display: flex;
    align-items: flex-end;
    justify-content: center;
    gap: 52px;
    padding: 20px 16px 8px;
}
#solowet-btn {
    height: 28px;
    padding: 0 12px;
    margin-left: 10px;
    border-radius: 8px;
    border: 1px solid #383D45;
    background: transparent;
    color: var(--dim);
    cursor: pointer;
    display: flex;
    align-items: center;
    gap: 6px;
    font-family: 'Manrope', 'Segoe UI', sans-serif;
    font-size: 9px;
    font-weight: 800;
    letter-spacing: 0.14em;
    transition: all 0.18s;
}
#solowet-btn:hover { border-color: var(--text2); color: var(--text2); }
#solowet-btn.on {
    border-color: var(--neon);
    color: var(--neon);
    box-shadow: 0 0 12px var(--neon-glow), inset 0 0 10px var(--neon-soft);
}

/* ---- Modules ---- */
#modules {
    display: flex;
    gap: 12px;
    margin: 10px 16px 0;
}
.module {
    flex: 1;
    background: var(--drawer);
    border: 1px solid var(--line2);
    border-radius: 10px;
    padding: 10px 16px 12px;
    position: relative;
}
.module-label {
    font-size: 10px;
    font-weight: 800;
    letter-spacing: 0.24em;
    color: var(--dim);
    margin-bottom: 4px;
}
.module-row {
    display: flex;
    align-items: center;
    gap: 18px;
}
.module-canvas {
    flex: 1;
    height: 112px;
    min-width: 0;
    background: var(--card);
    border: 1px solid var(--line);
    border-radius: 8px;
    overflow: hidden;
}
.module-canvas canvas { width: 100%; height: 100%; display: block; }
.module-side {
    display: flex;
    flex-direction: column;
    gap: 6px;
    align-items: stretch;
    min-width: 108px;
}

/* ---- Select group (family) ---- */
.select-group { display: flex; flex-direction: column; gap: 2px; }
.select-group label {
    font-size: 8px;
    font-weight: 800;
    text-transform: uppercase;
    letter-spacing: 0.16em;
    color: var(--dim);
}
.select-group select {
    background: var(--card);
    color: var(--text);
    border: 1px solid var(--line2);
    border-radius: 6px;
    padding: 5px 8px;
    font-family: 'Manrope', 'Segoe UI', sans-serif;
    font-size: 11px;
    font-weight: 600;
    outline: none;
    cursor: pointer;
    transition: border-color 0.15s;
}
.select-group select:focus, .select-group select:hover { border-color: var(--neon); }
.select-group select option { background: var(--drawer); color: var(--text); }
.side-note {
    font-size: 8px;
    font-weight: 700;
    letter-spacing: 0.1em;
    color: var(--faint);
    font-variant-numeric: tabular-nums;
}

/* ---- Segmented control (family) ---- */
.seg {
    display: flex;
    background: var(--card);
    border: 1px solid var(--line2);
    border-radius: 8px;
    overflow: hidden;
    width: max-content;
}
.seg span {
    font-size: 9px;
    font-weight: 800;
    letter-spacing: 0.14em;
    padding: 6px 12px;
    color: var(--dim);
    cursor: pointer;
    transition: all 0.15s;
}
.seg span:hover { color: var(--text); }
.seg span.on {
    color: #0F1114;
    background: var(--neon);
    box-shadow: 0 0 12px var(--neon-glow);
}
</style>
)rawhtml"

    + R"rawhtml(<style>
/* ---- Bottom row ---- */
#bottom-row {
    display: flex;
    align-items: center;
    gap: 22px;
    margin: 12px 16px;
    flex: 1;
    position: relative;
}
.action-btn {
    padding: 10px 22px;
    border-radius: 8px;
    border: 1px solid #383D45;
    background: transparent;
    color: var(--text2);
    font-family: 'Manrope', 'Segoe UI', sans-serif;
    font-size: 11px;
    font-weight: 800;
    letter-spacing: 0.2em;
    cursor: pointer;
    transition: all 0.2s;
}
.action-btn:hover { border-color: var(--dim); color: var(--text); }
.action-btn.active {
    border-color: var(--neon);
    background: var(--neon-soft);
    color: var(--neon);
    box-shadow: 0 0 12px var(--neon-glow);
    text-shadow: 0 0 8px var(--neon-glow);
}
#freeze-btn.active {
    border-color: transparent;
    background: var(--neon);
    color: #0F1114;
    text-shadow: none;
    box-shadow: 0 0 18px var(--neon-glow);
}

/* Character macro slider */
#char-wrap {
    flex: 1;
    display: flex;
    align-items: center;
    gap: 14px;
}
.char-end {
    font-size: 9px;
    font-weight: 800;
    letter-spacing: 0.2em;
    color: var(--dim);
    transition: color 0.2s, text-shadow 0.2s;
}
.char-end.on { color: var(--neon); text-shadow: 0 0 8px var(--neon-glow); }
#char-track {
    flex: 1;
    height: 26px;
    position: relative;
    cursor: ew-resize;
}
#char-track .rail {
    position: absolute; left: 0; right: 0; top: 12px; height: 3px;
    background: var(--line2); border-radius: 2px;
}
#char-track .fill {
    position: absolute; left: 0; top: 12px; height: 3px;
    background: var(--neon); border-radius: 2px;
    box-shadow: 0 0 8px var(--neon-glow);
}
#char-track .handle {
    position: absolute; top: 7px;
    width: 13px; height: 13px;
    border-radius: 50%;
    margin-left: -6.5px;
    background: #F2F4F6;
    box-shadow: 0 0 8px var(--neon-glow);
}
#char-label {
    font-size: 9px;
    font-weight: 800;
    letter-spacing: 0.24em;
    color: var(--faint);
    position: absolute;
    top: -6px; left: 50%;
    transform: translateX(-50%);
}

/* ---- Advanced drawer (floats above the bottom row, family pattern) ---- */
#adv-drawer {
    position: absolute;
    bottom: calc(100% + 10px);
    left: 0; right: 0;
    background: var(--drawer);
    border: 1px solid var(--line2);
    border-radius: 10px;
    padding: 12px 14px 14px;
    box-shadow: 0 -10px 34px rgba(0,0,0,0.55);
    z-index: 30;
    display: none;
    gap: 10px;
}
#adv-drawer.open { display: flex; }
.adv-sec {
    flex: 1;
    background: var(--card);
    border: 1px solid var(--line);
    border-radius: 8px;
    padding: 9px 6px 10px;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 7px;
}
.adv-sec .sec-title {
    font-size: 8px;
    font-weight: 800;
    text-transform: uppercase;
    letter-spacing: 0.22em;
    color: var(--neon);
}
.adv-grid {
    display: flex;
    flex-wrap: wrap;
    justify-content: center;
    gap: 6px 12px;
}
.adv-grid .knob-group label { font-size: 8px; letter-spacing: 0.14em; }
.adv-foot {
    display: flex;
    flex-direction: column;
    gap: 6px;
    align-items: center;
}
.mini-btn {
    height: 24px;
    padding: 0 12px;
    border-radius: 6px;
    border: 1px solid #383D45;
    background: transparent;
    color: var(--dim);
    font-family: 'Manrope', 'Segoe UI', sans-serif;
    font-size: 8px;
    font-weight: 800;
    letter-spacing: 0.16em;
    cursor: pointer;
    transition: all 0.18s;
}
.mini-btn:hover { color: var(--text); border-color: var(--dim); }
.mini-btn.on {
    border-color: var(--neon);
    background: var(--neon-soft);
    color: var(--neon);
    box-shadow: 0 0 10px var(--neon-glow);
}
.custom-hint {
    font-size: 7px;
    font-weight: 800;
    letter-spacing: 0.16em;
    color: var(--faint);
    transition: opacity 0.2s;
}

/* ---- Tooltip ---- */
#tooltip {
    position: fixed;
    background: var(--drawer);
    border: 1px solid var(--line2);
    border-radius: 7px;
    padding: 7px 12px;
    font-size: 11px;
    font-weight: 600;
    color: var(--text2);
    max-width: 260px;
    z-index: 100;
    pointer-events: none;
    opacity: 0;
    transition: opacity 0.15s;
    box-shadow: 0 8px 24px rgba(0,0,0,0.5);
}
#tooltip.show { opacity: 1; }

::-webkit-scrollbar { width: 9px; }
::-webkit-scrollbar-thumb { background: var(--line2); border-radius: 5px; }
::-webkit-scrollbar-track { background: transparent; }
</style>
</head>
<body>
<div id="app">

  <!-- ================= HEADER ================= -->
  <header id="logo-banner">
    <div id="glyph"></div>
    <h1>AFTER<i>SPACE</i></h1>
    <span class="subtitle">RONE PLUGINS</span>
    <div class="preset">
      <button id="p-prev" title="Previous preset">&#9664;</button>
      <div id="preset-name" data-tip="Browse presets.">INIT - CLEAN SPACE</div>
      <button id="p-next" title="Next preset">&#9654;</button>
      <div id="preset-menu"></div>
    </div>
    <button id="about-btn" title="About">i</button>
    <button id="solowet-btn" data-tip="Listen to the space alone.">
      <svg width="11" height="11" viewBox="0 0 12 12" fill="none">
        <path d="M1.5 8V6a4.5 4.5 0 019 0v2" stroke="currentColor" stroke-width="1.3" stroke-linecap="round"/>
        <rect x="0.8" y="7.2" width="2.6" height="3.6" rx="1.1" fill="currentColor"/>
        <rect x="8.6" y="7.2" width="2.6" height="3.6" rx="1.1" fill="currentColor"/>
      </svg>WET</button>
    <button id="bypass" data-tip="Bypasses AFTERSPACE completely.">BYPASS</button>
  </header>

  <!-- ================= SPACE DISPLAY ================= -->
  <div id="viz-wrap">
    <canvas id="viz"></canvas>
    <div class="chip" id="chip-spatial">SPATIAL&nbsp;<b id="spatial-val">WIDE</b></div>
    <div class="chip" id="chip-bpm">BPM&nbsp;<b id="bpm-val">120</b></div>
    <div class="chip" id="chip-engine">ENGINE&nbsp;<b id="engine-val">ACTIVE</b></div>
    <div class="chip" id="chip-duck">DUCK&nbsp;<b id="duckgr-val">0.0</b><small>dB</small></div>
  </div>

  <!-- ================= MAIN KNOBS ================= -->
  <div id="main-knobs">
    <div id="knob-size"></div>
    <div id="knob-decay"></div>
    <div id="knob-bloom"></div>
    <div id="knob-silk"></div>
    <div id="knob-mix"></div>
  </div>

  <!-- ================= MODULES ================= -->
  <div id="modules">
    <div class="module">
      <div class="module-label">DUCK</div>
      <div class="module-row">
        <div class="module-canvas"><canvas id="duck-meter"></canvas></div>
        <div id="knob-duck"></div>
        <div class="module-side">
          <div class="select-group">
            <label>Mode</label>
            <select id="sel-duckmode" data-tip="Moves the wet signal out of the way while the source is active.">
              <option>SMART</option><option>FULL</option><option>VOCAL</option>
              <option>TRANSIENT</option><option>CUSTOM</option>
            </select>
          </div>
        </div>
      </div>
    </div>
    <div class="module">
      <div class="module-label">ECHO</div>
      <div class="module-row">
        <div class="module-canvas"><canvas id="echo-dots"></canvas></div>
        <div id="knob-echo"></div>
        <div class="module-side">
          <div class="select-group">
            <label>Time</label>
            <select id="sel-echotime" data-tip="Blends tempo-synced echoes into the space."></select>
          </div>
          <span class="side-note" id="echo-ms"></span>
        </div>
      </div>
    </div>
  </div>

  <!-- ================= BOTTOM ROW ================= -->
  <div id="bottom-row">
    <button class="action-btn" id="freeze-btn" data-tip="Holds the current reverb tail.">&#10052;&nbsp;&nbsp;FREEZE</button>
    <div id="char-wrap" data-tip="Moves the engine from tight and clean to wide and atmospheric.">
      <span class="char-end" id="char-clean">CLEAN</span>
      <div id="char-track">
        <span id="char-label">CHARACTER</span>
        <div class="rail"></div><div class="fill" id="char-fill"></div><div class="handle" id="char-handle"></div>
      </div>
      <span class="char-end" id="char-dream">DREAM</span>
    </div>
    <button class="action-btn" id="adv-btn">ADVANCED</button>

    <!-- Advanced drawer floats above this row -->
    <div id="adv-drawer">
      <div class="adv-sec"><span class="sec-title">Space</span>
        <div class="adv-grid" id="adv-space"></div>
        <div class="adv-foot">
          <button class="mini-btn" id="ag-btn" data-tip="Keeps the wet level steady when Size or Decay changes.">AUTO GAIN</button>
        </div></div>
      <div class="adv-sec"><span class="sec-title">Tone</span>
        <div class="adv-grid" id="adv-tone"></div></div>
      <div class="adv-sec"><span class="sec-title">Motion &amp; Stereo</span>
        <div class="adv-grid" id="adv-mod"></div>
        <div class="adv-foot">
          <button class="mini-btn" id="ml-btn" data-tip="Keeps the wet signal mono below 150Hz for a clean low end.">MONO LOW</button>
        </div></div>
      <div class="adv-sec"><span class="sec-title">Duck</span>
        <div class="adv-grid" id="adv-duck"></div>
        <span class="custom-hint" id="duck-hint">ACTIVE IN CUSTOM MODE</span></div>
      <div class="adv-sec"><span class="sec-title">Echo</span>
        <div class="adv-grid" id="adv-echo"></div>
        <div class="adv-foot">
          <div class="seg" id="seg-routing" data-tip="Where the echo lives: before, inside or after the space.">
            <span data-i="0">BEFORE</span><span data-i="1">INSIDE</span><span data-i="2">AFTER</span>
          </div>
          <button class="mini-btn" id="pp-btn">PING PONG</button>
        </div></div>
    </div>
  </div>

  <div id="tooltip"></div>
</div>
<div id="resize-handle" title="Resize"></div>
)rawhtml"

    + R"rawhtml(<script>
"use strict";
// ============================================================================
// Bridge
// ============================================================================
function emit(n,v){ if(window.__JUCE__ && window.__JUCE__.backend) window.__JUCE__.backend.emitEvent(n,v||{}); }
function setParam(id,v){ emit("setParameter",{name:id,value:v}); }

// ============================================================================
// Parameter registry
// ============================================================================
var P = {};
var DEFAULTS = {
  "main.mix":0.25,"main.size":0.55,"main.decay":2.2,"main.bloom":0.10,
  "silk.amount":0.35,"main.autogain":1,"main.solowet":0,
  "main.character":0.25,"main.bypass":0,"freeze.active":0,
  "duck.amount":0.20,"duck.mode":0,"duck.attack":10,"duck.hold":40,
  "duck.release":300,"duck.sensitivity":0.5,
  "echo.amount":0,"echo.time":9,"echo.freeMs":350,"echo.feedback":0.25,
  "echo.routing":0,"echo.pingpong":0,"echo.lowcut":150,"echo.highcut":10000,
  "space.predelay":20,"space.texture":0.70,"space.earlylate":0.30,
  "tone.lowcut":120,"tone.highcut":12000,"tone.lowdamp":0.20,"tone.highdamp":0.40,
  "mod.rate":0.25,"mod.depth":0.15,"stereo.width":1.15,"stereo.monolow":1
};
for (var k in DEFAULTS) P[k]=DEFAULTS[k];

// Not preset content: audition/preference toggles keep their state
var PRESET_SKIP = { "freeze.active":1, "main.solowet":1, "main.autogain":1,
                    "stereo.monolow":1, "main.bypass":1 };

// Value formatters -> HTML with <small> units (family cval style)
function fPct(v){ return Math.round(v*100)+"<small>%</small>"; }
function fSec(v){ return (v<10?v.toFixed(2):v.toFixed(1))+"<small>s</small>"; }
function fMs(v){ return Math.round(v)+"<small>ms</small>"; }
function fHz(v){ return v>=1000 ? (v/1000).toFixed(1)+"<small>kHz</small>" : Math.round(v)+"<small>Hz</small>"; }
function fHzR(v){ return v.toFixed(2)+"<small>Hz</small>"; }
function fEL(v){ var p=Math.round(Math.abs(v)*100);
  return v<-0.005?p+"<small>E</small>":(v>0.005?p+"<small>L</small>":"MID"); }

// ============================================================================
// Arc knob component (family .arck — value inside, label below)
// ============================================================================
var SIZES = {
  l: { box:124, r:55, arc:259, gap:87 },
  m: { box:104, r:46, arc:217, gap:73 },
  s: { box:58,  r:25, arc:118, gap:39 }
};
var knobs = {}; var dragCtx = null;

function clamp01(v){ return v<0?0:(v>1?1:v); }
function toNorm(cfg,v){
  if (cfg.curve==="log") return Math.log(v/cfg.min)/Math.log(cfg.max/cfg.min);
  return (v-cfg.min)/(cfg.max-cfg.min);
}
function fromNorm(cfg,n){
  if (cfg.curve==="log") return cfg.min*Math.pow(cfg.max/cfg.min,n);
  return cfg.min+(cfg.max-cfg.min)*n;
}

function mkKnob(holderId,cfg){
  var S = SIZES[cfg.size||"m"];
  var holder = document.getElementById(holderId);
  var group = document.createElement("div");
  group.className = "knob-group";
  var c = S.box/2;
  group.innerHTML =
    '<div class="arck size-'+(cfg.size||"m")+' knob">'+
      '<svg viewBox="0 0 '+S.box+' '+S.box+'">'+
        '<circle class="track" cx="'+c+'" cy="'+c+'" r="'+S.r+'" stroke-dasharray="'+S.arc+' '+S.gap+'" transform="rotate(135 '+c+' '+c+')"/>'+
        '<circle class="lit" cx="'+c+'" cy="'+c+'" r="'+S.r+'" stroke-dasharray="0 999" transform="rotate(135 '+c+' '+c+')"/>'+
      '</svg>'+
      '<div class="kbody"></div><div class="kdot"></div>'+
      '<div class="cval"></div>'+
    '</div>'+
    '<label>'+cfg.label+'</label>';
  if (cfg.tip) group.setAttribute("data-tip", cfg.tip);
  holder.appendChild(group);

  var el = group.querySelector(".arck");
  var K = {
    cfg:cfg, arcLen:S.arc,
    litEl: el.querySelector(".lit"),
    dotEl: el.querySelector(".kdot"),
    valEl: el.querySelector(".cval"),
    norm: clamp01(toNorm(cfg,P[cfg.id]))
  };
  knobs[cfg.id]=K;

  el.addEventListener("mousedown", function(e){
    dragCtx = { knob:K, startY:e.clientY, startNorm:K.norm };
    emit("beginGesture",{name:cfg.id});
    e.preventDefault();
  });
  el.addEventListener("dblclick", function(){
    setKnob(K, clamp01(toNorm(cfg, DEFAULTS[cfg.id])), true);
  });
  el.addEventListener("wheel", function(e){
    var step = e.shiftKey?0.005:0.03;
    setKnob(K, clamp01(K.norm - Math.sign(e.deltaY)*step), true);
    e.preventDefault();
  },{passive:false});

  renderKnob(K);
  return K;
}
function setKnob(K,norm,send){
  K.norm=clamp01(norm);
  var v=fromNorm(K.cfg,K.norm);
  P[K.cfg.id]=v;
  if (send) setParam(K.cfg.id,v);
  renderKnob(K);
}
function renderKnob(K){
  K.litEl.setAttribute("stroke-dasharray",(K.norm*K.arcLen)+" 999");
  K.litEl.style.opacity = K.norm>0.004 ? "1" : "0";
  K.dotEl.style.transform = "rotate("+(-135+K.norm*270)+"deg)";
  K.valEl.innerHTML = K.cfg.fmt(P[K.cfg.id]);
}

document.addEventListener("mousemove", function(e){
  if(!dragCtx) return;
  var range = e.shiftKey ? 1400 : 220;
  setKnob(dragCtx.knob, dragCtx.startNorm + (dragCtx.startY-e.clientY)/range, true);
});
document.addEventListener("mouseup", function(){
  if(dragCtx){ emit("endGesture",{name:dragCtx.knob.cfg.id}); dragCtx=null; }
  if(charDrag){ emit("endGesture",{name:"main.character"}); charDrag=false; }
});
</script>
)rawhtml"

    + R"rawhtml(<script>
"use strict";
// ============================================================================
// Build controls
// ============================================================================
mkKnob("knob-size", {id:"main.size",label:"Size",min:0,max:1,size:"l",fmt:fPct,
  tip:"Changes the perceived size of the space."});
mkKnob("knob-decay",{id:"main.decay",label:"Decay",min:0.2,max:30,curve:"log",size:"l",fmt:fSec,
  tip:"Controls how long the reverb tail lasts."});
mkKnob("knob-bloom",{id:"main.bloom",label:"Bloom",min:0,max:1,size:"l",fmt:fPct,
  tip:"Makes the late reverb expand into an evolving atmosphere."});
mkKnob("knob-silk",{id:"silk.amount",label:"Silk",min:0,max:1,size:"m",fmt:fPct,
  tip:"Tames harsh sibilance in the tail - a de-esser built into the space."});
mkKnob("knob-mix",{id:"main.mix",label:"Mix",min:0,max:1,size:"m",fmt:fPct,
  tip:"Wet/dry balance of the whole effect."});
mkKnob("knob-duck",{id:"duck.amount",label:"Amount",min:0,max:1,size:"m",fmt:fPct,
  tip:"Moves the wet signal out of the way while the source is active."});
mkKnob("knob-echo",{id:"echo.amount",label:"Amount",min:0,max:1,size:"m",fmt:fPct,
  tip:"Blends tempo-synced echoes into the space."});

// Advanced knobs (size-s)
mkKnob("adv-space",{id:"space.predelay",label:"Pre-Dly",min:0,max:250,size:"s",fmt:fMs,
  tip:"Delay before the space begins."});
mkKnob("adv-space",{id:"space.texture",label:"Texture",min:0,max:1,size:"s",fmt:fPct,
  tip:"Thickness and smoothness of the reverb wash."});
mkKnob("adv-tone",{id:"tone.lowcut",label:"Low Cut",min:20,max:1000,curve:"log",size:"s",fmt:fHz,
  tip:"Removes lows from the wet signal."});
mkKnob("adv-tone",{id:"tone.highcut",label:"High Cut",min:2000,max:20000,curve:"log",size:"s",fmt:fHz,
  tip:"Removes highs from the wet signal."});
mkKnob("adv-tone",{id:"tone.lowdamp",label:"Lo Damp",min:0,max:1,size:"s",fmt:fPct,
  tip:"Makes low frequencies decay faster."});
mkKnob("adv-tone",{id:"tone.highdamp",label:"Hi Damp",min:0,max:1,size:"s",fmt:fPct,
  tip:"Makes high frequencies decay faster."});
mkKnob("adv-mod",{id:"mod.depth",label:"Motion",min:0,max:1,size:"s",fmt:fPct,
  tip:"Amount of movement inside the tail."});
mkKnob("adv-mod",{id:"stereo.width",label:"Width",min:0,max:2,size:"s",fmt:fPct,
  tip:"Stereo width of the space."});
mkKnob("adv-duck",{id:"duck.attack",label:"Attack",min:0.1,max:200,curve:"log",size:"s",fmt:fMs,
  tip:"How fast ducking engages (Custom mode)."});
mkKnob("adv-duck",{id:"duck.hold",label:"Hold",min:0,max:500,size:"s",fmt:fMs,
  tip:"How long ducking holds after the source stops."});
mkKnob("adv-duck",{id:"duck.release",label:"Release",min:20,max:3000,curve:"log",size:"s",fmt:fMs,
  tip:"How fast the space returns."});
mkKnob("adv-duck",{id:"duck.sensitivity",label:"Sens",min:0,max:1,size:"s",fmt:fPct,
  tip:"How easily the ducker reacts to the source."});
mkKnob("adv-echo",{id:"echo.feedback",label:"Feedback",min:0,max:0.95,size:"s",fmt:fPct,
  tip:"Number of repeats."});
mkKnob("adv-echo",{id:"echo.freeMs",label:"Free Ms",min:10,max:2000,curve:"log",size:"s",fmt:fMs,
  tip:"Echo time when TIME is set to Free."});
mkKnob("adv-echo",{id:"echo.lowcut",label:"Low Cut",min:20,max:2000,curve:"log",size:"s",fmt:fHz,
  tip:"Removes lows from the repeats."});
mkKnob("adv-echo",{id:"echo.highcut",label:"High Cut",min:1000,max:20000,curve:"log",size:"s",fmt:fHz,
  tip:"Removes highs from the repeats."});

// Echo time select
var ECHO_TIMES=["1/32","1/16T","1/16","1/16D","1/8T","1/8","1/8D","1/4T","1/4","1/4D",
  "1/2T","1/2","1/2D","1 BAR","2 BARS","FREE"];
var selTime=document.getElementById("sel-echotime");
ECHO_TIMES.forEach(function(t){ var o=document.createElement("option"); o.textContent=t; selTime.appendChild(o); });
selTime.selectedIndex=9;
selTime.addEventListener("change",function(){ P["echo.time"]=selTime.selectedIndex; setParam("echo.time",selTime.selectedIndex); });

var selMode=document.getElementById("sel-duckmode");
selMode.addEventListener("change",function(){
  P["duck.mode"]=selMode.selectedIndex; setParam("duck.mode",selMode.selectedIndex); updateDuckHint();
});
function updateDuckHint(){
  document.getElementById("duck-hint").style.opacity = (Math.round(P["duck.mode"])===4)?"0":"1";
}

// Routing segmented
var segRouting=document.getElementById("seg-routing");
segRouting.querySelectorAll("span").forEach(function(s){
  s.addEventListener("click",function(){
    var i=parseInt(s.getAttribute("data-i"));
    P["echo.routing"]=i; setParam("echo.routing",i); renderRouting();
  });
});
function renderRouting(){
  var cur=Math.round(P["echo.routing"]);
  segRouting.querySelectorAll("span").forEach(function(s){
    s.classList.toggle("on",parseInt(s.getAttribute("data-i"))===cur);
  });
}

// Toggles
function bindToggle(btnId,paramId,cls){
  var b=document.getElementById(btnId);
  b.addEventListener("click",function(){
    var nv = P[paramId]>0.5?0:1; P[paramId]=nv; setParam(paramId,nv);
    b.classList.toggle(cls,nv>0.5);
  });
  return b;
}
var btnFreeze=bindToggle("freeze-btn","freeze.active","active");
var btnBypass=bindToggle("bypass","main.bypass","on");
var btnPP=bindToggle("pp-btn","echo.pingpong","on");
var btnSolo=bindToggle("solowet-btn","main.solowet","on");
var btnAG=bindToggle("ag-btn","main.autogain","on");
var btnML=bindToggle("ml-btn","stereo.monolow","on");

// Character slider
var charDrag=false;
var charTrack=document.getElementById("char-track");
function renderChar(){
  var v=P["main.character"], w=charTrack.clientWidth;
  document.getElementById("char-fill").style.width=(v*w)+"px";
  document.getElementById("char-handle").style.left=(v*w)+"px";
  document.getElementById("char-clean").classList.toggle("on",v<0.25);
  document.getElementById("char-dream").classList.toggle("on",v>0.75);
}
charTrack.addEventListener("mousedown",function(e){
  charDrag=true; emit("beginGesture",{name:"main.character"}); charFromEvent(e);
});
document.addEventListener("mousemove",function(e){ if(charDrag) charFromEvent(e); });
function charFromEvent(e){
  var r=charTrack.getBoundingClientRect();
  var v=clamp01((e.clientX-r.left)/r.width);
  P["main.character"]=v; setParam("main.character",v); renderChar();
}
charTrack.addEventListener("dblclick",function(){
  P["main.character"]=DEFAULTS["main.character"]; setParam("main.character",P["main.character"]); renderChar();
});

// Advanced drawer toggle
var advBtn=document.getElementById("adv-btn");
var advDrawer=document.getElementById("adv-drawer");
advBtn.addEventListener("click",function(){
  var open=!advDrawer.classList.contains("open");
  advDrawer.classList.toggle("open",open);
  advBtn.classList.toggle("active",open);
});
document.addEventListener("keydown",function(e){
  if(e.key==="Escape"){
    advDrawer.classList.remove("open"); advBtn.classList.remove("active");
    presetMenu.classList.remove("open");
  }
});

// About
document.getElementById("about-btn").addEventListener("click",function(){ emit("showAbout"); });
</script>
)rawhtml"

    + R"rawhtml(<script>
"use strict";
// ============================================================================
// Factory presets (MVP library, spec §13/§28)
// ============================================================================
var PRESETS=[
 {n:"Init - Clean Space",c:"INIT",p:{}},
 {n:"Huge Clean Vocal",c:"VOCALS",p:{"main.mix":0.32,"main.size":0.72,"main.decay":2.85,"main.bloom":0.30,
   "main.character":0.35,"duck.amount":0.54,"duck.mode":2,"echo.amount":0.28,"echo.time":9,"echo.feedback":0.30,
   "space.predelay":35,"tone.lowcut":150,"tone.highcut":11000,"stereo.width":1.3}},
 {n:"Wide Pop Tail",c:"VOCALS",p:{"main.size":0.60,"main.decay":1.8,"main.bloom":0.20,"main.character":0.30,
   "duck.amount":0.45,"duck.mode":2,"echo.amount":0.15,"echo.time":5,"space.predelay":25,"stereo.width":1.5}},
 {n:"Dream Vocal Throw",c:"VOCALS",p:{"main.mix":0.45,"main.size":0.85,"main.decay":5.5,"main.bloom":0.55,
   "main.character":0.65,"duck.amount":0.50,"duck.mode":2,"echo.amount":0.35,"echo.time":9,"echo.feedback":0.45}},
 {n:"Dark Intimate Hall",c:"VOCALS",p:{"main.size":0.45,"main.decay":1.4,"main.bloom":0.05,"main.character":0.15,
   "duck.amount":0.35,"duck.mode":2,"tone.highcut":7500,"tone.highdamp":0.6,"space.predelay":12}},
 {n:"Psy Lead Space",c:"LEADS",p:{"main.size":0.65,"main.decay":2.4,"main.character":0.40,"duck.amount":0.60,
   "echo.amount":0.40,"echo.time":6,"echo.feedback":0.35,"echo.pingpong":1,"tone.lowcut":200,"space.predelay":10}},
 {n:"Clean 1/8D Lead",c:"LEADS",p:{"main.size":0.50,"main.decay":1.6,"main.character":0.20,"duck.amount":0.50,
   "echo.amount":0.45,"echo.time":6}},
 {n:"Festival Hall",c:"LEADS",p:{"main.size":0.90,"main.decay":4.5,"main.bloom":0.35,"duck.amount":0.45,
   "stereo.width":1.4,"space.predelay":40}},
 {n:"Acid Echo Space",c:"LEADS",p:{"main.size":0.55,"main.decay":2.0,"main.character":0.50,"echo.amount":0.55,
   "echo.time":2,"echo.feedback":0.55,"echo.pingpong":1,"echo.routing":2}},
 {n:"Infinite Violet Cloud",c:"ATMOSPHERES",p:{"main.mix":0.60,"main.size":0.95,"main.decay":18,"main.bloom":0.70,
   "main.character":0.75,"duck.amount":0.20,"mod.depth":0.30,"stereo.width":1.6}},
 {n:"Deep Temple",c:"ATMOSPHERES",p:{"main.size":0.85,"main.decay":8,"main.bloom":0.40,"main.character":0.45,
   "tone.lowdamp":0.35,"tone.highdamp":0.55,"space.predelay":60}},
 {n:"Blooming Sky",c:"ATMOSPHERES",p:{"main.size":0.80,"main.decay":6,"main.bloom":0.85,"main.character":0.60,
   "echo.amount":0.20,"echo.time":11,"echo.feedback":0.40,"stereo.width":1.5}},
 {n:"Frozen Air",c:"ATMOSPHERES",p:{"main.mix":0.50,"main.size":0.75,"main.decay":12,"main.bloom":0.50,
   "main.character":0.55,"tone.highcut":16000}},
 {n:"Tight Transient Room",c:"DRUMS",p:{"main.size":0.30,"main.decay":0.7,"main.bloom":0,"main.character":0.10,
   "duck.amount":0.50,"duck.mode":3,"space.predelay":5,"stereo.width":1.0}},
 {n:"Big Snare Bloom",c:"DRUMS",p:{"main.size":0.60,"main.decay":1.9,"main.bloom":0.45,"main.character":0.35,
   "duck.amount":0.40,"duck.mode":3}},
 {n:"Percussion Chamber",c:"DRUMS",p:{"main.size":0.40,"main.decay":1.1,"main.character":0.20,
   "duck.amount":0.45,"duck.mode":3,"tone.lowcut":250}},
 {n:"Silk Pad Widener",c:"PADS",p:{"main.mix":0.35,"main.size":0.70,"main.decay":3.5,"main.bloom":0.30,
   "main.character":0.50,"duck.amount":0.15,"mod.depth":0.20,"stereo.width":1.7}},
 {n:"End Of Phrase Throw",c:"FX / THROWS",p:{"main.mix":0.55,"main.size":0.80,"main.decay":4,"main.character":0.50,
   "duck.amount":0.30,"echo.amount":0.50,"echo.time":8,"echo.feedback":0.50}},
 {n:"Riser Space",c:"FX / THROWS",p:{"main.mix":0.50,"main.size":1.0,"main.decay":10,"main.bloom":0.60,
   "main.character":0.70,"stereo.width":1.6}},
 {n:"Cathedral Of Glass",c:"HUGE SPACES",p:{"main.size":0.95,"main.decay":7,"main.bloom":0.25,"main.character":0.40,
   "tone.highcut":15000,"space.predelay":45,"stereo.width":1.45}},
 {n:"Violet Dream Machine",c:"CREATIVE",p:{"main.size":0.80,"main.decay":5,"main.bloom":0.80,"main.character":0.90,
   "echo.amount":0.40,"echo.time":7,"echo.feedback":0.60,"stereo.width":1.7}},
 {n:"Ducked Infinity",c:"CREATIVE",p:{"main.mix":0.50,"main.size":0.90,"main.decay":25,"main.bloom":0.50,
   "duck.amount":0.80,"stereo.width":1.5}}
];
var currentPreset=0, lockMix=false;
var presetMenu=document.getElementById("preset-menu");

function applyPreset(i){
  currentPreset=(i+PRESETS.length)%PRESETS.length;
  var pr=PRESETS[currentPreset];
  for (var id in DEFAULTS){
    if (lockMix && id==="main.mix") continue;
    if (PRESET_SKIP[id]) continue;
    var v = (pr.p[id]!==undefined)?pr.p[id]:DEFAULTS[id];
    P[id]=v; setParam(id,v);
  }
  emit("setPresetName",{name:pr.n});
  document.getElementById("preset-name").textContent=pr.n.toUpperCase();
  refreshAllControls(); renderPresetMenu();
}
function renderPresetMenu(){
  presetMenu.innerHTML="";
  var lastCat=null;
  PRESETS.forEach(function(pr,i){
    if(pr.c!==lastCat){ lastCat=pr.c;
      var h=document.createElement("div"); h.className="p-cat"; h.textContent=pr.c; presetMenu.appendChild(h); }
    var it=document.createElement("div"); it.className="p-item"+(i===currentPreset?" current":"");
    it.textContent=pr.n;
    it.addEventListener("click",function(e){ e.stopPropagation(); applyPreset(i); });
    presetMenu.appendChild(it);
  });
  var sep=document.createElement("div"); sep.className="p-sep"; presetMenu.appendChild(sep);
  var lock=document.createElement("div"); lock.className="p-lock";
  lock.innerHTML='<input type="checkbox" '+(lockMix?"checked":"")+'>LOCK MIX WHILE BROWSING';
  lock.addEventListener("click",function(e){
    e.stopPropagation(); lockMix=!lockMix; renderPresetMenu();
  });
  presetMenu.appendChild(lock);
}
document.getElementById("preset-name").addEventListener("click",function(e){
  e.stopPropagation(); renderPresetMenu(); presetMenu.classList.toggle("open");
});
document.getElementById("p-prev").addEventListener("click",function(){ applyPreset(currentPreset-1); });
document.getElementById("p-next").addEventListener("click",function(){ applyPreset(currentPreset+1); });
document.addEventListener("click",function(e){
  if(!presetMenu.contains(e.target) && e.target.id!=="preset-name") presetMenu.classList.remove("open");
});

// ============================================================================
// Refresh UI from P
// ============================================================================
function refreshAllControls(){
  for (var id in knobs){
    var K=knobs[id];
    K.norm=clamp01(toNorm(K.cfg,P[id]));
    renderKnob(K);
  }
  selTime.selectedIndex=Math.round(P["echo.time"]);
  selMode.selectedIndex=Math.round(P["duck.mode"]);
  btnFreeze.classList.toggle("active",P["freeze.active"]>0.5);
  btnBypass.classList.toggle("on",P["main.bypass"]>0.5);
  btnPP.classList.toggle("on",P["echo.pingpong"]>0.5);
  btnSolo.classList.toggle("on",P["main.solowet"]>0.5);
  btnAG.classList.toggle("on",P["main.autogain"]>0.5);
  btnML.classList.toggle("on",P["stereo.monolow"]>0.5);
  renderRouting(); renderChar(); updateDuckHint();
  document.getElementById("spatial-val").textContent =
    P["stereo.width"]>1.35?"WIDE":(P["stereo.width"]<0.65?"NARROW":"STEREO");
  document.getElementById("engine-val").textContent =
    P["main.bypass"]>0.5?"BYPASS":(P["freeze.active"]>0.5?"FROZEN":"ACTIVE");
}

// ============================================================================
// Host bridge — receive state
// ============================================================================
var vizEnergy=[], duckGrDb=0, wetRms=0, grHistory=[];
for (var gh=0; gh<120; ++gh) grHistory.push(0);

function setupBridge(){
  if(!window.__JUCE__||!window.__JUCE__.backend){ setTimeout(setupBridge,50); return; }
  var B=window.__JUCE__.backend;
  B.addEventListener("parameterState",function(d){
    for (var id in DEFAULTS){
      if (d[id]===undefined) continue;
      if (dragCtx && dragCtx.knob.cfg.id===id) continue;
      if (charDrag && id==="main.character") continue;
      P[id]=d[id];
    }
    refreshAllControls();
  });
  B.addEventListener("hostState",function(d){
    var name=(d.presetName||"").toUpperCase();
    var el=document.getElementById("preset-name");
    if(name && el.textContent!==name) el.textContent=name;
    document.getElementById("bpm-val").textContent=Math.round(d.effectiveBPM);
    var ms=d.echoTimeMs?Math.round(d.echoTimeMs):0;
    document.getElementById("echo-ms").textContent = ms+" MS";
  });
  B.addEventListener("visualizerData",function(d){
    vizEnergy=d.energyProfile||[];
    duckGrDb=d.duckGrDb||0;
    wetRms=d.wetRms||0;
    grHistory.push(duckGrDb); if(grHistory.length>120) grHistory.shift();
    document.getElementById("duckgr-val").textContent="-"+duckGrDb.toFixed(1);
  });
  B.addEventListener("licenseStatus",function(d){ /* beta: always licensed */ });
}
setupBridge();

// ============================================================================
// Tooltips (spec §12 — one sentence, short hover delay)
// ============================================================================
var tipEl=document.getElementById("tooltip"), tipTimer=null;
document.addEventListener("mouseover",function(e){
  var t=e.target.closest("[data-tip]");
  if(!t){ hideTip(); return; }
  clearTimeout(tipTimer);
  tipTimer=setTimeout(function(){
    tipEl.textContent=t.getAttribute("data-tip");
    var r=t.getBoundingClientRect();
    tipEl.style.left=Math.min(window.innerWidth-270,Math.max(8,r.left+r.width/2-80))+"px";
    tipEl.style.top=Math.min(window.innerHeight-50,r.bottom+8)+"px";
    tipEl.classList.add("show");
  },650);
});
document.addEventListener("mouseout",function(){ hideTip(); });
function hideTip(){ clearTimeout(tipTimer); tipEl.classList.remove("show"); }
</script>
)rawhtml"

    + R"rawhtml(<script>
"use strict";
// ============================================================================
// Space visualization — violet nebula in the display card (spec §23)
// Size -> radius | Decay -> trail persistence | Bloom -> diffusion |
// Width -> horizontal spread | Duck -> contraction | Freeze -> suspension
// ============================================================================
var viz=document.getElementById("viz"), vctx=viz.getContext("2d");
var VW=0, VH=0;
function sizeViz(){
  var w=viz.clientWidth, h=viz.clientHeight;
  if (w>0 && h>0){ viz.width=VW=w*2; viz.height=VH=h*2; }
}
sizeViz(); window.addEventListener("resize",sizeViz);

var particles=[];
(function(){
  for (var i=0;i<150;++i){
    particles.push({
      ang:Math.random()*Math.PI*2,
      rad:0.25+Math.pow(Math.random(),0.7)*0.85,
      spd:(0.0018+Math.random()*0.004)*(Math.random()<0.5?1:-1),
      sz:0.9+Math.random()*2.1,
      tw:Math.random()*Math.PI*2,
      band:Math.random()
    });
  }
})();
var lastFrame=0;
function drawViz(ts){
  requestAnimationFrame(drawViz);
  if(ts-lastFrame<33) return; // ~30fps cap (spec §23 PERFORMANCE)
  var dt=Math.min(3,(ts-lastFrame)/33); lastFrame=ts;
  if(VW===0) sizeViz();
  if(VW===0) return;

  var size=P["main.size"], decayN=clamp01(Math.log(P["main.decay"]/0.2)/Math.log(150));
  var bloom=P["main.bloom"], width=P["stereo.width"];
  var frozen=P["freeze.active"]>0.5, bypassed=P["main.bypass"]>0.5;
  var grK=clamp01(duckGrDb/24);
  var energy=0.40+clamp01(wetRms*5)*0.60;
  if (bypassed) energy*=0.25;

  // Trail fade: long decay -> long persistence
  vctx.globalCompositeOperation="source-over";
  vctx.fillStyle="rgba(16,18,22,"+(0.34-0.30*decayN).toFixed(3)+")";
  vctx.fillRect(0,0,VW,VH);

  var cx=VW/2, cy=VH/2;
  var baseR=VH*0.44*(0.40+0.60*size);
  var wScale=(0.85+width*0.28)*(1-grK*0.16);
  var vScale=0.46*(1-grK*0.16);

  // Dotted orbit guide
  vctx.strokeStyle="rgba(255,138,61,0.10)";
  vctx.lineWidth=1.5; vctx.setLineDash([2,9]);
  vctx.beginPath();
  vctx.ellipse(cx,cy,baseR*1.28*wScale,baseR*1.28*vScale,0,0,7);
  vctx.stroke(); vctx.setLineDash([]);

  // Core glow
  var coreR=baseR*0.5;
  var cg=vctx.createRadialGradient(cx,cy,0,cx,cy,coreR);
  cg.addColorStop(0,"rgba(255,196,150,"+(0.26*energy*(1-grK*0.4)).toFixed(3)+")");
  cg.addColorStop(0.5,"rgba(255,138,61,"+(0.11*energy).toFixed(3)+")");
  cg.addColorStop(1,"rgba(255,138,61,0)");
  vctx.fillStyle=cg;
  vctx.beginPath(); vctx.ellipse(cx,cy,coreR*wScale,coreR*vScale*1.3,0,0,7); vctx.fill();

  // Particles
  vctx.globalCompositeOperation="lighter";
  var speedK=frozen?0.03:1;
  for (var i=0;i<particles.length;++i){
    var p=particles[i];
    p.ang+=p.spd*dt*speedK;
    p.tw+=0.05*dt*(frozen?0.15:1);
    var spread=1+bloom*p.band*0.85;
    var px=cx+Math.cos(p.ang)*baseR*p.rad*wScale*spread;
    var py=cy+Math.sin(p.ang)*baseR*p.rad*vScale*spread
           +Math.sin(p.ang*2.3+p.tw*0.4)*VH*0.02*bloom;
    var twk=0.45+0.55*Math.sin(p.tw)*Math.sin(p.tw);
    var a=(0.05+0.30*twk)*energy*(1-grK*0.45);
    var white=p.rad<0.45?0.6:0.15;
    vctx.fillStyle="rgba(255,"+Math.round(138+80*white)+","+Math.round(61+120*white)+","+a.toFixed(3)+")";
    vctx.beginPath(); vctx.arc(px,py,p.sz*(1+bloom*0.5),0,7); vctx.fill();
  }
  vctx.globalCompositeOperation="source-over";
}
requestAnimationFrame(drawViz);

// ============================================================================
// Duck meter — thin GR history line in a card (spec §7 METERING)
// ============================================================================
var dm=document.getElementById("duck-meter"), dctx=dm.getContext("2d");
function drawDuckMeter(){
  requestAnimationFrame(drawDuckMeter);
  var w=dm.clientWidth, h=dm.clientHeight;
  if (w>0 && (dm.width!==w*2 || dm.height!==h*2)){ dm.width=w*2; dm.height=h*2; }
  var W=dm.width,H=dm.height;
  if(W===0) return;
  dctx.clearRect(0,0,W,H);
  dctx.strokeStyle="rgba(255,255,255,0.07)";
  dctx.lineWidth=1;
  dctx.beginPath(); dctx.moveTo(0,H*0.28); dctx.lineTo(W,H*0.28); dctx.stroke();
  dctx.strokeStyle="#FF8A3D"; dctx.lineWidth=2.5; dctx.lineJoin="round";
  dctx.shadowColor="rgba(255,138,61,0.5)"; dctx.shadowBlur=6;
  dctx.beginPath();
  for (var i=0;i<grHistory.length;++i){
    var x=i/(grHistory.length-1)*W;
    var y=H*0.28+clamp01(grHistory[i]/24)*H*0.55;
    if(i===0) dctx.moveTo(x,y); else dctx.lineTo(x,y);
  }
  dctx.stroke();
  dctx.shadowBlur=0;
}
requestAnimationFrame(drawDuckMeter);

// ============================================================================
// Echo dots — repeats fading by feedback, glowing violet
// ============================================================================
var ed=document.getElementById("echo-dots"), ectx=ed.getContext("2d");
var echoPulse=0;
function drawEchoDots(){
  requestAnimationFrame(drawEchoDots);
  var w=ed.clientWidth, h=ed.clientHeight;
  if (w>0 && (ed.width!==w*2 || ed.height!==h*2)){ ed.width=w*2; ed.height=h*2; }
  var W=ed.width,H=ed.height,cy2=H/2;
  if(W===0) return;
  ectx.clearRect(0,0,W,H);
  var amt=P["echo.amount"], fb=P["echo.feedback"];
  echoPulse=(echoPulse+0.012)%1;
  var n=6, x0=W*0.10, dx=W*0.15;
  ectx.strokeStyle="rgba(255,255,255,0.06)"; ectx.lineWidth=1;
  ectx.beginPath(); ectx.moveTo(x0-16,cy2); ectx.lineTo(x0+dx*(n-1)+18,cy2); ectx.stroke();
  for (var i=0;i<n;++i){
    var g=Math.pow(Math.max(0.18,fb),i);
    var a=(0.10+0.75*amt)*g;
    var r=5+9*g*(0.6+0.4*amt);
    var pulse=(i/n>echoPulse-0.08 && i/n<echoPulse)?1.35:1;
    ectx.shadowColor="rgba(255,138,61,0.6)"; ectx.shadowBlur=amt>0.01?10:0;
    ectx.fillStyle="rgba(255,138,61,"+Math.min(0.95,a*pulse).toFixed(3)+")";
    ectx.beginPath(); ectx.arc(x0+dx*i,cy2,r*pulse,0,7); ectx.fill();
  }
  ectx.shadowBlur=0;
}
requestAnimationFrame(drawEchoDots);

// ============================================================================
// Proportional UI scaling + corner resize grip
// The editor resizes (host frame or the grip below); #app scales to fit.
// ============================================================================
var appEl=document.getElementById("app");
var BASE_W=1150, BASE_H=780;
function fitScale(){
  var k=Math.min(window.innerWidth/BASE_W, window.innerHeight/BASE_H);
  var ox=Math.max(0,(window.innerWidth-BASE_W*k)/2);
  var oy=Math.max(0,(window.innerHeight-BASE_H*k)/2);
  appEl.style.transform="translate("+ox.toFixed(1)+"px,"+oy.toFixed(1)+"px) scale("+k.toFixed(4)+")";
}
window.addEventListener("resize",fitScale);
fitScale();

var rsDrag=null;
document.getElementById("resize-handle").addEventListener("mousedown",function(e){
  rsDrag={x:e.screenX,y:e.screenY,w:window.innerWidth,h:window.innerHeight};
  e.preventDefault(); e.stopPropagation();
});
document.addEventListener("mousemove",function(e){
  if(!rsDrag) return;
  emit("requestResize",{width:rsDrag.w+(e.screenX-rsDrag.x),
                        height:rsDrag.h+(e.screenY-rsDrag.y)});
});
document.addEventListener("mouseup",function(){ rsDrag=null; });

renderChar(); refreshAllControls();
window.addEventListener("resize",renderChar);
</script>
</body>
</html>
)rawhtml";

    return html;
}
} // namespace WebUI
