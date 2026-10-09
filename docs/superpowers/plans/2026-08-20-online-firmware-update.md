# Online Firmware Update Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an online firmware update button in Settings page that checks GitHub (`autolinkpromax/AutoDoor-RF2/firmware`), prompts user when a newer version is found, downloads and flashes the firmware via OTA, and auto-generates release files upon build.

**Architecture:** Browser-mediated Fetch & Stream OTA. The WebUI fetches `version.json` from GitHub raw, compares semver with current device firmware version from `/api/status`, prompts confirmation with changelog, downloads binary from GitHub with progress bar, and posts it to `/api/ota` to flash and restart. Build automation packages `firmware/` artifacts (`version.json` and `firmware_autodoor_ota.bin`).

**Tech Stack:** C++ (ESP-IDF/Arduino ESP32), HTML5, Vanilla JavaScript, CSS, Python (PlatformIO extra script).

---

### Task 1: Backend Firmware Version Constant and API Exposure
**Files:**
- Modify: `src/app/config.h`
- Modify: `src/modules/http_api.cpp`

- [ ] **Step 1: Add `kFirmwareVersion` in `src/app/config.h`**
- [ ] **Step 2: Expose `fwVersion` and `buildDate` in `src/modules/http_api.cpp` in `handleStatus()`**

---

### Task 2: WebUI Settings Interface (HTML & CSS)
**Files:**
- Modify: `webui/html/partials/settings.html`
- Modify: `webui/css/settings.css`

- [ ] **Step 1: Add current version display, "ตรวจอัพเดทออนไลน์" button, and online update info card in `webui/html/partials/settings.html`**
- [ ] **Step 2: Add modal/card styling for update confirmation in `webui/css/settings.css`**

---

### Task 3: WebUI Online OTA Client Logic (JavaScript)
**Files:**
- Modify: `webui/js/src/00-constants.js`
- Modify: `webui/js/src/05-actions.js`
- Modify: `webui/js/src/19-system.js`

- [ ] **Step 1: Add GitHub repo constants in `00-constants.js`**
- [ ] **Step 2: Add action triggers in `05-actions.js`**
- [ ] **Step 3: Implement `renderOtaStatus`, `checkOnlineFirmwareUpdate`, `compareVersions`, and `performOnlineFirmwareUpdate` in `19-system.js`**

---

### Task 4: Build System & Release Artifacts
**Files:**
- Create: `firmware/version.json`
- Modify: `scripts/pack_webui.py`
- Modify: `scripts/pio_targets.py`

- [ ] **Step 1: Create `firmware/version.json`**
- [ ] **Step 2: Update `scripts/pack_webui.py` to bundle WebUI and compile/copy release binaries into `firmware/` and `dist/`**
- [ ] **Step 3: Test WebUI pack and PlatformIO build**
