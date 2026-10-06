import React, { useState } from 'react'
import { Dialog, Seg } from './ui'
import { LANGS } from '../i18n'
import { accentOf } from '../catalog'

const ALL = [['fl', 'FL Studio'], ['ableton', 'Ableton Live'], ['cubase', 'Cubase'], ['studioone', 'Studio One'],
             ['reaper', 'REAPER'], ['bitwig', 'Bitwig Studio'], ['logic', 'Logic Pro']]

// First launch: the language, which DAW (so every install can say where the
// plugin appears), and the free plugin - installed in one click when signed in.
export default function Onboarding ({ t, open, prefs, setPrefs, daws, freePlugin, signedIn, onInstallFree, onAccount, onClose }) {
  const [step, setStep] = useState(0)
  const found = daws.map(d => d.id)
  const options = [...daws.map(d => [d.id, d.name]), ...ALL.filter(([id]) => !found.includes(id))]
  const chosen = prefs.daw || daws[0]?.id || null
  const finish = () => { setPrefs({ onboarded: true }); onClose() }

  return (
    <Dialog open={open} onClose={finish} label={t('onb.title')} width={520} closeOnBackdrop={false}>
      <div className="flex items-center justify-between gap-3 mb-4">
        <h2 className="m-0 font-display font-extrabold text-[19px] text-rone-text-primary">{t('onb.title')}</h2>
        <span className="flex gap-1" aria-hidden="true">
          {[0, 1].map(i => <i key={i} className={`block h-[4px] rounded-[2px] ${i === step ? 'w-6 bg-rone-purple' : 'w-3 bg-rone-border-3'}`} />)}
        </span>
      </div>

      {step === 0 && (
        <>
          <div className="flex items-center justify-between gap-3 mb-4">
            <span className="text-[13px] font-bold text-rone-text-secondary">{t('onb.lang')}</span>
            <Seg label={t('onb.lang')} value={prefs.lang} onChange={(v) => setPrefs({ lang: v })} options={LANGS.map(l => [l.id, l.label])} />
          </div>
          <p className="m-0 text-[15px] font-bold text-rone-text-primary">{t('onb.dawTitle')}</p>
          <p className="m-0 mt-1 mb-3 text-[12.5px] text-rone-text-dim">{t('onb.dawSub')}{found.length > 0 && <> <span className="text-rone-green">✓ {t('onb.found')}</span></>}</p>
          <div className="grid grid-cols-2 gap-2" role="radiogroup" aria-label={t('onb.dawTitle')}>
            {options.map(([id, name]) => (
              <button key={id} role="radio" aria-checked={chosen === id} onClick={() => setPrefs({ daw: id })}
                      className={`flex items-center gap-2 h-[40px] px-3 rounded-[10px] border text-[13px] font-bold text-left
                                  ${chosen === id ? 'border-rone-purple bg-rone-purple/10 text-rone-text-primary' : 'border-rone-border-2 text-rone-text-secondary hover:border-rone-border-3'}`}>
                <span className="flex-1 truncate">{name}</span>
                {found.includes(id) && (
                  <span className="flex-none w-[18px] h-[18px] rounded-full grid place-items-center bg-rone-green/15 text-rone-green" title={t('onb.found')}>
                    <svg viewBox="0 0 24 24" className="w-3 h-3" fill="none" stroke="currentColor" strokeWidth="3" strokeLinecap="round" strokeLinejoin="round"><path d="M5 13l4 4L19 7" /></svg>
                    <span className="sr-only">{t('onb.found')}</span>
                  </span>
                )}
              </button>
            ))}
          </div>
          <div className="flex justify-end gap-2 mt-5">
            <button className="btn btn-ghost" onClick={finish}>{t('onb.skip')}</button>
            <button className="btn btn-pri" onClick={() => setStep(1)} data-autofocus>{t('onb.next')}</button>
          </div>
        </>
      )}

      {step === 1 && (
        <>
          <div className="flex gap-4 items-center p-4 rounded-[14px] border border-rone-border-2"
               style={{ background: `linear-gradient(120deg, color-mix(in srgb, ${freePlugin ? accentOf(freePlugin) : '#3D8BFF'} 14%, transparent), transparent 70%)` }}>
            {freePlugin && <img src={freePlugin.logoUrl} alt="" className="w-[64px] h-[64px] rounded-[16px]" />}
            <div>
              <p className="m-0 text-[15px] font-bold text-rone-text-primary">{t('onb.freeTitle')}</p>
              <p className="m-0 mt-1 text-[12.5px] text-rone-text-secondary">{t('onb.freeBody')}</p>
            </div>
          </div>
          <div className="flex justify-end gap-2 mt-5 flex-wrap" style={{ '--acc': freePlugin ? accentOf(freePlugin) : '#3D8BFF' }}>
            <button className="btn btn-ghost" onClick={finish}>{t('onb.done')}</button>
            {signedIn ? (
              freePlugin && freePlugin.status === 'not_installed' &&
                <button className="btn btn-pri" onClick={() => { onInstallFree(); finish() }} data-autofocus>{t('onb.installFree')}</button>
            ) : (
              <>
                <button className="btn btn-out" onClick={() => { finish(); onAccount() }}>{t('onb.signIn')}</button>
                <button className="btn btn-pri" onClick={() => { finish(); onAccount('signup') }} data-autofocus>{t('onb.create')}</button>
              </>
            )}
          </div>
        </>
      )}
    </Dialog>
  )
}
