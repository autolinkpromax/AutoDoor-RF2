# WiFi Scan Fix Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Resolve the issue where WiFi scanning fails during operation, and prevent web UI disconnections when connected over STA. We will suspend ESP-NOW during WiFi scanning to free up the radio, and keep the STA connection alive by removing the unnecessary `WiFi.disconnect()` call.

**Architecture:** 
1. Add `suspendEspNow()` and `resumeEspNow()` functions to `esp_now_runtime.cpp` to deinitialize and reinitialize ESP-NOW.
2. In `checkEspNowReceiverHealth()`, skip health checks if a WiFi scan is in progress (`WiFi.scanComplete() == WIFI_SCAN_RUNNING`) to prevent premature re-initialization of ESP-NOW.
3. Modify `handleWifiScanStart()` in `http_api.cpp` to call `suspendEspNow()` and remove `WiFi.disconnect()`.
4. Modify `handleWifiScanStatus()` in `http_api.cpp` to call `resumeEspNow()` as soon as the scan completes (succeeds or fails).

**Tech Stack:** ESP32 Arduino SDK, ESP-NOW, ESP32 WebServer.

---

### Task 1: Declare and Implement ESP-NOW Suspension Functions

**Files:**
- Modify: [esp_now_runtime.h](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/modules/esp_now_runtime.h)
- Modify: [esp_now_runtime.cpp](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/modules/esp_now_runtime.cpp)

- [ ] **Step 1: Declare suspension functions in `esp_now_runtime.h`**
  Add declarations for `suspendEspNow()` and `resumeEspNow()`.
  ```cpp
  void suspendEspNow();
  void resumeEspNow();
  ```

- [ ] **Step 2: Implement suspension functions and update health check in `esp_now_runtime.cpp`**
  Add implementations for `suspendEspNow()` and `resumeEspNow()`, and modify `checkEspNowReceiverHealth()` to yield during active scans.
  ```cpp
  void suspendEspNow() {
    if (gEspNowReady) {
      esp_now_deinit();
      gEspNowReady = false;
      Serial.println("[espnow] suspended (esp_now_deinit)");
    }
  }

  void resumeEspNow() {
    if (!gEspNowDisabled && !gEspNowReady) {
      Serial.println("[espnow] resuming ESP-NOW...");
      initEspNow();
    }
  }
  ```
  And update `checkEspNowReceiverHealth()`:
  ```cpp
  void checkEspNowReceiverHealth() {
    if (gEspNowDisabled) return;
    if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) return; // Skip health check during active WiFi scans
    
    const uint32_t now = millis();
    ...
  ```

---

### Task 2: Update WiFi Scan APIs in HTTP Web Server

**Files:**
- Modify: [http_api.cpp](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/modules/http_api.cpp)

- [ ] **Step 1: Update `handleWifiScanStart()` in `http_api.cpp`**
  Remove `WiFi.disconnect()` and call `suspendEspNow()` instead.
  ```cpp
  void handleWifiScanStart() {
    Serial.println("[wifi] scan requested. suspending ESP-NOW...");
    suspendEspNow();
    delay(100);
    
    gWifiScanDone = false;
    gWifiScanCount = WiFi.scanNetworks(/*async=*/true, /*show_hidden=*/true);
    Serial.printf("[wifi] scan started. scanNetworks status: %d\n", gWifiScanCount);
    
    JsonDocument doc;
    doc["ok"] = true;
    doc["started"] = true;
    sendJson(200, doc);
  }
  ```

- [ ] **Step 2: Update `handleWifiScanStatus()` in `http_api.cpp`**
  Call `resumeEspNow()` once the scan finishes.
  ```cpp
  void handleWifiScanStatus() {
    const int n = WiFi.scanComplete();
    Serial.printf("[wifi] scan status poll: scanComplete status = %d\n", n);
    
    if (n == WIFI_SCAN_RUNNING) {
      JsonDocument doc;
      doc["ok"] = true;
      doc["done"] = false;
      sendJson(200, doc);
      return;
    }
    
    // Resume ESP-NOW since the scan is finished (succeeded or failed)
    resumeEspNow();
    
    JsonDocument doc;
    if (n == WIFI_SCAN_FAILED) {
      ...
  ```

---

## Verification Plan

### Automated Tests
- Run `pio run -e esp32-c3-devkitm-1` to check for compilation and syntax correctness.

### Manual Verification
- Deploy firmware to the ESP32-C3 board and access the Web UI via its STA IP address.
- Click "สแกน Wi-Fi" and verify that:
  - The Web UI does not lose connection.
  - The scan successfully returns the list of available WiFi networks.
  - The Serial Monitor shows ESP-NOW suspending and then resuming once the scan concludes.
