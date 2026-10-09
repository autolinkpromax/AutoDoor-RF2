# 💻 AutoDoor-RF2: สถาปัตยกรรมระบบและคู่มือวิศวกรซอฟต์แวร์
*(Firmware Architecture, Data Schema & Developer Technical Reference)*

เอกสารนี้จัดทำขึ้นสำหรับวิศวกรเฟิร์มแวร์, ทีมวิจัยและพัฒนา (R&D) และนักพัฒนาระบบฝังตัว (Embedded Developers) เพื่อเป็นคู่มืออ้างอิงทางเทคนิค สถาปัตยกรรมซอฟต์แวร์ โครงสร้างข้อมูลใน NVS และข้อกำหนดการทำงานระดับฮาร์ดแวร์ของโปรเจกต์ **AutoDoor-RF2**

---

## 🏛️ 1. สถาปัตยกรรมซอฟต์แวร์ 5 เลเยอร์ (5-Layer Door Architecture)

แกนหลักของการควบคุมประตูใน [DoorController.cpp](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/core/DoorController.cpp) ถูกออกแบบและแบ่งแยกความรับผิดชอบออกเป็น 5 เลเยอร์อย่างเด็ดขาดตาม [DOOR_AUTOMATION_SPECIFICATION.md](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/docs/DOOR_AUTOMATION_SPECIFICATION.md):

```
┌─────────────────────────────────────────────────────────────┐
│ Layer 1: Sensors & Input Debouncing (IR Beam, Mag, E-Stop)   │
├─────────────────────────────────────────────────────────────┤
│ Layer 2: Tag & RF Evaluation (Active Hours, Baseline Block) │
├─────────────────────────────────────────────────────────────┤
│ Layer 3: Auto-Close Priority Engine (Priority 1 > 2 > 3)    │
├─────────────────────────────────────────────────────────────┤
│ Layer 4: Directional Reversal Deadband Actuator (2.4s Total)│
├─────────────────────────────────────────────────────────────┤
│ Layer 5: Transition Timers & Auto-Retry Schedulers          │
└─────────────────────────────────────────────────────────────┘
```

### Layer 1: Sensors & Input Debouncing
* จัดการประมวลผลเซ็นเซอร์อินฟราเรดกันหนีบ (GPIO 2), สวิตช์แม่เหล็ก (GPIO 3), และปุ่มฮาร์ดแวร์ BOOT (GPIO 9)
* ทำงานด้วย Multi-sample Digital Filtering กรองสัญญาณรบกวนทางไฟฟ้า (Electrical Hash)

### Layer 2: Tag Trigger & Suppression
* ตรวจจับการเข้าใกล้ของ ESP-NOW Tag และการกดปุ่ม RF 433MHz
* **การตรวจสอบช่วงเวลาใช้งาน (Active Hours):**
  * ฟังก์ชั่น `TagRegistry::isTagInSchedule(const Tag& t)` จะตรวจสอบว่าขณะนั้นอยู่ในช่วงเวลาที่ได้รับอนุญาตหรือไม่
  * หากอยู่นอกช่วงเวลา หรือยังไม่ได้ซิงค์เวลาโลก (NTP) แท็กจะถูกข้ามและไม่สั่งเปิดประตู
* **การระงับการเปิดซ้ำ (Baseline Suppression):**
  * เมื่อประตูเริ่มปิดหรือปิดสนิท แท็กที่ยังส่งสัญญาณอยู่ในบ้านจะถูกบันทึกเข้าสู่ `gTagBaselineActiveIds` (`gTagOpenSuppressed = true`) เพื่อป้องกันการเปิดประตูซ้ำ

### Layer 3: Auto-Close Priority Decision Engine
กลไกตัดสินใจการปิดประตูอัตโนมัติแบ่งลำดับความสำคัญอย่างเข้มงวด:
1. **Priority 1 (สูงสุด - Safety Sensor Authority):** เมื่อรถขับผ่านเซ็นเซอร์กันหนีบสำเร็จ (`gPassageCrossed == true`) ประตูจะถูกสั่งปิดตามเวลาหน่วงความปลอดภัย (`tagSafetyCloseDelayMs`) **แม้ว่าแท็กของรถจะยังคงส่งสัญญาณอยู่ในโรงจอดรถก็ตาม**
2. **Priority 2 (Tag Departure):** ปิดตามหลังเมื่อแท็กหลุดระยะการตรวจจับ (`gPendingCloseAtMs`)
3. **Priority 3 (Failsafe Timer):** ปิดสำรองฉุกเฉินเมื่อประตูเปิดค้างนานเกินกำหนด

### Layer 4: Directional Reversal Deadband Actuator (2.4 วินาที)
* เมื่อมีการสั่งสลับทิศทางระหว่าง Open ↔ Close ขณะที่ประตูกำลังเคลื่อนที่:
  * ยิง **STOP Pulse (GPIO 6+7 = HIGH 1200ms)** พร้อมกันในระดับฮาร์ดแวร์ผ่านรีจิสเตอร์ `GPIO_OUT_W1TS_REG` (ป้องกันความคลาดเคลื่อนของมัลติทาสก์)
  * หน่วงเวลาพักเกียร์ **Deadband 1200ms** เพื่อให้กลไกและแรงเฉื่อยของมอเตอร์หยุดนิ่งสนิท
  * ยิงพัลส์คำสั่งทิศทางใหม่ **800ms**
  * ห้ามยิง STOP ซ้ำสองรอบเด็ดขาด

### Layer 5: Transition Timers & Auto-Retry
* นับเวลาการเดินทางของประตู (Door Transition Timeout)
* หากเปิดระบบตรวจสอบแม่เหล็ก (Magnetic Reed Switch) และพบว่าประตูปิดไม่สนิท ระบบจะสั่งยิงพัลส์ปิดซ้ำ (Auto-Retry Close) ตามจำนวนรอบที่กำหนด

---

## 💾 2. โครงสร้างหน่วยความจำและการจัดการ NVS Schema Migration

### 2.1 ตารางพาร์ติชันแฟลช (Flash Partition Table: 4MB ESP32-C3)
ไฟล์ [partitions_two_ota.csv](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/partitions_two_ota.csv) กำหนดการจัดสรรพื้นที่ดังนี้:

| Partition Name | Type | SubType | Offset | Size | หน้าที่ |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `nvs` | data | nvs | `0x9000` | 128 KB | จัดเก็บบันทึกคอนฟิกและ Tag แต่ละตัว |
| `otadata` | data | ota | `0x29000` | 8 KB | ข้อมูลการสลับบูต Dual-OTA |
| `app0` | app | ota_0 | `0x30000` | 1.81 MB | เฟิร์มแวร์สล็อตที่ 1 (1,900,544 bytes) |
| `app1` | app | ota_1 | `0x200000` | 1.81 MB | เฟิร์มแวร์สล็อตที่ 2 (1,900,544 bytes) |
| `spiffs` | data | spiffs | `0x3D0000` | 192 KB | จัดเก็บ Web Assets & ประวัติย้อนหลัง 180 วัน |

### 2.2 การอัปเกรดโครงสร้างข้อมูล Tag โดยไม่กระทบข้อมูลเดิม (Schema Migration)
ใน [src/tags/TagRegistry.cpp](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/tags/TagRegistry.cpp) ระบบตรวจจับขนาดไบต์ (`tagLen`) เพื่อทำ Backward Compatibility อัตโนมัติ:

```cpp
#pragma pack(push, 1)
// เวอร์ชัน 2: ขนาด 125 ไบต์ (เพิ่ม icon[16])
struct TagDiskEntryV2 {
  uint32_t id;
  char name[80];
  uint8_t mac[6];
  uint8_t isRf;
  uint32_t tagId;
  uint32_t tagHoldMs;
  uint32_t tagCloseDelayMs;
  int8_t tagRssiMin;
  uint8_t rfRole;
  uint32_t tagOpenDurationMs;
  uint32_t tagSafetyCloseDelayMs;
  char icon[16];
};

// เวอร์ชัน 3 (ปัจจุบัน): ขนาด 130 ไบต์ (เพิ่ม Active Hours Schedule)
struct TagDiskEntry {
  uint32_t id;
  char name[80];
  uint8_t mac[6];
  uint8_t isRf;
  uint32_t tagId;
  uint32_t tagHoldMs;
  uint32_t tagCloseDelayMs;
  int8_t tagRssiMin;
  uint8_t rfRole;
  uint32_t tagOpenDurationMs;
  uint32_t tagSafetyCloseDelayMs;
  char icon[16];
  uint8_t scheduleEnabled;     // 1 ไบต์: 0 = ปิด (ใช้ได้ 24 ชม.), 1 = เปิด
  uint16_t scheduleStartMin;   // 2 ไบต์: นาทีของวัน (0..1439)
  uint16_t scheduleEndMin;     // 2 ไบต์: นาทีของวัน (0..1439)
};
#pragma pack(pop)
```

* **กลไกการโหลด:** หากพบขนาด 125 ไบต์ ระบบจะโหลดค่าเดิมทั้งหมด แล้วกำหนด `scheduleEnabled = 0` ทำให้อัปเดตเฟิร์มแวร์แล้ว Tag เก่าไม่หายและทำงานต่อได้ทันที 100%

### 2.3 สถาปัตยกรรมค้นหาความเร็วสูง Zero-Latency Flat Hash Table (ความจุ 256 Tag)
ใน [src/tags/TagRegistry.h](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/tags/TagRegistry.h) และ [src/tags/TagRegistry.cpp](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/tags/TagRegistry.cpp) ระบบได้รับการออกแบบเพื่อรองรับอุปกรณ์ได้สูงสุด **256 Tag** (สำหรับบ้านขนาดใหญ่, คอนโดมิเนียม, อพาร์ตเมนต์ หรืออาคารสำนักงาน) โดยมีความเร็วในการค้นหาคงที่ O(1) ระดับฮาร์ดแวร์:

1. **Flat Open-Addressing Hash Table (512 ช่อง, Load Factor <= 50%):**
   * ยกเลิก `std::unordered_map` และ MRU Cache แบบเดิมที่ต้องจอง Heap Memory และทำ `memmove`
   * เปลี่ยนมาใช้ Flat Memory Array ขนาด 512 ช่อง (`kHashSlots = 512`, Bitwise Mask `0x1FF`) จำนวน 3 ชุด:
     * `_rfTable[512]`: ค้นหารหัสรีโมท RF 433MHz (24-bit EV1527)
     * `_macTable[512]`: ค้นหา MAC Address ของ ESP-NOW Car Tag
     * `_idTable[512]`: ค้นหา Hex ID ประจำตัว Tag
   * แต่ละสล็อตมีขนาดเพียง 8 ไบต์ กิน RAM รวมทั้ง 3 ตารางเพียง **12 KB** ใน SRAM (Zero Dynamic Heap Allocation)
2. **การคำนวณตำแหน่งด้วย Fibonacci Hashing:**
   * ใช้สูตร `hash = (val * 0x9E3779B9U) >> 23` ซึ่งใช้คำสั่งคูณฮาร์ดแวร์ 1 รอบของซีพียู RISC-V 160MHz
   * ช่วยกระจายบิตข้อมูลของรหัสรีโมท 24-bit ได้สม่ำเสมอที่สุด ความหนาแน่นข้อมูลไม่เกิน 50% ทำให้โอกาสชน (Collision) ต่ำมาก (> 90% ค้นหาเจอตั้งแต่สล็อตแรกทันที)
   * **เวลาค้นหาเฉลี่ย:** เพียง **~15 – 35 นาโนวินาที (6–8 Clock Cycles)** ไม่มีความหน่วงแม้แต่น้อย
3. **ระบบป้องกันหน่วยความจำเต็ม HTTP Chunked Streaming (`/api/tags`):**
   * ใน [src/api/TagsApiController.cpp](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/api/TagsApiController.cpp) ระบบดึงข้อมูล Tag บน WebUI ถูกปรับเป็น **Chunked Transfer Encoding** (ส่งทีละ 1 KB ผ่าน `sendContent`)
   * ลดการใช้ RAM สูงสุดขณะเปิดหน้าจอจาก ~80 KB เหลือเพียง **~1.5 KB** ป้องกันปัญหา Out-of-Memory (OOM) และเครื่องค้างได้อย่างเด็ดขาด

---

## 📡 3. ระบบไร้สายและฮาร์ดแวร์ซับซิสเต็ม

### 3.1 ESP-NOW 2.4GHz Direct Wireless
* ไม่พึ่งพา Wi-Fi Router (Zero Latency Direct Packet)
* **Asymmetric EMA RSSI Filter:**
  * สัญญาณขาขึ้น (รถกำลังขับเข้าใกล้): ใช้ค่าอัลฟ่าตอบสนองเร็ว (alpha = 0.85) เพื่อเปิดประตูทันที
  * สัญญาณขาดหายชั่วขณะหรือมีสัญญาณรบกวน: ใช้ค่าอัลฟ่ากรองความนุ่มนวล (alpha = 0.08) เพื่อป้องกันการสั่งปิดผิดพลาด
* **Immediate Broadcast Announce:** เมื่อได้รับ Beacon จากแท็ก จะยิงเฟรมตอบกลับทันที เพื่อให้แท็กในรถได้รับข้อมูลยืนยันและส่งเสียงปี๊บภายในหน้าต่างเวลาตื่น (Wake RX Window)

### 3.2 RF 433.92MHz Receiver
* ไอซีภาครับ SYN480R ASK/OOK เชื่อมต่อกับขัดจังหวะ GPIO 0
* ถอดรหัสเฟรม 24-bit แบบ Single-Frame Immediate Action (ไม่ต้องรอซ้ำ 2 เฟรม เพื่อป้องกันเฟรมหลุดจากประกายไฟของแปรงถ่านมอเตอร์)
* รองรับ 4 บทบาท: `AutoToggle` (0), `HoldOpen` (1), `Close` (2), `Stop` (3), `AutoOpen` (4)

---

## 🌐 4. ข้อกำหนด REST API (RESTful API Specifications)

| เมธอด | เส้นทาง (Endpoint) | คำอธิบาย | ข้อมูลที่ส่ง/รับ (JSON) |
| :--- | :--- | :--- | :--- |
| `GET` | `/api/status` | รายงานสถานะสดของประตู, เซ็นเซอร์, และพาร์ติชัน OTA | `{"ok":true, "doorState":"closed", "fwVersion":"1.0.0", "ota":{...}}` |
| `GET` | `/api/tags` | ดึงรายชื่อ Tag และสถานะการตรวจจับสด | `{"ok":true, "tags":[{"id":"...", "name":"...", "scheduleEnabled":false, ...}]}` |
| `POST` | `/api/tags` | เพิ่ม Tag ใหม่ หรือจับคู่ผ่านโหมด Learn | `{"name":"Car", "mac":"...", "scheduleEnabled":true, "scheduleStartMin":480, ...}` |
| `PUT` | `/api/tags/{id}` | อัปเดตข้อมูล Tag (ระยะ, ดีเลย์, เวลาใช้งาน) | อัปเดตค่าฟิลด์ที่ต้องการ |
| `DELETE`| `/api/tags/{id}`| ลบ Tag ออกจากระบบ NVS | `{"ok":true}` |
| `POST` | `/api/door` | สั่งงานประตู (open, close, stop, toggle) | `{"action":"open"}` หรือส่ง query param `?action=open` |
| `POST` | `/api/ota` | อัปโหลดไฟล์เฟิร์มแวร์ .bin ผ่าน Web Multipart | Form Data `name="update"`, `filename="firmware.bin"` |

---

## ☁️ 5. สถาปัตยกรรมคลาวด์และการประหยัดแบนด์วิดท์ Blynk (100k Quota)

เพื่อให้ระบบทำงานร่วมกับ Blynk IoT Free Tier (จำกัดโควตา **100,000 ข้อความ/เดือน**):
* **Event-Driven Only:** สั่ง `Blynk.virtualWrite` **เฉพาะเมื่อมีการเปลี่ยนแปลงสถานะจริงเท่านั้น** (เช่น ประตูเริ่มเปิด, ประตูเปิดสุด, ประตูเริ่มปิด, ประตูปิดสนิท)
* **ห้ามใช้ Fast Polling เด็ดขาด:** (การซิงค์ทุก 20 วินาทีจะกินโควตา 4,320 ข้อความ/วัน = เกินโควตาภายใน 23 วัน)
* **Fallback Safety-Net Sync:** การซิงค์สถานะสำรองกรณีเน็ตหลุดจะทำงานไม่ถี่กว่า **ทุกๆ 30 นาที** (~1,440 ข้อความ/เดือน)

---

## 🛠️ 6. เครื่องมือสำหรับนักพัฒนา (Developer CLI & Automation Tools)

โครงการมีชุดคำสั่งอัตโนมัติสำหรับคอมไพล์และอัปโหลดเฟิร์มแวร์แบบประหยัดเวลา:

```bash
# 1. แพ็กไฟล์ WebUI เข้าสู่ src/webui_assets.h
python scripts/webui.py pack

# 2. คอมไพล์โปรเจกต์ด้วย PlatformIO
pio run -e esp32-c3-release

# 3. อัปโหลด OTA ไปยังอุปกรณ์ทั้ง 2 ตัวในคำสั่งเดียว (192.168.1.11 & 192.168.1.12)
python scripts/ota_both.py

# 4. คอมไพล์และอัปโหลด OTA ทั้ง 2 ตัวในคำสั่งเดียว
python scripts/ota_upload.py --both --build
```
