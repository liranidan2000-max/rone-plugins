import React from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import PluginCard from './PluginCard'

const containerVariants = {
  hidden: {},
  show: {
    transition: { staggerChildren: 0.1 },
  },
}

export default function PluginGrid({ plugins, licensed, onInstall, onOpen, onInfo, unlockPlaying }) {
  if (plugins.length === 0) {
    return (
      <div className="flex items-center justify-center h-full">
        <p className="text-rone-text-dim text-sm">No plugins found</p>
      </div>
    )
  }

  return (
    <div className="h-full overflow-y-auto plugin-grid-scroll p-4">
      <motion.div
        className="grid grid-cols-2 gap-3 max-w-[900px] mx-auto"
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
