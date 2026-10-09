const Dom = {
  _c: {},
  get(id) {
    const el = this._c[id];
    if (el && el.isConnected) return el;
    const found = document.getElementById(id);
    if (found) this._c[id] = found;
    return found;
  },
  clear() {
    this._c = {};
  },
};

function $(id) {
  return Dom.get(id);
}

function escHtml(s) {
  return String(s)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/"/g, '&quot;');
}

function cleanEntityName(s) {
  if (!s || typeof s !== 'string') return '—';
  let cleaned = s.trim();
  if (cleaned === 'REMOTE') return 'รีโมทประตู';
  // Strip trailing (id) like (72c8c6fa) or (aabbccddeeff) or (0x1234)
  cleaned = cleaned.replace(/\s*\([0-9a-fA-F:_-]+\)$/, '').trim();
  return cleaned || '—';
}

function setHint(id, cls, text) {
  const el = $(id);
  if (!el) return;
  if (cls) el.className = cls;
  if (text != null) el.textContent = text;
}
