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
          // --- RONE graphite surfaces (Xvox-inspired house style) ---
          bg:           '#14161A',
          panel:        '#17191E',
          sidebar:      '#17191E',
          footer:       '#15171B',
          card:         '#101216',
          'card-hover': '#15181D',
          drawer:       '#1B1E23',
          border:       '#23262C',
          'border-2':   '#2A2E35',
          'border-3':   '#383D45',
          header:       '#17191E',
          license:      '#15171B',

          // --- Accent: Center neon = purple (one neon per product) ---
          purple:       '#9D6BFF',
          violet:       '#B18AFF',
          pink:         '#FF3E6C',
          'deep-purple':'#9D6BFF',
          'light-purple':'#C4ADFF',
          'neon-dark':  '#160F21',
          button:       '#1B1E23',

          // --- Semantic (separate from the accent) ---
          green:        '#3EFF8B',
          error:        '#F43F5E',
          amber:        '#FFD02B',
          cyan:         '#2BD9FF',

          // --- Format badges: graphite chips ---
          'badge-vst3': '#2A2E35',
          'badge-au':   '#2A2E35',
          'badge-standalone': '#2A2E35',

          // --- Text ---
          'text-primary':   '#E8EAED',
          'text-secondary': '#B9BDC4',
          'text-dim':       '#7A7F88',
          'text-faint':     '#4E535B',
          'progress-track': '#1B1E23',

          // --- Surface elevation ---
          'surface-0':  '#14161A',
          'surface-1':  '#15171B',
          'surface-2':  '#101216',
          'surface-3':  '#1B1E23',
        }
      },
      fontFamily: {
        sans: ['Manrope', 'Segoe UI', 'Roboto', 'sans-serif'],
        display: ['Sora', 'Segoe UI', 'sans-serif'],
      },
      animation: {
        'pulse-glow': 'pulseGlow 2s ease-in-out infinite',
        'progress': 'progressShimmer 1.5s linear infinite',
        'shimmer': 'shimmer 1.8s ease-in-out infinite',
        'fade-in-up': 'fadeInUp 0.4s ease-out',
      },
      keyframes: {
        pulseGlow: {
          '0%, 100%': { boxShadow: '0 0 5px rgba(157, 107, 255, 0.3)' },
          '50%': { boxShadow: '0 0 14px rgba(157, 107, 255, 0.55)' },
        },
        progressShimmer: {
          '0%': { backgroundPosition: '200% 0' },
          '100%': { backgroundPosition: '-200% 0' },
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
