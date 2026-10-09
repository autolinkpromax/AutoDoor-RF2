// 21-analytics.js - Client-Side History Analytics & Access Entry Logs Engine
(function() {
  State.historyAllCycles = [];
  State.historyCycles = [];
  State.historyKpi = {};
  State.historyRawEvents = [];
  State.historyViewMode = 'cycles'; // 'cycles' or 'raw'
  State.historyVehicleFilter = 'all';

  // Helper to ensure names are always clean without (id) or parentheses
  function getCleanName(entity) {
    if (!entity || typeof entity !== 'string') return '—';
    return typeof cleanEntityName === 'function' ? cleanEntityName(entity) : entity.replace(/\s*\([0-9a-fA-F:_-]+\)$/, '').trim();
  }

  // Helper to resolve icon, theme, and labels for an event
  function resolveEventMeta(ev) {
    let configuredIcon = '';
    const tags = State.lastLiveData?.tags || [];
    const tagMatch = tags.find(t => t.name === ev.msg || t.id === ev.entity || t.name === ev.entity);
    if (tagMatch && tagMatch.icon && tagMatch.icon !== 'auto') {
      configuredIcon = tagMatch.icon.toLowerCase();
    }

    const typeKey = (configuredIcon || ev.type || '').toLowerCase();
    const reason = ev.reason || '';
    const reasonLower = reason.toLowerCase();
    const msg = ev.msg || '';
    const msgLower = msg.toLowerCase();

    let emoji = '🏷️';
    let themeClass = 'access-theme-other';
    let channelLabel = reason || 'ทั่วไป';
    let isHold = false;
    let isRf = false;
    let isTag = false;
    let isWeb = false;
    let isApp = false;

    if (typeKey === 'rf' || typeKey.includes('remote') || typeKey.includes('key') || reasonLower.includes('รีโมท') || msgLower.includes('รีโมท')) {
      isRf = true;
      emoji = '🔑';
      themeClass = 'access-theme-rf';
      channelLabel = 'รีโมท RF (433MHz)';
      if (typeKey === 'rf_hold' || reasonLower.includes('hold') || msgLower.includes('ค้าง')) {
        isHold = true;
      }
    } else if (typeKey === 'web' || reasonLower.includes('เว็บ') || msgLower.includes('web')) {
      isWeb = true;
      emoji = '💻';
      themeClass = 'access-theme-web';
      channelLabel = 'เว็บคอนโซล (Web UI)';
    } else if (typeKey === 'app' || reasonLower.includes('แอป') || msgLower.includes('blynk')) {
      isApp = true;
      emoji = '📱';
      themeClass = 'access-theme-app';
      channelLabel = 'แอปมือถือ (Blynk)';
    } else if (typeKey === 'btn' || reasonLower.includes('สวิตช์') || reasonLower.includes('ปุ่ม')) {
      emoji = '🔘';
      themeClass = 'access-theme-other';
      channelLabel = 'ปุ่มกดหน้าตู้';
    } else if (typeKey === 'ext' || reasonLower.includes('ภายนอก') || reasonLower.includes('เซ็นเซอร์')) {
      emoji = '⚡';
      themeClass = 'access-theme-other';
      channelLabel = 'เซ็นเซอร์ภายนอก';
    } else if (typeKey === 'schedule' || reasonLower.includes('ตารางเวลา')) {
      emoji = '⏰';
      themeClass = 'access-theme-other';
      channelLabel = 'ตั้งเวลาอัตโนมัติ';
    } else {
      // Tags (ESP-NOW)
      isTag = true;
      themeClass = 'access-theme-car';
      if (typeKey === 'suv' || reasonLower.includes('suv')) {
        emoji = '🚙';
        channelLabel = 'รถยนต์ SUV (ESP-NOW)';
      } else if (typeKey === 'sedan' || reasonLower.includes('เก๋ง')) {
        emoji = '🚗';
        channelLabel = 'รถเก๋ง (ESP-NOW)';
      } else if (typeKey === 'pickup' || reasonLower.includes('กระบะ') || typeKey.includes('truck')) {
        emoji = '🚚';
        channelLabel = 'รถกระบะ (ESP-NOW)';
      } else if (typeKey === 'bike' || reasonLower.includes('มอเตอร์ไซค์') || reasonLower.includes('มอไซ')) {
        emoji = '🏍️';
        themeClass = 'access-theme-bike';
        channelLabel = 'รถมอเตอร์ไซค์ (ESP-NOW)';
      } else if (typeKey === 'scooter' || reasonLower.includes('สกู๊ตเตอร์')) {
        emoji = '🛵';
        themeClass = 'access-theme-bike';
        channelLabel = 'รถสกู๊ตเตอร์ (ESP-NOW)';
      } else if (typeKey === 'van' || reasonLower.includes('รถตู้')) {
        emoji = '🚐';
        channelLabel = 'รถตู้ (ESP-NOW)';
      } else if (typeKey === 'bicycle' || reasonLower.includes('จักรยาน')) {
        emoji = '🚲';
        themeClass = 'access-theme-bike';
        channelLabel = 'รถจักรยาน (ESP-NOW)';
      } else if (typeKey === 'person' || reasonLower.includes('คนเดิน') || reasonLower.includes('บุคคล')) {
        emoji = '🚶';
        themeClass = 'access-theme-person';
        channelLabel = 'คนเดิน (ESP-NOW)';
      } else {
        emoji = typeof getVehicleEmoji === 'function' ? getVehicleEmoji(typeKey, false) : '🚗';
        channelLabel = 'แท็กยานพาหนะ (ESP-NOW)';
      }
    }

    return { typeKey, emoji, themeClass, channelLabel, reason: reason || channelLabel, isHold, isRf, isTag, isWeb, isApp };
  }

  // Reconstruct raw events into discrete access entry records with timeline steps
  function reconstructDoorCycles(events) {
    if (!events || !events.length) return [];

    // Clone and sort chronologically (oldest first)
    const sorted = [...events].reverse();
    const cycles = [];
    let curCycle = null;
    let lastCompletedCycle = null;

    sorted.forEach((ev) => {
      const type = (ev.type || '').toLowerCase();
      const msg = ev.msg || '';
      // Non-access legacy diagnostic events
      const isLegacyDiag = ['beam_cut', 'pass_arm', 'reversal', 'safe_close', 'auto_close', 'fall_close', 'mag_close', 'state_close', 'limit_close', 'limit_open'].includes(type);
      const isAccessOrOpen = !isLegacyDiag;
      const isComplete = type === 'mag_close' || (type === 'state_close' && msg.includes('ปิดสนิท'));

      // If we see an access or open trigger
      if (isAccessOrOpen) {
        let rawEntity = ev.entity || ev.msg || '—';
        if (rawEntity === '—' && msg) {
          const m = msg.match(/\[(.*?)\]\s*(.*?)\s*(เข้าประตู|สั่งเปิด)/);
          if (m && m[2]) rawEntity = m[2].trim();
        }
        let entity = getCleanName(rawEntity);

        // Ignore orphan / spurious manual opens where no entity was provided
        if (type === 'manual_open' && (rawEntity === '—' || rawEntity.includes('ไม่ทราบ')) && (!msg || msg.includes('ไม่ทราบ'))) {
          return;
        }

        const meta = resolveEventMeta(ev);

        // Finalize previous cycle if one existed
        if (curCycle) {
          curCycle.completed = true;
          if (curCycle.status === 'running') curCycle.status = 'success';
          cycles.push(curCycle);
          lastCompletedCycle = curCycle;
        }

        const actionTitle = meta.isHold
          ? 'สั่งเปิดค้างต่อเนื่อง (Hold Open)'
          : (meta.isTag
              ? 'ยานพาหนะเข้าประตู (Auto)'
              : (meta.isRf
                  ? 'กดรีโมทสั่งเปิดประตู'
                  : (meta.isWeb
                      ? 'สั่งเปิดประตูจากเว็บ'
                      : (meta.isApp ? 'สั่งเปิดประตูจากแอปมือถือ' : 'สั่งเปิดประตูอัตโนมัติ'))));

        const actionDesc = meta.isTag
          ? `${meta.emoji} ตรวจพบแท็ก [${entity}] (${meta.reason}) เข้าใกล้ประตู ➔ เปิดอัตโนมัติ`
          : (meta.isRf
              ? (meta.isHold ? `🔒 กดรีโมท [${entity}] ค้าง 4 วิ ➔ ล็อกเปิดค้างต่อเนื่อง` : `🔑 กดรีโมท [${entity}] ➔ สั่งเปิดประตูอัตโนมัติ`)
              : (meta.isWeb
                  ? `💻 สั่งเปิดประตูจากเว็บคอนโซล [${entity}]`
                  : (meta.isApp ? `📱 สั่งเปิดประตูจากแอปมือถือ [${entity}]` : `${meta.emoji} สั่งเปิดประตู [${entity}]`)));

        curCycle = {
          id: cycles.length + 1,
          startTime: ev.time || '—',
          endTime: ev.time || '—',
          triggerEvent: ev,
          triggerEntity: entity,
          triggerType: meta.typeKey,
          emoji: meta.emoji,
          themeClass: meta.themeClass,
          channelLabel: meta.channelLabel,
          reason: meta.reason,
          actionTitle: actionTitle,
          actionDesc: actionDesc,
          isHold: meta.isHold,
          isRf: meta.isRf,
          isTag: meta.isTag,
          isWeb: meta.isWeb,
          isApp: meta.isApp,
          triggerRssi: (typeof ev.metric === 'number' && ev.metric < 0) ? ev.metric : null,
          beamCutTime: null,
          passArmDelayMs: null,
          autoCloseTime: null,
          doubleCloseTime: null,
          runDurationMs: ev.durationMs || null,
          completed: true, // In Access Entry paradigm, all authorized triggers are successful entries
          closingStarted: false,
          status: 'success',
          notes: [],
          events: [ev],
          steps: [
            {
              name: actionTitle,
              time: ev.time || '',
              desc: actionDesc,
              dot: meta.isHold ? 'warn' : 'ok'
            }
          ]
        };
        return;
      }

      // If legacy diagnostic events arrive for this cycle (from older full logs)
      if (!curCycle) {
        if (isComplete && lastCompletedCycle) {
          lastCompletedCycle.events.push(ev);
          if (ev.durationMs && ev.durationMs > 0 && !lastCompletedCycle.runDurationMs) {
            lastCompletedCycle.runDurationMs = ev.durationMs;
          }
        }
        return;
      }

      curCycle.events.push(ev);

      if (type === 'beam_cut') {
        curCycle.beamCutTime = ev.time;
        curCycle.steps.push({
          name: 'ยานพาหนะตัดผ่านลำแสงกันหนีบ',
          time: ev.time || '',
          desc: `เซ็นเซอร์กันหนีบทำงาน (สถานะ: ${(ev.sensorBits & 1) ? 'ตัดแสง' : 'ปกติ'})`,
          dot: 'ok'
        });
      } else if (type === 'pass_arm') {
        curCycle.passArmDelayMs = ev.durationMs;
        curCycle.steps.push({
          name: 'พ้นลำแสงกันหนีบ',
          time: ev.time || '',
          desc: `ลำแสงกลับมาโล่ง ➔ หน่วงเวลานับถอยหลังปิด ${((ev.durationMs || 0) / 1000).toFixed(1)}s`,
          dot: 'ok'
        });
      } else if (type === 'reversal') {
        curCycle.steps.push({
          name: 'สลับทิศทางเตรียมปิด (Safety Deadband)',
          time: ev.time || '',
          desc: 'หยุดชะลอมอเตอร์ก่อนสลับทิศทางเพื่อความปลอดภัย',
          dot: 'pri'
        });
      } else if (type === 'safe_close' || type === 'auto_close' || type === 'fall_close' || type === 'manual_close') {
        curCycle.closingStarted = true;
        if (!curCycle.autoCloseTime) {
          curCycle.autoCloseTime = ev.time;
          const isFall = type === 'fall_close';
          const isManual = type === 'manual_close';
          curCycle.steps.push({
            name: isManual ? 'สั่งปิดประตูด้วยมือ' : (isFall ? 'ระบบสั่งปิดอัตโนมัติ (Fallback)' : 'ระบบสั่งปิดประตูอัตโนมัติ'),
            time: ev.time || '',
            desc: 'ส่งพัลส์รีเลย์ปิด (GPIO 7 = HIGH 500ms)',
            dot: 'pri'
          });
        }
      } else if (isComplete) {
        curCycle.endTime = ev.time || '—';
        curCycle.completed = true;
        curCycle.status = 'success';
        if (ev.durationMs && ev.durationMs > 0) {
          curCycle.runDurationMs = ev.durationMs;
        }
        curCycle.steps.push({
          name: 'ประตูปิดสนิทสมบูรณ์',
          time: ev.time || '',
          desc: `แม่เหล็กดูดติดสนิท${curCycle.runDurationMs ? ' (เวลาวิ่งปิด ' + (curCycle.runDurationMs / 1000).toFixed(1) + 's)' : ''}`,
          dot: 'ok'
        });
        cycles.push(curCycle);
        lastCompletedCycle = curCycle;
        curCycle = null;
      }
    });

    if (curCycle) {
      cycles.push(curCycle);
    }

    // Renumber IDs sequentially 1..N
    cycles.forEach((c, idx) => {
      c.id = idx + 1;
    });

    // Return in reverse order (newest entry first for display)
    return cycles.reverse();
  }

  function calculateAnalyticsKpi(cycles, rawEvents) {
    const total = cycles.length;
    let completedCount = 0;
    let holdCount = 0;
    const vehicleMap = {};
    const methodCounts = { espNow: 0, rf: 0, web: 0, app: 0, other: 0 };
    const anomalies = [];

    cycles.forEach((c) => {
      if (c.completed) completedCount++;
      if (c.isHold) holdCount++;

      const ent = getCleanName(c.triggerEntity);
      if (ent && ent !== '—') {
        vehicleMap[ent] = (vehicleMap[ent] || 0) + 1;
      }

      if (c.isTag) methodCounts.espNow++;
      else if (c.isRf) methodCounts.rf++;
      else if (c.isWeb) methodCounts.web++;
      else if (c.isApp) methodCounts.app++;
      else methodCounts.other++;
    });

    // Check raw events for true error anomalies only
    (rawEvents || []).forEach((ev) => {
      const type = ev.type || '';
      const msg = ev.msg || '';
      if (ev.level === 'error' || ev.level === 'danger') {
        anomalies.push(`[${ev.time}] ข้อผิดพลาดระบบ: ${msg || type}`);
      }
    });

    // Determine top access method
    let topMethod = '—';
    let topCount = 0;
    if (methodCounts.espNow > topCount) { topMethod = 'ESP-NOW'; topCount = methodCounts.espNow; }
    if (methodCounts.rf > topCount) { topMethod = 'รีโมท RF'; topCount = methodCounts.rf; }
    if (methodCounts.web > topCount) { topMethod = 'Web UI'; topCount = methodCounts.web; }
    if (methodCounts.app > topCount) { topMethod = 'แอปมือถือ'; topCount = methodCounts.app; }

    const latestCycle = cycles.length > 0 ? cycles[0] : null;

    let methodBreakdown = `ESP-NOW ${methodCounts.espNow} | RF ${methodCounts.rf} | Web ${methodCounts.web}`;
    if (methodCounts.app > 0) methodBreakdown += ` | App ${methodCounts.app}`;
    if (methodCounts.other > 0) methodBreakdown += ` | อื่นๆ ${methodCounts.other}`;

    return {
      totalCycles: total,
      completedCycles: completedCount,
      successRate: 100,
      latestTime: latestCycle ? latestCycle.startTime : '—',
      latestPerson: latestCycle ? getCleanName(latestCycle.triggerEntity) : '—',
      topMethod: topMethod,
      methodBreakdown: methodBreakdown,
      vehicleMap,
      anomalies: [...new Set(anomalies)],
    };
  }

  // Populate the Vehicle / User Filter dropdown
  function populateHistoryVehicleFilter(allCycles) {
    const vSel = $('historyVehicleFilter');
    if (!vSel) return;

    const counts = {};
    const iconMap = {};
    allCycles.forEach(c => {
      const ent = getCleanName(c.triggerEntity) || 'ไม่ระบุ';
      counts[ent] = (counts[ent] || 0) + 1;
      if (!iconMap[ent]) {
        iconMap[ent] = c.emoji || (typeof getVehicleEmoji === 'function' ? getVehicleEmoji(c.triggerType, c.isRf) : '👤');
      }
    });

    const currentVal = State.historyVehicleFilter || 'all';
    let html = `<option value="all">👥 ทั้งหมด (ทุกคน) (${allCycles.length})</option>`;
    Object.keys(counts).sort().forEach(ent => {
      const ico = iconMap[ent] || '👤';
      html += `<option value="${escHtml(ent)}">${ico} ${escHtml(ent)} (${counts[ent]})</option>`;
    });

    vSel.innerHTML = html;
    if (Array.from(vSel.options).some(o => o.value === currentVal)) {
      vSel.value = currentVal;
    } else {
      vSel.value = 'all';
      State.historyVehicleFilter = 'all';
    }

    if (!vSel._boundChange) {
      vSel._boundChange = true;
      vSel.addEventListener('change', () => {
        applyHistoryVehicleFilter();
      });
    }
  }

  function applyHistoryVehicleFilter() {
    const vSel = $('historyVehicleFilter');
    const filter = vSel ? vSel.value : 'all';
    State.historyVehicleFilter = filter;

    const allCycles = State.historyAllCycles || [];
    const filtered = (filter === 'all')
      ? allCycles
      : allCycles.filter(c => getCleanName(c.triggerEntity) === filter);

    State.historyCycles = filtered;
    const kpi = calculateAnalyticsKpi(filtered, State.historyRawEvents);
    State.historyKpi = kpi;

    renderHistoryDashboard(filtered, kpi, State.historyRawEvents);
  }

  // Render Dashboard KPI Summary & Entry Cards
  function renderHistoryDashboard(cycles, kpi, rawEvents) {
    // 1. KPI Summary Bar (Total Access Entries)
    const elCycles = $('historyKpiCycles');
    if (elCycles) elCycles.textContent = `${kpi.totalCycles} ครั้ง`;

    const elSuccess = $('historyKpiSuccessRate');
    if (elSuccess) {
      elSuccess.textContent = `บันทึกเรียบร้อย 100% (${kpi.completedCycles}/${kpi.totalCycles})`;
      elSuccess.style.color = 'var(--ok)';
    }

    // 2. Anomaly Alerts Banner
    const alertBanner = $('historyAlertBanner');
    const alertContent = $('historyAlertContent');
    if (alertBanner && alertContent) {
      if (kpi.anomalies.length > 0) {
        alertBanner.classList.remove('hidden');
        alertContent.innerHTML = kpi.anomalies.map(a => `• ${escHtml(a)}`).join('<br/>');
      } else {
        alertBanner.classList.add('hidden');
      }
    }

    // 3. Render Entry Cards
    const cycleContainer = $('historyCycleContainer');
    if (cycleContainer) {
      if (!cycles.length) {
        cycleContainer.innerHTML = '<div style="color:var(--muted);font-size:12px;text-align:center;padding:16px;background:rgba(255,255,255,0.02);border:1px solid var(--line);border-radius:10px">ไม่พบข้อมูลบันทึกการเข้าประตูตามเงื่อนไขที่เลือก</div>';
      } else {
        cycleContainer.innerHTML = cycles.map(c => {
          let cleanName = getCleanName(c.triggerEntity);
          if (c.isWeb || cleanName.includes('#255')) {
            cleanName = 'Web Console';
          }
          const typeIcon = c.emoji || (typeof getVehicleEmoji === 'function' ? getVehicleEmoji(c.triggerType, c.isRf) : '🏷️');
          
          let statusBadge = '';
          if (c.isHold) {
            statusBadge = '<span class="badgeMini" style="background:rgba(245,158,11,0.15);color:#f59e0b;border:1px solid rgba(245,158,11,0.3)">🔒 เปิดค้าง (Hold)</span>';
          } else if (c.isWeb) {
            statusBadge = '<span class="badgeMini" style="background:rgba(59,130,246,0.15);color:#60a5fa;border:1px solid rgba(59,130,246,0.3)">💻 Web Console</span>';
          } else if (c.isApp) {
            statusBadge = '<span class="badgeMini" style="background:rgba(168,85,247,0.15);color:#c084fc;border:1px solid rgba(168,85,247,0.3)">📱 Blynk App</span>';
          } else if (c.isRf) {
            statusBadge = '<span class="badgeMini" style="background:rgba(245,158,11,0.15);color:#fbbf24;border:1px solid rgba(245,158,11,0.3)">🔑 รีโมท RF</span>';
          } else if (c.isTag) {
            statusBadge = '<span class="badgeMini" style="background:rgba(62,207,142,0.15);color:#3ecf8e;border:1px solid rgba(62,207,142,0.3)">✓ เข้าประตู (Auto)</span>';
          } else {
            statusBadge = '<span class="badgeMini" style="background:rgba(62,207,142,0.15);color:#3ecf8e;border:1px solid rgba(62,207,142,0.3)">✓ เปิดสำเร็จ</span>';
          }

          const rssiBadge = c.triggerRssi
            ? `<span style="font-family:monospace;font-size:10px;background:rgba(255,255,255,0.06);padding:1px 6px;border-radius:4px;color:#60a5fa">📶 ${c.triggerRssi} dBm</span>`
            : '';

          const durBadge = c.runDurationMs
            ? `<span style="font-family:monospace;font-size:10px;background:rgba(255,255,255,0.06);padding:1px 6px;border-radius:4px;color:#cbd5e1">⏱ ${(c.runDurationMs / 1000).toFixed(1)}s</span>`
            : '';

          const stepsHtml = (c.steps || []).map(s => `
            <div class="anlStep">
              <div class="anlStepDot ${s.dot || 'ok'}"></div>
              <div class="anlStepHead">
                <span class="anlStepName">${escHtml(s.name)}</span>
                <span class="anlStepTime">${escHtml(s.time)} น.</span>
              </div>
              <div class="anlStepDesc">${escHtml(s.desc)}</div>
            </div>
          `).join('');

          const timeSuffix = c.startTime && c.startTime !== '—' ? ` ${escHtml(c.startTime)} น.` : '';

          return `
            <div class="anlCycleCard" id="historyCycleCard-${c.id}">
              <div class="anlCycleHead">
                <div class="anlCycleTitle">
                  <span style="font-size:18px">${typeIcon}</span>
                  <span>รายการที่ ${c.id}: ${escHtml(cleanName)}${timeSuffix}</span>
                </div>
                <div class="anlCycleMeta">
                  ${rssiBadge}
                  ${durBadge}
                  ${statusBadge}
                </div>
              </div>
              <div class="anlCycleBody">
                <div class="anlTimeline">
                  ${stepsHtml}
                </div>
              </div>
              <div class="anlCycleFoot">
                <span>เวลา: <b>${escHtml(c.startTime)} น.</b></span>
                <span>ประเภท / ช่องทาง: <b>${escHtml(c.reason || c.channelLabel || 'แท็ก / รีโมท')}</b></span>
              </div>
            </div>
          `;
        }).join('');
      }
    }

    // 4. Render Raw List
    const rawList = $('historyEventList');
    if (rawList && typeof formatEventItemHtml === 'function') {
      if (!rawEvents || !rawEvents.length) {
        rawList.innerHTML = '<div style="color:var(--muted);font-size:12px;text-align:center;padding:16px">ไม่มีข้อมูล Log ดิบ</div>';
      } else {
        rawList.innerHTML = rawEvents.map((ev, idx) => formatEventItemHtml(ev, idx, true)).join('');
      }
    }
  }

  function renderHistoryAnalytics(events) {
    State.historyRawEvents = events || [];
    const allCycles = reconstructDoorCycles(events || []);
    State.historyAllCycles = allCycles;

    populateHistoryVehicleFilter(allCycles);

    const filter = State.historyVehicleFilter || 'all';
    const filtered = (filter === 'all')
      ? allCycles
      : allCycles.filter(c => getCleanName(c.triggerEntity) === filter);
    State.historyCycles = filtered;

    const kpi = calculateAnalyticsKpi(filtered, events);
    State.historyKpi = kpi;

    renderHistoryDashboard(filtered, kpi, events);
  }

  function toggleHistoryView() {
    const cycleBox = $('historyCycleContainer');
    const rawBox = $('historyEventList');
    const btn = $('btnToggleHistoryView');
    if (!cycleBox || !rawBox) return;

    if (State.historyViewMode === 'raw') {
      State.historyViewMode = 'cycles';
      rawBox.classList.add('hidden');
      cycleBox.classList.remove('hidden');
      if (btn) btn.innerHTML = '<span>📜 สลับดู Log ดิบ</span>';
    } else {
      State.historyViewMode = 'raw';
      cycleBox.classList.add('hidden');
      rawBox.classList.remove('hidden');
      if (btn) btn.innerHTML = '<span>🚪 ดูแบบรายการเข้าประตู</span>';
    }
  }

  async function copyHistorySummary() {
    const kpi = State.historyKpi;
    const cycles = State.historyCycles || [];
    const sel = $('historyDateSelect');
    const sourceLabel = sel && sel.options[sel.selectedIndex] ? sel.options[sel.selectedIndex].text : 'ข้อมูลประวัติ';
    const vSel = $('historyVehicleFilter');
    const vehLabel = vSel && vSel.value !== 'all' ? ` | ฟิลเตอร์: ${vSel.value}` : '';

    if (!cycles.length) {
      if (typeof copyHistoryEvents === 'function') {
        copyHistoryEvents();
      }
      return;
    }

    const lines = [
      `=== บันทึกประวัติการเข้าประตู AutoDoor-RF2 (Access Entry Logs) ===`,
      `วันที่ / แหล่งข้อมูล: ${sourceLabel}${vehLabel}`,
      `บันทึกทั้งหมด: ${kpi.totalCycles || 0} ครั้ง (สำเร็จ 100%)`,
      `เข้าล่าสุด: ${kpi.latestTime || '—'} น. (${kpi.latestPerson || '—'})`,
      `ช่องทางหลัก: ${kpi.topMethod || '—'} (${kpi.methodBreakdown || '—'})`,
      ``,
      `--- รายการเข้าประตู ---`,
      ...cycles.map(c => {
        let cleanName = getCleanName(c.triggerEntity);
        if (c.isWeb || cleanName.includes('#255')) cleanName = 'Web Console';
        const mode = c.isHold ? 'เปิดค้าง (Hold Open)' : 'เข้าประตู (Auto)';
        const ico = c.emoji || (typeof getVehicleEmoji === 'function' ? getVehicleEmoji(c.triggerType, c.isRf) : '🏷️');
        return `[รายการที่ ${c.id}] ${ico} ${cleanName} ${c.startTime} น. | ${c.reason || c.channelLabel || '—'} | ${mode}`;
      })
    ];

    if (kpi.anomalies && kpi.anomalies.length) {
      lines.push(``, `--- ข้อสังเกต ---`);
      kpi.anomalies.forEach(a => lines.push(`• ${a}`));
    }

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
        setTimeout(() => { btn.textContent = 'คัดลอกสรุป'; }, 2000);
      }
    } catch (e) {
      console.error('Copy history summary error:', e);
    }
  }

  window.renderHistoryAnalytics = renderHistoryAnalytics;
  window.toggleHistoryView = toggleHistoryView;
  window.copyHistorySummary = copyHistorySummary;
})();
