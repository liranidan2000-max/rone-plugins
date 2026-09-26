// cdp_check.mjs <port> <expected pages> <event name> <timeout ms>
// Waits until <expected> https://juce.backend pages exist on the WebView2 debug port
// (or the timeout passes), then asks EACH page whether C++ is still pushing to it:
// it listens for one <event name> from the backend for up to 2.5 s.
// Prints one line per page; exit 0 only if every expected page exists AND hears the event.
const [port, expectedArg, eventName, timeoutArg] = process.argv.slice(2);
const expected = Number(expectedArg || 2);
const timeoutMs = Number(timeoutArg || 12000);
const t0 = Date.now();
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

async function listPages() {
  try {
    const r = await fetch(`http://127.0.0.1:${port}/json/list`);
    const all = await r.json();
    return all.filter((t) => t.type === 'page' && String(t.url).startsWith('https://juce.backend'));
  } catch { return null; }
}

function evaluate(wsUrl, expression) {
  return new Promise((resolve) => {
    const ws = new WebSocket(wsUrl);
    const done = (v) => { try { ws.close(); } catch {} resolve(v); };
    const guard = setTimeout(() => done('ws-timeout'), 6000);
    ws.onopen = () => ws.send(JSON.stringify({ id: 1, method: 'Runtime.evaluate',
      params: { expression, awaitPromise: true, returnByValue: true } }));
    ws.onmessage = (m) => {
      const msg = JSON.parse(m.data);
      if (msg.id === 1) { clearTimeout(guard); done(msg.result?.result?.value ?? JSON.stringify(msg.result ?? msg.error)); }
    };
    ws.onerror = () => { clearTimeout(guard); done('ws-error'); };
  });
}

let pages = [];
while (Date.now() - t0 < timeoutMs) {
  const p = await listPages();
  if (p) pages = p;
  if (pages.length >= expected) break;
  await sleep(250);
}
console.log(`cdp: ${pages.length} of ${expected} pages after ${((Date.now() - t0) / 1000).toFixed(1)} s`);

const probe = `new Promise((res) => {
  const b = window.__JUCE__ && window.__JUCE__.backend;
  if (!b) return res('no JUCE backend on the page');
  const t = setTimeout(() => res('NO ${eventName} within 2.5 s'), 2500);
  b.addEventListener('${eventName}', (p) => { clearTimeout(t); res('${eventName} arrives: ' + JSON.stringify(p).slice(0, 90)); });
})`;

let ok = pages.length >= expected;
for (const [i, p] of pages.entries()) {
  const v = await evaluate(p.webSocketDebuggerUrl, probe);
  if (!String(v).startsWith(`${eventName} arrives`)) ok = false;
  console.log(`cdp: page ${i + 1} (${p.id.slice(0, 8)}) -> ${v}`);
}
process.exit(ok ? 0 : 1);
