#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
AutoDoor ESP32-C3 1-Click Firmware Flasher
=========================================
Tool for 1-click firmware flashing with automatic port detection,
partition verification, and dual-mode upload (USB Serial / WiFi OTA).
"""

import os
import sys
import time
import json
import struct
import argparse
import subprocess
import threading
from pathlib import Path

# Fix Windows console encoding for UTF-8 and Thai symbols
if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
        sys.stderr.reconfigure(encoding="utf-8", errors="replace")
    except Exception:
        pass

DIR = Path(__file__).resolve().parent
BIN_DIR = DIR / "bin"

BOOTLOADER_BIN = BIN_DIR / "bootloader.bin"
PARTITIONS_BIN = BIN_DIR / "partitions.bin"
BOOT_APP0_BIN  = BIN_DIR / "boot_app0.bin"
FIRMWARE_BIN   = BIN_DIR / "firmware.bin"

DEFAULT_OTA_IP = "192.168.1.11"

# Correct Offsets according to partitions_two_ota.csv (ESP32-C3 4MB Flash)
OFFSETS = {
    "bootloader": 0x0000,
    "partitions": 0x8000,
    "otadata":    0x29000,
    "app0":       0x30000,
}

EXPECTED_PARTITIONS = [
    ("nvs",      0x01, 0x02, 0x9000,   0x20000),   # 128 KB
    ("otadata",  0x01, 0x00, 0x29000,  0x2000),    # 8 KB
    ("app0",     0x00, 0x10, 0x30000,  0x1D0000),  # 1856 KB (~1.81 MB)
    ("app1",     0x00, 0x11, 0x200000, 0x1D0000),  # 1856 KB (~1.81 MB)
    ("spiffs",   0x01, 0x82, 0x3D0000, 0x30000),   # 192 KB
]

def format_size(size_bytes: int) -> str:
    """Format bytes to human-readable size."""
    if size_bytes >= 1024 * 1024:
        return f"{size_bytes / (1024 * 1024):.2f} MB"
    elif size_bytes >= 1024:
        return f"{size_bytes / 1024:.1f} KB"
    return f"{size_bytes} B"

def verify_partition_table(bin_path: Path) -> tuple[bool, str, list]:
    """Verify partitions.bin against expected partition layout."""
    if not bin_path.exists():
        return False, f"Missing partition file: {bin_path.name}", []

    data = bin_path.read_bytes()
    entries = []
    # ESP32 partition table format: each entry is 32 bytes
    # offset 0..1: magic 0xAA50
    # offset 2: type
    # offset 3: subtype
    # offset 4..7: offset (uint32)
    # offset 8..11: size (uint32)
    # offset 12..27: label (16 bytes null-terminated)
    # offset 28..31: flags (uint32)
    for i in range(0, len(data), 32):
        chunk = data[i:i+32]
        if len(chunk) < 32:
            break
        magic, p_type, p_subtype, p_offset, p_size = struct.unpack("<HBBII", chunk[:12])
        if magic != 0x50AA:  # 0xAA50 in little-endian
            continue
        label = chunk[12:28].split(b'\x00')[0].decode("ascii", errors="replace")
        entries.append({
            "name": label,
            "type": p_type,
            "subtype": p_subtype,
            "offset": p_offset,
            "size": p_size,
        })

    # Validate against expected layout
    errors = []
    for exp_name, exp_type, exp_sub, exp_off, exp_sz in EXPECTED_PARTITIONS:
        found = next((e for e in entries if e["name"] == exp_name), None)
        if not found:
            errors.append(f"Partition '{exp_name}' is missing!")
        else:
            if found["offset"] != exp_off:
                errors.append(f"Partition '{exp_name}' offset is 0x{found['offset']:X} (expected 0x{exp_off:X})")
            if found["size"] != exp_sz:
                errors.append(f"Partition '{exp_name}' size is 0x{found['size']:X} (expected 0x{exp_sz:X})")

    if errors:
        return False, "; ".join(errors), entries
    return True, "All partitions match partitions_two_ota.csv layout perfectly", entries

def print_partition_table_status():
    """Print partition layout verification table to console."""
    ok, msg, entries = verify_partition_table(PARTITIONS_BIN)
    print("\n" + "=" * 70)
    print("📋 ตรวจสอบโครงสร้าง Partition Table (ESP32-C3 4MB 2x OTA)")
    print("=" * 70)
    print(f"{'ชื่อ (Name)':<10} {'Offset (Hex)':<14} {'ขนาด (Size)':<14} {'การใช้งาน':<25}")
    print("-" * 70)

    roles = {
        "nvs":     "เก็บข้อมูล Tags, WiFi, รหัสผ่าน",
        "otadata": "ควบคุมการบูตสลับพาร์ติชัน OTA",
        "app0":    "แอปหลักช่องที่ 1 (บูตแรกเริ่ม)",
        "app1":    "แอปสำรองช่องที่ 2 (สำหรับสลับ OTA)",
        "spiffs":  "พื้นที่เก็บไฟล์ระบบ SPIFFS",
    }

    for e in entries:
        name = e["name"]
        off = f"0x{e['offset']:06X}"
        sz = f"{format_size(e['size'])} (0x{e['size']:X})"
        role = roles.get(name, "System data")
        print(f"{name:<10} {off:<14} {sz:<14} {role:<25}")

    print("-" * 70)
    if ok:
        print(f"✅ สถานะการแบ่ง Partition: ถูกต้อง 100% ({msg})")
    else:
        print(f"❌ สถานะการแบ่ง Partition: ผิดพลาด! ({msg})")

    # Check firmware file
    if FIRMWARE_BIN.exists():
        fw_size = FIRMWARE_BIN.stat().st_size
        max_app = 0x1D0000  # 1,900,544 bytes
        pct = (fw_size / max_app) * 100
        print(f"📦 ไฟล์เฟิร์มแวร์ firmware.bin : {format_size(fw_size)} / {format_size(max_app)} ({pct:.1f}% ของ app0)")
        if fw_size > max_app:
            print("⚠️ คำเตือน: ขนาดเฟิร์มแวร์ใหญ่เกินขนาด Partition app0!")
    else:
        print("❌ ไม่พบไฟล์ bin/firmware.bin!")
    print("=" * 70 + "\n")

def find_serial_ports() -> list[dict]:
    """Find available COM ports and highlight likely ESP32 devices."""
    try:
        import serial.tools.list_ports
        ports = list(serial.tools.list_ports.comports())
    except ImportError:
        # Fallback using PowerShell
        try:
            cmd = ["powershell", "-Command", "Get-CimInstance Win32_SerialPort | Select-Object DeviceID, Description | ConvertTo-Json"]
            res = subprocess.run(cmd, capture_output=True, text=True)
            data = json.loads(res.stdout) if res.stdout.strip() else []
            if isinstance(data, dict):
                data = [data]
            ports = []
            for item in data:
                class DummyPort:
                    pass
                p = DummyPort()
                p.device = item.get("DeviceID", "")
                p.description = item.get("Description", "")
                p.hwid = ""
                ports.append(p)
        except Exception:
            ports = []

    res = []
    for p in ports:
        desc = p.description or ""
        dev = p.device or ""
        # Ignore Standard Bluetooth serial ports
        is_bt = "Bluetooth" in desc or "BTHENUM" in getattr(p, "hwid", "")
        # Highlight probable ESP32 / USB-Serial ports
        is_esp = any(k in desc.lower() for k in ["usb serial", "ch340", "cp210", "ftdi", "esp32", "jtag"])
        res.append({
            "port": dev,
            "desc": desc,
            "is_bt": is_bt,
            "is_esp": is_esp and not is_bt,
        })
    return res

def auto_detect_esp32_c3_port() -> str:
    """Scan and auto-detect ESP32-C3 port."""
    ports = find_serial_ports()
    esp_candidates = [p for p in ports if p["is_esp"]]

    # If only one likely USB-Serial device found, probe or use it
    if len(esp_candidates) == 1:
        return esp_candidates[0]["port"]

    # If multiple candidates, test with esptool to find ESP32-C3
    for p in esp_candidates:
        port_name = p["port"]
        try:
            cmd = [sys.executable, "-m", "esptool", "--port", port_name, "chip-id"]
            proc = subprocess.run(cmd, capture_output=True, text=True, timeout=3)
            if "ESP32-C3" in proc.stdout:
                return port_name
        except Exception:
            pass

    if esp_candidates:
        return esp_candidates[0]["port"]

    # Fallback to any non-bluetooth port
    non_bt = [p for p in ports if not p["is_bt"]]
    if len(non_bt) == 1:
        return non_bt[0]["port"]

    return ""

def flash_usb(port: str, baud: int = 460800, log_callback=print) -> bool:
    """Flash bootloader, partitions, boot_app0, and firmware to ESP32-C3."""
    for f in [BOOTLOADER_BIN, PARTITIONS_BIN, BOOT_APP0_BIN, FIRMWARE_BIN]:
        if not f.exists():
            log_callback(f"❌ Error: Required file {f.name} is missing in {BIN_DIR}")
            return False

    log_callback(f"🚀 เตรียมแฟลช ESP32-C3 บนพอร์ต {port} (Baud rate: {baud})...")
    log_callback("📌 พิกัด Flash Offsets (ตรงตาม partitions_two_ota.csv):")
    log_callback(f"   - 0x{OFFSETS['bootloader']:04X} ➔ bootloader.bin ({format_size(BOOTLOADER_BIN.stat().st_size)})")
    log_callback(f"   - 0x{OFFSETS['partitions']:04X} ➔ partitions.bin ({format_size(PARTITIONS_BIN.stat().st_size)})")
    log_callback(f"   - 0x{OFFSETS['otadata']:05X} ➔ boot_app0.bin   ({format_size(BOOT_APP0_BIN.stat().st_size)})")
    log_callback(f"   - 0x{OFFSETS['app0']:05X} ➔ firmware.bin     ({format_size(FIRMWARE_BIN.stat().st_size)})")

    cmd = [
        sys.executable, "-m", "esptool",
        "--chip", "esp32c3",
        "--port", port,
        "--baud", str(baud),
        "--before", "default_reset",
        "--after", "hard_reset",
        "write_flash", "-z",
        "--flash_mode", "dio",
        "--flash_freq", "80m",
        "--flash_size", "4MB",
        f"0x{OFFSETS['bootloader']:X}", str(BOOTLOADER_BIN),
        f"0x{OFFSETS['partitions']:X}", str(PARTITIONS_BIN),
        f"0x{OFFSETS['otadata']:X}",    str(BOOT_APP0_BIN),
        f"0x{OFFSETS['app0']:X}",       str(FIRMWARE_BIN),
    ]

    log_callback("\n⚡ กำลังส่งข้อมูลไปยัง ESP32-C3 โปรดรอสักครู่...")
    start_time = time.time()
    try:
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            encoding="utf-8",
            errors="replace"
        )
        for line in proc.stdout:
            line_clean = line.strip()
            if line_clean:
                log_callback("   " + line_clean)

        proc.wait()
        elapsed = time.time() - start_time

        if proc.returncode == 0:
            log_callback(f"\n🎉 แฟลชเฟิร์มแวร์สำเร็จเรียบร้อย! (ใช้เวลา {elapsed:.1f} วินาที)")
            log_callback("💡 บอร์ด ESP32-C3 รีเซ็ตและพร้อมทำงานแล้ว")
            return True
        else:
            log_callback(f"\n❌ การแฟลชล้มเหลว (รหัสข้อผิดพลาด: {proc.returncode})")
            log_callback("💡 คำแนะนำ: หากเชื่อมต่อไม่ได้ ให้กดปุ่ม BOOT บนบอร์ดค้างไว้ขณะเสียบสาย USB แล้วลองใหม่อีกครั้ง")
            return False
    except Exception as e:
        log_callback(f"❌ Exception during flash: {e}")
        return False

def flash_ota(ip: str = DEFAULT_OTA_IP, log_callback=print) -> bool:
    """Flash firmware via WiFi OTA to ESP32."""
    if not FIRMWARE_BIN.exists():
        log_callback(f"❌ Error: {FIRMWARE_BIN.name} not found!")
        return False

    ota_script = DIR.parent / "scripts" / "ota_upload.py"
    if ota_script.exists():
        cmd = [sys.executable, str(ota_script), "--ip", ip, "--bin", str(FIRMWARE_BIN)]
        log_callback(f"🌐 กำลังส่ง Firmware ผ่าน OTA ไปยัง {ip}...")
        proc = subprocess.run(cmd)
        return proc.returncode == 0
    else:
        log_callback(f"❌ ota_upload.py not found at {ota_script}")
        return False

def run_cli_interactive():
    """CLI Menu for 1-Click flasher."""
    print("=================================================================")
    print("🚪 AutoDoor-RF2 : ESP32-C3 1-Click Firmware Flasher")
    print("=================================================================")

    # 1. Print Partition Verification Table
    print_partition_table_status()

    # 2. Detect Ports
    detected_port = auto_detect_esp32_c3_port()
    ports = find_serial_ports()
    non_bt = [p for p in ports if not p["is_bt"]]

    print("🔌 รายการพอร์ต Serial ที่พบ:")
    if not non_bt:
        print("   (ไม่พบพอร์ต Serial กรุณาเสียบสาย USB ของบอร์ด ESP32-C3)")
    else:
        for idx, p in enumerate(non_bt, 1):
            tag = " 🌟 [ESP32-C3 แนะนำ]" if p["port"] == detected_port else ""
            print(f"   [{idx}] {p['port']} : {p['desc']}{tag}")

    print("\nตัวเลือกการทำงาน:")
    default_prompt = f" (กด Enter เพื่อเลือก {detected_port})" if detected_port else ""
    print(f" [1] ⚡ 1-Click Flash ผ่านสาย USB{default_prompt}")
    print(f" [2] 🌐 อัปเดตผ่าน WiFi OTA (เป้าหมาย: {DEFAULT_OTA_IP})")
    print(" [3] 🔍 ตรวจสอบข้อมูลชิปบนพอร์ต Serial (Chip Info)")
    print(" [4] 🖥️  เปิดหน้าต่างโปรแกรมแบบกราฟิก (GUI Mode)")
    print(" [0] ออกจากโปรแกรม")

    choice = input("\n👉 กรุณาพิมพ์ตัวเลขแล้วกด Enter [1]: ").strip()
    if choice == "" or choice == "1":
        port = detected_port
        if not port:
            if non_bt:
                sel = input("เลือกหมายเลขพอร์ต [1]: ").strip() or "1"
                try:
                    port = non_bt[int(sel) - 1]["port"]
                except Exception:
                    port = non_bt[0]["port"]
            else:
                port = input("กรุณาระบุพอร์ตด้วยตนเอง (เช่น COM37): ").strip().upper()

        if not port:
            print("❌ ไม่ได้เลือกพอร์ต ยกเลิกการทำงาน")
            return
        flash_usb(port)

    elif choice == "2":
        ip = input(f"ระบุ IP ของ AutoDoor [{DEFAULT_OTA_IP}]: ").strip() or DEFAULT_OTA_IP
        flash_ota(ip)

    elif choice == "3":
        port = detected_port or (non_bt[0]["port"] if non_bt else "COM37")
        print(f"\n🔍 กำลังอ่านข้อมูลชิปจาก {port}...")
        subprocess.run([sys.executable, "-m", "esptool", "--port", port, "flash-id"])

    elif choice == "4":
        run_gui()

def run_gui():
    """Launch modern Tkinter GUI."""
    try:
        import tkinter as tk
        from tkinter import ttk, messagebox, scrolledtext
    except ImportError:
        print("Tkinter is not available. Please use CLI mode.")
        return

    root = tk.Tk()
    root.title("AutoDoor ESP32-C3 1-Click Firmware Flasher")
    root.geometry("780x620")
    root.minsize(720, 560)

    # Style
    bg_color = "#1e1e2e"
    card_bg = "#252538"
    accent_color = "#00b4d8"
    text_color = "#f8f9fa"
    btn_flash_bg = "#0077b6"
    btn_ota_bg = "#2a9d8f"

    root.configure(bg=bg_color)

    # Header
    header_frame = tk.Frame(root, bg=bg_color, pady=12)
    header_frame.pack(fill="x")

    title_lbl = tk.Label(
        header_frame,
        text="🚪 AutoDoor ESP32-C3 1-Click Flasher",
        font=("Segoe UI", 16, "bold"),
        fg="#90e0ef",
        bg=bg_color
    )
    title_lbl.pack(anchor="w", padx=20)

    subtitle_lbl = tk.Label(
        header_frame,
        text="ระบบแฟลชเฟิร์มแวร์ ESP32-C3 4MB Flash (ตรวจสอบ Partition 2x OTA อัตโนมัติ)",
        font=("Segoe UI", 10),
        fg="#adb5bd",
        bg=bg_color
    )
    subtitle_lbl.pack(anchor="w", padx=20)

    # Main Container
    container = tk.Frame(root, bg=bg_color, padx=20, pady=5)
    container.pack(fill="both", expand=True)

    # Port Selection Card
    port_card = tk.LabelFrame(container, text=" 🔌 ตั้งค่าการเชื่อมต่อ (Connection Setup) ", font=("Segoe UI", 10, "bold"), fg="#caf0f8", bg=card_bg, padx=15, pady=10)
    port_card.pack(fill="x", pady=6)

    tk.Label(port_card, text="Serial Port:", font=("Segoe UI", 10), fg=text_color, bg=card_bg).grid(row=0, column=0, sticky="w", pady=4)

    port_var = tk.StringVar()
    port_combo = ttk.Combobox(port_card, textvariable=port_var, width=32, state="readonly", font=("Segoe UI", 10))
    port_combo.grid(row=0, column=1, padx=10, pady=4, sticky="w")

    def refresh_ports():
        ports = find_serial_ports()
        non_bt = [p for p in ports if not p["is_bt"]]
        detected = auto_detect_esp32_c3_port()
        port_list = []
        default_idx = 0
        for i, p in enumerate(non_bt):
            label = f"{p['port']} ({p['desc']})"
            port_list.append(label)
            if p["port"] == detected:
                default_idx = i

        port_combo["values"] = port_list
        if port_list:
            port_combo.current(default_idx)
            port_var.set(port_list[default_idx])
        else:
            port_combo.set("ไม่พบพอร์ต Serial")

    btn_refresh = tk.Button(port_card, text="🔄 สแกนพอร์ตใหม่", command=refresh_ports, font=("Segoe UI", 9), bg="#495057", fg=text_color, relief="groove")
    btn_refresh.grid(row=0, column=2, padx=5, pady=4)

    # OTA IP Setting
    tk.Label(port_card, text="WiFi OTA IP:", font=("Segoe UI", 10), fg=text_color, bg=card_bg).grid(row=1, column=0, sticky="w", pady=4)
    ip_var = tk.StringVar(value=DEFAULT_OTA_IP)
    ip_entry = tk.Entry(port_card, textvariable=ip_var, width=20, font=("Segoe UI", 10))
    ip_entry.grid(row=1, column=1, padx=10, pady=4, sticky="w")

    # Partition Summary Card
    part_card = tk.LabelFrame(container, text=" 📋 สถานะพาร์ติชัน (Partition Status: partitions_two_ota.csv) ", font=("Segoe UI", 10, "bold"), fg="#caf0f8", bg=card_bg, padx=15, pady=8)
    part_card.pack(fill="x", pady=6)

    part_ok, part_msg, _ = verify_partition_table(PARTITIONS_BIN)
    part_status_text = f"✅ โครงสร้างพาร์ติชันถูกต้อง: Bootloader (0x0000) | Partitions (0x8000) | Boot_App0 (0x29000) | App0 (0x30000)" if part_ok else f"❌ {part_msg}"
    tk.Label(part_card, text=part_status_text, font=("Segoe UI", 9), fg="#a7c957" if part_ok else "#e63946", bg=card_bg).pack(anchor="w")

    # Buttons Frame
    btn_frame = tk.Frame(container, bg=bg_color, pady=8)
    btn_frame.pack(fill="x")

    log_box = scrolledtext.ScrolledText(container, height=13, font=("Consolas", 9), bg="#11111b", fg="#cdd6f4", insertbackground="#ffffff")
    log_box.pack(fill="both", expand=True, pady=4)

    def append_log(msg):
        def _log():
            log_box.insert(tk.END, msg + "\n")
            log_box.see(tk.END)
        root.after(0, _log)

    def do_flash_usb():
        selected_text = port_var.get()
        if not selected_text or "ไม่พบ" in selected_text:
            messagebox.showwarning("แจ้งเตือน", "กรุณาเชื่อมต่อสาย USB หรือเลือกพอร์ต Serial ก่อนเริ่มแฟลช")
            return
        port = selected_text.split()[0]

        btn_flash.config(state="disabled")
        btn_ota.config(state="disabled")
        append_log("=" * 60)
        append_log(f"⚡ เริ่มต้นแฟลช ESP32-C3 บนพอร์ต {port}...")

        def thread_target():
            flash_usb(port, log_callback=append_log)
            root.after(0, lambda: btn_flash.config(state="normal"))
            root.after(0, lambda: btn_ota.config(state="normal"))

        threading.Thread(target=thread_target, daemon=True).start()

    def do_flash_ota():
        ip = ip_var.get().strip()
        if not ip:
            messagebox.showwarning("แจ้งเตือน", "กรุณากรอกหมายเลข IP ของ AutoDoor")
            return

        btn_flash.config(state="disabled")
        btn_ota.config(state="disabled")
        append_log("=" * 60)
        append_log(f"🌐 เริ่มต้นอัปเดตผ่าน WiFi OTA ไปยัง {ip}...")

        def thread_target():
            flash_ota(ip, log_callback=append_log)
            root.after(0, lambda: btn_flash.config(state="normal"))
            root.after(0, lambda: btn_ota.config(state="normal"))

        threading.Thread(target=thread_target, daemon=True).start()

    btn_flash = tk.Button(
        btn_frame,
        text="⚡ 1-Click Flash (USB Cable)",
        command=do_flash_usb,
        font=("Segoe UI", 11, "bold"),
        bg=btn_flash_bg,
        fg="#ffffff",
        padx=18,
        pady=8,
        cursor="hand2"
    )
    btn_flash.pack(side="left", padx=5)

    btn_ota = tk.Button(
        btn_frame,
        text="🌐 1-Click Update (WiFi OTA)",
        command=do_flash_ota,
        font=("Segoe UI", 11, "bold"),
        bg=btn_ota_bg,
        fg="#ffffff",
        padx=18,
        pady=8,
        cursor="hand2"
    )
    btn_ota.pack(side="left", padx=5)

    refresh_ports()
    append_log("✨ ยินดีต้อนรับสู่ AutoDoor 1-Click Flasher")
    append_log(f"📋 สถานะพาร์ติชัน: {part_msg}")
    append_log(f"💡 พอร์ตที่แนะนำ: {auto_detect_esp32_c3_port() or 'ไม่พบ'}")

    root.mainloop()

def main():
    parser = argparse.ArgumentParser(description="AutoDoor ESP32-C3 1-Click Flasher")
    parser.add_argument("--flash", action="store_true", help="1-Click flash via USB cable immediately")
    parser.add_argument("--ota", action="store_true", help="1-Click update via WiFi OTA")
    parser.add_argument("--gui", action="store_true", help="Launch Graphical User Interface (GUI)")
    parser.add_argument("--check", action="store_true", help="Check partition layout and files")
    parser.add_argument("--port", default="", help="Specify COM port directly (e.g. COM37)")
    parser.add_argument("--ip", default=DEFAULT_OTA_IP, help=f"Specify target IP for OTA (default: {DEFAULT_OTA_IP})")
    parser.add_argument("--baud", type=int, default=460800, help="Baud rate for flashing (default: 460800)")

    args = parser.parse_args()

    if args.check:
        print_partition_table_status()
        return

    if args.gui:
        run_gui()
        return

    if args.ota:
        flash_ota(args.ip)
        return

    if args.flash:
        port = args.port or auto_detect_esp32_c3_port()
        if not port:
            print("❌ ไม่พบพอร์ต ESP32-C3 กรุณาระบุพอร์ตด้วย --port เช่น --port COM37")
            sys.exit(1)
        ok = flash_usb(port, baud=args.baud)
        sys.exit(0 if ok else 1)

    # Default: Interactive CLI
    run_cli_interactive()

if __name__ == "__main__":
    main()
