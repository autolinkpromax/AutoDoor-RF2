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
    case 'copy-hook':
      copyHookUrl(el.dataset.slot);
      break;
    case 'copy-blynk-hook':
      copyBlynkHookUrl(el.dataset.slot);
      break;
    case 'copy-ha-lan':
      copyHaLanYaml();
      break;
    case 'copy-ha-blynk':
      copyHaBlynkYaml();
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
