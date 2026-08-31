import React from 'react'

export default function FormatBadge({ format }) {
  return (
    <span className="inline-flex items-center px-2 py-0.5 rounded text-[8px] font-extrabold
                     uppercase tracking-[0.16em] border border-rone-border-2 bg-rone-drawer text-rone-text-dim">
      {format.toUpperCase()}
    </span>
  )
}
