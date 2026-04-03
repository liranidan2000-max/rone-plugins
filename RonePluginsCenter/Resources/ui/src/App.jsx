import React, { useState, useEffect, useCallback, useRef } from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import { api, onEvent, isDevMode, mockPlugins } from './bridge'
import Header from './components/Header'
import LicenseBar from './components/LicenseBar'
import FeaturedSection from './components/FeaturedSection'
import FilterBar from './components/FilterBar'
import PluginGrid from './components/PluginGrid'
import InfoModal from './components/InfoModal'
import StatusToast from './components/StatusToast'

export default function App() {
  const [plugins, setPlugins] = useState([])
  const [license, setLicense] = useState({
    licensed: false,
    customerName: '',
    licenseKey: '',
    message: '',
  })
  const [searchQuery, setSearchQuery] = useState('')
  const [statusFilter, setStatusFilter] = useState('all')
  const [sortBy, setSortBy] = useState('name')
  const [toasts, setToasts] = useState([])
  const [infoPlugin, setInfoPlugin] = useState(null)
  const [loading, setLoading] = useState(true)
  const [lastSync, setLastSync] = useState(null)

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

  // ---- Add toast notification ----
  const addToast = useCallback((text, type = 'info') => {
    const id = Date.now()
    setToasts(prev => [...prev, { id, text, type }])
    setTimeout(() => {
      setToasts(prev => prev.filter(t => t.id !== id))
    }, 4000)
  }, [])

  // ---- Remove toast ----
  const removeToast = useCallback((id) => {
    setToasts(prev => prev.filter(t => t.id !== id))
  }, [])

  // ---- Load initial data ----
  useEffect(() => {
    async function init() {
      if (isDevMode()) {
        // Dev mode — use mock data
        setPlugins(mockPlugins)
        setLicense({ licensed: true, customerName: 'Dev User', licenseKey: 'dev-key', message: '' })
        setLoading(false)
        setLastSync(new Date())
        return
      }

      // Get license status
      const licStatus = await api.getLicenseStatus()
      if (licStatus) setLicense(licStatus)

      // Get plugins
      const result = await api.getPlugins()
      if (result?.plugins) {
        setPlugins(result.plugins)
        setLastSync(new Date())
      }

      setLoading(false)
    }
    init()
  }, [])

  // ---- Subscribe to C++ events ----
  useEffect(() => {
    if (isDevMode()) return

    onEvent('pluginsUpdated', (data) => {
      if (data?.plugins) {
        setPlugins(data.plugins)
        setLastSync(new Date())
      }
    })

    onEvent('downloadProgress', (data) => {
      if (!data?.pluginId) return
      setPlugins(prev => prev.map(p =>
        p.id === data.pluginId
          ? { ...p, downloadProgress: data.progress, status: 'downloading' }
          : p
      ))
    })

    onEvent('downloadComplete', (data) => {
      // Will be followed by pluginsUpdated
    })

    onEvent('licenseChanged', (data) => {
      if (data) setLicense(prev => ({ ...prev, ...data }))
    })

    onEvent('licenseActivationResult', (data) => {
      if (data) {
        setLicense(prev => ({
          ...prev,
          licensed: data.success || false,
          customerName: data.customerName || prev.customerName,
          message: data.message || '',
        }))
        if (!data.success) {
          addToast(data.message || 'Activation failed', 'error')
        }
      }
    })

    onEvent('licenseDeactivationResult', (data) => {
      if (data?.success) {
        setLicense({ licensed: false, customerName: '', licenseKey: '', message: data.message || '' })
      }
    })

    onEvent('statusMessage', (data) => {
      if (data?.text) addToast(data.text, data.type || 'info')
    })
  }, [addToast])

  // ---- Actions ----
  const handleInstall = async (pluginId) => {
    try {
      const result = await api.installPlugin(pluginId)
      if (result && !result.started && result.error) {
        addToast(result.error, 'error')
      }
    } catch (err) {
      addToast(err.message || 'Install failed', 'error')
    }
  }

  const handleOpen = async (pluginId) => {
    try {
      const result = await api.openPlugin(pluginId)
      if (result && !result.success && result.error) {
        addToast(result.error, 'error')
      }
    } catch (err) {
      addToast(err.message || 'Could not open plugin', 'error')
    }
  }

  const handleRefresh = async () => {
    try {
      addToast('Checking for updates...', 'info')
      await api.refreshPlugins()
    } catch (err) {
      addToast(err.message || 'Refresh failed', 'error')
    }
  }

  const handleUpdateAll = async () => {
    const updatable = plugins.filter(p => p.status === 'update_available')
    if (updatable.length === 0) return
    addToast(`Updating ${updatable.length} plugin${updatable.length !== 1 ? 's' : ''}...`, 'info')
    for (const plugin of updatable) {
      await handleInstall(plugin.id)
    }
  }

  const handleActivate = async (key) => {
    try {
      const result = await api.activateLicense(key)
      return result
    } catch (err) {
      addToast(err.message || 'Activation failed', 'error')
      return { success: false, message: err.message }
    }
  }

  const handleDeactivate = async () => {
    try {
      const result = await api.deactivateLicense()
      return result
    } catch (err) {
      addToast(err.message || 'Deactivation failed', 'error')
      return { success: false, message: err.message }
    }
  }

  // ---- Filtered & sorted plugins ----
  const processedPlugins = React.useMemo(() => {
    let result = [...plugins]

    // Text search
    if (searchQuery) {
      const q = searchQuery.toLowerCase()
      result = result.filter(p =>
        p.name.toLowerCase().includes(q) ||
        p.description.toLowerCase().includes(q)
      )
    }

    // Status filter
    if (statusFilter === 'installed') {
      result = result.filter(p => p.status === 'up_to_date' || p.status === 'update_available')
    } else if (statusFilter === 'updates') {
      result = result.filter(p => p.status === 'update_available')
    } else if (statusFilter === 'not_installed') {
      result = result.filter(p => p.status === 'not_installed')
    }

    // Sort
    if (sortBy === 'name') {
      result.sort((a, b) => a.name.localeCompare(b.name))
    } else if (sortBy === 'status') {
      const order = { update_available: 0, not_installed: 1, downloading: 2, installing: 3, error: 4, up_to_date: 5 }
      result.sort((a, b) => (order[a.status] ?? 99) - (order[b.status] ?? 99))
    }

    return result
  }, [plugins, searchQuery, statusFilter, sortBy])

  // ---- Filter counts ----
  const filterCounts = React.useMemo(() => ({
    all: plugins.length,
    installed: plugins.filter(p => p.status === 'up_to_date' || p.status === 'update_available').length,
    updates: plugins.filter(p => p.status === 'update_available').length,
    not_installed: plugins.filter(p => p.status === 'not_installed').length,
  }), [plugins])

  // ---- Update counts for header badge ----
  const updatesCount = plugins.filter(p =>
    p.status === 'update_available' || p.status === 'not_installed'
  ).length

  // ---- Refresh lastSync display every minute ----
  const [, setTick] = useState(0)
  useEffect(() => {
    const interval = setInterval(() => setTick(t => t + 1), 60000)
    return () => clearInterval(interval)
  }, [])

  return (
    <motion.div
      className="h-screen flex flex-col bg-rone-bg overflow-hidden"
      initial={{ opacity: 0 }}
      animate={{ opacity: 1 }}
      transition={{ duration: 0.4, ease: 'easeOut' }}
    >
      {/* Shimmer overlay on unlock */}
      <AnimatePresence>
        {unlockPlaying && (
          <motion.div
            key="shimmer"
            className="fixed inset-0 z-50 pointer-events-none"
            initial={{ x: '-100%' }}
            animate={{ x: '100%' }}
            exit={{ opacity: 0 }}
            transition={{ duration: 0.8, ease: 'easeInOut' }}
            style={{
              background: 'linear-gradient(90deg, transparent, rgba(181,55,242,0.15), rgba(224,64,251,0.1), transparent)',
              width: '100%',
            }}
          />
        )}
      </AnimatePresence>

      {/* Header */}
      <Header
        updatesCount={updatesCount}
        searchQuery={searchQuery}
        onSearchChange={setSearchQuery}
        onRefresh={handleRefresh}
        lastSync={lastSync}
      />

      {/* License Bar */}
      <LicenseBar
        license={license}
        onActivate={handleActivate}
        onDeactivate={handleDeactivate}
        unlockPlaying={unlockPlaying}
      />

      {/* Featured Section */}
      {!loading && (
        <FeaturedSection
          plugins={plugins}
          onInstall={handleInstall}
          onUpdateAll={handleUpdateAll}
          licensed={license.licensed}
        />
      )}

      {/* Filter Bar */}
      {!loading && plugins.length > 0 && (
        <FilterBar
          statusFilter={statusFilter}
          onStatusFilterChange={setStatusFilter}
          sortBy={sortBy}
          onSortChange={setSortBy}
          counts={filterCounts}
        />
      )}

      {/* Plugin Grid */}
      <div className="flex-1 overflow-hidden">
        <PluginGrid
          plugins={processedPlugins}
          licensed={license.licensed}
          onInstall={handleInstall}
          onOpen={handleOpen}
          onInfo={setInfoPlugin}
          unlockPlaying={unlockPlaying}
          loading={loading}
        />
      </div>

      {/* Info Modal */}
      <AnimatePresence>
        {infoPlugin && (
          <InfoModal
            plugin={infoPlugin}
            onClose={() => setInfoPlugin(null)}
          />
        )}
      </AnimatePresence>

      {/* Status Toasts */}
      <StatusToast toasts={toasts} onRemove={removeToast} />
    </motion.div>
  )
}
