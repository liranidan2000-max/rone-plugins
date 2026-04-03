import React from 'react'

export default function SkeletonCard() {
  return (
    <div className="glass-card rounded-xl border border-rone-border/30 p-4">
      <div className="flex gap-3">
        {/* Logo skeleton */}
        <div className="flex-shrink-0 w-14 h-14 rounded-xl skeleton" />

        {/* Content skeleton */}
        <div className="flex-1 min-w-0 space-y-2 pt-1">
          <div className="h-3.5 w-3/4 rounded skeleton" />
          <div className="h-2.5 w-full rounded skeleton" />
          <div className="h-2 w-1/3 rounded skeleton" />
        </div>
      </div>

      {/* Badge skeletons */}
      <div className="flex gap-1.5 mt-3">
        <div className="h-4 w-10 rounded skeleton" />
        <div className="h-4 w-8 rounded skeleton" />
        <div className="h-4 w-16 rounded skeleton" />
      </div>

      {/* Button skeleton */}
      <div className="flex gap-2 mt-3">
        <div className="h-8 w-24 rounded-lg skeleton" />
      </div>
    </div>
  )
}
