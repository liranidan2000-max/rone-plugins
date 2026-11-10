// reopen_watch.mjs <profile root> <seconds> - every 100 ms finds every WebView2 browser under
// <profile root> (each runs with --remote-debugging-port=0 and writes its port to
// <folder>\EBWebView\DevToolsActivePort, so two browsers of two folders never fight over one
// port) and prints (ms since epoch) when the number of live browsers and of
// https://juce.backend/ pages changes. reopen.ps1 lines it up with ReopenEditorHost's OPEN/CLOSE.
import { readdirSync, readFileSync, existsSync } from 'node:fs';
import { join } from 'node:path';

const [root, seconds] = [process.argv[2], Number(process.argv[3])];
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const end = Date.now() + seconds * 1000;

function ports() {
  const out = [];
  const visit = (dir, depth) => {
    if (depth > 3 || !existsSync(dir)) return;
    const f = join(dir, 'EBWebView', 'DevToolsActivePort');
    if (existsSync(f)) {
      try { const p = Number(readFileSync(f, 'utf8').split(/\r?\n/)[0]); if (p > 0) out.push(p); } catch {}
    }
    try { for (const e of readdirSync(dir, { withFileTypes: true })) if (e.isDirectory() && e.name !== 'EBWebView') visit(join(dir, e.name), depth + 1); } catch {}
  };
  visit(root, 0);
  return [...new Set(out)];
}

let alive = null, pages = null;
while (Date.now() < end) {
  let a = 0, n = 0;
  await Promise.all(ports().map(async (port) => {
    try {
      const r = await fetch(`http://127.0.0.1:${port}/json/list`, { signal: AbortSignal.timeout(400) });
      const list = await r.json();
      a++;
      n += list.filter((p) => p.type === 'page' && String(p.url).startsWith('https://juce.backend')).length;
    } catch {}
  }));
  if (a !== alive) { console.log(`BROWSERS ${a} ${Date.now()}`); alive = a; }
  if (n !== pages) { console.log(`PAGES ${n} ${Date.now()}`); pages = n; }
  await sleep(100);
}
