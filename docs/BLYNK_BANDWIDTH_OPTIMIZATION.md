# แนวทางการลดการสื่อสารและประหยัด Bandwidth ของ Blynk Cloud (Blynk Bandwidth & Stability Optimization)

เอกสารฉบับนี้วิเคราะห์และสรุปแนวทางการปรับแต่งระบบสื่อสารกับ **Blynk Cloud** ในโปรเจกต์ AutoDoor-RF2 เพื่อให้ **ประหยัด Bandwidth และ Quota มากที่สุด (ลดลง 70% – 90%)** โดยที่โปรแกรมยังคง **ทำงานราบรื่น ไม่แฮงค์ ไม่หน่วง และไม่หลุดจากการเชื่อมต่อ**

---

## 1. การวิเคราะห์จุดสิ้นเปลือง Bandwidth และสาเหตุโปรแกรมหน่วง/แฮงค์

ในการเชื่อมต่อ Blynk ตามมาตรฐาน:
1. **Heartbeat Pings (Ping ประจำ):** โดยปกติ Blynk Library จะส่งแพ็กเก็ต Ping ทุกๆ 10 วินาที (`BLYNK_HEARTBEAT 10`) เพื่อรักษาการเชื่อมต่อ ซึ่งกิน Bandwidth ตลอดเวลา 24 ชั่วโมงแม้ไม่มีการสั่งงานประตู
2. **การทำ Blocking Connect เมื่อเน็ตหลุด:** ฟังก์ชัน `Blynk.connect(3000)` จะบล็อกการทำงานของ CPU นานถึง 3,000 มิลลิวินาที (3 วินาที) หากเน็ตช้าหรือเซิร์ฟเวอร์ตอบสนองช้า ทำให้ Loop หลักหรือการกดปุ่ม/รีโมตประตูเกิดอาการค้าง (Lag/Freeze)
3. **การส่งข้อมูลซ้ำซ้อน (Redundant Telemetry):** หากมีการใช้ Timer ส่งค่าสถานะประตูทุกๆ 1 วินาทีโดยไม่เช็กว่าสถานะเปลี่ยนหรือไม่ จะเสีย Quota และ Bandwidth โดยไม่จำเป็น

---

## 2. 5 สถาปัตยกรรมประหยัด Bandwidth สูงสุดโดยไม่ทำให้ระบบค้าง

### 2.1 ขยายช่วง Heartbeat (`BLYNK_HEARTBEAT 30`)
- **วิธีการ:** กำหนดค่า `#define BLYNK_HEARTBEAT 30` ก่อนอิมพอร์ต `#include <BlynkSimpleEsp32.h>`
- **ผลลัพธ์:** เปลี่ยนจากการส่ง Ping ทุก 10 วินาที เป็นส่งทุก **30 วินาที**
- **ประหยัด Bandwidth:** **ลดลงทันที 66%** สำหรับการเชื่อมต่อสแตนด์บาย โดย 30 วินาทียังอยู่ในย่านปลอดภัยที่ NAT Router/Firewall ทั่วไปจะไม่ตัดการเชื่อมต่อ TCP

```cpp
// กำหนดไว้บนสุดของไฟล์ blynk_service.cpp
#define BLYNK_HEARTBEAT 30     // ส่ง Ping ทุก 30s แทน 10s (ลด Bandwidth 66%)
#define BLYNK_NO_BUILTIN       // ลดขนาด Binary และหน่วยความจำ
#include <BlynkSimpleEsp32.h>
```

---

### 2.2 ส่งข้อมูลแบบ Event-Driven 100% + ซิงค์สำรองทุก 30 นาที (Quota Protection)
- **หลักการสำคัญ:** ห้ามใช้ Timer ส่งข้อมูลสถานะแบบวนลูปถี่ๆ เด็ดขาด (เช่น การส่งทุก 20 วินาที จะทำให้เกิด 4,320 ข้อความ/วัน หรือ 129,600 ข้อความ/เดือน ซึ่งทะลุโควต้า 100,000 ข้อความ/เดือนของ Blynk ทันที!)
- **วิธีการที่ถูกต้อง (Event-Driven):**
  1. ส่ง `Blynk.virtualWrite()` ทันทีเมื่อเกิดการเปลี่ยนสถานะจริง (State Transition Only) เช่น ปิดสนิท -> กำลังเปิด -> เปิดอยู่ -> กำลังปิด -> ปิดสนิท (เฉลี่ยเพียง 4 ข้อความต่อ 1 รอบการใช้งานประตู)
  2. เมื่อ Reconnect สำเร็จ (`BLYNK_CONNECTED()`): ส่งสถานะปัจจุบันทันที 1 ครั้ง เพื่อซิงค์กับ Blynk Cloud
  3. ตั้งรอบส่งสำรอง (Periodic Safety Net Sync) ห้ามเร็วกว่า **ทุก 30 นาที (1,800,000 ms)** หรือ 1 ชั่วโมง เพื่อป้องกันข้อมูลตกหล่น โดยการส่งทุก 30 นาทีจะใช้เพียง 48 ข้อความ/วัน (~1,440 ข้อความ/เดือน) ซึ่งคิดเป็นเพียง 1.4% ของโควต้าเท่านั้น

```cpp
// ส่งข้อมูลเมื่อสถานะเปลี่ยนจริง หรือครบกำหนดรอบสำรอง 30 นาที
static uint32_t lastPeriodicSyncMs = 0;
if (lastPeriodicSyncMs == 0) {
  lastPeriodicSyncMs = now;
} else if ((uint32_t)(now - lastPeriodicSyncMs) >= 1800000UL) { // ทุก 30 นาที
  lastPeriodicSyncMs = now;
  gBlynkNeedForceSync = true;
}

if (String(doorThai) != gBlynkLastDoorText || gBlynkNeedForceSync) {
  Blynk.virtualWrite(V4, doorThai);
  gBlynkLastDoorText = String(doorThai);
  lastWriteMs = now;
  gBlynkNeedForceSync = false;
  return;
}
```

---

### 2.3 ป้องกันโปรแกรมแฮงค์ด้วย Non-Blocking Connection (Async / Non-blocking)
- **ปัญหาเดิม:** `Blynk.connect(3000)` บล็อก CPU 3 วินาทีเมื่ออินเทอร์เน็ตหลุด ทำให้ประตูเปิดไม่ทัน หรือปุ่มกดตอบสนองช้า
- **แนวทางแก้ไข:**
  1. ใช้ Timeout สั้นมาก เช่น `Blynk.connect(300)` ร่วมกับระบบ Retry เป็นระยะ (เช่น พยายามต่อทุกๆ 30 วินาที)
  2. ให้งานควบคุมประตูและเซนเซอร์ทำงานอิสระ ไม่ถูกหน่วงโดยการเชื่อมต่อของ Blynk

```cpp
// Non-blocking Reconnect ใน stepBlynk()
if (!Blynk.connected()) {
  gBlynkConnected = false;
  // ลองเชื่อมต่อทุกๆ 30 วินาที โดยใช้ Timeout เพียง 300ms
  if ((uint32_t)(now - gBlynkLastConnectAttemptMs) >= 30000) {
    gBlynkLastConnectAttemptMs = now;
    if (WiFi.status() == WL_CONNECTED && WiFi.localIP() != INADDR_NONE) {
      Blynk.connect(300); // Non-blocking connect timeout 300ms
    }
  }
  return;
}
```

---

### 2.4 ป้องกัน Blynk Cloud Throttling / Rate Limit
- Blynk Cloud มีโควตากำหนดห้ามส่งข้อมูลถี่เกินไป (มักไม่เกิน 10 ครั้ง/วินาที)
- **การป้องกัน:** ใส่ Rate Limit Interlock เพื่อเว้นระยะห่างระหว่างการสั่ง `Blynk.virtualWrite()` อย่างน้อย 100 ms ป้องกันกรณีมี Event เกิดขึ้นพร้อมๆ กันแล้วถูก Blynk Cloud บล็อก IP ชั่วคราว

---

### 2.5 ทางเลือกสูงสุด: Blynk HTTP REST API Mode (0% Background Bandwidth)
หากต้องการประหยัด Bandwidth 100% ขณะไม่ได้ใช้งาน (0 KB สแตนด์บาย):
- **สถาปัตยกรรม:** ปิดการเชื่อมต่อ persistent TCP ลูป `Blynk.run()`
- **การควบคุมเข้า:** ใช้ Blynk Webhooks หรือ Home Assistant สั่งการมาที่เครื่องผ่าน HTTP GET/POST API (`http://<ip>/api/door/open`)
- **การส่งออก:** เมื่อมีการเปิดประตู ESP32 จะยิง HTTP GET เล็กๆ ไปยัง Blynk Cloud REST API (`https://blynk.cloud/external/api/update?token=xxx&v4=Opened`) แล้วปิด Socket ทันที
- **ข้อดี:** ไม่มีแพ็กเก็ต Ping สแตนด์บายเลยแม้แต่ไบต์เดียว ประหยัดแบนด์วิธและโควตา Blynk 99.9%

---

## 3. ตารางเปรียบเทียบโควต้าและการสื่อสารกับ Blynk

| รูปแบบการเชื่อมต่อ | ปริมาณข้อความต่อวัน | ปริมาณข้อความต่อเดือน | % ของโควต้า 100k | ความเหมาะสม |
| :--- | :--- | :--- | :--- | :--- |
| **แบบเดิม (Loop ทุก 20s)** | 4,320 ข้อความ | **~129,600 ข้อความ** | **> 129% (โควต้าเต็ม/ถูกตัด)** | ❌ ห้ามใช้เด็ดขาด |
| **Event-Driven + สำรองทุก 30 นาที (ปัจจุบัน)** | **~120 ข้อความ** | **~3,800 ข้อความ** | **~3.8% (ประหยัด 97%)** | ✅ **แนะนำที่สุด ปลอดภัย 100%** |
| **Pure Event-Driven (ไม่มีลูปสำรอง)** | ~80 ข้อความ | ~2,400 ข้อความ | ~2.4% (ประหยัด 98%) | ✅ ดีมาก สำหรับประหยัดสูงสุด |
| **Blynk HTTP REST API Mode** | ~80 ข้อความ | ~2,400 ข้อความ | ~2.4% (แบนด์วิธสแตนด์บาย 0 KB) | 💡 เหมาะกับเน็ตซิมคิดตามปริมาณข้อมูล |

---

## 4. โครงสร้างซอร์สโค้ดอ้างอิงในโปรเจกต์

สามารถดูโครงสร้างการทำงานปัจจุบันของ Blynk ได้ที่:
- [src/modules/blynk_service.h](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/modules/blynk_service.h)
- [src/modules/blynk_service.cpp](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/modules/blynk_service.cpp)
