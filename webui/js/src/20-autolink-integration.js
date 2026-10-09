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

