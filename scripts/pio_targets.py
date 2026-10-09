"""PlatformIO custom targets for Web UI and Firmware distribution."""
Import("env")
import os
import shutil
import json
from pathlib import Path
from datetime import datetime, timezone

root = Path(env["PROJECT_DIR"])
py = env.subst("$PYTHONEXE")
pack = root / "scripts" / "pack_webui.py"

env.AddCustomTarget(
    name="pack-webui",
    dependencies=None,
    actions=[f'"{py}" "{pack}"'],
    title="Pack WebUI",
    description="Bundle webui/js/src to app-bundle.js + src/webui_assets.h",
)

env.AddCustomTarget(
    name="bundle-webui",
    dependencies=None,
    actions=[f'"{py}" "{pack}" --bundle-only'],
    title="Bundle WebUI JS",
    description="Concat webui/js/src/*.js to webui/js/app-bundle.js only",
)

ota = root / "scripts" / "ota_upload.py"
env.AddCustomTarget(
    name="ota",
    dependencies=None,
    actions=[f'"{py}" "{ota}"'],
    title="Upload OTA",
    description="Upload firmware via OTA to ESP32 at 192.168.1.11",
)

# Fix boot_app0 offset for 128KB NVS layout (otadata at 0x29000 instead of default 0xe000)
try:
    flash_extra = env.get("FLASH_EXTRA_IMAGES", [])
    new_extra = []
    for offset, img in flash_extra:
        if "boot_app0" in str(img) and str(offset) != "0x29000":
            new_extra.append(("0x29000", img))
        else:
            new_extra.append((offset, img))
    env["FLASH_EXTRA_IMAGES"] = new_extra
except Exception as e:
    pass


def copy_release_firmware(source, target, env):
    try:
        bin_path = Path(target[0].get_abspath())
        if not bin_path.exists():
            return

        fw_dir = root / "firmware"
        fw_dir.mkdir(exist_ok=True)
        dist_dir = root / "dist"
        dist_dir.mkdir(exist_ok=True)

        # 1. Copy to firmware/
        shutil.copy2(bin_path, fw_dir / "firmware_autodoor_ota.bin")

        # 2. Copy to dist/
        shutil.copy2(bin_path, dist_dir / "firmware_autodoor_ota.bin")
        shutil.copy2(bin_path, dist_dir / "firmware_esp32_c3_release.bin")

        # 3. Copy to 1-Click-Flash-ESP32C3/bin/
        click_bin_dir = root / "1-Click-Flash-ESP32C3" / "bin"
        if click_bin_dir.exists():
            shutil.copy2(bin_path, click_bin_dir / "firmware.bin")
            build_dir = bin_path.parent
            for extra in ["bootloader.bin", "partitions.bin"]:
                src_extra = build_dir / extra
                if src_extra.exists():
                    shutil.copy2(src_extra, click_bin_dir / extra)
                    shutil.copy2(src_extra, dist_dir / extra)

        # 4. Update firmware/version.json if needed
        version_json = fw_dir / "version.json"
        meta = {
            "version": "1.0.0",
            "releaseDate": datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC"),
            "targetChip": "ESP32-C3",
            "file": "firmware_autodoor_ota.bin",
            "changelog": "เพิ่มฟังก์ชันอัปเดตเฟิร์มแวร์ออนไลน์ (Online OTA Update) จาก GitHub โดยตรงผ่านหน้าตั้งค่า และปรับปรุงความเสถียรของระบบ"
        }
        if version_json.exists():
            try:
                curr = json.loads(version_json.read_text(encoding="utf-8"))
                if "version" in curr:
                    meta["version"] = curr["version"]
                if "changelog" in curr:
                    meta["changelog"] = curr["changelog"]
            except Exception:
                pass
        version_json.write_text(json.dumps(meta, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        print(f"[pio_targets] Exported release firmware to {fw_dir / 'firmware_autodoor_ota.bin'} and {dist_dir}")
    except Exception as e:
        print(f"[pio_targets] Warning: Failed to export firmware: {e}")

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_release_firmware)
