import React, { useEffect, useState } from 'react'

// The Center updating itself on a Mac (support #107): the new version is opened and copied in while
// this window is still up - up to two minutes on an older Mac, and people quit it when nothing moved
// (4 reports from 3 Macs stopped at "attach", one quit 115 s in). A line with a running clock says it
// is working and that the Center reopens by itself. Gone when the backend says how it ended.
export default function CenterApplying ({ t, since }) {
  const [now, setNow] = useState(Date.now())
  useEffect(() => {
    const id = setInterval(() => setNow(Date.now()), 1000)
    return () => clearInterval(id)
  }, [])
  const s = Math.max(0, Math.floor((now - since) / 1000))
  const clock = Math.floor(s / 60) + ':' + String(s % 60).padStart(2, '0')
  return (
    <div className="flex-shrink-0 flex items-center gap-3 px-6 py-2 bg-white/[0.04] border-b border-white/10" role="status" aria-live="polite">
      <span className="led led-upd" />
      <span className="text-[12px] font-bold text-rone-text-primary">{t('cu.applying')}</span>
      <span className="text-[12px] font-mono tabular-nums text-rone-text-secondary">{clock}</span>
      <span className="text-[12px] text-rone-text-secondary truncate">{s < 150 ? t('cu.applyingSub') : t('cu.applyingLong')}</span>
    </div>
  )
}
