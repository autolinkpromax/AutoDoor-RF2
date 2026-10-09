# AI Agent Instructions for AutoDoor-RF2

## 🚪 Core Architecture & Door Automation Reference
Before modifying or refactoring any code in `src/modules/door.cpp`, `src/modules/esp_now_runtime.cpp`, `src/modules/relay.cpp`, or `src/modules/tag_store.cpp`, you **MUST** read and strictly follow the comprehensive specification document:

👉 [DOOR_AUTOMATION_SPECIFICATION.md](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/docs/DOOR_AUTOMATION_SPECIFICATION.md)

### Key Rules & Invariants:
1. **5-Layer Architecture in `door.cpp`**: Keep code segregated cleanly into the 5 layers (Layer 1: Sensors, Layer 2: Tag Trigger & Suppression, Layer 3: Auto-Close Priority Engine, Layer 4: Reversal Deadband Actuator, Layer 5: Transition Timers).
2. **Safety Sensor is Primary Authority (ESP-NOW Tag Rule)**: When `gSafetySensorEnabled && gPassageCrossed`, closing decision is governed strictly by the Safety Sensor timer (`gSafetyCloseAtMs`). **For ESP-NOW tags, once the vehicle cuts/clears the safety beam, the door MUST CLOSE according to the tag's configured safety delay (`tagSafetyCloseDelayMs`), EVEN IF THE ESP-NOW TAG IS STILL ACTIVELY TRANSMITTING IN THE GARAGE.** Active tags in the garage must never delay or prevent this safety close.
3. **ESP-NOW Tag vs RF Remote Tag (433MHz) Distinction**:
   - **ESP-NOW Tag (Active 2-Way Beacon)**:
     - Tag buzzer sounds on **BOTH OPEN (`status = 1`) and CLOSE (`status = 2`)**.
     - Broadcast announce is sent periodically (250ms) AND immediately upon receiving every valid tag beacon (`triggerImmediateAnnounce()`) so the sleeping tag receives its confirmation chime during its wake RX window.
     - When closing starts (`noteClosingStarted`) or door closes, active ESP-NOW tags are snapshot into `gTagBaselineActiveIds` (`gTagOpenSuppressed = true`) to prevent reopening.
   - **RF Remote (433MHz Momentary Button)**:
     - 1-way passive transmitter (transmits only while button is physically pressed).
     - Short press = toggle / manual command. 4-second long press = Hold Open / Unhold toggle.
     - Never baseline suppressed.
4. **Guard Against Premature Re-Opening (`safetyCloseArmed`)**: Layer 2 must NOT auto-open if `gSafetyCloseAtMs != 0` or if a reversal to close is pending (`gReverseSlot == "close" && gReverseAtMs != 0`).
5. **Auto-Close Deadlock Prevention (`DoorState::Stopped`)**: Priority 2 Fallback Auto-Close and Priority 3 Failsafe MUST evaluate both `gDoorState == DoorState::Open` AND `gDoorState == DoorState::Stopped` so the door never hangs indefinitely if stopped midway.
6. **Directional Reversal Deadband (2.4s Total)**: When reversing direction between Open <-> Close (such as closing mid-way while opening, or opening during close): Always use a single STOP pulse (GPIO 6+7 = HIGH for 1200ms, `kStopPulseMs = 1200`, written simultaneously via atomic hardware register `GPIO_OUT_W1TS_REG`) ➔ followed by 1.2s settling deadband (`kReverseDeadbandMs = 1200`, total 2.4s) before firing the reverse command with solid 800ms pulse (GPIO 6 or 7 = HIGH for 800ms, `kReversePulseMs = 800`). Never send double STOP pulses. Ensure logs explicitly state GPIO pin numbers, HIGH state, and duration at each phase.
7. **Motor Control GPIO Mapping (Confirmed with Board Manufacturer)**:
   - **OPEN**: GPIO 6 = HIGH (500 ms)
   - **CLOSE**: GPIO 7 = HIGH (500 ms, or 800 ms after reverse/stop)
   - **STOP**: GPIO 6 & GPIO 7 = HIGH simultaneously via atomic register write (1200 ms)
   - **IDLE**: GPIO 6 & GPIO 7 = LOW
8. **Wi-Fi & ESP-NOW TX Power Configuration**: Always edit only `kWifiTxPowerDbm` in `src/app/config.h` (Single Point of Truth). Do NOT hardcode or modify `WiFi.setTxPower` across multiple files.
9. **Blynk Cloud Quota & Message Optimization (100k Monthly Limit)**:
   - Blynk Free Tier has a strict quota of **100,000 messages/month** for Datastream updates (`Blynk.virtualWrite`).
   - **Event-Driven Only**: Send `virtualWrite` ONLY when the actual door or sensor state changes (e.g., door starts opening, reaches open, starts closing, reaches closed, or stops).
   - **NEVER use fast polling / frequent periodic sync** (e.g. syncing every 20 seconds consumes 4,320 messages/day = ~130,000 messages/month, which exhausts the monthly quota).
   - Any fallback safety-net periodic sync MUST NOT be faster than **every 30 minutes** (1,800,000 ms, ~1,440 messages/month) or **1 hour**.
   - On connect / reconnect, `BLYNK_CONNECTED()` initializes and syncs current state once. Blynk Cloud caches this state indefinitely for all app and web viewers.

### 📝 Chat & Documentation Formatting Rule:
* **No LaTeX Math Syntax in Chat:** Do **NOT** use LaTeX math mode (`$...$`, `\rightarrow`, `\frac`, `\approx`, etc.) in chat responses or markdown docs. Use standard UTF-8 symbols (`➔`, `→`, `->`, `+`, `-`, `/`) to prevent broken rendering / missing glyph boxes in IDE chat UI.

---

## 🎮 Git Game-Style Command Rules (ระบบเซฟ / โหลด)
เมื่อผู้ใช้พิมพ์คำสั่งเกี่ยวกับการ "เซฟ", "โหลด" หรือ "จุดเซฟ" ให้แปลเป็นคำสั่ง Git อัตโนมัติดังนี้:

### 1. การเซฟ (Save)
- ถ้าผู้ใช้พิมพ์แค่ **"เซฟ"** หรือ **"บันทึก"** เฉยๆ (ไม่มีชื่อ):
  - ให้ทำการเซฟโดยตั้งชื่อจุดเซฟอัตโนมัติเป็นวันเวลาปัจจุบัน เช่น `เซฟ YYYY-MM-DD HH:mm`
  - ทำการ `git add .` แล้ว `git commit -m "เซฟ YYYY-MM-DD HH:mm"`
  - สรุปสั้นๆ ว่าเซฟไฟล์อะไรไปบ้าง พร้อมแสดง commit hash สั้น
- ถ้าผู้ใช้บอก **"เซฟ"** หรือ **"บันทึก"** พร้อมชื่อ (เช่น `เซฟ เพิ่มระบบค้นหาสินค้า`, `บันทึก v1.2 แก้บั๊กสลิป`):
  1. ให้รันคำสั่ง `git add <ไฟล์>` (เฉพาะไฟล์ที่ระบุ หรือ `git add .` ถ้าไม่ได้ระบุ)
  2. ทำการคอมมิท `git commit -m "<ชื่อที่ผู้ใช้บอก>"`
  3. สรุปสั้นๆ ให้ผู้ใช้ทราบว่าเซฟไฟล์อะไรไปบ้าง พร้อมแสดง commit hash สั้น

### 2. การโหลด / ย้อนกลับ (Load)
- ถ้าผู้ใช้พิมพ์แค่คำว่า **"โหลด"** (โดยไม่ได้ระบุเลขหรือชื่อ):
  - **ให้แสดงรายการจุดเซฟทั้งหมดพร้อมหมายเลขลำดับ (1, 2, 3...) ให้ผู้ใช้เลือกก่อนทันที** พร้อมแนะนำว่าต้องการโหลดหมายเลขใด (เช่น พิมพ์ `โหลด 1` หรือ `โหลด 2`)
- ถ้าผู้ใช้บอก **"โหลด [หมายเลข / รหัส / ชื่อจุดเซฟ]"** (เช่น `โหลด 1`, `โหลด 2`, `โหลด จุดเซฟเริ่มต้น`):
  - ให้อ้างอิงหมายเลขลำดับจากรายการที่แสดงล่าสุด หรือค้นหาจากชื่อจุดเซฟ/รหัส Hash
  - **Action Guard เพื่อความปลอดภัย**: ตรวจสอบ `git status` ก่อนเสมอ หากมีงานค้างที่ยังไม่เซฟ ให้เตือนและถามยืนยันก่อน
  - สรุปผลว่าย้อนกลับไปยังจุดเซฟหมายเลข/ชื่อนั้นเรียบร้อยแล้ว
- ถ้าผู้ใช้บอก **"โหลดไฟล์ [ชื่อไฟล์] กลับมา"**:
  - ให้ดึงไฟล์นั้นจาก commit ล่าสุด (`git checkout HEAD -- <file>`) หรือจาก commit ที่ผู้ใช้ระบุ (`git checkout <hash> -- <file>`)
  - สรุปผลว่าดึงไฟล์กลับมาเรียบร้อยแล้ว
- ถ้าผู้ใช้บอก **"ย้อนโปรเจกต์ทั้งหมดกลับไปจุดเซฟ [ชื่อ/รหัส]"**:
  - ดำเนินการย้อนกลับหลังตรวจสอบความปลอดภัย

### 3. การดูจุดเซฟ (Check Save Points)
- ถ้าผู้ใช้ถามหา **"จุดเซฟ"**, **"ประวัติเซฟ"**, **"ดูจุดเซฟ"**, หรือ **"เช็คจุดเซฟ"**:
  - ให้รันคำสั่งดึงรายการคอมมิท เช่น:
    `git log -n 10 --pretty=format:"%h | %ad | %s" --date=format:"%Y-%m-%d %H:%M"`
  - **ต้องแสดงหมายเลขลำดับ (Index: 1, 2, 3...) นำหน้าเสมอ** เพื่อให้ผู้ใช้สั่งโหลดง่ายๆ เช่น `โหลด 1`, `โหลด 2`
  - รูปแบบตาราง: `[ลำดับ] | [รหัสเซฟ] | [วันเวลา] | [ชื่อจุดเซฟ]` โดยลำดับ 1 คือจุดเซฟล่าสุดเสมอ
