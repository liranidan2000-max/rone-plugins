import React from 'react'

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
      return (
        <div className="h-screen flex flex-col items-center justify-center bg-rone-bg text-center p-8">
          <div className="text-4xl mb-4">⚠️</div>
          <h1 className="text-lg font-bold text-rone-text-primary mb-2">
            Something went wrong
          </h1>
          <p className="text-sm text-rone-text-secondary mb-4 max-w-md">
            The RONE Plugins Center encountered an unexpected error.
          </p>
          <button
            onClick={this.handleReload}
            className="px-4 py-2 bg-rone-purple text-white rounded-lg text-sm font-medium
                       hover:brightness-110 transition-all"
          >
            Try Again
          </button>
        </div>
      )
    }

    return this.props.children
  }
}
