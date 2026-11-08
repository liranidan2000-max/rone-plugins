import React, { useEffect, useMemo, useRef, useState } from 'react'
import { Icon } from '../components/ui'
import { api, isDevMode } from '../bridge'
import { accentOf, shortName, tint } from '../catalog'

// ============================================================================
// SUPPORT (2026-11-08): talk to the developer without leaving the Center.
//
// Two doors, like the site's form: NEED HELP (something is wrong) and IMPROVE
// (an idea, a missing feature). Both go to roneaudio.com/api/v1/support through
// the native sendSupport (MainComponent::handleSendSupport), so they land in
// Liran's console with the site's messages. What the site cannot know comes
// along by itself: the Center version, the OS and every installed plugin's
// version (the "Sent with it" box shows exactly what, and can be switched off).
//
// Screenshots: picked, pasted (Ctrl+V anywhere on the page) or dropped; up to 5,
// a big one is shrunk to JPEG first. The server keeps only real images.
// A half-written message survives leaving the page (localStorage draft).
// ============================================================================

const MAX_PICS = 5
const SHRINK_ABOVE = 2.5 * 1024 * 1024
const MAX_SIDE = 2560
const DRAFT_KEY = 'rone_center_support_draft'
const TOPICS = ['plugins', 'license', 'center', 'other']
const EMAIL_RE = /^[^\s@]+@[^\s@]+\.[^\s@]+$/
const picOk = (f) => !!f && (/^image\/(png|jpe?g|gif|webp|heic|heif)$/i.test(f.type || '') || (!f.type && /\.(heic|heif)$/i.test(f.name || '')))

const field = 'w-full px-3 h-[40px] text-[13.5px] bg-[#101216] border border-rone-border-2 rounded-[10px] text-rone-text-primary placeholder:text-rone-text-faint outline-none focus:border-rone-purple/60 transition-colors'
const label = 'block mb-1.5 text-[10.5px] font-extrabold tracking-[0.18em] uppercase text-rone-text-dim'

const ChatIcon = (p) => <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" {...p}><path d="M4 5h16v11H9l-5 4z" /><path d="M8.5 9.5h7M8.5 12.5h4" /></svg>
const SparkIcon = (p) => <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" {...p}><path d="M12 3v4M12 17v4M3 12h4M17 12h4M6 6l2.5 2.5M15.5 15.5L18 18M18 6l-2.5 2.5M8.5 15.5L6 18" /></svg>
const CamIcon = (p) => <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinejoin="round" {...p}><path d="M4 7h3l2-2.5h6L17 7h3v12H4z" /><circle cx="12" cy="13" r="3.6" /></svg>

function readDraft () {
  try { return JSON.parse(localStorage.getItem(DRAFT_KEY) || 'null') || {} } catch { return {} }
}

const asDataUrl = (blob) => new Promise((resolve, reject) => {
  const r = new FileReader()
  r.onload = () => resolve(String(r.result))
  r.onerror = () => reject(r.error)
  r.readAsDataURL(blob)
})

async function shrink (file) {
  if (file.size <= SHRINK_ABOVE || /gif|heic|heif/i.test(file.type)) return file
  try {
    const bmp = await createImageBitmap(file)
    const k = Math.min(1, MAX_SIDE / Math.max(bmp.width, bmp.height))
    const c = document.createElement('canvas')
    c.width = Math.round(bmp.width * k); c.height = Math.round(bmp.height * k)
    const g = c.getContext('2d')
    g.fillStyle = '#fff'; g.fillRect(0, 0, c.width, c.height)
    g.drawImage(bmp, 0, 0, c.width, c.height)
    const out = await new Promise((res) => c.toBlob(res, 'image/jpeg', 0.9))
    return out && out.size < file.size ? out : file
  } catch { return file }
}

function Door ({ on, onClick, icon: I, title, body }) {
  return (
    <button type="button" role="tab" aria-selected={on} onClick={onClick}
            className={`flex-1 min-w-[220px] flex items-start gap-3 p-4 rounded-[14px] border text-left transition-all
                        ${on ? 'border-rone-purple/70 bg-rone-purple/[0.07] shadow-[0_0_24px_rgb(var(--purple)/0.12)]' : 'border-rone-border bg-rone-card hover:border-rone-border-3'}`}>
      <span className={`flex-none w-[38px] h-[38px] rounded-[11px] grid place-items-center ${on ? 'bg-rone-purple text-rone-neon-dark' : 'bg-white/[0.05] text-rone-text-dim'}`}>
        <I className="w-[19px] h-[19px]" />
      </span>
      <span className="min-w-0">
        <b className={`block font-display text-[14.5px] tracking-[0.04em] ${on ? 'text-rone-text-primary' : 'text-rone-text-secondary'}`}>{title}</b>
        <span className="block mt-0.5 text-[12.5px] leading-snug text-rone-text-dim">{body}</span>
      </span>
    </button>
  )
}

export default function SupportView ({ t, plugins, account, platform, appVersion, onNavigate }) {
  const draft = useMemo(readDraft, [])
  const [kind, setKind] = useState(draft.kind === 'idea' ? 'idea' : 'help')
  const [email, setEmail] = useState(account.email || draft.email || '')
  const [name, setName] = useState(account.name || draft.name || '')
  const [topic, setTopic] = useState(TOPICS.includes(draft.topic) ? draft.topic : 'plugins')
  const [plugin, setPlugin] = useState(draft.plugin || '')
  const [subject, setSubject] = useState(draft.subject || '')
  const [message, setMessage] = useState(draft.message || '')
  const [withSystem, setWithSystem] = useState(true)
  const [showSystem, setShowSystem] = useState(false)
  const [pics, setPics] = useState([])             // { name, data, url }
  const [drag, setDrag] = useState(false)
  const [busy, setBusy] = useState(false)
  const [err, setErr] = useState('')
  const [sent, setSent] = useState(null)           // the address the answer goes to
  const fileInput = useRef(null)
  const root = useRef(null)

  // The account's address wins once it is known (it can arrive after the page opened).
  useEffect(() => { if (account.email && !email) setEmail(account.email) }, [account.email])  // eslint-disable-line react-hooks/exhaustive-deps

  useEffect(() => {
    try { localStorage.setItem(DRAFT_KEY, JSON.stringify({ kind, email, name, topic, plugin, subject, message })) } catch {}
  }, [kind, email, name, topic, plugin, subject, message])

  useEffect(() => () => pics.forEach(p => URL.revokeObjectURL(p.url)), [])  // eslint-disable-line react-hooks/exhaustive-deps

  const products = useMemo(() => plugins.filter(p => p.kind !== 'app' || p.unlocked), [plugins])
  const installed = useMemo(() => products.filter(p => p.installedVersion && p.installedVersion !== '?'), [products])
  const ordered = useMemo(() => [...installed, ...products.filter(p => !installed.includes(p))], [installed, products])
  const picked = products.find(p => p.id === plugin)

  const systemLines = useMemo(() => {
    const lines = []
    if (picked) lines.push(`About: ${picked.name || 'RONE ' + shortName(picked)}${picked.installedVersion ? ' ' + picked.installedVersion : ' (not installed)'}`)
    if (installed.length) lines.push('Installed: ' + installed.map(p => `${shortName(p)} ${p.installedVersion}`).join(', '))
    else lines.push('Installed: no RONE plugins')
    return lines
  }, [picked, installed])

  async function addPics (list) {
    const files = [...(list || [])]
    if (!files.length) return
    setErr('')
    const next = []
    let room = MAX_PICS - pics.length
    for (const f of files.filter(picOk)) {
      if (room <= 0) { setErr(t('sup.tooManyPics', { n: MAX_PICS })); break }
      const small = await shrink(f)
      if (small.size > 8 * 1024 * 1024) { setErr(t('sup.picTooBig')); continue }
      room -= 1
      const data = await asDataUrl(small)
      let n = f.name || 'screenshot.png'
      if (small !== f) n = n.replace(/\.[^.]+$/, '') + '.jpg'
      next.push({ name: n, data: data.slice(data.indexOf(',') + 1), url: URL.createObjectURL(small) })
    }
    if (files.some(f => !picOk(f))) setErr(t('sup.onlyPics'))
    if (next.length) setPics(prev => [...prev, ...next].slice(0, MAX_PICS))
  }

  // Ctrl+V anywhere on the page: a screenshot becomes an attachment; text pastes as usual.
  useEffect(() => {
    const onPaste = (e) => {
      if (sent) return
      const files = [...(e.clipboardData?.files || [])].filter(picOk)
      if (!files.length) return
      e.preventDefault()
      addPics(files)
    }
    window.addEventListener('paste', onPaste)
    return () => window.removeEventListener('paste', onPaste)
  })

  const removePic = (i) => setPics(prev => { URL.revokeObjectURL(prev[i].url); return prev.filter((_, j) => j !== i) })

  async function send (e) {
    e.preventDefault()
    setErr('')
    const idea = kind === 'idea'
    if (!EMAIL_RE.test(email.trim())) { setErr(t('sup.needEmail')); return }
    if (idea && subject.trim().length < 3) { setErr(t('sup.needSubject')); return }
    if (message.trim().length < 5) { setErr(t(idea ? 'sup.needIdea' : 'sup.needWords')); return }

    const body = {
      name: name.trim(), email: email.trim(), kind,
      topic: idea ? 'plugins' : topic,
      plugin: idea ? (plugin || 'all') : '',
      subject: idea ? subject.trim() : (picked ? 'RONE ' + shortName(picked) : ''),
      message: message.trim(),
      website: '', t: Date.now() - 5000,
      context: withSystem ? systemLines.join('\n') : '',
      files: pics.map(p => ({ name: p.name, data: p.data })),
    }
    setBusy(true)
    try {
      let r
      if (isDevMode()) { await new Promise(res => setTimeout(res, 900)); r = { ok: true } }
      else r = await api.sendSupport(body)
      if (r && r.ok) {
        setSent(body.email)
        setSubject(''); setMessage(''); setPlugin('')
        pics.forEach(p => URL.revokeObjectURL(p.url)); setPics([])
        root.current?.closest('.scroll-y')?.scrollTo({ top: 0, behavior: 'smooth' })
      } else {
        setErr(r?.offline ? t('sup.offline') : r?.status === 429 ? t('sup.tooMany') : (r?.message || t('sup.failed')))
      }
    } catch {
      setErr(t('sup.failed'))
    } finally {
      setBusy(false)
    }
  }

  if (sent) {
    return (
      <div ref={root} className="max-w-[640px] mx-auto pt-10">
        <div className="relative overflow-hidden rounded-[18px] border border-rone-border-2 p-8 text-center"
             style={{ background: 'radial-gradient(90% 120% at 50% -10%, rgb(var(--purple) / 0.16), transparent 60%), rgb(var(--card))' }}>
          <span className="mx-auto w-[54px] h-[54px] rounded-full grid place-items-center bg-rone-green/15 text-rone-green shadow-[0_0_30px_rgb(var(--ok)/0.25)]">
            <Icon.check className="w-6 h-6" />
          </span>
          <h1 className="m-0 mt-4 font-display font-extrabold text-[22px] text-rone-text-primary">{t(kind === 'idea' ? 'sup.sentIdea' : 'sup.sentHelp')}</h1>
          <p className="m-0 mt-2 text-[13.5px] leading-relaxed text-rone-text-secondary">{t('sup.sentBody', { email: sent })}</p>
          <div className="mt-6 flex justify-center gap-2">
            <button className="btn btn-pri btn-white" onClick={() => { setSent(null); setErr('') }}>{t('sup.another')}</button>
            <button className="btn btn-out" onClick={() => onNavigate('library')}>{t('sup.backLibrary')}</button>
          </div>
        </div>
      </div>
    )
  }

  const idea = kind === 'idea'
  return (
    <div ref={root} className="max-w-[860px]">
      <div className="flex items-end gap-4 flex-wrap">
        <div className="flex-1 min-w-[260px]">
          <h1 className="m-0 font-display font-extrabold text-[24px] text-rone-text-primary">{t('sup.title')}</h1>
          <p className="m-0 mt-1 text-[13px] text-rone-text-dim">{t('sup.sub')}</p>
        </div>
        <span className="inline-flex items-center gap-2 h-[28px] px-3 rounded-full border border-rone-border text-[11px] font-bold text-rone-text-dim">
          <span className="led led-ok" />{t('sup.reply')}
        </span>
      </div>

      <div role="tablist" aria-label={t('sup.title')} className="mt-4 flex gap-3 flex-wrap">
        <Door on={!idea} onClick={() => { setKind('help'); setErr('') }} icon={ChatIcon} title={t('sup.help')} body={t('sup.helpBody')} />
        <Door on={idea} onClick={() => { setKind('idea'); setErr('') }} icon={SparkIcon} title={t('sup.idea')} body={t('sup.ideaBody')} />
      </div>

      <form onSubmit={send} noValidate
            onDragOver={(e) => { if ([...(e.dataTransfer?.types || [])].includes('Files')) { e.preventDefault(); setDrag(true) } }}
            onDragLeave={(e) => { if (!e.currentTarget.contains(e.relatedTarget)) setDrag(false) }}
            onDrop={(e) => { if (e.dataTransfer?.files?.length) { e.preventDefault(); setDrag(false); addPics(e.dataTransfer.files) } }}
            className="mt-4 p-5 rounded-[16px] border border-rone-border bg-rone-card grid gap-4">

        <div className="grid gap-3" style={{ gridTemplateColumns: 'repeat(auto-fit, minmax(220px, 1fr))' }}>
          <label><span className={label}>{t('sup.name')}</span>
            <input className={field} value={name} onChange={e => setName(e.target.value)} maxLength={80} autoComplete="name" /></label>
          <label><span className={label}>{t('sup.email')}</span>
            <input className={field} type="email" value={email} onChange={e => setEmail(e.target.value)} maxLength={120} autoComplete="email" required /></label>
        </div>

        {!idea && (
          <div>
            <span className={label}>{t('sup.about')}</span>
            <div className="flex flex-wrap gap-1.5" role="radiogroup" aria-label={t('sup.about')}>
              {TOPICS.map(k => (
                <button key={k} type="button" role="radio" aria-checked={topic === k} onClick={() => setTopic(k)}
                        className={`h-[32px] px-3.5 rounded-full border text-[12px] font-bold transition-colors
                                    ${topic === k ? 'border-rone-purple bg-rone-purple text-rone-neon-dark' : 'border-rone-border-2 text-rone-text-dim hover:text-rone-text-primary'}`}>
                  {t('sup.topic.' + k)}
                </button>
              ))}
            </div>
          </div>
        )}

        {(idea || topic === 'plugins') && (
          <div>
            <span className={label}>{t(idea ? 'sup.whichIdea' : 'sup.which')}</span>
            <div className="flex flex-wrap gap-1.5">
              <button type="button" onClick={() => setPlugin('')} aria-pressed={!plugin}
                      className={`h-[34px] px-3 rounded-[10px] border text-[12px] font-bold transition-colors
                                  ${!plugin ? 'border-rone-purple text-rone-text-primary bg-rone-purple/10' : 'border-rone-border-2 text-rone-text-dim hover:text-rone-text-primary'}`}>
                {t(idea ? 'sup.allPlugins' : 'sup.notSure')}
              </button>
              {ordered.map(p => {
                const on = plugin === p.id
                const acc = accentOf(p)
                return (
                  <button key={p.id} type="button" onClick={() => setPlugin(on ? '' : p.id)} aria-pressed={on} title={p.installedVersion ? `${shortName(p)} ${p.installedVersion}` : shortName(p)}
                          className="h-[34px] pl-1 pr-3 rounded-[10px] border flex items-center gap-2 text-[12px] font-bold transition-all"
                          style={on ? { borderColor: acc, background: tint(acc, 0.14), color: '#fff', boxShadow: `0 0 14px ${tint(acc, 0.25)}` }
                                    : { borderColor: 'rgb(var(--line2))', color: 'rgb(var(--tx3))' }}>
                    {p.logoUrl ? <img src={p.logoUrl} alt="" className="w-[26px] h-[26px] rounded-[7px]" /> : <span className="w-[26px]" />}
                    {shortName(p)}
                  </button>
                )
              })}
            </div>
          </div>
        )}

        {idea && (
          <label><span className={label}>{t('sup.subject')}</span>
            <input className={field} value={subject} onChange={e => setSubject(e.target.value)} maxLength={140} placeholder={t('sup.subjectPh')} /></label>
        )}

        <label><span className={label}>{t('sup.message')}</span>
          <textarea value={message} onChange={e => setMessage(e.target.value)} maxLength={5000} rows={6}
                    placeholder={t(idea ? 'sup.ideaPh' : 'sup.helpPh')}
                    className={field.replace('h-[40px]', 'min-h-[140px]') + ' py-2.5 leading-relaxed resize-y'} /></label>

        <div className={`p-3 rounded-[12px] border border-dashed transition-colors ${drag ? 'border-rone-purple bg-rone-purple/[0.08]' : 'border-rone-border-2'}`}>
          <div className="flex items-center gap-3 flex-wrap">
            <button type="button" className="btn btn-out btn-sm" onClick={() => fileInput.current?.click()} disabled={pics.length >= MAX_PICS}>
              <CamIcon className="w-[15px] h-[15px]" />{t('sup.addPics')}
            </button>
            <span className="flex-1 min-w-[200px] text-[12px] text-rone-text-dim">{t('sup.picsHint', { n: MAX_PICS })}</span>
            <input ref={fileInput} type="file" hidden multiple accept="image/png,image/jpeg,image/gif,image/webp,image/heic,image/heif"
                   onChange={(e) => { addPics(e.target.files); e.target.value = '' }} />
          </div>
          {pics.length > 0 && (
            <div className="mt-3 flex flex-wrap gap-2">
              {pics.map((p, i) => (
                <div key={p.url} className="relative w-[84px] h-[84px] rounded-[10px] overflow-hidden border border-rone-border-2 bg-[#101216]">
                  <img src={p.url} alt={p.name} className="w-full h-full object-cover" />
                  <button type="button" onClick={() => removePic(i)} aria-label={t('sup.removePic', { name: p.name })}
                          className="absolute top-1 right-1 w-[22px] h-[22px] rounded-full bg-black/75 text-white grid place-items-center hover:bg-black">
                    <Icon.close className="w-3 h-3" />
                  </button>
                </div>
              ))}
            </div>
          )}
        </div>

        <div className="rounded-[12px] bg-white/[0.025] border border-rone-border px-3.5 py-2.5">
          <div className="flex items-center gap-3">
            <label className="flex items-center gap-2.5 flex-1 cursor-pointer text-[12.5px] text-rone-text-secondary">
              <input type="checkbox" checked={withSystem} onChange={e => setWithSystem(e.target.checked)} className="accent-[rgb(var(--purple))] w-4 h-4" />
              {t('sup.system')}
            </label>
            <button type="button" className="btn btn-ghost btn-sm" onClick={() => setShowSystem(v => !v)} aria-expanded={showSystem}>
              {t(showSystem ? 'sup.hide' : 'sup.show')}
            </button>
          </div>
          {showSystem && (
            <pre className="m-0 mt-2 whitespace-pre-wrap break-words font-mono text-[11.5px] leading-relaxed text-rone-text-dim">
              {[`RONE Plugins Center ${appVersion || ''} · ${platform === 'mac' ? 'macOS' : 'Windows'}`, ...systemLines].join('\n')}
            </pre>
          )}
        </div>

        <div className="flex items-center gap-3 flex-wrap">
          <p className={`m-0 flex-1 min-w-[200px] text-[12.5px] ${err ? 'text-rone-error' : 'text-rone-text-dim'}`} role="status" aria-live="polite">
            {err || (account.signedIn ? t('sup.signedIn') : t('sup.guest'))}
          </p>
          <button type="submit" className="btn btn-pri btn-white min-w-[170px]" disabled={busy}>
            {busy ? (pics.length ? t('sup.sendingPics') : t('sup.sending')) : t(idea ? 'sup.sendIdea' : 'sup.send')}
          </button>
        </div>
      </form>

      <div className="mt-4 grid gap-3" style={{ gridTemplateColumns: 'repeat(auto-fit, minmax(240px, 1fr))' }}>
        <button onClick={() => onNavigate('learn')} className="flex items-center gap-3 p-3.5 rounded-[14px] border border-rone-border bg-rone-card text-left hover:border-rone-border-3">
          <Icon.learn className="w-5 h-5 text-rone-purple flex-none" />
          <span><b className="block text-[13px] text-rone-text-primary">{t('sup.learnTitle')}</b><span className="text-[12px] text-rone-text-dim">{t('sup.learnBody')}</span></span>
        </button>
        <div className="flex items-center gap-3 p-3.5 rounded-[14px] border border-rone-border bg-rone-card">
          <Icon.checkCircle className="w-5 h-5 text-rone-green flex-none" />
          <span><b className="block text-[13px] text-rone-text-primary">{t('sup.threadTitle')}</b><span className="text-[12px] text-rone-text-dim">{t('sup.threadBody')}</span></span>
        </div>
      </div>
    </div>
  )
}
