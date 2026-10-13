// ---- What a message from the Center's backend says on the page ----
// The backend sends { text, type, code, params }: `text` is English (and what an
// error report carries), `code` + `params` let the page say it in the user's
// language. Pulled out of App.jsx (audit R032) so the rules can be tested without
// React or a WebView (test/messages.test.mjs).
//
// Returns null for an install that finished (the page announces it with its own
// toast and the DAW steps), else { text, type, sticky }.

export function translateStatus (m, t) {
  if (!m?.code) return { text: m?.text || '', type: m?.type || 'info', sticky: isSticky(m?.text) }
  const params = m.params || {}
  const nameRaw = params.name ? String(params.name) : ''
  const p = { ...params, name: nameRaw || params.id || '' }
  // What the installer itself said (Inno's log, the Mac installer's stderr -
  // audit R195, R281). Shown after the translated line, never instead of it.
  const reason = params.reason ? String(params.reason).trim() : ''

  if (m.code === 'installed') return null

  const key = 'msg.' + m.code
  if (m.code === 'download_failed' || !t(key) || t(key) === key) {
    return { text: m.text, type: m.type || 'info', sticky: isSticky(m.text) }
  }
  // 'failed' with no real installer exit code (0 or below) is a failure the Center
  // explains itself - why, and often how to fix it. The template turned that into
  // "installer code 0" (Andre, macOS, 12.10: "it says error 0"), so show the reason.
  if (m.code === 'failed' && m.text && !(Number(params.exitCode) > 0)) {
    return { text: m.text, type: m.type || 'error', sticky: isSticky(m.text) }
  }
  if (m.code === 'declined' || m.code === 'center_update_declined') {
    return { text: t(key, p), type: 'info', sticky: false }
  }
  const text = t(key, p) + (reason ? ' ' + reason : '')
  return { text, type: m.type || 'info', sticky: isSticky(m.text) }
}

// A message that tells the user how to fix something (the chown / Get Info line,
// audit R032) stays until it is closed: nine seconds is not enough to read it.
export function isSticky (text) {
  return typeof text === 'string' && text.includes('To fix it')
}

// What the page says when installPlugin answers { started: false, error }.
export function installErrorText (error, name, t) {
  if (error === 'License required') return t('msg.license_required', { name })
  if (error === 'Sign in required') return t('msg.sign_in_required', { name })   // audit R246
  return error
}
