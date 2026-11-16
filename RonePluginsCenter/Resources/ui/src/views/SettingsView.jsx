import React, { useEffect, useState } from 'react'
import { api, isDevMode, mockOldVersions, mockOldVersionsKept, mockUserFolderMove } from '../bridge'
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

// The old copies, one group per plugin: "RONE Iron · keeps 1.2.1 · 12 old copies,
// 24 FL Studio rows". Every row has a tick box, all ticked; the delete button
// sends only the ticked paths (the native side deletes nothing it did not find
// itself). Liran, 2026-11-16: FL listed RONE Iron .. RONE Iron_13.
function groupOld (items, kept) {
  const groups = new Map()
  for (const it of items) {
    const key = it.plugin || it.name
    if (!groups.has(key)) groups.set(key, { plugin: key, items: [], keep: (kept || []).find(k => k.plugin === key) })
    groups.get(key).items.push(it)
  }
  return [...groups.values()].sort((a, b) => a.plugin.localeCompare(b.plugin))
}

function Tick ({ checked, mixed, onChange, label, disabled }) {
  return (
    <input type="checkbox" aria-label={label} checked={checked} disabled={disabled}
           ref={el => { if (el) el.indeterminate = !!mixed }}
           onChange={e => onChange(e.target.checked)}
           className="accent-[rgb(var(--purple))] w-4 h-4 flex-none cursor-pointer" />
  )
}

function OldVersionsDialog ({ t, phase, items, kept, result, picked, setPicked, onConfirm, onClose }) {
  const shown = result?.items || items
  const busy = phase === 'deleting'
  const done = phase === 'done'
  const open = phase === 'confirm' || phase === 'deleting' || done
  const [openGroups, setOpenGroups] = useState({})
  const groups = groupOld(shown, kept)
  const nPicked = items.filter(it => picked.has(it.path)).length
  const setMany = (paths, on) => setPicked(prev => { const n = new Set(prev); for (const p of paths) { if (on) n.add(p); else n.delete(p) } return n })
  const allPaths = items.map(it => it.path)
  const counts = (g) => {
    const files = g.items.filter(it => it.kind !== 'daw-list').length
    const rows = g.items.length - files
    return [files && t('old.copies', { n: files }), rows && t('old.flRows', { n: rows })].filter(Boolean).join(' · ')
  }
  const statusText = (it) => t('old.status.' + (['deleted', 'in_use', 'kept'].includes(it.status) ? it.status : 'failed')) + (it.hosts ? ' · ' + it.hosts : '')
  return (
    <Dialog open={open} onClose={() => { if (!busy) onClose() }} label={t('set.deleteOld')} width={620}>
      <h3 className="m-0 mb-1 text-[15px] font-extrabold text-rone-text-primary">
        {done ? t('old.titleDone') : t('old.titleGroups', { n: items.length, p: groups.length })}
      </h3>
      <p className="m-0 mb-3 text-[12.5px] text-rone-text-secondary leading-relaxed">
        {done
          ? <>{t('old.deleted', { n: result?.deleted ?? 0 })} {(result?.inUse ?? 0) > 0 && t('old.inUseSome', { n: result.inUse })} {(result?.deleted ?? 0) > 0 && t('old.rescan')}
              {result?.elevationDeclined && <span className="block mt-1 text-rone-error">{t('old.declined')}</span>}</>
          : busy ? t('old.deleting') : t('old.bodyPick')}
      </p>
      {!done && (
        <label className="flex items-center gap-2 px-3 pb-2 text-[12px] font-bold text-rone-text-secondary cursor-pointer select-none">
          <Tick label={t('old.all')} checked={nPicked === items.length} mixed={nPicked > 0 && nPicked < items.length} disabled={busy}
                onChange={(on) => setMany(allPaths, on)} />
          {t('old.all')} <span className="text-rone-text-dim font-semibold">· {t('old.picked', { n: nPicked, total: items.length })}</span>
        </label>
      )}
      <div className="max-h-[360px] overflow-y-auto scroll-y rounded-[10px] border border-rone-border divide-y divide-rone-border">
        {groups.map(g => {
          const paths = g.items.map(it => it.path)
          const on = paths.filter(p => picked.has(p)).length
          const isOpen = !!openGroups[g.plugin]
          const gDeleted = g.items.filter(it => it.status === 'deleted').length
          return (
            <div key={g.plugin}>
              <div className="flex items-center gap-2.5 px-3 py-2.5">
                {!done && <Tick label={g.plugin} checked={on === paths.length} mixed={on > 0 && on < paths.length} disabled={busy}
                                onChange={(v) => setMany(paths, v)} />}
                <button className="flex-1 min-w-0 flex items-center gap-2 text-left bg-transparent border-0 p-0 cursor-pointer"
                        onClick={() => setOpenGroups(o => ({ ...o, [g.plugin]: !o[g.plugin] }))} aria-expanded={isOpen}>
                  <span className="text-[13px] font-bold text-rone-text-primary truncate">{g.plugin}</span>
                  {g.keep?.version && <span className="text-[11px] font-bold text-rone-green flex-none">{t('old.keeps', { v: g.keep.version })}</span>}
                  <span className="ml-auto text-[11.5px] text-rone-text-dim flex-none">
                    {done ? t('old.groupDone', { n: gDeleted, total: g.items.length }) : counts(g)}
                  </span>
                  <span className={`text-rone-text-dim text-[13px] leading-none flex-none transition-transform ${isOpen ? 'rotate-90' : ''}`}>›</span>
                </button>
              </div>
              {isOpen && (
                <div className="pb-2">
                  {g.items.map(it => (
                    <div key={it.path} className="flex items-start gap-2.5 pl-9 pr-3 py-1">
                      {!done && <Tick label={it.name} checked={picked.has(it.path)} disabled={busy} onChange={(v) => setMany([it.path], v)} />}
                      <div className="flex-1 min-w-0">
                        <div className="flex items-center gap-2">
                          <span className="text-[10px] font-extrabold uppercase tracking-[0.12em] text-rone-text-dim flex-none">{t('old.kind.' + it.kind)}</span>
                          <span className="text-[12px] font-semibold text-rone-text-primary truncate" title={it.path}>{it.name}</span>
                          {it.version && <span className="text-[11px] text-rone-text-dim flex-none">{it.version}</span>}
                          {it.status && (
                            <span className={`ml-auto text-[11px] font-bold flex-none ${it.status === 'deleted' ? 'text-rone-green' : it.status === 'kept' ? 'text-rone-text-dim' : 'text-rone-error'}`}>
                              {statusText(it)}
                            </span>
                          )}
                        </div>
                        <p className="m-0 mt-0.5 text-[11px] text-rone-text-dim truncate" title={it.reason}>{it.reason}</p>
                      </div>
                    </div>
                  ))}
                </div>
              )}
            </div>
          )
        })}
      </div>
      <div className="flex justify-end gap-2 mt-4">
        {done
          ? <button className="btn btn-pri" onClick={onClose} data-autofocus>{t('dlg.close')}</button>
          : <>
              <button className="btn btn-out" onClick={onClose} disabled={busy}>{t('dlg.cancel')}</button>
              <button className="btn btn-danger" onClick={onConfirm} disabled={busy || nPicked === 0}>{busy ? t('old.deletingShort') : t('old.delete', { n: nPicked })}</button>
            </>}
      </div>
    </Dialog>
  )
}

// Mac: the plugins a .pkg put in /Library and /Applications, moved into the
// user's own folders once, so every later update installs with no password
// (UserFolderMove.h). The list first, then one password.
function MoveDialog ({ t, phase, items, result, onConfirm, onClose }) {
  const shown = result?.items || items
  const busy = phase === 'moving'
  const open = phase === 'confirm' || phase === 'moving' || phase === 'done'
  const moved = result?.moved ?? 0
  const failed = result?.failed ?? 0
  return (
    <Dialog open={open} onClose={() => { if (!busy) onClose() }} label={t('set.moveUser')} width={560}>
      <h3 className="m-0 mb-1 text-[15px] font-extrabold text-rone-text-primary">
        {phase === 'done' ? t('mv.titleDone') : t('mv.title', { n: items.length })}
      </h3>
      <p className="m-0 mb-3 text-[12.5px] text-rone-text-secondary leading-relaxed">
        {phase === 'done'
          ? (result?.declined
              ? t('mv.declined')
              : <>{t('mv.done', { n: moved })} {failed > 0 && t('mv.failedSome', { n: failed })} {moved > 0 && t('mv.rescan')}
                  {result?.error && <span className="block mt-1 text-rone-error">{result.error}</span>}</>)
          : busy ? t('mv.moving') : t('mv.body')}
      </p>
      <div className="max-h-[260px] overflow-y-auto scroll-y rounded-[10px] border border-rone-border divide-y divide-rone-border">
        {shown.map((it, i) => (
          <div key={i} className="px-3 py-2">
            <div className="flex items-center gap-2">
              <span className="text-[10px] font-extrabold uppercase tracking-[0.12em] text-rone-text-dim flex-none">{t('mv.kind.' + it.kind)}</span>
              <span className="text-[12.5px] font-semibold text-rone-text-primary truncate">{it.name}</span>
              {it.status && (
                <span className={`ml-auto text-[11px] font-bold flex-none ${it.status === 'failed' ? 'text-rone-error' : 'text-rone-green'}`}>
                  {t('mv.status.' + it.status)}
                </span>
              )}
            </div>
            <p className="m-0 mt-0.5 text-[11.5px] text-rone-text-dim truncate">{it.from} → {it.to}</p>
          </div>
        ))}
      </div>
      <div className="flex justify-end gap-2 mt-4">
        {phase === 'done'
          ? <button className="btn btn-pri" onClick={onClose} data-autofocus>{t('dlg.close')}</button>
          : <>
              <button className="btn btn-out" onClick={onClose} disabled={busy}>{t('dlg.cancel')}</button>
              <button className="btn btn-pri" onClick={onConfirm} disabled={busy}>{busy ? t('set.looking') : t('mv.go', { n: items.length })}</button>
            </>}
      </div>
    </Dialog>
  )
}

export default function SettingsView ({ t, prefs, setPrefs, daws, platform, manifest, optin, onOptin, onRefresh, onShowOnboarding, addToast }) {
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
  const [kept, setKept] = useState([])
  const [picked, setPicked] = useState(() => new Set())
  const [result, setResult] = useState(null)
  const [msg, setMsg] = useState('')
  const startClean = async () => {
    setMsg(''); setResult(null); setPhase('scanning')
    try {
      const r = isDevMode() ? { items: mockOldVersions, kept: mockOldVersionsKept } : await api.scanOldVersions()
      const found = r?.items || []
      setItems(found); setKept(r?.kept || [])
      setPicked(new Set(found.map(it => it.path)))
      if (!found.length) { setPhase('idle'); setMsg(t('set.nothingOld')); return }
      setPhase('confirm')
    } catch (e) { setPhase('idle'); setMsg(e.message || '') }
  }
  const confirmClean = async () => {
    const paths = items.filter(it => picked.has(it.path)).map(it => it.path)
    if (!paths.length) return
    setPhase('deleting')
    try {
      const r = isDevMode()
        ? { deleted: paths.length, items: items.filter(it => picked.has(it.path)).map(it => ({ ...it, status: 'deleted' })) }
        : await api.deleteOldVersions(paths)
      setResult(r); setPhase('done')
    } catch (e) { setPhase('idle'); setMsg(e.message || '') }
  }

  // Updates without a password (Mac): idle -> scanning -> confirm -> moving -> done
  const [mvPhase, setMvPhase] = useState('idle')
  const [mvItems, setMvItems] = useState([])
  const [mvResult, setMvResult] = useState(null)
  const [mvMsg, setMvMsg] = useState('')
  const startMove = async () => {
    setMvMsg(''); setMvResult(null); setMvPhase('scanning')
    try {
      const r = isDevMode() ? { items: mockUserFolderMove } : await api.scanUserFolderMove()
      const found = r?.items || []
      setMvItems(found)
      if (!found.length) { setMvPhase('idle'); setMvMsg(t('set.moveUserNone')); return }
      setMvPhase('confirm')
    } catch (e) { setMvPhase('idle'); setMvMsg(e.message || '') }
  }
  const confirmMove = async () => {
    setMvPhase('moving')
    try {
      const r = isDevMode()
        ? { moved: mvItems.length, failed: 0, items: mvItems.map(it => ({ ...it, status: it.kind === 'remove' ? 'removed' : 'moved' })) }
        : await api.moveToUserFolders()
      setMvResult(r); setMvPhase('done')
    } catch (e) { setMvPhase('idle'); setMvMsg(e.message || '') }
  }

  // Audit R243: the backend's "that folder does not exist yet" used to vanish here.
  const openFolder = (kind) => {
    if (isDevMode()) return
    api.openInstallFolder(kind).catch(() => addToast?.({ text: t('set.noFolderYet'), type: 'info' }))
  }
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
            {optin && onOptin && (
              <Row title={t('set.deals')} desc={t('set.dealsSub')}>
                <Switch label={t('set.deals')} checked={!!optin.on} onChange={onOptin} />
              </Row>
            )}
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
          {platform === 'mac' && (
            <Row title={t('set.moveUser')} desc={t('set.moveUserSub') + (mvMsg ? ' ' + mvMsg : '')}>
              <button className="btn btn-out btn-sm" onClick={startMove} disabled={mvPhase === 'scanning' || mvPhase === 'moving'}>
                {mvPhase === 'scanning' ? t('set.looking') : t('set.moveUserBtn')}
              </button>
            </Row>
          )}
          <Row title={t('set.onboarding')}>
            <button className="btn btn-ghost btn-sm" onClick={onShowOnboarding}>{t('set.onboardingBtn')}</button>
          </Row>
        </Box>

        <Box title={t('set.about')}>
          <Row title="RONE Plugins Center" desc={`${t('set.version')} ${version || '—'}`}>
            <button className="btn btn-out btn-sm" onClick={() => setShowBack(true)}>{t('set.aboutSub')}</button>
          </Row>
        </Box>
      </div>

      <OldVersionsDialog t={t} phase={phase} items={items} kept={kept} result={result} picked={picked} setPicked={setPicked} onConfirm={confirmClean} onClose={() => { setPhase('idle'); setResult(null) }} />
      <MoveDialog t={t} phase={mvPhase} items={mvItems} result={mvResult} onConfirm={confirmMove} onClose={() => { setMvPhase('idle'); setMvResult(null) }} />
      {showBack && <BackPanel version={version} onClose={() => setShowBack(false)} />}
    </div>
  )
}
