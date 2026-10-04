import React, { useEffect } from 'react'
import { motion, AnimatePresence } from 'framer-motion'

// Card menu > MANUAL: the two ways to learn a plugin - the PDF that ships with
// it, or its guide on YouTube. A side the plugin doesn't have yet stays on the
// screen switched off, and clicking it does nothing.
function Choice({ icon, title, sub, enabled, onPick }) {
  return (
    <button
      onClick={enabled ? onPick : undefined}
      aria-disabled={!enabled}
      className={`flex-1 rounded-xl border px-4 py-5 flex flex-col items-center gap-2.5 text-center transition-colors
        ${enabled
          ? 'border-rone-border bg-white/[0.02] hover:border-rone-purple/60 hover:bg-rone-purple/[0.06]'
          : 'border-rone-border/40 opacity-40 cursor-default'}`}
    >
      <span className={enabled ? 'text-rone-purple' : 'text-rone-text-dim'}>{icon}</span>
      <span className="text-[11px] font-extrabold uppercase tracking-[0.18em] text-rone-text-primary">{title}</span>
      <span className="text-[11px] text-rone-text-dim leading-snug">{sub}</span>
    </button>
  )
}

export default function ManualDialog({ plugin, onPdf, onVideo, onClose }) {
  useEffect(() => {
    if (!plugin) return
    const handleKey = (e) => { if (e.key === 'Escape') onClose() }
    window.addEventListener('keydown', handleKey)
    return () => window.removeEventListener('keydown', handleKey)
  }, [plugin, onClose])

  return (
    <AnimatePresence>
      {plugin && (
        <motion.div
          className="fixed inset-0 z-[60] flex items-center justify-center confirm-backdrop"
          role="dialog" aria-modal="true" aria-label={`${plugin.name} manual`}
          initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} transition={{ duration: 0.2 }}
          onClick={(e) => { if (e.target === e.currentTarget) onClose() }}
        >
          <motion.div
            className="surface-3 border border-rone-border/50 rounded-2xl p-5 w-[400px] max-w-[90vw] shadow-2xl"
            initial={{ opacity: 0, scale: 0.95, y: 10 }} animate={{ opacity: 1, scale: 1, y: 0 }}
            exit={{ opacity: 0, scale: 0.95, y: 10 }} transition={{ duration: 0.25, ease: 'easeOut' }}
          >
            <div className="flex items-start justify-between mb-4">
              <div>
                <h3 className="text-sm font-bold text-rone-text-primary">{plugin.name}</h3>
                <p className="text-xs text-rone-text-secondary mt-0.5">How do you want to learn it?</p>
              </div>
              <button onClick={onClose} aria-label="Close dialog"
                      className="p-1 -mt-1 -mr-1 text-rone-text-dim hover:text-rone-text-primary transition-colors rounded">
                <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" /></svg>
              </button>
            </div>

            <div className="flex gap-3">
              <Choice
                enabled={!!plugin.hasManual}
                onPick={() => onPdf(plugin)}
                title="PDF"
                sub={plugin.hasManual ? 'The user manual' : 'Coming soon'}
                icon={<svg className="w-7 h-7" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth={1.6} d="M14 3H7a2 2 0 00-2 2v14a2 2 0 002 2h10a2 2 0 002-2V8l-5-5zm0 0v5h5M9 13h6M9 17h4" /></svg>}
              />
              <Choice
                enabled={!!plugin.videoUrl}
                onPick={() => onVideo(plugin)}
                title="YouTube video"
                sub={plugin.videoUrl ? 'The complete guide' : 'Coming soon'}
                icon={<svg className="w-7 h-7" fill="none" stroke="currentColor" viewBox="0 0 24 24"><rect x="2.5" y="5.5" width="19" height="13" rx="3.5" strokeWidth={1.6} /><path fill="currentColor" stroke="none" d="M10 9.2v5.6l5-2.8-5-2.8z" /></svg>}
              />
            </div>
          </motion.div>
        </motion.div>
      )}
    </AnimatePresence>
  )
}
