import React from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import PluginCard from './PluginCard'
import SkeletonCard from './SkeletonCard'

const containerVariants = {
  hidden: {},
  show: { transition: { staggerChildren: 0.07 } },
}

export default function PluginGrid({ plugins, licensed, onInstall, onOpen, onInfo, unlockPlaying, loading }) {
  if (loading) {
    return (
      <div className="h-full overflow-y-auto plugin-grid-scroll px-6 py-5">
        <div className="grid grid-cols-1 xl:grid-cols-2 gap-4 max-w-[1100px]">
          {[...Array(6)].map((_, i) => <SkeletonCard key={i} />)}
        </div>
      </div>
    )
  }

  if (plugins.length === 0) {
    return (
      <div className="flex flex-col items-center justify-center h-full gap-2">
        <svg className="w-10 h-10 text-rone-text-dim/40" fill="none" stroke="currentColor" viewBox="0 0 24 24">
          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={1.5} d="M21 21l-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z" />
        </svg>
        <p className="text-rone-text-dim text-sm">No plugins found</p>
      </div>
    )
  }

  return (
    <div className="h-full overflow-y-auto plugin-grid-scroll px-6 py-5">
      <motion.div
        className="grid grid-cols-1 xl:grid-cols-2 gap-4 max-w-[1100px]"
        variants={containerVariants}
        initial="hidden"
        animate="show"
      >
        <AnimatePresence>
          {plugins.map((plugin) => (
            <PluginCard
              key={plugin.id}
              plugin={plugin}
              licensed={licensed}
              onInstall={onInstall}
              onOpen={onOpen}
              onInfo={onInfo}
              unlockPlaying={unlockPlaying}
            />
          ))}
        </AnimatePresence>
      </motion.div>
    </div>
  )
}
