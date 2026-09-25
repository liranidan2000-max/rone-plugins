import { productKey } from './ownership.js'

// ---- Announcements: the website's popups, in the Center ----
// roneaudio.com/api/v1/popup is managed in the admin console, and a plugin added
// to the catalog gets its popup there by itself - so it reaches the Center too.
// A popup is shown once (per id), at most one a day, and only when it means
// something here: its plugin is not installed or has an update. The page lives
// in AppData (WebView2 user data folder), so this memory survives restarts.
const SEEN_KEY = 'rone_center_popups_seen'
const LAST_KEY = 'rone_center_popup_last'

function readSeen () {
  try {
    const v = JSON.parse(localStorage.getItem(SEEN_KEY) || '[]')
    return Array.isArray(v) ? v : []
  } catch { return [] }
}

export function rememberAnnouncement (id) {
  try {
    const seen = readSeen().filter((x) => x !== id)
    seen.push(id)
    localStorage.setItem(SEEN_KEY, JSON.stringify(seen.slice(-50)))
    localStorage.setItem(LAST_KEY, String(Date.now()))
  } catch {}
}

// A site link carries where it came from, so the console can count it.
function siteUrl (path, tag) {
  try {
    const u = new URL(path || '/', 'https://roneaudio.com/')
    if (u.origin !== 'https://roneaudio.com') return 'https://roneaudio.com/'
    u.searchParams.set('utm_source', 'plugins_center')
    u.searchParams.set('utm_medium', 'popup')
    u.searchParams.set('utm_campaign', 'center_' + String(tag || 'popup').toLowerCase())
    return u.toString()
  } catch { return 'https://roneaudio.com/' }
}

// A pass Liran gave by hand is billed by nobody: the server names its source
// 'comp' (entitlements.passSource). Its holder must never be offered FOUNDERS
// or asked to buy the pass (Liran, 2026-09-25). Anything else - 'paddle',
// 'lemonsqueezy', null for a giveaway trial, or no field at all from an older
// Center's saved account - reads as "not a gift" and changes nothing.
export const isGiftPass = (passSource) => productKey(passSource) === 'comp'

// The first popup that applies here, with what its button does. The card's own
// rule decides "can install": the pass, or the plugin in the account's owned list
// (which the server fills with the free plugins for every account). A deal is
// skipped for a gift pass - not remembered, so it could still greet the same
// person once the gift is over. Plugin and free popups show to a gift as before.
export function pickAnnouncement (popups, { plugins, ownedKeys, licensed, passSource, ignoreMemory = false }) {
  if (!ignoreMemory) {
    let last = 0
    try { last = Number(localStorage.getItem(LAST_KEY)) || 0 } catch {}
    if (Date.now() - last < 20 * 3600 * 1000) return null
  }
  const seen = ignoreMemory ? new Set() : new Set(readSeen())
  const giftPass = isGiftPass(passSource)

  for (const popup of popups || []) {
    if (!popup || typeof popup.id !== 'string' || seen.has(popup.id)) continue
    if (popup.kind === 'deal' && giftPass) continue
    const productId = popup.id.split(':')[1] || ''

    if ((popup.kind === 'plugin' || popup.kind === 'free') && productId) {
      const plugin = plugins.find((p) => productKey(p.id) === productKey(productId))
      if (!plugin) continue                                              // not in this Center's list
      if (!['not_installed', 'update_available', 'error'].includes(plugin.status)) continue   // has it, or busy

      const canInstall = licensed || ownedKeys.has(productKey(plugin.id))
      if (canInstall) {
        return {
          popup, plugin,
          primary: { kind: 'install', label: plugin.status === 'update_available' ? 'Update now' : 'Install now' },
          priceLine: popup.kind === 'free' ? (popup.price || '')
                   : licensed ? 'Included in your ALL ACCESS pass' : 'In your account',
        }
      }
      return {
        popup, plugin,
        primary: { kind: 'url', label: popup.cta?.label || 'See it', url: siteUrl(popup.cta?.url, productId) },
        priceLine: popup.price || '',
      }
    }

    if (popup.cta?.url) {
      return {
        popup, plugin: null,
        primary: { kind: 'url', label: popup.cta.label || 'See it', url: siteUrl(popup.cta.url, productId || popup.kind) },
        priceLine: popup.price || '',
      }
    }
  }
  return null
}
