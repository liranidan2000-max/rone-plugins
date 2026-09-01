import React, { useState } from 'react'
import { motion } from 'framer-motion'
import ConfirmDialog from './ConfirmDialog'
import { callNative } from '../bridge'

function initials(name) {
  if (!name) return 'U'
  const p = name.trim().split(/\s+/)
  return (p.length === 1 ? p[0].substring(0, 2) : p[0][0] + p[p.length - 1][0]).toUpperCase()
}

function openUrl(url) {
  callNative('openExternalUrl', url).catch(() => {})
}

function formatDate(ms) {
  if (!ms) return null
  return new Date(ms).toLocaleDateString(undefined, { year: 'numeric', month: 'short', day: 'numeric' })
}

export default function AccountPanel({
  license, account = {}, onSignIn, onSignOut, onActivate, onDeactivate, pluginCount,
}) {
  const [email, setEmail] = useState('')
  const [password, setPassword] = useState('')
  const [busy, setBusy] = useState(false)
  const [message, setMessage] = useState('')
  const [signOutOpen, setSignOutOpen] = useState(false)

  // The license-key box is the fallback for customers who bought before
  // accounts existed — hidden behind a link so it never competes with sign-in.
  const [showKey, setShowKey] = useState(false)
  const [keyInput, setKeyInput] = useState('')
  const [activating, setActivating] = useState(false)

  const signedIn = !!account.signedIn
  const displayName = account.name || account.email || license.customerName || 'Guest'

  const handleSignIn = async () => {
    if (!email.trim() || !password) return
    setBusy(true); setMessage('')
    try {
      const res = await onSignIn(email.trim(), password)
      if (res?.ok) setPassword('')
      else setMessage(res?.message || 'Sign-in failed')
    } catch (e) {
      setMessage('Error: ' + (e.message || e))
    }
    setBusy(false)
  }

  const handleActivate = async () => {
    if (!keyInput.trim()) return
    setActivating(true); setMessage('Activating…')
    try {
      const r = await onActivate(keyInput.trim())
      if (!r?.started) { setActivating(false); setMessage(r?.message || r?.error || 'Activation failed') }
    } catch (e) { setActivating(false); setMessage('Error: ' + (e.message || e)) }
  }

  React.useEffect(() => {
    if (activating && (license.licensed || license.message)) {
      setActivating(false)
      if (license.licensed) setKeyInput('')
      if (license.message) setMessage(license.message)
    }
  }, [license.licensed, license.message])

  const renewLine = account.renewsAt
    ? `Renews ${formatDate(account.renewsAt)}`
    : account.expiresAt
      ? `Active until ${formatDate(account.expiresAt)}`
      : 'All plugins unlocked'

  return (
    <motion.div
      className="px-6 py-6 max-w-[760px]"
      initial={{ opacity: 0, y: 8 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}
    >
      <h2 className="font-display text-[18px] font-bold text-rone-text-primary mb-4">Account</h2>

      {/* Profile card */}
      <div className="pro-card rounded-2xl p-5 flex items-center gap-4">
        <div className="w-14 h-14 rounded-full border border-rone-border-2 flex items-center justify-center font-display text-base font-bold text-rone-purple"
             style={{
               background: 'radial-gradient(circle at 38% 30%, #363B42, #26292F 52%, #1A1C21)',
               boxShadow: '0 6px 14px rgba(0,0,0,0.4)',
             }}>
          {initials(displayName)}
        </div>
        <div className="flex-1 min-w-0">
          <p className="text-[15px] font-bold text-rone-text-primary truncate">{displayName}</p>
          <div className="flex items-center gap-2 mt-1">
            <span className={`px-2 py-0.5 text-[9px] font-extrabold tracking-[0.18em] rounded ${license.licensed ? 'bg-rone-purple/[0.07] text-rone-purple border border-rone-purple/35' : 'bg-rone-surface-3 text-rone-text-dim border border-rone-border'}`}>
              {license.licensed ? 'ALL ACCESS' : 'FREE'}
            </span>
            <span className="text-[12px] text-rone-text-dim truncate">
              {license.licensed ? renewLine : 'No active pass'}
            </span>
          </div>
        </div>
        {signedIn && (
          <button
            onClick={() => setSignOutOpen(true)}
            className="px-4 py-2 text-[12px] font-semibold text-rone-text-secondary border border-rone-border rounded-xl hover:text-rone-error hover:border-rone-error/40 transition-colors"
          >
            Sign out
          </button>
        )}
        {!signedIn && license.licensed && (
          <button
            onClick={() => setSignOutOpen(true)}
            className="px-4 py-2 text-[12px] font-semibold text-rone-text-secondary border border-rone-border rounded-xl hover:text-rone-error hover:border-rone-error/40 transition-colors"
          >
            Deactivate
          </button>
        )}
      </div>

      {/* Stats */}
      <div className="grid grid-cols-2 gap-4 mt-4">
        <div className="pro-card rounded-2xl p-4">
          <p className="text-[10px] font-bold text-rone-text-dim uppercase tracking-[0.16em]">Plugins installed</p>
          <p className="font-display text-[24px] font-bold text-rone-text-primary mt-1 tabular-nums">{pluginCount}</p>
        </div>
        <div className="pro-card rounded-2xl p-4">
          <p className="text-[10px] font-bold text-rone-text-dim uppercase tracking-[0.16em]">Pass status</p>
          <p className={`font-display text-[24px] font-bold mt-1 ${license.licensed ? 'text-rone-green' : 'text-rone-text-dim'}`}>
            {license.licensed ? 'Active' : 'Inactive'}
          </p>
        </div>
      </div>

      {/* Sign in (when no account is signed in and nothing else unlocked it) */}
      {!signedIn && !license.licensed && (
        <div className="pro-card rounded-2xl p-5 mt-4">
          <p className="text-[13px] font-semibold text-rone-text-primary">Sign in to unlock your plugins</p>
          <p className="text-[12px] text-rone-text-dim mt-1 mb-4">
            Use the same email and password as your account on roneaudio.com.
          </p>

          <div className="flex flex-col gap-3">
            <input
              type="email" autoComplete="email" placeholder="you@example.com"
              value={email} onChange={(e) => setEmail(e.target.value)}
              onKeyDown={(e) => e.key === 'Enter' && handleSignIn()}
              className="px-3 py-2.5 text-[13px] bg-rone-surface-2 border border-rone-border/60 rounded-xl text-rone-text-primary placeholder:text-rone-text-dim focus:outline-none focus:border-rone-purple/50"
            />
            <input
              type="password" autoComplete="current-password" placeholder="Password"
              value={password} onChange={(e) => setPassword(e.target.value)}
              onKeyDown={(e) => e.key === 'Enter' && handleSignIn()}
              className="px-3 py-2.5 text-[13px] bg-rone-surface-2 border border-rone-border/60 rounded-xl text-rone-text-primary placeholder:text-rone-text-dim focus:outline-none focus:border-rone-purple/50"
            />
            <button
              onClick={handleSignIn}
              disabled={busy || !email.trim() || !password}
              className="w-full py-2.5 text-[10px] font-extrabold uppercase tracking-[0.18em] rounded-lg btn-gradient disabled:opacity-50 disabled:cursor-not-allowed"
            >
              {busy ? 'Signing in…' : 'Sign in'}
            </button>
          </div>

          {(message || account.message) && (
            <p className="text-[11px] text-rone-error mt-3">{message || account.message}</p>
          )}

          <div className="flex items-center justify-between mt-4 text-[11px]">
            <a href="#" onClick={(e) => { e.preventDefault(); openUrl('https://roneaudio.com/account/signup.html') }}
               className="text-rone-purple font-semibold">Create an account</a>
            <a href="#" onClick={(e) => { e.preventDefault(); openUrl('https://roneaudio.com/account/forgot.html') }}
               className="text-rone-text-dim hover:text-rone-text-secondary">Forgot password?</a>
          </div>

          <div className="mt-4 pt-4 border-t border-rone-border/60">
            {!showKey ? (
              <button onClick={() => setShowKey(true)}
                      className="text-[11px] text-rone-text-dim hover:text-rone-text-secondary">
                Bought before accounts? Use a license key instead
              </button>
            ) : (
              <div className="flex items-center gap-3">
                <input
                  type="text" placeholder="Your RONE Full Bundle license key…"
                  value={keyInput} onChange={(e) => setKeyInput(e.target.value)}
                  onKeyDown={(e) => e.key === 'Enter' && handleActivate()}
                  className="flex-1 px-3 py-2 text-[12px] bg-rone-surface-2 border border-rone-border/60 rounded-xl text-rone-text-primary placeholder:text-rone-text-dim focus:outline-none focus:border-rone-purple/50"
                />
                <button
                  onClick={handleActivate}
                  disabled={activating || !keyInput.trim()}
                  className="px-5 py-2 text-[10px] font-extrabold uppercase tracking-[0.18em] rounded-lg border border-rone-border text-rone-text-secondary disabled:opacity-50 disabled:cursor-not-allowed"
                >
                  {activating ? 'Activating…' : 'Activate'}
                </button>
              </div>
            )}
          </div>
        </div>
      )}

      {/* Signed in: where to manage the pass */}
      {signedIn && (
        <div className="pro-card rounded-2xl p-5 mt-4">
          <p className="text-[13px] font-semibold text-rone-text-primary">{account.email}</p>
          <p className="text-[12px] text-rone-text-dim mt-1">
            This computer uses one of your {account.deviceLimit || 2} activations.
            Manage your pass and release computers on the website.
          </p>
          <button
            onClick={() => openUrl('https://roneaudio.com/account/')}
            className="mt-3 px-4 py-2 text-[10px] font-extrabold uppercase tracking-[0.18em] rounded-lg border border-rone-border text-rone-text-secondary hover:border-rone-purple/50 hover:text-rone-text-primary transition-colors"
          >
            Open my account
          </button>
          {account.message && (
            <p className="text-[11px] text-rone-text-dim mt-3">{account.message}</p>
          )}
        </div>
      )}

      <ConfirmDialog
        open={signOutOpen}
        title={signedIn ? 'Sign out' : 'Deactivate License'}
        message={signedIn
          ? 'Sign out of this computer? The plugins will lock until you sign in again, and this activation slot is freed up.'
          : 'Are you sure you want to deactivate your license? You will need to re-enter your license key to use the plugins.'}
        confirmLabel={signedIn ? 'Sign out' : 'Deactivate'} cancelLabel="Cancel" variant="danger"
        onConfirm={() => { setSignOutOpen(false); signedIn ? onSignOut() : onDeactivate() }}
        onCancel={() => setSignOutOpen(false)}
      />
    </motion.div>
  )
}
