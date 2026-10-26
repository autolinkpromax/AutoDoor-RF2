function toggleSetupWizard() {
  State.setupWizardOpen = !State.setupWizardOpen;
  $('setupBody')?.classList.toggle('collapsed', !State.setupWizardOpen);
  $('setupWizardCard')?.classList.toggle('open', State.setupWizardOpen);
}

function toggleSetGroup(groupId) {
  const card = $(groupId);
  const body = $(groupId + 'Body');
  if (!card || !body) return;
  const willOpen = body.classList.contains('collapsed');
  body.classList.toggle('collapsed', !willOpen);
  card.classList.toggle('open', willOpen);
  
  if (groupId === 'cardWifiHome' && typeof setEspNowDisabled === 'function') {
    if (State.safeMode) {
      setEspNowDisabled(willOpen);
    }
  }
  if (groupId === 'cardEventHistory' && willOpen && typeof loadHistoryDates === 'function') {
    loadHistoryDates();
  }
}

function openSetGroup(groupId) {
  const card = $(groupId);
  const body = $(groupId + 'Body');
  if (!card || !body) return;
  body.classList.remove('collapsed');
  card.classList.add('open');
  
  if (groupId === 'cardWifiHome' && typeof setEspNowDisabled === 'function') {
    if (State.safeMode) {
      setEspNowDisabled(true);
    }
  }
}

function setupScrollTo(el) {
  if (!el) return;
  setTimeout(() => el.scrollIntoView({ behavior: 'smooth', block: 'start' }), 80);
}

function setupGoWifi() {
  showView('settings');
  openSetGroup('setGroupNetwork');
  openSetGroup('cardWifiHome');
  setupScrollTo($('cardWifiHome'));
}

function setupGoTag() {
  showView('devices');
  setTimeout(() => openEditorNew(), 120);
}

function setupGoBlynk() {
  showView('settings');
  openSetGroup('setGroupNetwork');
  openSetGroup('cardBlynk');
  setupScrollTo($('cardBlynk'));
}

function refreshSetupWizardUi(settings, blynk, tagCount) {
  if (settings) State.settingsCache = settings;
  if (blynk) State.blynkCache = blynk;
  if (typeof tagCount === 'number') {
    State.tagCount = tagCount;
  } else if (Array.isArray(tagCount)) {
    State.tagCount = tagCount.length;
  } else if (typeof settings?.tagCount === 'number') {
    State.tagCount = settings.tagCount;
  }

  const curSettings = State.settingsCache || {};
  const curBlynk = State.blynkCache || {};
  const curTagCount = typeof State.tagCount === 'number'
    ? State.tagCount
    : (typeof curSettings?.tagCount === 'number' ? curSettings.tagCount : 0);

  const wifiDone = !!curSettings?.staConnected;
  const tagDone = curTagCount > 0;
  const blynkDone = !!curBlynk?.enabled && (curBlynk.token || State.blynkTokenSaved || '').length >= 8;
  const motorRev = !!curSettings?.motorReverse;

  const adminPinDone = !!State.webAuthHasPassword;
  $('setupCardWifi')?.classList.toggle('done', wifiDone);
  $('setupCardMotor')?.classList.toggle('done', true);
  $('setupCardTag')?.classList.toggle('done', tagDone);
  $('setupCardBlynk')?.classList.toggle('done', blynkDone);
  $('setupCardAdminPin')?.classList.toggle('done', adminPinDone);

  const subWifi = $('setupSubWifi');
  if (subWifi) {
    subWifi.textContent = wifiDone
      ? `เชื่อมแล้ว · ${curSettings.staIp || curSettings.wifiSsid || 'Wi‑Fi บ้าน'}`
      : 'เชื่อมต่อ Wi‑Fi บ้านเพื่อสั่งงานผ่านแอปและตั้งเวลาอัตโนมัติ';
  }
  const subMotor = $('setupSubMotor');
  if (subMotor) {
    subMotor.textContent = motorRev
      ? 'ทิศทาง: กลับทิศทาง (ขวา) · หากเลื่อนเปิดผิดทางให้กดสลับ'
      : 'ทิศทาง: ปกติ (ซ้าย) · หากเลื่อนเปิดผิดทางให้กดสลับ';
  }
  const revBtn = $('btnWizReverseMotor');
  if (revBtn) {
    revBtn.textContent = motorRev ? 'สลับกลับเป็นทิศทางปกติ (ซ้าย)' : 'สลับเป็นทิศทางกลับด้าน (ขวา)';
  }
  const subTag = $('setupSubTag');
  if (subTag) {
    subTag.textContent = tagDone ? `มี Tag ${curTagCount} รายการ` : 'ยังไม่มี Tag ในระบบ';
  }
  const subBlynk = $('setupSubBlynk');
  if (subBlynk) {
    subBlynk.textContent = blynkDone
      ? 'ตั้งค่า Blynk แล้ว'
      : 'เชื่อมต่อแอปบนมือถือ เพื่อสั่งเปิด-ปิดประตูจากทุกที่';
  }
  const subAdminPin = $('setupSubAdminPin');
  if (subAdminPin) {
    subAdminPin.textContent = adminPinDone
      ? 'ตั้งรหัสผ่านความปลอดภัยแล้ว'
      : 'ป้องกันบุคคลอื่นในวง Wi‑Fi แอบสั่งเปิดประตูหรือแก้ไขการตั้งค่า';
  }
  const prog = $('setupProg');
  if (prog) prog.textContent = (wifiDone ? 1 : 0) + 1 + (tagDone ? 1 : 0) + (blynkDone ? 1 : 0) + (adminPinDone ? 1 : 0) + '/5';
}

async function refreshSetupWizard(tagsCountOverride) {
  try {
    const needTagsFetch = (typeof tagsCountOverride !== 'number' || tagsCountOverride <= 0);
    const [settings, blynk, tagsRes] = await Promise.all([
      api('/api/settings'),
      State.blynkCache ? Promise.resolve(State.blynkCache) : api('/api/blynk').catch(() => ({})),
      needTagsFetch ? api('/api/tags').catch(() => ({ tags: [] })) : Promise.resolve(null),
    ]);
    if (blynk && !State.blynkCache) State.blynkCache = blynk;
    if (tagsRes && Array.isArray(tagsRes.tags)) {
      State.tagCount = tagsRes.tags.length;
    } else if (typeof settings?.tagCount === 'number') {
      State.tagCount = settings.tagCount;
    } else if (typeof tagsCountOverride === 'number') {
      State.tagCount = tagsCountOverride;
    }
    refreshSetupWizardUi(settings, blynk, State.tagCount);
  } catch (_) {}
}

async function refreshSettingsFromLive() {
  State.settingsPollTick += 1;
  try {
    const settings = await api('/api/settings');
    updateWifiStaUi(settings);
    if (typeof settings?.tagCount === 'number') {
      State.tagCount = settings.tagCount;
    }
    refreshSetupWizardUi(settings, State.blynkCache, State.tagCount);
    if (State.settingsPollTick % 4 === 0) {
      await refreshBlynkStatus();
      if (typeof refreshMqttStatus === 'function') await refreshMqttStatus();
    }
  } catch (_) {}
}

async function loadSettingsForm() {
  const [settings, statusInfo] = await Promise.all([
    api('/api/settings').catch(() => ({})),
    api('/api/status').catch(() => ({})),
  ]);
  const [tagsRes, time] = await Promise.all([
    api('/api/tags').catch(() => ({ tags: [] })),
    api('/api/time').catch(() => ({})),
  ]);
  const [blynk, sched, mqtt] = await Promise.all([
    api('/api/blynk').catch(() => ({})),
    api('/api/schedule').catch(() => ({ schedules: [] })),
    api('/api/mqtt').catch(() => ({})),
  ]);
  if (tagsRes && Array.isArray(tagsRes.tags)) {
    State.tagCount = tagsRes.tags.length;
  } else if (typeof settings?.tagCount === 'number') {
    State.tagCount = settings.tagCount;
  } else if (typeof statusInfo?.tagCount === 'number') {
    State.tagCount = statusInfo.tagCount;
  }
  if (statusInfo?.fwVersion) {
    State.fwVersion = statusInfo.fwVersion;
  }
  if (statusInfo && statusInfo.maintenance) {
    updateMaintenanceUi(statusInfo.maintenance);
  }
  if (statusInfo && statusInfo.ota && typeof renderOtaStatus === 'function') {
    renderOtaStatus(statusInfo.ota, statusInfo);
  }
  if ($('wifiSsid')) $('wifiSsid').value = settings.wifiSsid || '';
  if ($('swTagAuto')) $('swTagAuto').checked = settings.tagAutoEnabled !== false;
  if ($('swTagBuzzer')) $('swTagBuzzer').checked = settings.tagBuzzerEnabled !== false;
  if ($('swSimMode')) $('swSimMode').checked = !!settings.simMode;
  if ($('swMotorReverse')) $('swMotorReverse').checked = !!settings.motorReverse;
  if ($('swDoubleClose')) $('swDoubleClose').checked = settings.doubleClosePulse !== false;
  if ($('deviceName')) $('deviceName').value = settings.deviceName || 'AutoDoor';
  if ($('deviceHostname')) {
    const host = settings.hostname || '';
    window.gDefaultHostname = settings.defaultHostname || '';
    $('deviceHostname').value = host;
    if (window.gDefaultHostname) $('deviceHostname').placeholder = window.gDefaultHostname;
    if (typeof updateDeviceHostnameLink === 'function') updateDeviceHostnameLink(host || window.gDefaultHostname);
  }
  
  if ($('swMagSensor')) $('swMagSensor').checked = !!settings.magSensorEnabled;
  if ($('magSensorType')) $('magSensorType').value = settings.magSensorType ?? 0;
  const mOpt = $('magSensorOptions');
  if (mOpt) mOpt.classList.toggle('hidden', !settings.magSensorEnabled);
  const calSec = $('magAutoCalibrateSection');
  if (calSec) calSec.classList.toggle('hidden', !settings.magSensorEnabled);

  if ($('swAutoRetryClose')) $('swAutoRetryClose').checked = settings.autoRetryCloseEnabled !== false;
  if ($('maxCloseRetryCount')) $('maxCloseRetryCount').value = settings.maxCloseRetryCount || 3;
  if ($('closeRetryDelaySec')) $('closeRetryDelaySec').value = settings.closeRetryDelayMs ? Math.round(settings.closeRetryDelayMs / 1000) : 3;
  const rOpt = $('autoRetryCloseParams');
  if (rOpt) rOpt.classList.toggle('hidden', settings.autoRetryCloseEnabled === false);

  if ($('swSafetySensor')) $('swSafetySensor').checked = !!settings.safetySensorEnabled;
  if ($('safetySensorType')) $('safetySensorType').value = settings.safetySensorType ?? 0;
  if ($('safetyCloseDelaySec')) $('safetyCloseDelaySec').value = settings.safetyCloseDelayMs != null ? Math.round(settings.safetyCloseDelayMs / 1000) : 3;
  if ($('safetyReverseDelaySec')) $('safetyReverseDelaySec').value = settings.safetyReverseDelayMs != null ? Math.round(settings.safetyReverseDelayMs / 1000) : 5;
  if ($('swRemotePassageClose')) $('swRemotePassageClose').checked = settings.remotePassageClose !== false;
  if ($('remotePassageDelaySec')) $('remotePassageDelaySec').value = settings.remotePassageDelayMs != null ? Math.round(settings.remotePassageDelayMs / 1000) : 5;
  const remSecEl = $('remotePassageDelaySection');
  if (remSecEl) remSecEl.classList.toggle('hidden', settings.remotePassageClose === false);

  const delaySec = $('safetyDelaySection');
  if (delaySec) delaySec.classList.toggle('hidden', !settings.safetySensorEnabled);
  State.safetySensorEnabled = !!settings.safetySensorEnabled;
  const sensBadge = $('sensorsHeadBadge');
  if (sensBadge) {
    sensBadge.textContent = settings.safetySensorEnabled ? 'กันหนีบ: เปิด' : 'กันหนีบ: ปิด';
    sensBadge.className = settings.safetySensorEnabled ? 'badgeMini ok' : 'badgeMini off';
  }

  const animSec = settings.doorTransitionMs ? Math.round(settings.doorTransitionMs / 1000) : 5;
  if ($('doorAnimSec')) $('doorAnimSec').value = animSec;
  if ($('motorHeadBadge')) $('motorHeadBadge').textContent = `${animSec} วิ`;

  updateWifiStaUi(settings);
  State.blynkTokenSaved = blynk.token || '';
  State.blynkCache = blynk;
  if ($('swBlynk')) $('swBlynk').checked = !!blynk.enabled;
  if ($('blynkTemplateId')) $('blynkTemplateId').value = blynk.templateId || '';
  if ($('blynkDeviceName')) $('blynkDeviceName').value = blynk.deviceName || 'AutoDoor';
  if ($('blynkToken')) {
    $('blynkToken').value = '';
    $('blynkToken').placeholder = State.blynkTokenSaved
      ? 'บันทึกแล้ว (...' + State.blynkTokenSaved.slice(-4) + ')'
      : 'รหัสอุปกรณ์จากแอป Blynk';
  }
  updateBlynkStatusUi(blynk);
  if (typeof updateMqttUi === 'function') updateMqttUi(mqtt);
  const timeLbl = $('sysTimeLbl');
  if (timeLbl) timeLbl.textContent = time.localTime || (time.synced ? 'ซิงค์แล้ว' : 'ยังไม่ซิงค์');
  const timeHint = $('sysTimeHint');
  if (timeHint) {
    timeHint.textContent = time.synced ? 'เวลาตรงกับเวลามาตรฐานแล้ว' : 'เชื่อมต่อ Wi‑Fi บ้านเพื่อซิงค์เวลาอัตโนมัติ';
  }
  const timeBadge = $('timeHeadBadge');
  if (timeBadge) {
    if (time.synced) {
      const hm = time.localTime ? (time.localTime.split(' ')[1]?.slice(0, 5) || 'ซิงค์แล้ว') : 'ซิงค์แล้ว';
      timeBadge.textContent = hm;
      timeBadge.className = 'badgeMini ok';
    } else {
      timeBadge.textContent = 'ยังไม่ซิงค์';
      timeBadge.className = 'badgeMini off';
    }
  }

  const rebootEn = settings.autoRebootEnabled !== false;
  if ($('swAutoReboot')) $('swAutoReboot').checked = rebootEn;
  const rh = pad2(settings.autoRebootHour ?? 3);
  const rm = pad2(settings.autoRebootMinute ?? 0);
  if ($('rebootTimeInput')) $('rebootTimeInput').value = `${rh}:${rm}`;
  const rebWrap = $('autoRebootTimeWrap');
  if (rebWrap) rebWrap.classList.toggle('hidden', !rebootEn);
  const rebBadge = $('rebootHeadBadge');
  if (rebBadge) rebBadge.textContent = rebootEn ? `เปิด (${rh}:${rm})` : 'ปิดใช้งาน';
  State.scheduleRows = (sched.schedules || []).map((s) => ({
    id: s.id,
    enabled: !!s.enabled,
    action: s.action || 'open',
    hour: s.hour ?? 8,
    minute: s.minute ?? 0,
    days: s.days && s.days.length ? s.days : [0, 1, 2, 3, 4, 5, 6],
  }));
  renderScheduleList();
  refreshSetupWizard(State.tagCount);
  loadWebhookUi();
  loadApUi();
  loadWebAuthUi();
  if (typeof loadHistoryDates === 'function') {
    loadHistoryDates();
  }
}

function togglePw(id, btn) {
  const inp = $(id);
  if (!inp) return;
  const show = inp.type === 'password';
  inp.type = show ? 'text' : 'password';
  if (btn) btn.style.opacity = show ? '1' : '.55';
}

async function wizardTestOpen() {
  const btn = $('btnWizTestOpen');
  if (btn) btn.disabled = true;
  setHint('wizMotorMsg', '', 'กำลังส่งคำสั่งเปิดประตูทดสอบ...');
  try {
    const res = await api('/api/door', { method: 'POST', body: { slot: 'open' } });
    if (res.ok !== false) {
      setHint('wizMotorMsg', 'hint ok', 'ส่งคำสั่งเปิดประตูแล้ว — สังเกตว่าประตูเปิดจริงหรือไม่ หากประตูปิดให้กด "สลับทิศทาง"');
    } else {
      setHint('wizMotorMsg', 'hint err', 'ส่งคำสั่งไม่สำเร็จ');
    }
  } catch (e) {
    setHint('wizMotorMsg', 'hint err', 'เกิดข้อผิดพลาดในการสั่งเปิด');
  } finally {
    setTimeout(() => { if (btn) btn.disabled = false; }, 2000);
  }
}

async function wizardToggleMotorReverse() {
  const btn = $('btnWizReverseMotor');
  if (btn) btn.disabled = true;
  const currentRev = !!State.settingsCache?.motorReverse;
  const newRev = !currentRev;
  setHint('wizMotorMsg', '', 'กำลังสลับทิศทางมอเตอร์...');
  try {
    const res = await api('/api/settings', { method: 'PUT', body: { motorReverse: newRev } });
    if (res.ok) {
      if (!State.settingsCache) State.settingsCache = {};
      State.settingsCache.motorReverse = newRev;
      if ($('swMotorReverse')) $('swMotorReverse').checked = newRev;
      const statusTxt = newRev ? 'กลับทิศทาง (ขวา)' : 'ปกติ (ซ้าย)';
      setHint('wizMotorMsg', 'hint ok', `สลับทิศทางมอเตอร์เป็น "${statusTxt}" สำเร็จ! แนะนำกดทดสอบเปิดอีกครั้ง`);
      refreshSetupWizardUi(State.settingsCache, State.blynkCache, State.tagCount);
    } else {
      setHint('wizMotorMsg', 'hint err', 'บันทึกทิศทางไม่สำเร็จ');
    }
  } catch (e) {
    setHint('wizMotorMsg', 'hint err', 'เกิดข้อผิดพลาดในการบันทึก');
  } finally {
    setTimeout(() => { if (btn) btn.disabled = false; }, 1000);
  }
}

async function wizardSaveAdminPin() {
  const inp = $('wizAdminPass');
  const pass = inp?.value?.trim() || '';
  if (!pass || pass.length < 4) {
    setHint('wizAdminPinMsg', 'hint err', 'กรุณาระบุรหัสผ่านอย่างน้อย 4 ตัวอักษร');
    return;
  }
  setHint('wizAdminPinMsg', '', 'กำลังบันทึกรหัสผ่าน...');
  try {
    const res = await api('/api/webauth', {
      method: 'POST',
      body: { password: pass }
    });
    if (res.hasPassword) {
      State.webAuthHasPassword = true;
      setHint('wizAdminPinMsg', 'hint ok', 'ตั้งรหัสผ่านความปลอดภัยสำเร็จแล้ว!');
      inp.value = '';
      refreshSetupWizardUi(State.settingsCache, State.blynkCache, State.tagCount);
      loadWebAuthUi();
    } else {
      setHint('wizAdminPinMsg', 'hint err', 'บันทึกไม่สำเร็จ');
    }
  } catch (e) {
    setHint('wizAdminPinMsg', 'hint err', 'เกิดข้อผิดพลาดในการบันทึก: ' + e.message);
  }
}

function setupGoWebAuth() {
  showView('settings');
  openSetGroup('setGroupSystem');
  openSetGroup('cardWebAuth');
  setupScrollTo($('cardWebAuth'));
}

function goToMaintenance() {
  showView('settings');
  openSetGroup('setGroupDoor');
  openSetGroup('cardMaintenance');
  setupScrollTo($('cardMaintenance'));
}

function updateMaintenanceUi(maint) {
  if (!maint) return;
  State.maintenanceCache = maint;
  if ($('maintTotalCycles')) $('maintTotalCycles').textContent = (maint.totalCycles ?? 0).toLocaleString();
  if ($('maintServiceCycles')) $('maintServiceCycles').textContent = (maint.serviceCycles ?? 0).toLocaleString();
  if ($('maintLastDate')) $('maintLastDate').textContent = maint.lastServiceDate || 'ยังไม่มีประวัติ';
  if ($('maintThresholdInp')) $('maintThresholdInp').value = maint.serviceThreshold || 5000;

  const isDue = !!maint.serviceDue;
  const alertBanner = $('maintAlertBanner');
  if (alertBanner) alertBanner.classList.toggle('hidden', !isDue);

  const dashAlert = $('dashMaintenanceAlert');
  if (dashAlert) dashAlert.classList.toggle('hidden', !isDue);

  const badge = $('maintHeadBadge');
  if (badge) {
    if (isDue) {
      badge.textContent = 'ถึงกำหนดเช็ค!';
      badge.className = 'badgeMini warn';
    } else {
      badge.textContent = `${(maint.serviceCycles ?? 0).toLocaleString()} รอบ`;
      badge.className = 'badgeMini';
    }
  }
}

async function resetServiceReminder() {
  if (!confirm('ยืนยันว่าทำการตรวจเช็คความตึงโซ่ หยอดจาระบี และตรวจรางเลื่อนเรียบร้อยแล้ว?\nระบบจะทำการรีเซ็ตตัวนับรอบการเช็คระยะเป็น 0 และบันทึกวันที่ปัจจุบัน')) {
    return;
  }
  const now = new Date();
  const y = now.getFullYear();
  const m = String(now.getMonth() + 1).padStart(2, '0');
  const d = String(now.getDate()).padStart(2, '0');
  const dateStr = `${y}-${m}-${d}`;

  setHint('maintResetMsg', '', 'กำลังบันทึกการเช็คระยะ...');
  try {
    const res = await api('/api/maintenance/reset-service', {
      method: 'POST',
      body: { date: dateStr }
    });
    if (res.ok) {
      setHint('maintResetMsg', 'hint ok', 'บันทึกการตรวจเช็คระยะเรียบร้อยแล้ว (รีเซ็ตรอบเป็น 0)');
      if (State.maintenanceCache) {
        State.maintenanceCache.serviceCycles = 0;
        State.maintenanceCache.serviceDue = false;
        State.maintenanceCache.lastServiceDate = dateStr;
        updateMaintenanceUi(State.maintenanceCache);
      }
    } else {
      setHint('maintResetMsg', 'hint err', 'บันทึกไม่สำเร็จ');
    }
  } catch (e) {
    setHint('maintResetMsg', 'hint err', 'เกิดข้อผิดพลาดในการบันทึก');
  }
}

async function saveMaintenanceThreshold() {
  const inp = $('maintThresholdInp');
  const val = parseInt(inp?.value, 10);
  if (!val || val < 100 || val > 100000) {
    setHint('maintThreshMsg', 'hint err', 'กรุณาระบุจำนวนรอบระหว่าง 100 ถึง 100,000 รอบ');
    return;
  }
  setHint('maintThreshMsg', '', 'กำลังบันทึก...');
  try {
    const res = await api('/api/maintenance/settings', {
      method: 'POST',
      body: { threshold: val }
    });
    if (res.ok) {
      setHint('maintThreshMsg', 'hint ok', `บันทึกรอบแจ้งเตือนเป็น ${val.toLocaleString()} รอบแล้ว`);
      if (State.maintenanceCache) {
        State.maintenanceCache.serviceThreshold = val;
        State.maintenanceCache.serviceDue = (State.maintenanceCache.serviceCycles >= val);
        updateMaintenanceUi(State.maintenanceCache);
      }
    } else {
      setHint('maintThreshMsg', 'hint err', 'บันทึกไม่สำเร็จ');
    }
  } catch (e) {
    setHint('maintThreshMsg', 'hint err', 'เกิดข้อผิดพลาดในการบันทึก');
  }
}

async function confirmAndFactoryReset() {
  const confirmed = confirm(
    "⚠️ คุณแน่ใจหรือไม่ว่าต้องการรีเซ็ตการตั้งค่าระบบทั้งหมดเป็นค่าเริ่มต้นโรงงาน (Factory Reset)?\n\n" +
    "• การตั้งค่า Wi‑Fi, รหัสผ่าน, และระบบจะถูกล้างใหม่ทั้งหมด\n" +
    "• Tag และรีโมททั้งหมดจะยังคงอยู่ ไม่สูญหาย\n" +
    "• ตัวเครื่องจะรีสตาร์ทกลับสู่โหมดเริ่มต้น (Setup Wizard)"
  );
  if (!confirmed) return;

  setHint('factoryResetMsg', 'hint warn', 'กำลังดำเนินการรีเซ็ตค่าโรงงาน กรุณารอสักครู่...');
  try {
    const res = await api('/api/system/factory-reset', { method: 'POST' });
    if (res.ok) {
      setHint('factoryResetMsg', 'hint ok', 'รีเซ็ตสำเร็จแล้ว! เครื่องกำลังรีสตาร์ท...');
      setTimeout(() => {
        alert('ตัวเครื่องได้ทำการรีเซ็ตเรียบร้อยแล้วและกำลังรีสตาร์ท\nกรุณาเชื่อมต่อ Wi-Fi "AutoLink-XXXX" เพื่อเข้าสู่ตัวช่วยตั้งค่าเริ่มต้น (Setup Wizard)');
        window.location.href = 'http://192.168.4.1/';
      }, 2500);
    } else {
      setHint('factoryResetMsg', 'hint err', 'รีเซ็ตไม่สำเร็จ: ' + (res.msg || 'error'));
    }
  } catch (e) {
    setHint('factoryResetMsg', 'hint ok', 'เครื่องกำลังรีสตาร์ทเข้าสู่โหมดเริ่มต้น...');
    setTimeout(() => {
      window.location.href = 'http://192.168.4.1/';
    }, 3000);
  }
}

