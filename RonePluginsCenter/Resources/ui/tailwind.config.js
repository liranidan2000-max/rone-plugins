/** @type {import('tailwindcss').Config} */
// Colours are CSS variables (index.css) so Settings > High contrast can lift
// every label and border at once. Values are "R G B" so opacity modifiers
// (bg-rone-purple/10) keep working.
import { fileURLToPath } from 'node:url'
import path from 'node:path'

const v = (name) => `rgb(var(--${name}) / <alpha-value>)`

// Absolute: the dev server can be started from another folder (.claude/launch.json),
// and relative globs would then match nothing - the page rendered unstyled.
const here = path.dirname(fileURLToPath(import.meta.url)).replace(/\\/g, '/')

export default {
  content: [
    `${here}/index.html`,
    `${here}/src/**/*.{js,jsx}`,
  ],
  theme: {
    extend: {
      colors: {
        rone: {
          // --- RONE graphite surfaces ---
          bg:           v('ground'),
          panel:        v('panel'),
          card:         v('card'),
          'card-hover': v('card-hover'),
          drawer:       v('drawer'),
          border:       v('line'),
          'border-2':   v('line2'),
          'border-3':   v('line3'),

          // --- Accent: the Center's neon is purple; each plugin card uses its own (style --acc) ---
          purple:       v('purple'),
          'light-purple': v('purple-light'),
          'neon-dark':  v('neon-dark'),

          // --- Semantic (separate from the accent) ---
          green:        v('ok'),
          error:        v('err'),
          amber:        v('amber'),
          cyan:         v('cyan'),

          // --- Text: dim and faint are lighter than before 2.0 (faint was 2.3:1 on the cards) ---
          'text-primary':   v('tx'),
          'text-secondary': v('tx2'),
          'text-dim':       v('tx3'),
          'text-faint':     v('tx4'),
        }
      },
      fontFamily: {
        sans: ['Manrope', 'Segoe UI', 'Roboto', 'sans-serif'],
        display: ['Sora', 'Segoe UI', 'sans-serif'],
        mono: ['"IBM Plex Mono"', 'Consolas', 'monospace'],
      },
    },
  },
  plugins: [],
}
