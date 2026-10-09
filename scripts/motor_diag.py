#!/usr/bin/env python3
"""AutoDoor-RF2 Motor Diagnostic Tool.

Executes controlled door sequences (Open -> Stop, or Open -> Stop -> Close)
and captures millisecond-precise event logs from http://192.168.1.11.
"""

import sys
import time
import json
import urllib.request
import urllib.parse
import argparse

if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
        sys.stderr.reconfigure(encoding="utf-8", errors="replace")
    except Exception:
        pass

DEFAULT_TARGET_IP = "192.168.1.11"

def api_call(endpoint: str, method: str = "GET", data: dict = None, ip: str = DEFAULT_TARGET_IP, timeout: float = 4.0):
    url = f"http://{ip}{endpoint}"
    req = urllib.request.Request(url, method=method)
    if data is not None:
        body = json.dumps(data).encode("utf-8")
        req.add_header("Content-Type", "application/json")
    else:
        body = None

    try:
        with urllib.request.urlopen(req, data=body, timeout=timeout) as resp:
            content = resp.read().decode("utf-8", errors="replace")
            return json.loads(content) if content else {}
    except Exception as e:
        print(f"[ERR] API {method} {endpoint} failed: {e}")
        return None

def clear_events(ip: str):
    api_call("/api/events/clear", method="POST", data={}, ip=ip)

def run_test_stop(ip: str, open_seconds: float = 5.0):
    print("=" * 60)
    print(f"🧪 [TEST 1: STOP ONLY] เป้าหมาย: http://{ip}")
    print("=" * 60)

    print("1. ล้าง Log เดิมในหน่วยความจำ ESP32...")
    clear_events(ip)
    time.sleep(0.5)

    print(f"2. สั่งเปิดประตู (OPEN)... (กำลังวิ่งเปิดนาน {open_seconds:.1f} วินาที)")
    res_open = api_call("/api/door", method="POST", data={"slot": "open"}, ip=ip)
    start_t = time.time()

    while True:
        elapsed = time.time() - start_t
        if elapsed >= open_seconds:
            break
        rem = open_seconds - elapsed
        print(f"   ⏱️ ประตูกำลังเปิด... เหลืออีก {rem:3.1f} วิ ก่อนสั่งหยุด", end="\r", flush=True)
        time.sleep(0.1)

    print(f"\n3. ครบ {open_seconds:.1f} วินาทีแล้ว! สั่งหยุดทันที (STOP - 800ms)...")
    res_stop = api_call("/api/door", method="POST", data={"slot": "stop"}, ip=ip)
    stop_t = time.time()

    time.sleep(1.0)

    print("\n4. ดึงข้อมูล Log จาก ESP32 เพื่อวิเคราะห์...")
    live = api_call("/api/live", ip=ip)
    if not live:
        print("[ERR] ไม่สามารถดึง Log จาก ESP32 ได้")
        return

    door_state = live.get("doorState", "unknown")
    events = live.get("recentEvents", [])

    print("-" * 60)
    print(f"📊 สถานะบอร์ดปัจจุบัน (doorState): {door_state}")
    print("📋 บันทึกเหตุการณ์จริงจาก ESP32 (เรียงตามเวลา):")
    print("-" * 60)
    for ev in reversed(events):
        print(f"  [{ev.get('time', '--:--:--')}] [{ev.get('type', '')}] {ev.get('msg', '')}")
    print("-" * 60)

def run_test_reverse(ip: str, open_seconds: float = 5.0, deadband_seconds: float = 2.5):
    print("=" * 60)
    print(f"🧪 [TEST 2: STOP ➔ CLOSE] เป้าหมาย: http://{ip}")
    print(f"   วิ่งเปิด {open_seconds:.1f}s ➔ STOP ➔ พัก Deadband {deadband_seconds:.1f}s ➔ CLOSE")
    print("=" * 60)

    print("1. ล้าง Log เดิมใน ESP32...")
    clear_events(ip)
    time.sleep(0.5)

    print(f"2. สั่งเปิดประตู (OPEN)...")
    api_call("/api/door", method="POST", data={"slot": "open"}, ip=ip)
    start_t = time.time()

    while True:
        elapsed = time.time() - start_t
        if elapsed >= open_seconds:
            break
        print(f"   ⏱️ กำลังเปิด... เหลือ {open_seconds - elapsed:3.1f} วิ ก่อนสั่งหยุด", end="\r", flush=True)
        time.sleep(0.1)

    print(f"\n3. สั่งหยุดทันที (STOP - 800ms)...")
    api_call("/api/door", method="POST", data={"slot": "stop"}, ip=ip)
    stop_t = time.time()

    while True:
        elapsed = time.time() - stop_t
        if elapsed >= deadband_seconds:
            break
        print(f"   ⏸️ พักเกียร์ (Deadband)... เหลือ {deadband_seconds - elapsed:3.1f} วิ ก่อนสั่งปิด", end="\r", flush=True)
        time.sleep(0.1)

    print(f"\n4. ครบ Deadband {deadband_seconds:.1f}s แล้ว! สั่งปิดทันที (CLOSE)...")
    api_call("/api/door", method="POST", data={"slot": "close"}, ip=ip)

    time.sleep(1.5)

    print("\n5. ดึงข้อมูล Log จาก ESP32...")
    live = api_call("/api/live", ip=ip)
    if not live:
        print("[ERR] ไม่สามารถดึง Log จาก ESP32 ได้")
        return

    door_state = live.get("doorState", "unknown")
    events = live.get("recentEvents", [])

    print("-" * 60)
    print(f"📊 สถานะบอร์ดปัจจุบัน (doorState): {door_state}")
    print("📋 บันทึกเหตุการณ์จริงจาก ESP32:")
    print("-" * 60)
    for ev in reversed(events):
        print(f"  [{ev.get('time', '--:--:--')}] [{ev.get('type', '')}] {ev.get('msg', '')}")
    print("-" * 60)

def run_test_close_to_open(ip: str):
    print("=" * 60)
    print(f"🧪 [TEST: CLOSE ➔ STOP ➔ OPEN] เป้าหมาย: http://{ip}")
    print("=" * 60)

    print("1. ล้าง Log เดิมใน ESP32...")
    clear_events(ip)
    time.sleep(0.5)

    print("2. สั่งเปิดประตู 8 วินาที เพื่อสร้างระยะวิ่งปิด...")
    api_call("/api/door", method="POST", data={"slot": "open"}, ip=ip)
    time.sleep(8.0)

    print("3. สั่งหยุด...")
    api_call("/api/door", method="POST", data={"slot": "stop"}, ip=ip)
    time.sleep(2.5)

    print("4. สั่งเริ่มเลื่อนปิด (CLOSE)... (วิ่งปิด 3.5 วินาที)")
    api_call("/api/door", method="POST", data={"slot": "close"}, ip=ip)
    time.sleep(3.5)

    print("5. จังหวะประตูกำลังปิด: สั่งหยุดทันที (STOP - 800ms)...")
    api_call("/api/door", method="POST", data={"slot": "stop"}, ip=ip)
    time.sleep(2.5)

    print("6. พัก Deadband ครบ 2.5s แล้ว! สั่งเปิดทันที (OPEN)...")
    api_call("/api/door", method="POST", data={"slot": "open"}, ip=ip)
    time.sleep(4.0)

    print("\n7. ดึงข้อมูล Log จาก ESP32...")
    live = api_call("/api/live", ip=ip)
    if not live:
        print("[ERR] ไม่สามารถดึง Log จาก ESP32 ได้")
        return

    door_state = live.get("doorState", "unknown")
    events = live.get("recentEvents", [])

    print("-" * 60)
    print(f"📊 สถานะบอร์ดปัจจุบัน (doorState): {door_state}")
    print("📋 บันทึกเหตุการณ์จริงจาก ESP32:")
    print("-" * 60)
    for ev in reversed(events):
        print(f"  [{ev.get('time', '--:--:--')}] [{ev.get('type', '')}] {ev.get('msg', '')}")
    print("-" * 60)

def run_test_sim_drive_through(ip: str):
    print("=" * 60)
    print(f"🚗💨 [SIMULATION TEST: FAST DRIVE-THROUGH] เป้าหมาย: http://{ip}")
    print("=" * 60)

    print("1. ล้าง Log และตั้งค่าเซ็นเซอร์เป็นสถานะปกติ...")
    clear_events(ip)
    api_call("/api/sim/safety", method="POST", data={"blocked": None}, ip=ip)
    time.sleep(0.5)

    print("2. [T+0.0s] สั่งเปิดประตู (OPEN) ➔ ประตูเริ่มวิ่งเปิด...")
    api_call("/api/door", method="POST", data={"slot": "open"}, ip=ip)
    start_t = time.time()

    for i in range(40):
        elapsed = time.time() - start_t
        if elapsed >= 4.0:
            break
        print(f"   ⏱️ ประตูกำลังเปิด... {elapsed:3.1f}s / 4.0s ก่อนรถวิ่งตัดเซ็นเซอร์", end="\r", flush=True)
        time.sleep(0.1)

    print("\n3. [T+4.0s] 🚨 จำลองรถขับเข้ามาตัดลำแสงเซ็นเซอร์ (BEAM CUT)!")
    print("   👉 ดูผล: ประตูต้องสั่ง STOP เบรกหยุดนิ่งทันที!")
    api_call("/api/sim/safety", method="POST", data={"blocked": True}, ip=ip)
    cut_t = time.time()

    for i in range(30):
        elapsed = time.time() - cut_t
        if elapsed >= 3.0:
            break
        print(f"   🚗 รถกำลังแล่นผ่านลำแสง... {elapsed:3.1f}s / 3.0s", end="\r", flush=True)
        time.sleep(0.1)

    print("\n4. [T+7.0s] 🟢 จำลองรถแล่นพ้นลำแสงออกไปแล้ว (BEAM CLEARED)!")
    print("   👉 ดูผล: ระบบเริ่มนับถอยหลังปิดเร็วตามเวลาเซ็นเซอร์ (3 วินาที)...")
    api_call("/api/sim/safety", method="POST", data={"blocked": False}, ip=ip)
    clear_t = time.time()

    for i in range(35):
        elapsed = time.time() - clear_t
        if elapsed >= 3.2:
            break
        print(f"   ⏱️ นับถอยหลังปิดเร็ว... {elapsed:3.1f}s / 3.0s", end="\r", flush=True)
        time.sleep(0.1)

    print("\n5. [T+10.2s] 🚪 ครบเวลานับถอยหลัง! ระบบต้องสั่ง CLOSE ปิดตามหลังรถทันที!")
    time.sleep(2.0)

    # คืนค่าเซ็นเซอร์กลับสู่โหมดฮาร์ดแวร์จริง
    api_call("/api/sim/safety", method="POST", data={"blocked": None}, ip=ip)

    print("6. รอประตูเลื่อนปิดจนชนแม่เหล็ก (ประมาณ 10-15 วินาที)...")
    for i in range(15):
        time.sleep(1.0)
        live = api_call("/api/live", ip=ip)
        st = live.get("doorState", "unknown") if live else "unknown"
        print(f"   🚪 สถานะประตู: {st}...", end="\r", flush=True)
        if st == "closed":
            break

    print("\n\n7. ดึงข้อมูลบันทึกเหตุการณ์จริงจาก ESP32...")
    live = api_call("/api/live", ip=ip)
    events = live.get("recentEvents", []) if live else []
    door_state = live.get("doorState", "unknown") if live else "unknown"

    print("-" * 60)
    print(f"📊 สถานะบอร์ดปัจจุบัน (doorState): {door_state}")
    print("📋 บันทึกเหตุการณ์จริงจาก ESP32:")
    print("-" * 60)
    for ev in reversed(events):
        print(f"  [{ev.get('time', '--:--:--')}] [{ev.get('type', '')}] {ev.get('msg', '')}")
    print("-" * 60)

def run_test_sim_anti_pinch(ip: str):
    print("=" * 60)
    print(f"🚨 [TEST 1: EMERGENCY ANTI-PINCH REVERSAL] เป้าหมาย: http://{ip}")
    print("=" * 60)

    print("1. ล้าง Log และตั้งค่าเซ็นเซอร์เป็นสถานะปกติ...")
    clear_events(ip)
    api_call("/api/sim/safety", method="POST", data={"blocked": None}, ip=ip)
    time.sleep(0.5)

    print("2. สั่งเปิดประตู 8 วินาที เพื่อสร้างระยะให้ประตูกำลังเลื่อนปิด...")
    api_call("/api/door", method="POST", data={"slot": "open"}, ip=ip)
    time.sleep(8.0)

    print("3. สั่งหยุด...")
    api_call("/api/door", method="POST", data={"slot": "stop"}, ip=ip)
    time.sleep(2.5)

    print("4. สั่งเริ่มเลื่อนปิด (CLOSE)... (ประตูกำลังวิ่งปิด)")
    api_call("/api/door", method="POST", data={"slot": "close"}, ip=ip)
    time.sleep(2.5)

    print("\n5. 🚨 [EMERGENCY] มีวัตถุตัดลำแสงขณะกำลังเลื่อนปิด (BEAM CUT)!")
    print("   👉 ดูผล: ประตูต้องสั่ง SafeAbort หยุดทันที และดีดกลับไปเปิด (Reverse to Open)!")
    api_call("/api/sim/safety", method="POST", data={"blocked": True}, ip=ip)
    time.sleep(3.0)

    print("\n6. 🟢 วัตถุพ้นลำแสงออกไปแล้ว (BEAM CLEARED)...")
    api_call("/api/sim/safety", method="POST", data={"blocked": False}, ip=ip)
    time.sleep(2.0)

    # คืนค่าเซ็นเซอร์จริง
    api_call("/api/sim/safety", method="POST", data={"blocked": None}, ip=ip)

    print("7. รอตรวจสอบผลลัพธ์และดูสถานะประตู...")
    time.sleep(4.0)

    live = api_call("/api/live", ip=ip)
    events = live.get("recentEvents", []) if live else []
    door_state = live.get("doorState", "unknown") if live else "unknown"

    print("-" * 60)
    print(f"📊 สถานะบอร์ดปัจจุบัน (doorState): {door_state}")
    print("📋 บันทึกเหตุการณ์จริงจาก ESP32:")
    print("-" * 60)
    for ev in reversed(events):
        print(f"  [{ev.get('time', '--:--:--')}] [{ev.get('type', '')}] {ev.get('msg', '')}")
    print("-" * 60)

def run_test_sim_blocked_prevention(ip: str):
    print("=" * 60)
    print(f"🛑 [TEST 2: BLOCKED BEAM CLOSE PREVENTION] เป้าหมาย: http://{ip}")
    print("=" * 60)

    print("1. ล้าง Log และคืนค่าเซ็นเซอร์...")
    clear_events(ip)
    api_call("/api/sim/safety", method="POST", data={"blocked": None}, ip=ip)
    time.sleep(0.5)

    print("2. สั่งเปิดประตู 6 วินาที...")
    api_call("/api/door", method="POST", data={"slot": "open"}, ip=ip)
    time.sleep(6.0)

    print("3. สั่งหยุด...")
    api_call("/api/door", method="POST", data={"slot": "stop"}, ip=ip)
    time.sleep(2.0)

    print("\n4. 🛑 จำลองมีรถจอดขวางลำแสงแช่ไว้ (BEAM BLOCKED ค้างไว้)...")
    api_call("/api/sim/safety", method="POST", data={"blocked": True}, ip=ip)
    time.sleep(1.0)

    print("5. ⚠️ ลองสั่งปิดประตู (CLOSE) ขณะมีสิ่งกีดขวางขวางอยู่...")
    res_close = api_call("/api/door", method="POST", data={"slot": "close"}, ip=ip)
    print(f"   👉 ผลลัพธ์จากบอร์ด: {res_close}")
    if res_close and res_close.get("ok") is False:
        print("   ✅ บอร์ดปฏิเสธคำสั่งปิดตามที่คาดไว้! (Command Rejected Successfully)")
    time.sleep(3.0)

    print("\n6. 🟢 จำลองรถขับพ้นลำแสงออกไปแล้ว (BEAM CLEARED)...")
    api_call("/api/sim/safety", method="POST", data={"blocked": False}, ip=ip)
    print("   👉 ดูผล: ระบบตรวจพบว่าลำแสงโล่ง ➔ เริ่มนับถอยหลังปิดเร็วตามเวลาเซ็นเซอร์...")
    time.sleep(4.0)

    # คืนค่าเซ็นเซอร์จริง
    api_call("/api/sim/safety", method="POST", data={"blocked": None}, ip=ip)

    print("7. รอประตูเลื่อนปิดจนชนแม่เหล็ก (ประมาณ 10 วินาที)...")
    for i in range(15):
        time.sleep(1.0)
        live = api_call("/api/live", ip=ip)
        st = live.get("doorState", "unknown") if live else "unknown"
        print(f"   🚪 สถานะประตู: {st}...", end="\r", flush=True)
        if st == "closed":
            break

    print("\n\n8. ดึงข้อมูลบันทึกเหตุการณ์จริงจาก ESP32...")
    live = api_call("/api/live", ip=ip)
    events = live.get("recentEvents", []) if live else []
    door_state = live.get("doorState", "unknown") if live else "unknown"

    print("-" * 60)
    print(f"📊 สถานะบอร์ดปัจจุบัน (doorState): {door_state}")
    print("📋 บันทึกเหตุการณ์จริงจาก ESP32:")
    print("-" * 60)
    for ev in reversed(events):
        print(f"  [{ev.get('time', '--:--:--')}] [{ev.get('type', '')}] {ev.get('msg', '')}")
    print("-" * 60)

def run_test_sim_multiple_passages(ip: str):
    print("=" * 60)
    print(f"🚶‍♂️🚗 [TEST 3: HESITANT CROSSING / MULTIPLE CARS] เป้าหมาย: http://{ip}")
    print("=" * 60)

    print("1. ล้าง Log และคืนค่าเซ็นเซอร์...")
    clear_events(ip)
    api_call("/api/sim/safety", method="POST", data={"blocked": None}, ip=ip)
    time.sleep(0.5)

    print("2. สั่งเปิดประตู 6 วินาที...")
    api_call("/api/door", method="POST", data={"slot": "open"}, ip=ip)
    time.sleep(6.0)

    print("3. สั่งหยุด...")
    api_call("/api/door", method="POST", data={"slot": "stop"}, ip=ip)
    time.sleep(2.0)

    print("\n4. 🚗 รถคันที่ 1 วิ่งผ่านลำแสง (ตัด 2 วินาที แล้วพ้นไป)...")
    api_call("/api/sim/safety", method="POST", data={"blocked": True}, ip=ip)
    time.sleep(2.0)
    api_call("/api/sim/safety", method="POST", data={"blocked": False}, ip=ip)
    print("   👉 ลำแสงโล่งแล้ว! ระบบเริ่มนับถอยหลังปิด...")
    time.sleep(1.5)

    print("\n5. ⚠️ ขณะกำลังนับถอยหลัง: รถคันที่ 2 (หรือคน) วิ่งตัดลำแสงซ้ำทันที!")
    print("   👉 ดูผล: ตัวนับเวลาถอยหลังต้องถูกยกเลิกทันที (Reset Countdown) ห้ามปิดเด็ดขาด!")
    api_call("/api/sim/safety", method="POST", data={"blocked": True}, ip=ip)
    time.sleep(3.0)

    print("\n6. 🟢 รถคันที่ 2 แล่นพ้นลำแสงออกไปแล้ว...")
    api_call("/api/sim/safety", method="POST", data={"blocked": False}, ip=ip)
    print("   👉 ดูผล: ระบบเริ่มนับถอยหลังรอบใหม่ ➔ ครบเวลาแล้วเลื่อนปิดตามหลัง...")
    time.sleep(4.0)

    # คืนค่าเซ็นเซอร์จริง
    api_call("/api/sim/safety", method="POST", data={"blocked": None}, ip=ip)

    print("7. รอประตูเลื่อนปิดจนชนแม่เหล็ก...")
    for i in range(15):
        time.sleep(1.0)
        live = api_call("/api/live", ip=ip)
        st = live.get("doorState", "unknown") if live else "unknown"
        print(f"   🚪 สถานะประตู: {st}...", end="\r", flush=True)
        if st == "closed":
            break

    print("\n\n8. ดึงข้อมูลบันทึกเหตุการณ์จริงจาก ESP32...")
    live = api_call("/api/live", ip=ip)
    events = live.get("recentEvents", []) if live else []
    door_state = live.get("doorState", "unknown") if live else "unknown"

    print("-" * 60)
    print(f"📊 สถานะบอร์ดปัจจุบัน (doorState): {door_state}")
    print("📋 บันทึกเหตุการณ์จริงจาก ESP32:")
    print("-" * 60)
    for ev in reversed(events):
        print(f"  [{ev.get('time', '--:--:--')}] [{ev.get('type', '')}] {ev.get('msg', '')}")
    print("-" * 60)

def run_test_sim_quick_pass(ip: str, open_seconds: float = 2.5):
    print("=" * 60)
    print(f"⚡ [SIMULATION: QUICK DRIVE-THROUGH (เปิดแป๊บเดียวพอรถผ่าน)] เป้าหมาย: http://{ip}")
    print(f"   เปิดเพียง {open_seconds:.1f} วินาที ➔ รถขับพุ่งตัดลำแสงทันที ➔ ปิดตามหลังอย่างรวดเร็ว")
    print("=" * 60)

    print("1. ล้าง Log และคืนค่าเซ็นเซอร์...")
    clear_events(ip)
    api_call("/api/sim/safety", method="POST", data={"blocked": None}, ip=ip)
    time.sleep(0.5)

    print(f"2. [T+0.0s] สั่งเปิดประตู (OPEN) ➔ ประตูเริ่มแง้มเปิด (รอแค่ {open_seconds:.1f} วินาที)...")
    api_call("/api/door", method="POST", data={"slot": "open"}, ip=ip)
    start_t = time.time()

    for i in range(int(open_seconds * 10)):
        elapsed = time.time() - start_t
        if elapsed >= open_seconds:
            break
        print(f"   ⏱️ ประตูกำลังแง้มเปิด... {elapsed:3.1f}s / {open_seconds:.1f}s (พอดีคันรถ)", end="\r", flush=True)
        time.sleep(0.1)

    print(f"\n3. [T+{open_seconds:.1f}s] 🚨 พอเปิดได้ช่องพอดี รถพุ่งตัดลำแสงเซ็นเซอร์ทันที (BEAM CUT)!")
    print("   👉 ดูผล: ประตูต้องสั่ง STOP เบรกหยุดทันที ไม่วิ่งเปิดต่อ!")
    api_call("/api/sim/safety", method="POST", data={"blocked": True}, ip=ip)
    cut_t = time.time()

    for i in range(20):
        elapsed = time.time() - cut_t
        if elapsed >= 2.0:
            break
        print(f"   🚗 รถกำลังแล่นผ่านช่องประตู... {elapsed:3.1f}s / 2.0s", end="\r", flush=True)
        time.sleep(0.1)

    print("\n4. [T+4.5s] 🟢 ท้ายรถพ้นลำแสงออกไปแล้ว (BEAM CLEARED)!")
    print("   👉 ดูผล: Deadband พักเกียร์ครบพอดีขณะรถผ่าน ➔ สั่ง CLOSE ปิดตามหลังทันที!")
    api_call("/api/sim/safety", method="POST", data={"blocked": False}, ip=ip)
    time.sleep(2.5)

    # คืนค่าเซ็นเซอร์จริง
    api_call("/api/sim/safety", method="POST", data={"blocked": None}, ip=ip)

    print("5. รอประตูเลื่อนปิดชนแม่เหล็ก (เนื่องจากเปิดแค่นิดเดียว จะปิดสนิทไวมาก)...")
    for i in range(12):
        time.sleep(0.8)
        live = api_call("/api/live", ip=ip)
        st = live.get("doorState", "unknown") if live else "unknown"
        print(f"   🚪 สถานะประตู: {st}...", end="\r", flush=True)
        if st == "closed":
            break

    print("\n\n6. ดึงข้อมูลบันทึกเหตุการณ์จริงจาก ESP32...")
    live = api_call("/api/live", ip=ip)
    events = live.get("recentEvents", []) if live else []
    door_state = live.get("doorState", "unknown") if live else "unknown"

    print("-" * 60)
    print(f"📊 สถานะบอร์ดปัจจุบัน (doorState): {door_state}")
    print("📋 บันทึกเหตุการณ์จริงจาก ESP32:")
    print("-" * 60)
    for ev in reversed(events):
        print(f"  [{ev.get('time', '--:--:--')}] [{ev.get('type', '')}] {ev.get('msg', '')}")
    print("-" * 60)

def run_direct_close(ip: str):
    print("สั่งปิดประตู (CLOSE)...")
    res = api_call("/api/door", method="POST", data={"slot": "close"}, ip=ip)
    print("ผลลัพธ์:", res)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="AutoDoor-RF2 Motor Diagnostic")
    parser.add_argument("--test", choices=["stop", "reverse", "close", "close-to-open", "sim-drive-through", "sim-anti-pinch", "sim-blocked", "sim-multiple", "sim-quick-pass"], default="stop", help="Test to run")
    parser.add_argument("--ip", default=DEFAULT_TARGET_IP, help="Target IP")
    parser.add_argument("--open-time", type=float, default=5.0, help="Seconds to let door open before stop")
    parser.add_argument("--deadband", type=float, default=2.5, help="Deadband seconds before close (test reverse)")
    args = parser.parse_args()

    if args.test == "stop":
        run_test_stop(args.ip, args.open_time)
    elif args.test == "reverse":
        run_test_reverse(args.ip, args.open_time, args.deadband)
    elif args.test == "close":
        run_direct_close(args.ip)
    elif args.test == "close-to-open":
        run_test_close_to_open(args.ip)
    elif args.test == "sim-drive-through":
        run_test_sim_drive_through(args.ip)
    elif args.test == "sim-anti-pinch":
        run_test_sim_anti_pinch(args.ip)
    elif args.test == "sim-blocked":
        run_test_sim_blocked_prevention(args.ip)
    elif args.test == "sim-multiple":
        run_test_sim_multiple_passages(args.ip)
    elif args.test == "sim-quick-pass":
        run_test_sim_quick_pass(args.ip)
