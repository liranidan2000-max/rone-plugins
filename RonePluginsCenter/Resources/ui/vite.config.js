import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const here = path.dirname(fileURLToPath(import.meta.url))
const resources = path.resolve(here, '..')

// The dev preview (npm run dev) serves the plugin icons and 3D units the way
// the Center does (MainComponent::getResource): /logos/<id>.png and
// /units/<id>.webp, straight from Resources/. Nothing is copied into dist.
function centerResources () {
  return {
    name: 'center-resources',
    configureServer (server) {
      server.middlewares.use((req, res, next) => {
        const m = /^\/(logos|units)\/([A-Za-z0-9_]+)\.(png|webp)$/.exec((req.url || '').split('?')[0])
        if (!m) return next()
        const file = m[1] === 'logos' ? path.join(resources, `${m[2]}_icon.png`)
                                      : path.join(resources, 'units', `unit_${m[2]}.webp`)
        if (!fs.existsSync(file)) { res.statusCode = 404; return res.end() }
        res.setHeader('Content-Type', m[3] === 'png' ? 'image/png' : 'image/webp')
        fs.createReadStream(file).pipe(res)
      })
    },
  }
}

export default defineConfig({
  root: here,
  plugins: [react(), centerResources()],
  css: { postcss: path.join(here, 'postcss.config.js') },
  build: {
    // The Mac runs this page in the system WebKit - on macOS 10.15 that can be
    // Safari 13 - so syntax and CSS are lowered that far (postcss.config.js too).
    target: ['es2019', 'chrome100', 'safari13'],
    cssTarget: ['chrome100', 'safari13'],
    outDir: 'dist',
    emptyOutDir: true,
    assetsDir: '.',
    // Fonts stay files (the Center serves them by name); nothing is inlined.
    assetsInlineLimit: 0,
    rollupOptions: {
      output: {
        inlineDynamicImports: true,
        entryFileNames: 'bundle.js',
        assetFileNames: (assetInfo) => {
          if (assetInfo.name && assetInfo.name.endsWith('.css')) {
            return 'styles.css'
          }
          return '[name][extname]'
        },
      },
    },
    cssCodeSplit: false,
  },
})
