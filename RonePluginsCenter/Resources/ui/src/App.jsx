import React, { useCallback, useEffect, useMemo, useRef, useState } from 'react'
import { api, callNative, onEvent, openExternal, isDevMode, devScenario, mockPlugins, mockAccount, mockDaws, mockTips, mockAnnouncements, mockUpsell } from './bridge'
import Sidebar from './components/Sidebar'
import TopBar from './components/TopBar'
import InstallDock from './components/InstallDock'
import Toasts from './components/Toasts'
import Onboarding from './components/Onboarding'
import ManualDialog from './components/ManualDialog'
import AnnouncementModal from './components/AnnouncementModal'
import UpsellModal from './components/UpsellModal'
import { Dialog } from './components/ui'
import LibraryView from './views/LibraryView'
import UpdatesView from './views/UpdatesView'
import DetailView, { stepsFor } from './views/DetailView'
import RackView from './views/RackView'
import LearnView from './views/LearnView'
import SettingsView from './views/SettingsView'
import AccountView from './views/AccountView'
import { makeT, LOCALES } from './i18n'
import { usePrefs, prefersReducedMotion } from './prefs'
import { playPowerOn } from './sounds'
import { productKey, ownedProductIds } from './ownership'
import { pickAnnouncement, rememberAnnouncement } from './announcements'
import { isBusy, isInstalled, shortName, updatable, accentOf } from './catalog'
import { translateStatus, installErrorText } from './messages'

const DEV = isDevMode()

export default function App () {
  const [prefs, setPrefs] = usePrefs()
  const t = useMemo(() => makeT(prefs.lang), [prefs.lang])
  const locale = LOCALES[prefs.lang] || 'en-US'
  const reduced = prefersReducedMotion(prefs)

  const [plugins, setPlugins] = useState([])
  const [loading, setLoading] = useState(true)
  const [license, setLicense] = useState({ licensed: false, customerName: '', licenseKey: '', message: '' })
  const [account, setAccount] = useState({ signedIn: false, licensed: false, email: '', name: '', plan: 'none', deviceLimit: 2, owned: [], passSource: '', message: '' })
  const [manifest, setManifest] = useState({ offline: false, loaded: false, syncedAt: 0 })
  const [tips, setTips] = useState([])
  const [daws, setDaws] = useState([])
  const [platform, setPlatform] = useState('windows')
  const [centerUpdate, setCenterUpdate] = useState(null)

  const [view, setView] = useState('library')
  const [detailId, setDetailId] = useState(null)
  const [returnView, setReturnView] = useState('library')
  const [query, setQuery] = useState('')
  const [cat, setCat] = useState('all')
  const [sort, setSort] = useState('yours')

  const [toasts, setToasts] = useState([])
  const [manualPlugin, setManualPlugin] = useState(null)
  const [uninstallAsk, setUninstallAsk] = useState(null)
  const [announcement, setAnnouncement] = useState(null)
  // This account's own prices from roneaudio.com/api/v1/upsell (null = none): { offer, owner, links }
  const [upsell, setUpsell] = useState(null)
  const [upsellShown, setUpsellShown] = useState(null)
  const [upsellTick, setUpsellTick] = useState(0)
  const [onboardingOpen, setOnboardingOpen] = useState(false)

  const catLabel = useCallback((c) => t('cat.' + c), [t])
  const scroller = useRef(null)

  // ---- toasts ----
  const addToast = useCallback((toast) => {
    const id = Date.now() + Math.random()
    const x = typeof toast === 'string' ? { text: toast, type: 'info' } : toast
    // The same words twice in a row (a message repeated by the backend) show once.
    setToasts(prev => prev.some(y => y.text === x.text && y.title === x.title) ? prev : [...prev.slice(-3), { id, ...x }])
    // A "To fix it: ..." line stays until it is closed (audit R032).
    if (!x.sticky) setTimeout(() => setToasts(prev => prev.filter(y => y.id !== id)), x.steps ? 12000 : x.type === 'error' ? 9000 : 5000)
  }, [])
  const removeToast = useCallback((id) => setToasts(prev => prev.filter(x => x.id !== id)), [])

  // ---- ownership (LIFETIME licences travel on the plugin object) ----
  const ownedIds = useMemo(() => ownedProductIds(account.owned), [account.owned])
  const ownedKeys = useMemo(() => new Set(ownedIds.map(productKey)), [ownedIds])
  // Each plugin as this page shows it: owned or not, and described in the user's language.
  // A plugin this owner lacks also carries their price (catalog.js priceOf reads it): the 24-hour $19 offer or the owner price.
  const myPrices = useMemo(() => {
    const m = new Map()
    if (!upsell) return m
    for (const [id, x] of Object.entries(upsell.owner?.prices || {})) m.set(productKey(id), { ownerPrice: x.price })
    // the $19 "one more" after a purchase, or the first-plugin offer of an account that owns nothing yet
    for (const o of [upsell.offer, upsell.first]) {
      if (o && Date.parse(o.endsAt) > Date.now()) m.set(productKey(o.product), { ...(m.get(productKey(o.product)) || {}), offerPrice: o.price, offerEndsAt: o.endsAt })
    }
    return m
  }, [upsell, upsellTick])
  // RONE's own apps (the manifest's "apps": RONE VST MANAGER) are not plugins: they leave the catalog and
  // come back as one gift card, shown only to a customer (the backend decides `unlocked`) on Windows.
  const catalog = useMemo(() => plugins.filter(p => p.kind !== 'app'), [plugins])
  const giftApp = useMemo(() => (platform === 'mac' ? null : plugins.find(p => p.kind === 'app' && p.unlocked) || null), [plugins, platform])
  const tagged = useMemo(() => catalog.map(p => {
    const d = p.i18n?.[prefs.lang]?.description
    const q = d ? { ...p, description: d } : p
    if (ownedKeys.has(productKey(p.id))) return { ...q, owned: true }
    const mine = myPrices.get(productKey(p.id))
    return mine ? { ...q, ...mine } : q
  }), [catalog, ownedKeys, prefs.lang, myPrices])
  const localTips = useMemo(() => tips.map(x => ({ ...x, text: (typeof x.i18n?.[prefs.lang] === 'string' && x.i18n[prefs.lang]) || x.text })), [tips, prefs.lang])
  const ownedPlugins = useMemo(() => tagged.filter(p => p.owned), [tagged])
  const access = useMemo(() => ({ licensed: !!license.licensed, signedIn: !!account.signedIn }), [license.licensed, account.signedIn])
  const ups = updatable(tagged, access)
  const dawId = prefs.daw || daws[0]?.id || null
  const dawName = (daws.find(d => d.id === dawId) || {}).name || t('daw.generic')

  // ---- navigation ----
  const viewRef = useRef(view)
  viewRef.current = view
  const navigate = useCallback((v) => {
    setView(v)
    if (v !== 'detail') setReturnView(v)
    if (v === 'library') setCat('all')
    requestAnimationFrame(() => scroller.current?.scrollTo?.({ top: 0 }))
  }, [])
  const openDetail = useCallback((id) => {
    setDetailId(id)
    setView(cur => { if (cur !== 'detail') setReturnView(cur); return 'detail' })
    requestAnimationFrame(() => scroller.current?.scrollTo?.({ top: 0 }))
  }, [])

  // ---- what an install that just finished says ----
  const announceInstalled = useCallback((plugin, version) => {
    if (prefs.sounds) playPowerOn()
    addToast({
      type: 'success', accent: plugin ? accentOf(plugin) : undefined,
      title: t('msg.installedTitle', { name: 'RONE ' + (plugin ? shortName(plugin) : '') + (version ? ' ' + version : '') }),
      text: t('msg.installedIn', { daw: dawName }),
      steps: stepsFor(t, dawId, plugin ? shortName(plugin) : ''),
    })
  }, [prefs.sounds, addToast, t, dawId, dawName])

  // ---- load ----
  useEffect(() => {
    async function init () {
      if (DEV) {
        const sc = devScenario()
        setPlugins(mockPlugins(sc))
        const acct = mockAccount(sc)
        setAccount(acct)
        setLicense({ licensed: acct.licensed, customerName: acct.name, licenseKey: '', message: '' })
        setManifest({ offline: new URLSearchParams(location.search).has('offline'), loaded: true, syncedAt: Date.now() - 60000 })
        setTips(mockTips); setDaws(mockDaws)
        if (new URLSearchParams(location.search).has('centerupdate')) setCenterUpdate('2.0.1.250')
        if (new URLSearchParams(location.search).has('mac')) setPlatform('mac')   // the Mac-only rows
        setLoading(false)
        return
      }
      try { const lic = await api.getLicenseStatus(); if (lic) setLicense(lic) } catch {}
      try { const acct = await api.getAccountStatus(); if (acct) setAccount(acct) } catch {}
      try { const v = await api.getAppVersion(); if (v?.platform) setPlatform(v.platform) } catch {}
      try {
        const r = await api.getPlugins()
        if (r?.plugins) setPlugins(r.plugins)
        if (r?.manifest) setManifest(r.manifest)
        if (Array.isArray(r?.tips)) setTips(r.tips)
        if (r?.centerUpdate?.version) setCenterUpdate(r.centerUpdate.version)
        if (r?.navigate) handleNavigateTo(r.navigate)
      } catch {}
      setLoading(false)
      try { const d = await api.getDaws(); if (Array.isArray(d)) setDaws(d) } catch {}
    }
    init()
  }, [])

  // ---- first launch ----
  useEffect(() => {
    if (loading) return
    if (!prefs.onboarded && (!DEV || new URLSearchParams(location.search).has('onboarding'))) setOnboardingOpen(true)
  }, [loading])

  // ---- events from the Center ----
  const handleNavigateTo = useCallback((to) => {
    if (typeof to !== 'string') return
    if (to.startsWith('plugin:')) openDetail(to.slice(7))
    else if (to === 'updates') navigate('updates')
  }, [openDetail, navigate])

  const pluginsRef = useRef(plugins)
  pluginsRef.current = tagged
  const giftRef = useRef(giftApp)
  giftRef.current = giftApp

  // The rules live in messages.js (tested there); an install that finished gets
  // its own toast with the DAW steps.
  const translateMessage = useCallback((m) => {
    if (m?.code === 'installed') {
      const params = m.params || {}
      const gift = giftRef.current
      if (gift && gift.id === params.id) {
        if (prefs.sounds) playPowerOn()
        return { type: 'success', title: t('msg.installedTitle', { name: gift.name }), text: t('gift.installedText') }
      }
      const plugin = pluginsRef.current.find(x => x.id === params.id)
      announceInstalled(plugin, (params.version || '').split('.').slice(0, 3).join('.'))
      return null
    }
    return translateStatus(m, t)
  }, [t, announceInstalled, prefs.sounds])

  const translateRef = useRef(translateMessage)
  translateRef.current = translateMessage

  useEffect(() => {
    if (DEV) return
    onEvent('pluginsUpdated', (d) => { if (d?.plugins) setPlugins(d.plugins) })
    onEvent('downloadProgress', (d) => {
      if (!d?.pluginId) return
      setPlugins(prev => prev.map(p => p.id === d.pluginId ? { ...p, downloadProgress: d.progress, status: 'downloading' } : p))
    })
    onEvent('manifestState', (d) => { if (d) setManifest(d) })
    onEvent('licenseChanged', (d) => { if (d) setLicense(prev => ({ ...prev, ...d })) })
    onEvent('accountChanged', (d) => { if (d) setAccount(d) })
    onEvent('licenseActivationResult', (d) => {
      if (!d) return
      setLicense(prev => ({ ...prev, licensed: d.success || false, customerName: d.customerName || prev.customerName, message: d.message || '' }))
      if (!d.success) addToast({ text: d.message || 'Activation failed', type: 'error' })
    })
    onEvent('licenseDeactivationResult', (d) => { if (d?.success) setLicense({ licensed: false, customerName: '', licenseKey: '', message: d.message || '' }) })
    onEvent('statusMessage', (d) => {
      if (!d?.text) return
      const x = translateRef.current(d)
      if (x) addToast(x)
    })
    onEvent('centerUpdateAvailable', (d) => { if (d?.version) setCenterUpdate(d.version) })
    onEvent('navigate', (d) => handleNavigateTo(d?.to))
    // Behind the DAW nothing animates (index.css .app-idle).
    onEvent('windowActive', (d) => document.documentElement.classList.toggle('app-idle', d?.active === false))
  }, [])

  // ---- the Center updates itself, once per version, and never mid-install (the backend waits) ----
  const centerTried = useRef(null)
  useEffect(() => {
    if (!centerUpdate || DEV || centerTried.current === centerUpdate) return
    centerTried.current = centerUpdate
    const key = 'centerAutoTried:' + centerUpdate
    let tried = false
    try { tried = localStorage.getItem(key) === '1' } catch {}
    if (tried) return
    try { localStorage.setItem(key, '1') } catch {}
    setTimeout(() => api.applyCenterUpdate().catch(() => {}), 2200)
  }, [centerUpdate])

  // ---- keyboard: Ctrl+R / F5 check for updates (the page itself never reloads) ----
  useEffect(() => {
    const onKey = (e) => {
      if ((e.ctrlKey && e.key.toLowerCase() === 'r') || e.key === 'F5') { e.preventDefault(); refresh() }
    }
    window.addEventListener('keydown', onKey)
    return () => window.removeEventListener('keydown', onKey)
  })

  // ---- dev preview: installs that run (so the queue, the dock and the power-on can be seen) ----
  const devTimer = useRef(null)
  const devInstall = (ids) => {
    setPlugins(prev => prev.map(p => ids.includes(p.id) && !isBusy(p) ? { ...p, status: 'queued', downloadProgress: 0 } : p))
    if (devTimer.current) return
    devTimer.current = setInterval(() => {
      setPlugins(prev => {
        const cur = prev.find(p => p.status === 'downloading') || prev.find(p => p.status === 'queued')
        const inst = prev.find(p => p.status === 'installing')
        if (!cur && !inst) { clearInterval(devTimer.current); devTimer.current = null; return prev }
        return prev.map(p => {
          if (inst && p.id === inst.id) {
            if ((p._ticks || 0) > 8) {
              setTimeout(() => announceInstalled(p, (p.remoteVersion || '').split('.').slice(0, 3).join('.')), 0)
              return { ...p, status: 'up_to_date', installedVersion: p.remoteVersion, _ticks: 0 }
            }
            return { ...p, _ticks: (p._ticks || 0) + 1 }
          }
          if (cur && p.id === cur.id) {
            const prog = Math.min(1, (p.downloadProgress || 0) + 0.07)
            return prog >= 1 && !inst ? { ...p, status: 'installing', downloadProgress: 1 } : { ...p, status: prog >= 1 ? 'ready' : 'downloading', downloadProgress: prog }
          }
          if (!inst && p.status === 'ready' && !cur) return { ...p, status: 'installing' }
          return p
        })
      })
    }, 140)
  }

  // ---- actions ----
  const install = async (id) => {
    if (DEV) { devInstall([id]); return }
    try {
      const r = await api.installPlugin(id)
      if (r && !r.started && r.error) {
        const plugin = pluginsRef.current.find(p => p.id === id)
        const name = plugin ? 'RONE ' + shortName(plugin) : giftRef.current?.id === id ? giftRef.current.name : id
        addToast({ text: installErrorText(r.error, name, t), type: 'error' })
      }
    } catch (e) { addToast({ text: e.message || 'Install failed', type: 'error' }) }
  }
  const updateAll = async () => {
    const ids = updatable(pluginsRef.current, access).map(p => p.id)
    if (!ids.length) return
    if (DEV) { devInstall(ids); return }
    for (const id of ids) await install(id)
  }
  const cancel = async (id) => {
    if (DEV) { setPlugins(prev => prev.map(p => p.id === id ? { ...p, status: p.installedVersion ? 'update_available' : 'not_installed', downloadProgress: 0 } : p)); return }
    try { await api.cancelInstall(id) } catch {}
  }
  const cancelAll = async () => {
    if (DEV) { setPlugins(prev => prev.map(p => isBusy(p) && p.status !== 'installing' ? { ...p, status: p.installedVersion ? 'update_available' : 'not_installed', downloadProgress: 0 } : p)); return }
    try { await api.cancelInstall('') } catch {}
  }
  const open = async (id) => {
    if (DEV) { addToast({ text: 'Opens the standalone app.', type: 'info' }); return }
    try { const r = await api.openPlugin(id); if (r && !r.success && r.error) addToast({ text: r.error, type: 'error' }) }
    catch (e) { addToast({ text: e.message, type: 'error' }) }
  }
  const openFolder = async (id) => {
    if (DEV) return
    try { const r = await api.openFolder(id); if (r && !r.success && r.error) addToast({ text: r.error, type: 'error' }) } catch (e) { addToast({ text: e.message, type: 'error' }) }
  }
  const buy = (plugin, src = 'card') => {
    // An owner's price opens their signed roneaudio.com/owner link, where checkout applies it (src = where they clicked).
    const link = Object.entries(upsell?.links || {}).find(([id]) => productKey(id) === productKey(plugin.id))?.[1]
    if (link) { openExternal(link + (link.includes('?') ? '&' : '?') + 'src=' + src); return }
    const url = (plugin.store_url || 'https://roneaudio.com/pricing.html')
    openExternal(url + (url.includes('?') ? '&' : '?') + 'utm_source=plugins_center&utm_medium=card')
  }
  const manualPdf = async (plugin) => {
    setManualPlugin(null)
    if (DEV) { addToast(t('msg.openingPdf')); return }
    try {
      const r = await api.openManual(plugin.id)
      if (r && !r.success && r.error) addToast({ text: r.error, type: 'error' })
      else if (r?.source === 'online') addToast(t('msg.openingPdfOnline'))
    } catch (e) { addToast({ text: e.message, type: 'error' }) }
  }
  const manualVideo = (plugin) => { setManualPlugin(null); addToast(t('msg.openingVideo')); openExternal(plugin.videoUrl) }
  const refresh = async () => {
    if (DEV) { setManifest(m => ({ ...m, syncedAt: Date.now(), offline: false })); addToast(t('msg.checking')); return }
    try { await api.refreshPlugins() } catch (e) { addToast({ text: e.message, type: 'error' }) }
  }
  const confirmUninstall = async () => {
    const plugin = uninstallAsk
    setUninstallAsk(null)
    if (!plugin) return
    const done = t('msg.uninstalled', { name: 'RONE ' + shortName(plugin) })
    if (DEV) {
      setPlugins(prev => prev.map(p => p.id === plugin.id ? { ...p, status: 'uninstalling' } : p))
      setTimeout(() => { setPlugins(prev => prev.map(p => p.id === plugin.id ? { ...p, status: 'not_installed', installedVersion: '' } : p)); addToast({ text: done, type: 'success' }) }, 1200)
      return
    }
    try {
      const r = await api.uninstallPlugin(plugin.id)
      addToast(r?.ok ? { text: done, type: 'success' }
                     : { text: r?.error || t('msg.uninstallFailed', { name: 'RONE ' + shortName(plugin) }), type: 'error' })
    } catch (e) { addToast({ text: e.message, type: 'error' }) }
  }

  // ---- account ----
  const afterSignIn = (res, fallback) => {
    if (res?.account) setAccount(res.account)
    if (res?.ok) {
      setLicense(prev => ({ ...prev, licensed: !!res.account?.licensed, customerName: res.account?.name || res.account?.email || prev.customerName }))
      addToast({ text: res.message || fallback, type: 'success' })
      // Audit R071: a first sign-in ended on the Account page with nothing to do
      // there - My Rack shows what the account owns, with Install on each.
      if (viewRef.current === 'account') navigate('rack')
    }
    return res
  }
  const signIn = async (email, password) => {
    try { return afterSignIn(await api.accountSignIn(email, password), t('msg.signedIn')) }
    catch (e) { addToast({ text: e.message, type: 'error' }); return { ok: false, message: e.message } }
  }
  const googleSignIn = async () => {
    try { return afterSignIn(await api.accountGoogleSignIn(), t('msg.signedIn')) }
    catch (e) { return { ok: false, message: e.message } }
  }
  const signOut = async () => {
    try {
      const r = await api.accountSignOut()
      setAccount({ signedIn: false, licensed: false, email: '', name: '', plan: 'none', owned: [], passSource: '', message: '' })
      setLicense(prev => ({ ...prev, licensed: false, customerName: '' }))
      addToast({ text: r?.message || t('msg.signedOut'), type: 'info' })
    } catch (e) { addToast({ text: e.message, type: 'error' }) }
  }
  const activate = async (key) => { try { return await api.activateLicense(key) } catch (e) { return { success: false, message: e.message } } }
  const deactivate = async () => { try { return await api.deactivateLicense() } catch (e) { return { success: false, message: e.message } } }
  const goAccount = (to) => {
    if (to === 'signup') openExternal('https://roneaudio.com/account/signup.html?utm_source=plugins_center&utm_medium=onboarding')
    navigate('account')
  }

  // ---- this account's own prices: the $19 day after a purchase and the owner price (roneaudio.com/api/v1/upsell) ----
  // Asked again whenever what they own changes (a purchase reaches the Center through accountChanged) and when the
  // 24-hour offer runs out. Never for a guest or an ALL ACCESS pass; the server also says nothing to a gift.
  const ownedSig = (account.owned || []).join(',')
  useEffect(() => {
    if (loading) return
    if (!account.signedIn || license.licensed) { setUpsell(null); return }
    let alive = true
    let timer = null
    ;(async () => {
      let r = null
      if (DEV) r = new URLSearchParams(location.search).has('upsell') ? mockUpsell() : null
      else { try { r = await api.getUpsell() } catch {} }
      if (!alive) return
      setUpsell(r && r.ok ? r : null)
      const ms = (r?.offer || r?.first) ? Date.parse((r.offer || r.first).endsAt) - Date.now() : 0
      if (ms > 0 && ms < 2 ** 31) timer = setTimeout(() => setUpsellTick(n => n + 1), ms + 3000)
    })()
    return () => { alive = false; if (timer) clearTimeout(timer) }
  }, [loading, account.signedIn, license.licensed, ownedSig, upsellTick])

  // The video popup: once per offer, at an open of the Center after the plugin they bought is installed (it was
  // installed when this window loaded), never over an install or another popup.
  const installedAtStart = useRef(null)
  useEffect(() => { if (!loading && plugins.length && !installedAtStart.current) installedAtStart.current = new Set(plugins.filter(isInstalled).map(p => productKey(p.id))) }, [loading, plugins])
  // The first-plugin offer (an account with nothing of its own yet) pops the same way, at an open of the Center.
  useEffect(() => {
    const live = (x) => (x && Date.parse(x.endsAt) > Date.now() ? x : null)
    const o = live(upsell?.offer) || live(upsell?.first)
    if (!o || loading || onboardingOpen || announcement || upsellShown || !installedAtStart.current) return
    if (tagged.some(isBusy)) return
    if (o.kind !== 'first' && !installedAtStart.current.has(productKey(o.because))) return
    const key = 'rone_center_upsell_seen:' + (o.kind === 'first' ? 'first' : o.because) + ':' + o.endsAt
    let seen = false
    try { seen = localStorage.getItem(key) === '1' } catch {}
    if (seen && !DEV) return
    try { localStorage.setItem(key, '1') } catch {}
    setUpsellShown(o)
  }, [upsell, loading, onboardingOpen, announcement, tagged])
  const upsellPlugin = upsellShown ? tagged.find(p => productKey(p.id) === productKey(upsellShown.product)) : null

  // The banner's personal slide: the $19 plugin while that day runs, else the owner pick.
  const personal = useMemo(() => {
    if (!upsell) return null
    const f = upsell.first && Date.parse(upsell.first.endsAt) > Date.now() ? upsell.first : null
    if (f) {
      const plugin = tagged.find(p => productKey(p.id) === productKey(f.product))
      return plugin && !plugin.owned ? { plugin, kind: 'first', line: f.line || '', endsAt: f.endsAt } : null
    }
    const o = upsell.offer && Date.parse(upsell.offer.endsAt) > Date.now() ? upsell.offer : null
    const id = o ? o.product : upsell.owner?.pick
    const plugin = id && tagged.find(p => productKey(p.id) === productKey(id))
    if (!plugin || plugin.owned) return null
    return { plugin, becauseName: (o ? o.becauseName : upsell.owner?.becauseName) || '', line: o?.line || '' }
  }, [upsell, tagged])

  // ---- the deals list (roneaudio.com/api/v1/optin; Liran's sales plan 14.10, Black Friday early access) ----
  // A switch in Settings for every signed-in account, and ONE banner ask - never to a gift pass.
  const [optin, setOptin] = useState(null)
  useEffect(() => {
    if (loading || !account.signedIn) { setOptin(null); return }
    if (DEV) { setOptin({ on: new URLSearchParams(location.search).get('optin') === '1' }); return }
    let alive = true
    api.getOptin().then(r => { if (alive) setOptin(r && typeof r.on === 'boolean' ? { on: r.on } : null) }).catch(() => {})
    return () => { alive = false }
  }, [loading, account.signedIn])
  const changeOptin = async (on) => {
    setOptin({ on })
    if (DEV) { addToast({ text: t(on ? 'msg.optinOn' : 'msg.optinOff'), type: 'success' }); return }
    let r = null
    try { r = await api.setOptin(on) } catch {}
    if (r && typeof r.on === 'boolean') { setOptin({ on: r.on }); addToast({ text: t(r.on ? 'msg.optinOn' : 'msg.optinOff'), type: 'success' }) }
    else { setOptin({ on: !on }); addToast({ text: t('msg.optinFail'), type: 'error' }) }
  }
  const [optinAsked, setOptinAsked] = useState(() => { try { return localStorage.getItem('rone_center_optin_asked') === '1' } catch { return false } })
  const answerOptin = (yes) => {
    try { localStorage.setItem('rone_center_optin_asked', '1') } catch {}
    setOptinAsked(true)
    if (yes) changeOptin(true)
  }
  const optinAsk = optin && !optin.on && !optinAsked && account.passSource !== 'comp' ? answerOptin : null

  // ---- announcements (the website's popups), once per page after the catalog is in ----
  const announceAsked = useRef(false)
  const latest = useRef({})
  latest.current = { plugins: tagged, ownedKeys, licensed: license.licensed, passSource: account.passSource }
  useEffect(() => {
    if (loading || announceAsked.current || !plugins.length || onboardingOpen || upsellShown) return
    if (!(account.signedIn || license.licensed)) return
    announceAsked.current = true
    setTimeout(async () => {
      let feed = null
      if (DEV) feed = new URLSearchParams(location.search).has('announce') ? mockAnnouncements : null
      else { try { feed = await api.getAnnouncements() } catch {} }
      if (!feed?.popups?.length) return
      const item = pickAnnouncement(feed.popups, { ...latest.current, ignoreMemory: DEV, t })
      if (!item) return
      if (!DEV) rememberAnnouncement(item.popup.id)
      setAnnouncement(item)
    }, 1800)
  }, [loading, plugins.length, account.signedIn, license.licensed, onboardingOpen, upsellShown])
  const runAnnouncement = () => {
    const item = announcement
    setAnnouncement(null)
    if (!item) return
    if (item.primary.kind === 'install') install(item.plugin.id)
    else if (item.primary.url) callNative('openExternalUrl', item.primary.url).catch(() => {})
  }

  const handlers = {
    onInstall: install, onCancel: cancel, onOpen: open, onDetail: openDetail, onBuy: buy,
    onSignIn: () => navigate('account'), onManual: setManualPlugin, onOpenFolder: openFolder, onUninstall: setUninstallAsk,
  }
  const detailPlugin = tagged.find(p => p.id === detailId)
  const installedCount = tagged.filter(isInstalled).length

  return (
    <div className="h-screen flex flex-col bg-rone-bg overflow-hidden">
      {centerUpdate && !(view === 'library' && !query && ups.length > 0) && (
        <div className="flex-shrink-0 flex items-center gap-3 px-6 py-2 bg-white/[0.04] border-b border-white/10">
          <span className="led led-upd" />
          <span className="text-[12px] font-bold text-rone-text-secondary">{t('cu.ready', { version: centerUpdate.split('.').slice(0, 3).join('.') })}</span>
          <span className="flex-1" />
          <button className="btn btn-pri btn-white btn-sm" onClick={() => api.applyCenterUpdate().catch(() => {})}>{t('cu.restart')}</button>
        </div>
      )}

      <div className="flex-1 min-h-0 flex">
        <Sidebar t={t} view={view} onNavigate={navigate} updatesCount={ups.length} account={account} license={license}
                 ownedPlugins={ownedPlugins} locale={locale} />

        <main className="flex-1 min-w-0 flex flex-col relative">
          <TopBar t={t} searchQuery={query} onSearchChange={(q) => { setQuery(q); if (q && view !== 'library') navigate('library') }}
                  onRefresh={refresh} manifest={manifest} account={account} license={license} daws={daws} dawId={dawId}
                  onPickDaw={() => navigate('settings')} onAccount={() => navigate('account')} />

          {manifest.offline && (
            <div className="flex-shrink-0 flex items-center gap-3 px-6 py-2 border-b border-rone-amber/25 bg-rone-amber/[0.05] text-[12.5px] text-rone-text-secondary" role="status">
              <span className="led led-upd" />{t('offline.banner')}
              <span className="flex-1" />
              <button className="btn btn-ghost btn-sm" onClick={refresh}>{t('offline.retry')}</button>
            </div>
          )}

          <div ref={scroller} className="flex-1 min-h-0 overflow-y-auto scroll-y px-6 pt-5 pb-24">
            {view === 'library' && (
              <LibraryView t={t} plugins={tagged} loading={loading} access={access} account={account} license={license} personal={personal} optinAsk={optinAsk}
                           centerUpdate={centerUpdate} onCenterUpdate={() => api.applyCenterUpdate().catch(() => {})}
                           tips={localTips} reduced={reduced} query={query} setQuery={setQuery} cat={cat} setCat={setCat} sort={sort} setSort={setSort}
                           catLabel={catLabel} onNavigate={navigate} onUpdateAll={updateAll} onRefresh={refresh} handlers={handlers} giftApp={giftApp} />
            )}
            {view === 'updates' && (
              <UpdatesView t={t} plugins={tagged} access={access} onInstall={install} onUpdateAll={updateAll} onCancel={cancel} onRefresh={refresh} onDetail={openDetail} />
            )}
            {view === 'detail' && detailPlugin && (
              <DetailView t={t} plugin={detailPlugin} access={access} reduced={reduced} daws={daws} dawId={dawId}
                          onDaw={(id) => setPrefs({ daw: id })} catLabel={catLabel} onBack={() => navigate(returnView)}
                          onInstall={install} onCancel={cancel} onOpen={open} onBuy={(p) => buy(p, 'detail')} onSignIn={() => navigate('account')}
                          onManualPdf={manualPdf} onOpenFolder={openFolder} onUninstall={setUninstallAsk} />
            )}
            {view === 'rack' && <RackView t={t} plugins={tagged} access={access} onDetail={openDetail} />}
            {view === 'learn' && <LearnView t={t} plugins={tagged} tips={localTips} onManualPdf={manualPdf} onDetail={openDetail} />}
            {view === 'settings' && (
              <SettingsView t={t} prefs={prefs} setPrefs={setPrefs} daws={daws} platform={platform} manifest={manifest} optin={optin} onOptin={changeOptin}
                            onRefresh={refresh} onShowOnboarding={() => setOnboardingOpen(true)} addToast={addToast} />
            )}
            {view === 'account' && (
              <AccountView t={t} locale={locale} license={license} account={account} onSignIn={signIn} onSignOut={signOut}
                           onGoogleSignIn={googleSignIn} onGoogleCancel={() => api.accountGoogleCancel().catch(() => {})}
                           onActivate={activate} onDeactivate={deactivate} installedCount={installedCount} ownedPlugins={ownedPlugins}
                           onOpenDetail={openDetail} />
            )}
          </div>

          <InstallDock t={t} plugins={tagged} platform={platform} onCancelAll={cancelAll} />
        </main>
      </div>

      <ManualDialog t={t} plugin={manualPlugin} onPdf={manualPdf} onVideo={manualVideo} onClose={() => setManualPlugin(null)} />

      <Dialog open={!!uninstallAsk} onClose={() => setUninstallAsk(null)} label={t('dlg.uninstall')} width={400}>
        <h3 className="m-0 mb-2 text-[15px] font-extrabold text-rone-text-primary">{t('dlg.uninstallTitle', { name: uninstallAsk ? 'RONE ' + shortName(uninstallAsk) : '' })}</h3>
        <p className="m-0 mb-5 text-[12.5px] text-rone-text-secondary leading-relaxed">{t('dlg.uninstallBody')}</p>
        <div className="flex justify-end gap-2">
          <button className="btn btn-out" onClick={() => setUninstallAsk(null)} data-autofocus>{t('dlg.cancel')}</button>
          <button className="btn btn-danger" onClick={confirmUninstall}>{t('dlg.uninstall')}</button>
        </div>
      </Dialog>

      {upsellShown && !onboardingOpen && (
        <UpsellModal t={t} offer={upsellShown} plugin={upsellPlugin} accent={upsellPlugin ? accentOf(upsellPlugin) : '#E8EAED'} reduced={reduced}
                     onBuy={() => { const p = upsellPlugin; setUpsellShown(null); if (p) buy(p, 'popup') }} onClose={() => setUpsellShown(null)} />
      )}

      {announcement && !onboardingOpen && (
        <AnnouncementModal t={t} item={announcement} onPrimary={runAnnouncement} onClose={() => setAnnouncement(null)} />
      )}

      <Onboarding t={t} open={onboardingOpen} prefs={prefs} setPrefs={setPrefs} daws={daws}
                  freePlugin={tagged.find(p => p.free)} signedIn={!!account.signedIn}
                  onInstallFree={() => { const f = tagged.find(p => p.free); if (f) install(f.id) }}
                  onAccount={goAccount} onClose={() => setOnboardingOpen(false)} />

      <Toasts toasts={toasts} onRemove={removeToast} />
    </div>
  )
}
