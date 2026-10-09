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
