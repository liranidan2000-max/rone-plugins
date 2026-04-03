import React, { useEffect } from 'react'
import { motion, AnimatePresence } from 'framer-motion'

export default function ConfirmDialog({ open, title, message, confirmLabel = 'Confirm', cancelLabel = 'Cancel', variant = 'danger', onConfirm, onCancel }) {
  useEffect(() => {
    if (!open) return
    const handleKey = (e) => {
      if (e.key === 'Escape') onCancel()
    }
    window.addEventListener('keydown', handleKey)
    return () => window.removeEventListener('keydown', handleKey)
  }, [open, onCancel])

  return (
    <AnimatePresence>
      {open && (
        <motion.div
          className="fixed inset-0 z-[60] flex items-center justify-center confirm-backdrop"
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          exit={{ opacity: 0 }}
          transition={{ duration: 0.2 }}
          onClick={(e) => { if (e.target === e.currentTarget) onCancel() }}
        >
          <motion.div
            className="surface-3 border border-rone-border/50 rounded-2xl p-5 w-[340px] max-w-[90vw] shadow-2xl"
            initial={{ opacity: 0, scale: 0.95, y: 10 }}
            animate={{ opacity: 1, scale: 1, y: 0 }}
            exit={{ opacity: 0, scale: 0.95, y: 10 }}
            transition={{ duration: 0.25, ease: 'easeOut' }}
          >
            <h3 className="text-sm font-bold text-rone-text-primary mb-2">{title}</h3>
            <p className="text-xs text-rone-text-secondary mb-5 leading-relaxed">{message}</p>

            <div className="flex items-center justify-end gap-2">
              <button
                onClick={onCancel}
                className="px-4 py-1.5 text-xs font-medium text-rone-text-secondary rounded-lg
                           border border-rone-border/40 hover:border-rone-border/60
                           hover:text-rone-text-primary transition-colors"
              >
                {cancelLabel}
              </button>
              <button
                onClick={onConfirm}
                className={`px-4 py-1.5 text-xs font-bold text-white rounded-lg transition-colors
                  ${variant === 'danger'
                    ? 'bg-rone-error hover:bg-rone-error/80'
                    : 'bg-rone-purple hover:brightness-110'
                  }`}
              >
                {confirmLabel}
              </button>
            </div>
          </motion.div>
        </motion.div>
      )}
    </AnimatePresence>
  )
}
