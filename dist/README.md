# คู่มือไฟล์ Firmware (Clean Build)

ไฟล์ Firmware ทั้งหมดถูกคอมไพล์แบบ Clean Build (Release Mode) ปราศจาก Warning/Debug ขยะ และพร้อมใช้งานทันที:

---

## รายการไฟล์ในโฟลเดอร์ `dist/`

| ชื่อไฟล์ | ชิปเป้าหมาย | ขนาด | การใช้งาน |
| :--- | :--- | :--- | :--- |
| **`firmware_autodoor_ota.bin`** (หรือ `firmware.bin`) | ESP32-C3 | ~1.35 MB | 🌐 **Web UI OTA Firmware:** ใช้อัปโหลดผ่านหน้าเว็บตั้งค่าของ AutoDoor เมนู OTA Update |
| **`firmware_esp32_c3_release.bin`** | ESP32-C3 | ~1.35 MB | ⚡ **Flash ผ่านสาย USB:** สำหรับบอร์ด Receiver Hub |
| **`bootloader.bin`** | ESP32-C3 | ~19.5 KB | ⚙️ Bootloader สำหรับ Flash ครั้งแรกผ่านสาย (Offset: `0x0000`) |
| **`partitions.bin`** | ESP32-C3 | 3.0 KB | ⚙️ Partition Table 2x OTA (Offset: `0x8000`) |
| **`boot_app0.bin`** | ESP32-C3 | 8.0 KB | ⚙️ OTA Data Initializer (Offset: `0x29000`) |
| **`firmware_tag_esp8285.bin`** | ESP8266 / ESP8285 | ~279 KB | 🚗 **Tag Firmware (Release):** สำหรับแท็กติดรถยนต์ (ปิด Serial Debug เพื่อประหยัดพลังงาน) |
| **`firmware_tag_esp8285_debug.bin`** | ESP8266 / ESP8285 | ~280 KB | 🔍 **Tag Firmware (Debug):** สำหรับแท็กติดรถยนต์ พร้อม Serial Monitor 115200bps |

---

## เครื่องมือแฟลชแบบ 1-Click (ง่ายที่สุด)
ดูรายละเอียดและใช้งานได้ที่โฟลเดอร์ **`1-Click-Flash-ESP32C3/`**:
* ดับเบิลคลิก **`1-CLICK-FLASH.bat`** เพื่อสแกนพอร์ตและแฟลชทันที
* หรือดับเบิลคลิก **`1-CLICK-GUI.bat`** สำหรับหน้าต่างโปรแกรมแบบกราฟิก

---

## คำสั่ง Flash ผ่านสาย (esptool.py)

### 1. Flash ตัวแม่ Hub (ESP32-C3 4MB 2x OTA)
> **หมายเหตุ:** ตาม `partitions_two_ota.csv` NVS มีขนาด 128KB ทำให้ `otadata` อยู่ที่ `0x29000` และ `app0` เริ่มต้นที่ **`0x30000`** (ไม่ใช่ `0x10000`)
```bash
python -m esptool --chip esp32c3 --baud 460800 write_flash -z \
  0x0000 dist/bootloader.bin \
  0x8000 dist/partitions.bin \
  0x29000 dist/boot_app0.bin \
  0x30000 dist/firmware_esp32_c3_release.bin
```

### 2. Flash ตัวลูก Tag (ESP8266 / ESP8285)
```bash
esptool.py --chip esp8266 --baud 921600 write_flash -fm dout 0x00000 dist/firmware_tag_esp8285.bin
```

---

## การอัปเดตผ่าน Web OTA (ไม่ต้องต่อสาย)
1. เชื่อมต่อ Wi-Fi ของ AutoDoor หรือผ่าน IP ในวงบ้าน
2. เข้าหน้า WebUI $\rightarrow$ ไปที่แถบ **Firmware Update (OTA)**
3. เลือกไฟล์ `dist/firmware_autodoor_ota.bin` แล้วกด **Upload**
4. ระบบจะทำการเขียน Flash และรีสตาร์ทอัตโนมัติภายใน 10 วินาที
