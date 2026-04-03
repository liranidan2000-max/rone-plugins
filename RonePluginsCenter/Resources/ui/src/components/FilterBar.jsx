import React from 'react'
import { motion } from 'framer-motion'

const STATUS_FILTERS = [
  { key: 'all', label: 'All' },
  { key: 'installed', label: 'Installed' },
  { key: 'updates', label: 'Updates' },
  { key: 'not_installed', label: 'Not Installed' },
]

const SORT_OPTIONS = [
  { key: 'name', label: 'Name A-Z' },
  { key: 'status', label: 'Status' },
]

export default function FilterBar({ statusFilter, onStatusFilterChange, sortBy, onSortChange, counts }) {
  return (
    <div className="flex-shrink-0 px-4 pt-3 pb-1 flex items-center gap-3 flex-wrap">
      {/* Status filter pills */}
      <div className="flex items-center gap-1.5">
        {STATUS_FILTERS.map(filter => {
          const isActive = statusFilter === filter.key
          const count = counts[filter.key] || 0

          return (
            <button
              key={filter.key}
              onClick={() => onStatusFilterChange(filter.key)}
              className={`px-3 py-1 text-[11px] font-medium rounded-full border transition-all duration-200
                ${isActive
                  ? 'filter-pill-active'
                  : 'border-rone-border/30 text-rone-text-dim hover:text-rone-text-secondary hover:border-rone-border/50'
                }`}
            >
              {filter.label}
              {filter.key !== 'all' && count > 0 && (
                <span className={`ml-1.5 text-[10px] ${isActive ? 'text-rone-purple' : 'text-rone-text-dim'}`}>
                  {count}
                </span>
              )}
            </button>
          )
        })}
      </div>

      <div className="flex-1" />

      {/* Sort dropdown */}
      <div className="flex items-center gap-1.5">
        <svg className="w-3 h-3 text-rone-text-dim" fill="none" stroke="currentColor" viewBox="0 0 24 24">
          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2}
            d="M3 4h13M3 8h9m-9 4h6m4 0l4-4m0 0l4 4m-4-4v12" />
        </svg>
        <select
          value={sortBy}
          onChange={(e) => onSortChange(e.target.value)}
          className="bg-transparent text-[11px] text-rone-text-dim border-none outline-none cursor-pointer
                     hover:text-rone-text-secondary appearance-none pr-4"
          style={{
            backgroundImage: `url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='12' viewBox='0 0 24 24' fill='none' stroke='%23606080' stroke-width='2'%3E%3Cpath d='M6 9l6 6 6-6'/%3E%3C/svg%3E")`,
            backgroundRepeat: 'no-repeat',
            backgroundPosition: 'right 0 center',
          }}
        >
          {SORT_OPTIONS.map(opt => (
            <option key={opt.key} value={opt.key}>{opt.label}</option>
          ))}
        </select>
      </div>
    </div>
  )
}
