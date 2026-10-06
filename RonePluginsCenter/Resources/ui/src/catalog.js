// ---- Catalog helpers (pure: node tests them, test/catalog.test.mjs) ----
// Everything the page decides about a plugin from its manifest entry alone:
// its colour, its categories, whether it is new, what changed, whether it
// matches a search, where it sorts. The manifest fields are optional - an
// older manifest (or a plugin added before the field existed) falls back to
// what is known here, and nothing breaks.

// One neon per product (the bundle's accent registry). The manifest's own
// `accent` wins; this keeps the page right for a manifest without it.
export const ACCENTS = {
  ReverseReverb: '#2BD9FF', RoneStutter: '#FFD02B', RoneStucker: '#9D6BFF', RoneThrow: '#D8E4EC',
  RoneClipper: '#3D8BFF', RoneRise: '#FF5FB8', RoneIron: '#E552FF', RoneFlanger: '#FF3E6C',
  RoneAfterspace: '#FF8A3D', RONEAnalyzer: '#2DD4BF', RoneMotion: '#3DE8A0', RoneBassline: '#B6FF2E',
  RoneChoir: '#F2A93B', RoneControl: '#52FF45',
}
export const CENTER_PURPLE = '#9D6BFF'

export function accentOf (plugin) {
  const a = typeof plugin?.accent === 'string' ? plugin.accent.trim() : ''
  if (/^#[0-9a-fA-F]{6}$/.test(a)) return a
  return ACCENTS[plugin?.id] || CENTER_PURPLE
}

// Accent arithmetic happens here, not in CSS color-mix(): on the Mac the page
// runs in the system WebKit, and before Safari 16.2 a color-mix() value is
// thrown away - and with it the whole background or shadow it sits in.
function rgbOf (hex) {
  const m = /^#?([0-9a-fA-F]{6})$/.exec(String(hex || '').trim())
  const n = parseInt(m ? m[1] : CENTER_PURPLE.slice(1), 16)
  return [(n >> 16) & 255, (n >> 8) & 255, n & 255]
}

// The accent at `alpha` opacity: what color-mix(in srgb, hex N%, transparent) gives.
export function tint (hex, alpha) {
  const [r, g, b] = rgbOf(hex)
  return `rgba(${r}, ${g}, ${b}, ${alpha})`
}

// `weight` of `hex` over `base`, opaque: color-mix(in srgb, hex N%, base).
export function mixHex (hex, weight, base) {
  const a = rgbOf(hex), b = rgbOf(base)
  const c = a.map((v, i) => Math.round(v * weight + b[i] * (1 - weight)))
  return `rgb(${c[0]}, ${c[1]}, ${c[2]})`
}

// What a producer reaches for it for. Same idea as the accent: the manifest's
// `category` wins, this is the fallback.
export const CATEGORY_KEYS = ['transitions', 'space', 'rhythm', 'vocal', 'mix']
const FALLBACK_CATEGORIES = {
  ReverseReverb: ['transitions', 'space'], RoneStutter: ['rhythm'], RoneStucker: ['rhythm', 'transitions'],
  RoneThrow: ['space'], RoneClipper: ['mix'], RoneRise: ['transitions'], RoneIron: ['vocal'],
  RoneFlanger: ['transitions'], RoneAfterspace: ['space'], RONEAnalyzer: ['mix'],
}
const FALLBACK_TAGS = {
  ReverseReverb: ['riser', 'reverse', 'swell', 'reverb'], RoneStutter: ['stutter', 'gate', 'glitch', 'fill'],
  RoneStucker: ['loop roll', 'buffer', 'repeat', 'fill'], RoneThrow: ['delay', 'throw', 'dub', 'echo'],
  RoneClipper: ['clipper', 'loudness', 'master', 'limiter'], RoneRise: ['build-up', 'riser', 'transition', 'sweep'],
  RoneIron: ['vocal chop', 'slicer', 'metallic', 'vocal'], RoneFlanger: ['flanger', 'sweep', 'jet', 'riser'],
  RoneAfterspace: ['reverb', 'delay', 'atmosphere', 'ambient'], RONEAnalyzer: ['spectrum', 'loudness', 'lufs', 'reference'],
}

export function categoriesOf (plugin) {
  const c = Array.isArray(plugin?.categories) ? plugin.categories.filter(x => CATEGORY_KEYS.includes(x)) : []
  return c.length ? c : (FALLBACK_CATEGORIES[plugin?.id] || [])
}

export function tagsOf (plugin) {
  const t = Array.isArray(plugin?.tags) ? plugin.tags.filter(x => typeof x === 'string' && x.trim()) : []
  return t.length ? t : (FALLBACK_TAGS[plugin?.id] || [])
}

// "RONE Reverse Reverb" -> "Reverse Reverb": the brand is said once, in small type.
export function shortName (plugin) {
  const n = String(plugin?.name || plugin?.id || '')
  return n.replace(/^RONE\s+/i, '') || n
}

// The manifest's description, without the lead-in older Centers needed
// ("FREE plugin - sign in to the Center with a free RONE account and it unlocks.")
// - this page says "free" with its own badge and button.
export function descriptionOf (plugin) {
  return String(plugin?.description || '').replace(/^FREE plugin\s*[-–]\s*[^.]*\.\s*/i, '').trim()
}

// Released in the last 30 days (manifest `released`, "YYYY-MM-DD").
export function isNew (plugin, now = Date.now()) {
  const t = Date.parse(plugin?.released || '')
  return Number.isFinite(t) && now - t >= 0 && now - t < 30 * 86400000
}

// ---- What's new ----
// whats_new is written by hand as "1.1.10 - text. 1.1.9 - text. ...", newest
// first. Split it into versions, and each version into sentences.
export function parseChangelog (whatsNew) {
  const text = String(whatsNew || '').trim()
  if (!text) return []
  const re = /(?:^|\s)(\d+\.\d+(?:\.\d+){0,2})\s+[-–—]\s+/g
  const marks = []
  let m
  while ((m = re.exec(text)) !== null) marks.push({ version: m[1], at: m.index + m[0].length, start: m.index })
  if (!marks.length) return [{ version: '', items: splitSentences(text) }]
  return marks.map((mk, i) => ({
    version: mk.version,
    items: splitSentences(text.slice(mk.at, i + 1 < marks.length ? marks[i + 1].start : text.length)),
  }))
}

function splitSentences (s) {
  return s.split(/(?<=[.!?])\s+(?=[A-Z0-9"'(])/)
    .map(x => x.trim().replace(/\s+/g, ' '))
    .filter(Boolean)
}

// Compare "1.2.3" style versions numerically (missing parts are 0).
export function compareVersions (a, b) {
  const pa = String(a || '').split('.').map(n => parseInt(n, 10) || 0)
  const pb = String(b || '').split('.').map(n => parseInt(n, 10) || 0)
  for (let i = 0; i < Math.max(pa.length, pb.length); i++) {
    const d = (pa[i] || 0) - (pb[i] || 0)
    if (d) return d < 0 ? -1 : 1
  }
  return 0
}

// The changelog entries an update brings: newer than what is installed. An
// unknown installed version ("?" or empty) gets the latest entry only.
export function changesSince (plugin) {
  const log = parseChangelog(plugin?.whatsNew)
  const installed = String(plugin?.installedVersion || '')
  if (!installed || installed === '?') return log.slice(0, 1)
  const base = installed.split('.').slice(0, 3).join('.')
  const newer = log.filter(e => e.version && compareVersions(e.version, base) > 0)
  return newer.length ? newer : log.slice(0, 1)
}

// ---- Search ----
// A producer types what they need ("riser", "vocal chop", "delay") as often
// as a name. Every word has to hit the name, the description, a tag or a
// category.
export function matchesSearch (plugin, query, categoryLabel = (c) => c) {
  const words = String(query || '').toLowerCase().split(/\s+/).filter(Boolean)
  if (!words.length) return true
  const hay = [
    plugin?.name, plugin?.description, ...tagsOf(plugin),
    ...categoriesOf(plugin), ...categoriesOf(plugin).map(categoryLabel),
  ].join(' ').toLowerCase()
  return words.every(w => hay.includes(w))
}

// ---- States ----
export const BUSY = new Set(['queued', 'downloading', 'ready', 'installing', 'waiting', 'uninstalling'])
export const isBusy = (p) => BUSY.has(p?.status)
export const isInstalled = (p) => p?.status === 'up_to_date' || p?.status === 'update_available'

// Locked = nothing this account holds opens it. Free plugins open with any account.
export function isUnlocked (plugin, { licensed, signedIn }) {
  return !!licensed || plugin?.owned === true || (plugin?.free === true && !!signedIn)
}

// "Updates" counts plugins with a newer version that this account can install -
// never a plugin that was never installed (before 2.0 a new user saw "9 updates").
export function updatable (plugins, access) {
  return (plugins || []).filter(p => p.status === 'update_available' && isUnlocked(p, access))
}

// ---- Order: yours first ----
// Updates and work in progress, then what you own and have, then what you can
// install, then new, then the rest. Within a group the manifest's order stays.
export function sortPlugins (plugins, access, mode = 'yours', now = Date.now()) {
  const list = (plugins || []).map((p, i) => ({ p, i }))
  if (mode === 'name') list.sort((a, b) => shortName(a.p).localeCompare(shortName(b.p)))
  else if (mode === 'new') {
    list.sort((a, b) => (Date.parse(b.p.released || '') || 0) - (Date.parse(a.p.released || '') || 0) || a.i - b.i)
  } else {
    const rank = (p) => {
      const un = isUnlocked(p, access)
      if (!access.signedIn && !access.licensed) return p.free ? 0 : isNew(p, now) ? 1 : 2
      if (un && (p.status === 'update_available' || isBusy(p))) return 0
      if (un && isInstalled(p)) return 1
      if (un) return 2
      if (isNew(p, now)) return 3
      return 4
    }
    list.sort((a, b) => rank(a.p) - rank(b.p) || a.i - b.i)
  }
  return list.map(x => x.p)
}

// ---- Prices ----
// The live price (launch price during a sale) and the regular one; null when
// the plugin is not sold on its own (or free).
export function priceOf (plugin) {
  const regular = Number(plugin?.price)
  if (!Number.isFinite(regular) || regular <= 0) return null
  const launch = Number(plugin?.launch_price)
  const live = Number.isFinite(launch) && launch > 0 && launch < regular ? launch : regular
  return { live, regular, onSale: live < regular }
}
export const usd = (n) => '$' + (Number.isInteger(n) ? n : Number(n).toFixed(2))

// ---- Tips ----
// One tip a week from the manifest's `tips`, the same for everyone that week.
export function tipOfTheWeek (tips, now = Date.now()) {
  const list = Array.isArray(tips) ? tips.filter(t => t && typeof t.text === 'string' && t.text.trim()) : []
  if (!list.length) return null
  const week = Math.floor(now / (7 * 86400000))
  return list[week % list.length]
}

// Bytes -> "48 MB"
export function formatSize (bytes) {
  const b = Number(bytes)
  if (!Number.isFinite(b) || b <= 0) return ''
  return b >= 1024 * 1024 ? Math.round(b / (1024 * 1024)) + ' MB' : Math.max(1, Math.round(b / 1024)) + ' KB'
}
