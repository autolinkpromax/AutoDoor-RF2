# WiFi Scan Crash Fix Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix the ESP32 crash/reboot that occurs when a user starts a WiFi scan and then closes the Web UI before the scan finishes. This happened because:
1. The scan results were never deleted from the WiFi driver (`WiFi.scanDelete()` was only called if the Web UI polled for status).
2. The health check ran `esp_now_init()` to verify status, which returned `ESP_OK` in this environment even when already initialized, causing constant re-initialization and driver conflicts.

**Architecture:** 
1. Simplify `checkEspNowReceiverHealth()` in `esp_now_runtime.cpp` by removing the redundant `esp_now_init()` status probe.
2. In `checkEspNowReceiverHealth()`, if `!gEspNowReady` and a scan is no longer active, check if scan results are present (`WiFi.scanComplete() >= 0`) and delete them in the background via `WiFi.scanDelete()`.
3. Resume ESP-NOW safely after the cleanup.

**Tech Stack:** ESP32 Arduino SDK, WiFi & ESP-NOW APIs.

---

### Task 1: Update Health Checker in Receiver

**Files:**
- Modify: [esp_now_runtime.cpp](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/modules/esp_now_runtime.cpp)

- [ ] **Step 1: Simplify `checkEspNowReceiverHealth()` in `esp_now_runtime.cpp`**
  Modify `checkEspNowReceiverHealth()` to clean up completed scans in the background and resume ESP-NOW without redundant `esp_now_init()` calls:
  ```cpp
  void checkEspNowReceiverHealth() {
    if (gEspNowDisabled) return;

    const int scanState = WiFi.scanComplete();
    if (scanState == WIFI_SCAN_RUNNING) return;

    // 1. If ESP-NOW was suspended for a scan, or failed to initialize,
    // and the scan is no longer running, recover/resume it.
    if (!gEspNowReady) {
      if (scanState >= 0) {
        Serial.printf("[wifi] Background scan cleanup: found %d networks\n", scanState);
        WiFi.scanDelete();
      }
      Serial.println("[espnow] Resuming ESP-NOW...");
      resumeEspNow();
      return;
    }

    const uint32_t now = millis();
    static uint32_t lastCheckMs = 0;
    if (now - lastCheckMs < 10000) return; // Health check every 10 seconds
    lastCheckMs = now;

    // 2. Verify that the broadcast peer exists and has the correct channel configuration
    addBroadcastPeer();
  }
  ```

---

## Verification Plan

### Automated Tests
- Run `pio run -e esp32-c3-devkitm-1` to check for compilation correctness.

### Manual Verification
- Deploy firmware to the ESP32-C3 board and access the Web UI via its STA IP address.
- Click "สแกน Wi-Fi" and immediately close the Web UI browser tab.
- Monitor the Serial output to verify that:
  - The ESP32 does not crash/restart.
  - The background scan is successfully cleaned up: `[wifi] Background scan cleanup: found ... networks`
  - ESP-NOW is successfully resumed in the background: `[espnow] Resuming ESP-NOW...`
