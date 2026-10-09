#include "http_api.h"

#include "app/types.h"
#include "modules/door.h"
#include "modules/tag_store.h"

namespace {

HttpApi::LearnStartFn gLearnStartFn = nullptr;
HttpApi::LearnStopFn gLearnStopFn = nullptr;
HttpApi::LearnStatusFn gLearnStatusFn = nullptr;
HttpApi::AutoChannelFn gAutoChannelFn = nullptr;

const char* kIndexHtml = R"HTML(
<!doctype html>
<html lang="th">
<head>
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <meta charset="utf-8">
  <title>AutoDoor Lite ESP8285</title>
  <style>
    :root {
      --bg:#060a12; --panel:#0d1420; --panel2:#111a29; --line:#223044;
      --text:#e8eefb; --muted:#8fa3bf; --blue:#7bb4ff; --blue2:#3f7cff;
      --danger:#d45c63; --good:#47c88a;
    }
    * { box-sizing: border-box; }
    body { margin:0; font-family: system-ui, -apple-system, Segoe UI, Tahoma, sans-serif; background: radial-gradient(circle at top, #10182a 0%, var(--bg) 52%); color:var(--text); }
    .wrap { max-width: 760px; margin: 0 auto; padding: 12px 14px 28px; }
    .topbar { position: sticky; top:0; z-index:20; backdrop-filter: blur(8px); background: rgba(6,10,18,.78); border-bottom:1px solid rgba(255,255,255,.06); margin: -12px -14px 14px; padding: 12px 14px; }
    .title { font-size: 22px; font-weight: 800; margin:0; }
    .sub { margin:4px 0 0; color: var(--muted); font-size: 13px; }
    .tabs { display:flex; gap:8px; margin-top:12px; }
    .tab { flex:1; border:1px solid var(--line); background:var(--panel); color:var(--text); border-radius:14px; padding:12px 10px; font-weight:700; }
    .tab.active { background: linear-gradient(180deg, #2c59a8, #1a407e); border-color: transparent; }
    .card { background: linear-gradient(180deg, rgba(17,26,41,.95), rgba(13,20,32,.96)); border:1px solid var(--line); border-radius:18px; padding:14px; margin: 0 0 12px; box-shadow: 0 10px 30px rgba(0,0,0,.22); }
    .heroKicker { font-size: 11px; letter-spacing: .18em; color:var(--blue); font-weight:700; }
    .heroTitle { font-size: 28px; margin: 4px 0 2px; font-weight: 900; }
    .heroSub { margin:0; color: var(--muted); font-size: 13px; line-height:1.45; }
    .btn { border:0; border-radius: 16px; padding: 12px 14px; font-weight: 800; color:var(--text); background:var(--panel2); }
    .btnPri { background: linear-gradient(180deg, var(--blue), var(--blue2)); color:#061225; }
    .btnDanger { background: #2a1014; color:#ffb8bf; border:1px solid rgba(255,100,110,.28); }
    .btnGhost { background: transparent; border:1px solid var(--line); }
    .btnRow { display:flex; gap:10px; flex-wrap:wrap; }
    .btnRow .btn { flex:1; min-width: 110px; }
    input, select { width:100%; border-radius:14px; border:1px solid #243449; background:#09111b; color:var(--text); padding:12px 12px; outline:none; }
    label { display:block; font-size: 12px; color: var(--muted); margin: 10px 0 6px; }
    .grid2 { display:grid; grid-template-columns:1fr 1fr; gap:10px; }
    .grid1 { display:grid; grid-template-columns:1fr; gap:10px; }
    .bigRssi { font-size: 72px; line-height: .92; font-weight: 900; letter-spacing: -0.04em; text-align:center; }
    .bigRssiUnit { display:block; font-size: 13px; color: var(--muted); margin-top: 6px; font-weight: 700; }
    .center { text-align:center; }
    .muted { color: var(--muted); }
    .rssiRow { display:flex; align-items:center; justify-content:space-between; gap:12px; }
    .rssiPill { padding:7px 12px; border-radius: 999px; background: rgba(255,163,43,.12); border:1px solid rgba(255,163,43,.34); color:#ffba64; font-size: 12px; font-weight:800; white-space:nowrap; }
    .tagGrid { display:grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap:12px; }
    .tagCard { border-radius: 18px; background: linear-gradient(180deg, #162133, #111a2a); border:1px solid #2a3648; overflow:hidden; }
    .tagHead { display:flex; justify-content:space-between; align-items:flex-start; padding:14px 14px 8px; }
    .tagIcon { width:42px; height:42px; border-radius: 12px; background:#16243b; border:1px solid #31415a; display:grid; place-items:center; color:#8bb7ff; font-size:18px; }
    .tagMeta { padding-left: 10px; flex:1; }
    .tagKind { font-size: 11px; color:#8ea1be; font-weight:700; letter-spacing:.08em; }
    .tagName { font-size: 17px; font-weight:900; margin-top:2px; }
    .tagId { font-size: 12px; color:#9db4d2; margin-top:2px; }
    .tagDot { width:10px; height:10px; border-radius:50%; background:#4d5d74; margin-top:4px; }
    .tagBody { padding: 0 14px 12px; color:#c9d4e6; font-size: 13px; }
    .tagBody b { color: #fff; }
    .tagActions { display:grid; grid-template-columns:1fr 1fr; border-top:1px solid #2a3648; }
    .tagAct { padding: 12px 10px; text-align:center; background:transparent; border:0; color:#cdd7e6; font-weight:800; }
    .tagAct + .tagAct { border-left:1px solid #2a3648; }
    .tagAct.danger { color:#ff8f95; }
    .tagAct .ico { display:block; font-size: 16px; margin-bottom: 4px; }
    .sectionHead { font-size: 18px; font-weight: 900; margin: 0 0 10px; }
    .hint { font-size: 12px; color: var(--muted); margin-top: 8px; }
    .swRow { display:flex; justify-content:space-between; align-items:flex-start; gap:10px; }
    .swBox { width:54px; height:30px; border-radius:999px; background:#1a2434; border:1px solid #304158; position:relative; flex:0 0 auto; }
    .swBox input { position:absolute; opacity:0; inset:0; margin:0; }
    .swKnob { position:absolute; width:22px; height:22px; border-radius:50%; background:#8aa0bd; top:3px; left:4px; transition:.18s; }
    .swBox input:checked + .swKnob { left:28px; background:#8bd0ff; }
    .page { display:none; }
    .page.active { display:block; }
    .hidden { display:none; }
    @media (max-width: 640px) { .tagGrid { grid-template-columns:1fr; } .grid2 { grid-template-columns:1fr; } .btnRow .btn { min-width: 0; } }
  </style>
</head>
<body>
<div class="wrap">
  <div class="topbar">
    <div class="heroKicker">ESP-NOW</div>
    <h1 class="title">จัดการ Tag</h1>
    <p class="sub">ตั้งค่า Tag, เรียนรู้สัญญาณ, และสลับ Auto Link ได้จากหน้า AP เดียว</p>
    <div class="tabs">
      <button class="tab active" id="tabList" onclick="showPage('list')">Tag ทั้งหมด</button>
      <button class="tab" id="tabEdit" onclick="showPage('editor')">แก้ไข Tag</button>
      <button class="tab" id="tabAuto" onclick="showPage('autolink')">Auto Link</button>
    </div>
  </div>

  <section id="pageList" class="page active">
    <div class="card">
      <div class="rssiRow">
        <div>
          <div class="heroKicker">สัญญาณแท็ก</div>
          <h2 style="margin:4px 0 0;font-size:20px">RSSI ล่าสุด</h2>
        </div>
        <div class="rssiPill" id="sigTagLabel">รอแท็ก</div>
      </div>
      <div class="bigRssi" id="signalRssi">--<span class="bigRssiUnit">dBm</span></div>
      <div class="center muted" id="signalInfo">ยังไม่มีสัญญาณจากแท็กที่เรียนรู้ไว้</div>
      <div class="center" id="doorStatusText" style="margin-top:8px;font-weight:800;color:#8bd0ff">สถานะประตู: ไม่ทราบ</div>
      <div class="btnRow" style="margin-top:12px">
        <button class="btn btnPri" onclick="door('open')">เปิดประตู</button>
        <button class="btn btnGhost" onclick="door('stop')">หยุด</button>
        <button class="btn btnGhost" onclick="door('close')">ปิดประตู</button>
      </div>
    </div>

    <div class="card">
      <div class="rssiRow" style="margin-bottom:10px">
        <div>
          <div class="heroKicker">ESP-NOW</div>
          <div class="sectionHead" style="margin:4px 0 0">Tag ทั้งหมด</div>
        </div>
        <button class="btn btnPri" style="min-width:120px" onclick="newTag()">+ เพิ่ม Tag</button>
      </div>
      <div id="tagGrid" class="tagGrid"></div>
      <div id="tagEmpty" class="hint">ยังไม่มี Tag ในระบบ</div>
    </div>
  </section>

  <section id="pageEditor" class="page">
    <div class="card">
      <div class="rssiRow" style="margin-bottom:10px">
        <div>
          <div class="heroKicker">แก้ไข Tag</div>
          <div class="sectionHead" id="editorTitle">เพิ่ม Tag ใหม่</div>
        </div>
        <button class="btn btnGhost" onclick="showPage('list')">ย้อนกลับ</button>
      </div>

      <div class="card" style="padding:12px;margin-top:0;background:#101826">
        <div class="rssiRow" style="margin-bottom:8px">
          <b>เรียนรู้ Tag อัตโนมัติ</b>
          <span class="rssiPill" id="edLearnState">พร้อม</span>
        </div>
        <div class="hint" id="edLearnInfo">กด Learn แล้วนำ Tag มาแตะ/ผ่านหน้าเครื่อง ระบบจะเพิ่ม Tag ให้อัตโนมัติ</div>
        <div class="btnRow" style="margin-top:10px">
          <button class="btn btnPri" onclick="edStartLearn()">Learn Tag</button>
          <button class="btn btnGhost" onclick="edStopLearn()">หยุด Learn</button>
        </div>
      </div>

      <label>ชื่อ Tag</label>
      <input id="edName" placeholder="ชื่อ Tag" />

      <div class="grid2">
        <div>
          <label>Tag hold (วินาที)</label>
          <input id="edHoldSec" type="number" min="1" step="1" />
        </div>
        <div>
          <label>Tag close delay (วินาที)</label>
          <input id="edCloseDelaySec" type="number" min="0" step="1" />
        </div>
      </div>

      <div class="card" style="padding:12px;margin-top:12px;background:#101826">
        <div class="rssiRow">
          <b>ระยะเปิดประตู</b>
          <span class="rssiPill" id="edRssiMinLbl">-100 dBm</span>
        </div>
        <input id="edRssiMin" type="range" min="-127" max="0" step="1" oninput="syncEditorLabels()" />
        <div class="hint">เลื่อนค่ามากขึ้นเพื่อให้เปิดได้ใกล้ขึ้น</div>
      </div>

      <div class="card" style="padding:12px;margin-top:12px;background:#101826">
        <div class="rssiRow">
          <b>ระยะปิดประตู</b>
          <span class="rssiPill" id="edPassRssiLbl">-90 dBm</span>
        </div>
        <input id="edPassRssi" type="range" min="-127" max="0" step="1" oninput="syncEditorLabels()" />
        <label>หน่วงเวลาปิด (วินาที)</label>
        <input id="edPassCloseSec" type="number" min="0" step="1" />
        <div class="hint">เมื่อ RSSI ต่ำกว่าค่านี้จะรอหน่วงก่อนสั่งปิด</div>
      </div>

      <div class="btnRow" style="margin-top:12px">
        <button class="btn btnPri" onclick="saveEditor()">บันทึก Tag</button>
        <button class="btn btnDanger" id="btnDeleteTag" onclick="deleteTag()">ลบ Tag</button>
      </div>
      <div class="hint" id="editorMsg"></div>
    </div>
  </section>

  <section id="pageAuto" class="page">
    <div class="card">
      <div class="sectionHead">Auto Link</div>
      <div class="hint" style="margin-top:-4px">เปิด = Tag เปิด/ปิดประตูอัตโนมัติ · ปิด = ระบบ Tag ไม่ทำงาน</div>

      <div class="card" style="padding:12px;margin-top:12px;background:#101826">
        <div class="swRow">
          <div>
            <b>Tag ทำงานอัตโนมัติ</b>
            <div class="hint">เปิดใช้งานการเปิด-ปิดประตูจากสัญญาณ Tag</div>
          </div>
          <label class="swBox">
            <input id="swTagAuto" type="checkbox" onchange="saveAutoLink()" />
            <span class="swKnob"></span>
          </label>
        </div>
      </div>

      <div class="card" style="padding:12px;margin-top:12px;background:#101826">
        <div class="swRow">
          <div>
            <b>โหมดจำลอง (ไม่คุมประตูจริง)</b>
            <div class="hint">เปิดเพื่อทดสอบระบบโดยไม่สั่งรีเลย์จริง</div>
          </div>
          <label class="swBox">
            <input id="swSimMode" type="checkbox" onchange="saveAutoLink()" />
            <span class="swKnob"></span>
          </label>
        </div>
      </div>

      <div class="btnRow" style="margin-top:12px">
        <button class="btn btnGhost" onclick="showPage('list')">กลับไปหน้า Tag</button>
      </div>
      <div class="hint" id="autoLinkMsg"></div>
    </div>
  </section>

  <div class="card">
    <div class="sectionHead">สถานะระบบ</div>
    <div class="muted mono" id="status">loading...</div>
    <div class="hint" id="learn"></div>
  </div>
</div>

<script>
let tagsCache = [];
let editingTagId = 0;

async function api(path, opts){
  const r = await fetch(path, opts || {});
  const t = await r.text();
  try { return JSON.parse(t); } catch (_) { return {raw:t}; }
}

function showPage(name) {
  document.getElementById('pageList').classList.toggle('active', name === 'list');
  document.getElementById('pageEditor').classList.toggle('active', name === 'editor');
  document.getElementById('pageAuto').classList.toggle('active', name === 'autolink');
  document.getElementById('tabList').classList.toggle('active', name === 'list');
  document.getElementById('tabEdit').classList.toggle('active', name === 'editor');
  document.getElementById('tabAuto').classList.toggle('active', name === 'autolink');
}

async function refreshStatus(){
  const s = await api('/api/status');
  document.getElementById('status').textContent = JSON.stringify(s);
  const signalRssi = document.getElementById('signalRssi');
  const signalInfo = document.getElementById('signalInfo');
  const sigTagLabel = document.getElementById('sigTagLabel');
  const doorStatusText = document.getElementById('doorStatusText');

  const doorMap = {
    opened: 'ประตูเปิด',
    opening: 'กำลังเปิดประตู',
    closed: 'ประตูปิด',
    closing: 'กำลังปิดประตู',
    stopped: 'ประตูหยุด',
  };
  if (doorStatusText) {
    const txt = (s && s.door && doorMap[s.door]) ? doorMap[s.door] : 'สถานะประตู: ไม่ทราบ';
    doorStatusText.textContent = txt;
  }

  const hasSignalTag = s && typeof s.signalTagId === 'number' && s.signalTagId > 0;
  const hasActiveUnknown = s && s.anyActive === true;

  if (hasSignalTag || hasActiveUnknown) {
    if (typeof s.signalRssi === 'number' && s.signalRssi > -127) {
      signalRssi.innerHTML = s.signalRssi + '<span class="bigRssiUnit">dBm</span>';
    } else {
      signalRssi.innerHTML = 'N/A<span class="bigRssiUnit">RSSI</span>';
    }
    const ageText = (typeof s.signalAgeMs === 'number' && s.signalAgeMs < 0xFFFFFFFF) ? (' · ล่าสุด ' + Math.round(s.signalAgeMs / 100) / 10 + ' วิ') : '';
    const tagLabel = hasSignalTag ? ('Tag ' + s.signalTagId) : 'Tag กำลังออนไลน์';
    signalInfo.textContent = tagLabel + ageText;
    sigTagLabel.textContent = tagLabel;
  } else {
    signalRssi.innerHTML = '--<span class="bigRssiUnit">dBm</span>';
    signalInfo.textContent = 'ยังไม่มีสัญญาณจากแท็กที่เรียนรู้ไว้';
    sigTagLabel.textContent = 'รอแท็ก';
  }
}

async function refreshLearn(){
  const s = await api('/api/learn/status');
  document.getElementById('learn').textContent = s.active ? ('กำลังเรียนรู้... เหลือ ' + (s.leftSec ?? 0) + ' วินาที') : 'พร้อมใช้งาน';
  const edState = document.getElementById('edLearnState');
  const edInfo = document.getElementById('edLearnInfo');
  const edMsg = document.getElementById('editorMsg');
  if (edState) edState.textContent = s.active ? 'กำลัง Learn' : (s.added ? 'เรียนรู้แล้ว' : 'พร้อม');
  if (edInfo) {
    if (s.active) {
      edInfo.textContent = 'กำลังรอ Tag... นำ Tag ผ่านหน้าเครื่องเพื่อเพิ่มอัตโนมัติ';
    } else if (s.added) {
      edInfo.textContent = 'เรียนรู้สำเร็จแล้ว กดบันทึกหรือกลับไปหน้ารายการได้';
    } else {
      edInfo.textContent = 'กด Learn แล้วนำ Tag มาแตะ/ผ่านหน้าเครื่อง ระบบจะเพิ่ม Tag ให้อัตโนมัติ';
    }
  }
  if (edMsg && s.added && s.lastTagId) {
    edMsg.textContent = 'เพิ่ม Tag สำเร็จ: ' + s.lastTagId;
  }
}

function tagCardHtml(t) {
  const rssi = (typeof t.rssi === 'number' && t.rssi > -127) ? t.rssi : '--';
  return `<div class="tagCard">
    <div class="tagHead">
      <div style="display:flex;gap:10px;min-width:0;flex:1">
        <div class="tagIcon">≋</div>
        <div class="tagMeta">
          <div class="tagKind">TAG</div>
          <div class="tagName">${esc(t.name || 'Tag')}</div>
          <div class="tagId">ID: RF-${('00000000' + (t.tagId || 0).toString(16)).slice(-8).toUpperCase()}</div>
          <div class="tagId">RSSI: ${rssi} dBm · Hold ${Math.round((t.holdMs || 3000)/1000)} วิ</div>
        </div>
      </div>
      <div class="tagDot"></div>
    </div>
    <div class="tagBody">${t.enabled ? 'ใช้งานอยู่' : 'ปิดใช้งาน'}</div>
    <div class="tagActions">
      <button class="tagAct danger" onclick="removeTag(${t.tagId})"><span class="ico">🗑</span>ลบ</button>
      <button class="tagAct" onclick="openEditor(${t.tagId})"><span class="ico">✎</span>แก้ไข</button>
    </div>
  </div>`;
}

function esc(s){ return String(s ?? '').replace(/[&<>"]+/g, m => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[m])); }

async function refreshTags(){
  tagsCache = await api('/api/tags') || [];
  const grid = document.getElementById('tagGrid');
  const empty = document.getElementById('tagEmpty');
  grid.innerHTML = (tagsCache || []).map(tagCardHtml).join('');
  empty.classList.toggle('hidden', (tagsCache || []).length > 0);
}

async function refreshAutoLink(){
  const s = await api('/api/settings');
  if (!s) return;
  document.getElementById('swTagAuto').checked = !!s.tagAutoEnabled;
  document.getElementById('swSimMode').checked = !!s.simMode;
}

async function refreshAll(){
  await Promise.all([refreshStatus(), refreshLearn(), refreshTags(), refreshAutoLink()]);
}

async function door(cmd){ await api('/api/door/' + cmd, {method:'POST'}); refreshStatus(); }

async function newTag(){
  editingTagId = 0;
  document.getElementById('editorTitle').textContent = 'เพิ่ม Tag ใหม่';
  document.getElementById('btnDeleteTag').classList.add('hidden');
  document.getElementById('edName').value = '';
  document.getElementById('edHoldSec').value = 3;
  document.getElementById('edCloseDelaySec').value = 3;
  document.getElementById('edRssiMin').value = -100;
  document.getElementById('edPassRssi').value = -90;
  document.getElementById('edPassCloseSec').value = 3;
  syncEditorLabels();
  document.getElementById('editorMsg').textContent = '';
  showPage('editor');
}

async function openEditor(tagId){
  const t = (tagsCache || []).find(x => Number(x.tagId) === Number(tagId));
  if (!t) return;
  editingTagId = tagId;
  document.getElementById('editorTitle').textContent = 'แก้ไข Tag';
  document.getElementById('btnDeleteTag').classList.remove('hidden');
  document.getElementById('edName').value = t.name || '';
  document.getElementById('edHoldSec').value = Math.max(1, Math.round((t.holdMs || 3000) / 1000));
  document.getElementById('edCloseDelaySec').value = Math.max(0, Math.round((t.closeDelayMs || 3000) / 1000));
  document.getElementById('edRssiMin').value = t.tagRssiMin ?? -100;
  document.getElementById('edPassRssi').value = t.tagPassRssi ?? -90;
  document.getElementById('edPassCloseSec').value = Math.max(0, Math.round((t.tagPassCloseDelayMs || 3000) / 1000));
  syncEditorLabels();
  document.getElementById('editorMsg').textContent = '';
  showPage('editor');
}

function syncEditorLabels(){
  document.getElementById('edRssiMinLbl').textContent = document.getElementById('edRssiMin').value + ' dBm';
  document.getElementById('edPassRssiLbl').textContent = document.getElementById('edPassRssi').value + ' dBm';
}

async function edStartLearn(){
  const info = document.getElementById('edLearnInfo');
  const state = document.getElementById('edLearnState');
  await api('/api/learn/start', {
    method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'holdMs=' + encodeURIComponent((+document.getElementById('edHoldSec').value || 3) * 1000) + '&timeoutSec=30'
  });
  if (state) state.textContent = 'กำลัง Learn';
  if (info) info.textContent = 'กำลังรอ Tag... นำ Tag ผ่านหน้าเครื่องเพื่อเพิ่มอัตโนมัติ';
}

async function edStopLearn(){
  await api('/api/learn/stop', {method:'POST'});
  const info = document.getElementById('edLearnInfo');
  const state = document.getElementById('edLearnState');
  if (state) state.textContent = 'พร้อม';
  if (info) info.textContent = 'กด Learn แล้วนำ Tag มาแตะ/ผ่านหน้าเครื่อง ระบบจะเพิ่ม Tag ให้อัตโนมัติ';
}

async function saveEditor(){
  const body = new URLSearchParams({
    tagId: String(editingTagId || 0),
    name: document.getElementById('edName').value.trim() || 'Tag',
    holdMs: String(Math.max(1, (+document.getElementById('edHoldSec').value || 3)) * 1000),
    closeDelayMs: String(Math.max(0, (+document.getElementById('edCloseDelaySec').value || 0)) * 1000),
    tagRssiMin: String(+document.getElementById('edRssiMin').value || -100),
    tagPassRssi: String(+document.getElementById('edPassRssi').value || -90),
    tagPassCloseDelayMs: String(Math.max(0, (+document.getElementById('edPassCloseSec').value || 0)) * 1000),
    enabled: '1'
  });
  const path = editingTagId ? '/api/tags/update' : '/api/tags/add';
  const r = await api(path, { method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body: body.toString() });
  if (!r || r.ok === false) {
    document.getElementById('editorMsg').textContent = 'บันทึกไม่สำเร็จ';
    return;
  }
  await refreshTags();
  await refreshStatus();
  showPage('list');
}

async function deleteTag(){
  if (!editingTagId) return;
  await api('/api/tags/remove', { method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:'tagId='+editingTagId });
  await refreshTags();
  await refreshStatus();
  showPage('list');
}

async function removeTag(tagId){
  await api('/api/tags/remove', { method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:'tagId='+tagId });
  await refreshTags();
  await refreshStatus();
}

async function saveAutoLink(){
  const body = new URLSearchParams({
    tagAutoEnabled: document.getElementById('swTagAuto').checked ? '1' : '0',
    simMode: document.getElementById('swSimMode').checked ? '1' : '0'
  });
  await api('/api/auto-link', { method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body: body.toString() });
  document.getElementById('autoLinkMsg').textContent = 'บันทึกแล้ว';
  await refreshStatus();
}

setInterval(refreshAll, 500);
refreshAll();
syncEditorLabels();
</script>
</body>
</html>
)HTML";

String doorStateText() {
  switch (Door::state()) {
    case DoorState::Opening:
      return "opening";
    case DoorState::Closing:
      return "closing";
    case DoorState::Opened:
      return "opened";
    case DoorState::Closed:
      return "closed";
    default:
      return "stopped";
  }
}

uint32_t parseU32(const String& s) {
  if (!s.length()) {
    return 0;
  }
  return static_cast<uint32_t>(strtoul(s.c_str(), nullptr, 10));
}

void sendOk(ESP8266WebServer& server) {
  server.send(200, "application/json", "{\"ok\":true}");
}

struct SignalSnapshot {
  uint32_t tagId = 0;
  int8_t rssi = -127;
  uint32_t ageMs = 0xFFFFFFFFUL;
};

SignalSnapshot currentSignalSnapshot() {
  SignalSnapshot best;
  const uint32_t now = millis();
  for (size_t i = 0; i < TagStore::tagCount(); ++i) {
    const TagRecord* tag = TagStore::tagAt(i);
    const RuntimeTagState* runtime = TagStore::runtimeAt(i);
    if (!tag || !runtime || !tag->enabled) {
      continue;
    }
    const uint32_t age = now - runtime->lastSeenMs;
    if (runtime->lastSeenMs == 0 || age > 5000) {
      continue;
    }

    // Prefer freshest packet first. If same age, prefer known RSSI over unknown, then stronger RSSI.
    const bool bestKnown = best.rssi > -127;
    const bool candKnown = runtime->rssi > -127;
    if (age < best.ageMs ||
        (age == best.ageMs && ((candKnown && !bestKnown) || (candKnown == bestKnown && runtime->rssi > best.rssi)))) {
      best.tagId = tag->tagId;
      best.rssi = runtime->rssi;
      best.ageMs = age;
    }
  }
  return best;
}

}  // namespace

namespace HttpApi {

void begin(ESP8266WebServer& server,
           LearnStartFn startFn,
           LearnStopFn stopFn,
           LearnStatusFn statusFn,
           AutoChannelFn autoChannelFn) {
  gLearnStartFn = startFn;
  gLearnStopFn = stopFn;
  gLearnStatusFn = statusFn;
  gAutoChannelFn = autoChannelFn;

  server.on("/", HTTP_GET, [&server]() {
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
    server.send(200, "text/html", kIndexHtml);
  });

  server.on("/api/status", HTTP_GET, [&server]() {
    const AppSettings& s = TagStore::settings();
    const SignalSnapshot signal = currentSignalSnapshot();
    String out = "{";
    out += "\"door\":\"" + doorStateText() + "\",";
    out += "\"tagCount\":" + String(TagStore::tagCount()) + ",";
    out += "\"anyActive\":" + String(TagStore::anyActiveTags() ? "true" : "false") + ",";
    out += "\"channel\":" + String(s.espNowChannel) + ",";
    out += "\"relayPulseMs\":" + String(s.relayPulseMs) + ",";
    out += "\"tagSensitivity\":" + String(s.tagSensitivity) + ",";
    out += "\"tagCloseDelayMs\":" + String(s.tagCloseDelayMs) + ",";
    out += "\"signalTagId\":" + String(signal.tagId) + ",";
    out += "\"signalRssi\":" + String(signal.rssi) + ",";
    out += "\"signalAgeMs\":" + String(signal.ageMs) + ",";
    out += "\"signalRssiKnown\":" + String(signal.rssi > -127 ? "true" : "false");
    out += "}";
    server.send(200, "application/json", out);
  });

  server.on("/api/tags", HTTP_GET, [&server]() {
    server.send(200, "application/json", TagStore::tagsToJson());
  });

  server.on("/api/learn/status", HTTP_GET, [&server]() {
    if (!gLearnStatusFn) {
      server.send(500, "application/json", "{\"ok\":false,\"error\":\"learn_not_bound\"}");
      return;
    }
    server.send(200, "application/json", gLearnStatusFn());
  });

  server.on("/api/learn/start", HTTP_POST, [&server]() {
    if (!gLearnStartFn) {
      server.send(500, "application/json", "{\"ok\":false,\"error\":\"learn_not_bound\"}");
      return;
    }
    const uint16_t holdMs = static_cast<uint16_t>(parseU32(server.arg("holdMs")));
    const uint16_t timeoutSec = static_cast<uint16_t>(parseU32(server.arg("timeoutSec")));
    if (gLearnStartFn(holdMs > 0 ? holdMs : 3000, timeoutSec > 0 ? timeoutSec : 30)) {
      sendOk(server);
      return;
    }
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"learn_start_failed\"}");
  });

  server.on("/api/learn/stop", HTTP_POST, [&server]() {
    if (!gLearnStopFn) {
      server.send(500, "application/json", "{\"ok\":false,\"error\":\"learn_not_bound\"}");
      return;
    }
    gLearnStopFn();
    sendOk(server);
  });

  server.on("/api/tags/add", HTTP_POST, [&server]() {
    const uint32_t tagId = parseU32(server.arg("tagId"));
    const String name = server.arg("name");
    const uint16_t holdMs = static_cast<uint16_t>(parseU32(server.arg("holdMs")));
    if (TagStore::addTag(tagId, name.c_str(), holdMs > 0 ? holdMs : 3000)) {
      sendOk(server);
      return;
    }
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"add_failed\"}");
  });

  server.on("/api/tags/update", HTTP_POST, [&server]() {
    const uint32_t tagId = parseU32(server.arg("tagId"));
    const String name = server.arg("name");
    const uint16_t holdMs = static_cast<uint16_t>(parseU32(server.arg("holdMs")));
    const uint16_t closeDelayMs = static_cast<uint16_t>(parseU32(server.arg("closeDelayMs")));
    const int8_t tagRssiMin = static_cast<int8_t>(static_cast<int16_t>(parseU32(server.arg("tagRssiMin"))));
    const int8_t tagPassRssi = static_cast<int8_t>(static_cast<int16_t>(parseU32(server.arg("tagPassRssi"))));
    const uint16_t tagPassCloseDelayMs = static_cast<uint16_t>(parseU32(server.arg("tagPassCloseDelayMs")));
    const bool enabled = !server.hasArg("enabled") || parseU32(server.arg("enabled")) != 0;
    if (TagStore::setTag(tagId, name.c_str(), holdMs, closeDelayMs, tagRssiMin, tagPassRssi, tagPassCloseDelayMs, enabled)) {
      sendOk(server);
      return;
    }
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"update_failed\"}");
  });

  server.on("/api/tags/remove", HTTP_POST, [&server]() {
    const uint32_t tagId = parseU32(server.arg("tagId"));
    if (TagStore::removeTag(tagId)) {
      sendOk(server);
      return;
    }
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"remove_failed\"}");
  });

  server.on("/api/settings", HTTP_GET, [&server]() {
    const AppSettings& s = TagStore::settings();
    String out = "{";
    out += "\"ch\":" + String(s.espNowChannel) + ",";
    out += "\"pulseMs\":" + String(s.relayPulseMs) + ",";
    out += "\"tagSensitivity\":" + String(s.tagSensitivity) + ",";
    out += "\"tagCloseDelayMs\":" + String(s.tagCloseDelayMs) + ",";
    out += "\"tagAutoEnabled\":" + String(s.tagAutoEnabled ? "true" : "false") + ",";
    out += "\"simMode\":" + String(s.simMode ? "true" : "false") + ",";
    out += "\"rfOpen\":" + String(s.rfOpenCode) + ",";
    out += "\"rfClose\":" + String(s.rfCloseCode) + ",";
    out += "\"rfStop\":" + String(s.rfStopCode);
    out += "}";
    server.send(200, "application/json", out);
  });

  server.on("/api/settings", HTTP_POST, [&server]() {
    AppSettings s = TagStore::settings();
    if (server.hasArg("ch")) s.espNowChannel = static_cast<uint8_t>(parseU32(server.arg("ch")));
    if (server.hasArg("pulseMs")) s.relayPulseMs = static_cast<uint16_t>(parseU32(server.arg("pulseMs")));
    if (server.hasArg("tagSensitivity")) s.tagSensitivity = static_cast<uint8_t>(parseU32(server.arg("tagSensitivity")));
    if (server.hasArg("tagCloseDelayMs")) s.tagCloseDelayMs = static_cast<uint16_t>(parseU32(server.arg("tagCloseDelayMs")));
    if (server.hasArg("tagAutoEnabled")) s.tagAutoEnabled = parseU32(server.arg("tagAutoEnabled")) ? 1 : 0;
    if (server.hasArg("simMode")) s.simMode = parseU32(server.arg("simMode")) ? 1 : 0;
    // Backward-compatible aliases from old project naming
    if (server.hasArg("tagRx")) s.tagSensitivity = static_cast<uint8_t>(parseU32(server.arg("tagRx")));
    if (server.hasArg("tagCloseSec")) s.tagCloseDelayMs = static_cast<uint16_t>(parseU32(server.arg("tagCloseSec")) * 1000UL);
    if (server.hasArg("rfOpen")) s.rfOpenCode = parseU32(server.arg("rfOpen"));
    if (server.hasArg("rfClose")) s.rfCloseCode = parseU32(server.arg("rfClose"));
    if (server.hasArg("rfStop")) s.rfStopCode = parseU32(server.arg("rfStop"));
    if (TagStore::updateSettings(s)) {
      Door::setPulseMs(s.relayPulseMs);
      sendOk(server);
      return;
    }
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"save_failed\"}");
  });

  server.on("/api/auto-link", HTTP_POST, [&server]() {
    AppSettings s = TagStore::settings();
    if (server.hasArg("tagAutoEnabled")) s.tagAutoEnabled = parseU32(server.arg("tagAutoEnabled")) ? 1 : 0;
    if (server.hasArg("simMode")) s.simMode = parseU32(server.arg("simMode")) ? 1 : 0;
    if (TagStore::updateSettings(s)) {
      sendOk(server);
      return;
    }
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"save_failed\"}");
  });

  server.on("/api/channel/auto", HTTP_POST, [&server]() {
    if (!gAutoChannelFn) {
      server.send(500, "application/json", "{\"ok\":false,\"error\":\"auto_channel_not_bound\"}");
      return;
    }
    const uint8_t ch = gAutoChannelFn();
    String out = "{\"ok\":true,\"ch\":" + String(ch) + "}";
    server.send(200, "application/json", out);
  });

  server.on("/api/door/open", HTTP_POST, [&server]() {
    Door::open();
    sendOk(server);
  });
  server.on("/api/door/stop", HTTP_POST, [&server]() {
    Door::stop();
    sendOk(server);
  });
  server.on("/api/door/close", HTTP_POST, [&server]() {
    Door::close();
    sendOk(server);
  });
}

}  // namespace HttpApi
