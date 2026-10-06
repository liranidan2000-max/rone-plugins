import path from 'node:path'
import { fileURLToPath } from 'node:url'

// Explicit config path: the dev server may be started from any folder, and
// Tailwind otherwise looks for its config (and the files to scan) from there.
const here = path.dirname(fileURLToPath(import.meta.url))

export default {
  plugins: {
    tailwindcss: { config: path.join(here, 'tailwind.config.js') },
    autoprefixer: {},
  },
}
