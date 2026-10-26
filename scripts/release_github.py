#!/usr/bin/env python3
"""
CLI Tool: Release firmware .bin & metadata to GitHub (origin/main)
Enables other devices to detect and update firmware via Web UI.
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CONFIG_H = ROOT / "src" / "app" / "config.h"
FW_DIR = ROOT / "firmware"
FW_BIN = FW_DIR / "firmware_autodoor_ota.bin"
VERSION_JSON = FW_DIR / "version.json"
REPO_URL = "https://autolinkpromax@github.com/autolinkpromax/AutoDoor-RF2.git"

# Ensure UTF-8 output on Windows consoles
try:
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
except Exception:
    pass


def run_cmd(cmd: list[str] | str, check: bool = True, shell: bool = False) -> str:
    res = subprocess.run(cmd, cwd=str(ROOT), capture_output=True, text=True, shell=shell)
    if check and res.returncode != 0:
        err = res.stderr.strip() or res.stdout.strip()
        raise RuntimeError(f"Command failed ({res.returncode}): {cmd}\n{err}")
    return res.stdout.strip()


def get_current_config_version() -> str:
    if not CONFIG_H.exists():
        return "1.0.0"
    content = CONFIG_H.read_text(encoding="utf-8")
    m = re.search(r'kFirmwareVersion\s*=\s*"([^"]+)"', content)
    return m.group(1) if m else "1.0.0"


def set_config_version(ver: str) -> None:
    if not CONFIG_H.exists():
        return
    content = CONFIG_H.read_text(encoding="utf-8")
    new_content = re.sub(r'kFirmwareVersion\s*=\s*"[^"]+"', f'kFirmwareVersion = "{ver}"', content)
    CONFIG_H.write_text(new_content, encoding="utf-8")


def bump_patch_version(ver: str) -> str:
    parts = ver.split(".")
    if len(parts) >= 3 and parts[2].isdigit():
        parts[2] = str(int(parts[2]) + 1)
        return ".".join(parts)
    return f"{ver}.1"


def ensure_git_remote() -> None:
    remotes = run_cmd(["git", "remote", "-v"], check=False)
    if "origin" not in remotes:
        run_cmd(["git", "remote", "add", "origin", REPO_URL])
        print(f"🔗 Added git remote origin: {REPO_URL}")
    elif "autolinkpromax@" not in remotes:
        run_cmd(["git", "remote", "set-url", "origin", REPO_URL])


def build_firmware() -> None:
    print("📦 Step 1: Packing WebUI assets...")
    pack_res = run_cmd([sys.executable, "scripts/pack_webui.py"])
    print(f"   {pack_res}")

    print("⚙️  Step 2: Building firmware with PlatformIO (esp32-c3-release)...")
    build_res = run_cmd(["pio", "run", "-e", "esp32-c3-release"])
    if "SUCCESS" in build_res:
        print("   ✅ PlatformIO build succeeded!")
    else:
        print("   ⚠️ Build finished.")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Release firmware .bin and version.json to GitHub for online OTA Web UI updates"
    )
    parser.add_argument(
        "--version", "-v",
        help="Target version to release (e.g. 1.0.3). Defaults to current or bumped version."
    )
    parser.add_argument(
        "--bump", "-u",
        action="store_true",
        help="Automatically bump patch version (e.g. 1.0.2 -> 1.0.3)"
    )
    parser.add_argument(
        "--changelog", "-m",
        help="Changelog text to display in Web UI update banner."
    )
    parser.add_argument(
        "--build", "-b",
        action="store_true",
        help="Rebuild release firmware with PlatformIO before uploading."
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Simulate tree and commit generation without pushing to GitHub."
    )

    args = parser.parse_args()

    ensure_git_remote()

    current_ver = get_current_config_version()
    if args.version:
        target_ver = args.version
    elif args.bump:
        target_ver = bump_patch_version(current_ver)
    else:
        target_ver = current_ver

    existing_changelog = ""
    if VERSION_JSON.exists():
        try:
            curr_data = json.loads(VERSION_JSON.read_text(encoding="utf-8"))
            existing_changelog = curr_data.get("changelog", "")
        except Exception:
            pass

    changelog = args.changelog or existing_changelog or f"V{target_ver}: ปรับปรุงประสิทธิภาพและความเสถียรของระบบ"

    print(f"🚀 AutoDoor-RF2 GitHub Firmware Releaser")
    print(f"   - Target Version : v{target_ver} (current: v{current_ver})")
    print(f"   - Changelog      : {changelog}")

    # Update version in config.h if changed
    if target_ver != current_ver:
        set_config_version(target_ver)
        print(f"   - Updated config.h to v{target_ver}")

    # Build firmware if requested or binary missing
    if args.build or not FW_BIN.exists():
        build_firmware()

    if not FW_BIN.exists():
        print(f"❌ Error: Firmware binary not found at {FW_BIN}", file=sys.stderr)
        sys.exit(1)

    bin_size = FW_BIN.stat().st_size
    print(f"📄 Firmware Binary: {FW_BIN.name} ({bin_size:,} bytes / {bin_size / 1024 / 1024:.2f} MB)")

    # Update version.json
    now_utc = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")
    v_data = {
        "version": target_ver,
        "releaseDate": now_utc,
        "targetChip": "ESP32-C3",
        "file": "firmware_autodoor_ota.bin",
        "changelog": changelog
    }
    VERSION_JSON.write_text(json.dumps(v_data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"📝 Updated {VERSION_JSON.name}")

    # Fetch latest origin/main
    print("🔄 Fetching latest origin/main from GitHub...")
    run_cmd(["git", "fetch", "origin", "main"])
    parent_commit = run_cmd(["git", "rev-parse", "origin/main"])
    print(f"   - Parent commit: {parent_commit[:7]}")

    # Hash git objects
    bin_blob = run_cmd(["git", "hash-object", "-w", str(FW_BIN)])
    json_blob = run_cmd(["git", "hash-object", "-w", str(VERSION_JSON)])

    # Get README.md blob from parent commit
    readme_blob = "0e6063d2c6b4cf4e9695089d540451031610f5fd"
    try:
        ls_out = run_cmd(["git", "ls-tree", parent_commit, "README.md"])
        if ls_out:
            readme_blob = ls_out.split()[2]
    except Exception:
        pass

    # Build firmware sub-tree via git mktree
    fw_tree_in = f"100644 blob {bin_blob}\tfirmware_autodoor_ota.bin\n100644 blob {json_blob}\tversion.json\n".encode("utf-8")
    p1 = subprocess.run(["git", "mktree"], cwd=str(ROOT), input=fw_tree_in, capture_output=True, check=True)
    fw_tree = p1.stdout.decode("utf-8").strip()

    # Build root tree
    root_tree_in = f"100644 blob {readme_blob}\tREADME.md\n040000 tree {fw_tree}\tfirmware\n".encode("utf-8")
    p2 = subprocess.run(["git", "mktree"], cwd=str(ROOT), input=root_tree_in, capture_output=True, check=True)
    root_tree = p2.stdout.decode("utf-8").strip()

    # Create commit
    commit_msg = f"Release firmware v{target_ver} OTA binary & metadata\n\n{changelog}"
    p3 = subprocess.run(
        ["git", "commit-tree", root_tree, "-p", parent_commit, "-m", commit_msg],
        cwd=str(ROOT), capture_output=True, check=True
    )
    new_commit = p3.stdout.decode("utf-8").strip()
    print(f"✨ Created commit: {new_commit[:7]} (tree: {root_tree[:7]})")

    if args.dry_run:
        print("🔍 Dry-run complete. Skipping push to GitHub.")
        return

    # Push to origin/main
    print("⬆️  Pushing to GitHub (origin/main)...")
    push_res = run_cmd(["git", "push", "origin", f"{new_commit}:main"])
    print(f"   {push_res}")

    print("\n" + "=" * 60)
    print(f"🎉 FIRMWARE v{target_ver} RELEASED TO GITHUB SUCCESSFULLY!")
    print(f"   - Repo     : https://github.com/autolinkpromax/AutoDoor-RF2")
    print(f"   - Metadata : https://raw.githubusercontent.com/autolinkpromax/AutoDoor-RF2/main/firmware/version.json")
    print(f"   - Binary   : https://raw.githubusercontent.com/autolinkpromax/AutoDoor-RF2/main/firmware/firmware_autodoor_ota.bin")
    print("=" * 60)
    print("💡 เครื่องอื่นๆ สามารถเข้าไปที่ Web UI ➔ ตั้งค่า ➔ กด '🔍 ตรวจสอบเวอร์ชันใหม่' แล้วกดอัปเดตได้ทันที!")


if __name__ == "__main__":
    main()
