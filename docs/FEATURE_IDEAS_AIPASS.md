ใช้เวลาประมวลผล 39 วินาที
AutoDoor-RF2: แผนฟีเจอร์และแนวทางพัฒนาต่อ

ระบบตอนนี้ทำได้ครบกว่าประตูรีโมททั่วไปในตลาดแล้ว ผมเลยเน้นสามเรื่อง คือ (1) ช่องโหว่ที่ควรอุดก่อน (2) ฟีเจอร์ที่ทำให้ระบบดีกว่าคู่แข่งชัดเจน และ (3) โครงสร้างโค้ดที่รองรับการขยายระยะยาว

0. ข้อสังเกตสถาปัตยกรรมก่อนเพิ่มฟีเจอร์
0.1 ช่องว่างที่ควรอุดก่อน
ประเด็น	ความเสี่ยง	ทำไมสำคัญ
ระบบไม่รู้ตำแหน่งประตูจริง (ไม่มี limit/encoder feedback)	สูง	Auto-close, ตรวจ anomaly และ predictive maintenance ต้องรู้ว่าประตู "ปิดสนิทจริง" ไม่ใช่แค่ "สั่งปิดไปแล้ว"
ESP-NOW tag อาจถูก Replay/Relay Attack	สูง	ถ้าบีคอนเป็น MAC/payload คงที่ จะถูกดักแล้วส่งซ้ำได้ หรือถูก relay ขยายระยะได้เหมือนการขโมยรถ Keyless
รีโมท 433MHz แบบ Fixed code (EV1527/PT2262)	สูง	ใช้เครื่องคัดลอกราคาไม่กี่ร้อยบาทก็ clone ได้
ตัว Photocell ไม่ได้ทำ Self-test	กลาง–สูง	ถ้าตัวรับเสียแบบค้างค่า "ไม่ถูกบัง" ระบบจะเข้าใจว่าปลอดภัยตลอด
ESP32 ไม่ใช่อุปกรณ์ Safety-rated	ข้อเท็จจริง	ระบบกันหนีบหลักควรอยู่ที่บอร์ดมอเตอร์เดิม (force limit/photocell input) ส่วน ESP32 เป็นชั้นเสริม

จากที่สั่งงานแบบ pulse 800ms ผมเดาว่ารีเลย์ไปจำลองการกดปุ่มบนบอร์ดมอเตอร์เดิม ถ้าใช่ การวัดกระแสแบบไม่ต้องตัดสาย (CT clamp) ที่สายไฟเข้ามอเตอร์จะคุ้มที่สุด เพราะได้ทั้งตำแหน่ง สุขภาพมอเตอร์ และการตรวจสิ่งกีดขวางในครั้งเดียว

0.2 แนะนำ Refactor: Command Arbiter + Event Bus

ก่อนเพิ่มฟีเจอร์เยอะๆ ควรรวมคำสั่งทุกแหล่งให้ผ่านจุดเดียว และให้ทุกคำสั่งมี Source + Auth context แนบไปด้วย:

0. ข้อสังเกตสถาปัตยกรรมก่อนเพิ่มฟีเจอร์

Code · CPP

Open

ข้อดีของโครงนี้:

Audit log: รู้ได้ทุกครั้งว่า "ใคร สั่งอะไร ผ่านช่องทางไหน ถูกอนุญาตหรือปฏิเสธเพราะอะไร"
ฟีเจอร์ใหม่ส่วนใหญ่เขียนเป็น Policy ได้เลย: Vacation lock, Guest window, Night mode ไม่ต้องไปแก้ state machine หลัก
State machine ควรเป็น HSM: CLOSED → OPENING → OPEN → CLOSING → STOPPED_MID → REVERSING_DEADBAND → FAULT แยก FreeRTOS task เป็น safety_task (prio สูงสุด, 10ms tick), motor_task, radio_task, net_task สื่อสารผ่าน queue
1. Advanced Safety & Security
Low-hanging fruit

1.1 Photocell Self-Test (Monitored Safety Beam)

จ่ายไฟให้ตัวส่ง (TX) ผ่าน MOSFET ที่ GPIO คุม
ก่อนสั่งปิดทุกครั้ง: ปิด TX ไป 50ms แล้วต้องเห็นตัวรับรายงาน "ถูกบัง" จากนั้นเปิด TX แล้วต้องกลับเป็น "ปกติ" ภายใน 100ms
ถ้าเทสไม่ผ่าน ให้เข้าโหมด FAULT_BEAM ห้าม auto-close ยอมให้ปิดเฉพาะแบบ hold-to-run จาก WebUI แล้วแจ้งเตือน
หลักการเดียวกับ "monitored entrapment protection" ในมาตรฐาน UL 325 / EN 12453

1.2 ตรวจคุณภาพลำแสง (Beam Health Score)

นับ "glitch" ซึ่งคือการถูกตัดสั้นกว่า 30ms ต่อชั่วโมง ถ้าสูงขึ้นแปลว่าเลนส์สกปรก แนวเยื้อง หรือแดดส่อง (ในไทยเจอบ่อยช่วงเช้า/เย็นถ้าติดหันทิศตะวันออก/ตะวันตก)
เก็บ histogram ตามชั่วโมงของวัน จะได้เห็นแพทเทิร์นแดดชัดเจน

1.3 แจ้งเตือนการเปิดที่ไม่ได้รับอนุญาต (Unauthorized Movement)

ถ้าเซนเซอร์ตำแหน่ง (หัวข้อ 4) บอกว่าประตูขยับ แต่ไม่มี GateCommand ใดๆ ใน 3 วินาทีก่อนหน้า ให้ส่ง Critical Alert เพราะอาจเป็นการงัด, มีคนใช้รีโมทเดิมของบอร์ดมอเตอร์ หรือมีคนปลดคลัตช์

1.4 Door-Left-Open Escalation

เปิดค้างเกิน N นาที ส่งแจ้งเตือนระดับ 1 ถ้าเป็นกลางคืน (22:00–05:00) ส่งระดับ 2 พร้อมปุ่ม "ปิดเลย" ใน notification
หยุดแจ้งอัตโนมัติถ้าอยู่ใน Hold Open ที่ผู้ใช้ตั้งใจเปิดไว้

1.5 Rate Limiting และ Lockout

คำสั่งเกิน 6 ครั้ง/นาทีจากแหล่งเดียว จะ lock แหล่งนั้น 5 นาที (กันเด็กเล่นรีโมท/กันสคริปต์ API ผิดพลาด)
WebUI login ผิด 5 ครั้ง ใช้ exponential backoff

1.6 Enclosure Tamper Switch

ติด micro-switch ที่ฝากล่อง และใช้ input เดียวกันตรวจว่าไฟ 12V/24V ถูกตัดหรือไม่ (ผ่าน voltage divider เข้า ADC)
Mid-term

1.7 ESP-NOW Secure Challenge–Response (สำคัญที่สุดในหมวดนี้)

เปิด ESP-NOW encryption (PMK/LMK, CCMP) และทำ application-layer auth ซ้อนอีกชั้น:

Tag  → Gate : HELLO   {tagId, counter, RSSI_tx}
Gate → Tag  : CHAL    {nonce(16B), gateTs}
Tag  → Gate : RESP    {tagId, counter+1, HMAC-SHA256(K_tag, nonce||counter||tagId)[:16]}
Gate        : verify HMAC + counter > lastCounter + check RTT
K_tag ต่างกันทุกแท็ก สร้างตอน pairing แล้วเก็บใน NVS encrypted partition (เปิด Flash Encryption + NVS Encryption บน C3)
Monotonic counter ใช้กัน replay ฝั่งแท็กเก็บใน RTC memory และ flush ลง NVS ทุก 64 ครั้ง ตอนบูตให้กระโดดไปข้างหน้า 64 ครั้งเพื่อไม่ให้ตัวเลขย้อนกลับ
RTT Gate (กัน Relay แบบหยาบ): วัดเวลาตั้งแต่ส่ง CHAL จนได้ RESP ปกติ ESP-NOW ไปกลับจะอยู่ราว 1–3ms ส่วน relay ที่ผ่านอุปกรณ์กลางจะเพิ่ม latency เข้าไป ให้ calibrate baseline ของแต่ละแท็กแล้วตั้ง threshold เช่น baseline + 3σ วิธีนี้ไม่ใช่ distance bounding แท้ แต่ช่วยยกระดับความยากของการโจมตีได้มาก
Pairing: กดปุ่มที่กล่องควบคุม 3 วินาทีเพื่อเข้า pairing window 60 วินาที แท็กต้องอยู่ใกล้ (RSSI > −45dBm) จึงแลกคีย์ด้วย ECDH (X25519 ผ่าน mbedTLS)

1.8 อัปเกรดความปลอดภัยของ 433MHz

ทางเลือก A: เปลี่ยนเป็นรีโมท Rolling code (KeeLoq HCS301) แล้ว decode บน ESP32 (ต้องมี manufacturer key ของรีโมทนั้น)
ทางเลือก B (ถ้ายังใช้ fixed code): ใช้ Policy ลดความเสี่ยง เช่น กลางคืนรีโมท 433 ทำได้แค่ "ปิด" หรือต้องมีแท็ก/มือถืออยู่ในระยะด้วยถึงจะเปิดได้ (2-factor แบบนุ่มนวล)
Jamming Detection: SYN480R ไม่มีขา RSSI แต่ตรวจทางอ้อมได้จากความหนาแน่นของ edge บนขา DATA ถ้า noise pulse เกิน baseline มากๆ ต่อเนื่องเกิน 10 วินาที ให้แจ้ง "RF Jamming suspected" (โจรใช้ jammer กันเจ้าของล็อกรถ/ประตู)

1.9 Anti-Tailgating ด้วย Dual Beam หรือ mmWave

วิธี	หลักการ	ต้นทุน
Dual Photocell (ห่างกัน 40–60 ซม.)	ลำดับการตัด A→B หรือ B→A บอกทิศทาง ระยะเวลาการตัดบอกความยาว/ความเร็ว แยกรถยนต์ คน มอเตอร์ไซค์ได้	ต่ำ
mmWave Radar (HLK-LD2410 / LD2450)	LD2450 track ได้หลายเป้าหมายพร้อมพิกัด X/Y นับวัตถุที่ผ่านได้	ต่ำ–กลาง
กล้อง AI (ดูหัวข้อ 2)	นับรถ/คน และอ่านป้ายทะเบียน	กลาง

Logic ของ Anti-Tailgating:

authorizedEntries = จำนวนแท็กที่ auth สำเร็จในรอบเปิดนี้
crossings         = จำนวน object ที่ผ่าน (direction = IN)
if crossings > authorizedEntries:
    alert("Tailgating: มี {n} วัตถุผ่านเข้า แต่อนุญาตแค่ {m}")
    optionally: ลด auto-close delay ทันที / snapshot กล้อง
Advanced

1.10 UWB Ranging (DW3000 / DWM3000)

ESP32-C3 + DW3000 ทั้งที่แท็กและกล่อง ได้ระยะแม่นระดับ ±10 ซม. ด้วย Two-Way Ranging ซึ่งกัน relay attack ได้จริงทางฟิสิกส์ (แบบเดียวกับ digital car key ของ BMW/Apple)
ได้ "geofence ระดับเซนติเมตร" เช่น เปิดเมื่อรถอยู่ห่าง 8 ม. และกำลังเข้าใกล้ด้วยความเร็วมากกว่า 0.5 ม./วินาที

1.11 Signed Audit Log (Hash Chain)

แต่ละ log entry เก็บ hash = SHA256(prevHash || entry) ลง LittleFS แบบ ring buffer ทำให้แก้ย้อนหลังแล้วรู้ทันที เหมาะใช้เป็นหลักฐานกรณีมีเหตุ
2. UX & Smart Home Integration
Low-hanging fruit

2.1 Home Assistant: เพิ่ม Entities

sensor.gate_last_actor (ใครสั่งล่าสุด), sensor.gate_cycle_count, sensor.gate_open_duration, binary_sensor.gate_fault, sensor.tag_<name>_battery, sensor.tag_<name>_rssi, device_tracker.tag_<name> (home/away จากแท็ก)
select.gate_mode: Normal / Vacation / Party (เปิดค้าง) / Delivery / Lockdown
number.auto_close_delay ปรับได้จาก HA ตรงๆ
event entity สำหรับ "car_arrived", "car_departed", "tailgating" เพื่อให้ใช้เป็น automation trigger ได้ง่าย

2.2 CarPlay ผ่าน HomeKit Bridge (ไม่ต้องเขียน app เอง)

HA → HomeKit Bridge → expose cover เป็น Garage Door Opener accessory
อุปกรณ์ HomeKit ประเภทประตูโรงรถจะขึ้นใน CarPlay อัตโนมัติเมื่อรถเข้าใกล้บ้าน (ฟีเจอร์ของ iOS Home app) ได้ปุ่มบนจอรถฟรี
เพิ่ม Siri Shortcuts "เปิดประตูบ้าน" ได้เลย

2.3 Android Auto

ทางหลัก: HA Companion App รองรับ Android Auto มีหน้า entity/ปุ่มให้กดได้ (ฟีเจอร์ของแอปเอง)
ทางเสริม: Google Assistant ผ่าน Nabu Casa/Google Home สั่งด้วยเสียง โดยตั้ง PIN ป้องกันการสั่ง "เปิด" ด้วยเสียงไว้

2.4 แจ้งเตือนผ่าน LINE / Telegram

หมายเหตุ: LINE Notify ปิดให้บริการแล้ว (มี.ค. 2025) ต้องใช้ LINE Messaging API แทน
Telegram Bot ทำง่ายกว่า มี inline keyboard [เปิด] [ปิด] [หยุด] และส่งภาพ snapshot ได้

2.5 Guest / Delivery Access Link

สร้างลิงก์ใช้ครั้งเดียว: https://gate.local/g/<token> โดย token = base64(guestId|expiry|maxUses) + HMAC
ตั้งได้ว่าใช้ได้ช่วงเวลาไหน (เช่น ช่างแอร์ 13:00–15:00) ใช้ได้กี่ครั้ง และโหมด "เปิดแค่ 1 เมตรสำหรับคน" (partial open คุมด้วยเวลา/encoder)
แสดงเป็น QR code ใน WebUI เพื่อส่งให้แขกได้ทันที
Mid-term

2.6 Smart Geofence (Fusion หลายแหล่ง)

ใช้ geofence มือถืออย่างเดียวมักเปิดเร็วไปหรือช้าไป ควรรวมหลายแหล่ง:

score = w1·phoneZone(HA) + w2·phoneSpeedTrend + w3·tagRSSI_slope + w4·tagAuth
- phone เข้า zone 500 ม. → "pre-arm": ปลุกแท็กให้ถี่ขึ้น, เตรียม WiFi
- tag RSSI ขึ้นต่อเนื่อง (slope > 0) + auth OK → เปิด
- phone อยู่ที่บ้านแต่ tag ไม่มี → ไม่เปิด (อาจเดินมา ไม่ได้ขับ)

2.7 ทำงานร่วมกับกล้อง AI (Frigate + LPR)

Frigate (NVR บน HA ที่ใช้ Coral/OpenVINO) ตรวจ car/person แล้วส่ง MQTT event ให้ ESP32 subscribe
License Plate Recognition (Frigate 0.14+ มี LPR ในตัว หรือใช้ CodeProject.AI / Plate Recognizer) ใช้เป็น factor เสริม เช่น แท็ก OK + ป้ายทะเบียนตรง จึงเปิดในโหมดกลางคืน
ทุก event สำคัญ (เปิด, tailgating, unauthorized) ให้ trigger snapshot/clip แนบไปกับ notification
ฟีเจอร์ "ใครมาส่งของ": เห็นคนที่ประตู → ส่งภาพพร้อมปุ่ม "เปิด 1 เมตร"

2.8 Context-aware Automations (HA Blueprints ที่ทำแจกผู้ใช้ได้)

รถเข้าบ้านหลังมืด → เปิดไฟหน้าบ้าน/ไฟโรงรถ 3 นาที
รถออกจากบ้าน + ไม่มีใครอยู่บ้าน → เปิด alarm mode, ปิดแอร์
ฝนตก (weather/rain sensor) → ลด auto-close delay
ทุกคนออกจากบ้านแล้วประตูยังเปิด → แจ้งเตือนพร้อมปุ่มปิด
Advanced

2.9 Matter Support

ESP32-C3 รองรับ Matter over Wi-Fi ด้วย esp-matter SDK ตัวมาตรฐาน Matter รุ่นใหม่ๆ เริ่มมี device type กลุ่ม Closures แล้ว แต่ระบบนิเวศยังไม่นิ่ง ระหว่างนี้ใช้ Window Covering เป็นทางผ่านไปก่อนได้ ควรตรวจสถานะ spec และ SDK ล่าสุดก่อนลงมือ
ข้อดีคือได้ Apple Home, Google Home, Alexa พร้อมกันโดยไม่ต้องพึ่ง HA ข้อควรระวังคือ flash/RAM ของ C3 จะตึงมาก (อาจต้องตัด WebUI บางส่วนหรือใช้ flash 8MB)

2.10 Phone-as-Key ผ่าน BLE

ใช้ BLE 5 ของ C3 ทำ GATT service แบบ challenge-response เหมือน ESP-NOW tag
ฝั่ง iOS ทำงาน background ได้จำกัด ทางที่ใช้ได้จริงคือ Shortcut/App Clip หรือ NFC tag ที่ประตู (แตะแล้วเปิด) ส่วน Android ทำ foreground service ได้ยืดหยุ่นกว่า
3. ESP-NOW Car Tag v2
3.1 ข้อจำกัดในรถเมืองไทย (ต้องคิดก่อนเลือกฮาร์ดแวร์)
อุณหภูมิในรถจอดตากแดดขึ้นได้ 70–80°C ซึ่งเกินพิกัดชาร์จ/เก็บของ Li-ion/LiPo ทั่วไปและเสี่ยงบวม ทางเลือกที่แนะนำ:
แหล่งพลังงาน	ข้อดี	ข้อเสีย
ไฟรถ (USB/12V ACC) + Supercap	ไม่มีแบตเสื่อม, ทนร้อน	ทำงานเฉพาะตอนติดเครื่อง (ซึ่งก็พอสำหรับ use case นี้)
LiFePO4 (LFP) 18650/14500	ทนร้อนและปลอดภัยกว่ามาก	แรงดัน 3.2V ต้องใช้ LDO dropout ต่ำ
CR123A / Li-SOCl₂ (ER14505)	อายุยาว, ทน 85°C	ชาร์จไม่ได้, Li-SOCl₂ จ่ายกระแสพีคได้ต่ำ (ต้องมี cap ช่วยตอน TX)
3.2 Power Architecture ระดับไมโครแอมป์
สถานะ	กระแสโดยประมาณ	เงื่อนไข
Deep sleep + IMU wake-on-motion	~5 µA (C3) + ~1–3 µA (IMU)	รถจอดนิ่ง
Motion detected, ห่างบ้าน	wake ทุก 5–10 วินาที ยิง HELLO ~3ms	รถวิ่ง
Approach mode (เจอ gate)	beacon 200–500ms + challenge-response	อยู่ในระยะ
Parked-at-home	กลับไป deep sleep หลัง 60 วินาทีนิ่ง	Baseline suppression ทำงานฝั่งแท็กได้เลย

หลักออกแบบ:

Accelerometer: ใช้ LIS2DW12 หรือ LIS3DH ในโหมด wake-up interrupt ต่อ INT เข้า GPIO0–5 (บน C3 มีแค่กลุ่มนี้ที่ปลุกจาก deep sleep ได้)
LDO: ใช้ quiescent ต่ำ เช่น ME6211, TPS7A02, HT7333 ห้ามใช้ AMS1117 (Iq ~5mA ทำให้แบตหมดในไม่กี่วัน)
Battery divider: ต่อผ่าน P-MOSFET ที่ GPIO คุม เปิดเฉพาะตอนวัด ไม่อย่างนั้น divider จะกินกระแสตลอด
Buzzer: ขับผ่าน transistor และใช้แบบ passive + LEDC จะเล่นได้หลายโทน (เปิด = เสียงขึ้น, ปิด = เสียงลง, error = 3 สั้น)
ESP-NOW channel caching: ESP-NOW ต้องใช้ channel เดียวกับ Wi-Fi AP ที่กล่องเกาะอยู่ ให้เก็บ channel ล่าสุดใน RTC memory ถ้า 3 รอบไม่มีตอบให้ scan 1→13 (ลด on-air time ได้มาก) ฝั่งกล่องควรกำหนด channel ของ AP ให้คงที่

3. ESP-NOW Car Tag v2

Code · CPP

Open
3.3 ฟีเจอร์ใหม่ของแท็ก
Battery Telemetry: ส่ง Vbat, อุณหภูมิ (sensor ในชิป C3), จำนวน wake, firmware version ไปใน payload ฝั่งกล่องคำนวณ "คาดว่าแบตหมดในอีก X วัน" จาก slope ของ Vbat แล้วแจ้งเตือนล่วงหน้า 14 วัน
Approach Trend Detection: ใช้ RSSI จากหลายแพ็กเก็ตมาทำ EMA/Kalman แล้วดู slope ประกอบ ถ้าเข้าใกล้ (slope > 0) จึงเปิด ถ้าแค่ผ่านหน้าบ้านในระยะเท่ากันแต่ slope ≈ 0 แล้วลดลง จะไม่เปิด ช่วยลด false open ได้มาก
ปุ่มบนแท็ก (Panic/Manual): กดสั้นสั่ง Toggle (ผ่าน secure channel) กดค้าง 3 วินาทีเป็น "Lockdown" ปิดและล็อกทุก auto-open
Departure Confirmation: เมื่อรถออกจากบ้านและประตูปิดสนิท (ยืนยันด้วย limit switch) ให้ buzzer ดัง "ปิดสนิทแล้ว" ถ้าปิดไม่สนิทให้ดังเตือนต่างโทน คนขับรู้ทันทีโดยไม่ต้องมองกระจก
Tag OTA ผ่าน ESP-NOW: กล่องเป็นตัวกลางส่ง firmware เป็น chunk (ใช้ ESP-NOW 250 bytes/packet พร้อม CRC และ resume) อัปเดตแท็กได้โดยไม่ต้องถอดจากรถ
Tag Tamper/Removal: IMU ตรวจการถูกถอด/เขย่าผิดปกติ ประกอบกับ light sensor (ถ้ามี) แล้วแจ้ง
Multi-Gate Roaming: แท็กเดียวใช้ได้หลาย site (บ้าน, คอนโด, ออฟฟิศ) แยก key ต่อ gate
4. Diagnostics & Predictive Maintenance
4.1 Sensor Add-ons ที่จำเป็น (Low-hanging, ฮาร์ดแวร์ราคาถูก)
Sensor	ติดตั้ง	ได้อะไร
Reed switch / Limit switch ×2 (ปิดสุด/เปิดสุด)	แม่เหล็กที่บานประตู	ตำแหน่งจริง, travel time แม่นยำ
CT Clamp SCT-013 (มอเตอร์ AC) หรือ INA226 / ACS712 (DC 24V)	สายไฟมอเตอร์	Current signature, ตรวจสิ่งกีดขวาง, ตรวจรีเลย์ติด
DS18B20 / SHT40	ในกล่อง/ที่มอเตอร์	ชดเชยผลของอุณหภูมิต่อความหนืดจาระบี, เตือนกล่องร้อน
Voltage sense	ไฟเลี้ยง	ตรวจไฟดับ/ไฟตก/แบตสำรองมอเตอร์

ข้อควรระวังเรื่อง CT กับ ADC ของ C3: ADC ของ C3 มี non-linearity พอสมควร และวัด AC ต้อง sample ≥ 2kHz แล้วคำนวณ RMS ถ้าอยากได้ข้อมูลสะอาด ใช้ ADC ภายนอกอย่าง ADS1115 (I²C) หรือ IC วัดพลังงานอย่าง HLW8032/BL0942 จะได้ทั้ง V, I, P และ power factor

4.2 Cycle Counter แบบไม่ทำ Flash สึก
เก็บ counter ใน RTC RAM แล้ว flush ลง NVS ทุก 10 รอบ หรือทันทีเมื่อตรวจพบไฟตก (brownout ISR / voltage sense) NVS ของ ESP-IDF มี wear leveling อยู่แล้ว
นับแยกได้หลายแบบ: total cycles, cycles ตามแหล่งคำสั่ง, จำนวนครั้งกลับทิศ (reversal ทำเกียร์สึกมากกว่า), relay actuations แยกช่อง (รีเลย์ทั่วไปมีอายุทางไฟฟ้าราว 100k ครั้ง แล้วแต่ load ให้ดูใน datasheet ของรุ่นที่ใช้)
4.3 Travel-Time และ Current Signature Analytics

Feature ที่ดึงได้ต่อ 1 รอบ:

4. Diagnostics & Predictive Maintenance

Code · CPP

Open

Detection Engine (เบาพอรันบน C3):

Baseline: ใช้ EWMA แยกตามทิศ (เปิด/ปิด) และ bucket อุณหภูมิ (<25, 25–35, >35°C)
Drift ระยะยาว: ใช้ CUSUM บน travelMs และ steadyRms ซึ่งไวต่อการเปลี่ยนช้าๆ อย่างจาระบีแห้งหรือรางเริ่มคด
Spike ระยะสั้น: ถ้า |x − μ| > 3σ ให้แจ้งเหตุผิดปกติทันที (เช่น มีหินติดราง)
Position-indexed current map: แบ่งระยะทางเป็น 20 ช่วงตามเวลาที่ normalize แล้ว เก็บกระแสเฉลี่ยของแต่ละช่วง จะชี้ได้ว่า "ฝืดที่ช่วง 60–70% ของระยะทาง" ช่างตรงไปตรวจจุดนั้นได้เลย
cpp
// CUSUM แบบเบา
float sPos = 0; const float k = 0.5f*sigma, h = 5.0f*sigma;
void cusumUpdate(float x) {
  sPos = fmaxf(0, sPos + (x - mu) - k);
  if (sPos > h) raiseMaintenance("Travel time drifting up — check rail/lubrication");
}

สิ่งที่ตรวจได้จากกระแส:

อาการ	Signature
รางฝืด / จาระบีแห้ง	steadyRms สูงขึ้นช้าๆ, travelMs ยาวขึ้น
เกียร์สึก / ฟันเฟืองหลวม	currentStdDev สูง มี ripple เป็นจังหวะ
คาปาซิเตอร์สตาร์ทเสื่อม (มอเตอร์ AC)	inrushPeak เปลี่ยน, ใช้เวลาเร่งนานขึ้น
สิ่งกีดขวาง	กระแสพุ่งเกิน threshold ของ "ช่วงตำแหน่งนั้น" ให้ STOP แล้ว reverse (เป็นชั้นเสริมจาก force limit ของบอร์ดเดิม)
รีเลย์ติด (contact welding)	มีกระแสไหลทั้งที่ไม่ได้สั่ง ให้เข้า FAULT ทันที และตัดไฟหลักผ่าน contactor ตัวที่สอง (ถ้ามี)
4.4 Service Reminder Engine
เงื่อนไขแบบ OR: cycles ≥ 5,000 หรือ เวลาผ่าน 6 เดือน หรือ CUSUM trigger
แสดงใน WebUI/HA เป็น "Health Score 0–100" พร้อมเหตุผล เช่น "travel time ขาปิดช้าลง 12% ใน 30 วัน"
เก็บ Maintenance Log: กดปุ่ม "Service Done" แล้ว reset baseline (ระบบเรียนรู้ใหม่ 20 รอบ)
4.5 System Health ของตัวบอร์ดเอง
Reset reason (esp_reset_reason()), จำนวน brownout, crash count
Core dump ลง flash partition แล้วดาวน์โหลดผ่าน WebUI ได้
Heap free/min-ever, stack high-water mark ของแต่ละ task
Wi-Fi RSSI, reconnect count, MQTT/Blynk latency
Ring log ลง LittleFS (เช่น 256KB หมุนวน) และ export เป็น CSV จาก WebUI ได้
5. ไอเดียเชิงนวัตกรรม
Auto-Learn Calibration: กด "Learn" แล้วระบบสั่งเปิด–ปิดเองเต็มรอบ 3 ครั้ง เพื่อวัด travel time, current profile และตั้ง threshold อัตโนมัติ ผู้ใช้ไม่ต้องตั้งค่าเวลาเอง
Digital Twin ใน WebUI: แอนิเมชันประตูขยับตาม % ตำแหน่งจริงแบบ realtime ผ่าน WebSocket แสดงลำแสง (แดง/เขียว), แท็กในระยะพร้อมวงแหวน RSSI และกราฟกระแสสดระหว่างวิ่ง
Partial Open / Pedestrian Mode: เปิด 1 ม.ให้คนเดินผ่าน (ใช้เวลาจาก profile หรือ encoder) เหมาะกับรับของ/ทิ้งขยะ ไม่ต้องเปิดทั้งบาน
Flood / Storm Mode (บริบทไทย): รับข้อมูลระดับน้ำ (float switch) หรือพายุจาก HA เพื่อล็อก auto-open, แจ้งเตือน, และสั่งเปิดค้างได้ถ้าต้องย้ายรถขึ้นที่สูง
Power Outage Resilience: วัดแบตสำรองของมอเตอร์, เข้าโหมด "low-power" ลด Wi-Fi TX และปิด Blynk แจ้งเตือนว่า "เหลือประมาณ N รอบ" ถ้าไฟมาจาก UPS
Vacation / Presence Simulation: ปิด auto-open ทั้งหมดยกเว้นแท็กที่ whitelist ใครสั่งเปิดก็แจ้งทันทีพร้อมภาพจากกล้อง
Smart Auto-Close แบบเรียนรู้: เรียนรู้ว่าแต่ละคันใช้เวลาผ่านประตูเท่าไหร่ (จาก dual beam) แล้วปรับ delay อัตโนมัติ เช่น คันที่ขับช้าหรือถอยเข้าได้ delay ยาวกว่า
Multi-Controller Mesh: หลายกล่อง (ประตูรั้ว + ประตูโรงรถ) คุยกันผ่าน ESP-NOW ทำ sequence ได้ เช่น "รั้วเปิด → รถผ่าน → โรงรถเปิด → รั้วปิด" ใช้แท็กเดียวสั่งทั้งระบบ
Voice Feedback ที่กล่อง: โมดูลเสียงเล็กๆ (DFPlayer Mini/I²S amp) พูด "ประตูกำลังปิด กรุณาระวัง" ก่อนปิดอัตโนมัติ 3 วินาที คู่กับไฟกะพริบเตือน (warning beacon)
Open API + SDK: REST API แบบมี token และ scope (เช่น read, control, admin) เปิดให้ผู้ใช้ต่อกับระบบอื่นได้ เพิ่มมูลค่าถ้าจะทำเป็นสินค้า
Fleet / Multi-Site Dashboard: ถ้าจะขายจริง ทำ cloud backend (MQTT broker + TimescaleDB + Grafana) ให้ช่างติดตั้งดูสุขภาพประตูลูกค้าหลายบ้าน แล้วส่งช่างเข้าเซอร์วิสล่วงหน้า (เป็นโมเดลรายได้ต่อเนื่องได้)
6. ตารางจัดลำดับความคุ้มค่า
#	ฟีเจอร์	คุณค่า	ความยาก	HW เพิ่ม	ระยะ
1	Command Arbiter + Audit Log	สูงมาก	ต่ำ–กลาง	–	Low-hanging
2	Reed/Limit switch (ตำแหน่งจริง)	สูงมาก	ต่ำ	~50 บาท	Low-hanging
3	Photocell Self-Test	สูงมาก	ต่ำ	MOSFET	Low-hanging
4	Unauthorized movement alert	สูง	ต่ำ	(ต้องมี #2)	Low-hanging
5	Door-left-open escalation	สูง	ต่ำ	–	Low-hanging
6	Cycle counter + relay life	กลาง	ต่ำ	–	Low-hanging
7	HomeKit Bridge → CarPlay	สูง	ต่ำ (config HA)	–	Low-hanging
8	Telegram/LINE Messaging API	กลาง	ต่ำ	–	Low-hanging
9	Rate limit / Lockout / Tamper	กลาง	ต่ำ	micro-switch	Low-hanging
10	ESP-NOW Challenge-Response + Encryption	สูงมาก	กลาง	–	Mid-term
11	Tag v2: IMU wake + µA power + battery telemetry	สูง	กลาง	IMU, LDO	Mid-term
12	RSSI trend (approach detection)	สูง	กลาง	–	Mid-term
13	Current sensing + travel analytics + CUSUM	สูงมาก	กลาง	CT/INA226	Mid-term
14	Dual beam / mmWave anti-tailgating	สูง	กลาง	photocell/LD2450	Mid-term
15	Guest/Delivery link + QR	สูง	กลาง	–	Mid-term
16	Frigate + LPR integration	สูง	กลาง	กล้อง + server	Mid-term
17	433 jamming detection / rolling code	กลาง	กลาง	(รีโมทใหม่)	Mid-term
18	Auto-Learn + Digital Twin WebUI	กลาง–สูง	กลาง	–	Mid-term
19	Position-indexed current map	สูง	สูง	(ต้องมี #13)	Advanced
20	Tag OTA ผ่าน ESP-NOW	กลาง	สูง	–	Advanced
21	UWB ranging (anti-relay แท้)	สูง	สูง	DW3000 ×2	Advanced
22	Matter native	กลาง–สูง	สูง	(flash 8MB)	Advanced
23	Multi-controller mesh	กลาง	สูง	–	Advanced
24	Fleet cloud dashboard	สูง (เชิงธุรกิจ)	สูง	server	Advanced
7. Roadmap แนะนำ

Phase 1 (2–4 สัปดาห์): Foundation & Safety

Refactor ให้มี Command Arbiter, Policy chain, Audit log
ติด limit switch, ทำ Photocell self-test, Unauthorized/Left-open alerts
เพิ่ม HA entities, HomeKit/CarPlay, Telegram

Phase 2 (1–3 เดือน): Security & Intelligence

ESP-NOW secure protocol v2 (เปลี่ยนเป็น versioned packet header และรองรับแท็กเก่าช่วง migration)
Tag v2 hardware (IMU + low-Iq LDO + LiFePO4/ไฟรถ)
Current sensing + predictive maintenance engine
Dual beam anti-tailgating, Guest links

Phase 3 (3–6 เดือน+): Differentiation

UWB, Matter, mesh multi-gate, fleet dashboard, position-indexed diagnostics
8. ข้อควรระวังเชิงวิศวกรรม
Hardware interlock ของรีเลย์: ควรเดินสาย OPEN/CLOSE แบบ changeover interlock ให้ทางกายภาพไม่มีทางจ่ายไฟพร้อมกันได้ แม้ firmware จะ bug หรือ GPIO glitch ตอนบูต (ESP32-C3 มี strapping pins และบาง GPIO มีสถานะ glitch ตอน reset ควรหลีกเลี่ยงขาเหล่านั้นสำหรับรีเลย์)
Boot-safe state: ตอนบูต/OTA rollback รีเลย์ต้องปล่อย (OFF) เสมอ และห้าม auto-action จนกว่าจะอ่านตำแหน่งจริงได้
Watchdog: เปิด Task WDT ให้ทุก critical task ส่วน safety_task ควร feed hardware WDT ภายนอก (เช่น TPL5010) เป็นชั้นที่สอง
Packet versioning: ใส่ protoVersion ในทุก ESP-NOW packet ตั้งแต่ตอนนี้ จะได้อัปเกรดโปรโตคอลได้โดยไม่ทำให้แท็กเก่าใช้ไม่ได้
Secrets: เปิด Flash Encryption + Secure Boot v2 ก่อนส่งมอบจริง (ถ้าไม่เปิด ใครเข้าถึงบอร์ดได้ก็ดึง key ของแท็กทั้งหมดออกไปได้)

ถ้าจะเริ่มจริง สามารถเลือกพัฒนาได้ตามความสนใจ เช่น เขียนโครง ESP-NOW secure protocol v2, ออกแบบวงจร Tag v2 สำหรับไมโครแอมป์, หรือร่างโค้ด Command Arbiter + Policy engine บน ESP32-C3 ครับ