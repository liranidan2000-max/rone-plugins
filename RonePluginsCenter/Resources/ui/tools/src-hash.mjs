// The page's sources as one SHA256 - written into dist/src-hash.txt by every
// `npm run build` (vite.config.js) and checked by test/dist-fresh.test.mjs.
//
// Why (audit R032, R197): CI does not build this page - CMake embeds the committed
// dist/ - so a fix in src/ that was committed without a rebuild never shipped.
// Center 2.1.2 went out with a 2.0.4 bundle, and a failed Mac install still said
// "installer code 0". `npm test` now fails until dist/ is rebuilt from src/.
//
// Line ends are normalised, so a Windows checkout (CRLF) and a Mac one hash the same.
import fs from 'node:fs'
import path from 'node:path'
import crypto from 'node:crypto'

const ROOT_FILES = ['index.html', 'vite.config.js', 'tailwind.config.js', 'postcss.config.js', 'package.json', 'package-lock.json']

function listFiles (dir, base, out) {
  for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
    const full = path.join(dir, e.name)
    if (e.isDirectory()) listFiles(full, base, out)
    else out.push(path.relative(base, full).split(path.sep).join('/'))
  }
  return out
}

export function srcHash (uiDir) {
  const files = listFiles(path.join(uiDir, 'src'), uiDir, [])
  for (const f of ROOT_FILES) if (fs.existsSync(path.join(uiDir, f))) files.push(f)
  files.sort()

  const h = crypto.createHash('sha256')
  for (const f of files) {
    const text = fs.readFileSync(path.join(uiDir, f), 'utf8').replace(/\r\n/g, '\n')
    h.update(f + '\0' + text + '\0')
  }
  return h.digest('hex')
}
