const CACHE_NAME = 'autodoor-pwa-v1';

self.addEventListener('install', (e) => {
  self.skipWaiting();
});

self.addEventListener('activate', (e) => {
  e.waitUntil(clients.claim());
});

self.addEventListener('fetch', (e) => {
  // Network first for real-time app
  e.respondWith(fetch(e.request).catch(() => caches.match(e.request)));
});

