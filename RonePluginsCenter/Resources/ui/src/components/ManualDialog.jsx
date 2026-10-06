import React from 'react'
import { Dialog, Icon } from './ui'
import { accentOf, shortName } from '../catalog'

// MANUAL: the PDF or the YouTube guide.
export default function ManualDialog ({ t, plugin, onPdf, onVideo, onClose }) {
  if (!plugin) return null
  const acc = accentOf(plugin)
  const Choice = ({ icon: I, title, sub, onClick, enabled }) => (
    <button onClick={onClick} disabled={!enabled}
            className="flex items-center gap-3 w-full p-3 rounded-[12px] border border-rone-border-2 text-left hover:border-[var(--acc)] disabled:opacity-45 disabled:hover:border-rone-border-2">
      <span className="w-10 h-10 rounded-[10px] grid place-items-center text-[var(--acc)]" style={{ background: `color-mix(in srgb, ${acc} 14%, transparent)` }}>
        <I className="w-5 h-5" />
      </span>
      <span><b className="block text-[13.5px] text-rone-text-primary">{title}</b><span className="text-[12px] text-rone-text-dim">{sub}</span></span>
    </button>
  )
  return (
    <Dialog open={!!plugin} onClose={onClose} label={t('manual.title', { name: 'RONE ' + shortName(plugin) })} width={400}>
      <div style={{ '--acc': acc }}>
        <div className="flex items-center gap-3 mb-4">
          <img src={plugin.logoUrl} alt="" className="w-10 h-10 rounded-[10px]" />
          <h2 className="m-0 flex-1 font-display text-[16px] font-bold text-rone-text-primary">{t('manual.title', { name: 'RONE ' + shortName(plugin) })}</h2>
          <button onClick={onClose} aria-label={t('dlg.close')} className="p-1 text-rone-text-dim hover:text-rone-text-primary"><Icon.close className="w-4 h-4" /></button>
        </div>
        <div className="flex flex-col gap-2">
          <Choice icon={Icon.pdf} title={t('manual.pdf')} sub={t('manual.pdfSub')} enabled={!!plugin.hasManual} onClick={() => onPdf(plugin)} />
          <Choice icon={Icon.learn} title={t('manual.video')} sub={plugin.videoUrl ? t('manual.videoSub') : t('manual.videoNone')}
                  enabled={!!plugin.videoUrl} onClick={() => onVideo(plugin)} />
        </div>
      </div>
    </Dialog>
  )
}
