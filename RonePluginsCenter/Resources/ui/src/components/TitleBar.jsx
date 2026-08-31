import React from 'react'
import { api } from '../bridge'

// Custom window title bar for the frameless window (Windows).
// Dragging is handed to the OS via a native call, so Aero-snap still works.
export default function TitleBar() {
  const startDrag = (e) => {
    if (e.button !== 0) return
    api.startWindowDrag()
  }
  const stop = (e) => e.stopPropagation()

  return (
    <div className="titlebar flex-shrink-0 h-9 flex items-stretch select-none" onMouseDown={startDrag}>
      <span className="self-center pl-4 text-[8px] font-extrabold uppercase tracking-[0.3em] text-rone-text-faint pointer-events-none">
        Rone Plugins Center
      </span>
      <div className="flex-1" />

      {/* Minimize */}
      <button
        onMouseDown={stop}
        onClick={() => api.windowMinimize()}
        className="titlebar-btn w-12 flex items-center justify-center text-rone-text-dim"
        title="Minimize"
        aria-label="Minimize window"
      >
        <svg className="w-3 h-3" fill="none" stroke="currentColor" viewBox="0 0 24 24">
          <path strokeLinecap="round" strokeWidth={2} d="M5 12h14" />
        </svg>
      </button>

      {/* Close (hides to tray, like the native close button) */}
      <button
        onMouseDown={stop}
        onClick={() => api.windowClose()}
        className="titlebar-btn titlebar-btn-close w-12 flex items-center justify-center text-rone-text-dim"
        title="Close"
        aria-label="Close window"
      >
        <svg className="w-3 h-3" fill="none" stroke="currentColor" viewBox="0 0 24 24">
          <path strokeLinecap="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" />
        </svg>
      </button>
    </div>
  )
}
