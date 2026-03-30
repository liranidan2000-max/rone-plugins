/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,jsx}",
  ],
  theme: {
    extend: {
      colors: {
        rone: {
          bg:           '#0B0416',
          card:         '#150A26',
          'card-hover': '#1A0D2E',
          border:       '#251540',
          header:       '#0E0620',
          license:      '#110720',
          purple:       '#b537f2',
          pink:         '#E040FB',
          'deep-purple':'#7B1FA2',
          'light-purple':'#CE93D8',
          button:       '#3A1070',
          green:        '#00C853',
          error:        '#e94560',
          'badge-vst3': '#6A1B9A',
          'badge-au':   '#8E24AA',
          'badge-standalone': '#4A148C',
          'text-primary':   '#f0f0f0',
          'text-secondary': '#9999bb',
          'text-dim':       '#606080',
          'progress-track': '#1a0a30',
        }
      },
      fontFamily: {
        sans: ['-apple-system', 'BlinkMacSystemFont', 'Inter', 'Segoe UI', 'Roboto', 'sans-serif'],
      },
      animation: {
        'pulse-glow': 'pulseGlow 2s ease-in-out infinite',
        'progress': 'progressShimmer 1.5s ease-in-out infinite',
      },
      keyframes: {
        pulseGlow: {
          '0%, 100%': { boxShadow: '0 0 5px rgba(181, 55, 242, 0.3)' },
          '50%': { boxShadow: '0 0 20px rgba(181, 55, 242, 0.6)' },
        },
        progressShimmer: {
          '0%': { backgroundPosition: '-200% 0' },
          '100%': { backgroundPosition: '200% 0' },
        },
      },
    },
  },
  plugins: [],
}
