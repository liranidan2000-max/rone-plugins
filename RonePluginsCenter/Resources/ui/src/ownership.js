// ---- The one ownership comparison ----
// An owned id travels server -> C++ -> here, and every hop has its own idea of
// casing and padding. BundleLicenseChecker and MainComponent settled on one
// rule — trim, ignore case, whole token, never a substring — so the UI has to
// use exactly that one or a customer who paid still sees a lock.
export const productKey = (id) => (typeof id === 'string' ? id.trim().toLowerCase() : '')

// The server sends an array; the licence file keeps the same ids comma-joined.
// Accept either shape, drop the blanks, and keep the first spelling of each
// product. Splitting on commas is what makes the compare a whole-token one:
// "RoneStut" must never match "RoneStutter".
export function ownedProductIds (owned) {
  const entries = Array.isArray(owned) ? owned : typeof owned === 'string' ? [owned] : []
  const seen = new Set()
  const ids = []
  for (const entry of entries) {
    if (typeof entry !== 'string') continue
    for (const token of entry.split(',')) {
      const id = token.trim()
      const key = productKey(id)
      if (key === '' || seen.has(key)) continue
      seen.add(key)
      ids.push(id)
    }
  }
  return ids
}
