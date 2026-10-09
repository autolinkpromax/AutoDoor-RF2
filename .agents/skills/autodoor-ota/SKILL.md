---
name: autodoor-ota
description: >-
  Flash firmware via OTA to AutoDoor ESP32 devices (192.168.1.11 & 192.168.1.12). Use when user asks to upload firmware via OTA, flash both devices, OTA ทั้ง 2 ตัว, flash ESP32 over WiFi, check device status, or mentions updating 192.168.1.11 / 192.168.1.12.
---

# AutoDoor-RF2 OTA Firmware Upload Skill

Wirelessly flash firmware to the AutoDoor ESP32-C3 controllers via `/api/ota`.

## Quick Commands (Run in Workspace Root)

### 1. Flash BOTH Devices (192.168.1.11 & 192.168.1.12)
* **After code changes (Compile once + Flash both)**:
  ```bash
  python scripts/ota_upload.py --both --build
  ```
* **Flash existing binary directly**:
  ```bash
  python scripts/ota_both.py
  # or: python scripts/ota_upload.py --both
  ```

### 2. Flash Single Device (Custom IP)
* **Flash 192.168.1.11**:
  ```bash
  python scripts/ota_upload.py --ip 192.168.1.11
  ```
* **Flash 192.168.1.12**:
  ```bash
  python scripts/ota_upload.py --ip 192.168.1.12
  ```

### 3. Check Online & Partition Status (No Upload)
```bash
python scripts/ota_upload.py --check --both
```

## Execution Flow
1. Execute the single command above using `run_command`.
2. Inspect the `📊 OTA SUMMARY` in stdout.
3. Report success/fail to user directly. Do not run extra commands if summary is green.
