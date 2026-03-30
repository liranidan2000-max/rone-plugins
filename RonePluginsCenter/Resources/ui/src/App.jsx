import React, { useState, useEffect, useCallback, useRef } from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import { api, onEvent, isDevMode, mockPlugins } from './bridge'
import Header from './components/Header'
import LicenseBar from './components/LicenseBar'
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
  const [toasts, setToasts] = useState([])
  const [infoPlugin, setInfoPlugin] = useState(null)
  const [loading, setLoading] = useState(true)

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
        return
      }

      // Get license status
      const licStatus = await api.getLicenseStatus()
      if (licStatus) setLicense(licStatus)

      // Get plugins
      const result = await api.getPlugins()
      if (result?.plugins) setPlugins(result.plugins)

      setLoading(false)
    }
    init()
  }, [])

  // ---- Subscribe to C++ events ----
  useEffect(() => {
    if (isDevMode()) return

    onEvent('pluginsUpdated', (data) => {
      if (data?.plugins) setPlugins(data.plugins)
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
    const result = await api.installPlugin(pluginId)
    if (result && !result.started && result.error) {
      addToast(result.error, 'error')
    }
  }

  const handleOpen = async (pluginId) => {
    const result = await api.openPlugin(pluginId)
    if (result && !result.success && result.error) {
      addToast(result.error, 'error')
    }
  }

  const handleRefresh = async () => {
    addToast('Checking for updates...', 'info')
    await api.refreshPlugins()
  }

  const handleActivate = async (key) => {
    const result = await api.activateLicense(key)
    return result
  }

  const handleDeactivate = async () => {
    const result = await api.deactivateLicense()
    return result
  }

  // ---- Filtered plugins ----
  const filteredPlugins = plugins.filter(p =>
    !searchQuery ||
    p.name.toLowerCase().includes(searchQuery.toLowerCase()) ||
    p.description.toLowerCase().includes(searchQuery.toLowerCase())
  )

  // ---- Update counts ----
  const updatesCount = plugins.filter(p =>
    p.status === 'update_available' || p.status === 'not_installed'
  ).length

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
      />

      {/* License Bar */}
      <LicenseBar
        license={license}
        onActivate={handleActivate}
        onDeactivate={handleDeactivate}
        unlockPlaying={unlockPlaying}
      />

      {/* Plugin Grid */}
      <div className="flex-1 overflow-hidden">
        {loading ? (
          <div className="flex items-center justify-center h-full">
            <div className="text-rone-text-secondary text-sm animate-pulse">
              Loading plugins...
            </div>
          </div>
        ) : (
          <PluginGrid
            plugins={filteredPlugins}
            licensed={license.licensed}
            onInstall={handleInstall}
            onOpen={handleOpen}
            onInfo={setInfoPlugin}
            unlockPlaying={unlockPlaying}
          />
        )}
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
