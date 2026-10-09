# Drive-Through Tuning (ESP-NOW)

คู่มือจูนระบบประตูอัตโนมัติสำหรับรถวิ่งผ่าน — เปิดจากระยะไกล ปิดเร็วหลังรถออก

## พารามิเตอร์หลัก

| พารามิเตอร์ | ความหมาย | Drive-Through (default) | มาตรฐาน |
|-------------|----------|-------------------------|---------|
| `tagRssiMin` | RSSI ขั้นต่ำ (dBm) ถึงจะนับว่าเห็น tag | **-100** (ไกล) | -95 |
| `tagHoldMs` | ไม่มีสัญญาณครบกี่ ms ถือว่า tag หาย | **1000** | 2000 |
| `tagCloseDelayMs` | หลัง tag หายทั้งหมด รอกี่ ms ก่อนปิด | **0** | 3000 |
| `tagPassRssi` | RSSI ต่ำกว่านี้ = รถผ่านประตูแล้ว | **-85** | — |
| `tagPassCloseDelayMs` | หลังผ่านประตู รอกี่ ms แล้วปิด (แม้ tag ยังเห็น) | **500** | — |

**เวลาปิดโดยประมาณ** = `tagHoldMs` + `tagCloseDelayMs` หลัง RSSI ตกต่ำกว่า threshold

ระบบใช้ hysteresis 8 dBm: เมื่อ tag active แล้ว ต้องได้ RSSI ≥ `tagRssiMin - 8` จึงจะ refresh สถานะ

## Preset ใน Web UI

1. เชื่อม WiFi AP `AutoDoor-XXXX` (รหัสเริ่มต้น `12345678`)
2. ไป **อุปกรณ์** → เพิ่ม/แก้ Tag
3. กด **Drive-Through** หรือ **มาตรฐาน**
4. ดู **ปิดประมาณ X วิ หลังสัญญาณหาย** ใต้ช่องหน่วงปิด
5. บันทึก

Tag ที่ learn ใหม่จะได้ default Drive-Through อัตโนมัติ

## จูนด้วย Dashboard

หน้าแรกแสดงสถานะ RSSI ต่อ tag:

| สถานะ | ความหมาย |
|-------|----------|
| เข้าใกล้ | สัญญาณถึงเกณฑ์ แต่ยังไม่ active |
| อยู่ในระยะ | tag active — ประตูควรเปิด/ค้างเปิด |
| กำลังออก | RSSI ต่ำลง / กำลังหลุดจาก active |
| นอกระยะ | ไม่มีสัญญาณล่าสุด |

ขับรถทดสอบ ดู `lastRssi` เทียบ `tagRssiMin` บน dashboard

## ขั้นตอนทดสอบบนรถ

1. Flash receiver (ESP32-C3) + tag (ESP32-C6)
2. Learn tag ใหม่ หรือตั้ง preset Drive-Through ให้ tag เดิม
3. **ทดสอบเปิดไกล**: ขับเข้าจากไกล จดระยะที่ประตูเริ่มเปิด และ RSSI บน dashboard
4. **ทดสอบปิดเร็ว**: ขับผ่านไม่หยุด จับเวลาจาก "กำลังออก" → relay ปิด
5. ปรับทีละขั้น:
   - เปิดไม่ทัน → ลด `tagRssiMin` (เช่น -100 → -105)
   - ปิดช้า → ลด `tagHoldMs` หรือ `tagCloseDelayMs`
   - ปิดเร็วเกิน / กระตุก → เพิ่ม `tagHoldMs` หรือ `tagCloseDelayMs` เล็กน้อย
   - เปิดจากไกลเกิน (คนเดินผ่าน) → เพิ่ม `tagRssiMin` (เช่น -100 → -95)

## Serial log

เปิด Serial 115200 บน receiver:

```
[door] opening (tag active)
[door] schedule close in 0 ms
[door] closing (all tags gone)
```

## Troubleshooting

| อาการ | แนวทางแก้ |
|-------|-----------|
| ประตูไม่เปิดจากไกล | ลด `tagRssiMin` · ตรวจ tag C6 channel sync · ตรวจตำแหน่ง tag บนรถ |
| ประตูปิดช้าหลังวิ่งผ่าน | ลด `tagHoldMs` · ตั้ง `tagCloseDelayMs = 0` |
| ประตูปิดก่อนรถผ่าน | เพิ่ม `tagHoldMs` · ลด `tagRssiMin` ทำให้ active นานขึ้น |
| ประตูเปิด-ปิดกระตุก | เพิ่ม `tagHoldMs` เล็กน้อย · ปรับ `tagRssiMin` ให้สูงขึ้น |
| Tag ไม่เจอ | Safe mode ปิดอยู่? · channel ตรงกัน? · learn ใหม่ |

## ข้อจำกัด

- Door logic ใช้ active/inactive จากเวลา ไม่มี RSSI slope detection
- RSSI ESP-NOW มี jitter ± несколько dBm
- Tag ESP8266 (ch1, 500ms) ไม่เหมาะ drive-through — ใช้ ESP32-C6
