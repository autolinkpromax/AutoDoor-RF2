#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
AutoDoor ESP32-C3 1-Click Firmware Flasher
=========================================
Tool for 1-click firmware flashing with automatic port detection,
real-time progress bar, partition verification, and dual-mode upload (USB Serial / WiFi OTA).
"""

import os
import sys
import time
import json
import struct
import argparse
import subprocess
import threading
import re
import shutil
from datetime import datetime
from pathlib import Path

# Force UTF-8 encoding across Windows console and subprocesses
os.environ["PYTHONIOENCODING"] = "utf-8"
os.environ["PYTHONUTF8"] = "1"

if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
        sys.stderr.reconfigure(encoding="utf-8", errors="replace")
    except Exception:
        pass

DIR = Path(__file__).resolve().parent
PROJECT_ROOT = DIR.parent
BIN_DIR = DIR / "bin"

BOOTLOADER_BIN = BIN_DIR / "bootloader.bin"
PARTITIONS_BIN = BIN_DIR / "partitions.bin"
BOOT_APP0_BIN  = BIN_DIR / "boot_app0.bin"
FIRMWARE_BIN   = BIN_DIR / "firmware.bin"

PIO_BUILD_DIR  = PROJECT_ROOT / ".pio" / "build" / "esp32-c3-release"

DEFAULT_OTA_IP = "192.168.1.11"

# Partition Offsets according to partitions_two_ota.csv (ESP32-C3 4MB Flash)
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
    for i in range(0, len(data), 32):
        chunk = data[i:i+32]
        if len(chunk) < 32:
            break
        magic, p_type, p_subtype, p_offset, p_size = struct.unpack("<HBBII", chunk[:12])
        if magic != 0x50AA:
            continue
        label = chunk[12:28].split(b'\x00')[0].decode("ascii", errors="replace")
        entries.append({
            "name": label,
            "type": p_type,
            "subtype": p_subtype,
            "offset": p_offset,
            "size": p_size,
        })

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

    if FIRMWARE_BIN.exists():
        fw_size = FIRMWARE_BIN.stat().st_size
        max_app = 0x1D0000
        pct = (fw_size / max_app) * 100
        print(f"📦 ไฟล์เฟิร์มแวร์ firmware.bin : {format_size(fw_size)} / {format_size(max_app)} ({pct:.1f}% ของ app0)")
        if fw_size > max_app:
            print("⚠️ คำเตือน: ขนาดเฟิร์มแวร์ใหญ่เกินขนาด Partition app0!")
    else:
        print("❌ ไม่พบไฟล์ bin/firmware.bin!")
    print("=" * 70 + "\n")

def find_serial_ports() -> list[dict]:
    """Find available COM ports and highlight likely ESP32 devices."""
    ports = []
    try:
        import serial.tools.list_ports
        for p in serial.tools.list_ports.comports():
            dev = p.device or ""
            desc = p.description or ""
            hwid = getattr(p, "hwid", "") or ""
            is_bt = "Bluetooth" in desc or "BTHENUM" in hwid
            # Common ESP32 USB VIDs:
            # 303A: Espressif (ESP32-C3 USB JTAG / CDC)
            # 1A86: WCH CH340 / CH341
            # 10C4: Silicon Labs CP210x
            # 0403: FTDI
            is_esp_vid = any(vid in hwid.upper() for vid in ["VID:303A", "VID_303A", "VID:1A86", "VID_1A86", "VID:10C4", "VID_10C4", "VID:0403", "VID_0403"])
            is_esp_desc = any(k in desc.lower() for k in ["usb serial", "ch340", "cp210", "ftdi", "esp32", "jtag", "espressif"])
            is_esp = (is_esp_vid or is_esp_desc) and not is_bt

            ports.append({
                "port": dev,
                "desc": desc,
                "hwid": hwid,
                "is_bt": is_bt,
                "is_esp": is_esp,
            })
    except Exception:
        pass
    return ports

def auto_detect_esp32_c3_port() -> str:
    """Scan and auto-detect ESP32-C3 port."""
    ports = find_serial_ports()
    esp_candidates = [p for p in ports if p["is_esp"]]

    if len(esp_candidates) == 1:
        return esp_candidates[0]["port"]

    for p in esp_candidates:
        port_name = p["port"]
        try:
            cmd = [sys.executable, "-m", "esptool", "--port", port_name, "chip-id"]
            proc = subprocess.run(cmd, capture_output=True, text=True, timeout=3, env=get_clean_env())
            if "ESP32-C3" in proc.stdout:
                return port_name
        except Exception:
            pass

    if esp_candidates:
        return esp_candidates[0]["port"]

    non_bt = [p for p in ports if not p["is_bt"]]
    if len(non_bt) == 1 and non_bt[0]["port"] != "COM1":
        return non_bt[0]["port"]

    return ""

def get_clean_env() -> dict:
    """Get environment with forced UTF-8 for subprocesses."""
    env = os.environ.copy()
    env["PYTHONIOENCODING"] = "utf-8"
    env["PYTHONUTF8"] = "1"
    return env

def sync_from_pio_build() -> tuple[bool, str]:
    """Copy the newest firmware from PlatformIO build to 1-Click-Flash-ESP32C3/bin."""
    if not PIO_BUILD_DIR.exists():
        return False, "ไม่พบโฟลเดอร์ .pio/build/esp32-c3-release"

    pio_fw = PIO_BUILD_DIR / "firmware.bin"
    if not pio_fw.exists():
        return False, "ยังไม่มีการคอมไพล์เฟิร์มแวร์ใน .pio/build/esp32-c3-release"

    try:
        BIN_DIR.mkdir(parents=True, exist_ok=True)
        shutil.copy2(pio_fw, FIRMWARE_BIN)
        for extra in ["bootloader.bin", "partitions.bin"]:
            src = PIO_BUILD_DIR / extra
            dst = BIN_DIR / extra
            if src.exists():
                shutil.copy2(src, dst)
        return True, f"ซิงค์เฟิร์มแวร์ล่าสุดเรียบร้อย ({format_size(FIRMWARE_BIN.stat().st_size)})"
    except Exception as e:
        return False, f"เกิดข้อผิดพลาดในการซิงค์: {e}"

def read_chip_info(port: str, log_callback=print) -> bool:
    """Read chip info and MAC address from ESP32."""
    log_callback(f"🔍 กำลังอ่านข้อมูลชิปบนพอร์ต {port}...")
    try:
        cmd = [sys.executable, "-m", "esptool", "--chip", "esp32c3", "--port", port, "chip-id"]
        proc = subprocess.run(cmd, capture_output=True, text=True, env=get_clean_env(), timeout=10)
        output = proc.stdout + proc.stderr
        for line in output.splitlines():
            line_str = line.strip()
            if line_str and not line_str.startswith("esptool v"):
                log_callback("   " + line_str)
        return proc.returncode == 0
    except Exception as e:
        log_callback(f"❌ ไม่สามารถอ่านข้อมูลชิปได้: {e}")
        return False

def flash_usb(port: str, baud: int = 460800, erase_all: bool = False, log_callback=print, progress_callback=None) -> bool:
    """Flash bootloader, partitions, boot_app0, and firmware to ESP32-C3 with real-time progress."""
    for f in [BOOTLOADER_BIN, PARTITIONS_BIN, BOOT_APP0_BIN, FIRMWARE_BIN]:
        if not f.exists():
            log_callback(f"❌ Error: ไม่พบไฟล์จำเป็น {f.name} ใน {BIN_DIR}")
            return False

    log_callback("=" * 65)
    log_callback(f"🚀 เตรียมแฟลช ESP32-C3 บนพอร์ต {port} (Baud rate: {baud})...")
    log_callback("📌 พิกัด Flash Offsets (ตรงตาม partitions_two_ota.csv):")
    log_callback(f"   - 0x{OFFSETS['bootloader']:04X} ➔ bootloader.bin ({format_size(BOOTLOADER_BIN.stat().st_size)})")
    log_callback(f"   - 0x{OFFSETS['partitions']:04X} ➔ partitions.bin ({format_size(PARTITIONS_BIN.stat().st_size)})")
    log_callback(f"   - 0x{OFFSETS['otadata']:05X} ➔ boot_app0.bin   ({format_size(BOOT_APP0_BIN.stat().st_size)})")
    log_callback(f"   - 0x{OFFSETS['app0']:05X} ➔ firmware.bin     ({format_size(FIRMWARE_BIN.stat().st_size)})")
    if erase_all:
        log_callback("⚠️ โหมด Erase All Flash: กำลังล้างข้อมูล Flash ทั้งชิปก่อนเขียน")
    log_callback("=" * 65)

    cmd = [
        sys.executable, "-m", "esptool",
        "--chip", "esp32c3",
        "--port", port,
        "--baud", str(baud),
        "--before", "default-reset",
        "--after", "hard-reset",
        "write-flash", "-z",
        "--flash-mode", "dio",
        "--flash-freq", "80m",
        "--flash-size", "4MB",
    ]
    if erase_all:
        cmd.append("--erase-all")

    cmd.extend([
        f"0x{OFFSETS['bootloader']:X}", str(BOOTLOADER_BIN),
        f"0x{OFFSETS['partitions']:X}", str(PARTITIONS_BIN),
        f"0x{OFFSETS['otadata']:X}",    str(BOOT_APP0_BIN),
        f"0x{OFFSETS['app0']:X}",       str(FIRMWARE_BIN),
    ])

    log_callback("\n⚡ กำลังส่งข้อมูลไปยัง ESP32-C3 โปรดรอสักครู่...")
    start_time = time.time()
    last_pct = 0

    try:
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            bufsize=0,
            env=get_clean_env()
        )

        buffer = bytearray()
        pct_regex = re.compile(r'\((\d+)\s*%\)')

        while True:
            byte = proc.stdout.read(1)
            if not byte:
                if buffer:
                    line = buffer.decode("utf-8", errors="replace").strip()
                    if line:
                        log_callback("   " + line)
                break

            if byte in (b'\r', b'\n'):
                line = buffer.decode("utf-8", errors="replace").strip()
                buffer.clear()
                if line:
                    match = pct_regex.search(line)
                    if match:
                        pct = int(match.group(1))
                        if progress_callback:
                            progress_callback(pct, line)
                        if pct != last_pct and pct % 20 == 0:
                            log_callback(f"   ⏳ ความคืบหน้า: {pct}%")
                            last_pct = pct
                    else:
                        log_callback("   " + line)
            else:
                buffer.extend(byte)

        proc.wait()
        elapsed = time.time() - start_time

        if proc.returncode == 0:
            if progress_callback:
                progress_callback(100, "เสร็จสมบูรณ์ 100%")
            log_callback("\n" + "=" * 65)
            log_callback(f"🎉 แฟลชเฟิร์มแวร์สำเร็จเรียบร้อย 100%! (ใช้เวลา {elapsed:.1f} วินาที)")
            log_callback("💡 บอร์ด ESP32-C3 รีเซ็ตอัตโนมัติและพร้อมทำงานทันที")
            log_callback("=" * 65 + "\n")
            return True
        else:
            log_callback("\n" + "=" * 65)
            log_callback(f"❌ การแฟลชล้มเหลว (รหัสข้อผิดพลาด: {proc.returncode})")
            log_callback("💡 วิธีแก้ไข:")
            log_callback("   1. กดปุ่ม BOOT บนบอร์ดค้างไว้ แล้วกดปุ่ม RESET (EN) 1 ครั้ง เพื่อเข้า Download Mode")
            log_callback("   2. ตรวจสอบว่าสาย USB เป็นสายส่งข้อมูล ไม่ใช่สายชาร์จไฟอย่างเดียว")
            log_callback("   3. ลองลดความเร็วลงเหลือ 115200 bps หากสายยาว")
            log_callback("=" * 65 + "\n")
            return False

    except Exception as e:
        log_callback(f"❌ Exception during flash: {e}")
        return False

def flash_ota(ip: str = DEFAULT_OTA_IP, log_callback=print) -> bool:
    """Flash firmware via WiFi OTA to ESP32."""
    if not FIRMWARE_BIN.exists():
        log_callback(f"❌ Error: ไม่พบไฟล์ {FIRMWARE_BIN.name}!")
        return False

    ota_script = PROJECT_ROOT / "scripts" / "ota_upload.py"
    if ota_script.exists():
        cmd = [sys.executable, str(ota_script), "--ip", ip, "--bin", str(FIRMWARE_BIN)]
        log_callback(f"🌐 กำลังส่ง Firmware ผ่าน OTA ไปยัง {ip}...")
        proc = subprocess.run(cmd, env=get_clean_env())
        return proc.returncode == 0
    else:
        log_callback(f"❌ ไม่พบสคริปต์ ota_upload.py ที่ {ota_script}")
        return False

# ==============================================================================
# Modern GUI Mode (Tkinter with Dark Theme & Real-Time Auto Polling)
# ==============================================================================
def run_gui():
    """Launch modern Tkinter GUI."""
    try:
        import tkinter as tk
        from tkinter import ttk, messagebox, scrolledtext
    except ImportError:
        print("Tkinter is not available. Running in interactive CLI mode instead.")
        run_cli_interactive()
        return

    root = tk.Tk()
    root.title("⚡ AutoDoor ESP32-C3 - One Click Flasher")
    root.geometry("820x760")
    root.minsize(760, 680)

    # Styling Palette (Modern Dark Theme)
    BG_MAIN    = "#181825"
    BG_CARD    = "#1e1e2e"
    BG_CARD_ALT= "#252538"
    BORDER_COL = "#313244"
    TEXT_MAIN  = "#cdd6f4"
    TEXT_MUTED = "#9399b2"
    ACCENT_CYAN= "#00d2ff"
    ACCENT_BLUE= "#0077b6"
    ACCENT_GREEN= "#10b981"
    ACCENT_RED = "#ef4444"
    ACCENT_WARN= "#f59e0b"

    root.configure(bg=BG_MAIN)

    style = ttk.Style()
    style.theme_use("clam")

    # Configure TTK styles
    style.configure("TProgressbar", thickness=18, troughcolor="#2b2d42", background=ACCENT_CYAN)
    style.configure("TCombobox", fieldbackground="#11111b", background="#313244", foreground="#ffffff", selectbackground=ACCENT_BLUE, selectforeground="#ffffff")
    style.map("TCombobox", fieldbackground=[("readonly", "#11111b")], foreground=[("readonly", "#ffffff")])

    # Header Frame
    header = tk.Frame(root, bg="#11111b", pady=12, padx=20, highlightbackground=BORDER_COL, highlightthickness=1)
    header.pack(fill="x", side="top")

    title_frame = tk.Frame(header, bg="#11111b")
    title_frame.pack(side="left", fill="both", expand=True)

    tk.Label(
        title_frame,
        text="⚡ AutoDoor ESP32-C3 One-Click Flasher",
        font=("Segoe UI", 16, "bold"),
        fg=ACCENT_CYAN,
        bg="#11111b"
    ).pack(anchor="w")

    tk.Label(
        title_frame,
        text="โปรแกรมแฟลชเฟิร์มแวร์ผ่าน USB สำหรับบอร์ด AutoDoor (Offset 0x30000 | 2x OTA + NVS 128KB)",
        font=("Segoe UI", 9),
        fg=TEXT_MUTED,
        bg="#11111b"
    ).pack(anchor="w")

    status_pill = tk.Label(
        header,
        text="🔴 ยังไม่พบพอร์ต USB",
        font=("Segoe UI", 9, "bold"),
        fg=ACCENT_RED,
        bg="#252538",
        padx=12,
        pady=5,
        relief="groove"
    )
    status_pill.pack(side="right", padx=5)

    # Main Scrollable / Stack Container
    container = tk.Frame(root, bg=BG_MAIN, padx=18, pady=10)
    container.pack(fill="both", expand=True)

    # --------------------------------------------------------------------------
    # Card 1: Connection Setup
    # --------------------------------------------------------------------------
    card_conn = tk.LabelFrame(
        container,
        text=" 🔌 การเชื่อมต่อพอร์ต Serial (USB COM Port) ",
        font=("Segoe UI", 10, "bold"),
        fg="#89dceb",
        bg=BG_CARD,
        padx=14,
        pady=10,
        highlightbackground=BORDER_COL
    )
    card_conn.pack(fill="x", pady=5)

    row1 = tk.Frame(card_conn, bg=BG_CARD)
    row1.pack(fill="x", pady=3)

    tk.Label(row1, text="COM Port:", font=("Segoe UI", 10, "bold"), fg=TEXT_MAIN, bg=BG_CARD, width=10, anchor="w").pack(side="left")

    port_var = tk.StringVar()
    port_combo = ttk.Combobox(row1, textvariable=port_var, width=42, state="readonly", font=("Segoe UI", 10))
    port_combo.pack(side="left", padx=8)

    known_ports = []
    is_scanning = False

    def scan_ports(auto_select=True):
        nonlocal known_ports
        ports = find_serial_ports()
        non_bt = [p for p in ports if not p["is_bt"]]
        detected = auto_detect_esp32_c3_port()

        port_list = []
        default_idx = 0
        current_selection = port_var.get().split()[0] if port_var.get() else ""

        for idx, p in enumerate(non_bt):
            tag = " 🌟 [ESP32-C3 แนะนำ]" if p["port"] == detected else ""
            label = f"{p['port']} : {p['desc']}{tag}"
            port_list.append(label)
            if p["port"] == current_selection:
                default_idx = idx
            elif not current_selection and p["port"] == detected:
                default_idx = idx

        port_combo["values"] = port_list
        known_ports = [p["port"] for p in non_bt]

        if port_list:
            if auto_select or not port_var.get() or "ไม่พบ" in port_var.get():
                port_combo.current(default_idx)
                port_var.set(port_list[default_idx])
            selected_port = port_var.get().split()[0]
            status_pill.config(text=f"🟢 เชื่อมต่อแล้ว ({selected_port})", fg=ACCENT_GREEN)
            btn_flash.config(state="normal")
            btn_test.config(state="normal")
        else:
            port_combo.set("ไม่พบพอร์ต Serial (กรุณาเสียบสาย USB)")
            status_pill.config(text="🔴 รอเสียบสาย USB", fg=ACCENT_RED)
            btn_flash.config(state="disabled")
            btn_test.config(state="disabled")

    def auto_poll_ports():
        """Periodically check for newly attached/detached USB devices every 1.5s."""
        try:
            ports = find_serial_ports()
            non_bt = [p["port"] for p in ports if not p["is_bt"]]
            if sorted(non_bt) != sorted(known_ports):
                scan_ports(auto_select=True)
        except Exception:
            pass
        root.after(1500, auto_poll_ports)

    btn_refresh = tk.Button(
        row1,
        text="🔄 สแกนพอร์ต",
        command=lambda: scan_ports(auto_select=True),
        font=("Segoe UI", 9),
        bg="#313244",
        fg=TEXT_MAIN,
        padx=10,
        pady=2,
        relief="groove",
        cursor="hand2"
    )
    btn_refresh.pack(side="left", padx=4)

    def do_open_dev_mgmt():
        try:
            subprocess.Popen("devmgmt.msc", shell=True)
        except Exception as e:
            messagebox.showerror("Error", f"ไม่สามารถเปิด Device Manager ได้: {e}")

    btn_devmgr = tk.Button(
        row1,
        text="🛠️ Device Manager",
        command=do_open_dev_mgmt,
        font=("Segoe UI", 9),
        bg="#313244",
        fg=TEXT_MAIN,
        padx=10,
        pady=2,
        relief="groove",
        cursor="hand2"
    )
    btn_devmgr.pack(side="left", padx=4)

    row2 = tk.Frame(card_conn, bg=BG_CARD)
    row2.pack(fill="x", pady=4)

    tk.Label(row2, text="ความเร็ว (Baud):", font=("Segoe UI", 9), fg=TEXT_MUTED, bg=BG_CARD, width=12, anchor="w").pack(side="left")

    baud_var = tk.StringVar(value="460800")
    baud_combo = ttk.Combobox(
        row2,
        textvariable=baud_var,
        values=["460800", "921600", "115200", "230400"],
        width=14,
        state="readonly",
        font=("Segoe UI", 9)
    )
    baud_combo.pack(side="left", padx=8)

    tk.Label(row2, text="(460800 แนะนำเร็วและเสถียร | 115200 สำหรับสายยาว)", font=("Segoe UI", 8), fg=TEXT_MUTED, bg=BG_CARD).pack(side="left", padx=5)

    def do_test_chip():
        selected = port_var.get()
        if not selected or "ไม่พบ" in selected:
            messagebox.showwarning("แจ้งเตือน", "กรุณาเชื่อมต่อสาย USB ก่อนทดสอบ")
            return
        port = selected.split()[0]
        append_log("=" * 60)
        append_log(f"🔍 ทดสอบเชื่อมต่อและอ่านข้อมูลชิปบน {port}...")

        def _thread():
            btn_test.config(state="disabled")
            read_chip_info(port, log_callback=append_log)
            root.after(0, lambda: btn_test.config(state="normal"))

        threading.Thread(target=_thread, daemon=True).start()

    btn_test = tk.Button(
        row2,
        text="🔍 เช็คชิป ESP32",
        command=do_test_chip,
        font=("Segoe UI", 9),
        bg="#313244",
        fg="#a6e3a1",
        padx=8,
        pady=2,
        relief="groove",
        cursor="hand2"
    )
    btn_test.pack(side="right", padx=5)

    # --------------------------------------------------------------------------
    # Card 2: Firmware & Partition Status
    # --------------------------------------------------------------------------
    card_fw = tk.LabelFrame(
        container,
        text=" 📦 เฟิร์มแวร์และโครงสร้างพาร์ติชัน (Partitions & Firmware Files) ",
        font=("Segoe UI", 10, "bold"),
        fg="#89dceb",
        bg=BG_CARD,
        padx=14,
        pady=8,
        highlightbackground=BORDER_COL
    )
    card_fw.pack(fill="x", pady=5)

    part_ok, part_msg, _ = verify_partition_table(PARTITIONS_BIN)
    fw_size = FIRMWARE_BIN.stat().st_size if FIRMWARE_BIN.exists() else 0
    fw_mtime = datetime.fromtimestamp(FIRMWARE_BIN.stat().st_mtime).strftime("%Y-%m-%d %H:%M") if FIRMWARE_BIN.exists() else "N/A"

    fw_info_label = tk.Label(
        card_fw,
        text=f"✅ พิกัดครบ 4 ไฟล์: Bootloader (0x0000) | Partitions (0x8000) | Boot_App0 (0x29000) | Firmware (0x30000)",
        font=("Segoe UI", 9),
        fg=ACCENT_GREEN if part_ok else ACCENT_RED,
        bg=BG_CARD
    )
    fw_info_label.pack(anchor="w")

    fw_detail_label = tk.Label(
        card_fw,
        text=f"📦 firmware.bin: {format_size(fw_size)} | อัปเดตล่าสุด: {fw_mtime} (พาร์ติชัน 2x OTA รองรับ NVS 128KB)",
        font=("Segoe UI", 8),
        fg=TEXT_MUTED,
        bg=BG_CARD
    )
    fw_detail_label.pack(anchor="w")

    row_fw_act = tk.Frame(card_fw, bg=BG_CARD)
    row_fw_act.pack(fill="x", pady=4)

    def do_sync_fw():
        ok, msg = sync_from_pio_build()
        if ok:
            fw_sz = FIRMWARE_BIN.stat().st_size if FIRMWARE_BIN.exists() else 0
            fw_mt = datetime.fromtimestamp(FIRMWARE_BIN.stat().st_mtime).strftime("%Y-%m-%d %H:%M") if FIRMWARE_BIN.exists() else "N/A"
            fw_detail_label.config(text=f"📦 firmware.bin: {format_size(fw_sz)} | อัปเดตล่าสุด: {fw_mt} (พาร์ติชัน 2x OTA รองรับ NVS 128KB)")
            append_log(f"✅ {msg}")
            messagebox.showinfo("สำเร็จ", msg)
        else:
            append_log(f"⚠️ {msg}")
            messagebox.showwarning("แจ้งเตือน", msg)

    btn_sync = tk.Button(
        row_fw_act,
        text="🔄 ซิงค์เฟิร์มแวร์ล่าสุดจาก PlatformIO Build",
        command=do_sync_fw,
        font=("Segoe UI", 8),
        bg="#313244",
        fg="#cdd6f4",
        padx=6,
        pady=1,
        relief="groove",
        cursor="hand2"
    )
    btn_sync.pack(side="left")

    erase_var = tk.BooleanVar(value=False)
    chk_erase = tk.Checkbutton(
        row_fw_act,
        text="ล้างข้อมูล Flash ทั้งหมดก่อนอัพโหลด (Erase Flash)",
        variable=erase_var,
        font=("Segoe UI", 8),
        fg="#f38ba8",
        bg=BG_CARD,
        selectcolor="#11111b",
        activebackground=BG_CARD
    )
    chk_erase.pack(side="right")

    # --------------------------------------------------------------------------
    # Card 3: Action & Progress
    # --------------------------------------------------------------------------
    card_act = tk.Frame(container, bg=BG_MAIN, pady=6)
    card_act.pack(fill="x")

    progress_var = tk.DoubleVar(value=0)
    progress_bar = ttk.Progressbar(card_act, variable=progress_var, maximum=100, style="TProgressbar")
    progress_bar.pack(fill="x", pady=4)

    prog_text_frame = tk.Frame(card_act, bg=BG_MAIN)
    prog_text_frame.pack(fill="x")

    lbl_prog_status = tk.Label(
        prog_text_frame,
        text="พร้อมทำงาน - กรุณาเลือกพอร์ต COM แล้วกดปุ่ม One Click Flash ด้านล่าง",
        font=("Segoe UI", 9),
        fg=TEXT_MUTED,
        bg=BG_MAIN
    )
    lbl_prog_status.pack(side="left")

    lbl_pct = tk.Label(
        prog_text_frame,
        text="0%",
        font=("Segoe UI", 9, "bold"),
        fg=ACCENT_CYAN,
        bg=BG_MAIN
    )
    lbl_pct.pack(side="right")

    # ONE CLICK FLASH BUTTON
    btn_flash = tk.Button(
        card_act,
        text="⚡ ONE CLICK FLASH (อัพโหลดลงบอร์ด)",
        font=("Segoe UI", 12, "bold"),
        bg="#00b4d8",
        activebackground="#0077b6",
        fg="#000000",
        activeforeground="#ffffff",
        pady=10,
        relief="flat",
        cursor="hand2"
    )
    btn_flash.pack(fill="x", pady=6)

    # --------------------------------------------------------------------------
    # Card 4: Console Log Output
    # --------------------------------------------------------------------------
    card_log = tk.LabelFrame(
        container,
        text=" 💻 บันทึกการทำงานสด (Live Console Log) ",
        font=("Segoe UI", 9, "bold"),
        fg="#89dceb",
        bg=BG_CARD,
        padx=10,
        pady=6,
        highlightbackground=BORDER_COL
    )
    card_log.pack(fill="both", expand=True, pady=4)

    log_box = scrolledtext.ScrolledText(
        card_log,
        height=10,
        font=("Consolas", 9),
        bg="#11111b",
        fg="#cdd6f4",
        insertbackground="#ffffff",
        relief="flat"
    )
    log_box.pack(fill="both", expand=True, pady=4)

    def append_log(msg: str):
        def _log():
            log_box.insert(tk.END, msg + "\n")
            log_box.see(tk.END)
        root.after(0, _log)

    def update_progress(pct: int, status_line: str):
        def _prog():
            progress_var.set(pct)
            lbl_pct.config(text=f"{pct}%")
            if pct >= 100:
                lbl_prog_status.config(text="🎉 สำเร็จเรียบร้อย 100%!", fg=ACCENT_GREEN)
            else:
                lbl_prog_status.config(text=f"กำลังอัพโหลดข้อมูล... ({pct}%)", fg=ACCENT_CYAN)
        root.after(0, _prog)

    log_ctrl_frame = tk.Frame(card_log, bg=BG_CARD)
    log_ctrl_frame.pack(fill="x")

    def do_clear_log():
        log_box.delete("1.0", tk.END)

    tk.Button(
        log_ctrl_frame,
        text="🧹 ล้าง Log",
        command=do_clear_log,
        font=("Segoe UI", 8),
        bg="#313244",
        fg=TEXT_MUTED,
        relief="groove",
        cursor="hand2"
    ).pack(side="right")

    tk.Label(
        log_ctrl_frame,
        text="💡 หากค้างที่ Connecting... ให้กดปุ่ม BOOT ค้างไว้ แล้วกดปุ่ม RESET (EN) 1 ครั้ง",
        font=("Segoe UI", 8),
        fg="#f9e2af",
        bg=BG_CARD
    ).pack(side="left")

    # --------------------------------------------------------------------------
    # Flash Action Implementation
    # --------------------------------------------------------------------------
    def on_click_flash():
        selected = port_var.get()
        if not selected or "ไม่พบ" in selected:
            messagebox.showwarning("แจ้งเตือน", "กรุณาเชื่อมต่อสาย USB หรือเลือกพอร์ต Serial ก่อนเริ่มแฟลช")
            return

        port = selected.split()[0]
        try:
            baud = int(baud_var.get())
        except ValueError:
            baud = 460800

        erase = erase_var.get()
        if erase:
            if not messagebox.askyesno("ยืนยันการล้าง Flash", "คำเตือน: การล้างข้อมูล Flash ทั้งหมดจะทำให้ข้อมูล Tag และการตั้งค่า WiFi สูงสุดหายไป ต้องการดำเนินการต่อหรือไม่?"):
                return

        # Disable buttons
        btn_flash.config(state="disabled", text="⏳ กำลังแฟลชเฟิร์มแวร์ โปรดรอสักครู่...", bg="#585b70")
        btn_test.config(state="disabled")
        btn_refresh.config(state="disabled")
        progress_var.set(0)
        lbl_pct.config(text="0%")
        lbl_prog_status.config(text=f"กำลังเชื่อมต่อกับ ESP32-C3 บน {port}...", fg=ACCENT_CYAN)

        append_log("=" * 60)
        append_log(f"⚡ เริ่มต้นกระบวนการ One Click Flash บนพอร์ต {port}...")

        def _worker():
            success = flash_usb(
                port=port,
                baud=baud,
                erase_all=erase,
                log_callback=append_log,
                progress_callback=update_progress
            )

            def _restore():
                btn_flash.config(state="normal", text="⚡ ONE CLICK FLASH (อัพโหลดลงบอร์ด)", bg="#00b4d8")
                btn_test.config(state="normal")
                btn_refresh.config(state="normal")
                if success:
                    lbl_prog_status.config(text="🎉 แฟลชสำเร็จเรียบร้อย! บอร์ดพร้อมทำงาน", fg=ACCENT_GREEN)
                    messagebox.showinfo("สำเร็จ", "🎉 แฟลชเฟิร์มแวร์ลงบอร์ด ESP32-C3 สำเร็จเรียบร้อย 100%!\n\nบอร์ดรีเซ็ตและพร้อมทำงานทันที")
                else:
                    lbl_prog_status.config(text="❌ การแฟลชล้มเหลว กรุณาตรวจสอบการเชื่อมต่อ", fg=ACCENT_RED)
                    messagebox.showerror("เกิดข้อผิดพลาด", "❌ การแฟลชล้มเหลว!\n\nคำแนะนำ:\n1. ตรวจสอบว่าสาย USB เชื่อมต่อแน่นหนา\n2. กดปุ่ม BOOT ค้างไว้แล้วกด RESET เพื่อเข้า Bootloader Mode\n3. ลองสแกนพอร์ตใหม่อีกครั้ง")

            root.after(0, _restore)

        threading.Thread(target=_worker, daemon=True).start()

    btn_flash.config(command=on_click_flash)

    # Initial scan & start periodic polling
    scan_ports(auto_select=True)
    append_log("✨ ยินดีต้อนรับสู่ AutoDoor ESP32-C3 One-Click Flasher")
    append_log(f"📋 สถานะพาร์ติชัน: {part_msg}")
    append_log(f"📦 ขนาดเฟิร์มแวร์: {format_size(fw_size)}")
    append_log("🔌 เสียบสาย USB เข้ากับคอมพิวเตอร์ ระบบจะตรวจหาพอร์ตอัตโนมัติ")

    auto_poll_ports()
    root.mainloop()

# ==============================================================================
# Interactive CLI Mode
# ==============================================================================
def run_cli_interactive():
    """CLI Menu for 1-Click flasher."""
    print("=================================================================")
    print("🚪 AutoDoor-RF2 : ESP32-C3 1-Click Firmware Flasher")
    print("=================================================================")

    print_partition_table_status()

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
    print(" [4] 🔄 ซิงค์เฟิร์มแวร์ล่าสุดจาก PlatformIO Build")
    print(" [5] 🖥️  เปิดหน้าต่างโปรแกรมแบบกราฟิก (GUI Mode)")
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
                port = input("กรุณาระบุพอร์ตด้วยตนเอง (เช่น COM15): ").strip().upper()

        if not port:
            print("❌ ไม่ได้เลือกพอร์ต ยกเลิกการทำงาน")
            return
        flash_usb(port)

    elif choice == "2":
        ip = input(f"ระบุ IP ของ AutoDoor [{DEFAULT_OTA_IP}]: ").strip() or DEFAULT_OTA_IP
        flash_ota(ip)

    elif choice == "3":
        port = detected_port or (non_bt[0]["port"] if non_bt else "COM15")
        read_chip_info(port)

    elif choice == "4":
        ok, msg = sync_from_pio_build()
        print(f"\n{msg}\n")

    elif choice == "5":
        run_gui()

# ==============================================================================
# Main Entry Point
# ==============================================================================
def main():
    parser = argparse.ArgumentParser(description="AutoDoor ESP32-C3 1-Click Flasher")
    parser.add_argument("--flash", action="store_true", help="1-Click flash via USB cable immediately")
    parser.add_argument("--ota", action="store_true", help="1-Click update via WiFi OTA")
    parser.add_argument("--gui", action="store_true", help="Launch Graphical User Interface (GUI)")
    parser.add_argument("--check", action="store_true", help="Check partition layout and files")
    parser.add_argument("--sync", action="store_true", help="Sync latest build from PlatformIO")
    parser.add_argument("--port", default="", help="Specify COM port directly (e.g. COM15)")
    parser.add_argument("--ip", default=DEFAULT_OTA_IP, help=f"Specify target IP for OTA (default: {DEFAULT_OTA_IP})")
    parser.add_argument("--baud", type=int, default=460800, help="Baud rate for flashing (default: 460800)")
    parser.add_argument("--erase", action="store_true", help="Erase all flash memory before flashing")

    args = parser.parse_args()

    if args.check:
        print_partition_table_status()
        return

    if args.sync:
        ok, msg = sync_from_pio_build()
        print(msg)
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
            print("❌ ไม่พบพอร์ต ESP32-C3 กรุณาระบุพอร์ตด้วย --port เช่น --port COM15")
            sys.exit(1)
        ok = flash_usb(port, baud=args.baud, erase_all=args.erase)
        sys.exit(0 if ok else 1)

    # If launched without arguments, default to GUI if display available, otherwise CLI
    try:
        run_gui()
    except Exception:
        run_cli_interactive()

if __name__ == "__main__":
    main()
