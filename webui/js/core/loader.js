const assetCache = new Map();
const loadedStyles = new Set();
const loadedScripts = new Set();
let manifest = null;

export async function fetchAsset(name) {
  if (assetCache.has(name)) return assetCache.get(name);
  const res = await fetch('/assets/' + name, { credentials: 'same-origin' });
  if (!res.ok) throw new Error('asset_' + name);
  const text = await res.text();
  assetCache.set(name, text);
  return text;
}

export async function loadManifest() {
  const res = await fetch('/assets/manifest.json', { credentials: 'same-origin', cache: 'no-cache' });
  if (!res.ok) throw new Error('manifest');
  manifest = await res.json();
  return manifest;
}

export async function loadStyle(name) {
  if (loadedStyles.has(name)) return;
  const link = document.createElement('link');
  link.rel = 'stylesheet';
  link.href = '/assets/' + name;
  document.head.appendChild(link);
  loadedStyles.add(name);
}

export async function loadClassicScript(name) {
  if (loadedScripts.has(name)) return;
  await new Promise((resolve, reject) => {
    const s = document.createElement('script');
    s.src = '/assets/' + name;
    s.onload = () => resolve();
    s.onerror = () => reject(new Error('script_' + name));
    document.body.appendChild(s);
  });
  loadedScripts.add(name);
}

function viewDomId(v) {
  return { dash: 'viewDash', devices: 'viewDevices', settings: 'viewSettings', editor: 'viewEditor' }[v];
}

export async function mountPartial(name, containerId) {
  const m = manifest.partials[name];
  if (!m) return;
  const html = await fetchAsset(m);
  const el = document.getElementById(containerId);
  if (el) el.innerHTML = html;
}

export async function ensureView(viewName, opts = {}) {
  if (!manifest) await loadManifest();
  const key = viewName;
  const domId = viewDomId(key);
  if (!domId) return;
  if (document.getElementById(domId)) return;

  const spec = manifest.views[key];
  if (!spec) throw new Error('view_' + key);
  const root = document.getElementById('appRoot');
  const mount = document.createElement('div');
  mount.className = 'viewMount' + (opts.preload ? ' hidden' : '');
  mount.dataset.view = key;
  mount.innerHTML = await fetchAsset(spec.html);
  for (const node of [...mount.childNodes]) {
    if (node.nodeType === Node.TEXT_NODE && node.textContent.trim()) {
      node.remove();
    }
  }
  root.appendChild(mount);
  await Promise.all((spec.css || []).map(loadStyle));
  await Promise.all((spec.js || []).map(loadClassicScript));
}

export async function loadCoreAssets() {
  await Promise.all((manifest.core.css || []).map(loadStyle));
  for (const j of manifest.core.js || []) await loadClassicScript(j);
}

export function hideLoader() {
  const el = document.getElementById('appLoader');
  if (el) el.classList.add('done');
}

export function preloadViews() {
  setTimeout(() => {
    ['devices', 'settings'].forEach((v) => ensureView(v, { preload: true }).catch(() => {}));
  }, 2000);
}

export async function initShell() {
  await loadManifest();
  await mountPartial('login', 'loginMount');
  await mountPartial('safemode', 'safeModeMount');
  await mountPartial('nav', 'navMount');
  const navHost = document.getElementById('navMount');
  if (navHost && navHost.querySelector('.nav')) {
    document.body.appendChild(navHost.querySelector('.nav'));
  }
  await loadCoreAssets();
  await ensureView('dash');
}

window.AppLoader = { ensureView, hideLoader, preloadViews };
