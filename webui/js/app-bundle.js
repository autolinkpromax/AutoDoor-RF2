/* generated — edit webui/js/src/ */
const TAG_DEF = {
  openDurationSec: 0,
  holdSec: 20,
  rfHoldSec: 1,
  closeDelaySec: 10,
  rssiMin: -100,
  passRssi: -90,
  passCloseSec: 5,
  safetyCloseDelaySec: 3,
};

const DAY_LABELS = ['อา', 'จ', 'อ', 'พ', 'พฤ', 'ศ', 'ส'];

const RSSI_ZONE_LABEL = {
  approaching: 'กำลังเข้าใกล้',
  in_range: 'สัญญาณปกติ',
  departing: 'กำลังออก',
  out: 'นอกระยะ',
};

const CLOCK_SVG =
  '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 2"/></svg>';

const TAG_SIG_SVG =
  '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"><circle cx="12" cy="12" r="2"/><path d="M16.24 7.76a6 6 0 0 1 0 8.49M7.76 7.76a6 6 0 0 0 0 8.49M19.07 4.93a10 10 0 0 1 0 14.14M4.93 4.93a10 10 0 0 0 0 14.14"/></svg>';

const RF_REMOTE_SVG =
  '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="6" y="3" width="12" height="18" rx="3"/><circle cx="12" cy="8" r="1.5" fill="currentColor"/><circle cx="12" cy="13" r="1.5" fill="currentColor"/><path d="M12 17h.01"/></svg>';

const VEHICLE_ICONS = {
  suv: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M19 17h2c.6 0 1-.4 1-1v-3c0-.9-.7-1.7-1.5-1.9C18.7 10.6 16 10 16 10s-1.3-1.4-2.2-2.3c-.5-.4-1.1-.7-1.8-.7H5c-.6 0-1.1.4-1.4.9l-1.5 2.8C2 11 2 11.3 2 11.6V16c0 .6.4 1 1 1h2"/><circle cx="7" cy="17" r="2.5"/><circle cx="17" cy="17" r="2.5"/><path d="M5 11h11"/></svg>',
  sedan: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="1" y="5" width="15" height="11" rx="2"/><circle cx="5.5" cy="18.5" r="2.5"/><circle cx="17.5" cy="18.5" r="2.5"/><path d="M16 8h4l3 4v4h-7V8z"/></svg>',
  pickup: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 17h1m4 0h8m4 0h1a1 1 0 0 0 1-1v-4a2 2 0 0 0-2-2h-3l-2-4H5a2 2 0 0 0-2 2v8a1 1 0 0 0 1 1"/><circle cx="6" cy="17" r="2.5"/><circle cx="18" cy="17" r="2.5"/><path d="M11 6v5h10"/></svg>',
  bike: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="5.5" cy="17.5" r="3.5"/><circle cx="18.5" cy="17.5" r="3.5"/><path d="M15 6h2m-5 11.5l3-7.5h3.5M5.5 17.5l4-7.5h5"/></svg>',
  scooter: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="6" cy="18" r="3"/><circle cx="18" cy="18" r="3"/><path d="M18 15V7h-3"/><path d="M9 18h6"/><path d="M6 15l3-5h4"/></svg>',
  van: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="6" width="20" height="11" rx="2"/><circle cx="7" cy="17" r="2.5"/><circle cx="17" cy="17" r="2.5"/><path d="M2 12h20M9 6v6"/></svg>',
  rf: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="6" y="3" width="12" height="18" rx="3"/><circle cx="12" cy="8" r="1.5" fill="currentColor"/><circle cx="12" cy="13" r="1.5" fill="currentColor"/><path d="M12 17h.01"/></svg>',
  bicycle: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="5.5" cy="17.5" r="3.5"/><circle cx="18.5" cy="17.5" r="3.5"/><path d="M12 17.5V14l-3-3 4-3 2 3h3M12 14l3.5 3.5"/></svg>',
  person: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="5" r="3"/><path d="M12 8v8m-3 3l3-3 3 3m-6-10l3 2 3-2"/></svg>',
  auto: TAG_SIG_SVG,
};

const VEHICLE_EMOJIS = {
  suv: '🚙',
  sedan: '🚗',
  car: '🚗',
  pickup: '🚚',
  bike: '🏍️',
  scooter: '🛵',
  van: '🚐',
  rf: '🔑',
  bicycle: '🚲',
  person: '🚶',
  auto: '🏷️',
  web: '💻',
  app: '📱',
  schedule: '⏰',
  btn: '🔘',
  ext: '⚡',
};

function getVehicleEmoji(iconKey, isRf = false) {
  if (iconKey && iconKey !== 'auto' && VEHICLE_EMOJIS[iconKey]) {
    return VEHICLE_EMOJIS[iconKey];
  }
  return isRf ? '🔑' : '🏷️';
}

function getVehicleIconSvg(iconKey, isRf = false) {
  if (iconKey && iconKey !== 'auto' && VEHICLE_ICONS[iconKey]) {
    return VEHICLE_ICONS[iconKey];
  }
  return isRf ? RF_REMOTE_SVG : TAG_SIG_SVG;
}

const ICO_TRASH =
  '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M3 6h18M8 6V4h8v2M19 6l-1 14H6L5 6"/></svg>';

const ICO_EDIT =
  '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 20h9M16.5 3.5a2.1 2.1 0 0 1 3 3L7 19l-4 1 1-4 12.5-12.5z"/></svg>';

const RSSI_BAR_HEIGHTS = [10, 14, 18, 22, 26];

const GITHUB_FW_REPO = 'autolinkpromax/AutoDoor-RF2';
const GITHUB_FW_BASE_URL = 'https://raw.githubusercontent.com/' + GITHUB_FW_REPO + '/main/firmware';
const GITHUB_FW_FALLBACK_URL = 'https://raw.githubusercontent.com/' + GITHUB_FW_REPO + '/master/firmware';
const Dom = {
  _c: {},
  get(id) {
    const el = this._c[id];
    if (el && el.isConnected) return el;
    const found = document.getElementById(id);
    if (found) this._c[id] = found;
    return found;
  },
  clear() {
    this._c = {};
  },
};

function $(id) {
  return Dom.get(id);
}

function escHtml(s) {
  return String(s)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/"/g, '&quot;');
}

function cleanEntityName(s) {
  if (!s || typeof s !== 'string') return '—';
  let cleaned = s.trim();
  if (cleaned === 'REMOTE') return 'รีโมทประตู';
  // Strip trailing (id) like (72c8c6fa) or (aabbccddeeff) or (0x1234)
  cleaned = cleaned.replace(/\s*\([0-9a-fA-F:_-]+\)$/, '').trim();
  return cleaned || '—';
}

function setHint(id, cls, text) {
  const el = $(id);
  if (!el) return;
  if (cls) el.className = cls;
  if (text != null) el.textContent = text;
}
async function api(path, opts = {}) {
  const timeoutMs = opts.timeoutMs || 4000;
  const controller = new AbortController();
  const timer = setTimeout(() => controller.abort(), timeoutMs);

  try {
    const res = await fetch(path, {
      cache: 'no-store',
      credentials: 'same-origin',
      headers: opts.body ? { 'Content-Type': 'application/json' } : undefined,
      signal: opts.signal || controller.signal,
      ...opts,
      body: opts.body ? JSON.stringify(opts.body) : undefined,
    });
    const data = await res.json().catch(() => ({}));
    if (!res.ok) throw new Error(data.error || 'request_failed');
    return data;
  } finally {
    clearTimeout(timer);
  }
}
const State = {
  liveTimer: null,
  learnTimer: null,
  currentView: 'dash',
  editingId: null,
  scheduleRows: [],
  setupWizardOpen: false,
  blynkTokenSaved: '',
  webhookCache: null,
  appBooted: false,
  liveTagKeys: '',
  liveCloseMode: false,
  deviceListKey: '',
  deviceActiveMap: {},
  settingsPollTick: 0,
  blynkCache: null,
  settingsCache: null,
  tagCount: null,
  fwVersion: '1.0.0',
  latestOnlineFw: null,
  lastLiveData: null,
  lastLiveRecvMs: 0,
  liveTickTimer: null,
};
function normalizeDoorState(s) {
  return !s || s === 'unknown' ? 'closed' : s;
}

function doorStateLabel(s) {
  const st = normalizeDoorState(s);
  const map = {
    open: 'ประตูเปิด',
    closed: 'ประตูปิด',
    opening: 'กำลังเปิด…',
    closing: 'กำลังปิด…',
    stopped: 'หยุด',
  };
  return map[st] || 'ประตูปิด';
}

function pickDoorCloseCountdown(tags, pendingCloseMs, safetyCloseRemainingMs) {
  let ms = null;
  let label = '';
  if (safetyCloseRemainingMs != null && safetyCloseRemainingMs > 0) {
    ms = safetyCloseRemainingMs;
    label = 'ปิดหลังรถผ่านใน';
  } else {
    for (const t of tags) {
      if (t.passCloseMs != null && t.passCloseMs > 0) {
        if (ms == null || t.passCloseMs < ms) {
          ms = t.passCloseMs;
          label = 'ปิดหลังผ่านประตูใน';
        }
      }
    }
    if (pendingCloseMs != null && pendingCloseMs > 0) {
      if (ms == null || pendingCloseMs < ms) {
        ms = pendingCloseMs;
        label = 'ปิดประตูใน';
      }
    }
  }
  return { ms, label };
}

function tagDisplayId(t) {
  const isRf = !!(t.isRf || (t.mac && t.mac.toUpperCase().includes('RF')) || t.tagId);
  if (t.tagId) {
    return 'RF-' + Number(t.tagId).toString(16).toUpperCase().padStart(8, '0');
  }
  if (isRf) {
    const hex = (t.mac || '').replace(/[^0-9A-Fa-f]/g, '').toUpperCase();
    return 'RF-' + hex.slice(-8);
  }
  return t.mac || t.id || '—';
}

function tagZone(t) {
  return t.rssiZone || (t.active ? 'in_range' : 'out');
}

function isTagDetected(t) {
  const zone = tagZone(t);
  return !!t.active || !!t.isTriggerTag || (zone && zone !== 'out');
}

function zoneBadgeClass(zone) {
  if (zone === 'in_range') return '';
  if (zone === 'approaching') return 'warn';
  return 'muted';
}

function formatHoldClock(ms) {
  const sec = Math.max(0, Math.ceil((ms || 0) / 1000));
  const mm = String(Math.floor(sec / 60)).padStart(2, '0');
  const ss = String(sec % 60).padStart(2, '0');
  return mm + ':' + ss;
}

function pad2(n) {
  return String(n).padStart(2, '0');
}

function rssiBarLitCount(rssi, minDbm) {
  const min = minDbm ?? -100;
  const max = -40;
  const val = typeof rssi === 'number' ? rssi : min;
  const norm = Math.max(0, Math.min(1, (val - min) / (max - min)));
  return Math.max(1, Math.round(norm * 5));
}

function rssiBarHtml(rssi, minDbm) {
  const lit = rssiBarLitCount(rssi, minDbm);
  return RSSI_BAR_HEIGHTS.map(
    (h, i) => `<span class="liveTagBar ${i < lit ? 'on' : ''}" style="height:${h}px"></span>`
  ).join('');
}

function tagCloseRemainingMs(t, doorCloseMs) {
  if (doorCloseMs > 0) return doorCloseMs;
  return 0;
}

function liveTagTimerText(t, doorCloseMs, doorCloseLabel) {
  const isRf = !!(t.isRf || (t.mac && t.mac.toUpperCase().includes('RF')) || t.tagId);
  const doorSt = State.lastLiveData?.doorState;

  if (doorSt === 'closing') {
    return 'กำลังปิดประตู…';
  }
  if (doorSt === 'closed') {
    return '';
  }

  // Show close countdown if active from server (both during opening and when open)
  if ((doorSt === 'opening' || doorSt === 'open') && doorCloseMs != null && doorCloseMs > 0) {
    return (doorCloseLabel || 'ปิดประตูใน') + ' ' + formatHoldClock(doorCloseMs);
  }

  if (doorSt === 'opening') {
    return isRf ? '' : (t.active ? 'กำลังส่งสัญญาณ…' : '');
  }

  let holdRem = t.holdRemainingMs != null && t.holdRemainingMs > 0 ? t.holdRemainingMs : 0;
  if (isRf) {
    const age = t.lastSeenAgeMs != null ? t.lastSeenAgeMs : 0;
    holdRem = Math.max(0, 1000 - age);
  }

  if (t.active && holdRem > 0) {
    return (isRf ? 'ทำงาน ' : 'จะหายใน ') + formatHoldClock(holdRem);
  }

  return '';
}

function liveTagStructureKey(t) {
  const isRf = !!(t.isRf || (t.mac && t.mac.toUpperCase().includes('RF')) || t.tagId);
  const zone = tagZone(t);
  const rssi = isRf ? 'rf' : (t.lastRssi != null ? t.lastRssi : '—');
  return `${t.id}|${isRf ? 'rf' : zone}|${rssi}|${t.name}|${t.isTriggerTag ? 'trig' : ''}`;
}
function findActionEl(target, root) {
  return target.closest ? target.closest('[data-action]') : null;
}

function onActionClick(e) {
  const el = findActionEl(e.target);
  if (!el || el.disabled) return;
  const action = el.dataset.action;
  if (!action) return;

  switch (action) {
    case 'door-send':
      if (!el.disabled) doorSend(el.dataset.slot);
      break;
    case 'nav':
      showView(el.dataset.view);
      break;
    case 'delete-tag':
      deleteTagFromList(el.dataset.id, e);
      break;
    case 'open-editor':
      openEditor(el.dataset.id);
      break;
    case 'open-editor-new':
      openEditorNew();
      break;
    case 'close-editor':
      closeEditor();
      break;
    case 'ed-start-learn':
      edStartLearn();
      break;
    case 'save-editor':
      saveEditor();
      break;
    case 'test-ed-webhook':
      testEdWebhook();
      break;
    case 'select-icon':
      selectIconChip(el.dataset.icon);
      break;
    case 'delete-tag-editor':
      deleteTag();
      break;
    case 'toggle-setup-wizard':
      toggleSetupWizard();
      break;
    case 'toggle-set-group':
      toggleSetGroup(el.dataset.group);
      break;
    case 'setup-go-wifi':
      setupGoWifi();
      break;
    case 'setup-go-tag':
      setupGoTag();
      break;
    case 'setup-go-blynk':
      setupGoBlynk();
      break;
    case 'wizard-test-open':
      if (typeof wizardTestOpen === 'function') wizardTestOpen();
      break;
    case 'wizard-toggle-motor-reverse':
      if (typeof wizardToggleMotorReverse === 'function') wizardToggleMotorReverse();
      break;
    case 'wizard-save-admin-pin':
      if (typeof wizardSaveAdminPin === 'function') wizardSaveAdminPin();
      break;
    case 'setup-go-webauth':
      if (typeof setupGoWebAuth === 'function') setupGoWebAuth();
      break;
    case 'go-to-maintenance':
      if (typeof goToMaintenance === 'function') goToMaintenance();
      break;
    case 'reset-service-reminder':
      if (typeof resetServiceReminder === 'function') resetServiceReminder();
      break;
    case 'save-maintenance-threshold':
      if (typeof saveMaintenanceThreshold === 'function') saveMaintenanceThreshold();
      break;
    case 'system-factory-reset':
      if (typeof confirmAndFactoryReset === 'function') confirmAndFactoryReset();
      break;
    case 'toggle-pw':
      togglePw(el.dataset.target, el);
      break;
    case 'wifi-scan':
      wifiScan();
      break;
    case 'wifi-connect':
      wifiConnect();
      break;
    case 'wifi-pick-ssid':
      if ($('wifiSsid')) $('wifiSsid').value = el.dataset.ssid || '';
      break;
    case 'save-ap':
      saveApSettings();
      break;
    case 'save-blynk':
      saveBlynk();
      break;
    case 'save-mqtt':
      saveMqtt();
      break;
    case 'copy-hook':
      copyHookUrl(el.dataset.slot);
      break;
    case 'copy-blynk-hook':
      copyBlynkHookUrl(el.dataset.slot);
      break;
    case 'copy-ha-dashboard':
      copyHaDashboardYaml();
      break;
    case 'copy-ha-lan':
      copyHaLanYaml();
      break;
    case 'copy-ha-blynk':
      copyHaBlynkYaml();
      break;
    case 'save-wh-out':
      saveOutboundWebhook();
      break;
    case 'test-wh-out':
      testOutboundWebhook();
      break;
    case 'dash-filter-events':
      if (typeof setDashEventFilter === 'function') {
        setDashEventFilter(el.dataset.filter);
      }
      break;
    case 'copy-events':
      copyEventLogs();
      break;
    case 'clear-events':
      clearEventLogs();
      break;
    case 'save-events':
    case 'save-events-flash':
      saveEventLogsToFlash();
      break;
    case 'copy-history-events':
      if (typeof copyHistorySummary === 'function') {
        copyHistorySummary();
      } else {
        copyHistoryEvents();
      }
      break;
    case 'export-history-csv':
      if (typeof exportHistoryCsv === 'function') exportHistoryCsv();
      break;
    case 'clear-history-flash':
      clearHistoryFlash();
      break;
    case 'toggle-history-view':
      if (typeof toggleHistoryView === 'function') toggleHistoryView();
      break;
    case 'copy-json-api':
      copyJsonApi();
      break;
    case 'save-webauth':
      saveWebAuth();
      break;
    case 'save-device-name':
      saveDeviceName();
      break;
    case 'save-hostname':
      saveHostname();
      break;
    case 'save-relay-pulse':
      saveRelayPulse();
      break;
    case 'save-safety-sensor-type':
      saveSafetySensorType();
      break;
    case 'save-safety-close-delay':
      saveSafetyCloseDelay();
      break;
    case 'save-safety-reverse-delay':
      saveSafetyReverseDelay();
      break;
    case 'save-remote-passage-delay':
      saveRemotePassageDelay();
      break;
    case 'save-safety-params':
      saveSafetyParams();
      break;
    case 'save-door-anim':
      saveDoorAnim();
      break;
    case 'start-auto-calibrate':
      startAutoCalibrate();
      break;
    case 'cancel-auto-calibrate':
      cancelAutoCalibrate();
      break;
    case 'save-mag-sensor-type':
      saveMagSensorType();
      break;
    case 'save-auto-retry-params':
      saveAutoRetryParams();
      break;
    case 'open-autolink':
      openAutoLink();
      break;
    case 'save-hostname':
      saveHostname();
      break;
    case 'reset-hostname':
      resetHostname();
      break;

    case 'save-schedules':
      saveSchedules();
      break;
    case 'add-schedule':
      addScheduleRow();
      break;
    case 'remove-sched-row':
      removeScheduleRow(+el.dataset.i);
      break;
    case 'sync-time':
      syncTime();
      break;
    case 'save-reboot-time':
      saveRebootTime();
      break;
    case 'restart-device':
      restartDevice();
      break;
    case 'check-online-ota':
      checkOnlineFirmwareUpdate();
      break;
    case 'confirm-online-ota':
      confirmOnlineFirmwareUpdate();
      break;
    case 'dismiss-online-ota':
      dismissOnlineFirmwareUpdate();
      break;
    case 'upload-ota':
    case 'pick-ota-file':
      const inp = $('otaFileInput');
      if (inp) {
        inp.onchange = () => {
          if (inp.files && inp.files.length > 0) {
            uploadOtaFirmware();
          }
        };
        inp.click();
      }
      break;
    case 'enter-safemode':
      if (confirm('คุณแน่ใจหรือไม่ว่าต้องการเข้าสู่ Safe Mode ทันที? (ระบบจะทำการรีสตาร์ทเครื่อง)')) {
        enterSafeMode();
      }
      break;
    case 'exit-safemode':
      exitSafeMode();
      break;
    case 'submit-web-login':
      submitWebLogin();
      break;
    case 'safe-mode-continue':
      dismissSafeModeGate();
      completeBoot();
      break;
    case 'external-link':
      window.open(el.dataset.href, '_blank', 'noopener');
      break;
    case 'go-to-safemode-settings':
      openSetGroup('setGroupSystem');
      openSetGroup('cardSafeMode');
      setupScrollTo($('cardSafeMode'));
      break;
    default:
      break;
  }
}

function onActionChange(e) {
  const el = e.target;
  const action = el.dataset.action;
  if (!action) return;

  switch (action) {
    case 'toggle-ed-sched':
      toggleEdSched();
      break;
    case 'toggle-ed-webhook':
      toggleEdWebhook();
      break;
    case 'toggle-tag-auto':
      toggleTagAuto();
      break;
    case 'toggle-tag-buzzer':
      toggleTagBuzzer();
      break;
    case 'toggle-sim-mode':
      toggleSimMode();
      break;
    case 'toggle-motor-reverse':
      toggleMotorReverse();
      break;
    case 'toggle-double-close':
      toggleDoubleClose();
      break;
    case 'toggle-mag-sensor':
      toggleMagSensor();
      break;
    case 'toggle-auto-retry-close':
      toggleAutoRetryClose();
      break;
    case 'toggle-safety-sensor':
      toggleSafetySensor();
      break;
    case 'toggle-remote-passage-close':
      toggleRemotePassageClose();
      break;
    case 'toggle-fix-ip':
      toggleFixIp();
      break;
    case 'toggle-ap-user':
      toggleApUser();
      break;
    case 'toggle-blynk':
      toggleBlynk();
      break;
    case 'toggle-mqtt':
      toggleMqtt();
      break;
    case 'toggle-auto-reboot':
      toggleAutoReboot();
      break;
    case 'sched-day': {
      const i = +el.dataset.i;
      const d = +el.dataset.d;
      toggleSchedDay(i, d, el.checked);
      break;
    }
    case 'sched-time':
      setSchedTime(+el.dataset.i, el.value);
      break;
    case 'sched-action':
      if (State.scheduleRows[+el.dataset.i]) State.scheduleRows[+el.dataset.i].action = el.value;
      break;
    case 'sched-enabled':
      if (State.scheduleRows[+el.dataset.i]) State.scheduleRows[+el.dataset.i].enabled = el.checked;
      break;
    default:
      break;
  }
}

function onActionInput(e) {
  const el = e.target;
  const action = el.dataset.action;
  if (!action) return;
  if (action === 'update-ed-rssi') updateEdRssiLbl();
  else if (action === 'update-ed-safety-delay') updateEdSafetyDelayWarn();
}

function onActionKeydown(e) {
  if (e.key !== 'Enter') return;
  const el = e.target;
  if (el.id === 'loginPass' || el.dataset.action === 'submit-web-login') {
    e.preventDefault();
    submitWebLogin();
  }
}

function initActions() {
  document.body.addEventListener('click', onActionClick);
  document.body.addEventListener('change', onActionChange);
  document.body.addEventListener('input', onActionInput);
  document.body.addEventListener('keydown', onActionKeydown);
}

async function toggleTagBuzzer() {
  const sw = $('swTagBuzzer');
  if (!sw) return;
  const val = sw.checked;
  const msg = $('tagBuzzerMsg');
  if (msg) msg.textContent = 'กำลังบันทึก...';
  try {
    const res = await api('/api/settings', {
      method: 'PUT',
      body: { tagBuzzerEnabled: val }
    });
    if (res.ok) {
      if (msg) msg.textContent = val ? 'เปิดเสียงเตือน Tag แล้ว' : 'ปิดเสียงเตือน Tag แล้ว';
    }
  } catch (err) {
    if (msg) msg.textContent = 'บันทึกไม่สำเร็จ';
    sw.checked = !val;
  }
}
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

function updateGateVisual(state) {
  const g = $('gateVisual');
  const main = $('dashStatusMain');
  const pulse = $('dashPulse');
  if (!g) return;

  if (State.doorTransitionMs) {
    g.style.setProperty('--gate-travel', (State.doorTransitionMs / 1000) + 's');
  }

  if (g.dataset.doorState === state) return;
  g.dataset.doorState = state;

  g.classList.remove('open', 'closed', 'opening', 'closing', 'stopped', 'moving', 'unknown');
  main?.classList.remove('open', 'closed', 'warn', 'moving', 'stop');
  pulse?.classList.remove('pulse-red', 'pulse-green', 'pulse-amber', 'pulse-blue');

  switch (state) {

    case 'opening':

      g.classList.add('opening', 'moving');

      main?.classList.add('moving', 'open');

      pulse?.classList.add('pulse-blue');

      break;

    case 'closing':

      g.classList.add('closing', 'moving');

      main?.classList.add('moving', 'open');

      pulse?.classList.add('pulse-blue');

      break;

    case 'open':

      g.classList.add('open');

      main?.classList.add('open');

      pulse?.classList.add('pulse-green');

      break;

    case 'stopped':

      g.classList.add('stopped');

      main?.classList.add('warn', 'stop');

      pulse?.classList.add('pulse-amber');

      break;

    case 'closed':

      g.classList.add('closed');

      main?.classList.add('closed');

      pulse?.classList.add('pulse-red');

      break;

    default:

      g.classList.add('closed', 'unknown');

      main?.classList.add('closed');

      break;

  }

}



function applyDoorStateFromServer(doorState) {

  const st = normalizeDoorState(doorState);

  updateGateVisual(st);

  const statusText = $('dashStatusText');

  if (statusText) statusText.textContent = doorStateLabel(st);

}



function setDoorControlsEnabled(enabled) {

  document.querySelectorAll('[data-action="door-send"]').forEach((btn) => {

    btn.disabled = !enabled;

  });

}



function doorPendingVisual(slot) {

  if (slot === 'open') return 'opening';

  if (slot === 'close') return 'closing';

  if (slot === 'stop') return 'stopped';

  return null;

}



async function doorSend(slot) {

  if (!State.appBooted) return;



  const pending = doorPendingVisual(slot);

  if (pending) updateGateVisual(pending);



  const btns = document.querySelectorAll('[data-action="door-send"]');

  btns.forEach((b) => { b.disabled = true; });



  try {

    const d = await api('/api/door', { method: 'POST', body: { slot } });

    if (d.ok === false) throw new Error('door_failed');

    if (d.doorState) applyDoorStateFromServer(d.doorState);

    pollLive();

  } catch (e) {

    try {

      await pollLive();

    } catch (_) {}

    alert('ส่งคำสั่งไม่สำเร็จ');

  } finally {

    if (State.appBooted) setDoorControlsEnabled(true);

  }

}
function applyLiveTagTimerEl(el, timerTxt) {
  if (!el) return;
  if (timerTxt) {
    el.innerHTML = CLOCK_SVG + timerTxt;
    el.classList.remove('hidden');
  } else {
    el.innerHTML = '';
    el.classList.add('hidden');
  }
}

function getTagBadgeInfo(t, isRf, zone, doorCloseMs) {
  const doorSt = State.lastLiveData?.doorState;
  if (doorSt === 'opening' && t.isTriggerTag) return { label: 'กำลังเปิดประตู', cls: '' };
  if (doorSt === 'closing' && t.isTriggerTag) return { label: 'กำลังปิดประตู', cls: 'warn' };

  if (isRf) {
    if (t.active) return { label: 'กำลังส่งสัญญาณ', cls: '' };
    if (doorSt === 'open' && doorCloseMs != null && doorCloseMs > 0 && t.isTriggerTag) {
      return { label: 'รอปิดประตู', cls: 'warn' };
    }
    return { label: 'ส่งสัญญาณแล้ว', cls: 'muted' };
  }

  if (doorSt === 'open' && doorCloseMs != null && doorCloseMs > 0 && !t.active && t.isTriggerTag) {
    return { label: 'รอปิดประตู', cls: 'warn' };
  }

  return {
    label: RSSI_ZONE_LABEL[zone] || zone,
    cls: zoneBadgeClass(zone),
  };
}

function buildLiveTagCardHtml(t, doorCloseMs, doorCloseLabel) {
  const isRf = !!(t.isRf || (t.mac && t.mac.toUpperCase().includes('RF')) || t.tagId);
  const zone = tagZone(t);
  const badgeInfo = getTagBadgeInfo(t, isRf, zone, doorCloseMs);
  const zoneLbl = badgeInfo.label;
  const badgeCls = badgeInfo.cls;
  const rssi = t.lastRssi != null ? t.lastRssi : '—';
  const timerTxt = liveTagTimerText(t, doorCloseMs, doorCloseLabel);
  const timerCls = timerTxt ? 'liveTagTimer' : 'liveTagTimer hidden';
  const rssiHtml = isRf
    ? `<div class="liveTagRssi" style="font-size:15px;letter-spacing:0.5px;">📡 RF433<span class="liveTagRssiUnit" style="margin-left:4px;">รีโมท</span></div>`
    : `<div class="liveTagRssi">${rssi}<span class="liveTagRssiUnit">dBm</span></div>`;
  const barsHtml = isRf
    ? `<div class="liveTagBars"><span class="liveTagBar on" style="height:12px;background:#3ecf8e;"></span><span class="liveTagBar on" style="height:16px;background:#3ecf8e;"></span><span class="liveTagBar on" style="height:20px;background:#3ecf8e;"></span><span class="liveTagBar on" style="height:24px;background:#3ecf8e;"></span><span class="liveTagBar on" style="height:28px;background:#3ecf8e;"></span></div>`
    : `<div class="liveTagBars">${rssiBarHtml(rssi, t.tagRssiMin ?? -100)}</div>`;

  return `<div class="liveTagCard" data-tag-id="${escHtml(t.id)}">
    <div class="liveTagHead">
      <span class="liveTagName">${escHtml(t.name)}</span>
      <span class="liveTagBadge ${badgeCls}"><span class="badgeDot"></span>${zoneLbl}</span>
    </div>
    <div class="liveTagMain">
      ${rssiHtml}
      <div class="${timerCls}">${timerTxt ? CLOCK_SVG + timerTxt : ''}</div>
    </div>
    <div class="liveTagFoot">${barsHtml}</div>
  </div>`;
}

function buildCloseOnlyCardHtml(ms, label) {
  return `<div class="liveTagCard liveTagCloseWarn" data-close-only="1">
    <div class="liveTagHead">
      <span class="liveTagName">รอปิดประตูอัตโนมัติ</span>
      <span class="liveTagBadge warn"><span class="badgeDot"></span>นับถอยหลัง</span>
    </div>
    <div class="liveTagMain liveTagMainCenter">
      <div class="liveTagTimer liveTagTimerLg">${CLOCK_SVG}${label} ${formatHoldClock(ms)}</div>
    </div>
  </div>`;
}

function updateLiveTagCardEl(card, t, doorCloseMs, doorCloseLabel) {
  const isRf = !!(t.isRf || (t.mac && t.mac.toUpperCase().includes('RF')) || t.tagId);
  const zone = tagZone(t);
  const badgeInfo = getTagBadgeInfo(t, isRf, zone, doorCloseMs);
  const zoneLbl = badgeInfo.label;
  const badgeCls = badgeInfo.cls;
  const rssi = t.lastRssi != null ? t.lastRssi : '—';
  const timerTxt = liveTagTimerText(t, doorCloseMs, doorCloseLabel);

  const nameEl = card.querySelector('.liveTagName');
  if (nameEl) nameEl.textContent = t.name;

  const badge = card.querySelector('.liveTagBadge');
  if (badge) {
    badge.className = 'liveTagBadge ' + badgeCls;
    badge.innerHTML = `<span class="badgeDot"></span>${zoneLbl}`;
  }

  const rssiEl = card.querySelector('.liveTagRssi');
  if (rssiEl) {
    if (isRf) {
      rssiEl.innerHTML = `📡 RF433<span class="liveTagRssiUnit" style="margin-left:4px;">รีโมท</span>`;
    } else {
      rssiEl.innerHTML = `${rssi}<span class="liveTagRssiUnit">dBm</span>`;
    }
  }

  applyLiveTagTimerEl(card.querySelector('.liveTagTimer'), timerTxt);

  const bars = card.querySelector('.liveTagBars');
  if (bars && !isRf) {
    const lit = rssiBarLitCount(rssi, t.tagRssiMin ?? -100);
    bars.querySelectorAll('.liveTagBar').forEach((bar, i) => {
      bar.classList.toggle('on', i < lit);
    });
  }
}

function renderLiveTagList(listEl, detected, closeCd, hasCloseCd) {
  const structureKey = hasCloseCd && !detected.length
    ? 'close-only'
    : `tags:${detected.map(liveTagStructureKey).join(';')}`;

  if (structureKey === State.liveTagKeys && listEl.childElementCount > 0) {
    if (hasCloseCd && !detected.length) {
      const timerEl = listEl.querySelector('.liveTagTimer');
      if (timerEl) timerEl.innerHTML = CLOCK_SVG + closeCd.label + ' ' + formatHoldClock(closeCd.ms);
    } else {
      const cards = listEl.querySelectorAll('.liveTagCard[data-tag-id]');
      detected.forEach((t, i) => {
        const card = cards[i];
        if (card) updateLiveTagCardEl(card, t, closeCd.ms, closeCd.label);
      });
    }
    return;
  }

  State.liveTagKeys = structureKey;
  State.liveCloseMode = hasCloseCd && !detected.length;

  if (detected.length) {
    listEl.innerHTML = detected.map((t) => buildLiveTagCardHtml(t, closeCd.ms, closeCd.label)).join('');
  } else if (hasCloseCd) {
    listEl.innerHTML = buildCloseOnlyCardHtml(closeCd.ms, closeCd.label);
  } else {
    listEl.innerHTML = '';
  }
}

function handleEditorLearnPoll(d) {
  if (State.currentView !== 'editor') return;
  if (d.learnLastMac && $('edTagMac')) {
    $('edTagMac').value = d.learnLastMac;
    if (typeof toggleRssiCards === 'function') {
      toggleRssiCards(!d.learnLastMac.toUpperCase().startsWith('RF:'));
    }
  }
  if (!d.learnCreatedTagId) return;
  State.editingId = d.learnCreatedTagId;
  $('edAdmin')?.classList.remove('hidden');
  const scanLbl = $('edScanLabel');
  if (scanLbl) scanLbl.textContent = 'เรียนรู้สำเร็จ — ตั้งชื่อแล้วกดบันทึก';
  ['edDot1', 'edDot2', 'edDot3'].forEach((id) => {
    const el = $(id);
    if (el) {
      el.classList.remove('on');
      el.classList.add('success');
    }
  });
  const msg = $('edMsg');
  if (msg) msg.textContent = '';
}

let gIsPollingLive = false;
async function pollLive() {
  if (gIsPollingLive) return;
  gIsPollingLive = true;
  try {
    const d = await api('/api/live', { timeoutMs: 3000 });
    if (d.doorTransitionMs) {
      State.doorTransitionMs = d.doorTransitionMs;
    }
    const st = normalizeDoorState(d.doorState);
    const statusText = $('dashStatusText');
    if (statusText) statusText.textContent = doorStateLabel(st);
    updateGateVisual(st);

    const magWrap = $('dashMagWrap');
    const magEl = $('dashMagState');
    if (d.magSensorEnabled === false) {
      if (magWrap) magWrap.style.display = 'none';
    } else {
      if (magWrap) magWrap.style.display = 'block';
      if (magEl) {
        const closed = !!d.magSensorClosed;
        if (d.closeRetryCount && d.closeRetryCount > 0) {
          magEl.textContent = '🔁 ปิดไม่สนิท (กำลังลองซ้ำรอบที่ ' + d.closeRetryCount + ')';
          magEl.style.color = '#f59e0b';
        } else {
          magEl.textContent = closed ? '🔒 ปิดสนิท' : '🔓 เปิดอยู่';
          magEl.style.color = closed ? '#3ecf8e' : '#5b9cff';
        }
      }
    }

    const safetyWrap = $('dashSafetyWrap');
    const safetyEl = $('dashSafetyState');
    if (d.safetySensorEnabled === false) {
      if (safetyWrap) safetyWrap.style.display = 'none';
    } else {
      if (safetyWrap) safetyWrap.style.display = 'block';
      if (safetyEl) {
        const blocked = !!d.safetySensorBlocked;
        safetyEl.textContent = blocked ? '⚠️ มีสิ่งกีดขวาง' : '✅ ปกติ';
        safetyEl.style.color = blocked ? '#ef4444' : '#3ecf8e';
      }
    }

    State.lastLiveData = d;
    State.lastLiveRecvMs = Date.now();

    const tags = d.tags || [];
    const detected = tags.filter(isTagDetected);
    const closeCd = pickDoorCloseCountdown(tags, d.pendingCloseMs, d.safetyCloseRemainingMs);
    const hasCloseCd = closeCd.ms != null && closeCd.ms > 0;

    const section = $('dashTagSection');
    if (section) section.classList.toggle('hidden', !detected.length && !hasCloseCd);

    const listEl = $('dashTagList');
    if (listEl) renderLiveTagList(listEl, detected, closeCd, hasCloseCd);

    const banner = $('connBanner');
    if (banner) banner.classList.add('hidden');

    if (d.recentEvents) {
      renderLiveEventLog(d.recentEvents);
    }

    if (typeof d.tagCount === 'number') {
      State.tagCount = d.tagCount;
    } else if (typeof d.tagTotalCount === 'number') {
      State.tagCount = d.tagTotalCount;
    }

    if (State.currentView === 'devices') updateDeviceListFromLive(tags);
    if (State.currentView === 'settings') refreshSettingsFromLive();
    if (d.safeMode != null) updateSafeModeBanner(!!d.safeMode);
    if (d.safetySensorEnabled != null) {
      State.safetySensorEnabled = !!d.safetySensorEnabled;
    }
    handleEditorLearnPoll(d);
  } catch (e) {
    $('connBanner')?.classList.remove('hidden');
  } finally {
    gIsPollingLive = false;
  }
}

function formatEventItemHtml(ev, idx, isHistory = false) {
  // Check if tag exists in active tags registry to get user's configured icon
  let configuredIcon = '';
  const tags = State.lastLiveData?.tags || [];
  const tagMatch = tags.find(t => t.name === ev.msg || t.id === ev.entity || t.name === ev.entity);
  if (tagMatch && tagMatch.icon && tagMatch.icon !== 'auto') {
    configuredIcon = tagMatch.icon.toLowerCase();
  }

  const type = (configuredIcon || ev.type || '').toLowerCase();
  const subText = (ev.reason || '').toLowerCase();
  let themeClass = 'access-theme-other';
  let emoji = '🏷️';

  if (type === 'rf' || type.includes('remote') || type.includes('key') || subText.includes('รีโมท')) {
    themeClass = 'access-theme-rf';
    emoji = '🔑';
  } else if (type === 'person' || subText.includes('คนเดิน') || subText.includes('บุคคล')) {
    themeClass = 'access-theme-person';
    emoji = '🚶';
  } else if (type === 'suv' || subText.includes('suv')) {
    themeClass = 'access-theme-car';
    emoji = '🚙';
  } else if (type === 'sedan' || subText.includes('เก๋ง')) {
    themeClass = 'access-theme-car';
    emoji = '🚗';
  } else if (type === 'pickup' || type.includes('truck') || subText.includes('กระบะ')) {
    themeClass = 'access-theme-car';
    emoji = '🚚';
  } else if (type === 'bike' || subText.includes('มอเตอร์ไซค์') || subText.includes('มอไซ') || subText.includes('บิ๊กไบค์')) {
    themeClass = 'access-theme-bike';
    emoji = '🏍️';
  } else if (type === 'scooter' || subText.includes('สกู๊ตเตอร์')) {
    themeClass = 'access-theme-bike';
    emoji = '🛵';
  } else if (type === 'van' || subText.includes('รถตู้')) {
    themeClass = 'access-theme-car';
    emoji = '🚐';
  } else if (type === 'bicycle' || subText.includes('จักรยาน')) {
    themeClass = 'access-theme-bike';
    emoji = '🚲';
  } else if (type === 'web' || subText.includes('เว็บ')) {
    themeClass = 'access-theme-web';
    emoji = '💻';
  } else if (type === 'app' || subText.includes('แอป')) {
    themeClass = 'access-theme-app';
    emoji = '📱';
  } else if (type === 'btn' || subText.includes('สวิตช์') || subText.includes('ปุ่ม')) {
    themeClass = 'access-theme-other';
    emoji = '🔘';
  } else if (type === 'ext' || subText.includes('ภายนอก') || subText.includes('เซ็นเซอร์')) {
    themeClass = 'access-theme-other';
    emoji = '⚡';
  } else {
    const msgLower = (ev.msg || '').toLowerCase();
    if (msgLower.includes('กุญแจ') || msgLower.includes('รีโมท') || msgLower.includes('remote') || msgLower.includes('rf')) {
      themeClass = 'access-theme-rf';
      emoji = '🔑';
    } else if (msgLower.includes('มอไซ') || msgLower.includes('bike') || msgLower.includes('wave') || msgLower.includes('pcx')) {
      themeClass = 'access-theme-bike';
      emoji = '🏍️';
    } else if (msgLower.includes('คน') || msgLower.includes('เดิน')) {
      themeClass = 'access-theme-person';
      emoji = '🚶';
    } else {
      themeClass = 'access-theme-car';
      emoji = '🚗';
    }
  }

  const title = (ev.msg || ev.entity || 'ไม่ทราบอุปกรณ์').replace(/([^\s()]+)\s*\([0-9a-fA-F:_-]+\)/g, '$1');
  const sub = ev.reason || (themeClass === 'access-theme-rf' ? 'รีโมท RF' : themeClass === 'access-theme-bike' ? 'รถมอเตอร์ไซค์' : themeClass === 'access-theme-person' ? 'คนเดิน' : 'รถยนต์');
  const timeStr = ev.time || '--:--:--';

  return `<div class="access-log-card ${themeClass}">
    <div class="access-log-left">
      <div class="access-log-icon">
        <span class="access-emoji">${emoji}</span>
      </div>
      <div class="access-log-info">
        <div class="access-log-title">${escHtml(title)}</div>
        <div class="access-log-sub">${escHtml(sub)}</div>
      </div>
    </div>
    <div class="access-log-time">${escHtml(timeStr)}</div>
  </div>`;
}

function getEventDeviceName(ev) {
  if (!ev) return 'ไม่ระบุ';
  const raw = ev.msg || ev.entity || 'ไม่ทราบอุปกรณ์';
  return typeof cleanEntityName === 'function' ? cleanEntityName(raw) : raw.replace(/([^\s()]+)\s*\([0-9a-fA-F:_-]+\)/g, '$1').trim();
}

function getEventDeviceEmoji(ev) {
  if (!ev) return '🚗';
  let configuredIcon = '';
  const tags = State.lastLiveData?.tags || [];
  const tagMatch = tags.find(t => t.name === ev.msg || t.id === ev.entity || t.name === ev.entity);
  if (tagMatch && tagMatch.icon && tagMatch.icon !== 'auto') {
    configuredIcon = tagMatch.icon.toLowerCase();
  }

  const type = (configuredIcon || ev.type || '').toLowerCase();
  const subText = (ev.reason || '').toLowerCase();
  if (type === 'rf' || subText.includes('รีโมท')) return '🔑';
  if (type === 'person' || subText.includes('คนเดิน')) return '🚶';
  if (type === 'suv' || subText.includes('suv')) return '🚙';
  if (type === 'sedan' || subText.includes('เก๋ง')) return '🚗';
  if (type === 'pickup' || subText.includes('กระบะ') || type.includes('truck')) return '🚚';
  if (type === 'bike' || subText.includes('มอไซ') || subText.includes('มอเตอร์ไซค์')) return '🏍️';
  if (type === 'scooter' || subText.includes('สกู๊ตเตอร์')) return '🛵';
  if (type === 'van' || subText.includes('รถตู้')) return '🚐';
  if (type === 'bicycle' || subText.includes('จักรยาน')) return '🚲';
  if (type === 'web' || subText.includes('เว็บ')) return '💻';
  if (type === 'app' || subText.includes('แอป')) return '📱';
  if (type === 'btn' || subText.includes('สวิตช์')) return '🔘';
  if (type === 'ext' || subText.includes('เซ็นเซอร์')) return '⚡';
  if (typeof getVehicleEmoji === 'function') return getVehicleEmoji(type, false);
  return '🚗';
}

function setDashEventFilter(filterName) {
  State.dashEventFilter = filterName || 'all';
  applyDashEventFilter();
}

function applyDashEventFilter() {
  const listEl = $('dashEventList');
  const countEl = $('dashEventCount');
  const pillsEl = $('dashEventFilterPills');
  if (!listEl) return;

  const validEvents = State.lastDashValidEvents || [];
  if (!validEvents.length) {
    listEl.innerHTML = '<div style="color:var(--muted); font-size:12px; text-align:center; padding:16px;">ยังไม่มีประวัติเปิดประตู</div>';
    if (countEl) countEl.textContent = '0 รายการล่าสุด';
    if (pillsEl) pillsEl.classList.add('hidden');
    return;
  }

  const currentFilter = State.dashEventFilter || 'all';

  // Highlight active pill
  if (pillsEl) {
    const pills = pillsEl.querySelectorAll('.dash-filter-pill');
    pills.forEach(p => {
      const f = p.getAttribute('data-filter');
      if (f === currentFilter) {
        p.classList.add('active');
      } else {
        p.classList.remove('active');
      }
    });
  }

  // Filter events
  const filtered = (currentFilter === 'all')
    ? validEvents
    : validEvents.filter(ev => getEventDeviceName(ev) === currentFilter);

  // Update count badge
  if (countEl) {
    if (currentFilter === 'all') {
      countEl.textContent = `${validEvents.length} รายการล่าสุด`;
    } else {
      countEl.textContent = `${filtered.length}/${validEvents.length} รายการ (${currentFilter})`;
    }
  }

  // Render list
  if (!filtered.length) {
    listEl.innerHTML = `<div style="color:var(--muted); font-size:12px; text-align:center; padding:16px;">ไม่พบรายการเข้าประตูของ "${escHtml(currentFilter)}"</div>`;
  } else {
    listEl.innerHTML = filtered.map((ev, idx) => formatEventItemHtml(ev, idx, false)).join('');
  }
}

function renderLiveEventLog(events) {
  const listEl = $('dashEventList');
  const countEl = $('dashEventCount');
  const pillsEl = $('dashEventFilterPills');
  if (!listEl) return;

  const validEvents = (events || []).filter(ev => {
    if (!ev || typeof ev !== 'object') return false;
    if (ev.msg && ev.msg.startsWith('{"ok":')) return false;
    if (ev.type === 'MANUAL_OPEN' && (ev.msg || '').includes('ไม่ทราบ')) return false;
    return true;
  });

  State.lastDashValidEvents = validEvents;

  if (!validEvents.length) {
    State._lastEventsSig = '';
    State.dashEventFilter = 'all';
    listEl.innerHTML = '<div style="color:var(--muted); font-size:12px; text-align:center; padding:16px;">ยังไม่มีประวัติเปิดประตู</div>';
    if (countEl) countEl.textContent = '0 รายการล่าสุด';
    if (pillsEl) {
      pillsEl.innerHTML = '';
      pillsEl.classList.add('hidden');
    }
    return;
  }

  // Group devices and count
  const deviceCounts = {};
  const deviceEmojis = {};
  validEvents.forEach(ev => {
    const name = getEventDeviceName(ev);
    deviceCounts[name] = (deviceCounts[name] || 0) + 1;
    if (!deviceEmojis[name]) {
      deviceEmojis[name] = getEventDeviceEmoji(ev);
    }
  });

  const deviceNames = Object.keys(deviceCounts);

  // If current filter no longer exists in events, reset to 'all'
  if (State.dashEventFilter && State.dashEventFilter !== 'all' && !deviceCounts[State.dashEventFilter]) {
    State.dashEventFilter = 'all';
  }

  // Render Pills
  if (pillsEl) {
    pillsEl.classList.remove('hidden');
    let pillsHtml = `<button type="button" class="dash-filter-pill ${State.dashEventFilter === 'all' ? 'active' : ''}" data-action="dash-filter-events" data-filter="all" style="width:auto; min-width:0; flex:0 0 auto;"><span class="pill-emoji">👥</span><span class="pill-name">ทั้งหมด</span><span class="pill-count">${validEvents.length}</span></button>`;

    deviceNames.sort().forEach(name => {
      const emoji = deviceEmojis[name] || '🚗';
      const count = deviceCounts[name];
      const isActive = (State.dashEventFilter === name);
      pillsHtml += `<button type="button" class="dash-filter-pill ${isActive ? 'active' : ''}" data-action="dash-filter-events" data-filter="${escHtml(name)}" style="width:auto; min-width:0; flex:0 0 auto;"><span class="pill-emoji">${emoji}</span><span class="pill-name">${escHtml(name)}</span><span class="pill-count">${count}</span></button>`;
    });

    const pillsSig = `${State.dashEventFilter}:${validEvents.length}:${deviceNames.sort().map(n => n + '=' + deviceCounts[n]).join(',')}`;
    if (pillsEl._lastPillsSig !== pillsSig) {
      pillsEl._lastPillsSig = pillsSig;
      pillsEl.innerHTML = pillsHtml;
    }

    if (!pillsEl._boundClick) {
      pillsEl._boundClick = true;
      pillsEl.addEventListener('click', (e) => {
        const btn = e.target.closest('.dash-filter-pill');
        if (btn && btn.dataset.filter !== undefined) {
          setDashEventFilter(btn.dataset.filter);
        }
      });
    }
  }

  applyDashEventFilter();
}

async function copyEventLogs() {
  const allEvents = State.lastDashValidEvents || State.lastLiveData?.recentEvents || [];
  const currentFilter = State.dashEventFilter || 'all';
  const events = (currentFilter === 'all')
    ? allEvents
    : allEvents.filter(ev => getEventDeviceName(ev) === currentFilter);

  if (!events.length) {
    const btn = $('dashCopyEventsBtn');
    if (btn) {
      btn.style.borderColor = 'rgba(239,68,68,0.5)';
      setTimeout(() => { btn.style.borderColor = ''; }, 1500);
    }
    return;
  }

  const filterNote = currentFilter !== 'all' ? ` [ฟิลเตอร์: ${currentFilter}]` : '';
  const lines = [
    `=== บันทึกเปิดประตู AutoDoor-RF2 (Live)${filterNote} ===`,
    ...events.map(ev => {
      const time = ev.time || '';
      const name = getEventDeviceName(ev);
      const sub = ev.reason || '';
      return `[${time}] ${name} (${sub})`;
    }),
  ];
  const text = lines.join('\n');

  try {
    if (navigator.clipboard?.writeText) {
      await navigator.clipboard.writeText(text);
    } else {
      const ta = document.createElement('textarea');
      ta.value = text;
      ta.style.position = 'fixed';
      ta.style.left = '-9999px';
      document.body.appendChild(ta);
      ta.select();
      document.execCommand('copy');
      document.body.removeChild(ta);
    }
    const btn = $('dashCopyEventsBtn');
    if (btn) {
      btn.style.borderColor = 'rgba(16,185,129,0.7)';
      btn.style.color = '#34d399';
      setTimeout(() => {
        btn.style.borderColor = '';
        btn.style.color = '';
      }, 1500);
    }
  } catch (e) {
    console.error('Copy failed', e);
  }
}

async function clearEventLogs() {
  try {
    const res = await api('/api/events/clear', { method: 'POST' });
    if (res?.ok) {
      State._lastEventsSig = '';
      if (State.expandedEvents) State.expandedEvents.clear();
      if (State.lastLiveData) {
        State.lastLiveData.recentEvents = [];
      }
      renderLiveEventLog([]);
    }
  } catch (e) {
    console.error('Clear events error:', e);
  }
}

async function loadHistoryDates() {
  const sel = $('historyDateSelect');
  if (!sel) return;

  if (!sel._boundChange) {
    sel._boundChange = true;
    sel.addEventListener('change', (e) => {
      const val = e.target.value;
      State.selectedHistoryDate = val || null;
      if (val) {
        loadHistoryEvents(val);
      } else {
        const list = $('historyEventList');
        if (list) list.innerHTML = '<div style="color:var(--muted); font-size:12px; text-align:center; padding:16px;">กรุณาเลือกวันที่เพื่อดูข้อมูลย้อนหลัง</div>';
      }
    });
  }

  try {
    const res = await api('/api/history/dates');
    if (res?.ok && Array.isArray(res.dates)) {
      State.historyDates = res.dates;
      const countBadge = $('historyHeadCount');
      if (countBadge) countBadge.textContent = `${res.dates.length} วัน`;

      const fsUsage = $('historyFsUsage');
      if (fsUsage) fsUsage.textContent = `${res.fsUsedKb || 0} KB / ${res.fsTotalKb || 192} KB`;

      const prevVal = sel.value;
      let html = '<option value="">-- กรุณาเลือกวันที่ --</option>';
      html += '<option value="live">🔴 ข้อมูลสดปัจจุบัน (RAM)</option>';
      for (const d of res.dates) {
        const countTxt = d.eventCount != null ? ` (${d.eventCount} รายการ)` : '';
        html += `<option value="${escHtml(d.date)}">📁 ${escHtml(d.formatted || d.date)}${countTxt}</option>`;
      }
      sel.innerHTML = html;

      if (prevVal && Array.from(sel.options).some(o => o.value === prevVal)) {
        sel.value = prevVal;
        State.selectedHistoryDate = prevVal;
        loadHistoryEvents(prevVal);
      } else if (res.dates.length > 0) {
        sel.value = res.dates[0].date;
        State.selectedHistoryDate = res.dates[0].date;
        loadHistoryEvents(res.dates[0].date);
      }
    }
  } catch (e) {
    console.error('Load history dates error:', e);
  }
}

async function loadHistoryEvents(dateStr) {
  const listEl = $('historyEventList');
  const cycleEl = $('historyCycleContainer');
  if (cycleEl) {
    cycleEl.innerHTML = '<div style="color:var(--muted); font-size:12px; text-align:center; padding:16px;">กำลังโหลดข้อมูล...</div>';
  }
  try {
    let res = null;
    if (dateStr === 'live') {
      res = await api('/api/events?limit=400', { timeoutMs: 15000 });
    } else {
      res = await api(`/api/history?date=${encodeURIComponent(dateStr)}`, { timeoutMs: 15000 });
    }
    if (res?.ok && Array.isArray(res.events)) {
      State.historyEvents = res.events;
      if (typeof renderHistoryAnalytics === 'function') {
        renderHistoryAnalytics(res.events);
      } else {
        renderHistoryEventLog(res.events);
      }
    } else {
      State.historyEvents = [];
      if (cycleEl) cycleEl.innerHTML = '<div style="color:var(--muted); font-size:12px; text-align:center; padding:16px;">ไม่พบประวัติของวันที่เลือก</div>';
      if (listEl) listEl.innerHTML = '<div style="color:var(--muted); font-size:12px; text-align:center; padding:16px;">ไม่พบประวัติของวันที่เลือก</div>';
    }
  } catch (e) {
    console.error('Load history events error:', e);
    if (cycleEl) cycleEl.innerHTML = '<div style="color:#ef4444; font-size:12px; text-align:center; padding:16px;">โหลดข้อมูลไม่สำเร็จ</div>';
  }
}

async function saveEventLogsToFlash() {
  const textEl = $('historySaveText');
  const msgEl = $('historyMsg');
  try {
    if (textEl) textEl.textContent = 'กำลังบันทึก...';
    const res = await api('/api/history/save', { method: 'POST' });
    if (res?.ok) {
      if (textEl) {
        textEl.textContent = '✓ บันทึกแล้ว';
        setTimeout(() => { textEl.textContent = 'บันทึกทันที'; }, 2000);
      }
      if (msgEl) {
        msgEl.textContent = '✓ บันทึกเหตุการณ์ปัจจุบันลง Flash เรียบร้อยแล้ว';
        msgEl.className = 'hint ok';
        setTimeout(() => { msgEl.textContent = ''; }, 3000);
      }
      await loadHistoryDates();
    } else {
      if (textEl) {
        textEl.textContent = '✗ ผิดพลาด';
        setTimeout(() => { textEl.textContent = 'บันทึกทันที'; }, 2000);
      }
    }
  } catch (e) {
    console.error('Save events error:', e);
    if (textEl) textEl.textContent = 'บันทึกทันที';
  }
}

function exportHistoryCsv() {
  const btn = $('historyExportText');
  if (btn) btn.textContent = 'กำลังโหลด...';
  try {
    const link = document.createElement('a');
    link.href = '/api/history/export';
    link.download = 'autodoor_history_180days.csv';
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    setTimeout(() => {
      if (btn) btn.textContent = 'ส่งออก CSV';
    }, 2000);
  } catch (e) {
    console.error('Export CSV error:', e);
    if (btn) btn.textContent = 'ส่งออก CSV';
  }
}

async function copyHistoryEvents() {
  const events = State.historyEvents || [];
  const dateStr = State.selectedHistoryDate || '';
  if (!events.length) {
    const btn = $('historyCopyText');
    if (btn) {
      const orig = btn.textContent;
      btn.textContent = 'ไม่มีข้อมูล';
      setTimeout(() => { btn.textContent = orig; }, 1500);
    }
    return;
  }

  const lines = [
    `=== บันทึกเหตุการณ์ย้อนหลัง AutoDoor-RF2 (วันที่ ${dateStr}) ===`,
    ...events.map(ev => {
      const time = (ev.time || '').padEnd(8, ' ');
      const code = ('[' + (ev.type || 'INFO') + ']').padEnd(14, ' ');
      let tele = '';
      if (ev.durationMs && ev.durationMs > 0) tele += ` | ⏱ ${(ev.durationMs / 1000).toFixed(1)}s`;
      if (typeof ev.metric === 'number' && ev.metric < 0) tele += ` | 📶 ${ev.metric}dBm`;
      if (ev.sensorBits & 1) tele += ` | 🛑 ลำแสงตัด`;
      if (ev.sensorBits & 2) tele += ` | 🧲 แม่เหล็กติด`;
      if (ev.reason) tele += ` | RC:${ev.reason}`;
      if (ev.entity) tele += ` | Src:${ev.entity}`;
      return `[${time}] ${code} ${ev.msg || ''}${tele}`;
    }),
  ];
  const text = lines.join('\n');

  try {
    if (navigator.clipboard?.writeText) {
      await navigator.clipboard.writeText(text);
    } else {
      const ta = document.createElement('textarea');
      ta.value = text;
      ta.style.position = 'fixed';
      ta.style.left = '-9999px';
      document.body.appendChild(ta);
      ta.select();
      document.execCommand('copy');
      document.body.removeChild(ta);
    }
    const btn = $('historyCopyText');
    if (btn) {
      btn.textContent = '✓ คัดลอกแล้ว';
      setTimeout(() => { btn.textContent = 'คัดลอก'; }, 2000);
    }
  } catch (e) {
    const btn = $('historyCopyText');
    if (btn) {
      btn.textContent = '✗ ไม่สำเร็จ';
      setTimeout(() => { btn.textContent = 'คัดลอก'; }, 2000);
    }
  }
}

async function clearHistoryFlash() {
  if (!confirm('ต้องการล้างประวัติไฟล์ย้อนหลังทั้งหมดใน Flash ใช่หรือไม่?')) return;
  const msgEl = $('historyMsg');
  try {
    const res = await api('/api/history/clear', { method: 'POST' });
    if (res?.ok) {
      State.historyEvents = [];
      State.selectedHistoryDate = null;
      const list = $('historyEventList');
      if (list) list.innerHTML = '<div style="color:var(--muted); font-size:12px; text-align:center; padding:16px;">ล้างประวัติ Flash เรียบร้อยแล้ว</div>';
      if (msgEl) {
        msgEl.textContent = '✓ ล้างไฟล์ประวัติใน Flash ทั้งหมดแล้ว';
        msgEl.className = 'hint ok';
        setTimeout(() => { msgEl.textContent = ''; }, 3000);
      }
      await loadHistoryDates();
    }
  } catch (e) {
    console.error('Clear history error:', e);
  }
}

function tickLiveCountdown() {
  if (!State.lastLiveData || !State.lastLiveRecvMs) return;
  const elapsed = Date.now() - State.lastLiveRecvMs;
  if (elapsed <= 0) return;

  const d = State.lastLiveData;
  const tags = (d.tags || []).map((t) => {
    const isRf = !!(t.isRf || (t.mac && t.mac.toUpperCase().includes('RF')) || t.tagId);
    const origHold = typeof t.holdRemainingMs === 'number' && t.holdRemainingMs > 0
      ? t.holdRemainingMs
      : (isRf ? 1000 : (t.tagHoldMs || 2000));
    const remHold = Math.max(0, origHold - elapsed);
    const origPass = t.passCloseMs != null ? t.passCloseMs : null;
    const remPass = origPass != null ? Math.max(0, origPass - elapsed) : null;
    return {
      ...t,
      holdRemainingMs: remHold,
      passCloseMs: remPass,
      active: remHold > 0 && !!t.active,
    };
  });

  const pendingCloseMs = d.pendingCloseMs != null ? Math.max(0, d.pendingCloseMs - elapsed) : null;
  const safetyCloseRemainingMs = d.safetyCloseRemainingMs != null ? Math.max(0, d.safetyCloseRemainingMs - elapsed) : null;

  const detected = tags.filter(isTagDetected);
  const closeCd = pickDoorCloseCountdown(tags, pendingCloseMs, safetyCloseRemainingMs);
  const hasCloseCd = closeCd.ms != null && closeCd.ms > 0;

  const section = $('dashTagSection');
  if (section) section.classList.toggle('hidden', !detected.length && !hasCloseCd);

  const listEl = $('dashTagList');
  if (listEl && listEl.childElementCount > 0) {
    renderLiveTagList(listEl, detected, closeCd, hasCloseCd);
  }

  if (State.currentView === 'devices') updateDeviceListFromLive(tags);
}

function startLivePoll() {
  pollLive();
  if (State.liveTimer) clearInterval(State.liveTimer);
  State.liveTimer = setInterval(pollLive, 1500);

  if (State.liveTickTimer) clearInterval(State.liveTickTimer);
  State.liveTickTimer = setInterval(tickLiveCountdown, 500);
}

window.setDashEventFilter = setDashEventFilter;
async function refreshTagList() {
  const d = await api('/api/tags');
  const tags = d.tags || [];
  State.tagCount = tags.length;
  renderTagList(tags);
}

function deviceListKey(tags) {
  return tags.map((t) => `${t.id}:${t.name}:${t.icon || ''}:${t.active ? 1 : 0}:${t.tagOpenDurationMs}:${t.tagCloseDelayMs}:${t.tagSafetyCloseDelayMs}:${t.tagHoldMs}:${t.rfRole || 0}:${t.scheduleEnabled ? 1 : 0}:${t.scheduleStartMin}:${t.scheduleEndMin}:${t.webhookEnabled === false ? 0 : 1}:${t.webhookOpen === false ? 0 : 1}:${t.webhookClosing === false ? 0 : 1}:${t.webhookClose === false ? 0 : 1}:${t.webhookUrl || ''}`).join('|');
}

function getTagParamText(t) {
  const isRf = !!(t.isRf || (t.mac && t.mac.toUpperCase().includes('RF')) || t.tagId);
  const holdSec = isRf ? 1 : Math.max(1, Math.round((t.tagHoldMs || TAG_DEF.holdSec * 1000) / 1000));
  const openSec = Math.round((t.tagOpenDurationMs || 0) / 1000);
  const closeDelaySec = Math.round((t.tagCloseDelayMs ?? TAG_DEF.closeDelaySec * 1000) / 1000);
  const safetyCloseSec = Math.round((t.tagSafetyCloseDelayMs ?? TAG_DEF.safetyCloseDelaySec * 1000) / 1000);

  const formatMin = (m) => {
    const h = String(Math.floor(m / 60)).padStart(2, '0');
    const min = String(m % 60).padStart(2, '0');
    return `${h}:${min}`;
  };
  const schedBadge = t.scheduleEnabled
    ? ` · <span style="color:#38bdf8;font-weight:500;" title="จำกัดเวลาใช้งาน">🕒 ${formatMin(t.scheduleStartMin || 0)}-${formatMin(t.scheduleEndMin != null ? t.scheduleEndMin : 1439)}</span>`
    : '';

  let whBadge = '';
  if (t.webhookEnabled === false || (t.webhookOpen === false && t.webhookClosing === false && t.webhookClose === false)) {
    whBadge = ` · <span style="color:#64748b;" title="ปิดแจ้งเตือน">🔕 ไม่แจ้งเตือน</span>`;
  } else if (t.webhookUrl) {
    const isLine = t.webhookUrl.startsWith('U') || t.webhookUrl.startsWith('C') || t.webhookUrl.startsWith('R') || t.webhookUrl.startsWith('line:');
    const badgeIcon = isLine ? '💬' : '🔔';
    const evs = [];
    if (t.webhookOpen !== false) evs.push('เปิด');
    if (t.webhookClosing !== false) evs.push('กำลังปิด');
    if (t.webhookClose !== false) evs.push('ปิด');
    const evText = evs.join('+');
    whBadge = ` · <span style="color:${isLine ? '#4ade80' : '#a78bfa'};font-weight:500;" title="${isLine ? 'LINE ID' : 'Webhook'}: ${escHtml(t.webhookUrl)}">${badgeIcon} ${escHtml(t.webhookUrl)} (${evText})</span>`;
  }

  if (t.active) {
    const remMs = (typeof t.holdRemainingMs === 'number' && t.holdRemainingMs > 0)
      ? t.holdRemainingMs
      : (isRf ? 1000 : (t.tagHoldMs || 2000));
    const remSec = Math.max(0.1, (remMs / 1000)).toFixed(1);
    return isRf
      ? `<span style="color:#10b981;font-weight:600;">🟢 ส่งสัญญาณ (ทำงานอีก ${remSec} วิ)</span>${schedBadge}${whBadge}`
      : `<span style="color:#10b981;font-weight:600;">🟢 กำลังทำงาน (จะหายใน ${remSec} วิ)</span>${schedBadge}${whBadge}`;
  }

  const openPrefix = openSec > 0 ? `เปิดช่อง ${openSec} วิ · รอ ${closeDelaySec} วิ` : `หน่วงปิด ${closeDelaySec} วิ`;
  if (isRf) {
    if (t.rfRole === 1) return `<span style="color:#fbbf24;font-weight:500;">🟡 หน้าที่: เปิดค้าง (Hold Open)</span>${schedBadge}${whBadge}`;
    if (t.rfRole === 2) return `<span style="color:#f87171;font-weight:500;">🔴 หน้าที่: สั่งปิดประตูทันที</span>${schedBadge}${whBadge}`;
    if (t.rfRole === 3) return `<span style="color:#9ca3af;font-weight:500;">⏹️ หน้าที่: สั่งหยุดมอเตอร์</span>${schedBadge}${whBadge}`;
    if (t.rfRole === 4) return `${openPrefix} · กันหนีบ ${safetyCloseSec} วิ · <span style="color:#10b981;font-weight:500;">🟢 เปิดอย่างเดียว (Auto/Open)</span>${schedBadge}${whBadge}`;
    return `${openPrefix} · กันหนีบ ${safetyCloseSec} วิ · <span style="color:#34d399;">🟢 เปิดออโต้ (Auto/Toggle)</span>${schedBadge}${whBadge}`;
  }
  return `${openPrefix} · กันหนีบ ${safetyCloseSec} วิ · หายใน ${holdSec} วิ${schedBadge}${whBadge}`;
}

function buildDeviceCardHtml(t) {
  const id = escHtml(t.id);
  const isRf = !!(t.isRf || (t.mac && t.mac.toUpperCase().includes('RF')) || t.tagId);
  const emoji = getVehicleEmoji(t.icon, isRf);
  const rfRole = t.rfRole || 0;
  const roleNames = { 0: 'Auto', 1: 'Hold Open', 2: 'Close', 3: 'Stop', 4: 'Auto/Open' };
  const subLabel = isRf ? `RF433 · ${roleNames[rfRole] || 'Auto'}` : 'TAG';
  const paramText = getTagParamText(t);
  return `<div class="devCard devTag" data-tag-id="${id}">
      <div class="devCardBody">
        <div class="devIconBox"><span style="font-size:24px; line-height:1; filter:drop-shadow(0 2px 4px rgba(0,0,0,0.35));">${emoji}</span></div>
        <div class="devCardInfo">
          <div class="devCardMeta">
            <span class="devCardSub">${subLabel}</span>
            <span class="devStatusDot ${t.active ? '' : 'off'}" title="${t.active ? (isRf ? 'กำลังส่งสัญญาณ' : 'กำลังทำงาน') : 'ไม่ได้ส่งสัญญาณ'}"></span>
          </div>
          <p class="devCardTitle">${escHtml(t.name)}</p>
          <p class="devCardId">ID: ${escHtml(tagDisplayId(t))}</p>
          <p class="devCardParam">${paramText}</p>
        </div>
      </div>
      <div class="devCardFoot">
        <button type="button" class="btnDel" data-action="delete-tag" data-id="${id}">${ICO_TRASH}ลบ</button>
        <button type="button" class="btnEdit" data-action="open-editor" data-id="${id}">${ICO_EDIT}แก้ไข</button>
      </div>
    </div>`;
}

function renderTagList(tags) {
  const el = $('deviceList');
  if (!el) return;
  const key = deviceListKey(tags);
  if (key === State.deviceListKey && el.querySelector('.tagGrid')) {
    updateDeviceListActive(tags);
    return;
  }
  State.deviceListKey = key;
  State.deviceActiveMap = Object.fromEntries(tags.map((t) => [t.id, !!t.active]));
  if (!tags.length) {
    el.innerHTML = '<div class="tagGrid"><p class="devEmpty">ยังไม่มี Tag — กดเพิ่ม Tag</p></div>';
    return;
  }
  el.innerHTML = '<div class="tagGrid">' + tags.map(buildDeviceCardHtml).join('') + '</div>';
}

function updateDeviceListActive(tags) {
  for (const t of tags) {
    const active = !!t.active;
    State.deviceActiveMap[t.id] = active;
    let card = null;
    const list = $('deviceList');
    if (list) {
      for (const c of list.querySelectorAll('[data-tag-id]')) {
        if (c.dataset.tagId === t.id) {
          card = c;
          break;
        }
      }
    }
    if (!card) continue;

    const dot = card.querySelector('.devStatusDot');
    if (dot) {
      dot.classList.toggle('off', !active);
      dot.title = active ? 'กำลังทำงาน' : 'หาย';
    }

    const paramEl = card.querySelector('.devCardParam');
    if (paramEl) {
      paramEl.innerHTML = getTagParamText(t);
    }
  }
}

function updateDeviceListFromLive(tags) {
  const el = $('deviceList');
  if (!el || !el.querySelector('.tagGrid .devCard')) {
    refreshTagList();
    return;
  }
  updateDeviceListActive(tags);
}

async function deleteTagFromList(id, ev) {
  ev?.stopPropagation();
  ev?.preventDefault();
  if (!confirm('ลบ Tag นี้?')) return;
  try {
    await api('/api/tags/' + id, { method: 'DELETE' });
    State.deviceListKey = '';
    await refreshTagList();
  } catch (e) {
    setHint('devMsg', 'hint err', 'ลบ Tag ไม่สำเร็จ');
  }
}
function toggleRssiCards(show) {
  const minCard = $('edRssiMinCard');
  const holdWrap = $('edTagHoldWrap');
  const titleHold = $('edSecTitleHold');
  const closeHint = $('edCloseDelayHint');
  const rfRoleSec = $('edRfRoleSec');

  if (holdWrap) holdWrap.classList.toggle('hidden', !show);
  if (titleHold) titleHold.textContent = show ? 'เมื่อ Tag หาย' : 'ตั้งค่าหน่วงเวลาปิดประตู';
  if (closeHint) closeHint.textContent = show ? 'หลัง Tag หาย รอกี่วินาทีแล้วปิดประตู' : 'หลังกดรีโมทรอกี่วินาทีแล้วปิดประตู';

  if (minCard) minCard.classList.toggle('hidden', !show);
  if (rfRoleSec) rfRoleSec.classList.toggle('hidden', show);
}

function toggleEdSched() {
  const chk = $('edSchedEnabled');
  const body = $('edSchedBody');
  if (chk && body) {
    body.classList.toggle('hidden', !chk.checked);
  }
}

function toggleEdWebhook() {
  const chk = $('edWebhookEnabled');
  const body = $('edWebhookBody');
  if (chk && body) {
    body.classList.toggle('hidden', !chk.checked);
  }
}

function selectIconChip(iconVal) {
  const target = iconVal || 'auto';
  const hiddenInput = $('edTagIcon');
  if (hiddenInput) hiddenInput.value = target;
  document.querySelectorAll('#edIconGrid .icon-chip').forEach((chip) => {
    if (chip.getAttribute('data-icon') === target) {
      chip.classList.add('active');
    } else {
      chip.classList.remove('active');
    }
  });
}

function setEditorDefaults() {
  const openDuration = $('edTagOpenDurationSec');
  if (openDuration) openDuration.value = TAG_DEF.openDurationSec || 0;
  const hold = $('edTagHoldSec');
  if (hold) hold.value = TAG_DEF.holdSec;
  const close = $('edTagCloseDelaySec');
  if (close) close.value = TAG_DEF.closeDelaySec;
  const safetyClose = $('edTagSafetyCloseDelaySec');
  if (safetyClose) safetyClose.value = TAG_DEF.safetyCloseDelaySec;
  const rssiMin = $('edTagRssiMin');
  if (rssiMin) rssiMin.value = TAG_DEF.rssiMin;
  if ($('edRfRole')) $('edRfRole').value = '0';
  const schedChk = $('edSchedEnabled');
  if (schedChk) schedChk.checked = false;
  if ($('edSchedStart')) $('edSchedStart').value = '08:00';
  if ($('edSchedEnd')) $('edSchedEnd').value = '17:00';
  toggleEdSched();
  const whChk = $('edWebhookEnabled');
  if (whChk) whChk.checked = false;
  if ($('edWebhookOpen')) $('edWebhookOpen').checked = true;
  if ($('edWebhookClosing')) $('edWebhookClosing').checked = true;
  if ($('edWebhookClose')) $('edWebhookClose').checked = true;
  if ($('edWebhookUrl')) $('edWebhookUrl').value = '';
  if ($('edWebhookMsg')) $('edWebhookMsg').textContent = '';
  toggleEdWebhook();
  selectIconChip('auto');
  updateEdRssiLbl();
  updateEdSafetyDelayWarn();
}

async function openEditorNew() {
  State.editingId = null;
  stopLearnUi();
  await window.AppLoader.ensureView('editor');
  $('editorTitle').textContent = 'เพิ่ม Tag ใหม่';
  $('edName').value = '';
  $('edTagMac').value = '';
  setEditorDefaults();
  toggleRssiCards(true);
  $('edAdmin')?.classList.add('hidden');
  $('edMsg').textContent = '';
  await showView('editor');
}

async function openEditor(id) {
  try {
    State.editingId = id;
    stopLearnUi();
    const d = await api('/api/tags');
    const t = (d.tags || []).find((x) => String(x.id) === String(id));
    if (!t) {
      console.error('Tag not found for id:', id, d.tags);
      alert('ไม่พบข้อมูล Tag ID: ' + id);
      return;
    }
    await window.AppLoader.ensureView('editor');
    const titleEl = $('editorTitle');
    if (titleEl) titleEl.textContent = 'แก้ไข Tag';
    
    if ($('edName')) $('edName').value = t.name || '';
    if ($('edTagMac')) $('edTagMac').value = t.mac || '';
    
    const isRf = !!(t.isRf || (t.mac && t.mac.toUpperCase().includes('RF')));
    const defaultHold = isRf ? TAG_DEF.rfHoldSec : TAG_DEF.holdSec;
    const holdSec = Math.max(1, Math.round((t.tagHoldMs || defaultHold * 1000) / 1000));
    if ($('edTagHoldSec')) $('edTagHoldSec').value = holdSec;
    
    const openDurationSec = Math.round((t.tagOpenDurationMs || 0) / 1000);
    if ($('edTagOpenDurationSec')) $('edTagOpenDurationSec').value = openDurationSec;

    const closeDelaySec = Math.round((t.tagCloseDelayMs ?? TAG_DEF.closeDelaySec * 1000) / 1000);
    if ($('edTagCloseDelaySec')) $('edTagCloseDelaySec').value = closeDelaySec;

    const safetyCloseSec = Math.round((t.tagSafetyCloseDelayMs ?? TAG_DEF.safetyCloseDelaySec * 1000) / 1000);
    if ($('edTagSafetyCloseDelaySec')) $('edTagSafetyCloseDelaySec').value = safetyCloseSec;
    
    if ($('edTagRssiMin')) $('edTagRssiMin').value = t.tagRssiMin ?? TAG_DEF.rssiMin;
    if ($('edRfRole')) $('edRfRole').value = String(t.rfRole ?? 0);

    const schedEnabled = !!t.scheduleEnabled;
    if ($('edSchedEnabled')) $('edSchedEnabled').checked = schedEnabled;
    const startMin = t.scheduleStartMin || 0;
    const endMin = (t.scheduleEndMin != null) ? t.scheduleEndMin : 1439;
    const formatMin = (m) => {
      const h = String(Math.floor(m / 60)).padStart(2, '0');
      const min = String(m % 60).padStart(2, '0');
      return `${h}:${min}`;
    };
    if ($('edSchedStart')) $('edSchedStart').value = formatMin(startMin);
    if ($('edSchedEnd')) $('edSchedEnd').value = formatMin(endMin);
    toggleEdSched();

    const whEnabled = !!t.webhookEnabled;
    if ($('edWebhookEnabled')) $('edWebhookEnabled').checked = whEnabled;
    if ($('edWebhookOpen')) $('edWebhookOpen').checked = (t.webhookOpen !== false);
    if ($('edWebhookClosing')) $('edWebhookClosing').checked = (t.webhookClosing !== false);
    if ($('edWebhookClose')) $('edWebhookClose').checked = (t.webhookClose !== false);
    if ($('edWebhookUrl')) $('edWebhookUrl').value = t.webhookUrl || '';
    if ($('edWebhookMsg')) $('edWebhookMsg').textContent = '';
    toggleEdWebhook();

    selectIconChip(t.icon || 'auto');
    
    toggleRssiCards(!isRf);

    $('edAdmin')?.classList.remove('hidden');
    updateEdRssiLbl();
    updateEdSafetyDelayWarn();
    await showView('editor');
  } catch (err) {
    console.error('Error opening editor:', err);
    alert('เกิดข้อผิดพลาดในการเปิดหน้าแก้ไข: ' + err.message);
  }
}

function closeEditor() {
  stopLearnUi();
  showView('devices');
}

function updateEdRssiLbl() {
  const v = +$('edTagRssiMin').value || -100;
  const lbl = $('edRssiDbm');
  if (lbl) lbl.textContent = v + ' dBm';
}

function updateEdSafetyDelayWarn() {
  const el = $('edTagSafetyCloseDelaySec');
  const warn = $('edSafetyCloseWarn');
  if (!el || !warn) return;
  const val = +el.value;
  if (el.value !== '' && !isNaN(val) && val <= 1) {
    warn.classList.remove('hidden');
  } else {
    warn.classList.add('hidden');
  }
}

async function edStartLearn() {
  try {
    await api('/api/tags/learn', { method: 'POST', body: { action: 'start' } });
    $('edScanLabel').textContent = 'รอรับสัญญาณจาก Tag…';
    ['edDot1', 'edDot2', 'edDot3'].forEach((id, i) => {
      setTimeout(() => $(id)?.classList.add('on'), i * 400);
    });
    if (State.learnTimer) clearTimeout(State.learnTimer);
    State.learnTimer = setTimeout(stopLearnUi, 30000);
  } catch (e) {
    $('edMsg').textContent = 'เริ่มเรียนรู้ไม่สำเร็จ';
  }
}

function stopLearnUi() {
  api('/api/tags/learn', { method: 'POST', body: { action: 'stop' } }).catch(() => {});
  const scanLbl = $('edScanLabel');
  if (scanLbl) scanLbl.textContent = 'กดเพื่อเรียนรู้ Tag';
  ['edDot1', 'edDot2', 'edDot3'].forEach((id) => {
    const el = $(id);
    if (el) {
      el.classList.remove('on');
      el.classList.remove('success');
    }
  });
  if (State.learnTimer) clearTimeout(State.learnTimer);
}

async function saveEditor() {
  const name = $('edName').value.trim() || 'Tag';
  const mac = $('edTagMac').value.trim().toUpperCase();
  const isRf = !!(mac.startsWith('RF:') || ($('edTagMac')?.value || '').toUpperCase().startsWith('RF:') || $('edTagHoldWrap')?.classList.contains('hidden'));
  const holdSec = isRf ? TAG_DEF.rfHoldSec : Math.max(1, +$('edTagHoldSec').value || TAG_DEF.holdSec);
  const openVal = $('edTagOpenDurationSec')?.value;
  const openDurationSec = (openVal !== '' && openVal != null && !isNaN(+openVal)) ? Math.max(0, +openVal) : 0;

  const closeVal = $('edTagCloseDelaySec')?.value;
  const closeDelaySec = (closeVal !== '' && closeVal != null && !isNaN(+closeVal)) ? Math.max(0, +closeVal) : TAG_DEF.closeDelaySec;

  const safetyVal = $('edTagSafetyCloseDelaySec')?.value;
  const safetyCloseSec = (safetyVal !== '' && safetyVal != null && !isNaN(+safetyVal)) ? Math.max(0, +safetyVal) : TAG_DEF.safetyCloseDelaySec;
  const rfRole = +$('edRfRole')?.value || 0;
  const icon = $('edTagIcon')?.value || 'auto';

  const parseMin = (val, defaultVal) => {
    if (!val || typeof val !== 'string' || !val.includes(':')) return defaultVal;
    const parts = val.split(':');
    const h = parseInt(parts[0], 10);
    const m = parseInt(parts[1], 10);
    return (isNaN(h) || isNaN(m)) ? defaultVal : (h * 60 + m);
  };
  const schedEnabled = !!$('edSchedEnabled')?.checked;
  const schedStartMin = parseMin($('edSchedStart')?.value, 0);
  const schedEndMin = parseMin($('edSchedEnd')?.value, 1439);
  const webhookEnabled = !!$('edWebhookEnabled')?.checked;
  const webhookOpen = !!$('edWebhookOpen')?.checked;
  const webhookClosing = !!$('edWebhookClosing')?.checked;
  const webhookClose = !!$('edWebhookClose')?.checked;
  const webhookUrl = $('edWebhookUrl')?.value?.trim() || '';

  const body = {
    name,
    icon,
    tagOpenDurationMs: openDurationSec * 1000,
    tagHoldMs: holdSec * 1000,
    tagCloseDelayMs: closeDelaySec * 1000,
    tagSafetyCloseDelayMs: safetyCloseSec * 1000,
    tagRssiMin: +$('edTagRssiMin').value || TAG_DEF.rssiMin,
    rfRole: isRf ? rfRole : 0,
    scheduleEnabled: schedEnabled,
    scheduleStartMin: schedStartMin,
    scheduleEndMin: schedEndMin,
    webhookEnabled,
    webhookOpen,
    webhookClosing,
    webhookClose,
    webhookUrl,
  };
  try {
    const tagId = State.editingId;
    if (tagId) {
      await api('/api/tags/' + tagId, { method: 'PUT', body });
    } else {
      if (!mac) {
        $('edMsg').textContent = 'กรุณาใส่ MAC หรือเรียนรู้จาก Tag';
        return;
      }
      await api('/api/tags', { method: 'POST', body: { ...body, mac } });
    }
    State.deviceListKey = '';
    stopLearnUi();
    closeEditor();
  } catch (e) {
    $('edMsg').textContent = 'บันทึกไม่สำเร็จ: ' + (e.message || '');
  }
}

async function deleteTag() {
  if (!State.editingId || !confirm('ลบ Tag นี้?')) return;
  await api('/api/tags/' + State.editingId, { method: 'DELETE' });
  State.deviceListKey = '';
  closeEditor();
}

async function testEdWebhook() {
  const msgEl = $('edWebhookMsg');
  const rawUrl = $('edWebhookUrl')?.value?.trim() || '';
  const tagName = $('edName')?.value?.trim() || 'Tag';
  const btn = $('btnEdWebhookTest');

  if (msgEl) {
    msgEl.className = 'hint';
    msgEl.textContent = 'กำลังทดสอบส่งข้อความ...';
  }
  if (btn) btn.disabled = true;

  try {
    const res = await api('/api/webhook/test', {
      method: 'POST',
      body: {
        url: rawUrl,
        tagName: tagName
      }
    });

    if (res.ok) {
      const targetDesc = res.target ? `ไปยัง ${res.target}` : '';
      if (msgEl) {
        msgEl.className = 'hint ok';
        msgEl.textContent = `ส่งคำขอทดสอบแล้ว ${targetDesc}...`;
      }
      setTimeout(async () => {
        try {
          const wh = await api('/api/webhook');
          const st = wh?.outbound?.lastStatus;
          if (st === 200) {
            if (msgEl) {
              msgEl.className = 'hint ok';
              msgEl.textContent = '✅ ทดสอบส่งแจ้งเตือนสำเร็จ (HTTP 200)';
            }
          } else if (st > 0) {
            if (msgEl) {
              msgEl.className = 'hint err';
              msgEl.textContent = `⚠️ ปลายทางตอบกลับ HTTP: ${st}`;
            }
          } else if (st < 0) {
            if (msgEl) {
              msgEl.className = 'hint err';
              msgEl.textContent = `❌ เชื่อมต่อไม่สำเร็จ (รหัสการเชื่อมต่อ: ${st})`;
            }
          }
        } catch (_) {
        } finally {
          if (btn) btn.disabled = false;
        }
      }, 1200);
    } else {
      if (msgEl) {
        msgEl.className = 'hint err';
        msgEl.textContent = 'ส่งไม่สำเร็จ: ' + (res.msg || 'คิวไม่ว่างหรือยังไม่ได้ตั้งค่า');
      }
      if (btn) btn.disabled = false;
    }
  } catch (e) {
    if (msgEl) {
      msgEl.className = 'hint err';
      msgEl.textContent = 'เกิดข้อผิดพลาด: ' + (e.message || '');
    }
    if (btn) btn.disabled = false;
  }
}
function renderScheduleList() {
  const el = $('scheduleList');
  if (!el) return;
  if (!State.scheduleRows.length) {
    el.innerHTML = '<p class="muted" style="font-size:12px">ยังไม่มีตารางเวลา — กดเพิ่มเวลาทำงาน</p>';
    return;
  }
  el.innerHTML = State.scheduleRows
    .map((row, i) => {
      const days = row.days || [0, 1, 2, 3, 4, 5, 6];
      const timeVal = pad2(row.hour ?? 8) + ':' + pad2(row.minute ?? 0);
      const dayChecks = DAY_LABELS.map(
        (lbl, d) =>
          `<label class="schedDayLbl"><input type="checkbox" data-action="sched-day" data-i="${i}" data-d="${d}" ${days.includes(d) ? 'checked' : ''} /><span>${lbl}</span></label>`
      ).join('');
      return `<div class="schedCard" data-i="${i}">
      <div class="schedCardHead">
        <b>ตาราง #${i + 1}</b>
        <button type="button" class="schedCardDel" data-action="remove-sched-row" data-i="${i}" title="ลบ">×</button>
      </div>
      <div class="schedField">
        <label>เวลาที่ต้องการให้ทำงาน</label>
        <input type="time" value="${timeVal}" data-action="sched-time" data-i="${i}" />
      </div>
      <div class="schedField">
        <label>การทำงาน</label>
        <select data-action="sched-action" data-i="${i}">
          <option value="open" ${row.action === 'open' ? 'selected' : ''}>เปิดประตู</option>
          <option value="close" ${row.action === 'close' ? 'selected' : ''}>ปิดประตู</option>
        </select>
      </div>
      <div class="schedField">
        <label>วัน</label>
        <div class="schedDaysRow">${dayChecks}</div>
      </div>
      <label class="schedDayLbl" style="margin-top:8px"><input type="checkbox" data-action="sched-enabled" data-i="${i}" ${row.enabled ? 'checked' : ''} /><span>เปิดใช้งานช่วงนี้</span></label>
    </div>`;
    })
    .join('');
  const schedBadge = $('schedHeadBadge');
  if (schedBadge) {
    const activeCount = State.scheduleRows.filter((r) => r.enabled).length;
    if (activeCount > 0) {
      schedBadge.textContent = `${activeCount} เวลา`;
      schedBadge.className = 'badgeMini ok';
    } else if (State.scheduleRows.length > 0) {
      schedBadge.textContent = 'ปิดชั่วคราว';
      schedBadge.className = 'badgeMini off';
    } else {
      schedBadge.textContent = '0 เวลา';
      schedBadge.className = 'badgeMini off';
    }
  }
}

function setSchedTime(i, val) {
  if (!State.scheduleRows[i] || !val) return;
  const [h, m] = val.split(':').map((x) => +x);
  State.scheduleRows[i].hour = h;
  State.scheduleRows[i].minute = m;
}

function addScheduleRow() {
  State.scheduleRows.push({
    id: 's' + Date.now().toString(36),
    enabled: true,
    action: 'open',
    hour: 8,
    minute: 0,
    days: [1, 2, 3, 4, 5],
  });
  renderScheduleList();
}

function removeScheduleRow(i) {
  State.scheduleRows.splice(i, 1);
  renderScheduleList();
}

function toggleSchedDay(i, d, checked) {
  const row = State.scheduleRows[i];
  if (!row) return;
  if (!row.days) row.days = [];
  const idx = row.days.indexOf(d);
  if (checked && idx < 0) row.days.push(d);
  else if (!checked && idx >= 0) row.days.splice(idx, 1);
  row.days.sort((a, b) => a - b);
}

async function saveSchedules() {
  $('schedMsg').textContent = 'บันทึก…';
  try {
    await api('/api/schedule', { method: 'PUT', body: { schedules: State.scheduleRows } });
    setHint('schedMsg', 'hint ok', 'บันทึกตารางเวลาเรียบร้อย');
  } catch (e) {
    setHint('schedMsg', 'hint err', 'บันทึกไม่สำเร็จ — กรุณาซิงค์เวลาก่อน');
  }
}
function updateWifiStaUi(d) {
  const connected = !!d.staConnected;
  $('wifiStaInfo')?.classList.toggle('hidden', !connected);
  const ipEl = $('wifiStaIp');
  if (ipEl) ipEl.textContent = connected ? d.staIp || '—' : '—';
  const fixIp = $('swFixIp');
  if (fixIp) fixIp.checked = !!d.wifiStatic;
  const hint = $('fixIpHint');
  if (hint) {
    hint.textContent = d.wifiStatic
      ? 'Fix IP เปิด — ใช้ ' + (d.wifiIp || d.staIp || '—')
      : 'เปิด Fix IP เพื่อใช้ IP ปัจจุบันถาวร';
  }
  const localUrlRow = $('wifiStaLocalUrlRow');
  const localUrlLink = $('wifiLocalUrlLink');
  const host = d.hostname || (d.localUrl ? d.localUrl.replace(/^https?:\/\//, '').replace(/\.local.*$/, '') : '');
  if (localUrlRow && localUrlLink) {
    if (connected && (host || d.localUrl)) {
      const url = d.localUrl || ('http://' + host + '.local');
      localUrlLink.href = url;
      localUrlLink.textContent = url;
      localUrlRow.classList.remove('hidden');
    } else {
      localUrlRow.classList.add('hidden');
    }
  }
  const wifiBadge = $('wifiHeadBadge');
  if (wifiBadge) {
    if (connected) {
      wifiBadge.textContent = d.staIp || 'เชื่อมต่อแล้ว';
      wifiBadge.className = 'badgeMini ok';
    } else {
      wifiBadge.textContent = 'ไม่ได้เชื่อมต่อ';
      wifiBadge.className = 'badgeMini off';
    }
  }
}

async function toggleFixIp() {
  const enabled = $('swFixIp')?.checked;
  try {
    const d = await api('/api/wifi/static', { method: 'POST', body: { enabled } });
    updateWifiStaUi({ staConnected: true, staIp: d.wifiIp, wifiStatic: d.wifiStatic, wifiIp: d.wifiIp });
    $('wifiMsg').textContent = enabled ? 'เปิด Fix IP แล้ว' : 'ปิด Fix IP — ใช้ DHCP';
  } catch (e) {
    $('wifiMsg').textContent = 'ตั้ง Fix IP ไม่สำเร็จ';
    if ($('swFixIp')) $('swFixIp').checked = !enabled;
  }
}

let gEspNowStateDisabled = false;
let gWifiConnectPollTimer = null;
let gWifiConnectStartTime = 0;

async function setEspNowDisabled(disabled) {
  if (!disabled) {
    if (gWifiConnectPollTimer) {
      clearTimeout(gWifiConnectPollTimer);
      gWifiConnectPollTimer = null;
    }
  }
  // In normal mode, do not disable ESP-NOW. Let it run all the time.
  if (disabled && !State.safeMode) {
    return;
  }
  if (gEspNowStateDisabled === disabled) return;
  try {
    const url = disabled ? '/api/espnow/disable' : '/api/espnow/enable';
    await api(url, { method: 'POST' });
    gEspNowStateDisabled = disabled;
    console.log('[espnow] state updated: disabled=' + disabled);
  } catch (e) {
    console.error('Failed to change ESP-NOW state', e);
  }
}

async function pollWifiConnectStatus() {
  try {
    const s = await api('/api/wifi/status');
    const msgEl = $('wifiMsg');
    if (!msgEl) return;
    
    msgEl.classList.remove('err', 'ok');
    msgEl.textContent = s.statusThai || 'กำลังเชื่อมต่อ...';
    
    const elapsed = Date.now() - gWifiConnectStartTime;
    
    if (s.status === 3) { // WL_CONNECTED
      msgEl.classList.add('ok');
      await setEspNowDisabled(false);
      clearTimeout(gWifiConnectPollTimer);
      gWifiConnectPollTimer = null;
      
      const d = await api('/api/settings');
      updateWifiStaUi(d);
      refreshSetupWizardUi(d);
    } else if (elapsed > 15000) { // Timeout after 15 seconds
      msgEl.classList.add('err');
      msgEl.textContent = 'เชื่อมต่อไม่สำเร็จ (หมดเวลา)';
      clearTimeout(gWifiConnectPollTimer);
      gWifiConnectPollTimer = null;
    } else if (s.status === 1 || s.status === 4 || s.status === 5) { // WL_NO_SSID_AVAIL, WL_CONNECT_FAILED, WL_CONNECTION_LOST
      // Wait for at least 4 seconds before reporting connection failures to avoid transient states
      if (elapsed > 4000) {
        msgEl.classList.add('err');
        clearTimeout(gWifiConnectPollTimer);
        gWifiConnectPollTimer = null;
      } else {
        gWifiConnectPollTimer = setTimeout(pollWifiConnectStatus, 1000);
      }
    } else {
      gWifiConnectPollTimer = setTimeout(pollWifiConnectStatus, 1000);
    }
  } catch (e) {
    console.error('Error polling wifi status:', e);
    gWifiConnectPollTimer = setTimeout(pollWifiConnectStatus, 1500);
  }
}

async function wifiScan() {
  if (gWifiConnectPollTimer) {
    clearTimeout(gWifiConnectPollTimer);
    gWifiConnectPollTimer = null;
  }
  const msgEl = $('wifiMsg');
  if (msgEl) {
    msgEl.classList.remove('err', 'ok');
    msgEl.textContent = 'กำลังสแกน Wi‑Fi...';
  }
  
  try {
    await api('/api/wifi/scan');
    const poll = async () => {
      try {
        const s = await api('/api/wifi/scan/status');
        if (!s.ok || s.error) {
          if (msgEl) {
            msgEl.classList.add('err');
            msgEl.textContent = 'สแกน Wi-Fi ล้มเหลว';
          }
          return;
        }
        if (!s.done) return setTimeout(poll, 800);
        
        const el = $('wifiScanList');
        if (el) {
          el.innerHTML = (s.networks || [])
            .map((n) => {
              const ssid = escHtml(n.ssid);
              return `<button type="button" class="filterPill" style="margin:4px 4px 0 0" data-action="wifi-pick-ssid" data-ssid="${ssid}">${ssid} (${n.rssi})</button>`;
            })
            .join('');
        }
        if (msgEl) {
          msgEl.classList.add('ok');
          msgEl.textContent = 'สแกนเสร็จสิ้น';
        }
      } catch (e) {
        if (msgEl) {
          msgEl.classList.add('err');
          msgEl.textContent = 'สแกน Wi-Fi ไม่สำเร็จ';
        }
      }
    };
    poll();
  } catch (e) {
    if (msgEl) {
      msgEl.classList.add('err');
      msgEl.textContent = 'เริ่มสแกน Wi-Fi ไม่สำเร็จ';
    }
  }
}

async function wifiConnect() {
  const ssid = $('wifiSsid')?.value || '';
  const password = $('wifiPass')?.value || '';
  const msgEl = $('wifiMsg');
  
  if (!ssid) {
    if (msgEl) {
      msgEl.classList.remove('ok');
      msgEl.classList.add('err');
      msgEl.textContent = 'กรุณากรอกชื่อ Wi-Fi (SSID)';
    }
    return;
  }
  
  try {
    if (gWifiConnectPollTimer) {
      clearTimeout(gWifiConnectPollTimer);
    }
    
    if (msgEl) {
      msgEl.classList.remove('err', 'ok');
      msgEl.textContent = 'กำลังเริ่มเชื่อมต่อ...';
    }
    
    gWifiConnectStartTime = Date.now();
    
    await api('/api/wifi/connect', {
      method: 'POST',
      body: { ssid, password },
    });
    
    gWifiConnectPollTimer = setTimeout(pollWifiConnectStatus, 500);
  } catch (e) {
    if (msgEl) {
      msgEl.classList.remove('ok');
      msgEl.classList.add('err');
      msgEl.textContent = 'ส่งคำสั่งเชื่อมต่อไม่สำเร็จ';
    }
  }
}
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
function updateMqttStatusUi(mqtt) {
  if (!mqtt) return;
  const box = $('mqttStatusBox');
  if (box) {
    box.classList.remove('on', 'wait');
    if (!mqtt.enabled) {
      box.textContent = 'สถานะ: ปิดอยู่';
    } else if (mqtt.connected) {
      box.textContent = 'สถานะ: ' + (mqtt.status || 'เชื่อมต่อ MQTT สำเร็จ');
      box.classList.add('on');
    } else {
      box.textContent = 'สถานะ: ' + (mqtt.status || 'กำลังเชื่อมต่อ…');
      box.classList.add('wait');
    }
  }

  const badge = $('mqttHeadBadge');
  if (badge) {
    if (!mqtt.enabled) {
      badge.textContent = 'ปิดใช้งาน';
      badge.className = 'badgeMini off';
    } else if (mqtt.connected) {
      badge.textContent = 'ออนไลน์';
      badge.className = 'badgeMini ok';
    } else {
      badge.textContent = 'รอเชื่อมต่อ';
      badge.className = 'badgeMini warn';
    }
  }
}

function updateMqttUi(mqtt) {
  if (!mqtt) return;
  if ($('swMqtt')) $('swMqtt').checked = !!mqtt.enabled;
  if ($('mqttHost')) $('mqttHost').value = mqtt.host || '';
  if ($('mqttPort')) $('mqttPort').value = mqtt.port || 1883;
  if ($('mqttUser')) $('mqttUser').value = mqtt.user || '';
  if ($('mqttPass')) $('mqttPass').value = mqtt.pass || '';
  if ($('mqttTopicPrefix')) $('mqttTopicPrefix').value = mqtt.topicPrefix || 'autodoor';
  if ($('mqttDiscPrefix')) $('mqttDiscPrefix').value = mqtt.discoveryPrefix || 'homeassistant';
  if ($('haYamlPreview')) $('haYamlPreview').value = buildHaDashboardYaml();
  updateMqttStatusUi(mqtt);
}

function cleanMqttHost(raw) {
  if (!raw) return '';
  let h = raw.trim();
  h = h.replace(/^(http:\/\/|https:\/\/|mqtt:\/\/|mqtts:\/\/)/i, '');
  h = h.split('/')[0];
  h = h.split(':')[0];
  return h.trim();
}

function mqttPayload(enabledOverride) {
  const hostVal = cleanMqttHost($('mqttHost')?.value || '');
  if ($('mqttHost') && hostVal) $('mqttHost').value = hostVal;
  return {
    enabled: enabledOverride != null ? enabledOverride : !!$('swMqtt')?.checked,
    host: hostVal,
    port: parseInt($('mqttPort')?.value || '1883', 10) || 1883,
    user: ($('mqttUser')?.value || '').trim(),
    pass: $('mqttPass')?.value || '',
    topicPrefix: ($('mqttTopicPrefix')?.value || '').trim() || 'autodoor',
    discoveryPrefix: ($('mqttDiscPrefix')?.value || '').trim() || 'homeassistant',
  };
}

async function toggleMqtt() {
  try {
    const d = await api('/api/mqtt', { method: 'PUT', body: mqttPayload() });
    updateMqttStatusUi(d);
  } catch (e) {
    if ($('mqttMsg')) $('mqttMsg').textContent = 'ตั้ง MQTT ไม่สำเร็จ';
  }
}

async function saveMqtt() {
  setHint('mqttMsg', 'hint', 'กำลังบันทึกและเชื่อมต่อ…');
  try {
    const d = await api('/api/mqtt', { method: 'PUT', body: mqttPayload() });
    setHint('mqttMsg', 'hint ok', 'บันทึกการตั้งค่า MQTT สำเร็จ');
    updateMqttStatusUi(d);
  } catch (e) {
    setHint('mqttMsg', 'hint err', 'บันทึกการตั้งค่า MQTT ไม่สำเร็จ');
  }
}

async function refreshMqttStatus() {
  try {
    const mqtt = await api('/api/mqtt');
    updateMqttStatusUi(mqtt);
  } catch (_) {}
}

function buildHaDashboardYaml() {
  return [
    'type: vertical-stack',
    'cards:',
    '  # 1. แถบแสดงสถานะประตู',
    '  - type: tile',
    '    entity: sensor.autodoor_rf2_sthaanapratuu',
    '    name: ประตูบ้าน (AutoDoor-RF2)',
    '    icon: mdi:garage-variant',
    '    color: blue',
    '  # 2. แผงปุ่มควบคุมขนาดใหญ่ 3 ปุ่ม (เปิด / หยุด / ปิด)',
    '  - type: grid',
    '    columns: 3',
    '    square: false',
    '    cards:',
    '      - type: button',
    '        name: เปิดประตู',
    '        icon: mdi:arrow-up-bold-box',
    '        entity: button.autodoor_rf2_epidpratuu',
    '        tap_action:',
    '          action: toggle',
    '      - type: button',
    '        name: หยุดประตู',
    '        icon: mdi:stop-circle',
    '        entity: button.autodoor_rf2_hyudpratuu',
    '        tap_action:',
    '          action: toggle',
    '      - type: button',
    '        name: ปิดประตู',
    '        icon: mdi:arrow-down-bold-box',
    '        entity: button.autodoor_rf2_pidpratuu',
    '        tap_action:',
    '          action: toggle',
    '  # 3. แผงข้อมูลเซนเซอร์',
    '  - type: entities',
    '    title: ข้อมูลเซนเซอร์',
    '    show_header_toggle: false',
    '    entities:',
    '      - entity: binary_sensor.autodoor_safety_beam',
    '        name: เซนเซอร์กันหนีบ (Safety Beam)',
    '      - entity: sensor.autodoor_last_trigger',
    '        name: รถ / รีโมตที่สั่งล่าสุด'
  ].join('\n');
}

async function copyHaDashboardYaml() {
  const yaml = buildHaDashboardYaml();
  try {
    if (navigator.clipboard?.writeText) {
      await navigator.clipboard.writeText(yaml);
    } else {
      const ta = document.createElement('textarea');
      ta.value = yaml;
      document.body.appendChild(ta);
      ta.select();
      document.execCommand('copy');
      document.body.removeChild(ta);
    }
    const btn = $('btnCopyHaYaml');
    if (btn) {
      const old = btn.textContent;
      btn.textContent = '✓ คัดลอกสำเร็จ!';
      btn.style.color = '#2ecc71';
      setTimeout(() => {
        btn.textContent = old;
        btn.style.color = '';
      }, 2500);
    }
    if (typeof toast === 'function') toast('คัดลอกโค้ด Lovelace Dashboard YAML เรียบร้อยแล้ว');
  } catch (e) {
    prompt('คัดลอกโค้ดนี้:', yaml);
  }
}
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
async function loadWebAuthUi() {
  try {
    const r = await api('/api/webauth');
    const hint = $('webAuthHint');
    if (hint) {
      hint.textContent = r.hasPassword ? '🔒 เปิดใช้รหัสผ่านแล้ว' : '🔓 ยังไม่ได้ตั้งรหัสผ่าน';
    }
    if ($('webPass')) $('webPass').value = '';
    if ($('webAuthMsg')) $('webAuthMsg').textContent = '';
    const authBadge = $('webAuthHeadBadge');
    if (authBadge) {
      if (r.hasPassword) {
        authBadge.textContent = 'มีรหัสผ่าน';
        authBadge.className = 'badgeMini ok';
      } else {
        authBadge.textContent = 'ไม่มีรหัส';
        authBadge.className = 'badgeMini off';
      }
    }
    State.webAuthHasPassword = !!r.hasPassword;
    updateSafeModeBanner(!!r.safeMode);
    if (typeof refreshSetupWizardUi === 'function') {
      refreshSetupWizardUi(State.settingsCache, State.blynkCache, State.tagCount);
    }
  } catch (_) {}
}

async function saveWebAuth() {
  const pass = $('webPass')?.value ?? '';
  if (!pass && !confirm('ปิดรหัสผ่านหน้าเว็บ?')) return;
  if ($('webAuthMsg')) $('webAuthMsg').textContent = 'บันทึก…';
  try {
    const r = await api('/api/webauth', { method: 'POST', body: { password: pass } });
    setHint(
      'webAuthMsg',
      'hint ok',
      r.hasPassword ? 'ตั้งรหัสผ่านแล้ว' : 'ปิดรหัสผ่านแล้ว'
    );
    if ($('webPass')) $('webPass').value = '';
    loadWebAuthUi();
  } catch (e) {
    setHint(
      'webAuthMsg',
      'hint err',
      e.message === 'safe_mode' ? 'อยู่ใน Safe Mode — ตั้งรหัสไม่ได้' : 'บันทึกไม่สำเร็จ'
    );
  }
}
async function loadWebhookUi() {
  try {
    State.webhookCache = await api('/api/webhook');
    const hint = $('hookHostHint');
    if (hint) hint.textContent = 'http://' + (State.webhookCache.host || '—');

    const out = State.webhookCache?.outbound;
    if (out) {
      if ($('swWhOut')) $('swWhOut').checked = !!out.enabled;
      if ($('whOutUrl')) $('whOutUrl').value = out.url || '';
      if ($('swWhLine')) $('swWhLine').checked = !!out.lineEnabled;
      if ($('whLineToken')) $('whLineToken').value = out.lineToken || '';
      if ($('whLineTarget')) $('whLineTarget').value = out.lineTargetId || '';
      if ($('chkWhSafety')) $('chkWhSafety').checked = out.eventSafety !== false;
      if ($('chkWhOpenDuration')) $('chkWhOpenDuration').checked = out.eventOpenDuration !== false;
      if ($('whOpenDurationMin')) $('whOpenDurationMin').value = out.openDurationMinutes || 5;
    }
  } catch (_) {
    State.webhookCache = null;
  }
}

async function copyHookText(text, okMsg) {
  try {
    if (navigator.clipboard?.writeText) {
      await navigator.clipboard.writeText(text);
    } else {
      const ta = document.createElement('textarea');
      ta.value = text;
      ta.style.position = 'fixed';
      ta.style.left = '-9999px';
      document.body.appendChild(ta);
      ta.select();
      document.execCommand('copy');
      document.body.removeChild(ta);
    }
    setHint('hookMsg', 'hint ok', okMsg || 'คัดลอกแล้ว');
  } catch (e) {
    setHint('hookMsg', 'hint err', 'คัดลอกไม่ได้');
  }
}

async function copyHookUrl(slot) {
  if (!State.webhookCache) await loadWebhookUi();
  const url = State.webhookCache?.hooks?.[slot] || '';
  if (!url) {
    setHint('hookMsg', 'hint err', 'โหลดลิงก์ไม่ได้');
    return;
  }
  const labels = { open: 'เปิด', stop: 'หยุด', close: 'ปิด' };
  await copyHookText(url, 'คัดลอกลิงก์ ' + (labels[slot] || slot) + ' แล้ว');
}

async function copyBlynkHookUrl(slot) {
  if (!State.webhookCache) await loadWebhookUi();
  const url = State.webhookCache?.blynk?.[slot] || '';
  if (!url) {
    setHint('hookMsg', 'hint err', 'ตั้งค่า Blynk Token ก่อน');
    return;
  }
  const labels = { open: 'เปิด (Blynk)', stop: 'หยุด (Blynk)', close: 'ปิด (Blynk)' };
  await copyHookText(url, 'คัดลอก ' + (labels[slot] || slot) + ' แล้ว');
}

function buildHaLanYaml() {
  if (!State.webhookCache?.hooks) return '';
  const h = State.webhookCache.hooks;
  return [
    '# Home Assistant — ควบคุมผ่าน LAN (ต้องอยู่เครือข่ายเดียวกับเครื่อง)',
    '# เพิ่มที่ /homeassistant/configuration.yaml',
    'rest_command:',
    '  autodoor_rf2_lan_open:',
    '    url: "' + h.open + '"',
    '    method: GET',
    '  autodoor_rf2_lan_stop:',
    '    url: "' + h.stop + '"',
    '    method: GET',
    '  autodoor_rf2_lan_close:',
    '    url: "' + h.close + '"',
    '    method: GET',
    '  autodoor_rf2_lan_tag_auto_toggle:',
    '    url: "' + (h.tagAutoToggle || '') + '"',
    '    method: GET',
    '',
    '',
    '# เพิ่มที่ /homeassistant/scripts.yaml',
    'autodoor_rf2_lan_open:',
    '  alias: "AutoDoor-RF2 เปิดประตู (LAN)"',
    '  mode: single',
    '  sequence:',
    '    - service: rest_command.autodoor_rf2_lan_open',
    '',
    'autodoor_rf2_lan_stop:',
    '  alias: "AutoDoor-RF2 หยุดประตู (LAN)"',
    '  mode: single',
    '  sequence:',
    '    - service: rest_command.autodoor_rf2_lan_stop',
    '',
    'autodoor_rf2_lan_close:',
    '  alias: "AutoDoor-RF2 ปิดประตู (LAN)"',
    '  mode: single',
    '  sequence:',
    '    - service: rest_command.autodoor_rf2_lan_close',
    '',
    'autodoor_rf2_lan_tag_auto_toggle:',
    '  alias: "AutoDoor-RF2 สลับ Tag อัตโนมัติ (LAN)"',
    '  mode: single',
    '  sequence:',
    '    - service: rest_command.autodoor_rf2_lan_tag_auto_toggle',
    '',
    '',
    '# การเพิ่มปุ่มในหน้า dashboard',
    '# กด Edit Dashboard -> กด Add Card -> Manual',
    'type: horizontal-stack',
    'cards:',
    '  - type: button',
    '    name: เปิดประตู',
    '    icon: mdi:garage-open',
    '    tap_action:',
    '      action: call-service',
    '      service: script.autodoor_rf2_lan_open',
    '    hold_action:',
    '      action: none',
    '  - type: button',
    '    name: หยุดประตู',
    '    icon: mdi:stop-circle-outline',
    '    tap_action:',
    '      action: call-service',
    '      service: script.autodoor_rf2_lan_stop',
    '    hold_action:',
    '      action: none',
    '  - type: button',
    '    name: ปิดประตู',
    '    icon: mdi:garage',
    '    tap_action:',
    '      action: call-service',
    '      service: script.autodoor_rf2_lan_close',
    '    hold_action:',
    '      action: none',
    '  - type: button',
    '    name: ล็อก Tag',
    '    icon: mdi:lock',
    '    tap_action:',
    '      action: call-service',
    '      service: script.autodoor_rf2_lan_tag_auto_toggle',
    '    hold_action:',
    '      action: none',
    'title: AutoDoor-RF2 (LAN)',
  ].join('\n');
}

function buildHaBlynkYaml() {
  const b = State.webhookCache?.blynk;
  const host = State.webhookCache?.blynkHost || 'sgp1.blynk.cloud';
  const tok =
    State.webhookCache?.blynkToken && State.webhookCache.blynkToken.length >= 8
      ? State.webhookCache.blynkToken
      : 'YOUR_BLYNK_TOKEN';
  const base = 'https://' + host + '/external/api/update?token=' + tok;
  const bOpen = b?.open || base + '&V0=1';
  const bStop = b?.stop || base + '&V1=1';
  const bClose = b?.close || base + '&V2=1';
  return [
    '# Home Assistant + Blynk Cloud (ไม่ต้องอยู่ Wi‑Fi เดียวกับเครื่อง)',
    '# เพิ่มที่ /homeassistant/configuration.yaml',
    'rest_command:',
    '  autodoor_rf2_blynk_open:',
    '    url: "' + bOpen + '"',
    '    method: GET',
    '  autodoor_rf2_blynk_stop:',
    '    url: "' + bStop + '"',
    '    method: GET',
    '  autodoor_rf2_blynk_close:',
    '    url: "' + bClose + '"',
    '    method: GET',
    '',
    '',
    '# เพิ่มที่ /homeassistant/scripts.yaml',
    'autodoor_rf2_blynk_open:',
    '  alias: "AutoDoor-RF2 เปิดประตู (Blynk)"',
    '  mode: single',
    '  sequence:',
    '    - service: rest_command.autodoor_rf2_blynk_open',
    '',
    'autodoor_rf2_blynk_stop:',
    '  alias: "AutoDoor-RF2 หยุดประตู (Blynk)"',
    '  mode: single',
    '  sequence:',
    '    - service: rest_command.autodoor_rf2_blynk_stop',
    '',
    'autodoor_rf2_blynk_close:',
    '  alias: "AutoDoor-RF2 ปิดประตู (Blynk)"',
    '  mode: single',
    '  sequence:',
    '    - service: rest_command.autodoor_rf2_blynk_close',
    '',
    '',
    '# การเพิ่มปุ่มในหน้า dashboard (ควบคุมผ่าน Blynk Cloud)',
    '# กด Edit Dashboard -> กด Add Card -> Manual',
    'type: horizontal-stack',
    'cards:',
    '  - type: button',
    '    name: เปิดประตู',
    '    icon: mdi:garage-open',
    '    tap_action:',
    '      action: call-service',
    '      service: script.autodoor_rf2_blynk_open',
    '    hold_action:',
    '      action: none',
    '  - type: button',
    '    name: หยุดประตู',
    '    icon: mdi:stop-circle-outline',
    '    tap_action:',
    '      action: call-service',
    '      service: script.autodoor_rf2_blynk_stop',
    '    hold_action:',
    '      action: none',
    '  - type: button',
    '    name: ปิดประตู',
    '    icon: mdi:garage',
    '    tap_action:',
    '      action: call-service',
    '      service: script.autodoor_rf2_blynk_close',
    '    hold_action:',
    '      action: none',
    'title: AutoDoor-RF2 (Blynk Cloud)',
  ].join('\n');
}

async function copyHaLanYaml() {
  if (!State.webhookCache) await loadWebhookUi();
  await copyHookText(buildHaLanYaml(), 'คัดลอก YAML Home Assistant (LAN) แล้ว');
}

async function copyHaBlynkYaml() {
  if (!State.webhookCache) await loadWebhookUi();
  await copyHookText(buildHaBlynkYaml(), 'คัดลอก YAML Home Assistant (Blynk) แล้ว');
}

function buildJsonApiString() {
  const host = State.webhookCache?.blynkHost || 'sgp1.blynk.cloud';
  const tok =
    State.webhookCache?.blynkToken && State.webhookCache.blynkToken.length >= 8
      ? State.webhookCache.blynkToken
      : 'YOUR_BLYNK_TOKEN';

  const lanHost = State.webhookCache?.host || '192.168.1.100';
  const lanBase = 'http://' + lanHost;
  const blynkBase = 'https://' + host + '/external/api';

  const jsonObj = {
    device: State.deviceStatus?.deviceName || 'AutoDoor',
    blynk_host: host,
    token: tok,
    endpoints: {
      control: {
        open: blynkBase + '/update?token=' + tok + '&V0=1',
        stop: blynkBase + '/update?token=' + tok + '&V1=1',
        close: blynkBase + '/update?token=' + tok + '&V2=1'
      },
      status: {
        online: blynkBase + '/isHardwareConnected?token=' + tok,
        door_text_thai: blynkBase + '/get?token=' + tok + '&V4',
        magnetic_sensor: blynkBase + '/get?token=' + tok + '&V3'
      }
    },
    lan_endpoints: {
      open: lanBase + '/api/control?slot=open',
      stop: lanBase + '/api/control?slot=stop',
      close: lanBase + '/api/control?slot=close',
      status: lanBase + '/api/status'
    }
  };

  return JSON.stringify(jsonObj, null, 2);
}

async function copyJsonApi() {
  if (!State.webhookCache) await loadWebhookUi();
  await copyHookText(buildJsonApiString(), 'คัดลอก JSON ควบคุมและสถานะเรียบร้อยแล้ว');
}

async function saveOutboundWebhook() {
  const enabled = !!$('swWhOut')?.checked;
  const lineEnabled = !!$('swWhLine')?.checked;
  const lineToken = $('whLineToken')?.value?.trim() || '';
  const lineTargetId = $('whLineTarget')?.value?.trim() || '';
  const eventSafety = !!$('chkWhSafety')?.checked;
  const eventOpenDuration = !!$('chkWhOpenDuration')?.checked;
  const openDurationMinutes = Math.max(1, Math.min(60, parseInt($('whOpenDurationMin')?.value, 10) || 5));

  if (lineEnabled && !lineToken) {
    setHint('whOutMsg', 'hint err', 'กรุณาระบุ LINE Channel Access Token หรือปิดสวิตช์ LINE');
    return;
  }

  try {
    setHint('whOutMsg', 'hint', 'กำลังบันทึก...');
    const body = {
      enabled,
      lineEnabled,
      lineToken,
      lineTargetId,
      eventSafety,
      eventOpenDuration,
      openDurationMinutes
    };
    if ($('whOutUrl')) {
      body.url = $('whOutUrl').value.trim();
    }
    const res = await api('/api/webhook', {
      method: 'POST',
      body
    });
    if (res.ok) {
      setHint('whOutMsg', 'hint ok', 'บันทึกการตั้งค่าระบบแจ้งเตือนเรียบร้อย');
      await loadWebhookUi();
    } else {
      setHint('whOutMsg', 'hint err', 'บันทึกไม่สำเร็จ: ' + (res.error || 'error'));
    }
  } catch (e) {
    setHint('whOutMsg', 'hint err', 'บันทึกไม่สำเร็จ: ' + e.message);
  }
}

async function testOutboundWebhook() {
  const enabled = !!$('swWhOut')?.checked;
  const lineEnabled = !!$('swWhLine')?.checked;
  const lineToken = $('whLineToken')?.value?.trim() || '';
  const lineTargetId = $('whLineTarget')?.value?.trim() || '';

  try {
    setHint('whOutMsg', 'hint', 'กำลังส่งข้อความทดสอบ...');
    const body = {
      enabled,
      lineEnabled,
      lineToken,
      lineTargetId,
      eventSafety: !!$('chkWhSafety')?.checked,
      eventOpenDuration: !!$('chkWhOpenDuration')?.checked,
      openDurationMinutes: Math.max(1, Math.min(60, parseInt($('whOpenDurationMin')?.value, 10) || 5))
    };
    if ($('whOutUrl')) {
      body.url = $('whOutUrl').value.trim();
    }
    await api('/api/webhook', {
      method: 'POST',
      body
    });
    const res = await api('/api/webhook/test', {
      method: 'POST',
      body: { url: body.url }
    });
    if (res.ok) {
      const targetDesc = res.target ? ` (${res.target})` : '';
      setHint('whOutMsg', 'hint ok', 'ส่งข้อความทดสอบแล้ว' + targetDesc + '...');
      setTimeout(async () => {
        try {
          const wh = await api('/api/webhook');
          const st = wh?.outbound?.lastStatus;
          if (st === 200) {
            setHint('whOutMsg', 'hint ok', '✅ ส่งการแจ้งเตือนสำเร็จ (HTTP 200)');
          } else if (st > 0) {
            setHint('whOutMsg', 'hint err', `⚠️ เซิร์ฟเวอร์ตอบกลับรหัส: ${st}`);
          } else if (st < 0) {
            setHint('whOutMsg', 'hint err', `❌ เชื่อมต่อไปยังปลายทางไม่สำเร็จ (รหัสข้อผิดพลาด: ${st})`);
          }
        } catch (_) {}
      }, 1200);
    } else {
      setHint('whOutMsg', 'hint err', 'ส่งไม่สำเร็จ: ' + (res.msg || 'error'));
    }
  } catch (e) {
    setHint('whOutMsg', 'hint err', 'เกิดข้อผิดพลาด: ' + e.message);
  }
}
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
  const closeSec = parseInt($('safetyCloseDelaySec')?.value ?? 3);
  if (isNaN(closeSec) || closeSec < 0 || closeSec > 60) {
    setHint('safetyParamsMsg', 'hint err', 'หน่วงปิดเมื่อพ้นลำแสง ต้องอยู่ระหว่าง 0 ถึง 60 วินาที');
    return;
  }
  const revSec = parseInt($('safetyReverseDelaySec')?.value ?? 5);
  if (isNaN(revSec) || revSec < 1 || revSec > 60) {
    setHint('safetyParamsMsg', 'hint err', 'หน่วงปิดเมื่อดีดกลับ ต้องอยู่ระหว่าง 1 ถึง 60 วินาที');
    return;
  }
  const remoteSec = parseInt($('remotePassageDelaySec')?.value ?? 5);
  const safeRemoteSec = Math.max(1, Math.min(60, isNaN(remoteSec) ? 5 : remoteSec));

  if ($('safetyParamsMsg')) $('safetyParamsMsg').textContent = 'กำลังบันทึก…';
  try {
    await api('/api/settings', {
      method: 'PUT',
      body: {
        safetyCloseDelayMs: closeSec * 1000,
        safetyReverseDelayMs: revSec * 1000,
        remotePassageDelayMs: safeRemoteSec * 1000
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
async function saveDeviceName() {
  const name = $('deviceName')?.value?.trim() || 'AutoDoor';
  setHint('deviceNameMsg', 'hint', 'กำลังบันทึก...');
  try {
    await api('/api/settings', {
      method: 'PUT',
      body: { deviceName: name }
    });
    setHint('deviceNameMsg', 'hint ok', 'บันทึกสำเร็จ');
  } catch (e) {
    setHint('deviceNameMsg', 'hint err', 'บันทึกไม่สำเร็จ');
  }
}

function cleanHostnameInput(val) {
  if (!val) return '';
  return val.toLowerCase().replace(/[^a-z0-9-]/g, '-').replace(/-+/g, '-').replace(/^-+/, '').slice(0, 32);
}

function updateDeviceHostnameLink(host) {
  const clean = cleanHostnameInput(host) || window.gDefaultHostname || '';
  const linkEl = $('deviceHostnameLink');
  if (linkEl) {
    if (clean) {
      linkEl.href = 'http://' + clean + '.local';
      linkEl.textContent = 'http://' + clean + '.local';
    } else {
      linkEl.href = '#';
      linkEl.textContent = '—';
    }
  }
  const urlBadge = $('urlHeadBadge');
  if (urlBadge) {
    urlBadge.textContent = clean ? clean + '.local' : '.local';
  }
}

async function saveHostname() {
  const raw = $('deviceHostname')?.value?.trim() || '';
  const clean = cleanHostnameInput(raw);
  if ($('deviceHostname')) $('deviceHostname').value = clean;
  if (!clean) {
    setHint('deviceHostnameMsg', 'hint err', 'กรุณาระบุภาษาอังกฤษ (a-z, 0-9)');
    return;
  }
  setHint('deviceHostnameMsg', 'hint', 'กำลังบันทึก...');
  try {
    const res = await api('/api/settings', {
      method: 'PUT',
      body: { hostname: clean }
    });
    const finalHost = res.hostname || clean;
    if (res.defaultHostname) window.gDefaultHostname = res.defaultHostname;
    updateDeviceHostnameLink(finalHost);
    setHint('deviceHostnameMsg', 'hint ok', 'บันทึกสำเร็จ');
    const wifiLink = $('wifiLocalUrlLink');
    if (wifiLink) {
      wifiLink.href = 'http://' + finalHost + '.local';
      wifiLink.textContent = 'http://' + finalHost + '.local';
    }
  } catch (e) {
    setHint('deviceHostnameMsg', 'hint err', 'บันทึกไม่สำเร็จ');
  }
}

async function resetHostname() {
  setHint('deviceHostnameMsg', 'hint', 'กำลังคืนค่าเดิม...');
  try {
    const res = await api('/api/settings', {
      method: 'PUT',
      body: { hostname: '' }
    });
    const def = res.defaultHostname || res.hostname || window.gDefaultHostname || '';
    if (res.defaultHostname) window.gDefaultHostname = res.defaultHostname;
    if ($('deviceHostname')) $('deviceHostname').value = '';
    updateDeviceHostnameLink(def);
    setHint('deviceHostnameMsg', 'hint ok', 'คืนค่าเดิมสำเร็จ');
    const wifiLink = $('wifiLocalUrlLink');
    if (wifiLink) {
      wifiLink.href = 'http://' + def + '.local';
      wifiLink.textContent = 'http://' + def + '.local';
    }
  } catch (e) {
    setHint('deviceHostnameMsg', 'hint err', 'คืนค่าไม่สำเร็จ');
  }
}

document.addEventListener('input', (e) => {
  if (e.target && e.target.id === 'deviceHostname') {
    const val = e.target.value;
    const hasInvalid = /[^a-zA-Z0-9-]/.test(val);
    if (hasInvalid) {
      setHint('deviceHostnameMsg', 'hint err', 'ใช้เฉพาะภาษาอังกฤษ (a-z, 0-9, -)');
    } else {
      setHint('deviceHostnameMsg', '', '');
    }
    const clean = cleanHostnameInput(val);
    updateDeviceHostnameLink(clean || window.gDefaultHostname || '');
  }
});

document.addEventListener('change', (e) => {
  if (e.target && e.target.id === 'deviceHostname') {
    e.target.value = cleanHostnameInput(e.target.value);
    updateDeviceHostnameLink(e.target.value || window.gDefaultHostname || '');
  }
});

async function openAutoLink() {
  const name = $('deviceName')?.value?.trim() || 'AutoDoor';
  setHint('deviceNameMsg', 'hint', 'กำลังโหลดข้อมูล WebHook...');
  try {
    const wh = await api('/api/webhook');
    
    // Must use Blynk webhooks to avoid Mixed Content (HTTP vs HTTPS) blocks in the browser
    if (!wh.blynk || !wh.blynk.open) {
      setHint('deviceNameMsg', 'hint err', 'กรุณาตั้งค่าและเปิดใช้งาน Blynk ก่อนใช้งานควบคุมผ่านหน้าเว็บ');
      return;
    }

    const key = wh.blynkToken || wh.key || '';
    const openUrl = wh.blynk.open || '';
    const stopUrl = wh.blynk.stop || '';
    const closeUrl = wh.blynk.close || '';
    
    // Blynk V3 pin: value=0 for lock (disable auto), value=1 for unlock (enable auto)
    const lockOnUrl = wh.blynk.tagAutoToggle ? wh.blynk.tagAutoToggle.replace('&V3=1', '&V3=0') : '';
    const lockOffUrl = wh.blynk.tagAutoToggle ? wh.blynk.tagAutoToggle.replace('&V3=1', '&V3=1') : '';

    const alUrl = new URL('https://autolinkpromax.github.io/autolink/index.html');
    
    alUrl.searchParams.set('mode', 'webhook');
    alUrl.searchParams.set('id', key || 'blynk_' + Date.now());
    alUrl.searchParams.set('name', name);
    alUrl.searchParams.set('wOpen', openUrl);
    alUrl.searchParams.set('wStop', stopUrl);
    alUrl.searchParams.set('wClose', closeUrl);
    alUrl.searchParams.set('wLockOn', lockOnUrl);
    alUrl.searchParams.set('wLockOff', lockOffUrl);

    setHint('deviceNameMsg', 'hint ok', 'เปิดหน้าเว็บควบคุม...');
    window.open(alUrl.toString(), '_blank', 'noopener');
  } catch (e) {
    setHint('deviceNameMsg', 'hint err', 'โหลดข้อมูลล้มเหลว: ' + e.message);
  }
}
// 21-analytics.js - Client-Side History Analytics & Access Entry Logs Engine
(function() {
  State.historyAllCycles = [];
  State.historyCycles = [];
  State.historyKpi = {};
  State.historyRawEvents = [];
  State.historyViewMode = 'cycles'; // 'cycles' or 'raw'
  State.historyVehicleFilter = 'all';

  // Helper to ensure names are always clean without (id) or parentheses
  function getCleanName(entity) {
    if (!entity || typeof entity !== 'string') return '—';
    return typeof cleanEntityName === 'function' ? cleanEntityName(entity) : entity.replace(/\s*\([0-9a-fA-F:_-]+\)$/, '').trim();
  }

  // Helper to resolve icon, theme, and labels for an event
  function resolveEventMeta(ev) {
    let configuredIcon = '';
    const tags = State.lastLiveData?.tags || [];
    const tagMatch = tags.find(t => t.name === ev.msg || t.id === ev.entity || t.name === ev.entity);
    if (tagMatch && tagMatch.icon && tagMatch.icon !== 'auto') {
      configuredIcon = tagMatch.icon.toLowerCase();
    }

    const typeKey = (configuredIcon || ev.type || '').toLowerCase();
    const reason = ev.reason || '';
    const reasonLower = reason.toLowerCase();
    const msg = ev.msg || '';
    const msgLower = msg.toLowerCase();

    let emoji = '🏷️';
    let themeClass = 'access-theme-other';
    let channelLabel = reason || 'ทั่วไป';
    let isHold = false;
    let isRf = false;
    let isTag = false;
    let isWeb = false;
    let isApp = false;

    if (typeKey === 'rf' || typeKey.includes('remote') || typeKey.includes('key') || reasonLower.includes('รีโมท') || msgLower.includes('รีโมท')) {
      isRf = true;
      emoji = '🔑';
      themeClass = 'access-theme-rf';
      channelLabel = 'รีโมท RF (433MHz)';
      if (typeKey === 'rf_hold' || reasonLower.includes('hold') || msgLower.includes('ค้าง')) {
        isHold = true;
      }
    } else if (typeKey === 'web' || reasonLower.includes('เว็บ') || msgLower.includes('web')) {
      isWeb = true;
      emoji = '💻';
      themeClass = 'access-theme-web';
      channelLabel = 'เว็บคอนโซล (Web UI)';
    } else if (typeKey === 'app' || reasonLower.includes('แอป') || msgLower.includes('blynk')) {
      isApp = true;
      emoji = '📱';
      themeClass = 'access-theme-app';
      channelLabel = 'แอปมือถือ (Blynk)';
    } else if (typeKey === 'btn' || reasonLower.includes('สวิตช์') || reasonLower.includes('ปุ่ม')) {
      emoji = '🔘';
      themeClass = 'access-theme-other';
      channelLabel = 'ปุ่มกดหน้าตู้';
    } else if (typeKey === 'ext' || reasonLower.includes('ภายนอก') || reasonLower.includes('เซ็นเซอร์')) {
      emoji = '⚡';
      themeClass = 'access-theme-other';
      channelLabel = 'เซ็นเซอร์ภายนอก';
    } else if (typeKey === 'schedule' || reasonLower.includes('ตารางเวลา')) {
      emoji = '⏰';
      themeClass = 'access-theme-other';
      channelLabel = 'ตั้งเวลาอัตโนมัติ';
    } else {
      // Tags (ESP-NOW)
      isTag = true;
      themeClass = 'access-theme-car';
      if (typeKey === 'suv' || reasonLower.includes('suv')) {
        emoji = '🚙';
        channelLabel = 'รถยนต์ SUV (ESP-NOW)';
      } else if (typeKey === 'sedan' || reasonLower.includes('เก๋ง')) {
        emoji = '🚗';
        channelLabel = 'รถเก๋ง (ESP-NOW)';
      } else if (typeKey === 'pickup' || reasonLower.includes('กระบะ') || typeKey.includes('truck')) {
        emoji = '🚚';
        channelLabel = 'รถกระบะ (ESP-NOW)';
      } else if (typeKey === 'bike' || reasonLower.includes('มอเตอร์ไซค์') || reasonLower.includes('มอไซ')) {
        emoji = '🏍️';
        themeClass = 'access-theme-bike';
        channelLabel = 'รถมอเตอร์ไซค์ (ESP-NOW)';
      } else if (typeKey === 'scooter' || reasonLower.includes('สกู๊ตเตอร์')) {
        emoji = '🛵';
        themeClass = 'access-theme-bike';
        channelLabel = 'รถสกู๊ตเตอร์ (ESP-NOW)';
      } else if (typeKey === 'van' || reasonLower.includes('รถตู้')) {
        emoji = '🚐';
        channelLabel = 'รถตู้ (ESP-NOW)';
      } else if (typeKey === 'bicycle' || reasonLower.includes('จักรยาน')) {
        emoji = '🚲';
        themeClass = 'access-theme-bike';
        channelLabel = 'รถจักรยาน (ESP-NOW)';
      } else if (typeKey === 'person' || reasonLower.includes('คนเดิน') || reasonLower.includes('บุคคล')) {
        emoji = '🚶';
        themeClass = 'access-theme-person';
        channelLabel = 'คนเดิน (ESP-NOW)';
      } else {
        emoji = typeof getVehicleEmoji === 'function' ? getVehicleEmoji(typeKey, false) : '🚗';
        channelLabel = 'แท็กยานพาหนะ (ESP-NOW)';
      }
    }

    return { typeKey, emoji, themeClass, channelLabel, reason: reason || channelLabel, isHold, isRf, isTag, isWeb, isApp };
  }

  // Reconstruct raw events into discrete access entry records with timeline steps
  function reconstructDoorCycles(events) {
    if (!events || !events.length) return [];

    // Clone and sort chronologically (oldest first)
    const sorted = [...events].reverse();
    const cycles = [];
    let curCycle = null;
    let lastCompletedCycle = null;

    sorted.forEach((ev) => {
      const type = (ev.type || '').toLowerCase();
      const msg = ev.msg || '';
      // Non-access legacy diagnostic events
      const isLegacyDiag = ['beam_cut', 'pass_arm', 'reversal', 'safe_close', 'auto_close', 'fall_close', 'mag_close', 'state_close', 'limit_close', 'limit_open'].includes(type);
      const isAccessOrOpen = !isLegacyDiag;
      const isComplete = type === 'mag_close' || (type === 'state_close' && msg.includes('ปิดสนิท'));

      // If we see an access or open trigger
      if (isAccessOrOpen) {
        let rawEntity = ev.entity || ev.msg || '—';
        if (rawEntity === '—' && msg) {
          const m = msg.match(/\[(.*?)\]\s*(.*?)\s*(เข้าประตู|สั่งเปิด)/);
          if (m && m[2]) rawEntity = m[2].trim();
        }
        let entity = getCleanName(rawEntity);

        // Ignore orphan / spurious manual opens where no entity was provided
        if (type === 'manual_open' && (rawEntity === '—' || rawEntity.includes('ไม่ทราบ')) && (!msg || msg.includes('ไม่ทราบ'))) {
          return;
        }

        const meta = resolveEventMeta(ev);

        // Finalize previous cycle if one existed
        if (curCycle) {
          curCycle.completed = true;
          if (curCycle.status === 'running') curCycle.status = 'success';
          cycles.push(curCycle);
          lastCompletedCycle = curCycle;
        }

        const actionTitle = meta.isHold
          ? 'สั่งเปิดค้างต่อเนื่อง (Hold Open)'
          : (meta.isTag
              ? 'ยานพาหนะเข้าประตู (Auto)'
              : (meta.isRf
                  ? 'กดรีโมทสั่งเปิดประตู'
                  : (meta.isWeb
                      ? 'สั่งเปิดประตูจากเว็บ'
                      : (meta.isApp ? 'สั่งเปิดประตูจากแอปมือถือ' : 'สั่งเปิดประตูอัตโนมัติ'))));

        const actionDesc = meta.isTag
          ? `${meta.emoji} ตรวจพบแท็ก [${entity}] (${meta.reason}) เข้าใกล้ประตู ➔ เปิดอัตโนมัติ`
          : (meta.isRf
              ? (meta.isHold ? `🔒 กดรีโมท [${entity}] ค้าง 4 วิ ➔ ล็อกเปิดค้างต่อเนื่อง` : `🔑 กดรีโมท [${entity}] ➔ สั่งเปิดประตูอัตโนมัติ`)
              : (meta.isWeb
                  ? `💻 สั่งเปิดประตูจากเว็บคอนโซล [${entity}]`
                  : (meta.isApp ? `📱 สั่งเปิดประตูจากแอปมือถือ [${entity}]` : `${meta.emoji} สั่งเปิดประตู [${entity}]`)));

        curCycle = {
          id: cycles.length + 1,
          startTime: ev.time || '—',
          endTime: ev.time || '—',
          triggerEvent: ev,
          triggerEntity: entity,
          triggerType: meta.typeKey,
          emoji: meta.emoji,
          themeClass: meta.themeClass,
          channelLabel: meta.channelLabel,
          reason: meta.reason,
          actionTitle: actionTitle,
          actionDesc: actionDesc,
          isHold: meta.isHold,
          isRf: meta.isRf,
          isTag: meta.isTag,
          isWeb: meta.isWeb,
          isApp: meta.isApp,
          triggerRssi: (typeof ev.metric === 'number' && ev.metric < 0) ? ev.metric : null,
          beamCutTime: null,
          passArmDelayMs: null,
          autoCloseTime: null,
          doubleCloseTime: null,
          runDurationMs: ev.durationMs || null,
          completed: true, // In Access Entry paradigm, all authorized triggers are successful entries
          closingStarted: false,
          status: 'success',
          notes: [],
          events: [ev],
          steps: [
            {
              name: actionTitle,
              time: ev.time || '',
              desc: actionDesc,
              dot: meta.isHold ? 'warn' : 'ok'
            }
          ]
        };
        return;
      }

      // If legacy diagnostic events arrive for this cycle (from older full logs)
      if (!curCycle) {
        if (isComplete && lastCompletedCycle) {
          lastCompletedCycle.events.push(ev);
          if (ev.durationMs && ev.durationMs > 0 && !lastCompletedCycle.runDurationMs) {
            lastCompletedCycle.runDurationMs = ev.durationMs;
          }
        }
        return;
      }

      curCycle.events.push(ev);

      if (type === 'beam_cut') {
        curCycle.beamCutTime = ev.time;
        curCycle.steps.push({
          name: 'ยานพาหนะตัดผ่านลำแสงกันหนีบ',
          time: ev.time || '',
          desc: `เซ็นเซอร์กันหนีบทำงาน (สถานะ: ${(ev.sensorBits & 1) ? 'ตัดแสง' : 'ปกติ'})`,
          dot: 'ok'
        });
      } else if (type === 'pass_arm') {
        curCycle.passArmDelayMs = ev.durationMs;
        curCycle.steps.push({
          name: 'พ้นลำแสงกันหนีบ',
          time: ev.time || '',
          desc: `ลำแสงกลับมาโล่ง ➔ หน่วงเวลานับถอยหลังปิด ${((ev.durationMs || 0) / 1000).toFixed(1)}s`,
          dot: 'ok'
        });
      } else if (type === 'reversal') {
        curCycle.steps.push({
          name: 'สลับทิศทางเตรียมปิด (Safety Deadband)',
          time: ev.time || '',
          desc: 'หยุดชะลอมอเตอร์ก่อนสลับทิศทางเพื่อความปลอดภัย',
          dot: 'pri'
        });
      } else if (type === 'safe_close' || type === 'auto_close' || type === 'fall_close' || type === 'manual_close') {
        curCycle.closingStarted = true;
        if (!curCycle.autoCloseTime) {
          curCycle.autoCloseTime = ev.time;
          const isFall = type === 'fall_close';
          const isManual = type === 'manual_close';
          curCycle.steps.push({
            name: isManual ? 'สั่งปิดประตูด้วยมือ' : (isFall ? 'ระบบสั่งปิดอัตโนมัติ (Fallback)' : 'ระบบสั่งปิดประตูอัตโนมัติ'),
            time: ev.time || '',
            desc: 'ส่งพัลส์รีเลย์ปิด (GPIO 7 = HIGH 500ms)',
            dot: 'pri'
          });
        }
      } else if (isComplete) {
        curCycle.endTime = ev.time || '—';
        curCycle.completed = true;
        curCycle.status = 'success';
        if (ev.durationMs && ev.durationMs > 0) {
          curCycle.runDurationMs = ev.durationMs;
        }
        curCycle.steps.push({
          name: 'ประตูปิดสนิทสมบูรณ์',
          time: ev.time || '',
          desc: `แม่เหล็กดูดติดสนิท${curCycle.runDurationMs ? ' (เวลาวิ่งปิด ' + (curCycle.runDurationMs / 1000).toFixed(1) + 's)' : ''}`,
          dot: 'ok'
        });
        cycles.push(curCycle);
        lastCompletedCycle = curCycle;
        curCycle = null;
      }
    });

    if (curCycle) {
      cycles.push(curCycle);
    }

    // Renumber IDs sequentially 1..N
    cycles.forEach((c, idx) => {
      c.id = idx + 1;
    });

    // Return in reverse order (newest entry first for display)
    return cycles.reverse();
  }

  function calculateAnalyticsKpi(cycles, rawEvents) {
    const total = cycles.length;
    let completedCount = 0;
    let holdCount = 0;
    const vehicleMap = {};
    const methodCounts = { espNow: 0, rf: 0, web: 0, app: 0, other: 0 };
    const anomalies = [];

    cycles.forEach((c) => {
      if (c.completed) completedCount++;
      if (c.isHold) holdCount++;

      const ent = getCleanName(c.triggerEntity);
      if (ent && ent !== '—') {
        vehicleMap[ent] = (vehicleMap[ent] || 0) + 1;
      }

      if (c.isTag) methodCounts.espNow++;
      else if (c.isRf) methodCounts.rf++;
      else if (c.isWeb) methodCounts.web++;
      else if (c.isApp) methodCounts.app++;
      else methodCounts.other++;
    });

    // Check raw events for true error anomalies only
    (rawEvents || []).forEach((ev) => {
      const type = ev.type || '';
      const msg = ev.msg || '';
      if (ev.level === 'error' || ev.level === 'danger') {
        anomalies.push(`[${ev.time}] ข้อผิดพลาดระบบ: ${msg || type}`);
      }
    });

    // Determine top access method
    let topMethod = '—';
    let topCount = 0;
    if (methodCounts.espNow > topCount) { topMethod = 'ESP-NOW'; topCount = methodCounts.espNow; }
    if (methodCounts.rf > topCount) { topMethod = 'รีโมท RF'; topCount = methodCounts.rf; }
    if (methodCounts.web > topCount) { topMethod = 'Web UI'; topCount = methodCounts.web; }
    if (methodCounts.app > topCount) { topMethod = 'แอปมือถือ'; topCount = methodCounts.app; }

    const latestCycle = cycles.length > 0 ? cycles[0] : null;

    let methodBreakdown = `ESP-NOW ${methodCounts.espNow} | RF ${methodCounts.rf} | Web ${methodCounts.web}`;
    if (methodCounts.app > 0) methodBreakdown += ` | App ${methodCounts.app}`;
    if (methodCounts.other > 0) methodBreakdown += ` | อื่นๆ ${methodCounts.other}`;

    return {
      totalCycles: total,
      completedCycles: completedCount,
      successRate: 100,
      latestTime: latestCycle ? latestCycle.startTime : '—',
      latestPerson: latestCycle ? getCleanName(latestCycle.triggerEntity) : '—',
      topMethod: topMethod,
      methodBreakdown: methodBreakdown,
      vehicleMap,
      anomalies: [...new Set(anomalies)],
    };
  }

  // Populate the Vehicle / User Filter dropdown
  function populateHistoryVehicleFilter(allCycles) {
    const vSel = $('historyVehicleFilter');
    if (!vSel) return;

    const counts = {};
    const iconMap = {};
    allCycles.forEach(c => {
      const ent = getCleanName(c.triggerEntity) || 'ไม่ระบุ';
      counts[ent] = (counts[ent] || 0) + 1;
      if (!iconMap[ent]) {
        iconMap[ent] = c.emoji || (typeof getVehicleEmoji === 'function' ? getVehicleEmoji(c.triggerType, c.isRf) : '👤');
      }
    });

    const currentVal = State.historyVehicleFilter || 'all';
    let html = `<option value="all">👥 ทั้งหมด (ทุกคน) (${allCycles.length})</option>`;
    Object.keys(counts).sort().forEach(ent => {
      const ico = iconMap[ent] || '👤';
      html += `<option value="${escHtml(ent)}">${ico} ${escHtml(ent)} (${counts[ent]})</option>`;
    });

    vSel.innerHTML = html;
    if (Array.from(vSel.options).some(o => o.value === currentVal)) {
      vSel.value = currentVal;
    } else {
      vSel.value = 'all';
      State.historyVehicleFilter = 'all';
    }

    if (!vSel._boundChange) {
      vSel._boundChange = true;
      vSel.addEventListener('change', () => {
        applyHistoryVehicleFilter();
      });
    }
  }

  function applyHistoryVehicleFilter() {
    const vSel = $('historyVehicleFilter');
    const filter = vSel ? vSel.value : 'all';
    State.historyVehicleFilter = filter;

    const allCycles = State.historyAllCycles || [];
    const filtered = (filter === 'all')
      ? allCycles
      : allCycles.filter(c => getCleanName(c.triggerEntity) === filter);

    State.historyCycles = filtered;
    const kpi = calculateAnalyticsKpi(filtered, State.historyRawEvents);
    State.historyKpi = kpi;

    renderHistoryDashboard(filtered, kpi, State.historyRawEvents);
  }

  // Render Dashboard KPI Summary & Entry Cards
  function renderHistoryDashboard(cycles, kpi, rawEvents) {
    // 1. KPI Summary Bar (Total Access Entries)
    const elCycles = $('historyKpiCycles');
    if (elCycles) elCycles.textContent = `${kpi.totalCycles} ครั้ง`;

    const elSuccess = $('historyKpiSuccessRate');
    if (elSuccess) {
      elSuccess.textContent = `บันทึกเรียบร้อย 100% (${kpi.completedCycles}/${kpi.totalCycles})`;
      elSuccess.style.color = 'var(--ok)';
    }

    // 2. Anomaly Alerts Banner
    const alertBanner = $('historyAlertBanner');
    const alertContent = $('historyAlertContent');
    if (alertBanner && alertContent) {
      if (kpi.anomalies.length > 0) {
        alertBanner.classList.remove('hidden');
        alertContent.innerHTML = kpi.anomalies.map(a => `• ${escHtml(a)}`).join('<br/>');
      } else {
        alertBanner.classList.add('hidden');
      }
    }

    // 3. Render Entry Cards
    const cycleContainer = $('historyCycleContainer');
    if (cycleContainer) {
      if (!cycles.length) {
        cycleContainer.innerHTML = '<div style="color:var(--muted);font-size:12px;text-align:center;padding:16px;background:rgba(255,255,255,0.02);border:1px solid var(--line);border-radius:10px">ไม่พบข้อมูลบันทึกการเข้าประตูตามเงื่อนไขที่เลือก</div>';
      } else {
        cycleContainer.innerHTML = cycles.map(c => {
          let cleanName = getCleanName(c.triggerEntity);
          if (c.isWeb || cleanName.includes('#255')) {
            cleanName = 'Web Console';
          }
          const typeIcon = c.emoji || (typeof getVehicleEmoji === 'function' ? getVehicleEmoji(c.triggerType, c.isRf) : '🏷️');
          
          let statusBadge = '';
          if (c.isHold) {
            statusBadge = '<span class="badgeMini" style="background:rgba(245,158,11,0.15);color:#f59e0b;border:1px solid rgba(245,158,11,0.3)">🔒 เปิดค้าง (Hold)</span>';
          } else if (c.isWeb) {
            statusBadge = '<span class="badgeMini" style="background:rgba(59,130,246,0.15);color:#60a5fa;border:1px solid rgba(59,130,246,0.3)">💻 Web Console</span>';
          } else if (c.isApp) {
            statusBadge = '<span class="badgeMini" style="background:rgba(168,85,247,0.15);color:#c084fc;border:1px solid rgba(168,85,247,0.3)">📱 Blynk App</span>';
          } else if (c.isRf) {
            statusBadge = '<span class="badgeMini" style="background:rgba(245,158,11,0.15);color:#fbbf24;border:1px solid rgba(245,158,11,0.3)">🔑 รีโมท RF</span>';
          } else if (c.isTag) {
            statusBadge = '<span class="badgeMini" style="background:rgba(62,207,142,0.15);color:#3ecf8e;border:1px solid rgba(62,207,142,0.3)">✓ เข้าประตู (Auto)</span>';
          } else {
            statusBadge = '<span class="badgeMini" style="background:rgba(62,207,142,0.15);color:#3ecf8e;border:1px solid rgba(62,207,142,0.3)">✓ เปิดสำเร็จ</span>';
          }

          const rssiBadge = c.triggerRssi
            ? `<span style="font-family:monospace;font-size:10px;background:rgba(255,255,255,0.06);padding:1px 6px;border-radius:4px;color:#60a5fa">📶 ${c.triggerRssi} dBm</span>`
            : '';

          const durBadge = c.runDurationMs
            ? `<span style="font-family:monospace;font-size:10px;background:rgba(255,255,255,0.06);padding:1px 6px;border-radius:4px;color:#cbd5e1">⏱ ${(c.runDurationMs / 1000).toFixed(1)}s</span>`
            : '';

          const stepsHtml = (c.steps || []).map(s => `
            <div class="anlStep">
              <div class="anlStepDot ${s.dot || 'ok'}"></div>
              <div class="anlStepHead">
                <span class="anlStepName">${escHtml(s.name)}</span>
                <span class="anlStepTime">${escHtml(s.time)} น.</span>
              </div>
              <div class="anlStepDesc">${escHtml(s.desc)}</div>
            </div>
          `).join('');

          const timeSuffix = c.startTime && c.startTime !== '—' ? ` ${escHtml(c.startTime)} น.` : '';

          return `
            <div class="anlCycleCard" id="historyCycleCard-${c.id}">
              <div class="anlCycleHead">
                <div class="anlCycleTitle">
                  <span style="font-size:18px">${typeIcon}</span>
                  <span>รายการที่ ${c.id}: ${escHtml(cleanName)}${timeSuffix}</span>
                </div>
                <div class="anlCycleMeta">
                  ${rssiBadge}
                  ${durBadge}
                  ${statusBadge}
                </div>
              </div>
              <div class="anlCycleBody">
                <div class="anlTimeline">
                  ${stepsHtml}
                </div>
              </div>
              <div class="anlCycleFoot">
                <span>เวลา: <b>${escHtml(c.startTime)} น.</b></span>
                <span>ประเภท / ช่องทาง: <b>${escHtml(c.reason || c.channelLabel || 'แท็ก / รีโมท')}</b></span>
              </div>
            </div>
          `;
        }).join('');
      }
    }

    // 4. Render Raw List
    const rawList = $('historyEventList');
    if (rawList && typeof formatEventItemHtml === 'function') {
      if (!rawEvents || !rawEvents.length) {
        rawList.innerHTML = '<div style="color:var(--muted);font-size:12px;text-align:center;padding:16px">ไม่มีข้อมูล Log ดิบ</div>';
      } else {
        rawList.innerHTML = rawEvents.map((ev, idx) => formatEventItemHtml(ev, idx, true)).join('');
      }
    }
  }

  function renderHistoryAnalytics(events) {
    State.historyRawEvents = events || [];
    const allCycles = reconstructDoorCycles(events || []);
    State.historyAllCycles = allCycles;

    populateHistoryVehicleFilter(allCycles);

    const filter = State.historyVehicleFilter || 'all';
    const filtered = (filter === 'all')
      ? allCycles
      : allCycles.filter(c => getCleanName(c.triggerEntity) === filter);
    State.historyCycles = filtered;

    const kpi = calculateAnalyticsKpi(filtered, events);
    State.historyKpi = kpi;

    renderHistoryDashboard(filtered, kpi, events);
  }

  function toggleHistoryView() {
    const cycleBox = $('historyCycleContainer');
    const rawBox = $('historyEventList');
    const btn = $('btnToggleHistoryView');
    if (!cycleBox || !rawBox) return;

    if (State.historyViewMode === 'raw') {
      State.historyViewMode = 'cycles';
      rawBox.classList.add('hidden');
      cycleBox.classList.remove('hidden');
      if (btn) btn.innerHTML = '<span>📜 สลับดู Log ดิบ</span>';
    } else {
      State.historyViewMode = 'raw';
      cycleBox.classList.add('hidden');
      rawBox.classList.remove('hidden');
      if (btn) btn.innerHTML = '<span>🚪 ดูแบบรายการเข้าประตู</span>';
    }
  }

  async function copyHistorySummary() {
    const kpi = State.historyKpi;
    const cycles = State.historyCycles || [];
    const sel = $('historyDateSelect');
    const sourceLabel = sel && sel.options[sel.selectedIndex] ? sel.options[sel.selectedIndex].text : 'ข้อมูลประวัติ';
    const vSel = $('historyVehicleFilter');
    const vehLabel = vSel && vSel.value !== 'all' ? ` | ฟิลเตอร์: ${vSel.value}` : '';

    if (!cycles.length) {
      if (typeof copyHistoryEvents === 'function') {
        copyHistoryEvents();
      }
      return;
    }

    const lines = [
      `=== บันทึกประวัติการเข้าประตู AutoDoor-RF2 (Access Entry Logs) ===`,
      `วันที่ / แหล่งข้อมูล: ${sourceLabel}${vehLabel}`,
      `บันทึกทั้งหมด: ${kpi.totalCycles || 0} ครั้ง (สำเร็จ 100%)`,
      `เข้าล่าสุด: ${kpi.latestTime || '—'} น. (${kpi.latestPerson || '—'})`,
      `ช่องทางหลัก: ${kpi.topMethod || '—'} (${kpi.methodBreakdown || '—'})`,
      ``,
      `--- รายการเข้าประตู ---`,
      ...cycles.map(c => {
        let cleanName = getCleanName(c.triggerEntity);
        if (c.isWeb || cleanName.includes('#255')) cleanName = 'Web Console';
        const mode = c.isHold ? 'เปิดค้าง (Hold Open)' : 'เข้าประตู (Auto)';
        const ico = c.emoji || (typeof getVehicleEmoji === 'function' ? getVehicleEmoji(c.triggerType, c.isRf) : '🏷️');
        return `[รายการที่ ${c.id}] ${ico} ${cleanName} ${c.startTime} น. | ${c.reason || c.channelLabel || '—'} | ${mode}`;
      })
    ];

    if (kpi.anomalies && kpi.anomalies.length) {
      lines.push(``, `--- ข้อสังเกต ---`);
      kpi.anomalies.forEach(a => lines.push(`• ${a}`));
    }

    const text = lines.join('\n');
    try {
      if (navigator.clipboard?.writeText) {
        await navigator.clipboard.writeText(text);
      } else {
        const ta = document.createElement('textarea');
        ta.value = text;
        ta.style.position = 'fixed';
        ta.style.left = '-9999px';
        document.body.appendChild(ta);
        ta.select();
        document.execCommand('copy');
        document.body.removeChild(ta);
      }
      const btn = $('historyCopyText');
      if (btn) {
        btn.textContent = '✓ คัดลอกแล้ว';
        setTimeout(() => { btn.textContent = 'คัดลอกสรุป'; }, 2000);
      }
    } catch (e) {
      console.error('Copy history summary error:', e);
    }
  }

  window.renderHistoryAnalytics = renderHistoryAnalytics;
  window.toggleHistoryView = toggleHistoryView;
  window.copyHistorySummary = copyHistorySummary;
})();
initActions();

window.checkWebLogin = checkWebLogin;
window.showView = showView;
window.startLivePoll = startLivePoll;
