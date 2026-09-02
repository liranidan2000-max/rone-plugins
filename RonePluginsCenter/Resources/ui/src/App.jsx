import React, { useState, useEffect, useCallback, useRef } from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import { api, onEvent, isDevMode, mockPlugins } from './bridge'
import Sidebar from './components/Sidebar'
import TopBar from './components/TopBar'
import FeaturedSection from './components/FeaturedSection'
import FilterBar from './components/FilterBar'
import PluginGrid from './components/PluginGrid'
import AccountPanel from './components/AccountPanel'
import SettingsPanel from './components/SettingsPanel'
import InfoModal from './components/InfoModal'
import StatusToast from './components/StatusToast'

export default function App() {
  const [plugins, setPlugins] = useState([])
  const [license, setLicense] = useState({ licensed: false, customerName: '', licenseKey: '', message: '' })
  const [account, setAccount] = useState({ signedIn: false, licensed: false, email: '', name: '', plan: 'none', deviceLimit: 2, message: '' })
  const [searchQuery, setSearchQuery] = useState('')
  const [statusFilter, setStatusFilter] = useState('all')
  const [sortBy, setSortBy] = useState('name')
  const [toasts, setToasts] = useState([])
  const [infoPlugin, setInfoPlugin] = useState(null)
  const [loading, setLoading] = useState(true)
  const [lastSync, setLastSync] = useState(null)
  const [activeNav, setActiveNav] = useState('home')
  const [centerUpdate, setCenterUpdate] = useState(null)   // version string, or null

  // ---- Center self-update ----
  // Auto-applies once per version: download -> verify -> silent install ->
  // relaunch (one UAC prompt). If that attempt didn't stick (UAC declined,
  // download failed), the banner stays and updating becomes a manual click -
  // never an every-launch UAC loop.
  const centerUpdateSeen = useRef(null)
  const addToastRef = useRef(null)
  const onCenterUpdate = useCallback((version) => {
    if (!version || centerUpdateSeen.current === version) return
    centerUpdateSeen.current = version
    setCenterUpdate(version)

    const triedKey = 'centerAutoTried:' + version
    let alreadyTried = false
    try { alreadyTried = localStorage.getItem(triedKey) === '1' } catch {}
    if (alreadyTried) return

    try { localStorage.setItem(triedKey, '1') } catch {}
    addToastRef.current?.(`Center v${version} is available - updating…`, 'info')
    setTimeout(() => api.applyCenterUpdate(), 2200)
  }, [])

  // ---- Unlock animation orchestration ----
  const [unlockPlaying, setUnlockPlaying] = useState(false)
  const prevLicensed = useRef(license.licensed)
  useEffect(() => {
    if (!prevLicensed.current && license.licensed) {
      setUnlockPlaying(true)
      const timer = setTimeout(() => setUnlockPlaying(false), 1500)
      return () => clearTimeout(timer)
    }
    prevLicensed.current = license.licensed
  }, [license.licensed])

  const addToast = useCallback((text, type = 'info') => {
    const id = Date.now() + Math.random()
    setToasts(prev => [...prev, { id, text, type }])
    setTimeout(() => setToasts(prev => prev.filter(t => t.id !== id)), 4000)
  }, [])
  addToastRef.current = addToast
  const removeToast = useCallback((id) => setToasts(prev => prev.filter(t => t.id !== id)), [])

  // ---- Load initial data ----
  useEffect(() => {
    async function init() {
      if (isDevMode()) {
        setPlugins(mockPlugins)
        // ?signedout=1 previews the sign-in form without a running backend
        const devSignedOut = new URLSearchParams(location.search).has('signedout')
        setLicense({ licensed: !devSignedOut, customerName: devSignedOut ? '' : 'Liran Kalifa',
                     licenseKey: devSignedOut ? '' : 'dev-key', message: '' })
        setAccount(devSignedOut
          ? { signedIn: false, licensed: false, email: '', name: '', plan: 'none', deviceLimit: 2, message: '' }
          : { signedIn: true, licensed: true, email: 'liran@roneaudio.com',
              name: 'Liran Kalifa', plan: 'all-access', deviceLimit: 2,
              renewsAt: Date.now() + 21 * 86400000, expiresAt: 0, message: '' })
        setLoading(false); setLastSync(new Date()); return
      }
      const licStatus = await api.getLicenseStatus()
      if (licStatus) setLicense(licStatus)
      const acct = await api.getAccountStatus()
      if (acct) setAccount(acct)
      const result = await api.getPlugins()
      if (result?.plugins) { setPlugins(result.plugins); setLastSync(new Date()) }
      if (result?.centerUpdate?.version) onCenterUpdate(result.centerUpdate.version)
      setLoading(false)
    }
    init()
  }, [])

  // ---- Subscribe to C++ events ----
  useEffect(() => {
    if (isDevMode()) return
    onEvent('pluginsUpdated', (data) => {
      if (data?.plugins) { setPlugins(data.plugins); setLastSync(new Date()) }
    })
    onEvent('downloadProgress', (data) => {
      if (!data?.pluginId) return
      setPlugins(prev => prev.map(p => p.id === data.pluginId ? { ...p, downloadProgress: data.progress, status: 'downloading' } : p))
    })
    onEvent('downloadComplete', () => {})
    onEvent('licenseChanged', (data) => { if (data) setLicense(prev => ({ ...prev, ...data })) })
    onEvent('accountChanged', (data) => { if (data) setAccount(data) })
    onEvent('licenseActivationResult', (data) => {
      if (data) {
        setLicense(prev => ({ ...prev, licensed: data.success || false, customerName: data.customerName || prev.customerName, message: data.message || '' }))
        if (!data.success) addToast(data.message || 'Activation failed', 'error')
      }
    })
    onEvent('licenseDeactivationResult', (data) => {
      if (data?.success) setLicense({ licensed: false, customerName: '', licenseKey: '', message: data.message || '' })
    })
    onEvent('statusMessage', (data) => { if (data?.text) addToast(data.text, data.type || 'info') })
    onEvent('centerUpdateAvailable', (data) => { if (data?.version) onCenterUpdate(data.version) })
  }, [addToast])

  // ---- Actions ----
  const handleInstall = async (pluginId) => {
    try {
      const result = await api.installPlugin(pluginId)
      if (result && !result.started && result.error) addToast(result.error, 'error')
    } catch (err) { addToast(err.message || 'Install failed', 'error') }
  }
  const handleOpen = async (pluginId) => {
    try {
      const result = await api.openPlugin(pluginId)
      if (result && !result.success && result.error) addToast(result.error, 'error')
    } catch (err) { addToast(err.message || 'Could not open plugin', 'error') }
  }
  const handleOpenFolder = async (pluginId) => {
    try {
      const result = await api.openFolder(pluginId)
      if (result && !result.success && result.error) addToast(result.error, 'error')
    } catch (err) { addToast(err.message || 'Could not open the folder', 'error') }
  }
  const handleManual = async (pluginId) => {
    try {
      const result = await api.openManual(pluginId)
      if (result && !result.success && result.error) addToast(result.error, 'error')
      else if (result?.source === 'online') addToast('Opening the manual from roneaudio.com', 'info')
    } catch (err) { addToast(err.message || 'Could not open the manual', 'error') }
  }
  const handleRefresh = async () => {
    try { addToast('Checking for updates…', 'info'); await api.refreshPlugins() }
    catch (err) { addToast(err.message || 'Refresh failed', 'error') }
  }
  const handleUpdateAll = async () => {
    const updatable = plugins.filter(p => p.status === 'update_available' || p.status === 'not_installed')
    if (updatable.length === 0) return
    addToast(`Updating ${updatable.length} plugin${updatable.length !== 1 ? 's' : ''}…`, 'info')
    for (const plugin of updatable) await handleInstall(plugin.id)
  }
  // Google: the native side opens the browser and resolves when it comes back (or fails / is cancelled)
  const handleGoogleSignIn = async () => {
    try {
      const res = await api.accountGoogleSignIn()
      if (res?.account) setAccount(res.account)
      if (res?.ok) {
        setLicense(prev => ({ ...prev, licensed: !!res.account?.licensed,
                              customerName: res.account?.name || res.account?.email || prev.customerName }))
        addToast(res.message || 'Signed in with Google', 'success')
      }
      return res
    } catch (err) {
      return { ok: false, message: err.message || 'Google sign-in failed' }
    }
  }
  const handleGoogleCancel = () => api.accountGoogleCancel().catch(() => {})
  const handleSignIn = async (email, password) => {
    try {
      const res = await api.accountSignIn(email, password)
      if (res?.account) setAccount(res.account)
      if (res?.ok) {
        setLicense(prev => ({ ...prev, licensed: !!res.account?.licensed,
                              customerName: res.account?.name || res.account?.email || prev.customerName }))
        addToast(res.message || 'Signed in', 'success')
      }
      return res
    } catch (err) {
      addToast(err.message || 'Sign-in failed', 'error')
      return { ok: false, message: err.message }
    }
  }
  const handleSignOut = async () => {
    try {
      const res = await api.accountSignOut()
      setAccount({ signedIn: false, licensed: false, email: '', name: '', plan: 'none', message: '' })
      setLicense(prev => ({ ...prev, licensed: false, customerName: '' }))
      addToast(res?.message || 'Signed out', 'info')
      return res
    } catch (err) {
      addToast(err.message || 'Sign-out failed', 'error')
      return { ok: false }
    }
  }
  const handleActivate = async (key) => {
    try { return await api.activateLicense(key) }
    catch (err) { addToast(err.message || 'Activation failed', 'error'); return { success: false, message: err.message } }
  }
  const handleDeactivate = async () => {
    try { return await api.deactivateLicense() }
    catch (err) { addToast(err.message || 'Deactivation failed', 'error'); return { success: false, message: err.message } }
  }

  const handleNavigate = (key) => {
    setActiveNav(key)
    if (key === 'updates') setStatusFilter('updates')
    else if (key === 'plugins' || key === 'home') setStatusFilter('all')
  }

  // ---- Filtered & sorted plugins ----
  const processedPlugins = React.useMemo(() => {
    let result = [...plugins]
    if (searchQuery) {
      const q = searchQuery.toLowerCase()
      result = result.filter(p => p.name.toLowerCase().includes(q) || p.description.toLowerCase().includes(q))
    }
    if (statusFilter === 'installed') result = result.filter(p => p.status === 'up_to_date' || p.status === 'update_available')
    else if (statusFilter === 'updates') result = result.filter(p => p.status === 'update_available' || p.status === 'not_installed')
    else if (statusFilter === 'not_installed') result = result.filter(p => p.status === 'not_installed')

    if (sortBy === 'name') result.sort((a, b) => a.name.localeCompare(b.name))
    else if (sortBy === 'status') {
      const order = { update_available: 0, not_installed: 1, downloading: 2, installing: 3, error: 4, up_to_date: 5 }
      result.sort((a, b) => (order[a.status] ?? 99) - (order[b.status] ?? 99))
    }
    return result
  }, [plugins, searchQuery, statusFilter, sortBy])

  const filterCounts = React.useMemo(() => ({
    all: plugins.length,
    installed: plugins.filter(p => p.status === 'up_to_date' || p.status === 'update_available').length,
    updates: plugins.filter(p => p.status === 'update_available').length,
    not_installed: plugins.filter(p => p.status === 'not_installed').length,
  }), [plugins])

  const updatesCount = plugins.filter(p => p.status === 'update_available' || p.status === 'not_installed').length

  // ---- Refresh lastSync display every minute ----
  const [, setTick] = useState(0)
  useEffect(() => {
    const interval = setInterval(() => setTick(t => t + 1), 60000)
    return () => clearInterval(interval)
  }, [])

  const showHomeView = activeNav === 'home' || activeNav === 'plugins' || activeNav === 'updates'

  return (
    <motion.div
      className="h-screen flex flex-col bg-rone-bg overflow-hidden"
      initial={{ opacity: 0 }} animate={{ opacity: 1 }} transition={{ duration: 0.4, ease: 'easeOut' }}
    >
      {/* Unlock shimmer */}
      <AnimatePresence>
        {unlockPlaying && (
          <motion.div key="shimmer" className="fixed inset-0 z-50 pointer-events-none"
            initial={{ x: '-100%' }} animate={{ x: '100%' }} exit={{ opacity: 0 }}
            transition={{ duration: 0.8, ease: 'easeInOut' }}
            style={{ background: 'linear-gradient(90deg, transparent, rgba(157,107,255,0.14), rgba(157,107,255,0.08), transparent)', width: '100%' }} />
        )}
      </AnimatePresence>

      {/* Center self-update strip (kept when the auto attempt didn't stick) */}
      {centerUpdate && (
        <div className="flex-shrink-0 flex items-center gap-3 px-6 py-2 bg-rone-purple/[0.07] border-b border-rone-purple/25">
          <span className="w-[7px] h-[7px] rounded-full led-upd status-dot-pulse" />
          <span className="text-[10px] font-extrabold uppercase tracking-[0.16em] text-rone-text-secondary">
            Center v{centerUpdate} is ready
          </span>
          <div className="flex-1" />
          <button
            onClick={() => api.applyCenterUpdate()}
            className="btn-gradient px-4 py-1.5 rounded-lg text-[10px] font-extrabold uppercase tracking-[0.18em]"
          >
            Restart &amp; Update
          </button>
        </div>
      )}

      <div className="flex-1 min-h-0 flex">

      {/* Sidebar */}
      <Sidebar active={activeNav} onNavigate={handleNavigate} updatesCount={updatesCount} license={license} />

      {/* Main column */}
      <main className="flex-1 flex flex-col min-w-0">
        <TopBar
          license={license}
          searchQuery={searchQuery}
          onSearchChange={setSearchQuery}
          onRefresh={handleRefresh}
          lastSync={lastSync}
          onSettings={() => setActiveNav('settings')}
        />

        <div className="flex-1 flex flex-col min-h-0">
          {activeNav === 'account' && (
            <div className="flex-1 overflow-y-auto plugin-grid-scroll">
              <AccountPanel license={license} account={account} onSignIn={handleSignIn} onSignOut={handleSignOut}
                            onGoogleSignIn={handleGoogleSignIn} onGoogleCancel={handleGoogleCancel}
                            onActivate={handleActivate} onDeactivate={handleDeactivate} pluginCount={filterCounts.installed} />
            </div>
          )}

          {activeNav === 'settings' && (
            <div className="flex-1 overflow-y-auto plugin-grid-scroll">
              <SettingsPanel onRefresh={handleRefresh} lastSync={lastSync} />
            </div>
          )}

          {showHomeView && (
            <>
              {(activeNav === 'home' || activeNav === 'updates') && !loading && (
                <FeaturedSection
                  plugins={plugins}
                  onUpdateAll={handleUpdateAll}
                  onRefresh={handleRefresh}
                  licensed={license.licensed}
                  signedIn={!!account.signedIn}
                  onSignIn={() => setActiveNav('account')}
                />
              )}

              {!loading && plugins.length > 0 && activeNav !== 'updates' && (
                <FilterBar
                  statusFilter={statusFilter}
                  onStatusFilterChange={setStatusFilter}
                  sortBy={sortBy}
                  onSortChange={setSortBy}
                  counts={filterCounts}
                />
              )}

              <div className="flex-1 min-h-0">
                <PluginGrid
                  plugins={processedPlugins}
                  licensed={license.licensed}
                  onInstall={handleInstall}
                  onOpen={handleOpen}
                  onOpenFolder={handleOpenFolder}
                  onManual={handleManual}
                  onInfo={setInfoPlugin}
                  unlockPlaying={unlockPlaying}
                  loading={loading}
                />
              </div>
            </>
          )}
        </div>
      </main>

      </div>

      {/* Info Modal */}
      <AnimatePresence>
        {infoPlugin && <InfoModal plugin={infoPlugin} onClose={() => setInfoPlugin(null)} />}
      </AnimatePresence>

      {/* Toasts */}
      <StatusToast toasts={toasts} onRemove={removeToast} />
    </motion.div>
  )
}
