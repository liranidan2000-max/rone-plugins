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
)rawhtml") + std::string (R"rawhtml(<style>
/* RONE UI fonts, served by the plugin itself (Shared/fonts, roneFontResource):
   the page has its type with no network. Latin subsets from @fontsource 5.3.0,
   SIL OFL 1.1. Sora / Manrope / Archivo are variable: one file, every weight. */
@font-face{font-family:'Sora';font-style:normal;font-weight:100 800;font-display:swap;src:url('fonts/sora-latin-wght-normal.woff2') format('woff2-variations'),url('fonts/sora-latin-wght-normal.woff2') format('woff2')}
@font-face{font-family:'Manrope';font-style:normal;font-weight:200 800;font-display:swap;src:url('fonts/manrope-latin-wght-normal.woff2') format('woff2-variations'),url('fonts/manrope-latin-wght-normal.woff2') format('woff2')}
@font-face{font-family:'Archivo';font-style:normal;font-weight:100 900;font-display:swap;src:url('fonts/archivo-latin-wght-normal.woff2') format('woff2-variations'),url('fonts/archivo-latin-wght-normal.woff2') format('woff2')}
@font-face{font-family:'IBM Plex Mono';font-style:normal;font-weight:400;font-display:swap;src:url('fonts/ibm-plex-mono-latin-400-normal.woff2') format('woff2')}
@font-face{font-family:'IBM Plex Mono';font-style:normal;font-weight:500;font-display:swap;src:url('fonts/ibm-plex-mono-latin-500-normal.woff2') format('woff2')}
@font-face{font-family:'IBM Plex Mono';font-style:normal;font-weight:600;font-display:swap;src:url('fonts/ibm-plex-mono-latin-600-normal.woff2') format('woff2')}
</style>
)rawhtml") + std::string (R"rawhtml(<style>/* rone-ui v2 */
/* ============================================================================
   RONE UI kit v2 (26.10.2026) - the graphite house language as one stylesheet.
   Decided with Liran on the controls-finish mockup (claude.ai/artifact/PL1XNPMJeg44Dqu24SAQXC):
     - the knob is the .arck of Reverse Reverb / Flanger (value inside, white dot, arc outside,
       no ticks); its lit arc glows only while touched (.touch) and on the hero (.hero)
     - keys are machined: no 1px grey border, a top highlight and a dark bottom edge, ONE on-state
       (neon fill), an action key carries a LED; a pressed key only sinks 1px (no animation)
     - only things that MOVE animate: the segment pill glides (380 ms, --pop), the switch thumb pops
     - one popover for everything that opens (presets, selects, header menus, pickers)
     - a 3 % grain on caps and panels (.rone-grain)
   Use: paste after Shared/fonts/rone-fonts.css, then set the plugin's neon once:
       :root { --neon:#FF5FB8; --neon-rgb:255,95,184; }
   Pair with rone-ui.js (RoneUI.knob / seg / menu / select / switch). Demo: Shared/ui/demo.html.
   ============================================================================ */

:root {
    --ground:#14161A; --panel:#17191E; --card:#101216; --drawer:#1B1E23;
    --line:#23262C; --line2:#2A2E35; --line3:#383D45;
    --text:#E8EAED; --text2:#B9BDC4;
    --dim:#7A7F88;      /* the lowest colour for a label anyone has to read (4.5:1) */
    --faint:#4E535B;    /* decoration only: ticks, dividers, disabled - never a label */
    --hot:#FF3B3B; --hot-rgb:255,59,59;
    --ink:#0F1114;      /* text on a lit neon surface */
    --neon:#E8EAED; --neon-rgb:232,234,237;
    --pop:cubic-bezier(.34,1.56,.64,1);   /* the one overshoot curve for things that move */
    --font-display:'Sora','Segoe UI',-apple-system,sans-serif;
    --font-ui:'Manrope','Segoe UI',-apple-system,sans-serif;
    --font-mono:'IBM Plex Mono','Consolas',monospace;
    --rone-noise:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='160' height='160'%3E%3Cfilter id='n'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='.9' numOctaves='2' stitchTiles='stitch'/%3E%3CfeColorMatrix values='0 0 0 0 1 0 0 0 0 1 0 0 0 0 1 0 0 0 .05 0'/%3E%3C/filter%3E%3Crect width='160' height='160' filter='url(%23n)'/%3E%3C/svg%3E");
}

/* ---- type roles: nothing under 9 px on the design canvas ---- */
.rone-display { font-family:var(--font-display); font-weight:800; letter-spacing:.02em; color:var(--text); }
.rone-label   { font-family:var(--font-ui); font-size:9px; font-weight:800; letter-spacing:.18em; text-transform:uppercase; color:var(--dim); }
.rone-value   { font-family:var(--font-display); font-weight:700; color:var(--text); font-variant-numeric:tabular-nums; }
.rone-mono    { font-family:var(--font-mono); font-weight:500; font-variant-numeric:tabular-nums; }

/* ---- grain: a 3 % noise overlay on any panel (the element needs position:relative) ---- */
.rone-grain::before { content:''; position:absolute; inset:0; background:var(--rone-noise); opacity:.5; mix-blend-mode:screen; pointer-events:none; border-radius:inherit; }
.rone-panel { position:relative; background:linear-gradient(180deg,var(--panel) 0%,var(--ground) 100%); border-radius:12px; box-shadow:inset 0 1px 0 rgba(255,255,255,.04); }

/* ---- the knob: .arck, as Reverse Reverb / Flanger draw it (geometry untouched) ----
   DOM: .arck.size-m > svg(path.track, path.lit) + .kbody + .kdot + .cval        */
.arck { position:relative; flex-shrink:0; cursor:grab; outline:none; touch-action:none; }
.arck:active { cursor:grabbing; }
.arck.locked { cursor:default; }
.arck svg { position:absolute; inset:0; pointer-events:none; overflow:visible; }
.arck .track { fill:none; stroke:var(--line); stroke-linecap:round; }
.arck .lit { fill:none; stroke:var(--neon); stroke-linecap:round; transition:filter .5s ease-out, opacity .1s; }
.arck.touch .lit, .arck.hero .lit { filter:drop-shadow(0 0 5px rgba(var(--neon-rgb),.6)); }
.arck .kbody { position:absolute; border-radius:50%;
    background:radial-gradient(circle at 38% 30%,#363B42 0%,#26292F 52%,#1A1C21 100%);
    box-shadow:0 12px 24px rgba(0,0,0,.55), inset 0 2px 3px rgba(255,255,255,.07), inset 0 -7px 16px rgba(0,0,0,.45); }
.arck .kbody::before { content:''; position:absolute; inset:9%; border-radius:50%;
    background:radial-gradient(circle at 40% 32%,#2C3037 0%,#1D2025 60%,#16181C 100%);
    box-shadow:inset 0 1px 2px rgba(255,255,255,.05), 0 2px 6px rgba(0,0,0,.4); }
.arck .kbody::after { content:''; position:absolute; inset:0; border-radius:50%; background:var(--rone-noise); opacity:.3; mix-blend-mode:overlay; pointer-events:none; }
.arck .kdot { position:absolute; left:50%; border-radius:50%; background:#F2F4F6; box-shadow:0 0 7px rgba(255,255,255,.5); transform:rotate(-135deg); z-index:2; pointer-events:none; }
.arck .cval { position:absolute; inset:0; display:flex; align-items:center; justify-content:center; pointer-events:none;
    font-family:var(--font-display); font-weight:700; color:var(--text); text-transform:uppercase; z-index:3; font-variant-numeric:tabular-nums; }
.arck .cval small { font-size:.55em; font-weight:700; margin-left:2px; }
.arck.size-s { width:58px;  height:58px;  } .arck.size-s .track, .arck.size-s .lit { stroke-width:3.5; } .arck.size-s .kbody { inset:8px;  } .arck.size-s .kdot { top:11px; width:4px;  height:4px;  margin-left:-2px;   transform-origin:50% 18px; } .arck.size-s .cval { font-size:10px; }
.arck.size-m { width:104px; height:104px; } .arck.size-m .track, .arck.size-m .lit { stroke-width:4.5; } .arck.size-m .kbody { inset:13px; } .arck.size-m .kdot { top:21px; width:6px;  height:6px;  margin-left:-3px;   transform-origin:50% 31px; } .arck.size-m .cval { font-size:15px; }
.arck.size-l { width:124px; height:124px; } .arck.size-l .track, .arck.size-l .lit { stroke-width:5;   } .arck.size-l .kbody { inset:15px; } .arck.size-l .kdot { top:23px; width:7px;  height:7px;  margin-left:-3.5px; transform-origin:50% 39px; } .arck.size-l .cval { font-size:18px; }
/* hero: the same knob scaled; value + name inside, the lit arc always glows */
.arck.size-xl { width:200px; height:200px; } .arck.size-xl .track, .arck.size-xl .lit { stroke-width:7; } .arck.size-xl .kbody { inset:24px; } .arck.size-xl .kdot { top:36px; width:10px; height:10px; margin-left:-5px; transform-origin:50% 64px; }
.arck.size-xl .cval { font-size:36px; flex-direction:column; gap:2px; letter-spacing:-.02em; }
.arck .cval .sub { font-family:var(--font-ui); font-size:9px; font-weight:800; letter-spacing:.24em; color:var(--dim); }
.arck:focus-visible .kbody { box-shadow:0 0 0 2px var(--neon), 0 12px 24px rgba(0,0,0,.55); }
.rone-knobgroup { display:flex; flex-direction:column; align-items:center; gap:6px; }

/* ---- keys ---- */
.rone-btn { font-family:var(--font-ui); font-size:10px; font-weight:700; letter-spacing:.12em; text-transform:uppercase; color:var(--text2);
    background:linear-gradient(180deg,#1E2126,#15171B); border:0; border-radius:7px; padding:9px 14px; cursor:pointer; position:relative; white-space:nowrap;
    box-shadow:inset 0 1px 0 rgba(255,255,255,.06), 0 1px 0 rgba(0,0,0,.6), 0 0 0 1px rgba(0,0,0,.35);
    transition:box-shadow .18s, color .15s, background .18s; }
.rone-btn:hover { color:var(--text); }
.rone-btn:active { transform:translateY(1px); box-shadow:inset 0 2px 4px rgba(0,0,0,.6), 0 0 0 1px rgba(0,0,0,.35); }
)rawhtml") + std::string (R"rawhtml(.rone-btn.on, .rone-btn[aria-pressed="true"] { color:var(--ink);
    background:linear-gradient(180deg,rgba(255,255,255,.18),rgba(255,255,255,0)), var(--neon);
    box-shadow:inset 0 1px 0 rgba(255,255,255,.35), inset 0 -1px 0 rgba(0,0,0,.25), 0 1px 0 rgba(0,0,0,.6), 0 0 0 1px rgba(0,0,0,.35); }
.rone-btn.hot.on { background:linear-gradient(180deg,rgba(255,255,255,.18),rgba(255,255,255,0)), var(--hot); color:#fff; }
.rone-btn.act { color:var(--text); }
.rone-btn.act::before { content:''; display:inline-block; width:6px; height:6px; border-radius:50%; background:var(--neon); box-shadow:0 0 6px rgba(var(--neon-rgb),.8); margin-right:8px; vertical-align:1px; }
.rone-btn.act.wait::before { background:var(--faint); box-shadow:none; }
.rone-btn.dd::after { content:''; display:inline-block; width:5px; height:5px; border-right:1.5px solid currentColor; border-bottom:1.5px solid currentColor; transform:rotate(45deg); margin-left:9px; vertical-align:2px; transition:transform .2s; }
.rone-btn.dd.open::after { transform:rotate(225deg) translate(-1px,-1px); }
.rone-btn.icon { padding:9px 11px; }
.rone-btn.arrow { padding:9px 10px; color:var(--dim); }
.rone-btn:disabled { color:var(--faint); cursor:default; transform:none; box-shadow:0 0 0 1px rgba(0,0,0,.25); }
.rone-btn:focus-visible { outline:2px solid var(--neon); outline-offset:2px; }

/* ---- preset bar: ◀ name ▶ (the name opens the preset popover) ---- */
.rone-preset { display:inline-flex; align-items:center; gap:2px; background:var(--card); border-radius:8px; padding:3px; box-shadow:inset 0 1px 3px rgba(0,0,0,.6), 0 1px 0 rgba(255,255,255,.04); }
.rone-preset .nm { font-family:var(--font-ui); font-size:10px; font-weight:700; letter-spacing:.1em; text-transform:uppercase; color:var(--text); padding:6px 14px; min-width:120px; text-align:center; cursor:pointer; }
.rone-preset .rone-btn { padding:6px 9px; color:var(--dim); box-shadow:none; background:transparent; }
.rone-preset .rone-btn:hover { color:var(--text); }

/* ---- segment: one of N, a sliding pill (rone-ui.js RoneUI.seg places it) ---- */
.rone-seg { display:inline-flex; gap:2px; padding:3px; background:var(--card); border-radius:8px; position:relative; box-shadow:inset 0 1px 3px rgba(0,0,0,.6), 0 1px 0 rgba(255,255,255,.04); }
.rone-seg .pill { position:absolute; top:3px; left:0; height:calc(100% - 6px); width:0; border-radius:5px; pointer-events:none;
    background:linear-gradient(180deg,rgba(255,255,255,.18),rgba(255,255,255,0)), var(--neon);
    box-shadow:inset 0 1px 0 rgba(255,255,255,.3), 0 1px 2px rgba(0,0,0,.5); transition:transform .38s var(--pop), width .38s var(--pop); }
.rone-seg > :not(.pill) { font-family:var(--font-ui); font-size:10px; font-weight:700; letter-spacing:.1em; text-transform:uppercase; padding:6px 11px; color:var(--dim); cursor:pointer;
    border-radius:5px; position:relative; z-index:1; transition:color .2s; white-space:nowrap; background:none; border:0; }
.rone-seg > :not(.pill):hover { color:var(--text2); }
.rone-seg > .on, .rone-seg > .active, .rone-seg > [aria-checked="true"], .rone-seg > [aria-pressed="true"] { color:var(--ink); }
.rone-seg.squash .pill { animation:rone-squash .38s var(--pop); }
@keyframes rone-squash { 0% { transform:scaleY(.8) translateX(var(--px)); } 50% { transform:scaleY(1.08) translateX(var(--px)); } 100% { transform:scaleY(1) translateX(var(--px)); } }
.rone-seg.off .pill { background:var(--line2); box-shadow:none; }   /* the row is switched off: the pill goes grey */
.rone-seg.off > .on { color:var(--text2); }

/* ---- chip (a read-out) ---- */
.rone-chip { display:inline-flex; align-items:baseline; gap:8px; background:var(--card); border-radius:7px; padding:6px 10px; white-space:nowrap; box-shadow:inset 0 1px 3px rgba(0,0,0,.6), 0 1px 0 rgba(255,255,255,.04); }
.rone-chip .k { font-family:var(--font-ui); font-size:9px; font-weight:700; letter-spacing:.14em; color:var(--dim); text-transform:uppercase; }
.rone-chip .v { font-family:var(--font-display); font-size:13px; font-weight:700; color:var(--text); font-variant-numeric:tabular-nums; }
.rone-chip .v small { font-size:9px; color:var(--dim); margin-left:3px; }

/* ---- switch ---- */
.rone-switch { display:inline-flex; align-items:center; gap:8px; font-family:var(--font-ui); font-size:10px; font-weight:700; letter-spacing:.12em; color:var(--dim); text-transform:uppercase; cursor:pointer; transition:color .2s; user-select:none; }
.rone-switch i { width:34px; height:18px; border-radius:9px; background:var(--card); box-shadow:inset 0 1px 3px rgba(0,0,0,.7); position:relative; flex:0 0 auto; }
.rone-switch i::after { content:''; position:absolute; top:2px; left:2px; width:14px; height:14px; border-radius:50%; background:linear-gradient(180deg,#3A3F47,#24272C); box-shadow:0 1px 2px rgba(0,0,0,.6); transition:transform .32s var(--pop), background .2s; }
.rone-switch.on { color:var(--text); }
.rone-switch.on i::after { transform:translateX(16px); background:var(--neon); box-shadow:0 1px 2px rgba(0,0,0,.6), 0 0 8px rgba(var(--neon-rgb),.6); }
.rone-led { display:inline-block; width:7px; height:7px; border-radius:50%; background:var(--neon); box-shadow:0 0 6px rgba(var(--neon-rgb),.8); vertical-align:1px; }
.rone-led.off { background:var(--faint); box-shadow:none; }

/* ---- popover: presets, selects, header menus, pickers (rone-ui.js RoneUI.menu / RoneUI.select) ---- */
.rone-anchor { position:relative; display:inline-block; }
.rone-menu { position:absolute; top:calc(100% + 6px); left:0; min-width:200px; z-index:60; background:var(--drawer); border-radius:9px; padding:5px;
    box-shadow:inset 0 1px 0 rgba(255,255,255,.06), 0 0 0 1px rgba(0,0,0,.6), 0 18px 40px rgba(0,0,0,.6);
    opacity:0; transform:translateY(-6px) scale(.98); transform-origin:top left; pointer-events:none; transition:opacity .16s ease-out, transform .18s var(--pop); max-height:420px; overflow-y:auto; }
.rone-menu.right { left:auto; right:0; transform-origin:top right; }
.rone-menu.up { top:auto; bottom:calc(100% + 6px); transform:translateY(6px) scale(.98); transform-origin:bottom left; }
.rone-menu.open { opacity:1; transform:none; pointer-events:auto; }
.rone-menu.closing { transition:opacity .1s ease-in, transform .1s ease-in; }
.rone-menu .h { font-family:var(--font-ui); font-size:9px; font-weight:800; letter-spacing:.16em; text-transform:uppercase; color:var(--dim); padding:7px 10px 5px; }
.rone-menu .it { display:flex; align-items:center; gap:10px; padding:7px 10px; border-radius:5px; cursor:pointer; font-family:var(--font-ui); font-size:10.5px; font-weight:700; letter-spacing:.1em; text-transform:uppercase; color:var(--text2); transition:background .12s, color .12s; white-space:nowrap; }
.rone-menu .it:hover, .rone-menu .it.hi { background:var(--line2); color:var(--text); }
.rone-menu .it .d { width:6px; height:6px; border-radius:50%; background:transparent; flex:0 0 auto; transition:background .15s, box-shadow .15s; }
.rone-menu .it.on { color:var(--text); }
.rone-menu .it.on .d { background:var(--neon); box-shadow:0 0 6px rgba(var(--neon-rgb),.8); }
.rone-menu .it .m { margin-left:auto; padding-left:14px; font-family:var(--font-mono); font-size:10px; font-weight:500; letter-spacing:.02em; text-transform:none; color:var(--dim); }
.rone-menu .it .star { margin-left:6px; color:var(--faint); font-size:11px; }
.rone-menu .it .star.on { color:var(--neon); }
.rone-menu .it.disabled { color:var(--faint); cursor:default; background:none; }
.rone-menu .sep { height:1px; background:var(--line); margin:4px 6px; }
.rone-menu .ft { font-family:var(--font-ui); font-size:10px; font-weight:600; color:var(--dim); padding:7px 10px 6px; max-width:260px; white-space:normal; line-height:1.5; }
.rone-menu.wide { min-width:300px; }
.rone-menu::-webkit-scrollbar { width:8px; } .rone-menu::-webkit-scrollbar-thumb { background:var(--line2); border-radius:4px; }
/* a native <select> wrapped by RoneUI.select: the select stays in the DOM (hidden), the key shows its text */
.rone-select-native { position:absolute !important; width:1px; height:1px; opacity:0; pointer-events:none; overflow:hidden; }
.rone-btn.active{color:var(--ink);background:linear-gradient(180deg,rgba(255,255,255,.18),rgba(255,255,255,0)),var(--neon);box-shadow:inset 0 1px 0 rgba(255,255,255,.35),inset 0 -1px 0 rgba(0,0,0,.25),0 1px 0 rgba(0,0,0,.6),0 0 0 1px rgba(0,0,0,.35)}
.rone-btn.hot.active{background:linear-gradient(180deg,rgba(255,255,255,.18),rgba(255,255,255,0)),var(--hot);color:#fff}
</style>
)rawhtml") + std::string (R"rawhtml(<style>
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
    --neon-rgb:   255,138,61;
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
    background: #0E1014;   /* outer stage behind the floating window (UI kit) */
    padding: 10px;
    position: relative;
    transform-origin: top left;
}
#win {
    height: 100%;
    display: flex;
    flex-direction: column;
    background: linear-gradient(180deg, var(--panel) 0%, var(--ground) 100%);
    border: 1px solid #262A31;
    border-radius: 16px;
    overflow: hidden;
    box-shadow: 0 18px 50px rgba(0,0,0,0.5);
    position: relative;
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
/* Header brand = the About trigger: hover glows + slightly grows, click opens the back panel */
#p-logo {
    display: flex;
    align-items: center;
    gap: 10px;
    cursor: pointer;
    transform-origin: left center;
    transition: transform 0.18s ease;
}
#p-logo:hover  { transform: scale(1.06); }
#p-logo:active { transform: scale(0.99); }
#p-logo:hover #glyph { filter: drop-shadow(0 0 11px var(--neon)); }
#p-logo:hover h1 i { text-shadow: 0 0 18px var(--neon); }
#glyph {
    width: 40px; height: 21px;
    flex-shrink: 0;
    overflow: visible;
    filter: drop-shadow(0 0 6px var(--neon-glow));
    transition: filter 0.18s ease;
}
#glyph .r-stem {
    fill: var(--neon);
}
#glyph .r-line {
    fill: none;
    stroke: var(--neon);
    stroke-width: 4.2;
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
    transition: text-shadow 0.18s ease;
}
#logo-banner .subtitle {
    font-size: 9px;
    font-weight: 700;
    color: var(--dim);
    letter-spacing: 0.3em;
    margin-top: 2px;
}
.preset { margin-left: auto; position: relative; }
#preset-name { min-width: 190px; white-space: nowrap; overflow: hidden; }
#preset-menu { left: 50%; margin-left: -130px; min-width: 260px; max-height: 560px; transform-origin: top center; }
#preset-menu .p-lock input[type="checkbox"] { accent-color: var(--neon); cursor: pointer; width: 13px; height: 13px; }
#bypass { margin-left: 10px; }
</style>
)rawhtml")

    + R"rawhtml(<style>
/* ---- Space display ---- */
#viz-wrap {
    margin: 12px 16px 0;
    position: relative;
    height: 288px; min-height: 288px;
    background: var(--card);
    border: 1px solid var(--line);
    border-radius: 8px;
    overflow: hidden;
}
#viz { position: absolute; inset: 0; width: 100%; height: 100%; }
.chip { position: absolute; }
.chip > small { font-size: 9px; color: var(--dim); margin-left: -4px; }
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
/* knob look: rone-ui v2 .arck (pasted above) */

/* ---- Main knob row ---- */
#main-knobs {
    display: flex;
    align-items: flex-end;
    justify-content: center;
    gap: 42px;
    padding: 18px 16px 8px;
}
.vsep {
    width: 1px;
    align-self: stretch;
    margin: 22px 0 26px;
    background: linear-gradient(180deg, transparent, rgba(255,255,255,0.07), transparent);
}
#solowet-btn { margin-left: 10px; display: inline-flex; align-items: center; gap: 6px; }

/* ---- Modules ---- */
#modules {
    display: flex;
    gap: 12px;
    margin: 10px 16px 0;
}
.module {
    flex: 1;
    background: linear-gradient(180deg, #1B1E24, #16191E); /* UI-kit panel gradient */
    border: 1px solid var(--line2);
    border-radius: 12px;
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
.select-group { display: flex; flex-direction: column; gap: 4px; }
.side-note {
    font-size: 9px;
    font-weight: 700;
    letter-spacing: 0.1em;
    color: var(--dim);
    font-variant-numeric: tabular-nums;
}

</style>
)rawhtml"

    + R"rawhtml(<style>
/* ===== About — hardware back panel (RONE unified, fills the window) =====
   The unit IS the window: metal stretches, hardware groups stay in scale
   (--s) and anchor to the edges, like the real back of the device. */
#bp-about{
  --bp-glow:#FF8A3D;--bp-glow-rgb:255,138,61;
  --bp-etch:#9aa0a8;--bp-etch-dim:#70757d;--bp-etch-faint:#565b62;
  --bp-sans:'Archivo','Segoe UI',Arial,sans-serif;
  --bp-mono:'IBM Plex Mono',Consolas,ui-monospace,monospace;
  position:fixed;inset:0;z-index:1000;background:#0a0b0d;}
#bp-about.bp-hidden{display:none;}
.bp-close{position:absolute;top:10px;right:12px;z-index:30;background:rgba(10,11,13,.55);
  border:1px solid #3a3e45;border-radius:4px;color:#9aa0a8;font-family:var(--bp-sans);
  font-size:11px;font-weight:600;letter-spacing:.18em;padding:6px 11px;cursor:pointer;
  transition:color .2s,border-color .2s;}
.bp-close:hover{color:#e8eaed;border-color:#6a6f77;}
.bp-unit{position:absolute;inset:0;--s:1;}
@keyframes bpSwingIn{0%{transform:perspective(1700px) rotateY(-32deg) scale(.94);opacity:0;}
  100%{transform:none;opacity:1;}}
.bp-unit.bp-entering{animation:bpSwingIn .95s cubic-bezier(.18,.75,.22,1) both;}
.bp-chassis{position:absolute;inset:0;background:linear-gradient(180deg,#101114,#08090a 12%,#050607 90%);
  padding:calc(18px * var(--s));
  box-shadow:inset 0 1px 0 rgba(255,255,255,.07),inset 0 -2px 6px rgba(0,0,0,.9);}
.bp-panel{position:relative;width:100%;height:100%;border-radius:calc(10px * var(--s));overflow:hidden;
  background:radial-gradient(130% 85% at 50% -12%,rgba(255,255,255,.09),transparent 55%),
  linear-gradient(180deg,#2a2c31 0%,#212327 32%,#1b1d20 68%,#17181b 100%);
  box-shadow:inset 0 1px 0 rgba(255,255,255,.06),inset 0 0 40px rgba(0,0,0,.35);}
.bp-panel::before{content:'';position:absolute;inset:0;pointer-events:none;
  background:repeating-linear-gradient(0deg,rgba(255,255,255,.014) 0 1px,transparent 1px 3px);}
.bp-panel::after{content:'';position:absolute;inset:0;pointer-events:none;opacity:.05;
  background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='140' height='140'%3E%3Cfilter id='n'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='0.85' numOctaves='2'/%3E%3C/filter%3E%3Crect width='140' height='140' filter='url(%23n)'/%3E%3C/svg%3E");}
/* decorative groups must not steal hover from links/buttons */
.bp-g{position:absolute;transform:scale(var(--s));pointer-events:none;}
a.bp-g,.bp-g a,.bp-g button{pointer-events:auto;}
.bpg-site{z-index:6;}
.bpg-brand{left:calc(58px * var(--s));top:calc(44px * var(--s));transform-origin:top left;}
.bpg-site{right:calc(64px * var(--s));top:max(48px,calc(96px * var(--s)));transform-origin:top right;}
.bpg-left{left:calc(64px * var(--s));top:37%;transform-origin:left top;
  display:flex;gap:40px;align-items:flex-start;}
.bpg-fan{right:calc(78px * var(--s));top:35%;transform-origin:right top;}
.bpg-legal{left:calc(64px * var(--s));bottom:calc(198px * var(--s));transform-origin:bottom left;}
.bpg-out{left:calc(64px * var(--s));bottom:calc(34px * var(--s));transform-origin:bottom left;}
.bpg-midi{left:calc(346px * var(--s));bottom:calc(34px * var(--s));transform-origin:bottom left;}
.bpg-sn{right:calc(452px * var(--s));bottom:calc(96px * var(--s));transform-origin:bottom right;}
.bpg-stick{right:calc(248px * var(--s));bottom:calc(18px * var(--s));transform-origin:bottom right;}
.bpg-pwr{right:calc(60px * var(--s));bottom:calc(34px * var(--s));transform-origin:bottom right;}
.bp-screw{position:absolute;width:calc(15px * var(--s));height:calc(15px * var(--s));border-radius:50%;z-index:4;
  background:radial-gradient(circle at 35% 30%,#585c62,#2c2e32 55%,#141517);
  box-shadow:0 1px 2px rgba(0,0,0,.8),inset 0 1px 1px rgba(255,255,255,.25);}
.bp-screw::after{content:'';position:absolute;left:17%;right:17%;top:43%;height:2px;
  background:#0b0c0d;border-radius:1px;transform:rotate(var(--sr,20deg));
  box-shadow:0 1px 0 rgba(255,255,255,.12);}
.bp-s-tl{left:calc(20px * var(--s));top:calc(20px * var(--s));}
.bp-s-tr{right:calc(20px * var(--s));top:calc(20px * var(--s));}
.bp-s-bl{left:calc(20px * var(--s));bottom:calc(20px * var(--s));}
.bp-s-br{right:calc(20px * var(--s));bottom:calc(20px * var(--s));}
.bp-s-tm{left:50%;top:calc(18px * var(--s));}
.bp-s-bm{left:50%;bottom:calc(18px * var(--s));}
.bp-s-ml{left:calc(18px * var(--s));top:48%;}
.bp-s-mr{right:calc(18px * var(--s));top:48%;}
.bp-brandline{display:flex;align-items:center;gap:10px;margin:0 0 10px 6px;}
.bp-brandmark{border:1.5px solid var(--bp-etch-dim);color:var(--bp-etch-dim);border-radius:3px;
  font-family:var(--bp-sans);font-weight:600;font-size:11px;letter-spacing:.36em;padding:3px 3px 3px 8px;line-height:1;}
.bp-brandtag{font-family:var(--bp-sans);font-weight:500;font-size:15px;letter-spacing:.22em;color:var(--bp-etch-faint);}
.bp-model{font-family:var(--bp-sans);font-weight:500;font-size:92px;line-height:1.05;
  letter-spacing:-.015em;color:#a6abb2;white-space:nowrap;
  text-shadow:0 1px 0 rgba(255,255,255,.05),0 -1px 1px rgba(0,0,0,.45);}
.bp-sitelink{text-decoration:none;display:flex;flex-direction:column;align-items:flex-end;gap:8px;cursor:pointer;}
.bp-sl-eyebrow{font-family:var(--bp-sans);font-weight:600;font-size:15px;letter-spacing:.3em;
  color:var(--bp-etch-faint);transition:color .25s;}
.bp-sl-url{font-family:var(--bp-sans);font-weight:600;font-size:34px;letter-spacing:.1em;line-height:1;
  color:#b9bec6;white-space:nowrap;text-shadow:0 1px 0 rgba(255,255,255,.05),0 -1px 1px rgba(0,0,0,.45);
  transition:color .25s,text-shadow .25s;}
.bp-sl-bar{width:100%;height:3px;border-radius:2px;
  background:linear-gradient(90deg,transparent,rgba(var(--bp-glow-rgb),.7));
  box-shadow:0 0 9px rgba(var(--bp-glow-rgb),.4);transition:opacity .9s;}
.bp-sitelink:hover .bp-sl-url{color:#fff;text-shadow:0 0 16px rgba(var(--bp-glow-rgb),.55);}
.bp-sitelink:hover .bp-sl-eyebrow{color:var(--bp-etch-dim);}
.bp-unit.bp-off .bp-sl-bar{opacity:.12;}
.bp-vents{width:72px;display:flex;flex-direction:column;gap:8px;}
.bp-slot{position:relative;height:8px;border-radius:4px;background:linear-gradient(180deg,#050607,#101114);
  box-shadow:inset 0 1px 3px rgba(0,0,0,.95),inset 0 -1px 1px rgba(255,255,255,.05);}
.bp-slot::before{content:'';position:absolute;inset:1px 6px;border-radius:3px;filter:blur(1.2px);
  background:linear-gradient(90deg,transparent,rgba(var(--bp-glow-rgb),var(--vI,0)) 45%,transparent 85%);
  transition:opacity 1s;animation:bpVentPulse 5.2s ease-in-out infinite alternate;}
.bp-slot:nth-child(3){--vI:.5;}.bp-slot:nth-child(4){--vI:.85;}
.bp-slot:nth-child(5){--vI:.4;}.bp-slot:nth-child(6){--vI:.6;}
.bp-slot:nth-child(7){--vI:.25;}.bp-slot:nth-child(8){--vI:.45;}
.bp-slot:nth-child(9){--vI:.15;}.bp-slot:nth-child(10){--vI:.3;}
.bp-slot:nth-child(5)::after{content:'';position:absolute;left:16px;top:2px;width:6px;height:4px;
  border-radius:2px;background:radial-gradient(circle,#ffc27a,#ff8f2e);
  box-shadow:0 0 7px 1px rgba(255,150,50,.8);animation:bpLedBlink 7s linear infinite;transition:opacity .5s;}
@keyframes bpVentPulse{from{opacity:.65;}to{opacity:1;}}
@keyframes bpLedBlink{0%,64%,68%,100%{opacity:1;}66%{opacity:.25;}}
.bp-unit.bp-off .bp-slot::before{opacity:0;animation:none;}
.bp-unit.bp-off .bp-slot:nth-child(5)::after{opacity:0;animation:none;}
)rawhtml"

    + R"rawhtml(.bp-credits{display:flex;flex-direction:column;gap:13px;font-family:var(--bp-sans);text-align:left;}
.bp-cr-row{display:flex;align-items:center;gap:11px;font-size:23px;white-space:nowrap;color:var(--bp-etch);}
.bp-cr-row b{font-weight:600;color:#c3c8ce;}
.bp-cr-lead{color:var(--bp-etch-dim);font-weight:400;}
.bp-avatar{width:34px;height:34px;border-radius:5px;overflow:hidden;flex:0 0 auto;box-shadow:0 1px 3px rgba(0,0,0,.6);}
.bp-avatar svg{display:block;width:100%;height:100%;shape-rendering:crispEdges;}
.bp-cr-sub{font-size:19px;white-space:nowrap;color:var(--bp-etch-dim);}
.bp-cr-tech{font-size:16px;white-space:nowrap;color:var(--bp-etch-faint);letter-spacing:.05em;}
.bp-legal{display:flex;align-items:center;gap:14px;font-family:var(--bp-sans);
  font-size:17px;color:var(--bp-etch-dim);white-space:nowrap;}
.bp-fanwell{position:relative;width:212px;height:212px;border-radius:50%;
  background:radial-gradient(circle at 50% 42%,#0b0c0d 58%,#141518 72%,#1e2024 84%,#17181b 100%);
  box-shadow:inset 0 3px 10px rgba(0,0,0,.9),inset 0 -1px 2px rgba(255,255,255,.05),0 1px 0 rgba(255,255,255,.04);}
.bp-fanwell::before{content:'';position:absolute;inset:6px;border-radius:50%;z-index:2;pointer-events:none;
  background:radial-gradient(circle at 36% 26%,rgba(255,255,255,.07),transparent 52%);}
.bp-blades{position:absolute;inset:22px;border-radius:50%;overflow:hidden;
  background:repeating-conic-gradient(from 10deg,#22252a 0deg 8deg,#15171a 8deg 24deg,#08090a 24deg 51.4deg);
  filter:blur(.4px);animation:bpSpin 6.5s linear infinite;}
@keyframes bpSpin{to{transform:rotate(360deg);}}
.bp-unit.bp-off .bp-blades{animation-play-state:paused;}
.bp-hub{position:absolute;left:50%;top:50%;width:56px;height:56px;transform:translate(-50%,-50%);
  border-radius:50%;background:radial-gradient(circle at 40% 32%,#25272b,#101113 70%);
  box-shadow:0 2px 6px rgba(0,0,0,.8),inset 0 1px 1px rgba(255,255,255,.08);}
.bp-ring{position:absolute;left:50%;top:50%;transform:translate(-50%,-50%);border-radius:50%;
  border:5px solid #212428;box-shadow:0 1px 1px rgba(0,0,0,.6),inset 0 1px 1px rgba(255,255,255,.05);}
.bp-r1{width:196px;height:196px;}.bp-r2{width:138px;height:138px;}.bp-r3{width:84px;height:84px;}
.bp-spoke{position:absolute;left:50%;top:50%;width:196px;height:7px;border-radius:4px;background:#212428;
  box-shadow:0 1px 1px rgba(0,0,0,.55);}
.bp-s1{transform:translate(-50%,-50%) rotate(45deg);}
.bp-s2{transform:translate(-50%,-50%) rotate(-45deg);}
.bp-mod{position:relative;height:118px;border:1px solid #63686f;border-radius:12px;
  display:flex;align-items:center;justify-content:space-evenly;padding-top:12px;}
.bp-mlab{position:absolute;top:-8px;left:50%;transform:translateX(-50%);background:#1a1c1f;padding:0 9px;
  font-family:var(--bp-sans);font-weight:700;font-size:11.5px;letter-spacing:.22em;color:#9aa0a8;white-space:nowrap;}
.bp-port{display:flex;flex-direction:column;align-items:center;gap:8px;}
.bp-plab{font-family:var(--bp-sans);font-weight:600;font-size:8.5px;letter-spacing:.14em;color:#8b9096;}
.bp-mod-out{width:252px;}
.bp-mod-midi{width:264px;}
.bp-mod-pwr{width:174px;}
.bp-cable{position:absolute;left:0;top:0;width:100%;height:190px;pointer-events:none;z-index:2;}
.bp-jack{position:relative;width:38px;height:38px;border-radius:50%;
  background:radial-gradient(circle at 35% 28%,#f0f2f5,#b9bec6 45%,#7e838b 72%,#575b60);
  box-shadow:0 2px 3px rgba(0,0,0,.7),inset 0 -2px 4px rgba(0,0,0,.3);}
.bp-jack::after{content:'';position:absolute;inset:11px;border-radius:50%;
  background:radial-gradient(circle at 40% 32%,#26282c,#0a0b0c 72%);box-shadow:inset 0 2px 4px rgba(0,0,0,.95);}
.bp-jack.bp-plugged::before{content:'';position:absolute;inset:6px;border-radius:50%;z-index:3;
  background:radial-gradient(circle at 36% 30%,#43464c,#17181b 58%,#0b0c0d);
  box-shadow:0 2px 6px rgba(0,0,0,.85),inset 0 1px 1px rgba(255,255,255,.12);}
.bp-din{position:relative;width:44px;height:44px;border-radius:50%;
  background:radial-gradient(circle at 35% 28%,#dde0e4,#a6abb2 52%,#6b7076 78%);
  box-shadow:0 2px 3px rgba(0,0,0,.7),inset 0 -2px 4px rgba(0,0,0,.3);}
.bp-din::after{content:'';position:absolute;inset:5px;border-radius:50%;
  background:radial-gradient(circle at 42% 34%,#1b1c1f,#08090a 75%);box-shadow:inset 0 2px 4px rgba(0,0,0,.95);}
.bp-din .bp-pins{position:absolute;left:50%;top:50%;width:3.5px;height:3.5px;border-radius:50%;
  background:#b9bdc2;z-index:2;transform:translate(-50%,-60%);
  box-shadow:-11px 5px 0 #b9bdc2,11px 5px 0 #b9bdc2,-8px -6px 0 #b9bdc2,8px -6px 0 #b9bdc2;}
.bp-din.bp-plugged::before{content:'';position:absolute;inset:3px;border-radius:50%;z-index:3;
  background:radial-gradient(circle at 36% 30%,#35383d,#141519 60%,#0a0b0c);
  box-shadow:0 2px 6px rgba(0,0,0,.85),inset 0 0 0 3px #1e2024,inset 0 1px 1px rgba(255,255,255,.1);}
.bp-rocker{position:relative;width:52px;height:34px;border-radius:5px;background:#050606;cursor:pointer;
  border:0;box-shadow:inset 0 0 0 2px #17191b,0 1px 2px rgba(0,0,0,.7);padding:4px;display:flex;}
.bp-rk{flex:1;display:flex;align-items:center;justify-content:center;font-family:var(--bp-sans);
  font-weight:700;font-size:10px;color:#cfd3d8;background:#141518;}
.bp-rk-on{border-radius:2px 0 0 2px;box-shadow:inset 0 2px 4px rgba(0,0,0,.85);}
.bp-rk-off{border-radius:0 2px 2px 0;background:#1d1f23;box-shadow:inset 0 -1px 1px rgba(255,255,255,.07);}
.bp-unit.bp-off .bp-rk-on{background:#1d1f23;box-shadow:inset 0 -1px 1px rgba(255,255,255,.07);}
.bp-unit.bp-off .bp-rk-off{background:#141518;box-shadow:inset 0 2px 4px rgba(0,0,0,.85);}
.bp-pled{position:absolute;top:14px;right:12px;width:7px;height:7px;border-radius:50%;
  background:radial-gradient(circle,#8dffb8,#37e07f);box-shadow:0 0 8px 1px rgba(70,255,150,.7);transition:all .5s;}
.bp-unit.bp-off .bp-pled{background:#22262a;box-shadow:none;}
.bp-iec{position:relative;width:64px;height:46px;background:#0a0b0c;border-radius:6px;
  clip-path:polygon(10% 0,90% 0,100% 100%,0 100%);box-shadow:inset 0 1px 3px rgba(0,0,0,.9);}
.bp-iec::after{content:'';position:absolute;inset:7px 9px;border-radius:4px;
  background:radial-gradient(circle at 40% 30%,#26282c,#111214 70%);
  box-shadow:inset 0 2px 4px rgba(0,0,0,.9),inset 0 0 0 2px #1a1c1f;}
.bp-volts{font-family:var(--bp-sans);font-weight:600;font-size:7.5px;letter-spacing:.1em;color:#8b9096;}
.bp-stickers{width:186px;display:flex;flex-direction:column;align-items:center;gap:7px;font-family:var(--bp-sans);}
.bp-stick-y{width:118px;background:#f3efe4;border-radius:3px;overflow:hidden;transform:rotate(-1.4deg);
  box-shadow:0 2px 4px rgba(0,0,0,.55);}
.bp-sy-head{background:#f2c61d;color:#151310;font-weight:700;font-size:9px;letter-spacing:.1em;
  text-align:center;padding:2px 0 1px;}
.bp-sy-body{display:flex;align-items:center;gap:5px;padding:3px 6px 4px;}
.bp-sy-body svg{flex:0 0 auto;}
.bp-sy-txt{font-size:7.2px;line-height:1.35;color:#2a2620;font-weight:500;text-align:left;}
.bp-stick-w{width:186px;background:#f1f0eb;border-radius:2px;padding:4px 6px 5px;transform:rotate(.5deg);
  box-shadow:0 2px 4px rgba(0,0,0,.55);color:#1d1c1a;}
.bp-sw-head{display:flex;align-items:center;justify-content:center;gap:6px;border:1px solid #1d1c1a;
  padding:1.5px 4px;margin-bottom:3px;}
.bp-sw-head span{font-weight:700;font-size:9px;letter-spacing:.24em;}
.bp-sw-bold{font-weight:700;font-size:7.2px;text-align:center;letter-spacing:.03em;margin-bottom:2px;}
.bp-sw-fine{font-size:6.4px;line-height:1.45;color:#3c3a36;text-align:justify;}
.bp-verline{margin-top:6px;font-family:var(--bp-mono);font-weight:500;font-size:22px;letter-spacing:.05em;
  color:#8f959c;white-space:nowrap;font-variant-numeric:tabular-nums;}
.bp-stick-sn{width:132px;transform:rotate(-.9deg);
  background:linear-gradient(180deg,#dcdee0,#c2c5c8);border-radius:2px;padding:5px 8px 6px;
  box-shadow:0 2px 4px rgba(0,0,0,.55);color:#17181a;font-family:var(--bp-sans);}
.bp-sn-top{font-weight:700;font-size:6.8px;letter-spacing:.1em;color:#3a3c3f;margin-bottom:3px;white-space:nowrap;}
.bp-sn-row{display:flex;align-items:center;gap:6px;}
.bp-barcode{width:52px;height:15px;flex:0 0 auto;
  background:repeating-linear-gradient(90deg,#17181a 0 1.5px,transparent 1.5px 3px,#17181a 3px 5px,transparent 5px 6.5px,#17181a 6.5px 7.5px,transparent 7.5px 10.5px);}
.bp-sn-num{font-family:var(--bp-mono);font-weight:600;font-size:9.5px;white-space:nowrap;}
@media (prefers-reduced-motion:reduce){
  .bp-unit.bp-entering{animation:none;}
  .bp-blades{animation:none;}
  .bp-slot::before,.bp-slot:nth-child(5)::after{animation:none;}
}
</style>)rawhtml"

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
    position: absolute; left: 0; right: 0; top: 12px; height: 5px;
    background: #0B0D10; border: 1px solid var(--line);
    border-radius: 3px;
}
#char-track .fill {
    position: absolute; left: 0; top: 13px; height: 3px;
    background: var(--neon); border-radius: 2px;
    box-shadow: 0 0 8px var(--neon-glow);
}
#char-track .handle {
    position: absolute; top: 4px;
    width: 18px; height: 18px;
    border-radius: 50%;
    margin-left: -9px;
    background: #14161A;                       /* UI-kit thumb: dark core */
    border: 2px solid var(--neon);             /* + accent ring */
    box-shadow: 0 0 14px var(--neon-soft);
}
#char-track::after {                            /* tick marks under the rail */
    content: "";
    position: absolute; left: 2px; right: 2px; top: 22px; height: 3px;
    background-image: repeating-linear-gradient(90deg,
        rgba(255,255,255,0.10) 0 1px, transparent 1px 34px);
}
#char-label {
    display: flex;
    align-items: center;
    gap: 12px;
    font-size: 9px;
    font-weight: 800;
    letter-spacing: 0.24em;
    color: var(--dim);
    position: absolute;
    top: -8px; left: 50%;
    transform: translateX(-50%);
    white-space: nowrap;
}
#char-label::before, #char-label::after {
    content: "";
    width: 56px; height: 1px;
    background: rgba(255,255,255,0.07);
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
    font-size: 9px;
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
.adv-foot {
    display: flex;
    flex-direction: column;
    gap: 6px;
    align-items: center;
}
.custom-hint {
    font-size: 9px;
    font-weight: 800;
    letter-spacing: 0.16em;
    color: var(--dim);
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

/* -- Lock screen (RONE ALL ACCESS required) -- */
.rl-overlay{position:fixed;inset:0;z-index:99999;background:#14161A;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:12px;font-family:'Manrope','Segoe UI',sans-serif;transition:opacity .35s ease}
.rl-overlay.rl-hidden{opacity:0;pointer-events:none}
.rl-name{font-family:'Sora','Segoe UI',sans-serif;font-size:22px;font-weight:800;letter-spacing:.14em;color:#E8EAED}
.rl-name i{font-style:normal;color:#FF8A3D}
.rl-prompt{font-family:'Sora','Segoe UI',sans-serif;font-size:13px;font-weight:700;letter-spacing:.08em;color:#E8EAED;margin-top:6px}
.rl-hint,.rl-sub{font-size:10px;font-weight:700;letter-spacing:.14em;text-transform:uppercase;color:#7A7F88;text-align:center;max-width:80%;line-height:1.6}
.rl-btn{margin-top:8px;padding:11px 24px;background:#FF8A3D;border:1px solid #FF8A3D;border-radius:8px;color:#0F1114;font-family:'Manrope','Segoe UI',sans-serif;font-size:10px;font-weight:800;letter-spacing:.2em;text-transform:uppercase;cursor:pointer}
</style>
</head>
<body>
<div class="rl-overlay rl-hidden" id="rlOverlay" style="display:none">
  <div class="rl-name">RONE <i>AFTERSPACE</i></div>
  <span class="rl-prompt">A RONE AFTER SPACE licence or RONE ALL ACCESS is required</span>
  <span class="rl-hint">Sign in with your roneaudio.com account in the RONE Plugins Center</span>
  <button class="rl-btn" id="rlLaunch">OPEN RONE PLUGINS CENTER</button>
  <span class="rl-sub">No account yet? 7-day free trial at roneaudio.com</span>
</div>
<div id="app">
<div id="win">

  <!-- ================= HEADER ================= -->
  <header id="logo-banner">
    <div id="p-logo" title="About" role="button">
      <svg id="glyph" viewBox="0 0 100 52" aria-label="RONE" role="img">
          <path class="r-stem" d="M 0 0 L 4.9 0 L 4.9 16.4 L 0 19.2 Z"/>
          <path class="r-line" d="M 0 2.1 H 46.7 A 10.5 10.5 0 0 1 46.7 23.1 H 11.6 A 6.9 6.9 0 0 0 11.6 36.9 H 34 C 46 36.9 47 48.2 59 48.2 H 100"/>
      </svg>
      <h1>AFTER<i>SPACE</i></h1>
    </div>
    <span class="subtitle">RONE PLUGINS</span>
    <div class="preset rone-preset">
      <button id="p-prev" class="rone-btn arrow" title="Previous preset">&#9664;</button>
      <div id="preset-name" class="nm" data-tip="Browse presets.">INIT - CLEAN SPACE</div>
      <button id="p-next" class="rone-btn arrow" title="Next preset">&#9654;</button>
      <div id="preset-menu" class="rone-menu"></div>
    </div>
    <button id="solowet-btn" class="rone-btn" data-tip="Listen to the space alone.">
      <svg width="11" height="11" viewBox="0 0 12 12" fill="none">
        <path d="M1.5 8V6a4.5 4.5 0 019 0v2" stroke="currentColor" stroke-width="1.3" stroke-linecap="round"/>
        <rect x="0.8" y="7.2" width="2.6" height="3.6" rx="1.1" fill="currentColor"/>
        <rect x="8.6" y="7.2" width="2.6" height="3.6" rx="1.1" fill="currentColor"/>
      </svg>WET</button>
    <button id="bypass" class="rone-btn hot" data-tip="Bypasses AFTERSPACE completely.">BYPASS</button>
  </header>

  <!-- ================= SPACE DISPLAY ================= -->
  <div id="viz-wrap">
    <canvas id="viz"></canvas>
    <div class="chip rone-chip" id="chip-spatial"><span class="k">SPATIAL</span><b class="v" id="spatial-val">WIDE</b></div>
    <div class="chip rone-chip" id="chip-bpm"><span class="k">BPM</span><b class="v" id="bpm-val">120</b></div>
    <div class="chip rone-chip" id="chip-engine"><span class="k">ENGINE</span><b class="v" id="engine-val">ACTIVE</b></div>
    <div class="chip rone-chip" id="chip-duck"><span class="k">DUCK</span><b class="v" id="duckgr-val">0.0</b><small>dB</small></div>
  </div>

  <!-- ================= MAIN KNOBS ================= -->
  <div id="main-knobs">
    <div id="knob-size"></div>
    <div class="vsep"></div>
    <div id="knob-decay"></div>
    <div class="vsep"></div>
    <div id="knob-bloom"></div>
    <div class="vsep"></div>
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
            <label class="rone-label">Mode</label>
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
            <label class="rone-label">Time</label>
            <select id="sel-echotime" data-tip="Blends tempo-synced echoes into the space."></select>
          </div>
          <span class="side-note" id="echo-ms"></span>
        </div>
      </div>
    </div>
  </div>

  <!-- ================= BOTTOM ROW ================= -->
  <div id="bottom-row">
    <button class="rone-btn" id="freeze-btn" data-tip="Holds the current reverb tail.">&#10052;&nbsp;&nbsp;FREEZE</button>
    <div id="char-wrap" data-tip="Moves the engine from tight and clean to wide and atmospheric.">
      <span class="char-end" id="char-clean">CLEAN</span>
      <div id="char-track">
        <span id="char-label">CHARACTER</span>
        <div class="rail"></div><div class="fill" id="char-fill"></div><div class="handle" id="char-handle"></div>
      </div>
      <span class="char-end" id="char-dream">DREAM</span>
    </div>
    <button class="rone-btn" id="adv-btn">ADVANCED</button>

    <!-- Advanced drawer floats above this row -->
    <div id="adv-drawer">
      <div class="adv-sec"><span class="sec-title">Space</span>
        <div class="adv-grid" id="adv-space"></div>
        <div class="adv-foot">
          <button class="rone-btn" id="ag-btn" data-tip="Keeps the wet level steady when Size or Decay changes.">AUTO GAIN</button>
        </div></div>
      <div class="adv-sec"><span class="sec-title">Tone</span>
        <div class="adv-grid" id="adv-tone"></div></div>
      <div class="adv-sec"><span class="sec-title">Motion &amp; Stereo</span>
        <div class="adv-grid" id="adv-mod"></div>
        <div class="adv-foot">
          <button class="rone-btn" id="ml-btn" data-tip="Keeps the wet signal mono below 150Hz for a clean low end.">MONO LOW</button>
        </div></div>
      <div class="adv-sec"><span class="sec-title">Duck</span>
        <div class="adv-grid" id="adv-duck"></div>
        <span class="custom-hint" id="duck-hint">ACTIVE IN CUSTOM MODE</span></div>
      <div class="adv-sec"><span class="sec-title">Echo</span>
        <div class="adv-grid" id="adv-echo"></div>
        <div class="adv-foot">
          <div class="rone-seg" id="seg-routing" data-tip="Where the echo lives: before, inside or after the space.">
            <span data-i="0">BEFORE</span><span data-i="1">INSIDE</span><span data-i="2">AFTER</span>
          </div>
          <button class="rone-btn" id="pp-btn">PING PONG</button>
        </div></div>
    </div>
  </div>

</div><!-- /win -->
  <div id="tooltip"></div>
</div>
<div id="resize-handle" title="Resize"></div>
)rawhtml"

    + R"rawhtml(
<!-- About — hardware back panel -->
<div id="bp-about" class="bp-hidden">
  <button id="bp-close-btn" class="bp-close" title="Close">&#10005;&nbsp; CLOSE</button>
  <div class="bp-unit">
<div class="bp-chassis">
  <div class="bp-panel">
    <span class="bp-screw bp-s-tl" style="--sr:24deg"></span>
    <span class="bp-screw bp-s-tr" style="--sr:-52deg"></span>
    <span class="bp-screw bp-s-bl" style="--sr:80deg"></span>
    <span class="bp-screw bp-s-br" style="--sr:8deg"></span>
    <span class="bp-screw bp-s-tm" style="--sr:-30deg"></span>
    <span class="bp-screw bp-s-bm" style="--sr:60deg"></span>
    <span class="bp-screw bp-s-ml" style="--sr:-75deg"></span>
    <span class="bp-screw bp-s-mr" style="--sr:40deg"></span>

    <div class="bp-g bpg-brand">
      <div class="bp-brandline">
        <span class="bp-brandmark">RONE</span>
        <span class="bp-brandtag">AUDIO INSTRUMENT SERIES</span>
      </div>
      <h1 class="bp-model">AFTER SPACE</h1>
    </div>

    <a class="bp-g bpg-site bp-sitelink" id="bp-site" href="#">
      <span class="bp-sl-eyebrow">PRESETS &middot; UPDATES &middot; SUPPORT</span>
      <span class="bp-sl-url">RONEAUDIO.COM</span>
      <span class="bp-sl-bar" aria-hidden="true"></span>
    </a>

    <div class="bp-g bpg-left">
      <div class="bp-vents" aria-hidden="true">
        <span class="bp-slot"></span><span class="bp-slot"></span><span class="bp-slot"></span>
        <span class="bp-slot"></span><span class="bp-slot"></span><span class="bp-slot"></span>
        <span class="bp-slot"></span><span class="bp-slot"></span><span class="bp-slot"></span>
        <span class="bp-slot"></span><span class="bp-slot"></span><span class="bp-slot"></span>
      </div>
      <div class="bp-credits">
        <div class="bp-cr-row">
          <span class="bp-cr-lead">Code &amp; DSP by</span>
          <span class="bp-avatar" aria-hidden="true">
            <svg viewBox="0 0 11 11" xmlns="http://www.w3.org/2000/svg">
              <rect width="11" height="11" fill="#23262b"/>
              <rect x="3" y="2" width="5" height="1" fill="#2e2013"/>
              <rect x="3" y="3" width="5" height="4" fill="#e6b78d"/>
              <rect x="4" y="4" width="1" height="1" fill="#1c1c1c"/>
              <rect x="7" y="4" width="1" height="1" fill="#1c1c1c"/>
              <rect x="4" y="6" width="3" height="1" fill="#c98f66"/>
              <rect x="3" y="8" width="5" height="3" fill="#2b2e34"/>
              <rect x="2" y="1" width="7" height="1" fill="#101114"/>
              <rect x="1" y="3" width="1" height="3" fill="#FF8A3D"/>
              <rect x="9" y="3" width="1" height="3" fill="#FF8A3D"/>
            </svg>
          </span>
          <b>Liran "RONE" Kalifa</b>
        </div>
        <div class="bp-cr-sub">Panel &amp; interface design by RONE Studio</div>
        <div class="bp-cr-tech">BUILT ON JUCE 8 &middot; WEBVIEW UI ENGINE</div>
      </div>
    </div>

    <div class="bp-g bpg-legal bp-legal">
      <span>Copyright &copy; 2024-2026 RONE PLUGINS. All rights reserved.</span>
    </div>

    <div class="bp-g bpg-fan" aria-hidden="true">
      <div class="bp-fanwell">
        <div class="bp-blades"></div>
        <div class="bp-ring bp-r3"></div>
        <div class="bp-spoke bp-s1"></div>
        <div class="bp-spoke bp-s2"></div>
        <div class="bp-ring bp-r2"></div>
        <div class="bp-ring bp-r1"></div>
        <div class="bp-hub"></div>
      </div>
    </div>

    <div class="bp-g bpg-out">
      <div class="bp-mod bp-mod-out">
        <span class="bp-mlab">OUTPUT</span>
        <div class="bp-port"><span class="bp-plab">LEFT</span><span class="bp-jack bp-plugged"></span></div>
        <div class="bp-port"><span class="bp-plab">RIGHT</span><span class="bp-jack bp-plugged"></span></div>
        <div class="bp-port"><span class="bp-plab">PHONES</span><span class="bp-jack"></span></div>
      </div>
      <svg class="bp-cable" viewBox="0 0 252 190" xmlns="http://www.w3.org/2000/svg" aria-hidden="true">
        <g fill="none" stroke-linecap="round">
          <path d="M56 62 C56 100 42 130 34 190" stroke="#08090a" stroke-width="11"/>
          <path d="M56 62 C56 100 42 130 34 190" stroke="#1e2024" stroke-width="7"/>
          <path d="M126 62 C126 104 136 134 141 190" stroke="#08090a" stroke-width="11"/>
          <path d="M126 62 C126 104 136 134 141 190" stroke="#1e2024" stroke-width="7"/>
        </g>
      </svg>
    </div>

    <div class="bp-g bpg-midi">
      <div class="bp-mod bp-mod-midi">
        <span class="bp-mlab">MIDI</span>
        <div class="bp-port"><span class="bp-plab">IN</span><span class="bp-din bp-plugged"><span class="bp-pins"></span></span></div>
        <div class="bp-port"><span class="bp-plab">OUT</span><span class="bp-din"><span class="bp-pins"></span></span></div>
        <div class="bp-port"><span class="bp-plab">THRU</span><span class="bp-din"><span class="bp-pins"></span></span></div>
      </div>
      <svg class="bp-cable" viewBox="0 0 264 190" xmlns="http://www.w3.org/2000/svg" aria-hidden="true">
        <g fill="none" stroke-linecap="round">
          <path d="M54 64 C54 100 36 130 28 190" stroke="#08090a" stroke-width="12"/>
          <path d="M54 64 C54 100 36 130 28 190" stroke="#17181c" stroke-width="8"/>
        </g>
      </svg>
    </div>

    <div class="bp-g bpg-sn bp-stick-sn">
      <div class="bp-sn-top">RONE PLUGINS &middot; DESIGNED &amp; CODED IN ISRAEL</div>
      <div class="bp-sn-row">
        <span class="bp-barcode" aria-hidden="true"></span>
        <span class="bp-sn-num">S/N RN-ASP-260090</span>
      </div>
    </div>

    <div class="bp-g bpg-stick bp-stickers">
      <div class="bp-stick-y">
        <div class="bp-sy-head">&#9888; CAUTION</div>
        <div class="bp-sy-body">
          <svg width="16" height="16" viewBox="0 0 16 16" xmlns="http://www.w3.org/2000/svg" aria-hidden="true">
            <circle cx="8" cy="8" r="7.3" fill="none" stroke="#2a2620" stroke-width="1.1"/>
            <path d="M4 9 a4 4 0 0 1 8 0" fill="none" stroke="#2a2620" stroke-width="1.3"/>
            <rect x="3" y="8.4" width="2.1" height="3.4" rx="1" fill="#2a2620"/>
            <rect x="10.9" y="8.4" width="2.1" height="3.4" rx="1" fill="#2a2620"/>
          </svg>
          <span class="bp-sy-txt">Loud sound hazard.<br>Ear protection must be worn.</span>
        </div>
      </div>
      <div class="bp-stick-w">
        <div class="bp-sw-head">
          <svg width="9" height="9" viewBox="0 0 10 10" aria-hidden="true"><path d="M5 .5 9.5 9H.5Z" fill="none" stroke="#1d1c1a" stroke-width=".9"/><path d="M5.4 3 4 5.6h1L4.6 7.5 6.4 4.8h-1.1L6 3Z" fill="#1d1c1a"/></svg>
          <span>CAUTION</span>
          <svg width="9" height="9" viewBox="0 0 10 10" aria-hidden="true"><path d="M5 .5 9.5 9H.5Z" fill="none" stroke="#1d1c1a" stroke-width=".9"/><path d="M5.4 3 4 5.6h1L4.6 7.5 6.4 4.8h-1.1L6 3Z" fill="#1d1c1a"/></svg>
        </div>
        <div class="bp-sw-bold">RISK OF ELECTRIC SHOCK &mdash; DO NOT OPEN</div>
        <div class="bp-sw-fine">CAUTION: TO REDUCE THE RISK OF ELECTRIC SHOCK, DO NOT REMOVE COVER OR BACK. CONTAINS NO USER-SERVICEABLE PRESETS INSIDE. REFER SERVICING TO QUALIFIED PERSONNEL.</div>
      </div>
      <div class="bp-verline">1.0.0 / VST3 &middot; AU / WIN&middot;MAC</div>
    </div>

    <div class="bp-g bpg-pwr">
      <div class="bp-mod bp-mod-pwr">
        <span class="bp-mlab">POWER</span>
        <span class="bp-pled" aria-hidden="true"></span>
        <div class="bp-port">
          <span class="bp-plab">ON/OFF</span>
          <button class="bp-rocker" id="bp-rocker" aria-pressed="true" aria-label="Power" title="Power">
            <span class="bp-rk bp-rk-on">I</span><span class="bp-rk bp-rk-off">O</span>
          </button>
        </div>
        <div class="bp-port">
          <span class="bp-volts">110-240V &middot; 50/60Hz ~</span>
          <span class="bp-iec"></span>
        </div>
      </div>
      <svg class="bp-cable" viewBox="0 0 174 190" xmlns="http://www.w3.org/2000/svg" aria-hidden="true">
        <g fill="none" stroke-linecap="round">
          <path d="M130 72 C130 108 142 138 148 190" stroke="#050607" stroke-width="15"/>
          <path d="M130 72 C130 108 142 138 148 190" stroke="#141519" stroke-width="10"/>
        </g>
      </svg>
    </div>
  </div>
</div>
  </div>
</div>
)rawhtml"

    + R"rawhtml(<script>/* rone-ui v2 */
// RONE UI kit v2 (26.10.2026) - the behaviour behind rone-ui.css. Paste after the CSS.
//   RoneUI.knob(el, {size:'m', value:.5, def:.5, fmt:v=>..., label:'Mix', hero:false, onChange:v=>...})
//       -> {set(v), get(), setText(html), lock(bool), el}      drag (shift = fine), wheel 2 %/notch, dbl-click = default, arrows
//   RoneUI.seg(el, {onClass:'on'})     -> adds the sliding pill; the plugin keeps toggling its own on-class, the pill follows
//   RoneUI.switch(el)                  -> toggles .on on click (optional helper)
//   RoneUI.menu(trigger, {items, header, footer, wide, side, onSelect}) -> {open(), close(), set(items), el}
//   RoneUI.select(selectEl, {wide})    -> wraps a native <select> in a key + popover; select.value / 'change' keep working
(function (root) {
  'use strict';
  const NS = 'http://www.w3.org/2000/svg';
  const clamp = (v, a, b) => Math.min(b, Math.max(a, v));
  const SIZES = { s: { S: 58, st: 3.5 }, m: { S: 104, st: 4.5 }, l: { S: 124, st: 5 }, xl: { S: 200, st: 7 } };

  function arcPath(cx, cy, r, a0, a1) {
    const p = a => { const t = (a - 90) * Math.PI / 180; return [cx + r * Math.cos(t), cy + r * Math.sin(t)]; };
    const [x0, y0] = p(a0), [x1, y1] = p(a1);
    return `M ${x0.toFixed(2)} ${y0.toFixed(2)} A ${r} ${r} 0 ${a1 - a0 > 180 ? 1 : 0} 1 ${x1.toFixed(2)} ${y1.toFixed(2)}`;
  }
  function el(tag, cls, html) { const e = document.createElement(tag); if (cls) e.className = cls; if (html != null) e.innerHTML = html; return e; }

  // ---------------------------------------------------------------- knob (.arck)
  function knob(target, opt) {
    const o = Object.assign({ size: 'm', value: 0, def: null, fmt: null, label: '', hero: false, onChange: null, locked: false }, opt || {});
    const { S, st } = SIZES[o.size] || SIZES.m, c = S / 2, r = c - st / 2 - 1;
    const k = target; k.classList.add('arck', 'size-' + o.size); if (o.hero) k.classList.add('hero'); if (!k.hasAttribute('tabindex')) k.tabIndex = 0;
    const svg = document.createElementNS(NS, 'svg'); svg.setAttribute('viewBox', `0 0 ${S} ${S}`);
    const track = document.createElementNS(NS, 'path'), lit = document.createElementNS(NS, 'path');
    track.setAttribute('class', 'track'); lit.setAttribute('class', 'lit'); track.setAttribute('d', arcPath(c, c, r, -135, 135));
    svg.append(track, lit);
    const body = el('div', 'kbody'), dot = el('div', 'kdot'), cval = el('div', 'cval');
    k.replaceChildren(svg, body, dot, cval);
    let v = clamp(o.value, 0, 1), text = null, lingerT = null;
    const def = o.def == null ? v : o.def;
    function draw() {
      const a = -135 + 270 * v;
      lit.setAttribute('d', v > 0.003 ? arcPath(c, c, r, -135, a) : '');
      dot.style.transform = `rotate(${a.toFixed(1)}deg)`;
      const t = text != null ? text : (o.fmt ? o.fmt(v) : Math.round(v * 100) + '%');
      cval.innerHTML = o.hero ? `<span>${t}</span><span class="sub">${o.label}</span>` : t;
    }
    function touch() { k.classList.add('touch'); clearTimeout(lingerT); lingerT = setTimeout(() => k.classList.remove('touch'), 500); }
    function user(nv) { nv = clamp(nv, 0, 1); if (nv === v) return; v = nv; text = null; draw(); if (o.onChange) o.onChange(v); }
    let y0 = 0, v0 = 0;
    k.addEventListener('pointerdown', e => { if (o.locked || e.button !== 0) return; k.setPointerCapture(e.pointerId); y0 = e.clientY; v0 = v; k.classList.add('touch'); clearTimeout(lingerT); e.preventDefault(); });
    k.addEventListener('pointermove', e => { if (!k.hasPointerCapture(e.pointerId)) return; user(v0 + (y0 - e.clientY) / (e.shiftKey ? 800 : 200)); });
    const up = () => { lingerT = setTimeout(() => k.classList.remove('touch'), 500); };
    k.addEventListener('pointerup', up); k.addEventListener('pointercancel', up);
    k.addEventListener('wheel', e => { if (o.locked) return; e.preventDefault(); user(v + (e.deltaY < 0 ? 1 : -1) * (e.shiftKey ? .005 : .02)); touch(); }, { passive: false });
    k.addEventListener('dblclick', () => { if (!o.locked) user(def); });
    k.addEventListener('keydown', e => { if (o.locked) return; const d = (e.key === 'ArrowUp' || e.key === 'ArrowRight') ? .02 : (e.key === 'ArrowDown' || e.key === 'ArrowLeft') ? -.02 : 0; if (!d) return; user(v + d); touch(); e.preventDefault(); });
    draw();
    return { set(nv) { v = clamp(nv, 0, 1); text = null; draw(); }, get: () => v, setText(t) { text = t; draw(); },
             lock(b) { o.locked = !!b; k.classList.toggle('locked', !!b); }, el: k };
  }

  // ---------------------------------------------------------------- segment pill
  function seg(s, opt) {
    const o = Object.assign({ onClass: 'on' }, opt || {});
    s.classList.add('rone-seg');
    let pill = s.querySelector(':scope > .pill');
    if (!pill) { pill = el('i', 'pill'); s.appendChild(pill); }   // LAST child: index-based plugin code (children[i]) keeps its indices
    const isOn = b => b.classList.contains(o.onClass) || b.getAttribute('aria-checked') === 'true' || b.getAttribute('aria-pressed') === 'true';
    function current() { return Array.from(s.children).find(b => b !== pill && isOn(b)); }
    function place(anim) {
      const b = current();
      if (!b) { pill.style.width = '0px'; return; }
      const x = b.offsetLeft;
      pill.style.setProperty('--px', x + 'px'); pill.style.transform = `translateX(${x}px)`; pill.style.width = b.offsetWidth + 'px';
      if (anim) { s.classList.remove('squash'); void pill.offsetWidth; s.classList.add('squash'); }
    }
    pill.style.transition = 'none'; place(false); void pill.offsetWidth; pill.style.transition = '';
    // the segment's own class (squash) and the pill must not re-trigger the placement, or it loops forever
    new MutationObserver(ms => { if (ms.some(m => m.target !== s && m.target !== pill)) place(true); })
      .observe(s, { attributes: true, subtree: true, attributeFilter: ['class', 'aria-checked', 'aria-pressed'] });
    if (root.ResizeObserver) new ResizeObserver(() => { pill.style.transition = 'none'; place(false); void pill.offsetWidth; pill.style.transition = ''; }).observe(s);
    if (document.fonts && document.fonts.ready) document.fonts.ready.then(() => place(false));
    return { place };
  }
  function switchEl(s) { s.classList.add('rone-switch'); if (!s.querySelector('i')) s.insertBefore(el('i'), s.firstChild); s.addEventListener('click', () => s.classList.toggle('on')); return s; }

  // ---------------------------------------------------------------- popover
  let openMenu = null;
  function closeAll() { if (openMenu) openMenu.close(); }
  document.addEventListener('click', closeAll);
  document.addEventListener('keydown', e => { if (e.key === 'Escape') closeAll(); });
  function menu(trigger, opt) {
    const o = Object.assign({ items: [], header: null, footer: null, wide: false, side: 'left', up: false, onSelect: null }, opt || {});
    let anchor = trigger.parentElement;
    if (!anchor || !anchor.classList.contains('rone-anchor')) { anchor = el('div', 'rone-anchor'); trigger.parentNode.insertBefore(anchor, trigger); anchor.appendChild(trigger); }
    const m = el('div', 'rone-menu' + (o.wide ? ' wide' : '') + (o.side === 'right' ? ' right' : '') + (o.up ? ' up' : ''));
    anchor.appendChild(m);
    const api = {
      el: m, trigger,
      set(items) {
        m.replaceChildren();
        if (o.header) m.appendChild(el('div', 'h', o.header));
        items.forEach(it => {
          if (it === '-') { m.appendChild(el('div', 'sep')); return; }
          if (it.html) { const w = el('div'); w.innerHTML = it.html; m.appendChild(w); return; }
          const r = el('div', 'it' + (it.on ? ' on' : '') + (it.disabled ? ' disabled' : ''));
          r.innerHTML = `<span class="d"></span><span class="t">${it.t}</span>${it.m != null ? `<span class="m">${it.m}</span>` : ''}${it.star != null ? `<span class="star${it.star ? ' on' : ''}">★</span>` : ''}`;
          r.addEventListener('click', e => {
            e.stopPropagation(); if (it.disabled) return;
            if (!it.action) m.querySelectorAll('.it').forEach(x => x.classList.toggle('on', x === r));
            api.close(); if (it.onSelect) it.onSelect(it); if (o.onSelect) o.onSelect(it);
          });
          m.appendChild(r);
        });
        if (o.footer) m.appendChild(el('div', 'ft', o.footer));
      },
      open() { if (openMenu && openMenu !== api) openMenu.close(); m.classList.remove('closing'); m.classList.add('open'); trigger.classList.add('open'); openMenu = api; },
      close() { if (!m.classList.contains('open')) return; m.classList.add('closing'); m.classList.remove('open'); trigger.classList.remove('open'); if (openMenu === api) openMenu = null; },
      toggle() { m.classList.contains('open') ? api.close() : api.open(); }
    };
    api.set(o.items);
    trigger.addEventListener('click', e => { e.stopPropagation(); api.toggle(); });
    m.addEventListener('click', e => e.stopPropagation());
    return api;
  }

  // ---------------------------------------------------------------- native <select> -> key + popover
  function select(sel, opt) {
    const o = Object.assign({ wide: false, side: 'left', up: false, className: '' }, opt || {});
    if (sel._rone) return sel._rone;
    const btn = el('button', 'rone-btn dd ' + o.className); btn.type = 'button';
    sel.parentNode.insertBefore(btn, sel); sel.classList.add('rone-select-native'); sel.tabIndex = -1;
    const label = () => { const op = sel.options[sel.selectedIndex]; btn.textContent = op ? op.textContent : ''; };
    const items = () => Array.from(sel.options).map(op => ({ t: op.textContent, on: op.selected, disabled: op.disabled, value: op.value,
      onSelect() { if (sel.value === op.value) return; sel.value = op.value; label(); sel.dispatchEvent(new Event('input', { bubbles: true })); sel.dispatchEvent(new Event('change', { bubbles: true })); } }));
    const m = menu(btn, { wide: o.wide, side: o.side, up: o.up, items: items() });
    btn.addEventListener('click', () => m.set(items()));          // rebuild from the live options every time it opens
    sel.addEventListener('change', label);
    // code that assigns sel.value / sel.selectedIndex (host -> UI) must move the key's text too
    ['value', 'selectedIndex'].forEach(p => { const d = Object.getOwnPropertyDescriptor(HTMLSelectElement.prototype, p);
      if (d && d.set) Object.defineProperty(sel, p, { get() { return d.get.call(sel); }, set(x) { d.set.call(sel, x); label(); }, configurable: true }); });
    new MutationObserver(label).observe(sel, { childList: true, subtree: true, attributes: true });
    if (sel.disabled) btn.disabled = true;
    label();
    sel._rone = { button: btn, menu: m, refresh: label };
    return sel._rone;
  }

  root.RoneUI = { knob, seg, switch: switchEl, menu, select, arcPath, closeAll };
})(window);
</script>
)rawhtml"

    + R"rawhtml(<script>
"use strict";
// ============================================================================
// Bridge
// ============================================================================
function rlSetLicensed(ok){var o=document.getElementById('rlOverlay');if(!o)return;if(ok){o.classList.add('rl-hidden');setTimeout(function(){if(o.classList.contains('rl-hidden'))o.style.display='none';},400);}else{o.style.display='';void o.offsetWidth;o.classList.remove('rl-hidden');}}
function emit(n,v){ if(window.__JUCE__ && window.__JUCE__.backend) window.__JUCE__.backend.emitEvent(n,v||{}); }
function setParam(id,v){ emit("setParameter",{name:id,value:v}); }
// One-shot write (a click, a menu pick, a preset, a reset): hosts in touch/latch/write only
// record a change that arrives inside a gesture, so even a single value is wrapped (as in
// RONE Flanger). Knob drags keep their own gesture from mousedown to mouseup.
function setParamOnce(id,v){ emit("beginGesture",{name:id}); setParam(id,v); emit("endGesture",{name:id}); }

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
// rone-ui v2 geometry: centre c = S/2, radius r = c - stroke/2 - 1, arc -135..135
var SIZES = {
  l: { box:124, st:5 },
  m: { box:104, st:4.5 },
  s: { box:58,  st:3.5 }
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
  var c = S.box/2, r = c - S.st/2 - 1;
  group.innerHTML =
    '<div class="arck size-'+(cfg.size||"m")+'">'+
      '<svg viewBox="0 0 '+S.box+' '+S.box+'">'+
        '<path class="track" d="'+RoneUI.arcPath(c,c,r,-135,135)+'"/>'+
        '<path class="lit" d=""/>'+
      '</svg>'+
      '<div class="kbody"></div><div class="kdot"></div>'+
      '<div class="cval"></div>'+
    '</div>'+
    '<label class="rone-label">'+cfg.label+'</label>';
  if (cfg.tip) group.setAttribute("data-tip", cfg.tip);
  holder.appendChild(group);

  var el = group.querySelector(".arck");
  var K = {
    cfg:cfg, c:c, r:r, el:el,
    litEl: el.querySelector(".lit"),
    dotEl: el.querySelector(".kdot"),
    valEl: el.querySelector(".cval"),
    norm: clamp01(toNorm(cfg,P[cfg.id]))
  };
  knobs[cfg.id]=K;

  el.addEventListener("mousedown", function(e){
    dragCtx = { knob:K, startY:e.clientY, startNorm:K.norm };
    emit("beginGesture",{name:cfg.id});
    clearTimeout(K.lingerT); el.classList.add("touch");   // the lit arc glows only while touched
    e.preventDefault();
  });
  el.addEventListener("dblclick", function(){
    emit("beginGesture",{name:cfg.id});
    setKnob(K, clamp01(toNorm(cfg, DEFAULTS[cfg.id])), true);
    emit("endGesture",{name:cfg.id});
  });
  el.addEventListener("wheel", function(e){
    var step = e.shiftKey?0.005:0.03;
    emit("beginGesture",{name:cfg.id});
    setKnob(K, clamp01(K.norm - Math.sign(e.deltaY)*step), true);
    emit("endGesture",{name:cfg.id});
    touchKnob(K);
    e.preventDefault();
  },{passive:false});
  // Right-click = the host's parameter menu (FL: automation/link)
  el.addEventListener("contextmenu", function(e){
    e.preventDefault(); e.stopPropagation();
    emit("paramContextMenu",{name:cfg.id});
  });

  renderKnob(K);
  return K;
}
// Glow while touched: on for a drag, 500 ms after a wheel notch (rone-ui.js knob() pattern)
function touchKnob(K){
  K.el.classList.add("touch"); clearTimeout(K.lingerT);
  K.lingerT=setTimeout(function(){ K.el.classList.remove("touch"); },500);
}
function setKnob(K,norm,send){
  K.norm=clamp01(norm);
  var v=fromNorm(K.cfg,K.norm);
  P[K.cfg.id]=v;
  if (send) setParam(K.cfg.id,v);
  renderKnob(K);
}
function renderKnob(K){
  // Write only what actually changed.
  //
  // This runs for every knob on every state push from the host, ~30 times a
  // second, and at idle NONE of these values differ from the last tick: a
  // parameter that has not moved formats to the same string every time. Writing
  // them anyway invalidated style and layout for the whole page 30 times a
  // second, and the innerHTML line additionally ran the HTML parser and built
  // two fresh nodes per knob per tick - which is where this page's DOM node
  // count came from. Comparing first costs a string compare and removes all of
  // it while a knob is still; a moving knob writes exactly as before.
  var d = K.norm>0.003 ? RoneUI.arcPath(K.c,K.c,K.r,-135,-135+K.norm*270) : "";
  if (d !== K._d) { K.litEl.setAttribute("d", d); K._d = d; }

  var rot = "rotate("+(-135+K.norm*270)+"deg)";
  if (rot !== K._rot) { K.dotEl.style.transform = rot; K._rot = rot; }

  var val = K.cfg.fmt(P[K.cfg.id]);
  if (val !== K._val) { K.valEl.innerHTML = val; K._val = val; }
}

document.addEventListener("mousemove", function(e){
  if(!dragCtx) return;
  var range = e.shiftKey ? 1400 : 220;
  setKnob(dragCtx.knob, dragCtx.startNorm + (dragCtx.startY-e.clientY)/range, true);
});
document.addEventListener("mouseup", function(){
  if(dragCtx){ emit("endGesture",{name:dragCtx.knob.cfg.id}); var tk=dragCtx.knob; dragCtx=null;
    tk.lingerT=setTimeout(function(){ tk.el.classList.remove("touch"); },500); }
  if(charDrag){ emit("endGesture",{name:"main.character"}); charDrag=false; }
});
</script>
)rawhtml"

    + R"rawhtml(<script>
"use strict";
// ============================================================================
// Build controls
// ============================================================================
mkKnob("knob-size", {id:"main.size",label:"Size",min:0,max:1,size:"m",fmt:fPct,
  tip:"Changes the perceived size of the space."});
mkKnob("knob-decay",{id:"main.decay",label:"Decay",min:0.2,max:30,curve:"log",size:"m",fmt:fSec,
  tip:"Controls how long the reverb tail lasts."});
mkKnob("knob-bloom",{id:"main.bloom",label:"Bloom",min:0,max:1,size:"m",fmt:fPct,
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
selTime.addEventListener("change",function(){ P["echo.time"]=selTime.selectedIndex; setParamOnce("echo.time",selTime.selectedIndex); });

var selMode=document.getElementById("sel-duckmode");
selMode.addEventListener("change",function(){
  P["duck.mode"]=selMode.selectedIndex; setParamOnce("duck.mode",selMode.selectedIndex); updateDuckHint();
});
// rone-ui v2: the native selects become a key + popover (they stay in the DOM; .value / change keep working).
// They open upward: the modules sit low in the window and #win clips what drops below it.
RoneUI.select(selMode,{up:true});
RoneUI.select(selTime,{up:true,side:"right"});
function updateDuckHint(){
  document.getElementById("duck-hint").style.opacity = (Math.round(P["duck.mode"])===4)?"0":"1";
}

// Routing segmented
var segRouting=document.getElementById("seg-routing");
RoneUI.seg(segRouting,{onClass:"on"});   // the sliding pill follows the "on" class
segRouting.querySelectorAll("span").forEach(function(s){
  s.addEventListener("click",function(){
    var i=parseInt(s.getAttribute("data-i"));
    P["echo.routing"]=i; setParamOnce("echo.routing",i); renderRouting();
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
    var nv = P[paramId]>0.5?0:1; P[paramId]=nv; setParamOnce(paramId,nv);
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
  P["main.character"]=DEFAULTS["main.character"]; setParamOnce("main.character",P["main.character"]); renderChar();
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

// About — hardware back panel overlay (fills the window)
var bpM=document.getElementById('bp-about');
var bpU=bpM.querySelector('.bp-unit');
var bpRocker=document.getElementById('bp-rocker');
function bpRescale(){
  var s=Math.min(window.innerWidth/1240,window.innerHeight/780);
  s=Math.min(Math.max(s,0.22),1.45);
  bpU.style.setProperty('--s',s);
}
function bpClose(){ bpM.classList.add('bp-hidden'); }
document.getElementById("p-logo").addEventListener("click",function(){
  bpM.classList.remove('bp-hidden');
  bpRescale();
  bpU.classList.remove('bp-off','bp-entering');
  void bpU.offsetWidth; // restart the swing-in animation
  bpU.classList.add('bp-entering');
  bpRocker.setAttribute('aria-pressed','true');
});
document.getElementById('bp-close-btn').addEventListener('click',bpClose);
document.addEventListener('keydown',function(e){
  if(e.key==='Escape'&&!bpM.classList.contains('bp-hidden'))bpClose();
});
window.addEventListener('resize',function(){
  if(!bpM.classList.contains('bp-hidden'))bpRescale();
});
bpU.addEventListener('animationend',function(e){
  if(e.animationName==='bpSwingIn')bpU.classList.remove('bp-entering');
});
bpRocker.addEventListener('click',function(){
  var off=bpU.classList.toggle('bp-off');
  bpRocker.setAttribute('aria-pressed',off?'false':'true');
});
document.getElementById('bp-site').addEventListener('click',function(e){
  e.preventDefault();
  emit('openExternalUrl',{url:'https://roneaudio.com'});
});
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
    P[id]=v; setParamOnce(id,v);
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
      var h=document.createElement("div"); h.className="h"; h.textContent=pr.c; presetMenu.appendChild(h); }
    var it=document.createElement("div"); it.className="it"+(i===currentPreset?" on":"");
    var dot=document.createElement("span"); dot.className="d"; it.appendChild(dot);
    var tx=document.createElement("span"); tx.textContent=pr.n; it.appendChild(tx);
    it.addEventListener("click",function(e){ e.stopPropagation(); applyPreset(i); });
    presetMenu.appendChild(it);
  });
  var sep=document.createElement("div"); sep.className="sep"; presetMenu.appendChild(sep);
  var lock=document.createElement("div"); lock.className="it p-lock";
  lock.innerHTML='<input type="checkbox" '+(lockMix?"checked":"")+'>LOCK MIX WHILE BROWSING';
  lock.addEventListener("click",function(e){
    e.stopPropagation(); lockMix=!lockMix; renderPresetMenu();
  });
  presetMenu.appendChild(lock);
}
document.getElementById("preset-name").addEventListener("click",function(e){
  e.stopPropagation(); RoneUI.closeAll(); renderPresetMenu(); presetMenu.classList.toggle("open");
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
    if (d.version && !window.__bpVerSet) {   // the back panel's version line is the binary's
      window.__bpVerSet = true;
      var vl = document.querySelector('.bp-verline');
      if (vl) vl.textContent = vl.textContent.replace(/^[^\/]+\//, d.version + ' /');
    }
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
    if(name && el.textContent!==name){
      el.textContent=name;
      // The page is rebuilt every time the editor opens, so the browser's
      // position comes back from the processor with the name: prev / next
      // step from the preset that is showing, not from the top of the list.
      for(var i=0;i<PRESETS.length;i++){
        if(PRESETS[i].n.toUpperCase()===name){ currentPreset=i; break; }
      }
    }
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
  B.addEventListener("licenseStatus",function(d){ rlSetLicensed(!!(d && d.licensed)); });
  var rlBtn=document.getElementById('rlLaunch'); if(rlBtn) rlBtn.addEventListener('click',function(){ emit('launchCenter',{}); });
  // The editor sends nothing until the page says it is up (its pageReady)
  emit("uiReady",{});
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
  for (var i=0;i<220;++i){
    particles.push({
      ang:Math.random()*Math.PI*2,
      rad:0.12+Math.pow(Math.random(),1.35)*1.0,   // denser toward the core
      spd:(0.0018+Math.random()*0.004)*(Math.random()<0.5?1:-1),
      sz:0.8+Math.random()*2.1,
      tw:Math.random()*Math.PI*2,
      band:Math.random()
    });
  }
})();
var filamentRot=0;
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

  // Core glow — two layers: hot amber heart + wide halo (mockup's bright core)
  var coreR=baseR*0.55;
  var cg=vctx.createRadialGradient(cx,cy,0,cx,cy,coreR);
  cg.addColorStop(0,"rgba(255,224,196,"+(0.50*energy*(1-grK*0.4)).toFixed(3)+")");
  cg.addColorStop(0.25,"rgba(255,170,110,"+(0.22*energy).toFixed(3)+")");
  cg.addColorStop(0.6,"rgba(255,138,61,"+(0.09*energy).toFixed(3)+")");
  cg.addColorStop(1,"rgba(255,138,61,0)");
  vctx.fillStyle=cg;
  vctx.beginPath(); vctx.ellipse(cx,cy,coreR*wScale,coreR*vScale*1.3,0,0,7); vctx.fill();

  // Elliptical filaments — slow swirling strands around the core
  filamentRot+=0.0016*dt*(frozen?0.03:1);
  vctx.shadowColor="rgba(255,138,61,0.55)";
  for (var fi=0;fi<3;++fi){
    var fr=baseR*(0.55+fi*0.28);
    var rot=filamentRot*(fi%2?-1:1)+fi*1.1;
    vctx.strokeStyle="rgba(255,"+(150+fi*18)+","+(75+fi*22)+","
                     +(0.10*energy*(1+bloom*0.8)).toFixed(3)+")";
    vctx.lineWidth=1.4+fi*0.4;
    vctx.shadowBlur=8;
    vctx.beginPath();
    vctx.ellipse(cx,cy,fr*wScale,fr*vScale*(0.75+fi*0.12),rot*0.22,0,7);
    vctx.stroke();
  }
  vctx.shadowBlur=0;

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
var duckWaveT=0;
var lastDuckFrame=0;
function drawDuckMeter(ts){
  requestAnimationFrame(drawDuckMeter);
  // Same 30fps cap drawViz already uses. This loop was running at the full
  // display rate and each pass strokes a shadowBlur polyline, which is a
  // software blur pass per frame - the most expensive thing on the page.
  if(ts-lastDuckFrame<33) return; lastDuckFrame=ts;
  var w=dm.clientWidth, h=dm.clientHeight;
  if (w>0 && (dm.width!==w*2 || dm.height!==h*2)){ dm.width=w*2; dm.height=h*2; }
  var W=dm.width,H=dm.height;
  if(W===0) return;
  dctx.clearRect(0,0,W,H);
  duckWaveT+=0.055;
  dctx.strokeStyle="rgba(255,255,255,0.06)";
  dctx.lineWidth=1;
  dctx.beginPath(); dctx.moveTo(0,H*0.32); dctx.lineTo(W,H*0.32); dctx.stroke();
  // Living wave (mockup squiggle): breathes with wet energy, dips with GR
  var idleAmp=H*0.075*(0.55+clamp01(wetRms*6)*0.9);
  dctx.strokeStyle="#FF8A3D"; dctx.lineWidth=2.5; dctx.lineJoin="round";
  dctx.shadowColor="rgba(255,138,61,0.5)"; dctx.shadowBlur=6;
  dctx.beginPath();
  for (var i=0;i<grHistory.length;++i){
    var x=i/(grHistory.length-1)*W;
    var grN=clamp01(grHistory[i]/24);
    var y=H*0.32+grN*H*0.5
          +Math.sin(x*0.05+duckWaveT)*idleAmp*(1-grN*0.75);
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
var lastEchoFrame=0;
function drawEchoDots(ts){
  requestAnimationFrame(drawEchoDots);
  // 30fps, as above.
  if(ts-lastEchoFrame<33) return; lastEchoFrame=ts;
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
// Right-click = the host's parameter menu, never the browser menu
// ============================================================================
document.addEventListener("contextmenu", function(e){ e.preventDefault(); });
(function(){
  function bind(id, param){
    var el=document.getElementById(id);
    if(!el) return;
    el.addEventListener("contextmenu", function(e){
      e.preventDefault(); e.stopPropagation();
      emit("paramContextMenu",{name:param});
    });
  }
  bind("char-track","main.character");
  bind("bypass","main.bypass");
  bind("solowet-btn","main.solowet");
})();

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

// Deltas only: the editor knows its own size (window.innerWidth is unreliable
// across WebView2 zoom/DPI, and an absolute size makes the window jump on click).
var rsDrag=null;
document.getElementById("resize-handle").addEventListener("mousedown",function(e){
  rsDrag={x:e.screenX,y:e.screenY};
  emit("beginResize",{});
  e.preventDefault(); e.stopPropagation();
});
document.addEventListener("mousemove",function(e){
  if(!rsDrag) return;
  emit("requestResize",{dx:e.screenX-rsDrag.x, dy:e.screenY-rsDrag.y});
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
