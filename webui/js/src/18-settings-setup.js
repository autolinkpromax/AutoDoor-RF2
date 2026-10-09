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

  $('setupCardWifi')?.classList.toggle('done', wifiDone);
  $('setupCardTag')?.classList.toggle('done', tagDone);
  $('setupCardBlynk')?.classList.toggle('done', blynkDone);

  const subWifi = $('setupSubWifi');
  if (subWifi) {
    subWifi.textContent = wifiDone
      ? `เชื่อมแล้ว · ${curSettings.staIp || curSettings.wifiSsid || 'Wi‑Fi บ้าน'}`
      : 'เชื่อมต่อ Wi‑Fi บ้านเพื่อสั่งงานผ่านแอปและตั้งเวลาอัตโนมัติ';
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
  const prog = $('setupProg');
  if (prog) prog.textContent = (wifiDone ? 1 : 0) + (tagDone ? 1 : 0) + (blynkDone ? 1 : 0) + '/3';
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
  const [blynk, sched] = await Promise.all([
    api('/api/blynk').catch(() => ({})),
    api('/api/schedule').catch(() => ({ schedules: [] })),
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
