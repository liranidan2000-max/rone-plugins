import React from 'react'
import { makeT } from '../i18n'
import { loadPrefs } from '../prefs'

// The page around App, so it cannot take App's `t`: it reads the language the
// user picked itself (audit R244 - it said everything in English).
function boundaryT () {
  try { return makeT(loadPrefs().lang) } catch { return makeT('en') }
}

export default class ErrorBoundary extends React.Component {
  constructor(props) {
    super(props)
    this.state = { hasError: false, error: null }
  }

  static getDerivedStateFromError(error) {
    return { hasError: true, error }
  }

  componentDidCatch(error, errorInfo) {
    console.error('[RONE] UI Error:', error, errorInfo)
  }

  handleReload = () => {
    this.setState({ hasError: false, error: null })
  }

  render() {
    if (this.state.hasError) {
      const t = boundaryT()
      return (
        <div className="h-screen flex flex-col items-center justify-center bg-rone-bg text-center p-8">
          <h1 className="text-lg font-bold text-rone-text-primary mb-2">
            {t('err.title')}
          </h1>
          <p className="text-sm text-rone-text-secondary mb-4 max-w-md">
            {t('err.body')}
          </p>
          <button
            onClick={this.handleReload}
            className="px-4 py-2 bg-rone-purple text-white rounded-lg text-sm font-medium
                       hover:brightness-110 transition-all"
          >
            {t('err.retry')}
          </button>
        </div>
      )
    }

    return this.props.children
  }
}
