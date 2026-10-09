async function showViewImpl(v) {
  document.querySelectorAll('.viewMount[data-view]').forEach((m) => {
    m.classList.toggle('hidden', m.dataset.view !== v);
  });
  const views = {
    dash: 'viewDash',
    devices: 'viewDevices',
    settings: 'viewSettings',
    editor: 'viewEditor',
  };
  for (const [key, id] of Object.entries(views)) {
    const el = $(id);
    if (el) el.classList.toggle('hidden', v !== key);
  }
  document.querySelectorAll('.navItem').forEach((b) => b.classList.remove('on'));
  const navMap = { dash: 'navDash', devices: 'navDev', settings: 'navSet' };
  if (navMap[v]) $(navMap[v])?.classList.add('on');
  
  if (v !== 'settings') {
    if (typeof setEspNowDisabled === 'function') {
      await setEspNowDisabled(false);
    }
    const wifiBody = $('cardWifiHomeBody');
    if (wifiBody && !wifiBody.classList.contains('collapsed')) {
      wifiBody.classList.add('collapsed');
      $('cardWifiHome')?.classList.remove('open');
    }
  }

  if (v === 'devices') await refreshTagList();
  if (v === 'settings') {
    await loadSettingsForm();
    if (typeof loadHistoryDates === 'function') await loadHistoryDates();
  }
}

async function showView(v) {
  State.currentView = v;
  try {
    if (window.AppLoader?.ensureView) {
      const map = { dash: 'dash', devices: 'devices', settings: 'settings', editor: 'editor' };
      await window.AppLoader.ensureView(map[v] || v);
    }
    await showViewImpl(v);
  } catch (e) {
    console.error('showView', e);
  }
}
