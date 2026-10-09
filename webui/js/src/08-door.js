function updateGateVisual(state) {
  const g = $('gateVisual');
  const main = $('dashStatusMain');
  const pulse = $('dashPulse');
  if (!g) return;

  if (State.doorTransitionMs) {
    g.style.setProperty('--gate-travel', (State.doorTransitionMs / 1000) + 's');
  }

  if (g.dataset.doorState === state) return;
  g.dataset.doorState = state;

  g.classList.remove('open', 'closed', 'opening', 'closing', 'stopped', 'moving', 'unknown');
  main?.classList.remove('open', 'closed', 'warn', 'moving', 'stop');
  pulse?.classList.remove('pulse-red', 'pulse-green', 'pulse-amber', 'pulse-blue');

  switch (state) {
    case 'opening':
      g.classList.add('opening', 'moving');
      main?.classList.add('moving', 'open');
      pulse?.classList.add('pulse-blue');
      break;
    case 'closing':
      g.classList.add('closing', 'moving');
      main?.classList.add('moving', 'open');
      pulse?.classList.add('pulse-blue');
      break;
    case 'open':
      g.classList.add('open');
      main?.classList.add('open');
      pulse?.classList.add('pulse-green');
      break;
    case 'stopped':
      g.classList.add('stopped');
      main?.classList.add('warn', 'stop');
      pulse?.classList.add('pulse-amber');
      break;
    case 'closed':
      g.classList.add('closed');
      main?.classList.add('closed');
      pulse?.classList.add('pulse-red');
      break;
    default:
      g.classList.add('closed', 'unknown');
      main?.classList.add('closed');
      break;
  }
}

function applyDoorStateFromServer(doorState) {
  const st = normalizeDoorState(doorState);
  updateGateVisual(st);
  const statusText = $('dashStatusText');
  if (statusText) statusText.textContent = doorStateLabel(st);
}

function setDoorControlsEnabled(enabled) {
  document.querySelectorAll('[data-action="door-send"]').forEach((btn) => {
    btn.disabled = !enabled;
  });
}

function doorPendingVisual(slot) {
  if (slot === 'open') return 'opening';
  if (slot === 'close') return 'closing';
  if (slot === 'stop') return 'stopped';
  return null;
}

async function doorSend(slot) {
  if (!State.appBooted) return;

  const pending = doorPendingVisual(slot);
  if (pending) updateGateVisual(pending);

  const btns = document.querySelectorAll('[data-action="door-send"]');
  btns.forEach((b) => { b.disabled = true; });

  try {
    const d = await api('/api/door', { method: 'POST', body: { slot } });
    if (d.ok === false) throw new Error('door_failed');
    if (d.doorState) applyDoorStateFromServer(d.doorState);
    pollLive();
  } catch (e) {
    try {
      await pollLive();
    } catch (_) {}
    alert('ส่งคำสั่งไม่สำเร็จ');
  } finally {
    if (State.appBooted) setDoorControlsEnabled(true);
  }
}
