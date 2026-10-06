import React, { useEffect, useState } from 'react'
import { Dialog } from '../components/ui'
import { openExternal } from '../bridge'
import { isGiftPass } from '../announcements'

function initials (name) {
  if (!name) return 'U'
  const p = name.trim().split(/\s+/)
  return (p.length === 1 ? p[0].substring(0, 2) : p[0][0] + p[p.length - 1][0]).toUpperCase()
}

const card = 'rounded-[14px] border border-rone-border bg-rone-card p-5'
const input = 'px-3 h-[40px] text-[13.5px] bg-[#101216] border border-rone-border-2 rounded-[10px] text-rone-text-primary placeholder:text-rone-text-faint outline-none focus:border-rone-purple/60'

export default function AccountView ({ t, locale, license, account = {}, onSignIn, onSignOut, onGoogleSignIn, onGoogleCancel,
                                       onActivate, onDeactivate, installedCount, ownedPlugins = [] }) {
  const [email, setEmail] = useState('')
  const [password, setPassword] = useState('')
  const [busy, setBusy] = useState(false)
  const [message, setMessage] = useState('')
  const [confirmOpen, setConfirmOpen] = useState(false)
  const [googleBusy, setGoogleBusy] = useState(false)
  const [showKey, setShowKey] = useState(false)
  const [keyInput, setKeyInput] = useState('')
  const [activating, setActivating] = useState(false)

  const signedIn = !!account.signedIn
  const displayName = account.name || account.email || license.customerName || ''
  const lifetime = ownedPlugins.filter(p => !p.free)
  const hasLifetime = lifetime.length > 0
  const gift = isGiftPass(account.passSource)
  const date = (ms) => ms ? new Date(ms).toLocaleDateString(locale, { year: 'numeric', month: 'short', day: 'numeric' }) : ''

  const google = async () => {
    setGoogleBusy(true); setMessage('')
    try { const r = await onGoogleSignIn(); if (!r?.ok) setMessage(r?.message || '') }
    catch (e) { setMessage(e.message || String(e)) }
    setGoogleBusy(false)
  }
  const signIn = async () => {
    if (!email.trim() || !password) return
    setBusy(true); setMessage('')
    try { const r = await onSignIn(email.trim(), password); if (r?.ok) setPassword(''); else setMessage(r?.message || '') }
    catch (e) { setMessage(e.message || String(e)) }
    setBusy(false)
  }
  const activate = async () => {
    if (!keyInput.trim()) return
    setActivating(true); setMessage(t('acc.activating'))
    try { const r = await onActivate(keyInput.trim()); if (!r?.started) { setActivating(false); setMessage(r?.message || r?.error || '') } }
    catch (e) { setActivating(false); setMessage(e.message || String(e)) }
  }
  useEffect(() => {
    if (activating && (license.licensed || license.message)) {
      setActivating(false)
      if (license.licensed) setKeyInput('')
      if (license.message) setMessage(license.message)
    }
  }, [license.licensed, license.message])

  // A gift never shows its holder a date (memory: gifts have hidden dates).
  const passLine = gift ? t('acc.allUnlocked')
    : account.renewsAt ? t('acc.renews', { date: date(account.renewsAt) })
    : account.expiresAt ? t('acc.until', { date: date(account.expiresAt) }) : t('acc.allUnlocked')

  return (
    <div className="max-w-[780px] flex flex-col gap-3.5">
      <h1 className="m-0 font-display font-extrabold text-[24px] text-rone-text-primary">{t('acc.title')}</h1>

      <div className={card + ' flex items-center gap-4'}>
        <div className="w-14 h-14 rounded-full border border-rone-border-2 grid place-items-center font-display text-base font-bold text-rone-purple"
             style={{ background: 'radial-gradient(circle at 38% 30%, #363B42, #26292F 52%, #1A1C21)' }}>{initials(displayName)}</div>
        <div className="flex-1 min-w-0">
          <p className="m-0 text-[16px] font-bold text-rone-text-primary truncate">{displayName || '—'}</p>
          <div className="flex items-center gap-2 mt-1">
            <span className={`pill ${license.licensed || hasLifetime ? 'pill-own' : ''}`}>
              {license.licensed ? t('acc.badgeAll') : hasLifetime ? t('acc.badgeLife') : t('acc.badgeFree')}
            </span>
            <span className="text-[12.5px] text-rone-text-dim truncate">
              {license.licensed ? passLine : hasLifetime ? t('acc.owned', { n: lifetime.length }) : t('acc.noPass')}
            </span>
          </div>
        </div>
        {(signedIn || license.licensed) && (
          <button className="btn btn-out" onClick={() => setConfirmOpen(true)}>{signedIn ? t('acc.signOut') : t('acc.deactivate')}</button>
        )}
      </div>

      <div className="grid grid-cols-2 gap-3.5">
        <div className={card}>
          <p className="m-0 text-[11px] font-extrabold text-rone-text-dim uppercase tracking-[0.14em]">{t('acc.installed')}</p>
          <p className="m-0 mt-1 font-display text-[26px] font-bold text-rone-text-primary num">{installedCount}</p>
        </div>
        <div className={card}>
          <p className="m-0 text-[11px] font-extrabold text-rone-text-dim uppercase tracking-[0.14em]">{t('acc.passStatus')}</p>
          <p className={`m-0 mt-1 font-display text-[26px] font-bold ${license.licensed ? 'text-rone-green' : 'text-rone-text-dim'}`}>
            {license.licensed ? t('acc.active') : t('acc.inactive')}
          </p>
        </div>
      </div>

      {(hasLifetime || (signedIn && !license.licensed)) && (
        <div className={card}>
          <p className="m-0 text-[11px] font-extrabold text-rone-text-dim uppercase tracking-[0.14em]">{t('acc.lifetime')}</p>
          {hasLifetime ? (
            <>
              <div className="flex flex-wrap gap-2 mt-3">
                {lifetime.map(p => <span key={p.id} className="pill pill-own normal-case tracking-normal text-[12px]">✓ {p.name}</span>)}
              </div>
              <p className="m-0 mt-3 text-[12px] text-rone-text-dim">{t('acc.lifetimeNote', { n: account.deviceLimit || 2 })}</p>
            </>
          ) : <p className="m-0 mt-2 text-[12.5px] text-rone-text-dim">{t('acc.noLifetime')}</p>}
        </div>
      )}

      {!signedIn && !license.licensed && (
        <div className={card}>
          <p className="m-0 text-[15px] font-bold text-rone-text-primary">{t('acc.signInTitle')}</p>
          <p className="m-0 mt-1 mb-4 text-[12.5px] text-rone-text-dim">{t('acc.signInSub')}</p>

          {googleBusy ? (
            <div className="rounded-[10px] border border-rone-border-2 px-4 py-3 mb-3">
              <p className="m-0 text-[13px] font-semibold text-rone-text-primary">{t('acc.waitingBrowser')}</p>
              <p className="m-0 mt-0.5 text-[12px] text-rone-text-dim">{t('acc.waitingBrowserSub')}</p>
              <button onClick={onGoogleCancel} className="btn btn-ghost btn-sm mt-1 px-0">{t('dlg.cancel')}</button>
            </div>
          ) : (
            <button onClick={google} disabled={busy}
                    className="w-full flex items-center justify-center gap-2.5 h-[42px] mb-3 rounded-[10px] bg-white text-[#1F1F1F] text-[13px] font-bold hover:bg-[#F1F1F1] disabled:opacity-50">
              <svg viewBox="0 0 48 48" width="16" height="16" aria-hidden="true">
                <path fill="#EA4335" d="M24 9.5c3.54 0 6.71 1.22 9.21 3.6l6.85-6.85C35.9 2.38 30.47 0 24 0 14.62 0 6.51 5.38 2.56 13.22l7.98 6.19C12.43 13.72 17.74 9.5 24 9.5z"/>
                <path fill="#4285F4" d="M46.98 24.55c0-1.57-.15-3.09-.38-4.55H24v9.02h12.94c-.58 2.96-2.26 5.48-4.78 7.18l7.73 6c4.51-4.18 7.09-10.36 7.09-17.65z"/>
                <path fill="#FBBC05" d="M10.53 28.59c-.48-1.45-.76-2.99-.76-4.59s.27-3.14.76-4.59l-7.98-6.19C.92 16.46 0 20.12 0 24c0 3.88.92 7.54 2.56 10.78l7.97-6.19z"/>
                <path fill="#34A853" d="M24 48c6.48 0 11.93-2.13 15.89-5.81l-7.73-6c-2.15 1.45-4.92 2.3-8.16 2.3-6.26 0-11.57-4.22-13.47-9.91l-7.98 6.19C6.51 42.62 14.62 48 24 48z"/>
              </svg>
              {t('acc.google')}
            </button>
          )}
          <div className="flex items-center gap-3 mb-3 text-[10.5px] font-extrabold uppercase tracking-[0.16em] text-rone-text-faint">
            <span className="flex-1 h-px bg-rone-border" />{t('acc.orEmail')}<span className="flex-1 h-px bg-rone-border" />
          </div>
          <form className="flex flex-col gap-2.5" onSubmit={(e) => { e.preventDefault(); signIn() }}>
            <input className={input} type="email" autoComplete="email" placeholder={t('acc.email')} aria-label="E-mail"
                   value={email} onChange={(e) => setEmail(e.target.value)} />
            <input className={input} type="password" autoComplete="current-password" placeholder={t('acc.password')} aria-label={t('acc.password')}
                   value={password} onChange={(e) => setPassword(e.target.value)} />
            <button type="submit" className="btn btn-pri h-[40px]" disabled={busy || !email.trim() || !password}>
              {busy ? t('acc.signingIn') : t('acc.signIn')}
            </button>
          </form>
          {(message || account.message) && <p className="m-0 mt-3 text-[12px] text-rone-error" role="alert">{message || account.message}</p>}
          <div className="flex items-center justify-between mt-4 text-[12px]">
            <button onClick={() => openExternal('https://roneaudio.com/account/signup.html?utm_source=plugins_center')} className="text-rone-purple font-bold">{t('acc.create')}</button>
            <button onClick={() => openExternal('https://roneaudio.com/account/forgot.html')} className="text-rone-text-dim hover:text-rone-text-secondary">{t('acc.forgot')}</button>
          </div>
          <div className="mt-4 pt-4 border-t border-rone-border">
            {!showKey ? (
              <button onClick={() => setShowKey(true)} className="text-[12px] text-rone-text-dim hover:text-rone-text-secondary">{t('acc.useKey')}</button>
            ) : (
              <div className="flex items-center gap-2.5">
                <input className={input + ' flex-1'} placeholder={t('acc.keyPlaceholder')} aria-label={t('acc.keyPlaceholder')}
                       value={keyInput} onChange={(e) => setKeyInput(e.target.value)} onKeyDown={(e) => e.key === 'Enter' && activate()} />
                <button className="btn btn-out" onClick={activate} disabled={activating || !keyInput.trim()}>{activating ? t('acc.activating') : t('acc.activate')}</button>
              </div>
            )}
          </div>
        </div>
      )}

      {signedIn && (
        <div className={card}>
          <p className="m-0 text-[14px] font-bold text-rone-text-primary">{account.email}</p>
          <p className="m-0 mt-1 text-[12.5px] text-rone-text-dim">{t('acc.deviceNote', { n: account.deviceLimit || 2 })}</p>
          <button className="btn btn-out mt-3" onClick={() => openExternal('https://roneaudio.com/account/')}>{t('acc.openAccount')}</button>
          {account.message && <p className="m-0 mt-3 text-[12px] text-rone-text-dim">{account.message}</p>}
        </div>
      )}

      <Dialog open={confirmOpen} onClose={() => setConfirmOpen(false)} label={signedIn ? t('acc.signOut') : t('acc.deactivateTitle')} width={380}>
        <h3 className="m-0 mb-2 text-[15px] font-extrabold text-rone-text-primary">{signedIn ? t('acc.signOut') : t('acc.deactivateTitle')}</h3>
        <p className="m-0 mb-5 text-[12.5px] text-rone-text-secondary leading-relaxed">{signedIn ? t('acc.signOutBody') : t('acc.deactivateBody')}</p>
        <div className="flex justify-end gap-2">
          <button className="btn btn-out" onClick={() => setConfirmOpen(false)} data-autofocus>{t('dlg.cancel')}</button>
          <button className="btn btn-danger" onClick={() => { setConfirmOpen(false); signedIn ? onSignOut() : onDeactivate() }}>
            {signedIn ? t('acc.signOut') : t('acc.deactivate')}
          </button>
        </div>
      </Dialog>
    </div>
  )
}
