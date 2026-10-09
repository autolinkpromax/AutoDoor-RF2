async function refreshTagList() {
  const d = await api('/api/tags');
  const tags = d.tags || [];
  State.tagCount = tags.length;
  renderTagList(tags);
}

function deviceListKey(tags) {
  return tags.map((t) => `${t.id}:${t.name}:${t.icon || ''}:${t.active ? 1 : 0}:${t.tagOpenDurationMs}:${t.tagCloseDelayMs}:${t.tagSafetyCloseDelayMs}:${t.tagHoldMs}:${t.rfRole || 0}:${t.scheduleEnabled ? 1 : 0}:${t.scheduleStartMin}:${t.scheduleEndMin}`).join('|');
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

  if (t.active) {
    const remMs = (typeof t.holdRemainingMs === 'number' && t.holdRemainingMs > 0)
      ? t.holdRemainingMs
      : (isRf ? 1000 : (t.tagHoldMs || 2000));
    const remSec = Math.max(0.1, (remMs / 1000)).toFixed(1);
    return isRf
      ? `<span style="color:#10b981;font-weight:600;">🟢 ส่งสัญญาณ (ทำงานอีก ${remSec} วิ)</span>${schedBadge}`
      : `<span style="color:#10b981;font-weight:600;">🟢 กำลังทำงาน (จะหายใน ${remSec} วิ)</span>${schedBadge}`;
  }

  const openPrefix = openSec > 0 ? `เปิดช่อง ${openSec} วิ · รอ ${closeDelaySec} วิ` : `หน่วงปิด ${closeDelaySec} วิ`;
  if (isRf) {
    if (t.rfRole === 1) return `<span style="color:#fbbf24;font-weight:500;">🟡 หน้าที่: เปิดค้าง (Hold Open)</span>${schedBadge}`;
    if (t.rfRole === 2) return `<span style="color:#f87171;font-weight:500;">🔴 หน้าที่: สั่งปิดประตูทันที</span>${schedBadge}`;
    if (t.rfRole === 3) return `<span style="color:#9ca3af;font-weight:500;">⏹️ หน้าที่: สั่งหยุดมอเตอร์</span>${schedBadge}`;
    if (t.rfRole === 4) return `${openPrefix} · กันหนีบ ${safetyCloseSec} วิ · <span style="color:#10b981;font-weight:500;">🟢 เปิดอย่างเดียว (Auto/Open)</span>${schedBadge}`;
    return `${openPrefix} · กันหนีบ ${safetyCloseSec} วิ · <span style="color:#34d399;">🟢 เปิดออโต้ (Auto/Toggle)</span>${schedBadge}`;
  }
  return `${openPrefix} · กันหนีบ ${safetyCloseSec} วิ · หายใน ${holdSec} วิ${schedBadge}`;
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
