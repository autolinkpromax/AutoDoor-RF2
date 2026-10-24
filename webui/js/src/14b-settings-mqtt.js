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
