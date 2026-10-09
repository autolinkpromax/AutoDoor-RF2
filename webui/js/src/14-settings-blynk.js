function updateBlynkStatusUi(blynk) {
  const box = $('blynkStatusBox');
  if (!box) return;
  box.classList.remove('on', 'wait');
  if (!blynk.enabled) {
    box.textContent = 'สถานะ: ปิดอยู่';
  } else if (blynk.connected) {
    box.textContent = 'สถานะ: เชื่อมต่อ Blynk แล้ว';
    box.classList.add('on');
  } else {
    const tok = blynk.token || State.blynkTokenSaved || '';
    box.textContent =
      tok.length >= 8 ? 'สถานะ: เปิดใช้งาน — กำลังเชื่อมต่อ…' : 'สถานะ: เปิดใช้งาน — ยังไม่มี Token';
    box.classList.add('wait');
  }
  const tok = blynk.token || State.blynkTokenSaved || '';
  const hint = $('blynkTokenHint');
  if (hint) {
    hint.textContent =
      tok.length >= 8
        ? 'บันทึก Token แล้ว (...' + tok.slice(-4) + ') — ช่องว่างตอนบันทึก = ใช้ค่าเดิม'
        : '';
  }
  const blynkBadge = $('blynkHeadBadge');
  if (blynkBadge) {
    if (!blynk.enabled) {
      blynkBadge.textContent = 'ปิดใช้งาน';
      blynkBadge.className = 'badgeMini off';
    } else if (blynk.connected) {
      blynkBadge.textContent = 'ออนไลน์';
      blynkBadge.className = 'badgeMini ok';
    } else {
      blynkBadge.textContent = 'รอเชื่อมต่อ';
      blynkBadge.className = 'badgeMini warn';
    }
  }
}

function blynkPayload(enabledOverride) {
  const body = {
    enabled: enabledOverride != null ? enabledOverride : !!$('swBlynk')?.checked,
    templateId: ($('blynkTemplateId')?.value || '').trim(),
    deviceName: ($('blynkDeviceName')?.value || '').trim() || 'AutoDoor',
  };
  const tok = ($('blynkToken')?.value || '').trim();
  if (tok.length >= 8) body.token = tok;
  return body;
}

async function toggleBlynk() {
  try {
    const d = await api('/api/blynk', { method: 'PUT', body: blynkPayload() });
    if (d.token) State.blynkTokenSaved = d.token;
    State.blynkCache = d;
    updateBlynkStatusUi(d);
    refreshSetupWizardUi(null, d);
  } catch (e) {
    $('blynkMsg').textContent = 'ตั้ง Blynk ไม่สำเร็จ';
  }
}

async function saveBlynk() {
  $('blynkMsg').textContent = 'บันทึก…';
  try {
    const d = await api('/api/blynk', { method: 'PUT', body: blynkPayload() });
    if (d.token) State.blynkTokenSaved = d.token;
    State.blynkCache = d;
    setHint('blynkMsg', 'hint ok', 'บันทึกแล้ว');
    updateBlynkStatusUi(d);
    if ($('blynkToken')) $('blynkToken').value = '';
    refreshSetupWizardUi(null, d);
  } catch (e) {
    setHint('blynkMsg', 'hint err', 'บันทึกไม่สำเร็จ');
  }
}

async function refreshBlynkStatus() {
  try {
    const blynk = await api('/api/blynk');
    State.blynkCache = blynk;
    updateBlynkStatusUi(blynk);
  } catch (_) {}
}
