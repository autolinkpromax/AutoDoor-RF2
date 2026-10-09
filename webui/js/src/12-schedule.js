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
