import path from 'node:path'
import { fileURLToPath } from 'node:url'
import tailwindcss from 'tailwindcss'
import autoprefixer from 'autoprefixer'

// Explicit config path: the dev server may be started from any folder, and
// Tailwind otherwise looks for its config (and the files to scan) from there.
const here = path.dirname(fileURLToPath(import.meta.url))

// On the Mac the Center's page runs in the system WebKit, which on an older
// macOS is an older Safari (Zanon's, 2026-10-06). Two things in the built CSS
// break it there, and this plugin rewrites both at build time:
//   :where(...)  - unknown before Safari 14. One unknown selector drops the
//                  WHOLE rule, so Tailwind's "button, input:where(...)" reset
//                  vanished: every button kept the grey native background and
//                  its light text disappeared until hover. :where(x) -> x.
//   inset: ...   - unknown before Safari 14.1: written out as top/right/
//                  bottom/left (the shorthand stays after them for new engines).
// Flex `gap` cannot be rewritten in CSS; Shared/RoneWebCompat.h does that at
// run time in the page.
function unwrapWhere (selector) {
  let out = '', i = 0
  while (i < selector.length) {
    if (selector.startsWith(':where(', i)) {
      let depth = 1, j = i + 7
      while (j < selector.length && depth > 0) {
        if (selector[j] === '(') depth++
        else if (selector[j] === ')') depth--
        j++
      }
      out += unwrapWhere(selector.slice(i + 7, j - 1))
      i = j
    } else {
      out += selector[i++]
    }
  }
  return out
}

const expanded = new WeakSet()
const oldWebKit = () => ({
  postcssPlugin: 'rone-old-webkit',
  Rule (rule) {
    if (rule.selector && rule.selector.includes(':where(')) {
      // A selector list inside :where() would change meaning once unwrapped.
      if (!/:where\([^()]*,/.test(rule.selector))
        rule.selector = unwrapWhere(rule.selector)
    }
  },
  Declaration: {
    inset (decl) {
      if (expanded.has(decl)) return
      expanded.add(decl)
      const v = decl.value.trim().split(/\s+/)
      const [t, r = t, b = t, l = r] = v
      decl.cloneBefore({ prop: 'top', value: t })
      decl.cloneBefore({ prop: 'right', value: r })
      decl.cloneBefore({ prop: 'bottom', value: b })
      decl.cloneBefore({ prop: 'left', value: l })
    },
  },
})
oldWebKit.postcss = true

export default {
  plugins: [
    tailwindcss({ config: path.join(here, 'tailwind.config.js') }),
    oldWebKit(),
    autoprefixer(),
  ],
}
