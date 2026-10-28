async function toggleTagAuto() {
  const enabled = $('swTagAuto')?.checked;
  if ($('tagAutoMsg')) {
    $('tagAutoMsg').textContent = enabled ? 'เปิด Tag อัตโนมัติ…' : 'ปิด Tag อัตโนมัติ…';
  }
  try {
    await api('/api/tag-auto', { method: 'POST', body: { enabled } });
    setHint(
      'tagAutoMsg',
      'hint ok',
      enabled ? 'Tag ทำงานอัตโนมัติ' : 'ปิดระบบ Tag แล้ว — รีโมท/Blynk ใช้ได้'
    );
  } catch (e) {
    setHint('tagAutoMsg', 'hint err', 'ตั้งค่าไม่สำเร็จ');
    if ($('swTagAuto')) $('swTagAuto').checked = !enabled;
  }
}

async function syncTime() {
  $('timeMsg').textContent = 'กำลังซิงค์…';
  try {
    const d = await api('/api/time/sync', { method: 'POST' });
    $('timeMsg').textContent = d.synced ? 'ซิงค์เวลาแล้ว' : 'รอซิงค์ — ลองอีกครั้ง';
    if ($('sysTimeLbl') && d.synced) {
      const t = await api('/api/time');
      $('sysTimeLbl').textContent = t.localTime || '—';
      const timeBadge = $('timeHeadBadge');
      if (timeBadge) {
        const hm = t.localTime ? (t.localTime.split(' ')[1]?.slice(0, 5) || 'ซิงค์แล้ว') : 'ซิงค์แล้ว';
        timeBadge.textContent = hm;
        timeBadge.className = 'badgeMini ok';
      }
    }
  } catch (e) {
    $('timeMsg').textContent = 'ซิงค์ไม่สำเร็จ — ตรวจ Wi‑Fi';
  }
}

async function restartDevice() {
  if (!confirm('รีสตาร์ทเครื่อง?')) return;
  const btn = $('btnRestartDevice') || document.querySelector('[data-action="restart-device"]');
  if (btn) {
    btn.disabled = true;
    btn.textContent = 'กำลังรีสตาร์ทเครื่อง... (10s)';
  }
  setHint('restartMsg', 'hint ok', 'กำลังรีสตาร์ทเครื่อง... กำลังโหลดหน้าใหม่ใน 10 วินาที');
  try {
    await api('/api/restart', { method: 'POST' });
  } catch (e) {
    console.log('Restart triggered:', e);
  }

  let count = 10;
  const interval = setInterval(() => {
    count--;
    if (count > 0) {
      if (btn) btn.textContent = `กำลังรีสตาร์ทเครื่อง... (${count}s)`;
      setHint('restartMsg', 'hint ok', `กำลังรีสตาร์ทเครื่อง... กำลังโหลดหน้าใหม่ใน ${count} วินาที`);
    } else {
      clearInterval(interval);
      if (btn) btn.textContent = 'กำลังโหลดหน้าเว็บ...';
      setHint('restartMsg', 'hint ok', 'กำลังโหลดหน้าเว็บ...');
      window.location.reload();
    }
  }, 1000);
}

async function toggleAutoReboot() {
  const enabled = $('swAutoReboot')?.checked;
  const msg = $('autoRebootMsg');
  if (msg) msg.textContent = enabled ? 'กำลังเปิดรีสตาร์ทอัตโนมัติ…' : 'กำลังปิดรีสตาร์ทอัตโนมัติ…';
  const wrap = $('autoRebootTimeWrap');
  if (wrap) wrap.classList.toggle('hidden', !enabled);
  try {
    await api('/api/settings', { method: 'PUT', body: { autoRebootEnabled: enabled } });
    setHint(
      'autoRebootMsg',
      'hint ok',
      enabled ? 'เปิดรีสตาร์ทอัตโนมัติแล้ว' : 'ปิดการรีสตาร์ทอัตโนมัติแล้ว'
    );
    const badge = $('rebootHeadBadge');
    if (badge) {
      const timeVal = $('rebootTimeInput')?.value || '03:00';
      badge.textContent = enabled ? `เปิด (${timeVal})` : 'ปิดใช้งาน';
    }
  } catch (e) {
    setHint('autoRebootMsg', 'hint err', 'บันทึกไม่สำเร็จ');
    if ($('swAutoReboot')) $('swAutoReboot').checked = !enabled;
    if (wrap) wrap.classList.toggle('hidden', enabled);
  }
}

async function saveRebootTime() {
  const inp = $('rebootTimeInput');
  if (!inp || !inp.value) return;
  const parts = inp.value.split(':').map((x) => +x);
  if (parts.length < 2 || isNaN(parts[0]) || isNaN(parts[1]) || parts[0] < 0 || parts[0] > 23 || parts[1] < 0 || parts[1] > 59) {
    setHint('rebootTimeMsg', 'hint err', 'รูปแบบเวลาไม่ถูกต้อง (HH:MM)');
    return;
  }
  const h = parts[0];
  const m = parts[1];
  const btn = $('btnSaveRebootTime');
  if (btn) btn.disabled = true;
  setHint('rebootTimeMsg', '', 'กำลังบันทึก...');
  try {
    await api('/api/settings', {
      method: 'PUT',
      body: {
        autoRebootHour: h,
        autoRebootMinute: m,
      },
    });
    setHint('rebootTimeMsg', 'hint ok', `บันทึกเวลารีสตาร์ท ${pad2(h)}:${pad2(m)} น. สำเร็จ`);
    const badge = $('rebootHeadBadge');
    const enabled = $('swAutoReboot')?.checked !== false;
    if (badge && enabled) {
      badge.textContent = `เปิด (${pad2(h)}:${pad2(m)})`;
    }
  } catch (e) {
    setHint('rebootTimeMsg', 'hint err', 'บันทึกไม่สำเร็จ');
  } finally {
    if (btn) btn.disabled = false;
  }
}

async function enterSafeMode() {
  try {
    await api('/api/safemode/enter', { method: 'POST' });
    alert('ระบบกำลังเข้าสู่ Safe Mode และจะรีสตาร์ทเครื่องภายใน 5 วินาที...');
    setTimeout(() => {
      window.location.reload();
    }, 5000);
  } catch (e) {
    alert('ล้มเหลว: ' + e.message);
  }
}

async function exitSafeMode() {
  if (!confirm('ยืนยันออกจาก Safe Mode และรีสตาร์ทเครื่องเข้าสู่โหมดปกติ?')) return;
  try {
    await api('/api/safemode/exit', { method: 'POST' });
    alert('ระบบกำลังออกจาก Safe Mode และจะรีสตาร์ทเครื่องภายใน 5 วินาที...');
    setTimeout(() => {
      window.location.reload();
    }, 5000);
  } catch (e) {
    alert('ล้มเหลว: ' + e.message);
  }
}

async function toggleSimMode() {
  const enabled = $('swSimMode')?.checked;
  if ($('simModeMsg')) {
    $('simModeMsg').textContent = enabled ? 'เปิดโหมดจำลอง…' : 'ปิดโหมดจำลอง…';
  }
  try {
    await api('/api/settings', { method: 'PUT', body: { simMode: enabled } });
    setHint(
      'simModeMsg',
      'hint ok',
      enabled ? 'เปิดโหมดจำลองแล้ว (ขาสัญญาณคุมประตูจะไม่ทำงาน)' : 'กลับสู่โหมดทำงานปกติแล้ว'
    );
  } catch (e) {
    setHint('simModeMsg', 'hint err', 'ตั้งค่าไม่สำเร็จ');
    if ($('swSimMode')) $('swSimMode').checked = !enabled;
  }
}

async function toggleMotorReverse() {
  const enabled = $('swMotorReverse')?.checked;
  if ($('motorReverseMsg')) {
    $('motorReverseMsg').textContent = enabled ? 'กำลังตั้งค่าทิศทางมอเตอร์ (ขวา)…' : 'กำลังตั้งค่าทิศทางมอเตอร์ (ซ้าย)…';
  }
  try {
    await api('/api/settings', { method: 'PUT', body: { motorReverse: enabled } });
    setHint(
      'motorReverseMsg',
      'hint ok',
      enabled ? 'ตั้งค่าทิศทางมอเตอร์เป็น ขวา (กลับทิศ) แล้ว' : 'ตั้งค่าทิศทางมอเตอร์เป็น ซ้าย (ปกติ) แล้ว'
    );
  } catch (e) {
    setHint('motorReverseMsg', 'hint err', 'ตั้งค่าไม่สำเร็จ');
    if ($('swMotorReverse')) $('swMotorReverse').checked = !enabled;
  }
}

async function toggleDoubleClose() {
  const enabled = $('swDoubleClose')?.checked;
  if ($('doubleCloseMsg')) {
    $('doubleCloseMsg').textContent = enabled ? 'กำลังเปิดยิงปิดย้ำรอบที่ 2…' : 'กำลังปิดยิงปิดย้ำรอบที่ 2…';
  }
  try {
    await api('/api/settings', { method: 'PUT', body: { doubleClosePulse: enabled } });
    setHint(
      'doubleCloseMsg',
      'hint ok',
      enabled ? 'เปิดใช้งานยิงคำสั่งปิดย้ำรอบที่ 2 (หน่วง 3 วิ) แล้ว' : 'ปิดใช้งานยิงคำสั่งปิดย้ำรอบที่ 2 แล้ว'
    );
  } catch (e) {
    setHint('doubleCloseMsg', 'hint err', 'ตั้งค่าไม่สำเร็จ');
    if ($('swDoubleClose')) $('swDoubleClose').checked = !enabled;
  }
}

async function toggleMagSensor() {
  const enabled = $('swMagSensor')?.checked;
  if ($('magSensorMsg')) {
    $('magSensorMsg').textContent = enabled ? 'กำลังเปิดเซ็นเซอร์แม่เหล็ก…' : 'กำลังปิดเซ็นเซอร์แม่เหล็ก…';
  }
  try {
    await api('/api/settings', { method: 'PUT', body: { magSensorEnabled: enabled } });
    setHint(
      'magSensorMsg',
      'hint ok',
      enabled ? 'เปิดใช้งานเซ็นเซอร์แม่เหล็กภายนอกแล้ว' : 'ปิดใช้งานเซ็นเซอร์แม่เหล็กแล้ว'
    );
    const mOpt = $('magSensorOptions');
    if (mOpt) mOpt.classList.toggle('hidden', !enabled);
    const calSec = $('magAutoCalibrateSection');
    if (calSec) calSec.classList.toggle('hidden', !enabled);
  } catch (e) {
    setHint('magSensorMsg', 'hint err', 'ตั้งค่าไม่สำเร็จ');
    if ($('swMagSensor')) $('swMagSensor').checked = !enabled;
  }
}

async function toggleSafetySensor() {
  const enabled = $('swSafetySensor')?.checked;
  if ($('safetySensorMsg')) {
    $('safetySensorMsg').textContent = enabled ? 'กำลังเปิดเซ็นเซอร์กันหนีบ…' : 'กำลังปิดเซ็นเซอร์กันหนีบ…';
  }
  try {
    await api('/api/settings', { method: 'PUT', body: { safetySensorEnabled: enabled } });
    setHint(
      'safetySensorMsg',
      'hint ok',
      enabled ? 'เปิดใช้งานเซ็นเซอร์กันหนีบแล้ว' : 'ปิดใช้งานเซ็นเซอร์กันหนีบแล้ว'
    );
    const delaySec = $('safetyDelaySection');
    if (delaySec) delaySec.classList.toggle('hidden', !enabled);
    State.safetySensorEnabled = enabled;
    const sensBadge = $('sensorsHeadBadge');
    if (sensBadge) {
      sensBadge.textContent = enabled ? 'กันหนีบ: เปิด' : 'กันหนีบ: ปิด';
      sensBadge.className = enabled ? 'badgeMini ok' : 'badgeMini off';
    }
  } catch (e) {
    setHint('safetySensorMsg', 'hint err', 'ตั้งค่าไม่สำเร็จ');
    if ($('swSafetySensor')) $('swSafetySensor').checked = !enabled;
  }
}

async function toggleRemotePassageClose() {
  const enabled = $('swRemotePassageClose')?.checked;
  if ($('remotePassageCloseMsg')) {
    $('remotePassageCloseMsg').textContent = enabled ? 'กำลังเปิดโหมดปิดตามหลังสำหรับรีโมทบ้าน…' : 'กำลังปิด…';
  }
  try {
    await api('/api/settings', { method: 'PUT', body: { remotePassageClose: enabled } });
    setHint(
      'remotePassageCloseMsg',
      'hint ok',
      enabled ? 'เปิดใช้งานปิดตามหลังเมื่อรถผ่าน (รีโมทบ้าน) แล้ว' : 'ปิดใช้งานแล้ว (เปิดค้างจนกว่าจะกดปิด)'
    );
    const remSecEl = $('remotePassageDelaySection');
    if (remSecEl) remSecEl.classList.toggle('hidden', !enabled);
  } catch (e) {
    setHint('remotePassageCloseMsg', 'hint err', 'ตั้งค่าไม่สำเร็จ');
    if ($('swRemotePassageClose')) $('swRemotePassageClose').checked = !enabled;
  }
}

async function saveSafetyParams() {
  const debounceSec = parseFloat($('safetyDebounceSec')?.value ?? 0.1);
  if (isNaN(debounceSec) || debounceSec < 0.05 || debounceSec > 3.0) {
    setHint('safetyParamsMsg', 'hint err', 'ความไวตรวจจับตัดลำแสง ต้องอยู่ระหว่าง 0.05 ถึง 3.00 วินาที');
    return;
  }
  const closeSec = parseFloat($('safetyCloseDelaySec')?.value ?? 3);
  if (isNaN(closeSec) || closeSec < 0 || closeSec > 60) {
    setHint('safetyParamsMsg', 'hint err', 'หน่วงปิดเมื่อพ้นลำแสง ต้องอยู่ระหว่าง 0 ถึง 60 วินาที');
    return;
  }
  const revSec = parseFloat($('safetyReverseDelaySec')?.value ?? 5);
  if (isNaN(revSec) || revSec < 1 || revSec > 60) {
    setHint('safetyParamsMsg', 'hint err', 'หน่วงปิดเมื่อดีดกลับ ต้องอยู่ระหว่าง 1 ถึง 60 วินาที');
    return;
  }
  const remoteSec = parseFloat($('remotePassageDelaySec')?.value ?? 5);
  const safeRemoteSec = Math.max(1, Math.min(60, isNaN(remoteSec) ? 5 : remoteSec));

  if ($('safetyParamsMsg')) $('safetyParamsMsg').textContent = 'กำลังบันทึก…';
  try {
    await api('/api/settings', {
      method: 'PUT',
      body: {
        safetyDebounceMs: Math.round(debounceSec * 1000),
        safetyCloseDelayMs: Math.round(closeSec * 1000),
        safetyReverseDelayMs: Math.round(revSec * 1000),
        remotePassageDelayMs: Math.round(safeRemoteSec * 1000)
      }
    });
    setHint('safetyParamsMsg', 'hint ok', 'บันทึกเวลาการทำงานเซ็นเซอร์สำเร็จ');
  } catch (e) {
    setHint('safetyParamsMsg', 'hint err', 'ตั้งค่าไม่สำเร็จ');
  }
}

async function saveRemotePassageDelay() {
  const sec = parseInt($('remotePassageDelaySec')?.value || 5);
  const val = Math.max(1, Math.min(60, isNaN(sec) ? 5 : sec));
  if ($('remotePassageDelayMsg')) $('remotePassageDelayMsg').textContent = 'กำลังบันทึก…';
  try {
    await api('/api/settings', {
      method: 'PUT',
      body: { remotePassageDelayMs: val * 1000 }
    });
    setHint('remotePassageDelayMsg', 'hint ok', 'บันทึกสำเร็จ (' + val + ' วินาที)');
  } catch (e) {
    setHint('remotePassageDelayMsg', 'hint err', 'ตั้งค่าไม่สำเร็จ');
  }
}

async function saveSafetySensorType() {
  const safetySensorType = parseInt($('safetySensorType')?.value || 0);
  if ($('safetySensorTypeMsg')) $('safetySensorTypeMsg').textContent = 'กำลังบันทึก…';
  try {
    await api('/api/settings', {
      method: 'PUT',
      body: { safetySensorType }
    });
    setHint('safetySensorTypeMsg', 'hint ok', 'บันทึกสำเร็จ');
  } catch (e) {
    setHint('safetySensorTypeMsg', 'hint err', 'ตั้งค่าไม่สำเร็จ');
  }
}

async function saveSafetyCloseDelay() {
  const val = parseInt($('safetyCloseDelaySec')?.value || 3);
  if (isNaN(val) || val < 0 || val > 60) {
    setHint('safetyCloseDelayMsg', 'hint err', 'ค่าต้องอยู่ระหว่าง 0 ถึง 60 วินาที');
    return;
  }
  if ($('safetyCloseDelayMsg')) $('safetyCloseDelayMsg').textContent = 'กำลังบันทึก…';
  try {
    await api('/api/settings', {
      method: 'PUT',
      body: { safetyCloseDelayMs: val * 1000 }
    });
    setHint('safetyCloseDelayMsg', 'hint ok', 'บันทึกสำเร็จ (' + val + ' วินาที)');
  } catch (e) {
    setHint('safetyCloseDelayMsg', 'hint err', 'ตั้งค่าไม่สำเร็จ');
  }
}

async function saveSafetyReverseDelay() {
  const val = parseInt($('safetyReverseDelaySec')?.value || 5);
  if (isNaN(val) || val < 1 || val > 60) {
    setHint('safetyReverseDelayMsg', 'hint err', 'ค่าต้องอยู่ระหว่าง 1 ถึง 60 วินาที');
    return;
  }
  if ($('safetyReverseDelayMsg')) $('safetyReverseDelayMsg').textContent = 'กำลังบันทึก…';
  try {
    await api('/api/settings', {
      method: 'PUT',
      body: { safetyReverseDelayMs: val * 1000 }
    });
    setHint('safetyReverseDelayMsg', 'hint ok', 'บันทึกสำเร็จ (' + val + ' วินาที)');
  } catch (e) {
    setHint('safetyReverseDelayMsg', 'hint err', 'ตั้งค่าไม่สำเร็จ');
  }
}

async function saveDoorAnim() {
  const val = parseInt($('doorAnimSec')?.value || 5);
  if (isNaN(val) || val < 1 || val > 60) {
    setHint('doorAnimMsg', 'hint err', 'ค่าต้องอยู่ระหว่าง 1 ถึง 60 วินาที');
    return;
  }
  if ($('doorAnimMsg')) $('doorAnimMsg').textContent = 'กำลังบันทึก…';
  try {
    await api('/api/settings', { method: 'PUT', body: { doorTransitionMs: val * 1000 } });
    State.doorTransitionMs = val * 1000;
    const gateEl = $('gateVisual');
    if (gateEl) {
      gateEl.style.setProperty('--gate-travel', val + 's');
    }
    const motBadge = $('motorHeadBadge');
    if (motBadge) motBadge.textContent = `${val} วิ`;
    setHint('doorAnimMsg', 'hint ok', 'บันทึกสำเร็จ');
  } catch (e) {
    setHint('doorAnimMsg', 'hint err', 'ตั้งค่าไม่สำเร็จ');
  }
}

let autoCalibrateInterval = null;
let autoCalibratePollInterval = null;
let autoCalibrateStartMs = 0;
let isCalibrating = false;

function stopAutoCalibrateUi(msg, isErr = false) {
  if (autoCalibrateInterval) {
    clearInterval(autoCalibrateInterval);
    autoCalibrateInterval = null;
  }
  if (autoCalibratePollInterval) {
    clearInterval(autoCalibratePollInterval);
    autoCalibratePollInterval = null;
  }
  isCalibrating = false;
  $('btnAutoCalibrate')?.classList.remove('hidden');
  $('btnCancelCalibrate')?.classList.add('hidden');
  $('autoCalibrateLive')?.classList.add('hidden');
  if (msg) {
    setHint('autoCalibrateMsg', isErr ? 'hint err' : 'hint ok', msg);
  }
}

async function startAutoCalibrate() {
  if (isCalibrating) return;
  const msgEl = $('autoCalibrateMsg');
  if (msgEl) msgEl.textContent = '';

  const magEnabled = $('swMagSensor')?.checked;
  if (!magEnabled) {
    setHint('autoCalibrateMsg', 'hint err', '⚠️ กรุณาเปิดใช้งานเซ็นเซอร์แม่เหล็กก่อนใช้งานฟังก์ชันนี้');
    return;
  }

  try {
    const live = await api('/api/live');
    if (live.magSensorClosed) {
      setHint('autoCalibrateMsg', 'hint err', '⚠️ เซ็นเซอร์แม่เหล็กยังดูดติดสนิทอยู่ กรุณาเลื่อนเปิดประตูออกมาก่อน แล้วจึงกดเริ่มจับเวลา');
      return;
    }
  } catch (_) {}

  try {
    setHint('autoCalibrateMsg', 'hint', '⚡ กำลังสั่งปิดประตูทันที…');
    const res = await api('/api/door', { method: 'POST', body: { slot: 'close', force: true } });
    if (res.ok === false) throw new Error('door_failed');
  } catch (e) {
    setHint('autoCalibrateMsg', 'hint err', 'ส่งคำสั่งปิดประตูไม่สำเร็จ กรุณาลองใหม่');
    return;
  }

  isCalibrating = true;
  autoCalibrateStartMs = performance.now();
  $('btnAutoCalibrate')?.classList.add('hidden');
  $('btnCancelCalibrate')?.classList.remove('hidden');
  $('autoCalibrateLive')?.classList.remove('hidden');
  setHint('autoCalibrateMsg', 'hint', 'กำลังปิดประตูและจับเวลา... กรุณารอจนกว่าแม่เหล็กจะดูดติดสนิท');

  autoCalibrateInterval = setInterval(() => {
    const elapsedSec = ((performance.now() - autoCalibrateStartMs) / 1000).toFixed(1);
    const timerEl = $('autoCalibrateTimer');
    if (timerEl) timerEl.textContent = elapsedSec;
  }, 50);

  autoCalibratePollInterval = setInterval(async () => {
    try {
      const d = await api('/api/live');
      const isClosed = !!d.magSensorClosed;

      if (isClosed) {
        const totalDurationSec = (performance.now() - autoCalibrateStartMs) / 1000;
        const finalSec = Math.max(1, Math.min(60, Math.ceil(totalDurationSec)));

        stopAutoCalibrateUi();

        if ($('doorAnimSec')) $('doorAnimSec').value = finalSec;

        await api('/api/settings', { method: 'PUT', body: { doorTransitionMs: finalSec * 1000 } });
        State.doorTransitionMs = finalSec * 1000;
        const gateEl = $('gateVisual');
        if (gateEl) {
          gateEl.style.setProperty('--gate-travel', finalSec + 's');
        }
        const motBadge = $('motorHeadBadge');
        if (motBadge) motBadge.textContent = `${finalSec} วิ`;

        setHint('doorAnimMsg', 'hint ok', 'บันทึกสำเร็จ (' + finalSec + ' วินาที)');
        setHint('autoCalibrateMsg', 'hint ok', '🎉 จับเวลาสำเร็จ! วัดเวลาจริงได้ ' + totalDurationSec.toFixed(1) + ' วิ (ตั้งค่า ' + finalSec + ' วินาที และบันทึกเรียบร้อย)');
      }
    } catch (_) {}
  }, 100);

  setTimeout(() => {
    if (isCalibrating) {
      stopAutoCalibrateUi('⚠️ หมดเวลาจับเวลา (เกิน 60 วินาที) กรุณาตรวจสอบเซ็นเซอร์แม่เหล็ก', true);
    }
  }, 65000);
}

function cancelAutoCalibrate() {
  stopAutoCalibrateUi('ยกเลิกการจับเวลาแล้ว', true);
}

async function saveMagSensorType() {
  const val = parseInt($('magSensorType')?.value || 0);
  if ($('magSensorTypeMsg')) $('magSensorTypeMsg').textContent = 'กำลังบันทึก…';
  try {
    await api('/api/settings', { method: 'PUT', body: { magSensorType: val } });
    setHint('magSensorTypeMsg', 'hint ok', 'บันทึกสำเร็จ');
  } catch (e) {
    setHint('magSensorTypeMsg', 'hint err', 'ตั้งค่าไม่สำเร็จ');
  }
}

async function toggleAutoRetryClose() {
  const enabled = $('swAutoRetryClose')?.checked;
  if ($('autoRetryCloseMsg')) {
    $('autoRetryCloseMsg').textContent = enabled ? 'กำลังเปิดระบบปิดซ้ำ…' : 'กำลังปิดระบบปิดซ้ำ…';
  }
  try {
    await api('/api/settings', { method: 'PUT', body: { autoRetryCloseEnabled: enabled } });
    setHint('autoRetryCloseMsg', 'hint ok', enabled ? 'เปิดระบบสั่งปิดซ้ำอัตโนมัติแล้ว' : 'ปิดระบบสั่งปิดซ้ำแล้ว');
    const rOpt = $('autoRetryCloseParams');
    if (rOpt) rOpt.classList.toggle('hidden', !enabled);
  } catch (e) {
    setHint('autoRetryCloseMsg', 'hint err', 'ตั้งค่าไม่สำเร็จ');
    if ($('swAutoRetryClose')) $('swAutoRetryClose').checked = !enabled;
  }
}

async function saveAutoRetryParams() {
  const count = parseInt($('maxCloseRetryCount')?.value || 3);
  const sec = parseInt($('closeRetryDelaySec')?.value || 3);
  if ($('autoRetryParamsMsg')) $('autoRetryParamsMsg').textContent = 'กำลังบันทึก…';
  try {
    await api('/api/settings', {
      method: 'PUT',
      body: {
        maxCloseRetryCount: count,
        closeRetryDelayMs: sec * 1000,
      }
    });
    setHint('autoRetryParamsMsg', 'hint ok', 'บันทึกการตั้งค่าปิดซ้ำสำเร็จ');
  } catch (e) {
    setHint('autoRetryParamsMsg', 'hint err', 'ตั้งค่าไม่สำเร็จ');
  }
}

function compareSemver(v1, v2) {
  if (!v1 || !v2) return 0;
  const clean = (v) => String(v).replace(/^[^\d]+/, '').trim();
  const p1 = clean(v1).split('.').map((x) => parseInt(x, 10) || 0);
  const p2 = clean(v2).split('.').map((x) => parseInt(x, 10) || 0);
  const len = Math.max(p1.length, p2.length);
  for (let i = 0; i < len; i++) {
    const num1 = p1[i] || 0;
    const num2 = p2[i] || 0;
    if (num1 > num2) return 1;
    if (num1 < num2) return -1;
  }
  return 0;
}

function renderOtaStatus(ota, statusInfo) {
  if (statusInfo?.fwVersion) {
    State.fwVersion = statusInfo.fwVersion;
  }
  const curVer = State.fwVersion || '1.0.0';
  if ($('otaCurVersion')) $('otaCurVersion').textContent = `v${curVer}`;
  if ($('otaHeadVer')) $('otaHeadVer').textContent = `v${curVer}`;

  if (!ota) return;
  if ($('otaPartStatus')) {
    if (ota.otaValid) {
      $('otaPartStatus').innerHTML = '<span style="color:var(--ok)">✓ พร้อมใช้งาน (รองรับการอัปเดตอัตโนมัติ)</span>';
    } else {
      $('otaPartStatus').innerHTML = '<span style="color:var(--stop)">⚠ โหมดปกติ (Single App)</span>';
    }
  }
  if ($('otaPartRunning')) {
    $('otaPartRunning').textContent = `${ota.runningPartition || '—'} (ส่วนสำรอง: ${ota.nextPartition || '—'})`;
  }
  if ($('otaPartSpace')) {
    const freeKb = Math.round((ota.freeSketchSpace || 0) / 1024);
    const sizeKb = Math.round((ota.sketchSize || 0) / 1024);
    $('otaPartSpace').textContent = `พื้นที่ว่าง ${freeKb} KB (ขนาดปัจจุบัน: ${sizeKb} KB)`;
  }
}

async function checkOnlineFirmwareUpdate() {
  const btn = $('btnCheckOnlineOta');
  if (btn) {
    btn.disabled = true;
    btn.textContent = '⏳ กำลังตรวจสอบกับ GitHub...';
  }
  setHint('onlineOtaMsg', 'hint ok', 'กำลังเชื่อมต่อไปยัง GitHub เพื่อตรวจสอบเวอร์ชัน...');
  if ($('onlineOtaCard')) $('onlineOtaCard').classList.add('hidden');

  let fwData = null;
  let usedBaseUrl = GITHUB_FW_BASE_URL;
  let lastStatus = 0;

  const tryFetch = async (url) => {
    const res = await fetch(`${url}/version.json?t=${Date.now()}`, { cache: 'no-store' });
    lastStatus = res.status;
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    return await res.json();
  };

  try {
    try {
      fwData = await tryFetch(GITHUB_FW_BASE_URL);
      usedBaseUrl = GITHUB_FW_BASE_URL;
    } catch (e1) {
      console.warn('Main branch fetch failed, trying fallback...', e1);
      fwData = await tryFetch(GITHUB_FW_FALLBACK_URL);
      usedBaseUrl = GITHUB_FW_FALLBACK_URL;
    }
  } catch (err) {
    console.error('Online update check failed:', err);
    if (btn) {
      btn.disabled = false;
      btn.textContent = '🔍 ตรวจสอบเวอร์ชันใหม่';
    }
    if (lastStatus === 404) {
      setHint(
        'onlineOtaMsg',
        'hint err',
        `ไม่พบบน GitHub (HTTP 404: ยังไม่มีไฟล์ firmware/version.json ใน repo ${GITHUB_FW_REPO})`
      );
    } else {
      setHint(
        'onlineOtaMsg',
        'hint err',
        `ไม่สามารถเชื่อมต่อ GitHub ได้ (${err.message || 'ตรวจการเชื่อมต่ออินเทอร์เน็ต'})`
      );
    }
    return;
  }

  if (btn) {
    btn.disabled = false;
    btn.textContent = '🔍 ตรวจสอบเวอร์ชันใหม่';
  }

  if (!fwData || !fwData.version) {
    setHint('onlineOtaMsg', 'hint err', 'ข้อมูลเวอร์ชันบน GitHub ไม่ถูกต้อง');
    return;
  }

  State.latestOnlineFw = fwData;
  State.latestOnlineFwBaseUrl = usedBaseUrl;

  const curVer = State.fwVersion || '1.0.0';
  const cmp = compareSemver(fwData.version, curVer);

  if (cmp > 0) {
    // Newer version found!
    if ($('newFwVerBadge')) $('newFwVerBadge').textContent = `v${fwData.version}`;
    if ($('newFwChangelog')) {
      $('newFwChangelog').textContent = fwData.changelog || 'ปรับปรุงประสิทธิภาพและความเสถียรของระบบ';
    }
    if ($('newFwDate')) {
      $('newFwDate').textContent = fwData.releaseDate || fwData.buildDate || 'ล่าสุด';
    }
    if ($('newFwFile')) {
      $('newFwFile').textContent = fwData.file || 'firmware_autodoor_ota.bin';
    }
    if ($('onlineOtaCard')) $('onlineOtaCard').classList.remove('hidden');
    setHint('onlineOtaMsg', 'hint ok', `✨ พบเวอร์ชันใหม่ v${fwData.version} (ปัจจุบัน: v${curVer})`);
  } else if (cmp === 0) {
    if ($('onlineOtaCard')) $('onlineOtaCard').classList.add('hidden');
    setHint('onlineOtaMsg', 'hint ok', `✓ อุปกรณ์ของคุณเป็นเวอร์ชันล่าสุดแล้ว (v${curVer})`);
  } else {
    if ($('onlineOtaCard')) $('onlineOtaCard').classList.add('hidden');
    setHint(
      'onlineOtaMsg',
      'hint ok',
      `คุณกำลังใช้งานเวอร์ชันสำหรับนักพัฒนา (v${curVer}) ซึ่งใหม่กว่าบน GitHub (v${fwData.version})`
    );
  }
}

function confirmOnlineFirmwareUpdate() {
  if (!State.latestOnlineFw) return;
  performOnlineFirmwareUpdate(State.latestOnlineFw, State.latestOnlineFwBaseUrl || GITHUB_FW_BASE_URL);
}

function dismissOnlineFirmwareUpdate() {
  if ($('onlineOtaCard')) $('onlineOtaCard').classList.add('hidden');
}

async function performOnlineFirmwareUpdate(fwInfo, baseUrl) {
  if ($('onlineOtaCard')) $('onlineOtaCard').classList.add('hidden');

  const btnCheck = $('btnCheckOnlineOta');
  const btnUpload = $('btnUploadOta');
  const progressWrap = $('otaProgressWrap');
  const progressBar = $('otaProgressBar');
  const progressTxt = $('otaProgressTxt');
  const progressLabel = $('otaProgressLabel');

  if (btnCheck) btnCheck.disabled = true;
  if (btnUpload) btnUpload.disabled = true;

  if (progressWrap) progressWrap.classList.remove('hidden');
  if (progressBar) progressBar.style.width = '0%';
  if (progressTxt) progressTxt.textContent = '0%';
  if (progressLabel) progressLabel.textContent = '📥 กำลังดาวน์โหลดเฟิร์มแวร์จาก GitHub...';
  setHint('onlineOtaMsg', 'hint ok', 'กำลังเริ่มดาวน์โหลดเฟิร์มแวร์จาก GitHub...');

  const fileName = fwInfo.file || 'firmware_autodoor_ota.bin';
  const downloadUrl = `${baseUrl}/${fileName}?t=${Date.now()}`;

  // Phase 1: Download binary Blob from GitHub
  const xhrDownload = new XMLHttpRequest();
  xhrDownload.open('GET', downloadUrl, true);
  xhrDownload.responseType = 'blob';

  xhrDownload.onprogress = (e) => {
    if (e.lengthComputable) {
      const percent = Math.round((e.loaded / e.total) * 100);
      const halfPercent = Math.round(percent * 0.5);
      if (progressBar) progressBar.style.width = halfPercent + '%';
      if (progressTxt) progressTxt.textContent = percent + '%';
      if (progressLabel) progressLabel.textContent = `📥 กำลังดาวน์โหลดจาก GitHub... (${percent}%)`;
      setHint('onlineOtaMsg', 'hint ok', `กำลังดาวน์โหลดจาก GitHub... ${percent}%`);
    }
  };

  xhrDownload.onload = () => {
    if (xhrDownload.status !== 200 || !xhrDownload.response) {
      if (btnCheck) btnCheck.disabled = false;
      if (btnUpload) btnUpload.disabled = false;
      setHint('onlineOtaMsg', 'hint err', `ดาวน์โหลดไม่สำเร็จ (HTTP ${xhrDownload.status})`);
      return;
    }

    const fwBlob = xhrDownload.response;
    if (progressLabel) progressLabel.textContent = '⚡ กำลังติดตั้งเฟิร์มแวร์ลงอุปกรณ์ AutoDoor...';
    setHint('onlineOtaMsg', 'hint ok', 'ดาวน์โหลดเสร็จสิ้น กำลังส่งเข้าบอร์ด AutoDoor...');

    // Phase 2: Upload Blob to ESP32 OTA endpoint
    const formData = new FormData();
    formData.append('update', fwBlob, fileName);

    const xhrUpload = new XMLHttpRequest();
    xhrUpload.open('POST', '/api/ota', true);

    xhrUpload.upload.onprogress = (e) => {
      if (e.lengthComputable) {
        const percent = Math.round((e.loaded / e.total) * 100);
        const totalProgress = 50 + Math.round(percent * 0.5);
        if (progressBar) progressBar.style.width = totalProgress + '%';
        if (progressTxt) progressTxt.textContent = percent + '%';
        if (progressLabel) {
          progressLabel.textContent =
            percent < 100
              ? `⚡ กำลังเขียนลง Flash บอร์ด... (${percent}%)`
              : '⚙️ กำลังตรวจสอบและบันทึกระบบ...';
        }
        setHint('onlineOtaMsg', 'hint ok', `กำลังติดตั้งลงอุปกรณ์... ${percent}%`);
      }
    };

    xhrUpload.onload = () => {
      if (btnCheck) btnCheck.disabled = false;
      if (btnUpload) btnUpload.disabled = false;

      if (xhrUpload.status === 200) {
        if (progressBar) progressBar.style.width = '100%';
        if (progressTxt) progressTxt.textContent = '100%';
        if (progressLabel) progressLabel.textContent = '✓ อัปเดตสำเร็จ!';

        let msg = 'อัปเดตระบบสำเร็จ กำลังรีสตาร์ทเครื่อง...';
        try {
          const resp = JSON.parse(xhrUpload.responseText);
          if (resp.message) msg = resp.message;
        } catch (e) {}

        setHint('onlineOtaMsg', 'hint ok', msg);

        let count = 10;
        const interval = setInterval(() => {
          setHint('onlineOtaMsg', 'hint ok', `รีสตาร์ทสำเร็จ กำลังโหลดหน้าใหม่ใน ${count} วินาที...`);
          count--;
          if (count < 0) {
            clearInterval(interval);
            window.location.reload();
          }
        }, 1000);
      } else {
        let errMsg = 'ติดตั้งเฟิร์มแวร์ไม่สำเร็จ';
        try {
          const resp = JSON.parse(xhrUpload.responseText);
          if (resp.error) errMsg = resp.error;
        } catch (e) {}
        setHint('onlineOtaMsg', 'hint err', errMsg);
      }
    };

    xhrUpload.onerror = () => {
      if (btnCheck) btnCheck.disabled = false;
      if (btnUpload) btnUpload.disabled = false;
      setHint('onlineOtaMsg', 'hint err', 'เกิดข้อผิดพลาดในการเชื่อมต่อกับบอร์ด AutoDoor');
    };

    xhrUpload.send(formData);
  };

  xhrDownload.onerror = () => {
    if (btnCheck) btnCheck.disabled = false;
    if (btnUpload) btnUpload.disabled = false;
    setHint('onlineOtaMsg', 'hint err', 'เกิดข้อผิดพลาดในการดาวน์โหลดจาก GitHub');
  };

  xhrDownload.send();
}

async function uploadOtaFirmware() {
  const fileInput = $('otaFileInput');
  const file = fileInput?.files?.[0];
  if (!file) return;

  const btn = $('btnUploadOta');
  const progressWrap = $('otaProgressWrap');
  const progressBar = $('otaProgressBar');
  const progressTxt = $('otaProgressTxt');
  const progressLabel = $('otaProgressLabel');

  if (btn) {
    btn.disabled = true;
    btn.textContent = 'กำลังอัปโหลดไฟล์...';
  }
  if (progressWrap) progressWrap.classList.remove('hidden');
  if (progressBar) progressBar.style.width = '0%';
  if (progressTxt) progressTxt.textContent = '0%';
  if (progressLabel) progressLabel.textContent = '⚡ กำลังติดตั้งเฟิร์มแวร์...';
  setHint('otaMsg', 'hint ok', 'กำลังเริ่มอัปโหลด...');

  const formData = new FormData();
  formData.append('update', file, file.name);

  const xhr = new XMLHttpRequest();
  xhr.open('POST', '/api/ota', true);

  xhr.upload.onprogress = (e) => {
    if (e.lengthComputable) {
      const percent = Math.round((e.loaded / e.total) * 100);
      if (progressBar) progressBar.style.width = percent + '%';
      if (progressTxt) progressTxt.textContent = percent + '%';
      if (percent < 100) {
        setHint('otaMsg', 'hint ok', `กำลังอัปโหลด... ${percent}%`);
      } else {
        setHint('otaMsg', 'hint ok', 'อัปโหลดเสร็จสิ้น กำลังติดตั้งระบบ...');
      }
    }
  };

  xhr.onload = () => {
    if (btn) {
      btn.disabled = false;
      btn.textContent = '📁 เลือกไฟล์ .bin จากเครื่อง';
    }
    if (xhr.status === 200) {
      let msg = 'อัปเดตระบบสำเร็จ กำลังรีสตาร์ทเครื่อง...';
      try {
        const resp = JSON.parse(xhr.responseText);
        if (resp.message) msg = resp.message;
      } catch (e) {}
      setHint('otaMsg', 'hint ok', msg);
      setTimeout(() => {
        let count = 10;
        const interval = setInterval(() => {
          setHint('otaMsg', 'hint ok', `รีสตาร์ทสำเร็จ กำลังโหลดหน้าใหม่ใน ${count} วินาที...`);
          count--;
          if (count < 0) {
            clearInterval(interval);
            window.location.reload();
          }
        }, 1000);
      }, 1000);
    } else {
      let errMsg = 'อัปเดตไม่สำเร็จ';
      try {
        const resp = JSON.parse(xhr.responseText);
        if (resp.error) errMsg = resp.error;
      } catch (e) {}
      setHint('otaMsg', 'hint err', errMsg);
    }
  };

  xhr.onerror = () => {
    if (btn) {
      btn.disabled = false;
      btn.textContent = '📁 เลือกไฟล์ .bin จากเครื่อง';
    }
    setHint('otaMsg', 'hint err', 'เกิดข้อผิดพลาดในการเชื่อมต่อ');
  };

  xhr.send(formData);
}


