import React from 'react'

function IconHome() {
  return (
    <svg className="w-[16px] h-[16px]" fill="none" stroke="currentColor" viewBox="0 0 24 24">
      <rect x="3" y="3" width="7" height="7" rx="1.5" strokeWidth={1.8} />
      <rect x="14" y="3" width="7" height="7" rx="1.5" strokeWidth={1.8} />
      <rect x="3" y="14" width="7" height="7" rx="1.5" strokeWidth={1.8} />
      <rect x="14" y="14" width="7" height="7" rx="1.5" strokeWidth={1.8} />
    </svg>
  )
}
function IconPlugins() {
  return (
    <svg className="w-[16px] h-[16px]" fill="none" stroke="currentColor" viewBox="0 0 24 24">
      <circle cx="12" cy="12" r="9" strokeWidth={1.8} />
      <path strokeWidth={1.8} strokeLinecap="round" d="M3 12h18M12 3c2.5 2.5 2.5 15 0 18M12 3c-2.5 2.5-2.5 15 0 18" />
    </svg>
  )
}
function IconUpdates() {
  return (
    <svg className="w-[16px] h-[16px]" fill="none" stroke="currentColor" viewBox="0 0 24 24">
      <path strokeWidth={1.8} strokeLinecap="round" strokeLinejoin="round" d="M12 3v12m0 0l-4-4m4 4l4-4M4 17v2a2 2 0 002 2h12a2 2 0 002-2v-2" />
    </svg>
  )
}
function IconAccount() {
  return (
    <svg className="w-[16px] h-[16px]" fill="none" stroke="currentColor" viewBox="0 0 24 24">
      <circle cx="12" cy="8" r="3.5" strokeWidth={1.8} />
      <path strokeWidth={1.8} strokeLinecap="round" d="M5 20a7 7 0 0114 0" />
    </svg>
  )
}
function IconSettings() {
  return (
    <svg className="w-[16px] h-[16px]" fill="none" stroke="currentColor" viewBox="0 0 24 24">
      <circle cx="12" cy="12" r="3" strokeWidth={1.8} />
      <path strokeWidth={1.8} strokeLinecap="round" d="M12 2v2.5M12 19.5V22M4.2 4.2l1.8 1.8M18 18l1.8 1.8M2 12h2.5M19.5 12H22M4.2 19.8L6 18M18 6l1.8-1.8" />
    </svg>
  )
}

const NAV = [
  { key: 'home', label: 'Home', Icon: IconHome },
  { key: 'plugins', label: 'Plugins', Icon: IconPlugins },
  { key: 'updates', label: 'Updates', Icon: IconUpdates },
  { key: 'account', label: 'Account', Icon: IconAccount },
  { key: 'settings', label: 'Settings', Icon: IconSettings },
]

function EqualizerArt() {
  // Decorative equalizer graphic - restrained neon glow
  const bars = [0.4, 0.7, 1.0, 0.55, 0.85, 0.45, 0.75, 0.6, 0.95, 0.5, 0.7, 0.4]
  return (
    <div className="relative h-40 mx-3 my-2 rounded-xl overflow-hidden"
         style={{ background: 'radial-gradient(120% 90% at 50% 100%, rgba(157,107,255,0.10) 0%, transparent 65%)' }}>
      <div className="absolute inset-x-0 bottom-0 flex items-end justify-center gap-[4px] h-28 px-4 opacity-85">
        {bars.map((h, i) => (
          <div
            key={i}
            className="eq-bar w-[3px] rounded-full"
            style={{
              height: `${h * 100}%`,
              background: 'linear-gradient(180deg, #E4D9FF 0%, #9D6BFF 55%, rgba(157,107,255,0) 100%)',
              animationDelay: `${i * 0.12}s`,
              animationDuration: `${1.3 + (i % 4) * 0.2}s`,
            }}
          />
        ))}
      </div>
      <div className="absolute inset-x-0 bottom-0 h-px"
           style={{ background: 'linear-gradient(90deg, transparent, rgba(157,107,255,0.5), transparent)' }} />
    </div>
  )
}

export default function Sidebar({ active, onNavigate, updatesCount = 0, license }) {
  return (
    <div className="sidebar-panel flex-shrink-0 w-[230px] h-full flex flex-col">
      {/* Wordmark: RONE white + PLUGINS neon (house pattern) */}
      <div className="px-5 pt-5 pb-4 select-none">
        <div className="font-display font-extrabold text-[19px] tracking-[0.02em] text-rone-text-primary whitespace-nowrap">
          RONE<span className="ml-1.5 text-rone-purple" style={{ textShadow: '0 0 12px rgba(157,107,255,0.35)' }}>PLUGINS</span>
        </div>
        <div className="mt-0.5 text-[9px] font-bold tracking-[0.34em] text-rone-text-faint uppercase">Center</div>
      </div>

      {/* Nav */}
      <nav className="px-3 mt-1 flex flex-col gap-1">
        {NAV.map(({ key, label, Icon }) => {
          const isActive = active === key
          const badge = key === 'updates' ? updatesCount : 0
          return (
            <button
              key={key}
              onClick={() => onNavigate(key)}
              className={`relative flex items-center gap-3 px-3.5 py-2.5 rounded-lg text-[11px] font-bold uppercase tracking-[0.14em]
                          transition-colors duration-200
                          ${isActive ? 'nav-active' : 'text-rone-text-dim hover:text-rone-text-secondary hover:bg-white/[0.025]'}`}
            >
              <span className={isActive ? 'text-rone-purple' : 'text-rone-text-faint'}>
                <Icon />
              </span>
              <span className="flex-1 text-left">{label}</span>
              {badge > 0 && (
                <span className="min-w-[18px] h-[18px] px-1 rounded-full bg-rone-purple text-rone-neon-dark text-[10px] font-extrabold tracking-normal flex items-center justify-center"
                      style={{ boxShadow: '0 0 10px rgba(157,107,255,0.35)' }}>
                  {badge}
                </span>
              )}
            </button>
          )
        })}
      </nav>

      {/* Decorative art */}
      <div className="flex-1 flex items-center">
        <div className="w-full">
          <EqualizerArt />
        </div>
      </div>

      {/* PRO plan card */}
      <div className="p-3">
        <div className="rounded-xl border border-rone-border p-4"
             style={{ background: 'linear-gradient(160deg, #1B1E23 0%, #101216 100%)' }}>
          <div className="flex items-center gap-2">
            <svg className="w-3.5 h-3.5" fill="#9D6BFF" viewBox="0 0 24 24">
              <path d="M13 2L4 14h6l-1 8 9-12h-6z" />
            </svg>
            <span className="font-display text-[12px] font-extrabold text-rone-text-primary tracking-[0.22em]">PRO</span>
          </div>
          <p className="text-[11px] text-rone-text-secondary mt-2 font-semibold">Professional Plan</p>
          <p className="text-[10px] text-rone-text-dim mt-0.5">
            {license?.licensed
              ? (<><span className="text-rone-green font-bold">Active</span> &middot; all plugins unlocked</>)
              : 'Not activated'}
          </p>
          <button
            onClick={() => onNavigate('account')}
            className="mt-3 flex items-center gap-1.5 text-[10px] font-extrabold uppercase tracking-[0.18em] text-rone-purple hover:text-rone-light-purple transition-colors"
          >
            Manage Plan
            <svg className="w-2.5 h-2.5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2.4} d="M9 5l7 7-7 7" />
            </svg>
          </button>
        </div>
      </div>
    </div>
  )
}
