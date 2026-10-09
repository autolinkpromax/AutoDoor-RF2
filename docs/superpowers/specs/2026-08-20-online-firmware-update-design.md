# Online Firmware Update from GitHub Specification

## Overview
Add an online firmware update feature to AutoDoor-RF2 Settings page.
The system checks for firmware updates against `autolinkpromax/AutoDoor-RF2/firmware` (fetching `version.json` via GitHub Raw), prompts the user with version diff and changelog when a newer version is available, downloads the binary in the browser with real-time download progress, and uploads it to the ESP32 `/api/ota` endpoint.
Additionally, the build system automatically compiles release binaries and prepares `firmware/` directory release artifacts (`firmware_autodoor_ota.bin` and `version.json`) ready for push/upload to GitHub.

---

## Target Repository & Structure
- **GitHub Target:** `autolinkpromax/AutoDoor-RF2`
- **Path:** `firmware/`
- **Files on GitHub:**
  - `firmware/version.json` - metadata and release version
  - `firmware/firmware_autodoor_ota.bin` - compiled firmware binary

### `firmware/version.json` Schema:
```json
{
  "version": "1.0.0",
  "buildDate": "2026-08-20 12:00:00",
  "targetChip": "ESP32-C3",
  "file": "firmware_autodoor_ota.bin",
  "changelog": "Initial online update release for AutoDoor-RF2 Hub."
}
```

---

## Technical Components

### 1. Firmware Backend (`src/app/config.h`, `src/modules/http_api.cpp`)
- Define `kFirmwareVersion = "1.0.0"` in `config.h`.
- Expose `fwVersion` and `buildDate` in `/api/status`.
- Keep existing `/api/ota` endpoint for multipart upload & flashing.

### 2. WebUI HTML & CSS (`webui/html/partials/settings.html`, `webui/css/settings.css`)
- In `#cardOta`:
  - Show Current Firmware Version (`#otaCurVersion`).
  - Add button `btnCheckOnlineOta` ("🌐 ตรวจสอบอัพเดทออนไลน์").
  - Add Update Confirmation Modal / Notification Card with version comparison and changelog.
  - Progress bar supporting both GitHub download and Board Flash phases.

### 3. WebUI JavaScript (`webui/js/src/19-system.js`, `webui/js/src/00-constants.js`)
- `checkOnlineFirmwareUpdate()`: Fetches `version.json` from `raw.githubusercontent.com/autolinkpromax/AutoDoor-RF2/main/firmware/version.json` with fallback to `master`.
- Version comparator `compareSemver(v1, v2)`.
- If new version found: shows update dialog.
- `performOnlineFirmwareUpdate(fwInfo)`: Downloads binary blob with progress, pipes to `/api/ota` with progress bar, handles restart countdown.

### 4. Build System (`scripts/pack_webui.py`, `scripts/pio_targets.py`)
- Python script / target to bundle WebUI, compile release firmware, copy binary to `dist/` and `firmware/`, and generate `firmware/version.json`.
