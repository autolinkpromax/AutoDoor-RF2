async function loadWebAuthUi() {
  try {
    const r = await api('/api/webauth');
    const hint = $('webAuthHint');
    if (hint) {
      hint.textContent = r.hasPassword ? '🔒 เปิดใช้รหัสผ่านแล้ว' : '🔓 ยังไม่ได้ตั้งรหัสผ่าน';
    }
    if ($('webPass')) $('webPass').value = '';
    if ($('webAuthMsg')) $('webAuthMsg').textContent = '';
    const authBadge = $('webAuthHeadBadge');
    if (authBadge) {
      if (r.hasPassword) {
        authBadge.textContent = 'มีรหัสผ่าน';
        authBadge.className = 'badgeMini ok';
      } else {
        authBadge.textContent = 'ไม่มีรหัส';
        authBadge.className = 'badgeMini off';
      }
    }
    updateSafeModeBanner(!!r.safeMode);
  } catch (_) {}
}

async function saveWebAuth() {
  const pass = $('webPass')?.value ?? '';
  if (!pass && !confirm('ปิดรหัสผ่านหน้าเว็บ?')) return;
  if ($('webAuthMsg')) $('webAuthMsg').textContent = 'บันทึก…';
  try {
    const r = await api('/api/webauth', { method: 'POST', body: { password: pass } });
    setHint(
      'webAuthMsg',
      'hint ok',
      r.hasPassword ? 'ตั้งรหัสผ่านแล้ว' : 'ปิดรหัสผ่านแล้ว'
    );
    if ($('webPass')) $('webPass').value = '';
    loadWebAuthUi();
  } catch (e) {
    setHint(
      'webAuthMsg',
      'hint err',
      e.message === 'safe_mode' ? 'อยู่ใน Safe Mode — ตั้งรหัสไม่ได้' : 'บันทึกไม่สำเร็จ'
    );
  }
}
