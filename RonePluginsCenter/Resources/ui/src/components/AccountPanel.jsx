import React, { useState } from 'react'
import { motion } from 'framer-motion'
import ConfirmDialog from './ConfirmDialog'

function initials(name) {
  if (!name) return 'U'
  const p = name.trim().split(/\s+/)
  return (p.length === 1 ? p[0].substring(0, 2) : p[0][0] + p[p.length - 1][0]).toUpperCase()
}

export default function AccountPanel({ license, onActivate, onDeactivate, pluginCount }) {
  const [keyInput, setKeyInput] = useState('')
  const [activating, setActivating] = useState(false)
  const [message, setMessage] = useState('')
  const [confirmOpen, setConfirmOpen] = useState(false)

  const handleActivate = async () => {
    if (!keyInput.trim()) return
    setActivating(true); setMessage('Activating…')
    try {
      const r = await onActivate(keyInput.trim())
      if (!r?.started) { setActivating(false); setMessage(r?.message || r?.error || 'Activation failed') }
    } catch (e) { setActivating(false); setMessage('Error: ' + (e.message || e)) }
  }

  React.useEffect(() => {
    if (activating && (license.licensed || license.message)) {
      setActivating(false)
      if (license.licensed) setKeyInput('')
      if (license.message) setMessage(license.message)
    }
  }, [license.licensed, license.message])

  return (
    <motion.div
      className="px-6 py-6 max-w-[760px]"
      initial={{ opacity: 0, y: 8 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}
    >
      <h2 className="text-[18px] font-bold text-rone-text-primary mb-4">Account</h2>

      {/* Profile card */}
      <div className="pro-card rounded-2xl p-5 flex items-center gap-4">
        <div className="w-14 h-14 rounded-full bg-gradient-to-br from-rone-deep-purple to-rone-violet flex items-center justify-center text-white text-base font-bold shadow-lg shadow-rone-purple/25">
          {initials(license.customerName)}
        </div>
        <div className="flex-1 min-w-0">
          <p className="text-[15px] font-bold text-rone-text-primary">{license.customerName || 'Guest'}</p>
          <div className="flex items-center gap-2 mt-1">
            <span className={`px-2 py-0.5 text-[10px] font-bold tracking-wider rounded ${license.licensed ? 'bg-rone-purple/15 text-rone-light-purple border border-rone-purple/25' : 'bg-rone-surface-3 text-rone-text-dim border border-rone-border'}`}>
              {license.licensed ? 'PRO' : 'FREE'}
            </span>
            <span className="text-[12px] text-rone-text-dim">
              {license.licensed ? 'Professional Plan · all plugins unlocked' : 'No active license'}
            </span>
          </div>
        </div>
        {license.licensed && (
          <button
            onClick={() => setConfirmOpen(true)}
            className="px-4 py-2 text-[12px] font-semibold text-rone-text-secondary border border-rone-border rounded-xl hover:text-rone-error hover:border-rone-error/40 transition-colors"
          >
            Deactivate
          </button>
        )}
      </div>

      {/* Stats */}
      <div className="grid grid-cols-2 gap-4 mt-4">
        <div className="pro-card rounded-2xl p-4">
          <p className="text-[11px] text-rone-text-dim uppercase tracking-wider">Plugins installed</p>
          <p className="text-[24px] font-bold text-rone-text-primary mt-1">{pluginCount}</p>
        </div>
        <div className="pro-card rounded-2xl p-4">
          <p className="text-[11px] text-rone-text-dim uppercase tracking-wider">License status</p>
          <p className={`text-[24px] font-bold mt-1 ${license.licensed ? 'text-rone-green' : 'text-rone-text-dim'}`}>
            {license.licensed ? 'Active' : 'Inactive'}
          </p>
        </div>
      </div>

      {/* Activation (when not licensed) */}
      {!license.licensed && (
        <div className="pro-card rounded-2xl p-5 mt-4">
          <p className="text-[13px] font-semibold text-rone-text-primary mb-2">Activate your license</p>
          <div className="flex items-center gap-3">
            <input
              type="text"
              placeholder="Enter your RONE Full Bundle license key…"
              value={keyInput}
              onChange={(e) => setKeyInput(e.target.value)}
              onKeyDown={(e) => e.key === 'Enter' && handleActivate()}
              className="flex-1 px-3 py-2 text-[12px] bg-rone-surface-2 border border-rone-border/60 rounded-xl text-rone-text-primary placeholder:text-rone-text-dim focus:outline-none focus:border-rone-purple/50"
            />
            <button
              onClick={handleActivate}
              disabled={activating || !keyInput.trim()}
              className="px-5 py-2 text-[12px] font-bold rounded-xl text-white btn-gradient disabled:opacity-50 disabled:cursor-not-allowed"
            >
              {activating ? 'Activating…' : 'Activate'}
            </button>
          </div>
          {(message || license.message) && (
            <p className="text-[11px] text-rone-text-dim mt-2">{message || license.message}</p>
          )}
        </div>
      )}

      <ConfirmDialog
        open={confirmOpen}
        title="Deactivate License"
        message="Are you sure you want to deactivate your license? You will need to re-enter your license key to use the plugins."
        confirmLabel="Deactivate" cancelLabel="Cancel" variant="danger"
        onConfirm={() => { setConfirmOpen(false); onDeactivate() }}
        onCancel={() => setConfirmOpen(false)}
      />
    </motion.div>
  )
}
