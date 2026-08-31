import React from 'react'

export default function ProgressBar({ progress = 0 }) {
  const percent = Math.round(progress * 100)

  return (
    <div className="flex items-center gap-2.5">
      <div className="flex-1 h-1 bg-rone-progress-track rounded-full overflow-hidden">
        <div
          className="h-full rounded-full progress-shimmer animate-progress transition-all duration-300 ease-out"
          style={{ width: `${percent}%` }}
        />
      </div>
      <span className="text-[10px] font-extrabold text-rone-cyan tabular-nums tracking-[0.1em] w-9 text-right">
        {percent}%
      </span>
    </div>
  )
}
