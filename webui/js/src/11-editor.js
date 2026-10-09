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
