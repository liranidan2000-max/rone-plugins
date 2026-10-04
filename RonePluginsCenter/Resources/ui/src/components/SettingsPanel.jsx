import React, { useState, useEffect } from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import { api, isDevMode, mockOldVersions } from '../bridge'
import BackPanel from './BackPanel'

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

const KIND_LABEL = { manifest: 'Stale file', duplicate: 'Duplicate', legacy: 'Old name', leftover: 'Leftover' }

// Settings > DELETE OLD VERSIONS: shows what it found, deletes on confirm, then says
// what happened. The native side (OldVersionCleaner.h) never deletes the only copy of a
// plugin; anything a DAW still has loaded is skipped and named.
function OldVersionsDialog({ phase, items, result, onConfirm, onClose }) {
  const shown = result?.items || items
  const busy = phase === 'deleting'
  return (
    <AnimatePresence>
      {phase !== 'idle' && phase !== 'scanning' && (
        <motion.div
          className="fixed inset-0 z-[60] flex items-center justify-center confirm-backdrop"
          initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} transition={{ duration: 0.2 }}
          onClick={(e) => { if (e.target === e.currentTarget && !busy) onClose() }}
        >
          <motion.div
            className="surface-3 border border-rone-border/50 rounded-2xl p-5 w-[520px] max-w-[92vw] shadow-2xl"
            initial={{ opacity: 0, scale: 0.95, y: 10 }} animate={{ opacity: 1, scale: 1, y: 0 }}
            exit={{ opacity: 0, scale: 0.95, y: 10 }} transition={{ duration: 0.25, ease: 'easeOut' }}
          >
            <h3 className="text-sm font-bold text-rone-text-primary mb-1">
              {phase === 'done' ? 'Old versions' : `Delete ${items.length} old ${items.length === 1 ? 'file' : 'files'}?`}
            </h3>
            {phase === 'done' ? (
              <p className="text-xs text-rone-text-secondary mb-3 leading-relaxed">
                Deleted {result?.deleted ?? 0}.
                {result?.inUse > 0 && ` ${result.inUse} ${result.inUse === 1 ? 'is' : 'are'} in use - close the program named below and run this again.`}
                {result?.failed > 0 && (result?.elevationDeclined
                  ? ` ${result.failed} need${result.failed === 1 ? 's' : ''} permission, which was not given.`
                  : ` ${result.failed} could not be deleted.`)}
                {(result?.deleted ?? 0) > 0 && ' Rescan the plugins in your DAW so it forgets the old copies (FL Studio: Options > Manage plugins > Start scan).'}
              </p>
            ) : (
              <p className="text-xs text-rone-text-secondary mb-3 leading-relaxed">
                {busy ? 'Deleting... Windows may ask for permission.'
                      : 'These are left over from earlier RONE versions and can make your DAW open the wrong one. Your current plugins stay.'}
              </p>
            )}

            <div className="max-h-[260px] overflow-y-auto rounded-lg border border-rone-border/40 divide-y divide-rone-border/30">
              {shown.map((it, i) => (
                <div key={i} className="px-3 py-2">
                  <div className="flex items-center gap-2">
                    <span className="text-[9px] font-extrabold uppercase tracking-[0.14em] text-rone-text-dim shrink-0">{KIND_LABEL[it.kind] || it.kind}</span>
                    <span className="text-[12px] font-semibold text-rone-text-primary truncate">{it.name}</span>
                    {it.status && (
                      <span className={`ml-auto text-[10px] font-bold shrink-0 ${it.status === 'deleted' ? 'text-rone-green' : 'text-rone-error'}`}>
                        {it.status === 'deleted' ? 'Deleted' : it.status === 'in_use' ? `In use${it.hosts ? ' by ' + it.hosts : ''}` : 'Not deleted'}
                      </span>
                    )}
                  </div>
                  <p className="text-[11px] text-rone-text-dim mt-0.5">{it.reason}</p>
                </div>
              ))}
            </div>

            <div className="flex items-center justify-end gap-2 mt-4">
              {phase === 'done' ? (
                <button onClick={onClose} className="px-4 py-1.5 text-xs font-bold rounded-lg btn-gradient">Close</button>
              ) : (
                <>
                  <button onClick={onClose} disabled={busy}
                          className="px-4 py-1.5 text-xs font-medium text-rone-text-secondary rounded-lg border border-rone-border/40 hover:border-rone-border/60 hover:text-rone-text-primary transition-colors disabled:opacity-40">
                    Cancel
                  </button>
                  <button onClick={onConfirm} disabled={busy}
                          className="px-4 py-1.5 text-xs font-bold rounded-lg transition-colors bg-rone-error text-white hover:bg-rone-error/80 disabled:opacity-60">
                    {busy ? 'Deleting...' : `Delete ${items.length}`}
                  </button>
                </>
              )}
            </div>
          </motion.div>
        </motion.div>
      )}
    </AnimatePresence>
  )
}

export default function SettingsPanel({ onRefresh, lastSync }) {
  const [autoCheck, setAutoCheck] = useState(true)
  const [notify, setNotify] = useState(true)
  const [beta, setBeta] = useState(false)
  const [showBackPanel, setShowBackPanel] = useState(false)

  // Start with Windows / Open at login - the OS entry is the source of truth
  const [autoStart, setAutoStart] = useState({ supported: false, enabled: false, platform: 'windows' })
  const [autoStartMsg, setAutoStartMsg] = useState('')
  useEffect(() => {
    if (isDevMode()) { setAutoStart({ supported: true, enabled: true, platform: 'windows' }); return }
    Promise.all([api.getAutoStart(), api.getAppVersion()])
      .then(([a, v]) => setAutoStart({ supported: !!a?.supported, enabled: !!a?.enabled, platform: v?.platform || 'windows' }))
      .catch(() => {})
  }, [])
  const toggleAutoStart = async (on) => {
    setAutoStart(s => ({ ...s, enabled: on }))
    if (isDevMode()) return
    try {
      const r = await api.setAutoStart(on)
      setAutoStart(s => ({ ...s, enabled: !!r?.enabled }))
      setAutoStartMsg(r?.success === false ? (r.error || 'Could not change the login entry') : '')
    } catch (e) { setAutoStartMsg(e.message || 'Could not change the login entry') }
  }

  // OPEN FOLDER: where the installers put the plugins on this computer
  const [folderMsg, setFolderMsg] = useState('')
  const openInstallFolder = async (kind) => {
    setFolderMsg('')
    if (isDevMode()) return
    try { await api.openInstallFolder(kind) }
    catch (e) { setFolderMsg(e.message || 'Could not open the folder') }
  }
  const folderKinds = autoStart.platform === 'mac'
    ? [['vst3', 'VST3'], ['au', 'AU'], ['standalone', 'Standalone']]
    : [['vst3', 'VST3'], ['standalone', 'Standalone']]

  // DELETE OLD VERSIONS: idle -> scanning -> confirm -> deleting -> done
  const [cleanPhase, setCleanPhase] = useState('idle')
  const [cleanItems, setCleanItems] = useState([])
  const [cleanResult, setCleanResult] = useState(null)
  const [cleanMsg, setCleanMsg] = useState('')
  const startClean = async () => {
    setCleanMsg(''); setCleanResult(null); setCleanPhase('scanning')
    try {
      const r = isDevMode() ? { items: mockOldVersions } : await api.scanOldVersions()
      const items = r?.items || []
      setCleanItems(items)
      if (!items.length) { setCleanPhase('idle'); setCleanMsg('Nothing to delete - no old RONE files found'); return }
      setCleanPhase('confirm')
    } catch (e) { setCleanPhase('idle'); setCleanMsg(e.message || 'Could not look for old versions') }
  }
  const confirmClean = async () => {
    setCleanPhase('deleting')
    try {
      const r = isDevMode()
        ? { deleted: cleanItems.length, inUse: 0, failed: 0, items: cleanItems.map(it => ({ ...it, status: 'deleted' })) }
        : await api.deleteOldVersions()
      setCleanResult(r); setCleanPhase('done')
    } catch (e) { setCleanPhase('idle'); setCleanMsg(e.message || 'Could not delete the old versions') }
  }
  const closeClean = () => { setCleanPhase('idle'); setCleanResult(null) }

  // Real app version, straight from the running binary
  const [version, setVersion] = useState('')
  useEffect(() => {
    if (isDevMode()) { setVersion('dev'); return }
    api.getAppVersion()
      .then(info => setVersion(info?.version || ''))
      .catch(() => setVersion(''))
  }, [])

  return (
    <motion.div
      className="px-6 py-6 max-w-[760px]"
      initial={{ opacity: 0, y: 8 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}
    >
      <h2 className="font-display text-[18px] font-bold text-rone-text-primary mb-4">Settings</h2>

      <div className="pro-card rounded-2xl px-5 py-2">
        <Row title={autoStart.platform === 'mac' ? 'Open at login' : 'Start with Windows'}
             desc={(autoStart.platform === 'mac' ? 'The Center opens minimised in the menu bar when you log in' : 'The Center starts minimised in the tray when you log in') + ' - keeps your plugins licensed and updates checked without opening it' + (autoStartMsg ? ' - ' + autoStartMsg : '')}>
          <Toggle on={autoStart.enabled} onChange={toggleAutoStart} />
        </Row>
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
          <button onClick={onRefresh} className="px-4 py-1.5 text-[10px] font-extrabold uppercase tracking-[0.16em] rounded-lg btn-gradient">
            Check now
          </button>
        </Row>
        <Row title="Open folder"
             desc={'Opens the folder on this computer where your RONE plugins are installed' + (folderMsg ? ' - ' + folderMsg : '')}>
          <div className="flex gap-2">
            {folderKinds.map(([kind, label]) => (
              <button key={kind} onClick={() => openInstallFolder(kind)}
                      className="px-4 py-1.5 text-[10px] font-extrabold uppercase tracking-[0.16em] rounded-lg btn-outline whitespace-nowrap">
                {label}
              </button>
            ))}
          </div>
        </Row>
        <Row title="Delete old versions"
             desc={'Finds what earlier RONE versions left behind - duplicate copies, old names, stale plugin files - that can make your DAW open the wrong version, and deletes it. You see the list first' + (cleanMsg ? ' - ' + cleanMsg : '')}>
          <button onClick={startClean} disabled={cleanPhase === 'scanning' || cleanPhase === 'deleting'}
                  className="px-4 py-1.5 text-[10px] font-extrabold uppercase tracking-[0.16em] rounded-lg btn-gradient whitespace-nowrap disabled:opacity-60">
            {cleanPhase === 'scanning' ? 'Looking...' : 'Delete old versions'}
          </button>
        </Row>
      </div>

      <div
        className="pro-card rounded-2xl p-5 mt-4 cursor-pointer transition-colors hover:border-rone-purple/40 group"
        onClick={() => setShowBackPanel(true)}
        role="button"
        title="About — flip the unit around"
      >
        <div className="flex items-center justify-between">
          <p className="text-[13px] font-semibold text-rone-text-primary">About</p>
          <span className="text-[9px] font-extrabold uppercase tracking-[0.2em] text-rone-text-dim group-hover:text-rone-text-secondary transition-colors">
            View back panel &rarr;
          </span>
        </div>
        <div className="mt-3 space-y-2 text-[12px]">
          <div className="flex justify-between"><span className="text-rone-text-dim">Application</span><span className="text-rone-text-secondary">RONE Plugins Center</span></div>
          <div className="flex justify-between"><span className="text-rone-text-dim">Version</span><span className="text-rone-text-secondary tabular-nums">{version || '—'}</span></div>
          <div className="flex justify-between"><span className="text-rone-text-dim">Last synced</span><span className="text-rone-text-secondary">{lastSync ? lastSync.toLocaleString() : '—'}</span></div>
        </div>
      </div>

      <OldVersionsDialog phase={cleanPhase} items={cleanItems} result={cleanResult}
                         onConfirm={confirmClean} onClose={closeClean} />

      {showBackPanel && (
        <BackPanel version={version} onClose={() => setShowBackPanel(false)} />
      )}
    </motion.div>
  )
}
