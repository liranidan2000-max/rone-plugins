import React from 'react'

const STATUS_FILTERS = [
  { key: 'all', label: 'All' },
  { key: 'installed', label: 'Installed' },
  { key: 'updates', label: 'Updates' },
  { key: 'not_installed', label: 'Not Installed' },
]

const SORT_OPTIONS = [
  { key: 'name', label: 'Name (A–Z)' },
  { key: 'status', label: 'Status' },
]

export default function FilterBar({ statusFilter, onStatusFilterChange, sortBy, onSortChange, counts }) {
  return (
    <div className="flex-shrink-0 px-6 pt-5 pb-1 flex items-center gap-3 flex-wrap">
      {/* Status filter pills */}
      <div className="flex items-center gap-2">
        {STATUS_FILTERS.map(filter => {
          const isActive = statusFilter === filter.key
          const count = counts[filter.key] || 0
          const showCount = filter.key !== 'all' && count > 0
          return (
            <button
              key={filter.key}
              onClick={() => onStatusFilterChange(filter.key)}
              className={`flex items-center gap-2 px-3.5 py-1.5 text-[12px] font-semibold rounded-lg border transition-all duration-200
                ${isActive
                  ? 'filter-pill-active'
                  : 'border-rone-border/60 text-rone-text-secondary hover:text-rone-text-primary hover:border-rone-border bg-rone-surface-2/50'
                }`}
            >
              {filter.label}
              {showCount && (
                <span className={`min-w-[18px] h-[18px] px-1 rounded-full text-[10px] font-bold flex items-center justify-center
                  ${isActive ? 'bg-white/25 text-white' : 'bg-rone-surface-3 text-rone-text-secondary'}`}>
                  {count}
                </span>
              )}
            </button>
          )
        })}
      </div>

      <div className="flex-1" />

      {/* Sort dropdown */}
      <div className="flex items-center gap-2 px-3 py-1.5 rounded-lg border border-rone-border/60 bg-rone-surface-2/50">
        <svg className="w-3.5 h-3.5 text-rone-text-dim" fill="none" stroke="currentColor" viewBox="0 0 24 24">
          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2}
            d="M3 4h13M3 8h9m-9 4h6m4 0l4-4m0 0l4 4m-4-4v12" />
        </svg>
        <select
          value={sortBy}
          onChange={(e) => onSortChange(e.target.value)}
          className="bg-transparent text-[12px] font-medium text-rone-text-secondary border-none outline-none cursor-pointer
                     hover:text-rone-text-primary appearance-none pr-5"
          style={{
            backgroundImage: `url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='12' viewBox='0 0 24 24' fill='none' stroke='%239B9BB5' stroke-width='2'%3E%3Cpath d='M6 9l6 6 6-6'/%3E%3C/svg%3E")`,
            backgroundRepeat: 'no-repeat',
            backgroundPosition: 'right 0 center',
          }}
        >
          {SORT_OPTIONS.map(opt => (
            <option key={opt.key} value={opt.key} className="bg-rone-surface-3 text-rone-text-primary">{opt.label}</option>
          ))}
        </select>
      </div>
    </div>
  )
}
