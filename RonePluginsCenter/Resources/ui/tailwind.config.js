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
          // --- Core navy surfaces (new pro design) ---
          bg:           '#0A0A12',
          sidebar:      '#0C0C16',
          card:         '#13131F',
          'card-hover': '#181826',
          border:       '#232333',
          header:       '#0C0C16',
          license:      '#0C0C16',

          // --- Accents ---
          purple:       '#8B5CF6',
          violet:       '#A855F7',
          pink:         '#E040FB',
          'deep-purple':'#7C3AED',
          'light-purple':'#C4B5FD',
          button:       '#1C1C2B',
          green:        '#22C55E',
          error:        '#F43F5E',
          amber:        '#F59E0B',

          // --- Format badges ---
          'badge-vst3': '#6D28D9',
          'badge-au':   '#7E22CE',
          'badge-standalone': '#5B21B6',

          // --- Text ---
          'text-primary':   '#F4F4F8',
          'text-secondary': '#9B9BB5',
          'text-dim':       '#5F5F78',
          'progress-track': '#1A1A28',

          // --- Surface elevation ---
          'surface-0':  '#0A0A12',
          'surface-1':  '#0E0E18',
          'surface-2':  '#13131F',
          'surface-3':  '#1A1A28',
        }
      },
      fontFamily: {
        sans: ['-apple-system', 'BlinkMacSystemFont', 'Inter', 'Segoe UI', 'Roboto', 'sans-serif'],
      },
      animation: {
        'pulse-glow': 'pulseGlow 2s ease-in-out infinite',
        'progress': 'progressShimmer 1.5s ease-in-out infinite',
        'shimmer': 'shimmer 1.8s ease-in-out infinite',
        'fade-in-up': 'fadeInUp 0.4s ease-out',
      },
      keyframes: {
        pulseGlow: {
          '0%, 100%': { boxShadow: '0 0 5px rgba(139, 92, 246, 0.3)' },
          '50%': { boxShadow: '0 0 20px rgba(139, 92, 246, 0.6)' },
        },
        progressShimmer: {
          '0%': { backgroundPosition: '-200% 0' },
          '100%': { backgroundPosition: '200% 0' },
        },
        shimmer: {
          '0%': { backgroundPosition: '-200% 0' },
          '100%': { backgroundPosition: '200% 0' },
        },
        fadeInUp: {
          '0%': { opacity: '0', transform: 'translateY(12px)' },
          '100%': { opacity: '1', transform: 'translateY(0)' },
        },
      },
    },
  },
  plugins: [],
}
