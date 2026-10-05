#pragma once

// ============================================================================
// RoneWebCompat - makes every RONE WebView page lay out on an OLD macOS WebKit.
// (Shipped in every plugin by the full rebuild that followed the GitHub Actions
// outage of 2026-10-05, which cancelled half of the first run.)
//
// Found 2026-10-05 in Zanon's videos (Logic, Mac): every plugin worked but its
// parts sat in the wrong places - Stucker's "OFF / STUCK" in the top-left
// corner instead of inside the knob, the knob's inner disc gone, the LEDs
// squashed together, the R glyph touching the name. A plugin's WebView on macOS
// is the system WKWebView, so its engine is whatever Safari the Mac has. Two
// CSS features every RONE page uses arrived only in Safari 14.1 (2021):
//     inset: 0          (top/right/bottom/left in one) - dropped at parse time,
//                       so each absolutely placed layer collapses to its corner
//     gap in a flexbox  - ignored, so every row's items touch
// Stripping both from Stucker's page in Edge reproduces his screen exactly.
//
// This script is added to every plugin's WebView through RoneUpdatePrompt::addTo
// (JUCE withUserScript, document start), so no plugin's HTML changes. Where the
// engine has both features it does nothing at all. Where it does not:
//   - inset: the raw text of every <style>, <link> sheet and style="" attribute
//     still holds it (only the CSSOM dropped it); it is rewritten to the four
//     long-hand properties, also for <style> elements added later.
//   - flex gap: each flex container's computed row-gap/column-gap (the parser
//     keeps them - grid has had gap since Safari 12) is turned into a margin on
//     every visible in-flow child after the first; re-applied as children
//     appear, disappear or change.
// window.__RONE_COMPAT_FORCE = true (set before the page loads) runs both paths
// in a modern browser and neutralises the native gap first - that is how the
// polyfill was checked against the real layout in Edge.
//
// Plain ES5, one IIFE, well under MSVC's 16 KB literal limit.
// ============================================================================

namespace RoneWebCompat
{
    inline const char* script()
    {
        return R"RONECOMPAT(
(function () {
  if (window.__roneCompat) return;
  window.__roneCompat = true;
  var FORCE = !!window.__RONE_COMPAT_FORCE;
  var hasCSS = !!(window.CSS && CSS.supports);
  var needInset = FORCE || !(hasCSS && CSS.supports('inset', '0'));

  // ---- inset -> top/right/bottom/left ---------------------------------------
  function splitValues(v) {
    var out = [], cur = '', depth = 0;
    for (var i = 0; i < v.length; i++) {
      var c = v.charAt(i);
      if (c === '(') depth++;
      if (c === ')') depth--;
      if (/\s/.test(c) && depth === 0) { if (cur) { out.push(cur); cur = ''; } }
      else cur += c;
    }
    if (cur) out.push(cur);
    return out;
  }
  var INSET_RE = /(^|[;{\s"'])inset\s*:\s*([^;}"']+?)\s*(!important)?\s*(?=;|}|"|'|$)/g;
  function rewriteInset(text) {
    if (!text || text.indexOf('inset') < 0) return text;
    return text.replace(INSET_RE, function (m, pre, val, imp) {
      var v = splitValues(val);
      if (!v.length || v.length > 4) return m;
      var t = v[0], r = v[1] || t, b = v[2] || t, l = v[3] || r;
      var s = imp ? ' !important' : '';
      return pre + 'top:' + t + s + ';right:' + r + s + ';bottom:' + b + s + ';left:' + l + s;
    });
  }
  function fixStyleEl(el) {
    if (el.__roneInset) return;
    el.__roneInset = true;
    var t = el.textContent, n = rewriteInset(t);
    if (n !== t) el.textContent = n;
  }
  function fixLinkEl(el) {
    if (el.__roneInset || !el.href || !/stylesheet/i.test(el.rel || '')) return;
    if (/^https?:/i.test(el.href) && el.href.split('/')[2] !== location.host) return;   // web fonts etc.
    el.__roneInset = true;
    try {
      var x = new XMLHttpRequest();
      x.open('GET', el.href, true);
      x.onload = function () {
        var t = x.responseText || '';
        if (t.indexOf('inset') < 0) return;
        var s = document.createElement('style');
        s.__roneInset = true;
        s.textContent = rewriteInset(t);
        if (el.parentNode) el.parentNode.insertBefore(s, el.nextSibling);
      };
      x.send();
    } catch (e) {}
  }
  function fixAttr(el) {
    var a = el.getAttribute && el.getAttribute('style');
    if (a && a.indexOf('inset') >= 0) {
      var n = rewriteInset(a);
      if (n !== a) el.setAttribute('style', n);
    }
  }
  function fixInsetIn(root) {
    var i, l;
    if (root.tagName === 'STYLE') fixStyleEl(root);
    else if (root.tagName === 'LINK') fixLinkEl(root);
    else if (root.nodeType === 1) fixAttr(root);
    if (!root.querySelectorAll) return;
    l = root.querySelectorAll('style'); for (i = 0; i < l.length; i++) fixStyleEl(l[i]);
    l = root.querySelectorAll('link'); for (i = 0; i < l.length; i++) fixLinkEl(l[i]);
    l = root.querySelectorAll('[style*="inset"]'); for (i = 0; i < l.length; i++) fixAttr(l[i]);
  }

  // ---- flex gap -> margins ----------------------------------------------------
  function flexGapWorks() {
    var d = document.createElement('div');
    d.style.cssText = 'display:flex;flex-direction:column;row-gap:1px;position:absolute;visibility:hidden';
    d.appendChild(document.createElement('div'));
    d.appendChild(document.createElement('div'));
    document.body.appendChild(d);
    var ok = d.scrollHeight === 1;
    d.parentNode.removeChild(d);
    return ok;
  }
  var gapBoxes = [];
  function px(v) { var n = parseFloat(v); return isNaN(n) ? 0 : n; }
  function scanGapBoxes() {
    var all = document.body.getElementsByTagName('*'), out = [];
    for (var i = 0; i < all.length; i++) {
      var el = all[i], cs = getComputedStyle(el);
      if (cs.display !== 'flex' && cs.display !== 'inline-flex') continue;
      if (el.__roneGap) { out.push(el); continue; }
      var rg = px(cs.rowGap), cg = px(cs.columnGap);
      if (!rg && !cg) continue;
      el.__roneGap = { row: rg, col: cg };
      if (FORCE) { el.style.rowGap = '0px'; el.style.columnGap = '0px'; }
      out.push(el);
    }
    gapBoxes = out;
  }
  // Is this margin written as "auto" anywhere that matches? (an auto margin
  // pushes items apart by itself and must not be turned into pixels)
  function autoMargin(el, prop, pseudo) {
    var css = cssProp(prop), side = css.split('-')[1], found = false;
    if (!pseudo && /auto/.test(el.style[prop])) return true;
    function walk(rules) {
      for (var i = 0; rules && i < rules.length && !found; i++) {
        var r = rules[i];
        if (r.cssRules && !r.selectorText) { walk(r.cssRules); continue; }
        if (!r.style || !r.selectorText) continue;
        var v = r.style.getPropertyValue(css) || r.style.getPropertyValue('margin-inline-' + (side === 'left' ? 'start' : 'end'));
        if (v.indexOf('auto') < 0) continue;
        var sels = r.selectorText.split(',');
        for (var s = 0; s < sels.length && !found; s++) {
          var sel = sels[s].trim();
          var hasP = /::?(before|after)\s*$/.test(sel);
          if (pseudo ? !hasP || sel.replace(/::?(before|after)\s*$/, '') === '' : hasP) continue;
          if (pseudo && sel.slice(-pseudo.length) !== pseudo) continue;
          try { if (el.matches(pseudo ? sel.replace(/::?(before|after)\s*$/, '') : sel)) found = true; } catch (e) {}
        }
      }
    }
    for (var i = 0; i < document.styleSheets.length && !found; i++) {
      try { walk(document.styleSheets[i].cssRules); } catch (e) {}
    }
    return found;
  }
  // The gap is ADDED to the margin the page gave the item (e.g. 2px): mine[prop]
  // keeps that base and the value last written.
  function setMargin(k, prop, value) {
    var mine = k.__roneMargins || (k.__roneMargins = {});
    if (!(prop in mine)) {
      if (!value) return;              // nothing to add and never touched
      if (autoMargin(k, prop)) { mine[prop] = null; return; }
      mine[prop] = { base: px(getComputedStyle(k)[prop]), inline: k.style[prop], v: null };
    }
    var m = mine[prop];
    if (m === null) return;
    var v = value ? (m.base + value) + 'px' : m.inline;
    if (m.v !== v) { m.v = v; k.style[prop] = v; }
  }
  // ::before / ::after flex items (icons, dots) cannot take an inline style:
  // they get a rule in one sheet of our own, keyed by a data attribute.
  var pseudoRules = {}, pseudoSheet = null, pseudoDirty = false, nextGapId = 1;
  function cssProp(p) { return p.replace(/[A-Z]/g, function (c) { return '-' + c.toLowerCase(); }); }
  function setPseudoMargin(host, which, prop, value) {
    var mine = host['__ronePM' + which] || (host['__ronePM' + which] = {});
    if (!(prop in mine)) {
      if (!value) return;
      if (autoMargin(host, prop, which)) { mine[prop] = null; return; }
      mine[prop] = { base: px(getComputedStyle(host, '::' + which)[prop]), v: 0 };
    }
    var m = mine[prop];
    if (m === null || m.v === value) return;
    m.v = value;
    if (!host.__roneGapId) { host.__roneGapId = nextGapId++; host.setAttribute('data-rone-gap', host.__roneGapId); }
    var key = host.__roneGapId + '::' + which + ' ' + prop;
    if (value) pseudoRules[key] = '[data-rone-gap="' + host.__roneGapId + '"]::' + which + '{' + cssProp(prop) + ':' + (m.base + value) + 'px !important}';
    else delete pseudoRules[key];
    pseudoDirty = true;
  }
  function flushPseudo() {
    if (!pseudoDirty) return;
    pseudoDirty = false;
    if (!pseudoSheet) {
      pseudoSheet = document.createElement('style');
      pseudoSheet.id = 'rone-compat-gap';
      pseudoSheet.__roneInset = true;
      document.head.appendChild(pseudoSheet);
    }
    var out = [];
    for (var k in pseudoRules) out.push(pseudoRules[k]);
    pseudoSheet.textContent = out.join('\n');
  }
  function pseudoItem(box, which) {
    var p = getComputedStyle(box, '::' + which);
    if (!p || !p.content || p.content === 'none' || p.content === 'normal' || p.display === 'none') return null;
    if (p.position === 'absolute' || p.position === 'fixed') return null;
    return { pseudo: which };
  }
  // The flex items of a box in order: elements, non-blank text runs, pseudos.
  function flexItems(box) {
    var items = [], b = pseudoItem(box, 'before'), a = pseudoItem(box, 'after');
    if (b) items.push(b);
    var n = box.firstChild, text = false;
    for (; n; n = n.nextSibling) {
      if (n.nodeType === 3) {
        if (/\S/.test(n.nodeValue) && !text) { items.push({ text: true }); text = true; }
      } else if (n.nodeType === 1) {
        var kcs = getComputedStyle(n);
        if (kcs.display === 'none') continue;
        if (kcs.position === 'absolute' || kcs.position === 'fixed') continue;
        if (kcs.display === 'contents') continue;
        items.push({ el: n }); text = false;
      }
    }
    if (a) items.push(a);
    return items;
  }
  var OPPOSITE = { marginLeft: 'marginRight', marginRight: 'marginLeft', marginTop: 'marginBottom', marginBottom: 'marginTop' };
  // The gap before item j goes on item j's leading side, or - when item j is a
  // text run - on the trailing side of item j-1.
  function gapBetween(box, prev, item, lead, value) {
    if (item.el) return setMargin(item.el, lead, value);
    if (item.pseudo) return setPseudoMargin(box, item.pseudo, lead, value);
    if (prev && prev.el) return setMargin(prev.el, OPPOSITE[lead], value);
    if (prev && prev.pseudo) return setPseudoMargin(box, prev.pseudo, OPPOSITE[lead], value);
  }
  function applyGaps() {
    for (var b = 0; b < gapBoxes.length; b++) {
      var box = gapBoxes[b], g = box.__roneGap, cs = getComputedStyle(box);
      if (cs.display === 'none') continue;
      var dir = cs.flexDirection, col = dir.indexOf('column') === 0, rev = dir.indexOf('reverse') > 0;
      var wrap = cs.flexWrap !== 'nowrap';
      var mainGap = col ? g.row : g.col, crossGap = col ? g.col : g.row;
      var mainProp = col ? (rev ? 'marginBottom' : 'marginTop') : (rev ? 'marginRight' : 'marginLeft');
      var crossProp = col ? 'marginLeft' : 'marginTop';
      var items = flexItems(box), kids = [];
      for (var j = 0; j < items.length; j++) {
        if (items[j].el) kids.push(items[j].el);
        if (j) gapBetween(box, items[j - 1], items[j], mainProp, mainGap);
        else if (items[j].el) setMargin(items[j].el, mainProp, 0);
      }
      if (wrap && kids.length > 1) {
        var line = 0, prev = col ? kids[0].offsetLeft : kids[0].offsetTop;
        for (var w = 0; w < kids.length; w++) {
          var pos = col ? kids[w].offsetLeft : kids[w].offsetTop;
          var newLine = w > 0 && Math.abs(pos - prev) > 1;
          if (newLine) { line++; setMargin(kids[w], mainProp, 0); }
          prev = pos;
          if (crossGap) setMargin(kids[w], crossProp, line ? crossGap : 0);
        }
      }
    }
    flushPseudo();
  }

  // ---- wiring -----------------------------------------------------------------
  function start() {
    if (needInset) fixInsetIn(document.documentElement);
    var needGap = FORCE || !flexGapWorks();
    if (!needInset && !needGap) return;
    var pending = false, rescan = false;
    function schedule(full) {
      rescan = rescan || full;
      if (pending) return;
      pending = true;
      // at most ~8 passes a second: meters flip classes at 30 Hz
      setTimeout(function () {
        pending = false;
        if (needGap) { if (rescan) scanGapBoxes(); applyGaps(); }
        rescan = false;
      }, 120);
    }
    new MutationObserver(function (recs) {
      var full = false;
      for (var r = 0; r < recs.length; r++) {
        var rec = recs[r];
        if (rec.type === 'childList') {
          full = true;
          if (needInset) for (var a = 0; a < rec.addedNodes.length; a++) {
            var n = rec.addedNodes[a];
            if (n.nodeType === 1) fixInsetIn(n);
            else if (n.parentNode && n.parentNode.tagName === 'STYLE') { n.parentNode.__roneInset = false; fixStyleEl(n.parentNode); }
          }
        }
      }
      if (needGap) schedule(full);
    }).observe(document.documentElement, { childList: true, subtree: true, attributes: true, attributeFilter: ['class', 'style', 'hidden'] });
    if (needGap) {
      scanGapBoxes(); applyGaps();
      window.addEventListener('resize', function () { schedule(false); });
      setInterval(function () { schedule(true); }, 1500);
    }
  }
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', start);
  else start();
})();
)RONECOMPAT";
    }
}
