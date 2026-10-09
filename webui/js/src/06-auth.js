function showLoginGate() {
  const gate = $('loginGate');
  if (!gate) return;
  hideSafeModeGate();
  gate.classList.remove('hidden');
  gate.setAttribute('aria-hidden', 'false');
  document.body.classList.add('loginLocked');
  document.body.classList.remove('safeModeLocked');
}

function hideLoginGate() {
  const gate = $('loginGate');
  if (!gate) return;
  gate.classList.add('hidden');
  gate.setAttribute('aria-hidden', 'true');
  document.body.classList.remove('loginLocked');
}

function showSafeModeGate() {
  const gate = $('safeModeGate');
  if (!gate) return;
  hideLoginGate();
  gate.classList.remove('hidden');
  gate.setAttribute('aria-hidden', 'false');
  document.body.classList.add('safeModeLocked');
}

function hideSafeModeGate() {
  const gate = $('safeModeGate');
  if (!gate) return;
  gate.classList.add('hidden');
  gate.setAttribute('aria-hidden', 'true');
  document.body.classList.remove('safeModeLocked');
}

function dismissSafeModeGate() {
  hideSafeModeGate();
  updateSafeModeBanner(true);
}

function updateSafeModeBanner(safeMode) {
  State.safeMode = !!safeMode;
  const el = $('safeModeBanner');
  if (el) el.classList.toggle('hidden', !safeMode);
  const badge = $('safeModeBadge');
  if (badge) badge.classList.toggle('hidden', !safeMode);
  const headBadge = $('safeModeHeadBadge');
  if (headBadge) {
    if (safeMode) {
      headBadge.textContent = 'Safe Mode';
      headBadge.className = 'badgeMini warn';
    } else {
      headBadge.textContent = 'ปกติ';
      headBadge.className = 'badgeMini ok';
    }
  }

  // Safe mode exit / enter buttons in settings
  const btnExit = $('btnExitSafeMode');
  if (btnExit) btnExit.classList.toggle('hidden', !safeMode);
  const btnEnter = $('btnEnterSafeMode');
  if (btnEnter) btnEnter.classList.toggle('hidden', !!safeMode);

  // Update Wi-Fi STA settings notice
  const noticeEl = $('wifiStaSafeModeNotice');
  if (noticeEl) {
    if (safeMode) {
      noticeEl.classList.remove('hidden');
      noticeEl.style.background = 'rgba(46, 196, 182, 0.1)';
      noticeEl.style.borderColor = 'rgba(46, 196, 182, 0.3)';
      noticeEl.style.color = '#2ec4b6';
      noticeEl.innerHTML = 'ℹ️ <b>อยู่ใน Safe Mode:</b> โหมดกู้คืนฉุกเฉิน (Fix IP ถูกปิดชั่วคราวเพื่อให้กู้คืนได้ทุกวงเน็ตเวิร์ก)';
    } else {
      noticeEl.classList.add('hidden');
      noticeEl.innerHTML = '';
    }
  }

  // Ensure Wi-Fi controls are always unlocked for authorized users
  const inputs = ['wifiSsid', 'wifiPass', 'swFixIp'];
  inputs.forEach(id => {
    const inp = $(id);
    if (inp) inp.disabled = false;
  });

  // Keep scan and connect buttons enabled
  const scanBtn = document.querySelector('[data-action="wifi-scan"]');
  if (scanBtn) {
    scanBtn.disabled = false;
    scanBtn.style.opacity = '1';
    scanBtn.style.cursor = 'pointer';
  }

  const connectBtn = document.querySelector('[data-action="wifi-connect"]');
  if (connectBtn) {
    connectBtn.disabled = false;
    connectBtn.style.opacity = '1';
    connectBtn.style.cursor = 'pointer';
  }
}

async function presentDashFirst() {
  document.body.classList.add('app-ready');
  if (window.AppLoader?.hideLoader) window.AppLoader.hideLoader();
  else {
    const loader = $('appLoader');
    if (loader) loader.classList.add('done');
  }
  await showView('dash');
  setDoorControlsEnabled(false);
}

async function completeBoot() {
  if (State.appBooted) return;
  State.appBooted = true;
  hideLoginGate();
  setDoorControlsEnabled(true);
  if (typeof preloadViews === 'function') preloadViews();
  else window.AppLoader?.preloadViews?.();
  if (typeof loadSettingsForm === 'function') loadSettingsForm().catch(() => {});
  startLivePoll();
}

async function bootApp() {
  await completeBoot();
}

async function checkWebLogin() {
  await presentDashFirst();
  try {
    const r = await fetch('/api/webauth', { credentials: 'same-origin' });
    const j = await r.json();
    if (j.safeMode) {
      showSafeModeGate();
      return;
    }
    updateSafeModeBanner(false);
    if (j.hasPassword && !j.authenticated) {
      showLoginGate();
      return;
    }
    hideSafeModeGate();
    hideLoginGate();
    await completeBoot();
  } catch (_) {
    showLoginGate();
  }
}

async function submitWebLogin() {
  setHint('loginMsg', 'hint', '');
  const pass = $('loginPass') ? $('loginPass').value : '';
  try {
    const r = await fetch('/api/login', {
      method: 'POST',
      credentials: 'same-origin',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ password: pass }),
    });
    const j = await r.json();
    if (!r.ok || !j.ok) throw j || { error: 'bad_password' };
    if ($('loginPass')) $('loginPass').value = '';
    hideLoginGate();
    await completeBoot();
  } catch (_) {
    setHint('loginMsg', 'hint err', 'รหัสผ่านไม่ถูกต้อง');
  }
}
