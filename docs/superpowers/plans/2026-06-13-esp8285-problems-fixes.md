# ESP8285 Tag Problems & Fixes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Resolve ESP8285/ESP8266 tag transmission and sleep bugs (including error -3, ISR WDT resets, and wrong EEPROM channel loads) and add custom PlatformIO configurations.

**Architecture:**
1. **Self Role & Init Order:** Configure the ESP8285 self role as `ESP_NOW_ROLE_CONTROLLER` instead of `ESP_NOW_ROLE_COMBO` to resolve `esp_now_send` returning `-3`. Maintain the exact SDK initialization order.
2. **ISR Safety:** Move serial printing and EEPROM channel saving/configuration out of `onEspNowRecv` (which runs in interrupt context) into the main thread loop via a deferred update pattern.
3. **Hardware & Power Stability:** Disable WiFi sleep mode using `WiFi.setSleepMode(WIFI_NONE_SLEEP)` and configure `WiFi.setPhyMode(WIFI_PHY_MODE_11N)`.
4. **PlatformIO Envs:** Modify `platformio.ini` to support `esp8285`, `esp8285-release` (no serial debug output), and `esp8285-hello` environments.

**Tech Stack:** ESP8266/ESP8285 Arduino SDK, PlatformIO, ESP-NOW, EEPROM.

---

### Task 1: Update PlatformIO Config & Add Hello World Test

**Files:**
- Modify: [platformio.ini](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/tag-esp8266/platformio.ini)
- Create: [hello/src/main.cpp](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/tag-esp8266/hello/src/main.cpp)

- [ ] **Step 1: Rewrite `platformio.ini` to define esp8285 environments**
  Replace `tag-esp8266/platformio.ini` contents with target-specific environments (`esp8285`, `esp8285-release`, `esp8285-hello`):
  ```ini
  [platformio]
  default_envs = esp8285

  [env:esp8285]
  platform = espressif8266
  board = esp8285
  framework = arduino
  monitor_speed = 115200
  upload_speed = 921600
  build_flags =
      -DTAG_ESPNOW_CHANNEL=9
      -DTAG_DEBUG_SERIAL=1
  board_build.flash_mode = dout
  board_build.f_flash = 26000000L
  upload_resetmethod = nodemcu

  [env:esp8285-release]
  platform = espressif8266
  board = esp8285
  framework = arduino
  monitor_speed = 115200
  upload_speed = 921600
  build_flags =
      -DTAG_ESPNOW_CHANNEL=9
      -DTAG_DEBUG_SERIAL=0
  board_build.flash_mode = dout
  board_build.f_flash = 26000000L
  upload_resetmethod = nodemcu

  [env:esp8285-hello]
  platform = espressif8266
  board = esp8285
  framework = arduino
  monitor_speed = 115200
  upload_speed = 921600
  build_src_filter = +<hello/src/> -<src/>
  board_build.flash_mode = dout
  board_build.f_flash = 26000000L
  upload_resetmethod = nodemcu
  ```

- [ ] **Step 2: Create Hello World test file `hello/src/main.cpp`**
  Write a simple hello world script to verify basic flashing and serial monitoring works:
  ```cpp
  #include <Arduino.h>

  void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Hello World from ESP8285 / ESP-01!");
  }

  void loop() {
    Serial.println("Running...");
    delay(2000);
  }
  ```

- [ ] **Step 3: Compile `esp8285-hello` to verify configuration**
  Run: `pio run -e esp8285-hello`
  Expected: Success

---

### Task 2: Implement tag firmware fixes in `src/main.cpp`

**Files:**
- Modify: [main.cpp](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/tag-esp8266/src/main.cpp)

- [ ] **Step 1: Replace `tag-esp8266/src/main.cpp` with optimized ESP8285 code**
  Implement the deferred ISR, correct `CONTROLLER` role, clean initialization order, auto-recovery logic, and rate-limited debug macros. Show the full source code.

- [ ] **Step 2: Compile `esp8285` debug environment**
  Run: `pio run -e esp8285`
  Expected: Success

- [ ] **Step 3: Compile `esp8285-release` environment**
  Run: `pio run -e esp8285-release`
  Expected: Success

---

### Task 3: Save Reference Notes in the Repository

**Files:**
- Create: [AI-NOTES.md](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/tag-esp8266/AI-NOTES.md)

- [ ] **Step 1: Save `AI-NOTES.md` inside `tag-esp8266`**
  Save the reference documentation inside the repository for future developer or AI reference.

---

## Verification Plan

### Automated Tests
- Run `pio run -e esp8285`
- Run `pio run -e esp8285-release`
- Run `pio run -e esp8285-hello`

### Manual Verification
- Flash the `esp8285` firmware onto the ESP8285 module.
- Confirm via Serial Monitor:
  - `=== AutoDoor Tag ESP8285 ===` banner prints first.
  - Initial bursts show `ok` sends (`[boot] first bursts ok=9 fail=0`).
  - Active scan logs show searching channels (`[ch] auto scan try ch=...`).
  - Upon receiving announcement, logs output `[hub] announce ch=...` followed by lock-on and persistence.
