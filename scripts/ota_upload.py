#!/usr/bin/env python3
"""AutoDoor-RF2 ESP32 Over-the-Air (OTA) Firmware Uploader.

Uploads firmware .bin to the AutoDoor ESP32 device via /api/ota with progress tracking,
health verification, and reboot partition check.
"""

import os
import sys
import time
import json
import argparse
import subprocess
import http.client
import urllib.request
import urllib.parse
import base64
from pathlib import Path

# Fix Windows console encoding for UTF-8 / Thai / emojis
if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
        sys.stderr.reconfigure(encoding="utf-8", errors="replace")
    except Exception:
        pass

ROOT_DIR = Path(__file__).resolve().parent.parent
DEFAULT_TARGET_IP = "192.168.1.11"
DEFAULT_RELEASE_BIN = ROOT_DIR / "firmware" / "firmware_autodoor_ota.bin"
FALLBACK_BUILD_BIN = ROOT_DIR / ".pio" / "build" / "esp32-c3-release" / "firmware.bin"

def format_bytes(num_bytes: float) -> str:
    """Format bytes to human readable string (KB, MB)."""
    for unit in ['B', 'KB', 'MB', 'GB']:
        if abs(num_bytes) < 1024.0:
            return f"{num_bytes:3.1f} {unit}"
        num_bytes /= 1024.0
    return f"{num_bytes:.1f} TB"

def build_progress_bar(current: int, total: int, width: int = 30) -> str:
    """Create a terminal progress bar string."""
    fraction = current / total if total > 0 else 0
    filled = int(width * fraction)
    bar = "=" * (filled - 1) + ">" if 0 < filled < width else "=" * filled
    bar = bar.ljust(width, " ")
    percent = fraction * 100
    return f"[{bar}] {percent:5.1f}%"

def get_auth_header(password: str) -> str:
    """Generate Basic auth header if password is provided."""
    if not password:
        return ""
    token = base64.b64encode(f":{password}".encode("utf-8")).decode("utf-8")
    return f"Basic {token}"

def query_json(url: str, timeout: float = 4.0, password: str = "") -> dict:
    """Query a JSON endpoint on the target device."""
    req = urllib.request.Request(url)
    if password:
        req.add_header("Authorization", get_auth_header(password))
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        content = resp.read().decode("utf-8")
        return json.loads(content)

def check_device(base_url: str, password: str = "") -> dict:
    """Query and return device status information."""
    status_url = f"{base_url}/api/status"
    try:
        return query_json(status_url, timeout=3.5, password=password)
    except Exception as e:
        # Fallback to /api/health if /api/status fails
        health_url = f"{base_url}/api/health"
        try:
            return query_json(health_url, timeout=3.5, password=password)
        except Exception:
            raise ConnectionError(f"Cannot connect to device at {base_url} ({e})")

def build_firmware(env_name: str = "esp32-c3-release") -> bool:
    """Compile firmware using PlatformIO."""
    print(f"\n⚙️  Building firmware with PlatformIO (env: {env_name})...")
    start = time.time()
    cmd = ["pio", "run", "-e", env_name]
    proc = subprocess.run(cmd, cwd=str(ROOT_DIR))
    elapsed = time.time() - start
    if proc.returncode != 0:
        print(f"\n❌ Build failed with exit code {proc.returncode} ({elapsed:.1f}s)")
        return False
    print(f"✅ Build finished successfully in {elapsed:.1f}s\n")
    return True

def find_firmware_binary(custom_path: str = None) -> Path:
    """Locate the firmware .bin file to upload."""
    if custom_path:
        p = Path(custom_path)
        if not p.is_absolute():
            p = ROOT_DIR / p
        if not p.exists():
            raise FileNotFoundError(f"Specified firmware file not found: {p}")
        return p

    if DEFAULT_RELEASE_BIN.exists():
        return DEFAULT_RELEASE_BIN
    if FALLBACK_BUILD_BIN.exists():
        return FALLBACK_BUILD_BIN

    raise FileNotFoundError(
        f"No firmware binary found. Looked in:\n"
        f"  - {DEFAULT_RELEASE_BIN}\n"
        f"  - {FALLBACK_BUILD_BIN}\n"
        f"Please run with --build or compile first with 'pio run -e esp32-c3-release'."
    )

def upload_ota(
    host: str,
    port: int,
    bin_path: Path,
    password: str = "",
    timeout: int = 90
) -> bool:
    """Upload firmware binary to ESP32 /api/ota using streaming HTTP multipart POST."""
    file_size = bin_path.stat().st_size
    boundary = f"----WebKitFormBoundary{int(time.time()*1000)}"

    try:
        import requests
        headers = {}
        if password:
            headers["Authorization"] = get_auth_header(password)
        with open(bin_path, "rb") as f:
            files = {"update": (bin_path.name, f, "application/octet-stream")}
            print(f"📦 Uploading {bin_path.name} ({format_bytes(file_size)})...")
            start_time = time.time()
            resp = requests.post(f"http://{host}:{port}/api/ota", files=files, headers=headers, timeout=timeout)
            elapsed = max(0.001, time.time() - start_time)
            speed = file_size / elapsed
            print(f"  {build_progress_bar(file_size, file_size)} {format_bytes(file_size)}/{format_bytes(file_size)} ({format_bytes(speed)}/s)")
            print("⏳ Transfer complete. Waiting for ESP32 flash write confirmation...")
            if resp.status_code == 200:
                msg = "Firmware updated successfully"
                try:
                    rj = resp.json()
                    if "message" in rj:
                        msg = rj["message"]
                except Exception:
                    pass
                print(f"✅ {msg}")
                return True
            else:
                print(f"❌ Upload failed (HTTP {resp.status_code}): {resp.text}")
                return False
    except Exception:
        pass

    # Fallback to streaming http.client
    header_parts = [
        f"--{boundary}\r\n",
        f'Content-Disposition: form-data; name="update"; filename="{bin_path.name}"\r\n',
        "Content-Type: application/octet-stream\r\n\r\n"
    ]
    header_bytes = "".join(header_parts).encode("utf-8")
    footer_bytes = f"\r\n--{boundary}--\r\n".encode("utf-8")
    total_length = len(header_bytes) + file_size + len(footer_bytes)

    print(f"🚀 Connecting to {host}:{port}...")
    conn = http.client.HTTPConnection(host, port, timeout=timeout)

    try:
        conn.putrequest("POST", "/api/ota")
        conn.putheader("Content-Type", f"multipart/form-data; boundary={boundary}")
        conn.putheader("Content-Length", str(total_length))
        conn.putheader("Connection", "close")
        if password:
            conn.putheader("Authorization", get_auth_header(password))
        conn.endheaders()

        # Send multipart leading boundary and header
        conn.send(header_bytes)

        # Stream file chunks (4 KB chunks with 2ms pacing to prevent lwIP buffer exhaustion)
        chunk_size = 4096
        uploaded = 0
        start_time = time.time()
        last_print = 0

        print(f"📦 Uploading {bin_path.name} ({format_bytes(file_size)})...")

        with open(bin_path, "rb") as f:
            while True:
                chunk = f.read(chunk_size)
                if not chunk:
                    break
                conn.send(chunk)
                uploaded += len(chunk)
                time.sleep(0.002)

                now = time.time()
                if now - last_print >= 0.15 or uploaded == file_size:
                    elapsed = max(0.001, now - start_time)
                    speed = uploaded / elapsed
                    bar = build_progress_bar(uploaded, file_size)
                    status_line = (
                        f"\r  {bar} "
                        f"{format_bytes(uploaded)}/{format_bytes(file_size)} "
                        f"({format_bytes(speed)}/s)"
                    )
                    sys.stdout.write(status_line)
                    sys.stdout.flush()
                    last_print = now

        # Send closing boundary
        conn.send(footer_bytes)
        print()  # newline after progress bar

        print("⏳ Transfer complete. Waiting for ESP32 flash write confirmation...")
        resp = conn.getresponse()
        resp_data = resp.read().decode("utf-8", errors="replace")

        if resp.status == 200:
            msg = "Firmware updated successfully"
            try:
                rj = json.loads(resp_data)
                if "message" in rj:
                    msg = rj["message"]
            except Exception:
                pass
            print(f"✅ ESP32 Response [{resp.status}]: {msg}")
            return True
        else:
            print(f"❌ Upload rejected with HTTP {resp.status}: {resp_data}")
            return False

    finally:
        conn.close()

def wait_for_reboot(
    base_url: str,
    prev_partition: str = "",
    password: str = "",
    max_wait_sec: int = 30
) -> bool:
    """Poll device health until it boots up and confirm partition switch."""
    print(f"\n🔄 Waiting for device reboot and health check (max {max_wait_sec}s)...")
    time.sleep(3.5)  # initial delay for ESP32 to drop connection and reboot

    start_wait = time.time()
    while time.time() - start_wait < max_wait_sec:
        sys.stdout.write(".")
        sys.stdout.flush()
        try:
            status = check_device(base_url, password=password)
            print("\n🎉 Device is BACK ONLINE!")

            fw_ver = status.get("fwVersion", "unknown")
            build_date = status.get("buildDate", "unknown")
            ota_info = status.get("ota", {})
            curr_partition = ota_info.get("runningPartition", "unknown")
            next_partition = ota_info.get("nextPartition", "unknown")

            print(f"   - Active Firmware: v{fw_ver} ({build_date})")
            print(f"   - Running Partition: {curr_partition} (Next standby: {next_partition})")

            if prev_partition and curr_partition != "unknown":
                if curr_partition != prev_partition:
                    print(f"   - Partition Switch: {prev_partition} ➔ {curr_partition} (SUCCESS)")
                else:
                    print(f"   - Note: Running partition is still {curr_partition}")

            return True
        except Exception:
            time.sleep(1.0)

    print(f"\n⚠️  Timeout waiting for device to respond after {max_wait_sec}s.")
    print("   Please verify power supply and check serial monitor or WiFi connection.")
def process_device(ip: str, port: int, bin_path: Path, args) -> tuple[bool, str]:
    """Inspect and flash a single device."""
    base_url = f"http://{ip}:{port}" if port != 80 else f"http://{ip}"

    print("=" * 60)
    print(f"🚪 AutoDoor-RF2 OTA Uploader ➔ {base_url}")
    print("=" * 60)

    print(f"🔍 Connecting to ESP32 at {base_url}...")
    try:
        status = check_device(base_url, password=args.password)
        fw_ver = status.get("fwVersion", "unknown")
        build_date = status.get("buildDate", "unknown")
        ota = status.get("ota", {})
        running_part = ota.get("runningPartition", "unknown")
        next_part = ota.get("nextPartition", "unknown")
        free_space = ota.get("freeSketchSpace", 0)
        sketch_size = ota.get("sketchSize", 0)
        door_state = status.get("doorState", "unknown")

        print(f"✅ Device Online:")
        print(f"   - Current Firmware : v{fw_ver} ({build_date})")
        print(f"   - Door State       : {door_state}")
        print(f"   - Running Partition: {running_part} (Standby: {next_part})")
        print(f"   - Current App Size : {format_bytes(sketch_size)}")
        print(f"   - Free Flash Space : {format_bytes(free_space)}")

        if not ota.get("otaValid", True):
            print("⚠️ Warning: otaValid reported false.")

        if args.check:
            return (True, f"ONLINE (v{fw_ver}, {running_part})")

        file_size = bin_path.stat().st_size
        if free_space > 0 and file_size > free_space:
            print(f"❌ Error: Firmware size exceeds free space ({free_space} B)!")
            return (False, "Free space insufficient")

        print("\n" + "-" * 60)
        upload_ok = upload_ota(
            host=ip,
            port=port,
            bin_path=bin_path,
            password=args.password,
            timeout=args.timeout
        )
        if not upload_ok:
            return (False, "Upload rejected")

        reboot_ok = wait_for_reboot(
            base_url=base_url,
            prev_partition=running_part,
            password=args.password,
            max_wait_sec=35
        )
        if reboot_ok:
            return (True, f"SUCCESS ({running_part} ➔ {next_part})")
        else:
            return (False, "Reboot timeout")

    except Exception as e:
        print(f"❌ Failed to reach device at {base_url}: {e}")
        return (False, f"Unreachable: {e}")

def main():
    parser = argparse.ArgumentParser(
        description="AutoDoor-RF2 ESP32 OTA Firmware Uploader",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""Examples:
  python scripts/ota_upload.py --both              # Check & upload to both 192.168.1.11 & 192.168.1.12
  python scripts/ota_upload.py --both --build      # Compile once, then upload to both
  python scripts/ota_upload.py                     # Upload to default 192.168.1.11
  python scripts/ota_upload.py --ip 192.168.1.12   # Upload to specific IP
"""
    )
    parser.add_argument("--ip", default=DEFAULT_TARGET_IP, help=f"Target device IP (default: {DEFAULT_TARGET_IP})")
    parser.add_argument("--both", action="store_true", help="Upload to both default devices (192.168.1.11 and 192.168.1.12)")
    parser.add_argument("--port", type=int, default=80, help="Target device HTTP port (default: 80)")
    parser.add_argument("--bin", help="Path to firmware binary .bin (auto-detected if omitted)")
    parser.add_argument("--build", action="store_true", help="Compile firmware using PlatformIO before uploading")
    parser.add_argument("--check", action="store_true", help="Query and display device status only")
    parser.add_argument("--password", default="", help="Web authentication password (if enabled)")
    parser.add_argument("--timeout", type=int, default=90, help="Upload timeout in seconds (default: 90)")

    args = parser.parse_args()

    target_ips = ["192.168.1.11", "192.168.1.12"] if args.both else [args.ip]

    # 1. Build if requested (build ONCE for all targets)
    if args.build and not args.check:
        if not build_firmware():
            sys.exit(1)

    # 2. Locate binary file
    bin_path = None
    if not args.check:
        try:
            bin_path = find_firmware_binary(args.bin)
            file_size = bin_path.stat().st_size
            print(f"\n📄 Target Firmware File: {bin_path}")
            print(f"   - File Size: {format_bytes(file_size)} ({file_size:,} bytes)\n")
        except Exception as e:
            print(f"❌ Firmware file error: {e}")
            sys.exit(1)

    # 3. Process each target device
    results = {}
    for idx, ip in enumerate(target_ips):
        if idx > 0:
            print("\n")
        ok, msg = process_device(ip, args.port, bin_path, args)
        results[ip] = (ok, msg)

    # 4. Summary table
    print("\n" + "=" * 60)
    print("📊 OTA SUMMARY:")
    for ip, (ok, msg) in results.items():
        icon = "✅" if ok else "❌"
        print(f"   {icon} {ip.ljust(15)} : {msg}")
    print("=" * 60)

    if not all(ok for ok, _ in results.values()):
        sys.exit(1)

if __name__ == "__main__":
    main()
