# ESP-NOW Receiver Health Check Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a periodic health check for the ESP-NOW receiver on the ESP32-C3 to detect if the ESP-NOW module has been uninitialized or if the broadcast peer has been lost or has a channel mismatch, and recover automatically to ensure continuous operation with tags.

**Architecture:** 
1. Modify `addPeerMac()` in `esp_now_runtime.cpp` to verify and correct a peer's channel using `esp_now_get_peer()` and `esp_now_mod_peer()` if the peer already exists.
2. Implement `checkEspNowReceiverHealth()` in `esp_now_runtime.cpp` which runs every 10 seconds.
3. The health check detects if ESP-NOW is deinitialized by calling `esp_now_init()`. If it returns `ESP_OK`, it means ESP-NOW was uninitialized under the hood (e.g., due to WiFi resets), so it recovers callbacks and peer configurations.
4. The health check calls `addBroadcastPeer()` to verify the broadcast peer's channel matches the active WiFi/ESP-NOW channel.
5. Invoke `checkEspNowReceiverHealth()` in the main `loop()` in `src/main.cpp`.

**Tech Stack:** ESP32 Arduino SDK, ESP-NOW, ESP-IDF WiFi APIs.

---

### Task 1: Update Peer Addition and Channel Correction in Receiver

**Files:**
- Modify: [esp_now_runtime.cpp](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/modules/esp_now_runtime.cpp)

- [ ] **Step 1: Modify `addPeerMac()` in `esp_now_runtime.cpp`**
  Update the function to check if the peer exists. If it exists, retrieve its config and check if its channel matches the active ESP-NOW channel. If the channel is different, update the peer's channel. If retrieving fails, delete it first.
  ```cpp
  bool addPeerMac(const uint8_t mac[6]) {
    const uint8_t wantCh = currentEspNowChannel();
    if (esp_now_is_peer_exist(mac)) {
      esp_now_peer_info_t peer = {};
      if (esp_now_get_peer(mac, &peer) == ESP_OK) {
        if (peer.channel == wantCh) {
          return true;
        }
        peer.channel = wantCh;
        memcpy(peer.peer_addr, mac, 6);
        return esp_now_mod_peer(&peer) == ESP_OK;
      }
      esp_now_del_peer(mac);
    }
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = wantCh;
    peer.encrypt = false;
    return esp_now_add_peer(&peer) == ESP_OK;
  }
  ```

---

### Task 2: Implement ESP-NOW Health Checker

**Files:**
- Modify: [esp_now_runtime.h](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/modules/esp_now_runtime.h)
- Modify: [esp_now_runtime.cpp](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/modules/esp_now_runtime.cpp)

- [ ] **Step 1: Declare the health check function in `esp_now_runtime.h`**
  Add `void checkEspNowReceiverHealth();` to the header.
  ```cpp
  void checkEspNowReceiverHealth();
  ```

- [ ] **Step 2: Implement the health check function in `esp_now_runtime.cpp`**
  Add the function implementation to `esp_now_runtime.cpp` after `stepReceiverAnnounce()`.
  ```cpp
  void checkEspNowReceiverHealth() {
    if (gEspNowDisabled) return;

    const uint32_t now = millis();
    static uint32_t lastCheckMs = 0;
    if (now - lastCheckMs < 10000) return; // Health check every 10 seconds
    lastCheckMs = now;

    // 1. If not ready, attempt initialization
    if (!gEspNowReady) {
      Serial.println("[espnow] Health Check: ESP-NOW is enabled but not ready. Re-initializing...");
      initEspNow();
      return;
    }

    // 2. Verify if ESP-NOW is initialized in the system.
    // esp_now_init() returns ESP_ERR_ESPNOW_EXIST if it is already running.
    // If it returns ESP_OK, it means ESP-NOW was NOT running and has just been initialized.
    esp_err_t err = esp_now_init();
    if (err == ESP_OK) {
      Serial.println("[espnow] Health Check Recovery: ESP-NOW was uninitialized. Restoring...");
      esp_now_register_recv_cb(onEspNowRecv);
      esp_now_register_send_cb(onEspNowSend);
      gEspNowReady = true;
      ensureEspNowChannel();
      addBroadcastPeer();
      syncEspNowPeers();
    } else if (err != ESP_ERR_ESPNOW_EXIST) {
      Serial.printf("[espnow] Health Check: esp_now_init returned unexpected error %d. Resetting ready flag.\n", err);
      gEspNowReady = false;
      return;
    }

    // 3. Verify that the broadcast peer exists and has the correct channel
    addBroadcastPeer();
  }
  ```

---

### Task 3: Integrate Health Checker into Main Loop

**Files:**
- Modify: [main.cpp](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/main.cpp)

- [ ] **Step 1: Call `checkEspNowReceiverHealth()` in `loop()`**
  Insert the call right after `stepEspNowChannelSync();` in `src/main.cpp`.
  ```cpp
    stepEspNowChannelSync();
    checkEspNowReceiverHealth();
    ensureEspNowChannel();
  ```

---

## Verification Plan

### Automated Tests
- Run `pio run` on the main receiver (`c:\Users\max\Desktop\workplace\AutoDoor-RF2\esp-now`) to ensure the changes compile without errors.

### Manual Verification
- Deploy to the device and view the serial logs.
- Trigger WiFi reconnection or toggle AP mode via the Web UI and check if the ESP-NOW module remains healthy or is automatically restored if it gets uninitialized.
- Change ESP-NOW/STA channel via settings and check if the broadcast peer's channel updates to match the new channel.
