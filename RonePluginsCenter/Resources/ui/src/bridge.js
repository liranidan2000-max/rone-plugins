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

    // Parse JSON string results from C++ and check for error responses
    return resultPromise.then((result) => {
      let parsed = result;
      if (typeof result === 'string') {
        try { parsed = JSON.parse(result); }
        catch { return result; }
      }

      // If the C++ response indicates an error, throw so callers can catch it
      if (parsed && typeof parsed === 'object' && parsed.success === false && parsed.error) {
        const err = new Error(parsed.error);
        err.response = parsed;
        throw err;
      }

      return parsed;
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
  openManual:        createNativeFunction('openManual'),
  openFolder:        createNativeFunction('openFolder'),
  refreshPlugins:    createNativeFunction('refreshPlugins'),
  activateLicense:   createNativeFunction('activateLicense'),
  deactivateLicense: createNativeFunction('deactivateLicense'),
  getLicenseStatus:  createNativeFunction('getLicenseStatus'),
  accountSignIn:     createNativeFunction('accountSignIn'),
  accountGoogleSignIn: createNativeFunction('accountGoogleSignIn'),
  accountGoogleCancel: createNativeFunction('accountGoogleCancel'),
  accountSignOut:    createNativeFunction('accountSignOut'),
  getAccountStatus:  createNativeFunction('getAccountStatus'),
  getAppVersion:     createNativeFunction('getAppVersion'),
  getAutoStart:      createNativeFunction('getAutoStart'),
  setAutoStart:      createNativeFunction('setAutoStart'),
  applyCenterUpdate: createNativeFunction('applyCenterUpdate'),
  getAnnouncements:  createNativeFunction('getAnnouncements'),
};

// ---- Dev mode mock data (when running outside JUCE) ----
export function isDevMode() {
  return !getBackend();
}

export const mockPlugins = [
  {
    id: 'ReverseReverb', name: 'RONE Reverse Reverb',
    description: 'Real-time reverse reverb effect with WebView2 UI',
    remoteVersion: '1.0.0', installedVersion: '1.0.0',
    status: 'up_to_date', downloadProgress: 0,
    formats: ['VST3', 'AU', 'Standalone'], type: 'plugin',
    whatsNew: 'Initial release', logoUrl: '/logos/ReverseReverb.png',
    hasStandalone: true, standaloneInstalled: true, hasManual: true,
  },
  {
    id: 'RoneStutter', name: 'RONE Stutter',
    description: 'Glitch and stutter effect with WebView2 UI',
    remoteVersion: '1.1.0', installedVersion: '1.0.0',
    status: 'update_available', downloadProgress: 0,
    formats: ['VST3', 'AU', 'Standalone'], type: 'plugin',
    whatsNew: 'New glitch patterns, improved UI', logoUrl: '/logos/RoneStutter.png',
    hasStandalone: true, standaloneInstalled: false,
  },
  {
    id: 'RoneFlanger', name: 'RONE Flanger',
    description: 'Manual flanger with custom visualizer',
    remoteVersion: '1.0.0', installedVersion: '1.0.0',
    status: 'up_to_date', downloadProgress: 0,
    formats: ['VST3', 'Standalone'], type: 'plugin',
    whatsNew: 'Initial release', logoUrl: '/logos/RoneFlanger.png',
    hasStandalone: true, standaloneInstalled: true,
  },
];

// ?announce=1 in dev mode: the website's popup feed as it was on 2026-09-24
// (roneaudio.com/api/v1/popup), with two not-installed cards to match.
export const mockAnnouncements = {
  ok: true,
  popups: [
    {
      id: 'free:RoneClipper:1790188198985', kind: 'free', eyebrow: 'New free plugin', title: 'RONE Clipper',
      body: 'A hard clipper that shows you exactly what it removed.', price: 'Free · no card needed',
      cta: { label: 'Get it free', url: '/products/rone-clipper' },
      image: 'graphics/cutouts/rone-clipper.webp', accent: '#3D8BFF',
    },
    {
      id: 'plugin:RoneRise:1790188200069', kind: 'plugin', eyebrow: 'New plugin', title: 'RONE Rise',
      body: 'One knob turns your mix into a build-up: the lows leave, the reverb opens, the echoes climb.',
      price: '$29 lifetime · launch price', cta: { label: 'See RONE Rise', url: '/products/rone-rise' },
      image: 'graphics/cutouts/rone-rise.webp', accent: '#FF5FB8',
    },
  ],
};

export const mockAnnouncedPlugins = [
  {
    id: 'RoneClipper', name: 'RONE Clipper', description: 'Hard clipper that shows what it cut',
    remoteVersion: '1.0.1', installedVersion: '', status: 'not_installed', downloadProgress: 0,
    formats: ['VST3', 'AU', 'Standalone'], type: 'plugin', whatsNew: '', logoUrl: '/logos/RoneClipper.png',
    hasStandalone: true, standaloneInstalled: false,
  },
  {
    id: 'RoneRise', name: 'RONE Rise', description: 'One-knob build-up',
    remoteVersion: '1.0.2', installedVersion: '', status: 'not_installed', downloadProgress: 0,
    formats: ['VST3', 'AU', 'Standalone'], type: 'plugin', whatsNew: '', logoUrl: '/logos/RoneRise.png',
    hasStandalone: true, standaloneInstalled: false,
  },
];
