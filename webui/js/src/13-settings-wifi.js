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
