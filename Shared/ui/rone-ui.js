// RONE UI kit (brand phase D) - the arc knob of the house language, from RONE Rise.
//   const k = RoneUI.knob(el, { size: 64, stroke: 4, value: 0.5, onChange: v => setParam('mix', v) });
//   k.set(0.7)   // host -> UI, no onChange
// Drag up/down (shift = fine), wheel 2 %/notch, double-click = the default. 270 degrees, -135..135.
(function (root) {
  'use strict';
  const NS = 'http://www.w3.org/2000/svg';
  const clamp = (v, a, b) => Math.min(b, Math.max(a, v));

  function arcPath(cx, cy, r, a0, a1) {
    const p = a => { const t = (a - 90) * Math.PI / 180; return [cx + r * Math.cos(t), cy + r * Math.sin(t)]; };
    const [x0, y0] = p(a0), [x1, y1] = p(a1);
    return `M ${x0.toFixed(2)} ${y0.toFixed(2)} A ${r} ${r} 0 ${a1 - a0 > 180 ? 1 : 0} 1 ${x1.toFixed(2)} ${y1.toFixed(2)}`;
  }

  function knob(el, opt) {
    const o = Object.assign({ size: 64, stroke: 4, value: 0, def: null, onChange: null }, opt || {});
    const S = o.size, c = S / 2, r = c - o.stroke;
    el.classList.add('rone-knob');
    el.style.width = el.style.height = S + 'px';
    const svg = document.createElementNS(NS, 'svg');
    svg.setAttribute('viewBox', `0 0 ${S} ${S}`);
    const track = document.createElementNS(NS, 'path'), lit = document.createElementNS(NS, 'path');
    track.setAttribute('class', 'track'); lit.setAttribute('class', 'lit');
    track.setAttribute('stroke-width', o.stroke); lit.setAttribute('stroke-width', o.stroke);
    track.setAttribute('d', arcPath(c, c, r, -135, 135));
    svg.append(track, lit);
    const cap = document.createElement('div'); cap.className = 'cap';
    const inset = Math.round(o.stroke * 2.5); cap.style.inset = inset + 'px';
    const pivot = document.createElement('div'); pivot.className = 'pivot';
    const dot = document.createElement('div'); dot.className = 'dot'; dot.style.top = Math.round(S * .1) + 'px';
    pivot.appendChild(dot); cap.appendChild(pivot);
    el.replaceChildren(svg, cap);

    let v = clamp(o.value, 0, 1);
    const def = o.def == null ? v : o.def;
    function draw() {
      const a = -135 + 270 * v;
      lit.setAttribute('d', v > 0.003 ? arcPath(c, c, r, -135, a) : '');
      pivot.style.transform = `rotate(${a.toFixed(1)}deg)`;
    }
    function user(nv) { nv = clamp(nv, 0, 1); if (nv === v) return; v = nv; draw(); if (o.onChange) o.onChange(v); }

    let y0 = 0, v0 = 0;
    el.addEventListener('pointerdown', e => { el.setPointerCapture(e.pointerId); y0 = e.clientY; v0 = v; });
    el.addEventListener('pointermove', e => {
      if (!el.hasPointerCapture(e.pointerId)) return;
      user(v0 + (y0 - e.clientY) / (e.shiftKey ? 800 : 200));
    });
    el.addEventListener('wheel', e => { e.preventDefault(); user(v + (e.deltaY < 0 ? 1 : -1) * (e.shiftKey ? .005 : .02)); }, { passive: false });
    el.addEventListener('dblclick', () => user(def));
    draw();
    return { set(nv) { v = clamp(nv, 0, 1); draw(); }, get: () => v, el };
  }

  root.RoneUI = { knob, arcPath };
})(window);
