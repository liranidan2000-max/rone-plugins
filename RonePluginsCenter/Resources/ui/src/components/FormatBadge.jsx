import React from 'react'

const BADGE_STYLES = {
  VST3: 'bg-rone-badge-vst3/25 border-rone-badge-vst3/40 text-purple-300',
  AU: 'bg-rone-badge-au/25 border-rone-badge-au/40 text-purple-200',
  Standalone: 'bg-rone-badge-standalone/25 border-rone-badge-standalone/40 text-purple-200',
}

export default function FormatBadge({ format }) {
  const style = BADGE_STYLES[format] || BADGE_STYLES.Standalone

  return (
    <span className={`inline-flex items-center px-2 py-0.5 rounded text-[9px] font-bold
                      border tracking-wide ${style}`}>
      {format.toUpperCase()}
    </span>
  )
}
