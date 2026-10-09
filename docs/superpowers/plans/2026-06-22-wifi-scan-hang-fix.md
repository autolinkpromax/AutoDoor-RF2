# WiFi Scan Hang Fix Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix the issue where the ESP32 hangs/freezes or loses connection completely when starting a WiFi scan. This is caused by background tasks (`stepEspNowChannelSync()`, `stepWiFiReconnect()`, and channel enforcement tasks) trying to execute, write to flash, and restart WiFi interfaces while the radio is busy hopping channels for the scan.

**Architecture:** 
1. Modify `stepEspNowChannelSync()` and `stepWiFiReconnect()` in `wifi_manager.cpp` to check if a scan is running (`WiFi.scanComplete() == WIFI_SCAN_RUNNING`) and return immediately if so.
2. Modify `ensureEspNowChannel()`, `stepReceiverAnnounce()`, and `stepTagLearn()` in `esp_now_runtime.cpp` to similarly return early during active WiFi scans.
3. This prevents write operations to flash, AP restarts, and WiFi begin calls from disrupting the WiFi driver during scanning.

**Tech Stack:** ESP32 Arduino SDK, WiFi & ESP-NOW APIs.

---

### Task 1: Update WiFi Manager to Bypass Tasks During Scanning

**Files:**
- Modify: [wifi_manager.cpp](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/modules/wifi_manager.cpp)

- [ ] **Step 1: Update `stepEspNowChannelSync()` in `wifi_manager.cpp`**
  Add a scan check at the beginning:
  ```cpp
  void stepEspNowChannelSync() {
    if (gEspNowDisabled) return;
    if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) return; // Bypass during active WiFi scans
    if (WiFi.status() != WL_CONNECTED) return;
    ...
  ```

- [ ] **Step 2: Update `stepWiFiReconnect()` in `wifi_manager.cpp`**
  Add a scan check at the beginning:
  ```cpp
  void stepWiFiReconnect() {
    if (gSafeMode || gEspNowDisabled) return;
    if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) return; // Bypass during active WiFi scans
    const String staSsid = gPrefs.getString("wifiSsid", "");
    ...
  ```

---

### Task 2: Update ESP-NOW Runtime to Bypass Tasks During Scanning

**Files:**
- Modify: [esp_now_runtime.cpp](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/modules/esp_now_runtime.cpp)

- [ ] **Step 1: Update `ensureEspNowChannel()` in `esp_now_runtime.cpp`**
  Add a scan check at the beginning:
  ```cpp
  void ensureEspNowChannel() {
    if (gEspNowDisabled) return;
    if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) return; // Bypass during active WiFi scans
    if (!gEspNowReady) return;
    ...
  ```

- [ ] **Step 2: Update `stepReceiverAnnounce()` in `esp_now_runtime.cpp`**
  Add a scan check at the beginning:
  ```cpp
  void stepReceiverAnnounce() {
    if (gEspNowDisabled) return;
    if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) return; // Bypass during active WiFi scans
    if (!gEspNowReady) return;
    ...
  ```

- [ ] **Step 3: Update `stepTagLearn()` in `esp_now_runtime.cpp`**
  Add a scan check at the beginning:
  ```cpp
  void stepTagLearn() {
    if (gEspNowDisabled) return;
    if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) return; // Bypass during active WiFi scans
    if (!gLearn.active) return;
    ...
  ```

---

## Verification Plan

### Automated Tests
- Run `pio run -e esp32-c3-devkitm-1` to check for compilation correctness.

### Manual Verification
- Deploy firmware to the ESP32-C3 board and access the Web UI via its STA IP address.
- Click "สแกน Wi-Fi" and verify that:
  - The Web UI does not freeze.
  - The connection remains alive.
  - The scan finishes successfully and displays the list of networks.
