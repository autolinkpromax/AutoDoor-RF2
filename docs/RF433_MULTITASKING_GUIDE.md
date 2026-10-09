# คู่มือการออกแบบและเขียนโปรแกรมสั่งงาน RF 433MHz แบบ Multi-Tasking (FreeRTOS Architecture Guide)

เอกสารฉบับนี้จัดทำขึ้นเพื่อเป็นแนวทางสถาปัตยกรรม (Architecture Blueprint) และไกด์การพัฒนาสำหรับการประมวลผลสัญญาณรีโมต/แท็ก **RF 433MHz (ASK/OOK เช่น EV1527, PT2262, HT6P20B)** ร่วมกับไมโครคอนโทรลเลอร์ (เช่น ESP32) โดยใช้ระบบปฏิบัติการเวลาจริง **FreeRTOS (Multi-Tasking)** เพื่อความเสถียรสูงสุด ตอบสนองได้รวดเร็วระดับมิลลิวินาที และไม่บล็อกการทำงานส่วนอื่นของระบบ (เช่น WebUI, ESP-NOW, Wi-Fi, NVS Flash)

---

## 1. ปัญหาของระบบแบบดั้งเดิม (Single-Thread / Polling Loop)

ในการเขียนโปรแกรมแบบดั้งเดิม (Super-Loop/Polling):
1. **สัญญาณ RF 433MHz มีความละเอียดระดับ Microsecond ($\mu s$):** คลื่น RF แบบ ASK/OOK ส่งข้อมูลด้วยการสลับระดับสัญญาณ HIGH/LOW ในช่วง $100\mu s - 2000\mu s$ หาก CPU กำลังทำงานอื่นอยู่ เช่น บันทึก Flash, ประมวลผล JSON, หรือส่งข้อมูล Wi-Fi จะเกิด Pulse timing jitter หรือถอดรหัสผิดพลาด
2. **Noise จากตัวรับ (SYN480R / RXB6 / MX-05V):** เมื่อไม่มีการส่งสัญญาณ คอนเดนเซอร์ AGC (Automatic Gain Control) ของตัวรับจะเพิ่มขยายสัญญาณจนเกิด Thermal Noise (Spikes ระดับ <30µs) ถ้าระบบใช้ Polling ใน `loop()` จะเสีย CPU cycles ไปกับการเช็ก Noise ตลอดเวลา
3. **การทำงานหนักใน ISR ทำให้ระบบ Crash:** หากใส่ Logic ซับซ้อน (เช่น `Serial.print`, บันทึก NVS, สั่งเปิดประตู) ไว้ใน Interrupt Service Routine (ISR) โดยตรง จะทำให้เกิด Stack Overflow, Watchdog Timer (WDT) Reset หรือระบบล็อก

---

## 2. สถาปัตยกรรม Multi-Tasking 3 เลเยอร์ (3-Layer Architecture Blueprint)

เพื่อให้ระบบทำงานแบบ Real-Time, Non-blocking และ Thread-Safe เราแบ่งสถาปัตยกรรมออกเป็น 3 เลเยอร์หลัก:

```
┌────────────────────────────────────────────────────────────────────────┐
│                     Layer 1: Hardware ISR & Hardware Timer             │
│  - Captures GPIO Edges (micros) via attachInterrupt()                  │
│  - Filters Glitch (<30us) & Stale Frame Timeout (>100ms)               │
│  - Demodulates EV1527/PT2262 pulse timing in IRAM                      │
└──────────────────────────────────┬─────────────────────────────────────┘
                                   │ xQueueSendFromISR() [Non-blocking]
                                   ▼
┌────────────────────────────────────────────────────────────────────────┐
│                     Layer 2: FreeRTOS Task (rf433Task)                 │
│  - Priority 5 Task (8KB Stack) blocked on xQueueReceive(portMAX_DELAY) │
│  - Consumes RF codes async without blocking ISR or main loop          │
│  - Handles Learn Mode logic & candidate verification                   │
└──────────────────────────────────┬─────────────────────────────────────┘
                                   │ TagStoreLock (Mutex Guard)
                                   ▼
┌────────────────────────────────────────────────────────────────────────┐
│                     Layer 3: Thread-Safe State & Action                │
│  - Thread-Safe Tag Store lookup (findTagByRfCode)                       │
│  - Door Control Execution (executeDoorSlot) & Debouncing               │
│  - Coexistence Management with ESP-NOW / Wi-Fi                         │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 3. รายละเอียดการทำงานของแต่ละเลเยอร์

### Layer 1: Hardware Interrupt Service Routine (ISR) & Demodulation
- **หน้าที่:** จับขอบสัญญาณขากลับ/ขาขึ้นของพิน RF ข้อมูลด้วย `attachInterrupt(CHANGE)`
- **ข้อกำหนดสำคัญ:**
  - ต้องทำงานอยู่ใน IRAM (`IRAM_ATTR`) เพื่อความเร็วสูงสุด
  - ห้ามใช้ฟังก์ชั่นบล็อก เช่น `delay()`, `Serial.printf()` หรือการจองหน่วยความจำ (`malloc`)
  - **Glitch Filter:** กรอง Pulse สั้นกว่า $30\mu s$ (AGC Thermal Noise) ทิ้งทันที
  - **Stale Timeout Reset:** หากช่วงว่างระหว่าง Pulse เกิน $100ms$ ให้รีเซ็ต Bit Buffer ทิ้งเพื่อป้องกันเฟรมค้างชำรุด
  - **Non-Blocking Handover:** เมื่อถอดรหัสได้รหัส RF 32-bit สมบูรณ์ ให้ส่งรหัสเข้า **FreeRTOS Queue** ผ่าน `xQueueSendFromISR` และเรียก `portYIELD_FROM_ISR` เพื่อให้ Task สลับมารับงานทันที

#### ตัวอย่างการส่งข้อมูลจาก ISR ลง Queue:
```cpp
// ส่งข้อมูลแบบ Non-blocking จาก ISR สู่ Queue
void IRAM_ATTR postRfCodeFromISR(uint32_t value) {
  if (gRfCmdQueue == NULL) return;

  const uint32_t nowMs = millis();
  // Filter out identical burst repeat frames arriving within 100ms at ISR level
  if (!gLearn.active && value == sLastIsrValue && (uint32_t)(nowMs - sLastIsrMs) < 100) {
    return;
  }
  sLastIsrValue = value;
  sLastIsrMs = nowMs;

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(gRfCmdQueue, &value, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}
```

---

### Layer 2: Async FreeRTOS Queue & Worker Task (`rf433Task`)
- **หน้าที่:** รอดักรับรหัส RF จาก Queue และประมวลผล Business Logic ในสภาวะ Multi-tasking
- **ข้อกำหนดสำคัญ:**
  - สร้าง Queue ขนาดเหมาะสม (เช่น `xQueueCreate(32, sizeof(uint32_t))`)
  - สร้าง Task ด้วย `xTaskCreate` กำหนด Priority สูง (เช่น Priority 5) และ Stack Size ที่พอเพียง (8KB) เพื่อรองรับการบันทึก NVS Flash / Log
  - Task จะเข้าสู่สถานะ **Blocked** ผ่าน `xQueueReceive(..., portMAX_DELAY)` เมื่อไม่มีข้อมูล ทำให้ **กิน CPU Load 0%** ขณะสะกดรอดักสัญญาณ

#### ตัวอย่างโครงสร้าง FreeRTOS Task:
```cpp
void rf433Task(void* pvParameters) {
  (void)pvParameters;
  uint32_t value = 0;
  for (;;) {
    // รอดักรับข้อมูลจาก Queue โดยไม่กิน CPU
    if (xQueueReceive(gRfCmdQueue, &value, portMAX_DELAY) == pdTRUE) {
      processRfCode(value); // ประมวลผลตรรกะ Tag
    }
  }
}

void initRf433() {
  initTagStoreMutex();
  gRfCmdQueue = xQueueCreate(32, sizeof(uint32_t));
  // สร้าง Task Priority 5 ขนาด Stack 8KB
  xTaskCreate(rf433Task, "rf433Task", 8192, NULL, 5, NULL);
  SYN480R::begin(kRf433RxPin);
}
```

---

### Layer 3: Thread-Safe State & Mutex Management
- **หน้าที่:** ตรวจสอบข้อมูล Tag, ทำการบันทึก Tag ใหม่ (Learn Mode) หรือสั่งการประตู/อุปกรณ์
- **ข้อกำหนดสำคัญ:**
  - **Mutex Protection:** เนื่องจากข้อมูล Tag สถิตอยู่ใน RAM (`gTags`) ซึ่งถูกเข้าถึงโดยหลาย Task พร้อมกัน (WebUI Task, ESP-NOW Task, RF433 Task) จึงต้องใช้ Mutex (`SemaphoreHandle_t` หรือ RAII `TagStoreLock`) ครอบไว้ทุกครั้งที่มีการ อ่าน/เขียน
  - **Execution Debouncing:** รีโมต RF มักส่งสัญญาณซ้ำ 3-10 เฟรมต่อการกด 1 ครั้ง ต้องทำระบบ Debounce (เช่น 350ms) เพื่อป้องกันการสั่งงานประตูซ้ำซ้อน

#### ตัวอย่างการจัดการ Thread-Safety ด้วย RAII Lock:
```cpp
struct TagStoreLock {
  TagStoreLock() {
    if (gTagStoreMutex) xSemaphoreTake(gTagStoreMutex, portMAX_DELAY);
  }
  ~TagStoreLock() {
    if (gTagStoreMutex) xSemaphoreGive(gTagStoreMutex);
  }
};

void processRfCode(uint32_t value) {
  if (value == 0 || value == 0x00FFFFFF || value == 0xFFFFFFFF) return;

  // ป้องกัน Race Condition ระหว่าง RF Task และ WebUI/NVS Task
  TagStoreLock lock;
  const uint32_t now = millis();

  if (gLearn.active) {
    // Logic การเรียนรู้ Tag ใหม่ (Learn Mode)
  } else {
    Tag* tag = findTagByRfCode(value);
    if (tag) {
      // Debounce 350ms ป้องกันการกดครั้งเดียวแล้วสั่งเปิดสองรอบ
      static uint32_t lastExecGlobalMs = 0;
      static uint32_t lastExecTagValue = 0;
      if (value != lastExecTagValue || (uint32_t)(now - lastExecGlobalMs) >= 350) {
        lastExecGlobalMs = now;
        lastExecTagValue = value;
        executeDoorSlot("open", false); // สั่งการทำงาน
      }
    }
  }
}
```

---

## 4. เทคนิคชั้นสูงและการจัดการ Edge Cases (Advanced Techniques)

### 4.1 Learn Mode Verification Pattern (ป้องกัน RF Noise ขยะ)
ในการเรียนรู้รีโมตใหม่ สัญญาณรบกวนในอากาศอาจสุ่มได้รหัสปลอม หากนำรหัสนั้นไปเซฟลง NVS Flash ทันทีจะทำให้ความจำเต็มและทำงานผิดพลาด
- **แนวทางแก้ไข:** กำหนดเงื่อนไข Candidate Verification โดยรีโมตจริงจะต้องส่งรหัส **ตรงกันอย่างน้อย 2 เฟรมต่อเนื่องภายใน 500ms** ก่อนจะอนุมัติสร้าง Tag และบันทึกลง Flash

```cpp
// ตรวจสอบ Candidate ว่าส่งรหัสเดิมซ้ำ 2 ครั้งใน 500ms หรือไม่
if (value != sLearnCandidateValue || (uint32_t)(now - sLearnCandidateLastMs) > 500) {
  sLearnCandidateValue = value;
  sLearnCandidateHits = 1;
  sLearnCandidateLastMs = now;
  return; // รอเฟรมถัดไปจากรีโมต
}

sLearnCandidateHits++;
if (sLearnCandidateHits >= 2) {
  // รหัสผ่านการยืนยันแล้ว -> ดำเนินการสร้าง Tag และเซฟลง NVS Flash
}
```

### 4.2 Smart Radio Coexistence (หลีกเลี่ยงคลื่นกวนจาก ESP-NOW / Wi-Fi)
โมดูล Wi-Fi / ESP-NOW ของ ESP32 มีกำลังส่งสูง (20dBm) ซึ่งอาจรบกวนภาครับ SYN480R (433MHz) ผ่านทางสายหรือคลื่นแม่เหล็กไฟฟ้าขณะกำลังรับข้อมูล
- **แนวทางแก้ไข:** ใช้สถานะ `SYN480R::isReceiving()` เช็กว่าภาครับ 433MHz กำลังรับเฟรมข้อมูลอยู่หรือไม่ หากกำลังรับอยู่ ให้หน่วงเวลาการส่ง packet ของ ESP-NOW / Wi-Fi ออกไป 100ms เพื่อไม่ให้ไปบดบังคลื่น RF433

```cpp
if (SYN480R::isReceiving()) {
  // ชะลอการส่ง ESP-NOW ออกไป 100ms เพื่อไม่ให้คลื่น Wi-Fi ไปบดหนังสัญญาณ RF433
  vTaskDelay(pdMS_TO_TICKS(100));
}
```

---

## 5. ตารางเปรียบเทียบสถาปัตยกรรม (Architecture Comparison Matrix)

| คุณสมบัติ | Single-Loop Polling | FreeRTOS Multi-Tasking (แนะนำ) |
| :--- | :--- | :--- |
| **Response Latency** | ช้า ($50ms - 500ms$) ขึ้นกับงานใน `loop()` | เร็วมาก ($<1ms - 5ms$) ตอบสนองทันทีด้วย ISR Queue |
| **CPU Utilization** | สูง ($100\%$ CPU Busy Loop) | ต่ำมาก ($0\%$ ขณะ Idle ด้วย `portMAX_DELAY`) |
| **Frame Loss Rate** | สูงเมื่อมีการทำงานหนัก (Flash/Wi-Fi) | แทบเป็น 0% (ISR ดักจับระดับ microsecond) |
| **Thread Safety** | ไม่มี (เสี่ยงต่อ Data corruption) | สูง (มี FreeRTOS Queue & Mutex ป้องกัน) |
| **ความซับซ้อนของโค้ด** | ต่ำ | ปานกลาง (ต้องเข้าใจ ISR, Queue, Mutex) |

---

## 6. โครงสร้างไฟล์ในโครงการอ้างอิง (Reference Code Map)

สามารถศึกษาโค้ดจริงในโปรเจกต์ได้จากไฟล์ดังต่อไปนี้:
- [lib/rflib/src/SYN480R.h](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/lib/rflib/src/SYN480R.h) และ [SYN480R.cpp](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/lib/rflib/src/SYN480R.cpp): การถอดรหัสระดับ Pulse Timing ใน ISR
- [src/modules/rf433_runtime.h](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/modules/rf433_runtime.h) และ [rf433_runtime.cpp](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/modules/rf433_runtime.cpp): การจัดการ FreeRTOS Task, Queue และ Business Logic
- [src/modules/tag_store.h](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/modules/tag_store.h): การใช้ Mutex Guard ป้องกัน Race Condition

---

## 7. เทคนิคการเพิ่มความไวรับสัญญาณรีโมต (RF Reception Optimization & Sensitivity Tuning)

หากต้องการให้ระบบรับสัญญาณรีโมตได้ไกลขึ้น ชัดเจนขึ้น และทนต่อสัญญาณรบกวนได้ดี โดย**ไม่ทำให้ FreeRTOS Task พังหรือระบบล่ม** สามารถประยุกต์ใช้เทคนิคต่อไปนี้:

### 7.1 ปรับย่าน Dynamic Duty-Cycle Thresholding (ระดับ ISR)
- **ปัญหา:** สัญญาณรีโมตในระยะไกลจะถูกลดทอนและเกิดความบิดเบี้ยว (Pulse Shrinking) ความกว้างช่วง HIGH ของ Bit 1 (ปกติ 75%) อาจหดเหลือเพียง 42%–48% หากใช้ Threshold ที่ 50% (`highPercent >= 50`) สัญญาณ Bit 1 จะถูกแยกแยะเป็น Bit 0 ทันที ทำให้ถอดรหัสล้มเหลว
- **แนวทางแก้ไข:** ปรับค่า Threshold สลับเป็น `highPercent >= 40` หรือช่วง 38%–42% เพื่อแยกแยะระหว่าง Bit 0 (ปกติ 25%) และ Bit 1 (ปกติ 75%) ได้กว้างและแม่นยำขึ้นแม้สัญญาณจะอ่อนแรง

```cpp
// ปรับปรุงใน ISR (SYN480R.cpp)
const uint32_t highPercent = (_highDuration * 100) / bitDuration;
// แยก Bit 1 (3T HIGH ~ 75%) และ Bit 0 (1T HIGH ~ 25%) ด้วย Threshold 40%
const uint8_t bitVal = (highPercent >= 40) ? 1 : 0;
```

### 7.2 ระบบ Majority Voting / Candidate Window (ระดับ Task)
- **ปัญหา:** ในระยะไกล สัญญาณ 24 บิตอาจมีบิตหลุดหรือเพี้ยนเพียง 1 บิตเนื่องจากคลื่นกวนในอากาศ ถ้าตัดทิ้งทันทีจะทำให้ต้องกดรีโมตหลายครั้ง
- **แนวทางแก้ไข:** ให้ Task ทำงานแบบ **Soft Majority Voting** หรือ Sliding Window 200ms โดยไม่ไปชะลอ ISR:
  - รีโมต 1 ครั้งจะส่ง burst ออกมา 4–10 เฟรม
  - เมื่อ Task ได้รับรหัส candidate เข้ามา ให้เปรียบเทียบกับ candidate ล่าสุดในระยะเวลา <200ms หากพบรหัสตรงกัน 2 ใน 3 เฟรม หรือรหัสตรงกับ Tag ID ในฐานข้อมูล ให้ยอมรับการเปิดประตูทันที

### 7.3 นโยบายป้องกัน ISR / Queue พัง (Task Stability Safeguards)
1. **Drop-on-Full Policy ใน Queue:** ใน ISR ห้ามใช้ `xQueueSendFromISR` แบบรอคิว หาก Queue เต็ม (เช่น ถูกคลื่นกวนกระหน่ำส่ง Pulse ขยะเข้ามาพันๆ ครั้ง) ให้ทิ้งข้อมูลทันทีเพื่อป้องกัน ISR ล็อก
2. **Isolate Stack & Priority:**
   - กำหนด Stack Size ของ `rf433Task` ให้ไม่ต่ำกว่า 4KB - 8KB เพื่อป้องกัน Stack Overflow เมื่อเกิด Interrupt ความถี่สูง
   - ตั้ง Priority ของ `rf433Task` (เช่น Priority 5) ให้สูงกว่า Task ประมวลผลทั่วไป (เช่น WebUI Priority 1-2) เพื่อประมวลผลรหัสใน Queue ได้หมดทันที ไม่สะสมค้างใน RAM
3. **Smart Coexistence หน่วงการส่ง Radio สัญญาณสูง:** ขณะที่ ESP32 ส่ง Wi-Fi / ESP-NOW ด้วยกำลังส่ง 20dBm จะเกิด Radio Crosstalk ไปยังโมดูลรับ SYN480R การใช้ `SYN480R::isReceiving()` ชะลอแพ็กเก็ต Wi-Fi ออกไป 50-100ms จะช่วยเพิ่มระยะรับรีโมตได้มากกว่า 50%

### 7.4 การปรับแต่งทางกายภาพและฮาร์ดแวร์ (Hardware Sensitivity)
- **เสาอากาศ (Antenna):** ใช้สายทองแดงเดี่ยวความยาว **17.3 ซม.** (1/4 Wavelength ของ 433.92MHz) ต่อเข้าขั้ว ANT ของโมดูลรับ SYN480R วางตั้งฉากแนวตั้ง
- **Decoupling Capacitor:** เชื่อมต่อ C-Filter ขนาด $100nF$ ขนานกับ $10\mu F$ ที่พิน VCC และ GND ของโมดูลรับ RF เพื่อกรอง Ripple Voltage จากสวิตชิ่งพาวเวอร์ซัพพลาย

---

## 8. Optimization Changelog (บันทึกการปรับแต่งค่า)

บันทึกค่าที่ถูกปรับเปลี่ยนและเหตุผลเชิงเทคนิค เพื่อให้ AI หรือนักพัฒนาอ่านเข้าใจภายหลัง:

### 8.1 ISR Burst Filter: 100ms → 40ms
- **ไฟล์:** `src/modules/rf433_runtime.cpp` → `postRfCodeFromISR()`
- **ค่าเดิม:** กรองรหัส RF ซ้ำภายใน 100ms ทิ้งทั้งหมด
- **ค่าใหม่:** 40ms
- **เหตุผล:** รีโมต EV1527 ส่ง burst 4-10 เฟรมใน ~200ms (gap ระหว่างเฟรม ≈ 8-15ms) ค่า 100ms เข้มเกินไป — ถ้าเฟรมแรกถอดรหัสเพี้ยนจะไม่มีเฟรมสำรองเลย ลดเหลือ 40ms ยังป้องกันเฟรมซ้ำติดกัน (echo) แต่ให้เฟรมที่ 2-3 ผ่านเข้า Queue เป็น backup
- **ผลกระทบ thread:** ไม่มี — ทำงานใน ISR context ไม่เกี่ยวกับ FreeRTOS scheduler

### 8.2 ISR Duplicate Frame Filter: 40,000µs → 25,000µs
- **ไฟล์:** `lib/rflib/src/SYN480R.cpp` → `isNewFrameOrExpired()`
- **ค่าเดิม:** บล็อกรหัสเดิมภายใน 40ms (40,000µs) ที่ระดับ ISR decoder
- **ค่าใหม่:** 25ms (25,000µs)
- **เหตุผล:** ค่านี้ซ้ำซ้อนกับ burst filter ใน `postRfCodeFromISR` → สร้าง "Double Filtering" ลดเหลือ 25ms เพื่อ debounce เฉพาะกรณี decoder อ่านเฟรมเดียวกันซ้ำ 2 ครั้ง (echo จาก sync pulse ที่ทับซ้อน)
- **ผลกระทบ thread:** ไม่มี — อยู่ใน ISR context เท่านั้น

### 8.3 Mutex Scope Narrowing ใน processRfCode()
- **ไฟล์:** `src/modules/rf433_runtime.cpp` → `processRfCode()`
- **ค่าเดิม:** `TagStoreLock` ครอบทั้งฟังก์ชัน (ทั้ง learn path และ normal path)
- **ค่าใหม่:**
  - **Learn path:** ทำ candidate verification ก่อน (ไม่ใช้ mutex) → ล็อกเฉพาะตอน create tag + NVS write
  - **Normal path:** ใช้ `xSemaphoreTakeRecursive` แบบ timeout 50ms แทน `portMAX_DELAY` → ถ้า WebUI/NVS ถือ lock นานกว่า 50ms ให้ skip เฟรมนี้แทนการบล็อก (เฟรมถัดไปจะมาจาก burst)
  - ปล่อย mutex ก่อน `executeDoorSlot()` เพื่อลดเวลาถือ lock
- **เหตุผล:** ลด mutex contention — เมื่อ WebUI กำลัง save tag ลง NVS (10-50ms) rf433Task จะไม่ถูกบล็อก
- **ผลกระทบ thread:** ปลอดภัย — ยังใช้ recursive mutex เดิม, ไม่เปลี่ยน mutex type

### 8.4 Task Creation: คง xTaskCreate (ไม่ใช้ xTaskCreatePinnedToCore)
- **ไฟล์:** `src/modules/rf433_runtime.cpp` → `initRf433()`
- **สิ่งที่คง:** `xTaskCreate` (FreeRTOS เลือก core เอง)
- **เหตุผล:** ESP32-C3 เป็น **single-core RISC-V** มีแค่ Core 0 เท่านั้น — การเรียก `xTaskCreatePinnedToCore(..., 1)` จะทำให้ `configASSERT` fail แล้ว **crash ทันที** เพราะ Core 1 ไม่มีอยู่จริง
- **หมายเหตุ:** หากย้ายไป ESP32 dual-core ในอนาคต ค่อยเปลี่ยนเป็น `xTaskCreatePinnedToCore(..., 1)` เพื่อ pin ไว้ Core 1 (แยกจาก Wi-Fi stack ที่อยู่ Core 0)

### 8.5 สรุป Timing Flow หลังปรับแต่ง
```
Remote Press → RF Burst (4-10 frames, gap ~10ms each)
    │
    ├─ Frame 1: ISR decode → isNewFrameOrExpired(≥25ms) ✓ → postRfCodeFromISR(≥40ms) ✓ → Queue
    ├─ Frame 2 (+10ms): blocked by 25ms dedup
    ├─ Frame 3 (+20ms): blocked by 25ms dedup  
    ├─ Frame 4 (+30ms): blocked by 40ms burst filter
    ├─ Frame 5 (+40ms): passes both filters → Queue (BACKUP FRAME)
    ├─ Frame 6 (+50ms): blocked by 25ms dedup
    └─ ...
    
Queue → rf433Task (Priority 5)
    ├─ Mutex timeout 50ms (ไม่บล็อกถ้า WebUI ถือ lock)
    ├─ findTagByRfCode() → Tag lookup
    ├─ Release mutex ก่อน executeDoorSlot()
    └─ 350ms debounce → executeDoorSlot("open")
    
กดรีโมท 1 ครั้ง → ได้ ~2-3 เฟรมเข้า Queue → ถ้าเฟรมแรก mutex timeout ก็ยังมีเฟรมสำรอง
```
