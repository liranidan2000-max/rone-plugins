// ============================================================================
// JUCE WebView Bridge — event-based native function calls + event listeners
// Follows the same protocol as ReverseReverbVST (JUCE 8)
// ============================================================================
import { MOCK_CATALOG, MOCK_TIPS } from './mockCatalog.js'

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
    create(timeoutMs = 30000) {
      const id = lastId++;
      const promise = new Promise((resolve, reject) => {
        promises.set(id, { resolve, reject });
        // Timeout (30 s by default) to avoid leaked promises
        setTimeout(() => {
          if (promises.has(id)) {
            promises.get(id).reject(new Error('Native call timed out'));
            promises.delete(id);
          }
        }, timeoutMs);
      });
      return [id, promise];
    }
  };
})();

// Create a callable wrapper for a native function
function createNativeFunction(name, timeoutMs) {
  return function (...args) {
    const backend = getBackend();
    if (!backend) {
      return Promise.resolve(null);
    }

    const [promiseId, resultPromise] = promiseHandler.create(timeoutMs);
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

  // JUCE doesn't provide remove, so this is a no-op placeholder
  return () => {};
}

export function openExternal(url) {
  return callNative('openExternalUrl', url).catch(() => {});
}

// ---- Typed API ----
export const api = {
  getPlugins:        createNativeFunction('getPlugins'),
  installPlugin:     createNativeFunction('installPlugin'),
  cancelInstall:     createNativeFunction('cancelInstall'),
  openPlugin:        createNativeFunction('openPlugin'),
  openManual:        createNativeFunction('openManual'),
  openFolder:        createNativeFunction('openFolder'),
  openInstallFolder: createNativeFunction('openInstallFolder'),
  refreshPlugins:    createNativeFunction('refreshPlugins'),
  activateLicense:   createNativeFunction('activateLicense'),
  deactivateLicense: createNativeFunction('deactivateLicense'),
  getLicenseStatus:  createNativeFunction('getLicenseStatus'),
  accountSignIn:     createNativeFunction('accountSignIn'),
  accountGoogleSignIn: createNativeFunction('accountGoogleSignIn', 300000),
  accountGoogleCancel: createNativeFunction('accountGoogleCancel'),
  accountSignOut:    createNativeFunction('accountSignOut'),
  getAccountStatus:  createNativeFunction('getAccountStatus'),
  getAppVersion:     createNativeFunction('getAppVersion'),
  getAutoStart:      createNativeFunction('getAutoStart'),
  setAutoStart:      createNativeFunction('setAutoStart'),
  getCenterSettings: createNativeFunction('getCenterSettings'),
  setCenterSetting:  createNativeFunction('setCenterSetting'),
  getDaws:           createNativeFunction('getDaws'),
  applyCenterUpdate: createNativeFunction('applyCenterUpdate'),
  getAnnouncements:  createNativeFunction('getAnnouncements'),
  scanOldVersions:   createNativeFunction('scanOldVersions'),
  // may wait for the OS permission prompt (an elevated copy of the Center deletes)
  deleteOldVersions: createNativeFunction('deleteOldVersions', 150000),
  // answers when done: waits for the permission prompt and the plugin's own uninstaller
  uninstallPlugin:   createNativeFunction('uninstallPlugin', 330000),
};

// ---- Dev mode (running outside JUCE: the vite preview) ----
export function isDevMode() {
  return !getBackend();
}

// The dev preview's account scenarios, chosen with ?as=guest|free|lifetime|all
// (the old ?signedout=1 / ?lifetime=1 still work).
export function devScenario() {
  const q = new URLSearchParams(location.search)
  if (q.has('signedout')) return 'guest'
  if (q.has('lifetime')) return 'lifetime'
  const as = q.get('as')
  return ['guest', 'free', 'lifetime', 'all'].includes(as) ? as : 'all'
}

// What each scenario has installed. Real catalog (versions.json), made-up machine.
const DEV_INSTALLED = {
  guest:    {},
  free:     { RoneClipper: 'cur' },
  lifetime: { RoneStutter: '1.5.1.245', RoneFlanger: 'cur', RoneClipper: 'cur' },
  all:      { RoneThrow: '1.7.1.230', RoneStutter: '1.5.1.245', ReverseReverb: 'cur', RoneRise: 'cur',
              RoneStucker: 'cur', RoneFlanger: 'cur', RoneAfterspace: 'cur', RoneClipper: 'cur' },
}
const DEV_META = {
  RoneIron: { released: '2026-10-04' },
  RoneClipper: { released: '2026-09-23' },
  RoneRise: { released: '2026-09-23' },
}

export function mockPlugins(scenario = devScenario()) {
  const installed = DEV_INSTALLED[scenario] || {}
  return MOCK_CATALOG.map(p => {
    const v = installed[p.id]
    const status = !v ? 'not_installed' : v === 'cur' ? 'up_to_date' : 'update_available'
    return {
      ...p, ...(DEV_META[p.id] || {}),
      installedVersion: !v ? '' : v === 'cur' ? p.remoteVersion : v,
      status, downloadProgress: 0, waitingFor: '',
      standaloneInstalled: !!v, unitUrl: `/units/${p.id}.webp`,
    }
  })
}

export function mockAccount(scenario = devScenario()) {
  if (scenario === 'guest') return { signedIn: false, licensed: false, email: '', name: '', plan: 'none', deviceLimit: 2, owned: [], passSource: '', message: '' }
  const base = { signedIn: true, email: 'daniel@example.com', name: 'Daniel M.', deviceLimit: 2, message: '', passSource: '' }
  if (scenario === 'free') return { ...base, licensed: false, plan: 'none', owned: ['RoneClipper'] }
  if (scenario === 'lifetime') return { ...base, licensed: false, plan: 'none', owned: ['RoneStutter', 'RoneFlanger', 'RoneClipper'] }
  return { ...base, licensed: true, plan: 'all-access', owned: [], passSource: 'paddle', renewsAt: Date.now() + 21 * 86400000 }
}

export const mockDaws = [{ id: 'fl', name: 'FL Studio 2026' }, { id: 'ableton', name: 'Ableton Live 12 Suite' }]

export const mockTips = MOCK_TIPS

// Settings > DELETE OLD VERSIONS in dev mode: what Liran's PC held on 2026-09-26.
export const mockOldVersions = [
  { name: 'RONE Reverse Reverb.vst3 / moduleinfo.json', kind: 'manifest',
    reason: 'An old description file (version 1.0.2) inside RONE Reverse Reverb.vst3 - your DAW reads it instead of the plugin' },
  { name: 'RONE Iron.vst3.locked-old01', kind: 'leftover',
    reason: 'A copy an earlier update set aside because a program was using it' },
];

// ?announce=1 in dev mode: the website's popup feed as it was on 2026-09-24
export const mockAnnouncements = {
  ok: true,
  popups: [
    {
      id: 'free:RoneClipper:1790188198985', kind: 'free', eyebrow: 'New free plugin', title: 'RONE Clipper',
      body: 'A hard clipper that shows you exactly what it removed.', price: 'Free · no card needed',
      cta: { label: 'Get it free', url: '/products/rone-clipper' },
      image: 'graphics/cutouts/rone-clipper.webp', accent: '#3D8BFF',
    },
  ],
};
