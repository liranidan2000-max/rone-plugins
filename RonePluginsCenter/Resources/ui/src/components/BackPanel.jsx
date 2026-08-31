import React, { useEffect, useRef } from 'react'
import { createPortal } from 'react-dom'
import { callNative, isDevMode } from '../bridge'

// ============================================================================
// BackPanel — the RONE unified About screen: the hardware rear of the unit.
// Fills the whole window: metal stretches, hardware groups stay in scale
// (--s) and anchor to the edges. Same bp- design system as the plugins.
// ============================================================================

const BP_CSS = `
/* ===== About — hardware back panel (RONE unified, fills the window) =====
   The unit IS the window: metal stretches, hardware groups stay in scale
   (--s) and anchor to the edges, like the real back of the device. */
#bp-about{
  --bp-glow:#E8EAED;--bp-glow-rgb:232,234,237;
  --bp-etch:#9aa0a8;--bp-etch-dim:#70757d;--bp-etch-faint:#565b62;
  --bp-sans:'Archivo','Segoe UI',Arial,sans-serif;
  --bp-mono:'IBM Plex Mono',Consolas,ui-monospace,monospace;
  position:fixed;inset:0;z-index:1000;background:#0a0b0d;}
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
.bp-g{position:absolute;transform:scale(var(--s));}
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
.bp-brandtag{font-family:var(--bp-sans);font-weight:500;font-size:10.5px;letter-spacing:.22em;color:var(--bp-etch-faint);}
.bp-model{font-family:var(--bp-sans);font-weight:500;font-size:74px;line-height:1.05;
  letter-spacing:-.015em;color:#a6abb2;white-space:nowrap;
  text-shadow:0 1px 0 rgba(255,255,255,.05),0 -1px 1px rgba(0,0,0,.45);}
.bp-sitelink{text-decoration:none;display:flex;flex-direction:column;align-items:flex-end;gap:8px;cursor:pointer;}
.bp-sl-eyebrow{font-family:var(--bp-sans);font-weight:600;font-size:10.5px;letter-spacing:.3em;
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
.bp-credits{display:flex;flex-direction:column;gap:13px;font-family:var(--bp-sans);text-align:left;}
.bp-cr-row{display:flex;align-items:center;gap:9px;font-size:15px;color:var(--bp-etch);}
.bp-cr-row b{font-weight:600;color:#c3c8ce;}
.bp-cr-lead{color:var(--bp-etch-dim);font-weight:400;}
.bp-avatar{width:24px;height:24px;border-radius:5px;overflow:hidden;flex:0 0 auto;box-shadow:0 1px 3px rgba(0,0,0,.6);}
.bp-avatar svg{display:block;width:100%;height:100%;shape-rendering:crispEdges;}
.bp-cr-sub{font-size:13px;color:var(--bp-etch-dim);}
.bp-cr-tech{font-size:11.5px;color:var(--bp-etch-faint);letter-spacing:.05em;}
.bp-legal{display:flex;align-items:center;gap:14px;font-family:var(--bp-sans);
  font-size:12.5px;color:var(--bp-etch-dim);white-space:nowrap;}
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
.bp-sy-head{background:#f2c61d;color:#151310;font-weight:700;font-size:8px;letter-spacing:.1em;
  text-align:center;padding:2px 0 1px;}
.bp-sy-body{display:flex;align-items:center;gap:5px;padding:3px 6px 4px;}
.bp-sy-body svg{flex:0 0 auto;}
.bp-sy-txt{font-size:6.3px;line-height:1.35;color:#2a2620;font-weight:500;text-align:left;}
.bp-stick-w{width:186px;background:#f1f0eb;border-radius:2px;padding:4px 6px 5px;transform:rotate(.5deg);
  box-shadow:0 2px 4px rgba(0,0,0,.55);color:#1d1c1a;}
.bp-sw-head{display:flex;align-items:center;justify-content:center;gap:6px;border:1px solid #1d1c1a;
  padding:1.5px 4px;margin-bottom:3px;}
.bp-sw-head span{font-weight:700;font-size:8px;letter-spacing:.24em;}
.bp-sw-bold{font-weight:700;font-size:6.4px;text-align:center;letter-spacing:.03em;margin-bottom:2px;}
.bp-sw-fine{font-size:5.6px;line-height:1.45;color:#3c3a36;text-align:justify;}
.bp-verline{margin-top:4px;font-family:var(--bp-mono);font-weight:500;font-size:12px;letter-spacing:.05em;
  color:#8f959c;white-space:nowrap;font-variant-numeric:tabular-nums;}
.bp-stick-sn{width:132px;transform:rotate(-.9deg);
  background:linear-gradient(180deg,#dcdee0,#c2c5c8);border-radius:2px;padding:5px 8px 6px;
  box-shadow:0 2px 4px rgba(0,0,0,.55);color:#17181a;font-family:var(--bp-sans);}
.bp-sn-top{font-weight:700;font-size:5.8px;letter-spacing:.1em;color:#3a3c3f;margin-bottom:3px;white-space:nowrap;}
.bp-sn-row{display:flex;align-items:center;gap:6px;}
.bp-barcode{width:52px;height:15px;flex:0 0 auto;
  background:repeating-linear-gradient(90deg,#17181a 0 1.5px,transparent 1.5px 3px,#17181a 3px 5px,transparent 5px 6.5px,#17181a 6.5px 7.5px,transparent 7.5px 10.5px);}
.bp-sn-num{font-family:var(--bp-mono);font-weight:600;font-size:7.8px;white-space:nowrap;}
@media (prefers-reduced-motion:reduce){
  .bp-unit.bp-entering{animation:none;}
  .bp-blades{animation:none;}
  .bp-slot::before,.bp-slot:nth-child(5)::after{animation:none;}
}`

function unitHTML(verline) {
  return `<div class="bp-chassis">
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
      <h1 class="bp-model">PLUGINS CENTER</h1>
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
              <rect x="1" y="3" width="1" height="3" fill="#E8EAED"/>
              <rect x="9" y="3" width="1" height="3" fill="#E8EAED"/>
            </svg>
          </span>
          <b>Liran "RONE" Kalifa</b>
        </div>
        <div class="bp-cr-sub">Panel &amp; interface design by RONE Studio</div>
        <div class="bp-cr-tech">BUILT ON JUCE 8 &middot; REACT UI ENGINE</div>
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
        <span class="bp-sn-num">S/N RN-CTR-260127</span>
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
      <div class="bp-verline">${verline}</div>
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
</div>`
}

export default function BackPanel({ version, onClose }) {
  const unitRef = useRef(null)

  useEffect(() => {
    const rescale = () => {
      if (!unitRef.current) return
      let s = Math.min(window.innerWidth / 1240, window.innerHeight / 780)
      s = Math.min(Math.max(s, 0.22), 1.45)
      unitRef.current.style.setProperty('--s', s)
    }
    rescale()
    window.addEventListener('resize', rescale)
    const onKey = (e) => { if (e.key === 'Escape') onClose() }
    window.addEventListener('keydown', onKey)
    return () => {
      window.removeEventListener('resize', rescale)
      window.removeEventListener('keydown', onKey)
    }
  }, [onClose])

  useEffect(() => {
    const u = unitRef.current
    if (!u) return
    const done = (e) => { if (e.animationName === 'bpSwingIn') u.classList.remove('bp-entering') }
    u.addEventListener('animationend', done)
    return () => u.removeEventListener('animationend', done)
  }, [])

  // The panel guts are shared static HTML (identical to the plugins' overlays);
  // interactivity is delegated from the unit node.
  const handleUnitClick = (e) => {
    const rocker = e.target.closest && e.target.closest('#bp-rocker')
    if (rocker) {
      const off = unitRef.current.classList.toggle('bp-off')
      rocker.setAttribute('aria-pressed', off ? 'false' : 'true')
      return
    }
    const site = e.target.closest && e.target.closest('#bp-site')
    if (site) {
      e.preventDefault()
      if (isDevMode()) window.open('https://roneaudio.com', '_blank')
      else callNative('openExternalUrl', 'https://roneaudio.com').catch(() => {})
    }
  }

  const verline = `${version || '1.1.0.127'} / DESKTOP APP / WIN&middot;MAC`

  // Portal to <body>: Settings lives inside a framer-motion transform, which
  // would otherwise trap position:fixed inside the content area.
  return createPortal(
    <div id="bp-about" role="dialog" aria-modal="true" aria-label="About RONE Plugins Center">
      <style>{BP_CSS}</style>
      <button className="bp-close" onClick={onClose} title="Close">&#10005;&nbsp; CLOSE</button>
      <div
        className="bp-unit bp-entering"
        ref={unitRef}
        onClick={handleUnitClick}
        dangerouslySetInnerHTML={{ __html: unitHTML(verline) }}
      />
    </div>,
    document.body
  )
}
