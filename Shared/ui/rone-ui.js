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
