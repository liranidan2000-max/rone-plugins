import React, { useEffect, useState } from 'react'
import { api, isDevMode, mockOldVersions } from '../bridge'
import { Dialog, Seg, Switch, timeAgo } from '../components/ui'
import BackPanel from '../components/BackPanel'
import { LANGS } from '../i18n'

// Before 2.0 three of the toggles here (automatic checks, notifications, beta
// channel) did nothing at all. Every switch on this page now does what it says.

const ALL_DAWS = [
  ['fl', 'FL Studio'], ['ableton', 'Ableton Live'], ['cubase', 'Cubase'], ['studioone', 'Studio One'],
  ['reaper', 'REAPER'], ['bitwig', 'Bitwig Studio'], ['logic', 'Logic Pro'],
]

function Row ({ title, desc, children }) {
  return (
    <div className="flex items-center gap-4 py-3 border-b border-rone-border last:border-0">
      <div className="flex-1 min-w-0">
        <p className="m-0 text-[13.5px] font-bold text-rone-text-primary">{title}</p>
        {desc && <p className="m-0 mt-0.5 text-[12px] text-rone-text-dim leading-snug">{desc}</p>}
      </div>
      {children}
    </div>
  )
}

function Box ({ title, children }) {
  return (
    <section className="rounded-[14px] border border-rone-border bg-rone-card px-4 pb-1">
      <h2 className="m-0 pt-3.5 pb-1 text-[11px] font-extrabold tracking-[0.16em] uppercase text-rone-text-dim">{title}</h2>
      {children}
    </section>
  )
}

function OldVersionsDialog ({ t, phase, items, result, onConfirm, onClose }) {
  const shown = result?.items || items
  const busy = phase === 'deleting'
  const open = phase === 'confirm' || phase === 'deleting' || phase === 'done'
  return (
    <Dialog open={open} onClose={() => { if (!busy) onClose() }} label={t('set.deleteOld')} width={540}>
      <h3 className="m-0 mb-1 text-[15px] font-extrabold text-rone-text-primary">
        {phase === 'done' ? t('old.titleDone') : t('old.title', { n: items.length })}
      </h3>
      <p className="m-0 mb-3 text-[12.5px] text-rone-text-secondary leading-relaxed">
        {phase === 'done'
          ? <>{t('old.deleted', { n: result?.deleted ?? 0 })} {(result?.deleted ?? 0) > 0 && t('old.rescan')}</>
          : busy ? t('old.deleting') : t('old.body')}
      </p>
      <div className="max-h-[260px] overflow-y-auto scroll-y rounded-[10px] border border-rone-border divide-y divide-rone-border">
        {shown.map((it, i) => (
          <div key={i} className="px-3 py-2">
            <div className="flex items-center gap-2">
              <span className="text-[10px] font-extrabold uppercase tracking-[0.12em] text-rone-text-dim flex-none">{t('old.kind.' + it.kind)}</span>
              <span className="text-[12.5px] font-semibold text-rone-text-primary truncate">{it.name}</span>
              {it.status && (
                <span className={`ml-auto text-[11px] font-bold flex-none ${it.status === 'deleted' ? 'text-rone-green' : 'text-rone-error'}`}>
                  {t('old.status.' + (it.status === 'deleted' || it.status === 'in_use' ? it.status : 'failed'))}{it.hosts ? ' · ' + it.hosts : ''}
                </span>
              )}
            </div>
            <p className="m-0 mt-0.5 text-[11.5px] text-rone-text-dim">{it.reason}</p>
          </div>
        ))}
      </div>
      <div className="flex justify-end gap-2 mt-4">
        {phase === 'done'
          ? <button className="btn btn-pri" onClick={onClose} data-autofocus>{t('dlg.close')}</button>
          : <>
              <button className="btn btn-out" onClick={onClose} disabled={busy}>{t('dlg.cancel')}</button>
              <button className="btn btn-danger" onClick={onConfirm} disabled={busy}>{busy ? t('set.looking') : t('old.delete', { n: items.length })}</button>
            </>}
      </div>
    </Dialog>
  )
}

export default function SettingsView ({ t, prefs, setPrefs, daws, platform, manifest, onRefresh, onShowOnboarding }) {
  const [center, setCenter] = useState({ backgroundDownload: true, autoStart: false, autoStartSupported: false })
  const [version, setVersion] = useState('')
  const [showBack, setShowBack] = useState(false)

  useEffect(() => {
    if (isDevMode()) { setCenter({ backgroundDownload: true, autoStart: true, autoStartSupported: true }); setVersion('2.0.0 (preview)'); return }
    api.getCenterSettings().then(s => s && setCenter(s)).catch(() => {})
    api.getAppVersion().then(v => setVersion(v?.version || '')).catch(() => {})
  }, [])

  const setCenterSetting = async (key, value) => {
    setCenter(c => ({ ...c, [key]: value }))
    if (isDevMode()) return
    try { const s = await api.setCenterSetting(key, value); if (s) setCenter(s) } catch {}
  }

  // DELETE OLD VERSIONS: idle -> scanning -> confirm -> deleting -> done
  const [phase, setPhase] = useState('idle')
  const [items, setItems] = useState([])
  const [result, setResult] = useState(null)
  const [msg, setMsg] = useState('')
  const startClean = async () => {
    setMsg(''); setResult(null); setPhase('scanning')
    try {
      const r = isDevMode() ? { items: mockOldVersions } : await api.scanOldVersions()
      const found = r?.items || []
      setItems(found)
      if (!found.length) { setPhase('idle'); setMsg(t('set.nothingOld')); return }
      setPhase('confirm')
    } catch (e) { setPhase('idle'); setMsg(e.message || '') }
  }
  const confirmClean = async () => {
    setPhase('deleting')
    try {
      const r = isDevMode() ? { deleted: items.length, items: items.map(it => ({ ...it, status: 'deleted' })) } : await api.deleteOldVersions()
      setResult(r); setPhase('done')
    } catch (e) { setPhase('idle'); setMsg(e.message || '') }
  }

  const openFolder = (kind) => { if (!isDevMode()) api.openInstallFolder(kind).catch(() => {}) }
  const folderKinds = platform === 'mac' ? [['vst3', 'VST3'], ['au', 'AU'], ['standalone', 'Standalone']] : [['vst3', 'VST3'], ['standalone', 'Standalone']]
  const dawOptions = daws.length ? daws.map(d => [d.id, d.name]) : ALL_DAWS
  const dawValue = prefs.daw || dawOptions[0]?.[0]

  return (
    <div className="max-w-[1020px]">
      <h1 className="m-0 mb-4 font-display font-extrabold text-[24px] text-rone-text-primary">{t('set.title')}</h1>

      <div className="grid gap-3.5" style={{ gridTemplateColumns: 'repeat(auto-fit, minmax(400px, 1fr))' }}>
        <Box title={t('set.comfort')}>
          <Row title={t('set.language')} desc={t('set.languageSub')}>
            <Seg label={t('set.language')} value={prefs.lang} onChange={(v) => setPrefs({ lang: v })} options={LANGS.map(l => [l.id, l.label])} />
          </Row>
          <Row title={t('set.text')} desc={t('set.textSub')}>
            <Seg label={t('set.text')} value={prefs.textSize} onChange={(v) => setPrefs({ textSize: v })} options={[['s', 'S'], ['m', 'M'], ['l', 'L']]} />
          </Row>
          <Row title={t('set.contrast')} desc={t('set.contrastSub')}>
            <Switch label={t('set.contrast')} checked={prefs.contrast} onChange={(v) => setPrefs({ contrast: v })} />
          </Row>
          <Row title={t('set.motion')} desc={t('set.motionSub')}>
            <Switch label={t('set.motion')} checked={prefs.reduceMotion} onChange={(v) => setPrefs({ reduceMotion: v })} />
          </Row>
          <Row title={t('set.sounds')} desc={t('set.soundsSub')}>
            <Switch label={t('set.sounds')} checked={prefs.sounds} onChange={(v) => setPrefs({ sounds: v })} />
          </Row>
        </Box>

        <div className="flex flex-col gap-3.5">
          <Box title={t('set.daws')}>
            <Row title={daws.length ? daws.map(d => d.name).join(' · ') : t('set.dawsNone')} desc={t('set.dawsSub')} />
            <Row title={t('set.dawMain')}>
              <select value={dawValue} onChange={(e) => setPrefs({ daw: e.target.value })} aria-label={t('set.dawMain')}
                      className="h-[32px] px-2 rounded-[8px] bg-rone-drawer border border-rone-border-2 text-[12.5px] text-rone-text-primary">
                {(daws.length ? dawOptions : ALL_DAWS).map(([id, name]) => <option key={id} value={id}>{name}</option>)}
              </select>
            </Row>
          </Box>

          <Box title={t('set.updates')}>
            <Row title={t('set.background')} desc={t('set.backgroundSub')}>
              <Switch label={t('set.background')} checked={center.backgroundDownload} onChange={(v) => setCenterSetting('backgroundDownload', v)} />
            </Row>
            {center.autoStartSupported && (
              <Row title={platform === 'mac' ? t('set.startMac') : t('set.startWin')} desc={t('set.startSub')}>
                <Switch label={platform === 'mac' ? t('set.startMac') : t('set.startWin')} checked={center.autoStart} onChange={(v) => setCenterSetting('autoStart', v)} />
              </Row>
            )}
            <Row title={t('set.checkNow')} desc={manifest.syncedAt ? t('set.lastCheck', { ago: timeAgo(manifest.syncedAt, t) }) : ''}>
              <button className="btn btn-out btn-sm" onClick={onRefresh}>{t('set.checkNow')}</button>
            </Row>
          </Box>
        </div>

        <Box title={t('set.maintenance')}>
          <Row title={t('set.openFolder')} desc={t('set.openFolderSub')}>
            <div className="flex gap-1.5">
              {folderKinds.map(([k, label]) => <button key={k} className="btn btn-out btn-sm" onClick={() => openFolder(k)}>{label}</button>)}
            </div>
          </Row>
          <Row title={t('set.deleteOld')} desc={t('set.deleteOldSub') + (msg ? ' ' + msg : '')}>
            <button className="btn btn-out btn-sm" onClick={startClean} disabled={phase === 'scanning' || phase === 'deleting'}>
              {phase === 'scanning' ? t('set.looking') : t('set.deleteOldBtn')}
            </button>
          </Row>
          <Row title={t('set.onboarding')}>
            <button className="btn btn-ghost btn-sm" onClick={onShowOnboarding}>›</button>
          </Row>
        </Box>

        <Box title={t('set.about')}>
          <Row title="RONE Plugins Center" desc={`${t('set.version')} ${version || '—'}`}>
            <button className="btn btn-out btn-sm" onClick={() => setShowBack(true)}>{t('set.aboutSub')}</button>
          </Row>
        </Box>
      </div>

      <OldVersionsDialog t={t} phase={phase} items={items} result={result} onConfirm={confirmClean} onClose={() => { setPhase('idle'); setResult(null) }} />
      {showBack && <BackPanel version={version} onClose={() => setShowBack(false)} />}
    </div>
  )
}
