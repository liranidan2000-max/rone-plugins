import React, { useState } from 'react'
import { motion } from 'framer-motion'

function Toggle({ on, onChange }) {
  return (
    <button
      onClick={() => onChange(!on)}
      className={`w-10 h-6 rounded-full transition-colors relative ${on ? 'bg-rone-deep-purple' : 'bg-rone-surface-3'}`}
    >
      <span className={`absolute top-0.5 w-5 h-5 rounded-full bg-white transition-all ${on ? 'left-[18px]' : 'left-0.5'}`} />
    </button>
  )
}

function Row({ title, desc, children }) {
  return (
    <div className="flex items-center gap-4 py-3.5 border-b border-rone-border/40 last:border-0">
      <div className="flex-1 min-w-0">
        <p className="text-[13px] font-semibold text-rone-text-primary">{title}</p>
        {desc && <p className="text-[11px] text-rone-text-dim mt-0.5">{desc}</p>}
      </div>
      {children}
    </div>
  )
}

export default function SettingsPanel({ onRefresh, lastSync }) {
  const [autoCheck, setAutoCheck] = useState(true)
  const [notify, setNotify] = useState(true)
  const [beta, setBeta] = useState(false)

  return (
    <motion.div
      className="px-6 py-6 max-w-[760px]"
      initial={{ opacity: 0, y: 8 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}
    >
      <h2 className="text-[18px] font-bold text-rone-text-primary mb-4">Settings</h2>

      <div className="pro-card rounded-2xl px-5 py-2">
        <Row title="Automatic update checks" desc="Check for new plugin versions on launch">
          <Toggle on={autoCheck} onChange={setAutoCheck} />
        </Row>
        <Row title="Update notifications" desc="Show a banner when updates are available">
          <Toggle on={notify} onChange={setNotify} />
        </Row>
        <Row title="Beta channel" desc="Receive pre-release builds for testing">
          <Toggle on={beta} onChange={setBeta} />
        </Row>
        <Row title="Check for updates now" desc="Manually re-sync with the RONE update server">
          <button onClick={onRefresh} className="px-4 py-1.5 text-[12px] font-semibold text-white rounded-xl btn-gradient">
            Check now
          </button>
        </Row>
      </div>

      <div className="pro-card rounded-2xl p-5 mt-4">
        <p className="text-[13px] font-semibold text-rone-text-primary">About</p>
        <div className="mt-3 space-y-2 text-[12px]">
          <div className="flex justify-between"><span className="text-rone-text-dim">Application</span><span className="text-rone-text-secondary">RONE Plugins Center</span></div>
          <div className="flex justify-between"><span className="text-rone-text-dim">Version</span><span className="text-rone-text-secondary">1.0.0</span></div>
          <div className="flex justify-between"><span className="text-rone-text-dim">Last synced</span><span className="text-rone-text-secondary">{lastSync ? lastSync.toLocaleString() : '—'}</span></div>
        </div>
      </div>
    </motion.div>
  )
}
