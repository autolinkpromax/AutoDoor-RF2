# Auto-Channel Scanning Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement automatic channel scanning on the main ESP32-C3 receiver to choose the cleanest channel on boot, and enable the ESP8285/ESP8266 tag to dynamically scan and sync its channel with the receiver.

**Architecture:** 
1. The ESP32-C3 receiver scans all channels on boot and calculates an RSSI-weighted interference score for each channel, selecting and saving the cleanest channel.
2. The ESP8285/ESP8266 tag implements channel hopping (every 100ms) when the receiver's periodic announcement packet (`ReceiverAnnounce`) is not heard.
3. Once the tag hears the receiver's announcement on a channel, it locks onto that channel and saves it to EEPROM/Preferences so it connects instantly on the next boot.

**Tech Stack:** ESP8266 Arduino SDK, ESP32 Arduino SDK, ESP-NOW, EEPROM/Preferences.

---

### Task 1: Improve Receiver Channel Selection Algorithm

**Files:**
- Modify: [wifi_manager.cpp](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/src/modules/wifi_manager.cpp#L75-L92)

- [ ] **Step 1: Modify `pickLeastCongestedChannel` in `wifi_manager.cpp`**
  Modify the channel selection function to use RSSI-weighted scoring to find the channel with the lowest RF noise/congestion:
  ```cpp
  uint8_t pickLeastCongestedChannel(int networkCount) {
    float channelScore[14] = {0};
    for (int i = 0; i < networkCount; ++i) {
      const int ch = WiFi.channel(i);
      if (ch >= 1 && ch <= 13) {
        const int rssi = WiFi.RSSI(i);
        // Weight power contribution of each network based on signal strength
        // RSSI to linear scale: e.g. -100dBm = 1 unit, -90dBm = 10 units, etc.
        float power = pow(10.0f, (rssi + 100.0f) / 10.0f);
        channelScore[ch] += power;
      }
    }
    uint8_t bestCh = kEspNowChannel;
    float minScore = 1e12f;
    for (int ch = 1; ch <= 13; ++ch) {
      if (channelScore[ch] < minScore) {
        minScore = channelScore[ch];
        bestCh = (uint8_t)ch;
      }
    }
    return bestCh;
  }
  ```

- [ ] **Step 2: Compile the receiver code to verify**
  Run compilation for ESP32-C3 to check for syntax or type errors.

---

### Task 2: Implement Channel Hopping & Auto-Sync on ESP8266/ESP8285 Tag

**Files:**
- Modify: [main.cpp](file:///c:/Users/max/Desktop/workplace/AutoDoor-RF2/esp-now/tag-esp8266/src/main.cpp)

- [ ] **Step 1: Implement the Channel Hopping & Sync Logic in `tag-esp8266/src/main.cpp`**
  Modify the tag implementation to:
  1. Initialize EEPROM to store the current channel.
  2. Register the ESP-NOW receive callback and use `ESP_NOW_ROLE_COMBO` to receive `ReceiverAnnounce` messages.
  3. Hop channels every 100ms if no `ReceiverAnnounce` message has been received within 15 seconds.
  4. Lock to the correct channel and save to EEPROM once an announcement is received.

- [ ] **Step 2: Compile the tag code to verify**
  Compile the ESP8266 project to verify there are no compilation errors.

---

## Verification Plan

### Automated Tests
- Run `pio run` on the main receiver (`c:\Users\max\Desktop\workplace\AutoDoor-RF2\esp-now`)
- Run `pio run` on the tag (`c:\Users\max\Desktop\workplace\AutoDoor-RF2\esp-now\tag-esp8266`)

### Manual Verification
- Flash the main receiver and watch the serial console to confirm that on boot it scans networks, prints channel scores, and selects/saves the cleanest channel.
- Flash the ESP8285/ESP8266 tag. With the receiver on, verify via the tag's serial output that it hops channels, receives the receiver's announcement, saves the channel, and stops hopping.
- Change the receiver's channel (via the Web UI or by rebooting) and verify that the tag detects the loss of receiver announcements, hops channels to find the new channel, and locks onto it.
