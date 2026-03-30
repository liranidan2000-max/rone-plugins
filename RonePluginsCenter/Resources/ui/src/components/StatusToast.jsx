import React from 'react'
import { motion, AnimatePresence } from 'framer-motion'

const TOAST_STYLES = {
  info: 'bg-rone-button/90 border-rone-purple/30',
  success: 'bg-rone-green/10 border-rone-green/30',
  error: 'bg-rone-error/10 border-rone-error/30',
}

const TOAST_TEXT = {
  info: 'text-rone-text-primary',
  success: 'text-rone-green',
  error: 'text-rone-error',
}

function ToastIcon({ type }) {
  if (type === 'success') {
    return (
      <svg className="w-4 h-4 text-rone-green flex-shrink-0" fill="none" stroke="currentColor" viewBox="0 0 24 24">
        <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2.5} d="M5 13l4 4L19 7" />
      </svg>
    )
  }
  if (type === 'error') {
    return (
      <svg className="w-4 h-4 text-rone-error flex-shrink-0" fill="none" stroke="currentColor" viewBox="0 0 24 24">
        <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2}
          d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-2.5L13.732 4c-.77-.833-1.964-.833-2.732 0L4.082 16.5c-.77.833.192 2.5 1.732 2.5z" />
      </svg>
    )
  }
  return (
    <svg className="w-4 h-4 text-rone-purple flex-shrink-0" fill="none" stroke="currentColor" viewBox="0 0 24 24">
      <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2}
        d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
    </svg>
  )
}

export default function StatusToast({ toasts, onRemove }) {
  return (
    <div className="fixed bottom-4 right-4 z-50 flex flex-col gap-2 max-w-sm">
      <AnimatePresence>
        {toasts.map(toast => (
          <motion.div
            key={toast.id}
            initial={{ opacity: 0, y: 20, scale: 0.95 }}
            animate={{ opacity: 1, y: 0, scale: 1 }}
            exit={{ opacity: 0, x: 50, scale: 0.95 }}
            transition={{ duration: 0.3, ease: 'easeOut' }}
            className={`flex items-center gap-2 px-4 py-2.5 rounded-xl border
                       backdrop-blur-md shadow-lg
                       ${TOAST_STYLES[toast.type] || TOAST_STYLES.info}`}
          >
            <ToastIcon type={toast.type} />
            <span className={`text-xs font-medium flex-1 ${TOAST_TEXT[toast.type] || TOAST_TEXT.info}`}>
              {toast.text}
            </span>
            <button
              onClick={() => onRemove(toast.id)}
              className="text-rone-text-dim hover:text-rone-text-primary transition-colors ml-2"
            >
              <svg className="w-3 h-3" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" />
              </svg>
            </button>
          </motion.div>
        ))}
      </AnimatePresence>
    </div>
  )
}
