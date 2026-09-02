// Doc-shot helper (served copy only, never shipped).
(function () {
  var q = new URLSearchParams(location.search);
  var state = (q.get('state') || '').split(',').filter(Boolean);
  function $(id) { return document.getElementById(id); }
  function click(id) { var el = $(id); if (el) { el.dispatchEvent(new MouseEvent('mousedown', {bubbles:true})); el.dispatchEvent(new MouseEvent('mouseup', {bubbles:true})); el.click(); } }
  function applyState() {
    // never show the license gate in docs shots
    var lo = $('license-overlay'); if (lo) { lo.classList.add('hidden'); lo.style.display = 'none'; }
    state.forEach(function (s) {
      var m = s.match(/^click:(.+)$/); if (m) { click(m[1]); return; }
      m = s.match(/^show:(.+)$/); if (m) { var el = $(m[1]); if (el) { el.classList.remove('hidden', 'collapsed', 'bp-hidden'); el.style.display = ''; } return; }
      m = s.match(/^class:([^:]+):([^:]+)$/); if (m) { var el2 = $(m[1]); if (el2) el2.classList.add(m[2]); return; }
      m = s.match(/^js:(.+)$/); if (m) { try { (0, eval)(decodeURIComponent(m[1])); } catch (e) { console.warn(e); } }
    });
  }
  function collect() {
    var out = {};
    var els = document.querySelectorAll('[id], [data-param], .p-mini, label, .knob-group, .p-minilabel, .glabel, .chip, .div-btn, .seg button, button, select, .tgl, .mini-btn, .action-btn');
    var n = 0;
    els.forEach(function (el) {
      var r = el.getBoundingClientRect();
      if (r.width < 2 || r.height < 2) return;
      var key = el.id ? '#' + el.id : (el.dataset && el.dataset.param) ? '@' + el.dataset.param + '[' + (n++) + ']' : (el.tagName.toLowerCase() + (el.className && typeof el.className === 'string' ? '.' + el.className.trim().split(/\s+/).join('.') : '') + '[' + (n++) + ']:' + (el.textContent || '').trim().slice(0, 24));
      out[key] = [Math.round(r.left * 10) / 10, Math.round(r.top * 10) / 10, Math.round(r.width * 10) / 10, Math.round(r.height * 10) / 10];
    });
    var payload = { w: window.innerWidth, h: window.innerHeight, dpr: window.devicePixelRatio, rects: out };
    if (window.parent !== window) window.parent.postMessage({ type: 'rects', payload: payload }, '*');
  }
  var done = false;
  function go() { if (done) return; done = true; setTimeout(function () { applyState(); setTimeout(collect, 1200); }, 800); }
  if (document.fonts && document.fonts.ready) document.fonts.ready.then(go);
  window.addEventListener('load', function () { setTimeout(go, 2500); });
})();
