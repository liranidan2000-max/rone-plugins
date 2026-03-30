// ============================================================================
// JUCE WebView Bridge — event-based native function calls + event listeners
// Follows the same protocol as ReverseReverbVST (JUCE 8)
// ============================================================================

function getBackend() {
  return window.__JUCE__?.backend;
}

// ---- Promise handler for native function call/response ----
const promiseHandler = (() => {
  let lastId = 0;
  const promises = new Map();

  const backend = getBackend();
  if (backend) {
    backend.addEventListener('__juce__complete', ({ promiseId, result }) => {
      if (promises.has(promiseId)) {
        promises.get(promiseId).resolve(result);
        promises.delete(promiseId);
      }
    });
  }

  return {
    create() {
      const id = lastId++;
      const promise = new Promise((resolve, reject) => {
        promises.set(id, { resolve, reject });
        // Timeout after 30s to avoid leaked promises
        setTimeout(() => {
          if (promises.has(id)) {
            promises.get(id).reject(new Error('Native call timed out'));
            promises.delete(id);
          }
        }, 30000);
      });
      return [id, promise];
    }
  };
})();

// Create a callable wrapper for a native function
function createNativeFunction(name) {
  return function (...args) {
    const backend = getBackend();
    if (!backend) {
      console.warn('[Bridge] JUCE backend not available');
      return Promise.resolve(null);
    }

    const [promiseId, resultPromise] = promiseHandler.create();
    backend.emitEvent('__juce__invoke', {
      name: name,
      params: args,
      resultId: promiseId,
    });

    // Parse JSON string results from C++
    return resultPromise.then((result) => {
      if (typeof result === 'string') {
        try { return JSON.parse(result); }
        catch { return result; }
      }
      return result;
    });
  };
}

// Call a C++ native function by name, returns a Promise
export function callNative(name, ...args) {
  return createNativeFunction(name)(...args);
}

// Listen for C++ events pushed via emitEventIfBrowserIsVisible
export function onEvent(name, callback) {
  const backend = getBackend();
  if (!backend) return () => {};

  backend.addEventListener(name, (data) => {
    // Data may arrive as an object already, or as a string
    let parsed = data;
    if (typeof data === 'string') {
      try { parsed = JSON.parse(data); }
      catch { /* keep as string */ }
    }
    callback(parsed);
  });

  // Return unsubscribe (JUCE doesn't provide remove, so this is a no-op placeholder)
  return () => {};
}

// ---- Typed API ----
export const api = {
  getPlugins:        createNativeFunction('getPlugins'),
  installPlugin:     createNativeFunction('installPlugin'),
  openPlugin:        createNativeFunction('openPlugin'),
  refreshPlugins:    createNativeFunction('refreshPlugins'),
  activateLicense:   createNativeFunction('activateLicense'),
  deactivateLicense: createNativeFunction('deactivateLicense'),
  getLicenseStatus:  createNativeFunction('getLicenseStatus'),
  getAppVersion:     createNativeFunction('getAppVersion'),
};

// ---- Dev mode mock data (when running outside JUCE) ----
export function isDevMode() {
  return !getBackend();
}

export const mockPlugins = [
  {
    id: 'ReverseReverb', name: 'ReverseReverb',
    description: 'Real-time reverse reverb effect with WebView2 UI',
    remoteVersion: '1.0.0', installedVersion: '1.0.0',
    status: 'up_to_date', downloadProgress: 0,
    formats: ['VST3', 'AU', 'Standalone'], type: 'plugin',
    whatsNew: 'Initial release', logoUrl: '/logos/ReverseReverb.png',
    hasStandalone: true, standaloneInstalled: true,
  },
  {
    id: 'RoneStutter', name: 'Rone Stutter',
    description: 'Glitch and stutter effect with WebView2 UI',
    remoteVersion: '1.1.0', installedVersion: '1.0.0',
    status: 'update_available', downloadProgress: 0,
    formats: ['VST3', 'AU', 'Standalone'], type: 'plugin',
    whatsNew: 'New glitch patterns, improved UI', logoUrl: '/logos/RoneStutter.png',
    hasStandalone: true, standaloneInstalled: false,
  },
  {
    id: 'RoneStemsFixer', name: 'RONE Stems Fixer',
    description: 'Audio stem analysis and repair tool',
    remoteVersion: '1.0.0', installedVersion: '',
    status: 'not_installed', downloadProgress: 0,
    formats: ['Standalone'], type: 'standalone',
    whatsNew: 'Initial release', logoUrl: '/logos/RoneStemsFixer.png',
    hasStandalone: true, standaloneInstalled: false,
  },
  {
    id: 'RoneFlanger', name: 'Rone Flanger',
    description: 'Manual flanger with custom visualizer',
    remoteVersion: '1.0.0', installedVersion: '1.0.0',
    status: 'up_to_date', downloadProgress: 0,
    formats: ['VST3', 'Standalone'], type: 'plugin',
    whatsNew: 'Initial release', logoUrl: '/logos/RoneFlanger.png',
    hasStandalone: true, standaloneInstalled: true,
  },
];
