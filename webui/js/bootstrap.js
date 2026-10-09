import { initShell, hideLoader, preloadViews } from './core/loader.js';

async function boot() {
  try {
    await initShell();
    if (typeof window.checkWebLogin === 'function') {
      await window.checkWebLogin();
    } else {
      document.body.classList.add('app-ready');
      hideLoader();
      if (typeof window.showView === 'function') await window.showView('dash');
      preloadViews();
      if (typeof window.startLivePoll === 'function') window.startLivePoll();
    }
  } catch (e) {
    document.body.classList.add('app-ready');
    hideLoader();
    const el = document.getElementById('loaderText');
    if (el) el.textContent = 'โหลดไม่สำเร็จ — รีเฟรชหน้า';
  }
}

boot();
