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
    const res = await api('/api/webhook/test', { method: 'POST' });
    if (res.ok) {
      setHint('whOutMsg', 'hint ok', 'ส่งข้อความทดสอบเข้าคิวแล้ว');
    } else {
      setHint('whOutMsg', 'hint err', 'ส่งไม่สำเร็จ: ' + (res.msg || 'error'));
    }
  } catch (e) {
    setHint('whOutMsg', 'hint err', 'เกิดข้อผิดพลาด: ' + e.message);
  }
}
