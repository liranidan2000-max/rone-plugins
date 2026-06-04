import React from 'react'
import { motion } from 'framer-motion'
import logo from '../logo.js'

function IconHome() {
  return (
    <svg className="w-[18px] h-[18px]" fill="none" stroke="currentColor" viewBox="0 0 24 24">
      <rect x="3" y="3" width="7" height="7" rx="1.5" strokeWidth={1.8} />
      <rect x="14" y="3" width="7" height="7" rx="1.5" strokeWidth={1.8} />
      <rect x="3" y="14" width="7" height="7" rx="1.5" strokeWidth={1.8} />
      <rect x="14" y="14" width="7" height="7" rx="1.5" strokeWidth={1.8} />
    </svg>
  )
}
function IconPlugins() {
  return (
    <svg className="w-[18px] h-[18px]" fill="none" stroke="currentColor" viewBox="0 0 24 24">
      <circle cx="12" cy="12" r="9" strokeWidth={1.8} />
      <path strokeWidth={1.8} strokeLinecap="round" d="M3 12h18M12 3c2.5 2.5 2.5 15 0 18M12 3c-2.5 2.5-2.5 15 0 18" />
    </svg>
  )
}
function IconUpdates() {
  return (
    <svg className="w-[18px] h-[18px]" fill="none" stroke="currentColor" viewBox="0 0 24 24">
      <path strokeWidth={1.8} strokeLinecap="round" strokeLinejoin="round" d="M12 3v12m0 0l-4-4m4 4l4-4M4 17v2a2 2 0 002 2h12a2 2 0 002-2v-2" />
    </svg>
  )
}
function IconAccount() {
  return (
    <svg className="w-[18px] h-[18px]" fill="none" stroke="currentColor" viewBox="0 0 24 24">
      <circle cx="12" cy="8" r="3.5" strokeWidth={1.8} />
      <path strokeWidth={1.8} strokeLinecap="round" d="M5 20a7 7 0 0114 0" />
    </svg>
  )
}
function IconSettings() {
  return (
    <svg className="w-[18px] h-[18px]" fill="none" stroke="currentColor" viewBox="0 0 24 24">
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
  // Decorative equalizer / light-rays graphic
  const bars = [0.4, 0.7, 1.0, 0.55, 0.85, 0.45, 0.75, 0.6, 0.95, 0.5, 0.7, 0.4]
  return (
    <div className="relative h-40 mx-2 my-2 rounded-2xl overflow-hidden"
         style={{ background: 'radial-gradient(120% 90% at 50% 100%, rgba(139,92,246,0.18) 0%, transparent 65%)' }}>
      <div className="absolute inset-x-0 bottom-0 flex items-end justify-center gap-[3px] h-28 px-4 opacity-80">
        {bars.map((h, i) => (
          <div
            key={i}
            className="eq-bar w-1 rounded-full"
            style={{
              height: `${h * 100}%`,
              background: 'linear-gradient(180deg, #C4B5FD 0%, #8B5CF6 60%, rgba(139,92,246,0) 100%)',
              animationDelay: `${i * 0.12}s`,
              animationDuration: `${1.3 + (i % 4) * 0.2}s`,
            }}
          />
        ))}
      </div>
      <div className="absolute inset-x-0 bottom-0 h-px bg-gradient-to-r from-transparent via-rone-purple/50 to-transparent" />
    </div>
  )
}

export default function Sidebar({ active, onNavigate, updatesCount = 0, license }) {
  return (
    <div className="sidebar-panel flex-shrink-0 w-[230px] h-full flex flex-col">
      {/* Logo */}
      <div className="px-5 pt-5 pb-4">
        <img src={logo} alt="RONE PLUGINS" className="h-8 w-auto select-none pointer-events-none" draggable={false} />
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
              className={`relative flex items-center gap-3 px-3.5 py-2.5 rounded-xl text-[13px] font-medium
                          transition-colors duration-200
                          ${isActive ? 'nav-active' : 'text-rone-text-secondary hover:text-rone-text-primary hover:bg-white/[0.03]'}`}
            >
              <span className={isActive ? 'text-rone-light-purple' : 'text-rone-text-dim'}>
                <Icon />
              </span>
              <span className="flex-1 text-left">{label}</span>
              {badge > 0 && (
                <span className="min-w-[18px] h-[18px] px-1 rounded-full bg-rone-deep-purple text-white text-[10px] font-bold flex items-center justify-center">
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
        <div className="rounded-2xl border border-rone-border/80 bg-gradient-to-br from-rone-deep-purple/15 to-transparent p-4">
          <div className="flex items-center gap-2">
            <svg className="w-4 h-4 text-rone-light-purple" fill="currentColor" viewBox="0 0 24 24">
              <path d="M12 2l3 5 5 .7-3.6 3.5.9 5L12 18.8 6.8 21.2l.9-5L4 12.7 9 12z" opacity="0.25" />
              <path d="M6 9h12l-2.2 3H8.2zM8.2 13h7.6L12 20z" />
            </svg>
            <span className="text-[13px] font-bold text-rone-text-primary tracking-wide">PRO</span>
          </div>
          <p className="text-[12px] text-rone-text-secondary mt-2 font-medium">Professional Plan</p>
          <p className="text-[10px] text-rone-text-dim mt-0.5">
            {license?.licensed ? 'Active · all plugins unlocked' : 'Not activated'}
          </p>
          <button
            onClick={() => onNavigate('account')}
            className="mt-3 flex items-center gap-1 text-[11px] font-semibold text-rone-light-purple hover:text-white transition-colors"
          >
            Manage Plan
            <svg className="w-3 h-3" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 5l7 7-7 7" />
            </svg>
          </button>
        </div>
      </div>
    </div>
  )
}
