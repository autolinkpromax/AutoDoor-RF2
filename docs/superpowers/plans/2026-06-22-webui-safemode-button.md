# Implementation Plan - Add Web UI Button to Enter Safe Mode Immediately

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a button inside the "Safe Mode" settings card in the Web UI to let authenticated users trigger Safe Mode and reboot the device immediately.

**Architecture:** 
1. Add a button styled with warning/stop styling in the Safe Mode settings card.
2. Bind the button to an `enter-safemode` action that confirms with the user and calls a new POST API `/api/safemode/enter`.
3. In the C++ back-end, implement `/api/safemode/enter` which sets a `forceSafeMode` flag in `Preferences` and restarts the ESP32.
4. Move `gPrefs.begin()` to run before `detectSafeModeBoot()` at boot time so that the boot code can read the `forceSafeMode` flag and enter Safe Mode if set.

**Tech Stack:** HTML5, CSS3, JavaScript (ES6), ESP32 WebServer, Preferences library.

---

## Proposed Changes

### Web UI Frontend
#### [MODIFY] [settings.html](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/webui/html/partials/settings.html)
- Add a button inside `#cardSafeModeBody`:
  ```html
  <button type="button" class="btnStop" style="margin-top:10px" data-action="enter-safemode">เข้าสู่ Safe Mode ทันที</button>
  ```

#### [MODIFY] [05-actions.js](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/webui/js/src/05-actions.js)
- Handle the `enter-safemode` action by prompting for confirmation and calling `enterSafeMode()`.

#### [MODIFY] [19-system.js](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/webui/js/src/19-system.js)
- Implement `enterSafeMode()` to call `/api/safemode/enter` using the `api()` client.

---

### ESP32 Backend
#### [MODIFY] [main.cpp](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/main.cpp)
- Move `gPrefs.begin("autodoor", false);` before `gSafeMode = detectSafeModeBoot();`.

#### [MODIFY] [status_led.cpp](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/modules/status_led.cpp)
- Update `detectSafeModeBoot()` to check if the `forceSafeMode` Preference flag is set. If true, clear it and return `true` to trigger Safe Mode.

#### [MODIFY] [http_api.cpp](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/modules/http_api.cpp)
- Define `handleEnterSafeMode()` which sets the `forceSafeMode` flag in `gPrefs` and restarts the ESP32.
- Map `/api/safemode/enter` route to `handleEnterSafeMode()`.

---

## Verification Plan

### Automated Tests
- Run `pio run -e esp32-c3-devkitm-1` to check for compilation correctness.

### Manual Verification
- Deploy firmware to the ESP32-C3 dev kit.
- Access the Web UI -> Settings -> ระบบและความปลอดภัย -> Safe Mode.
- Click "เข้าสู่ Safe Mode ทันที".
- Confirm the dialog box.
- Verify that:
  - The ESP32 restarts.
  - The ESP32 logs indicate `[safe] SAFE MODE enabled via Web UI flag` in the Serial Monitor.
  - The device re-initializes in Safe Mode (open AP, no web auth).
  - Restarting again (e.g. via Reset button or Restart Web UI button) returns the device to normal mode.
