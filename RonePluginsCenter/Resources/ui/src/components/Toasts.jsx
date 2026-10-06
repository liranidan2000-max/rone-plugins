import React from 'react'
import { Icon } from './ui'
import { mixHex } from '../catalog'

// Messages, read out by screen readers (role="status" / "alert"). An install
// that finished says where to find the plugin in the user's DAW.
const TONE = { success: '#3EFF8B', error: '#F43F5E', info: '#2BD9FF' }

export default function Toasts ({ toasts, onRemove }) {
  return (
    <div className="fixed right-5 top-[76px] z-[80] flex flex-col gap-2 w-[360px] max-w-[calc(100vw-40px)] pointer-events-none">
      {toasts.map(x => (
        <div key={x.id} role={x.type === 'error' ? 'alert' : 'status'}
             className="pointer-events-auto rounded-[12px] border px-3.5 py-3 shadow-[0_18px_40px_-12px_rgba(0,0,0,.8)] animate-[toastIn_.3s_ease-out]"
             style={{ background: 'rgba(27,30,35,.98)', borderColor: mixHex(x.accent || TONE[x.type] || TONE.info, 0.4, '#2A2E35') }}>
          <div className="flex items-start gap-2.5">
            <span className="mt-[5px] w-2 h-2 rounded-full flex-none"
                  style={{ background: x.accent || TONE[x.type] || TONE.info, boxShadow: `0 0 8px ${x.accent || TONE[x.type] || TONE.info}` }} />
            <div className="flex-1 min-w-0">
              {x.title && <p className="m-0 text-[13px] font-extrabold text-rone-text-primary">{x.title}</p>}
              <p className={`m-0 ${x.title ? 'text-[12px] text-rone-text-dim mt-0.5' : 'text-[12.5px] text-rone-text-primary'} whitespace-pre-line`}>{x.text}</p>
              {x.steps && (
                <ol className="mt-1.5 mb-0 pl-4 text-[12px] text-rone-text-secondary space-y-0.5 list-decimal">
                  {x.steps.map((s, i) => <li key={i}>{s}</li>)}
                </ol>
              )}
            </div>
            <button onClick={() => onRemove(x.id)} aria-label="Close" className="p-0.5 text-rone-text-faint hover:text-rone-text-primary">
              <Icon.close className="w-3.5 h-3.5" />
            </button>
          </div>
        </div>
      ))}
      <style>{'@keyframes toastIn{from{opacity:0;transform:translateY(-8px)}}'}</style>
    </div>
  )
}
