function updateApStatusUi(r) {
  const el = $('apStatusHint');
  const apBadge = $('apHeadBadge');
  if (r.safeMode) {
    if (el) el.textContent = 'Safe Mode — AP เปิดไม่มีรหัสผ่าน';
    if (apBadge) {
      apBadge.textContent = 'Safe Mode';
      apBadge.className = 'badgeMini warn';
    }
    return;
  }
  if (!r.enabled) {
    if (el) el.textContent = 'AP ปิดอยู่ (ตั้งค่าไม่เปิดฮอตสปอต)';
    if (apBadge) {
      apBadge.textContent = 'ปิดใช้งาน';
      apBadge.className = 'badgeMini off';
    }
    return;
  }
  if (r.apEnabled) {
    const n = r.apStations ?? 0;
    const idle = r.idleMin ?? 1;
    if (el) {
      el.textContent =
        'AP เปิดอยู่ · ' + n + ' เครื่องเชื่อม · ซ่อนอัตโนมัติหลัง ' + idle + ' นาที (ถ้าไม่มีคนเชื่อม)';
    }
    if (apBadge) {
      apBadge.textContent = n > 0 ? `${n} เครื่อง` : 'เปิดฮอตสปอต';
      apBadge.className = 'badgeMini ok';
    }
  } else {
    if (el) el.textContent = 'AP ปิดอยู่ (ซ่อนอัตโนมัติแล้ว หรือยังไม่เปิด)';
    if (apBadge) {
      apBadge.textContent = 'ซ่อนแล้ว';
      apBadge.className = 'badgeMini off';
    }
  }
}

async function loadApUi() {
  try {
    const r = await api('/api/ap');
    if ($('apSsid')) $('apSsid').value = r.ssid || '';
    if ($('apPass')) $('apPass').value = '';
    if ($('swApUser')) $('swApUser').checked = r.enabled !== false;
    if ($('apIdleMin')) $('apIdleMin').value = r.idleMin ?? 1;
    if ($('apMsg')) $('apMsg').textContent = '';
    updateApStatusUi(r);
    if ($('safeModeBadge')) $('safeModeBadge').classList.toggle('hidden', !r.safeMode);
    updateSafeModeBanner(!!r.safeMode);
  } catch (_) {}
}

async function toggleApUser() {
  const enabled = $('swApUser')?.checked;
  try {
    const r = await api('/api/ap', { method: 'POST', body: { enabled } });
    updateApStatusUi({ ...r, enabled, idleMin: $('apIdleMin')?.value });
    setHint('apMsg', 'hint ok', enabled ? 'เปิด AP แล้ว' : 'ปิด AP แล้ว');
  } catch (e) {
    if ($('swApUser')) $('swApUser').checked = !enabled;
    setHint('apMsg', 'hint err', 'ตั้งค่า AP ไม่สำเร็จ');
  }
}

async function saveApSettings() {
  if ($('apMsg')) $('apMsg').textContent = 'บันทึก…';
  const body = {
    ssid: ($('apSsid')?.value || '').trim(),
    enabled: $('swApUser')?.checked,
    idleMin: parseInt($('apIdleMin')?.value || '1', 10) || 1,
  };
  const pass = $('apPass')?.value;
  if (pass) body.password = pass;
  try {
    const r = await api('/api/ap', { method: 'POST', body });
    setHint('apMsg', 'hint ok', 'บันทึก AP แล้ว · ' + (r.effectiveSsid || ''));
    if ($('apPass')) $('apPass').value = '';
    updateApStatusUi({ ...r, enabled: body.enabled, idleMin: body.idleMin });
  } catch (e) {
    setHint(
      'apMsg',
      'hint err',
      e.message === 'ap_pass_short' ? 'รหัสผ่านสั้นเกินไป' : 'บันทึกไม่สำเร็จ'
    );
  }
}
