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
