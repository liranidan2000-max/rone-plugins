import React, { useState } from 'react'
import { motion, AnimatePresence } from 'framer-motion'

export default function LicenseBar({ license, onActivate, onDeactivate, unlockPlaying = false }) {
  const [keyInput, setKeyInput] = useState('')
  const [activating, setActivating] = useState(false)
  const [message, setMessage] = useState('')

  const handleActivate = async () => {
    if (!keyInput.trim()) return
    setActivating(true)
    setMessage('Activating... please wait')
    try {
      const result = await onActivate(keyInput.trim())
      console.log('[LicenseBar] Activate result:', JSON.stringify(result))
      if (!result?.started) {
        setActivating(false)
        setMessage(result?.message || result?.error || 'Failed to start activation')
      }
    } catch (err) {
      console.error('[LicenseBar] Activate error:', err)
      setActivating(false)
      setMessage('Error: ' + (err.message || String(err)))
    }
  }

  // When license state changes (from event), stop the activating spinner
  React.useEffect(() => {
    if (activating && (license.licensed || license.message)) {
      setActivating(false)
      if (license.licensed) setKeyInput('')
      if (license.message) setMessage(license.message)
    }
  }, [license.licensed, license.message])

  const handleDeactivate = async () => {
    const result = await onDeactivate()
    if (result?.message) setMessage(result.message)
  }

  const handleKeyDown = (e) => {
    if (e.key === 'Enter') handleActivate()
  }

  return (
    <AnimatePresence mode="wait">
      {license.licensed ? (
        <motion.div
          key="licensed"
          initial={{ opacity: 0, y: -10 }}
          animate={{ opacity: 1, y: 0 }}
          exit={{ opacity: 0, y: 10 }}
          transition={{ duration: 0.3, ease: 'easeOut' }}
          className="flex-shrink-0 h-11 bg-rone-license border-b border-rone-border/30 flex items-center px-5 gap-3"
        >
          {/* Avatar circle */}
          <div className="w-7 h-7 rounded-full bg-gradient-to-br from-rone-purple to-rone-pink flex items-center justify-center text-white text-xs font-bold">
            {(license.customerName || 'U').charAt(0).toUpperCase()}
          </div>

          {/* Licensed to */}
          <span className="text-rone-text-secondary text-xs">
            Licensed to{' '}
            <span className="text-rone-text-primary font-medium">
              {license.customerName || 'User'}
            </span>
          </span>

          {/* PRO badge with bounce on unlock */}
          <motion.span
            className="px-2 py-0.5 text-[10px] font-bold tracking-wider rounded bg-rone-pink/15 text-rone-pink border border-rone-pink/20"
            animate={unlockPlaying
              ? { scale: [0, 1.3, 1] }
              : { scale: 1 }}
            transition={unlockPlaying
              ? { type: 'spring', damping: 8, stiffness: 120, delay: 0.3 }
              : { duration: 0.2 }}
          >
            PRO
          </motion.span>

          <div className="flex-1" />

          {/* Deactivate */}
          <button
            onClick={handleDeactivate}
            className="text-xs text-rone-text-dim hover:text-rone-error transition-colors px-3 py-1
                       border border-rone-border/30 rounded-md hover:border-rone-error/30"
          >
            Deactivate
          </button>
        </motion.div>
      ) : (
        <motion.div
          key="unlicensed"
          initial={{ opacity: 0, y: -10 }}
          animate={{ opacity: 1, y: 0 }}
          exit={{ opacity: 0, y: 10 }}
          transition={{ duration: 0.3, ease: 'easeOut' }}
          className="flex-shrink-0 bg-rone-license border-b border-rone-border/30 px-5 py-2.5"
        >
          <div className="flex items-center gap-3">
            <input
              type="text"
              placeholder="Enter your RONE Full Bundle license key..."
              value={keyInput}
              onChange={(e) => setKeyInput(e.target.value)}
              onKeyDown={handleKeyDown}
              className="flex-1 px-3 py-1.5 text-xs bg-rone-bg/60 border border-rone-border/50 rounded-lg
                         text-rone-text-primary placeholder:text-rone-text-dim
                         focus:outline-none focus:border-rone-purple/50 transition-colors"
            />
            <motion.button
              onClick={handleActivate}
              disabled={activating || !keyInput.trim()}
              className="px-4 py-1.5 text-xs font-bold rounded-lg
                         bg-rone-purple text-white
                         hover:brightness-110
                         disabled:opacity-50 disabled:cursor-not-allowed
                         transition-colors duration-200"
              whileTap={{ scale: 0.95 }}
            >
              {activating ? 'ACTIVATING...' : 'ACTIVATE'}
            </motion.button>
          </div>
          {(message || license.message) && (
            <p className="text-xs text-rone-text-dim mt-1.5">
              {message || license.message}
            </p>
          )}
        </motion.div>
      )}
    </AnimatePresence>
  )
}
