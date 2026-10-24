#include "esp_now_runtime.h"

#include "app/config.h"
#include "app/globals.h"
#include "tags/TagRegistry.h"
#include "modules/wifi_manager.h"

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <SYN480R.h>

uint8_t currentEspNowChannel();

namespace {

bool gEspNowReady = false;
uint32_t gLastChannelFixMs = 0;
uint32_t gLastAnnounceMs = 0;
constexpr uint32_t kAnnounceIntervalMs = 250;       // Fast interval when tag is active
constexpr uint32_t kAnnounceIntervalIdleMs = 1000;  // Slow interval when no tag active (saves 75% Wi-Fi TX)
constexpr uint32_t kChannelFixIntervalMs = 2000;

bool isValidBeacon(const uint8_t* data, int len, TagBeacon* out) {
  if (len < (int)sizeof(TagBeacon)) return false;
  memcpy(out, data, sizeof(TagBeacon));
  if (out->magic[0] != kBeaconMagic0 || out->magic[1] != kBeaconMagic1) return false;
  if (out->version != kBeaconVersion && out->version != 2) return false;
  return true;
}

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

void removePeerMac(const uint8_t mac[6]) {
  if (esp_now_is_peer_exist(mac)) {
    esp_now_del_peer(mac);
  }
}

bool addBroadcastPeer() {
  static const uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  return addPeerMac(broadcast);
}

struct EspNowRxPacket {
  uint8_t mac[6];
  int8_t rssi;
  uint8_t len;
  uint8_t data[sizeof(TagBeacon)];
  uint32_t rxMs;
};

static QueueHandle_t sEspNowRxQueue = nullptr;

void onEspNowRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (!gSystemBootReady || !sEspNowRxQueue || !info || !data) return;
  if (len < (int)sizeof(TagBeacon)) return;
  if (data[0] != kBeaconMagic0 || data[1] != kBeaconMagic1) return;

  EspNowRxPacket pkt;
  memcpy(pkt.mac, info->src_addr, 6);
  pkt.rssi = info->rx_ctrl ? (int8_t)info->rx_ctrl->rssi : (int8_t)-127;
  pkt.len = (uint8_t)min((size_t)len, sizeof(pkt.data));
  memcpy(pkt.data, data, pkt.len);
  pkt.rxMs = millis();

  // Non-blocking queue send: Wi-Fi driver task NEVER blocks or waits on mutexes!
  xQueueSend(sEspNowRxQueue, &pkt, 0);
}

static void processEspNowPacket(const uint8_t mac[6], int8_t rssi, const uint8_t* data, uint8_t len, uint32_t now) {
  TagBeacon beacon{};
  if (!isValidBeacon(data, len, &beacon)) {
    if (gLearn.active) {
      Serial.println("[espnow] ignore: bad beacon");
    }
    return;
  }

  if (gLearn.active) {
    const String macStr = macToString(mac);
    Serial.printf("[espnow] rx %s rssi=%d len=%d ch=%u\n", macStr.c_str(), (int)rssi, (int)len,
                  (unsigned)currentEspNowChannel());
  }

  TagStoreLock lock;
  Tag* tag = findTagByMac(mac);
  if (gLearn.active && (int32_t)(now - gLearn.deadlineMs) < 0) {
    const String macStr = macToString(mac);
    if (!tag) {
      tag = createTag("", macStr, beacon.tagId);
      if (!tag) {
        tag = findTagByMac(mac);
        if (tag) {
          Serial.printf("[espnow] learn: tag already exists %s\n", macStr.c_str());
        } else {
          Serial.println("[espnow] learn: create tag failed");
        }
      }
    }
    if (tag) {
      gLearn.lastMac = macStr;
      gLearn.lastTagId = beacon.tagId;
      gLearn.createdTagId = tag->id;
      gLearn.active = false; // Stop learn mode immediately after learning 1 tag
      if (gSafeMode) {
        gSafeMode = false;
        Serial.println("[safe] ESP-NOW Tag learned -> Auto-clearing Safe Mode");
      }
      gTagRegistry.saveToPrefs();
      Serial.printf("[espnow] learned %s id=%s tagId=%lu\n", macStr.c_str(), tag->id.c_str(),
                    (unsigned long)beacon.tagId);
    } else {
      return;
    }
  }

  if (!tag) return;

  if (tag->lastSeenMs == 0 || (uint32_t)(now - tag->lastSeenMs) > tag->tagHoldMs) {
    tag->smoothedRssi = (float)rssi;
  } else if ((uint32_t)(now - tag->lastSeenMs) >= 20) {
    // Asymmetric EMA filter:
    // 1. Rising RSSI (approaching) -> Fast response (alpha = 0.85) to trigger door opening immediately when in range.
    // 2. Falling RSSI (departing or transient drop/noise) -> Smooth (alpha = 0.08) to filter out fluctuations without false close.
    // Filter out intra-burst repeat frames (<20ms) so 3-packet bursts don't jump EMA 3x faster.
    float alpha = ((float)rssi > tag->smoothedRssi) ? 0.85f : 0.08f;
    tag->smoothedRssi = alpha * (float)rssi + (1.0f - alpha) * tag->smoothedRssi;
  }
  const int8_t currentRssi = (int8_t)round(tag->smoothedRssi);

  // Print detailed RSSI tracking log to Serial Monitor for analysis
  AD_LOG(Serial.printf("[rssi] tag=%s raw=%d smoothed=%d (min=%d active=%s)\n",
                tag->id.c_str(), (int)rssi, (int)currentRssi,
                (int)tag->tagRssiMin,
                isTagActive(*tag, now) ? "yes" : "no"));

  const int8_t rssiThreshold =
      (tag->lastSeenMs != 0 && isTagActive(*tag, now)) ? (int8_t)(tag->tagRssiMin - kTagRssiHysteresisDb)
                                                       : tag->tagRssiMin;
  if (currentRssi < rssiThreshold) return;

  const bool wasActive = isTagActive(*tag, now);
  gTagRegistry.noteSeen(*tag, currentRssi, now);
  triggerImmediateAnnounce();
}

void onEspNowSend(const uint8_t* mac, esp_now_send_status_t status) {
  (void)mac;
  (void)status;
}

}  // namespace

void stepEspNowRxProcessing() {
  if (gSafeMode || !sEspNowRxQueue) return;
  EspNowRxPacket pkt;
  while (xQueueReceive(sEspNowRxQueue, &pkt, 0) == pdTRUE) {
    processEspNowPacket(pkt.mac, pkt.rssi, pkt.data, pkt.len, pkt.rxMs);
  }
}

uint8_t currentEspNowChannel() {
  if (WiFi.status() == WL_CONNECTED) {
    const int ch = WiFi.channel();
    if (ch >= 1 && ch <= 13) return (uint8_t)ch;
  }
  return gEspNowChannel;
}

void ensureEspNowChannel() {
  if (gSafeMode || gEspNowDisabled) return;
  if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) return; // Bypass during active WiFi scans
  if (!gEspNowReady) return;

  const uint32_t now = millis();
  static uint32_t lastEnsureChMs = 0;
  if ((uint32_t)(now - lastEnsureChMs) < 5000) return;
  lastEnsureChMs = now;

  const uint8_t want = currentEspNowChannel();
  const int have = WiFi.channel();
  if (have != (int)want) {
    esp_wifi_set_channel(want, WIFI_SECOND_CHAN_NONE);
    if (gLearn.active || (uint32_t)(now - gLastChannelFixMs) > 10000) {
      Serial.printf("[espnow] channel fix %d -> %u (sta=%s)\n", have, want,
                    WiFi.status() == WL_CONNECTED ? "on" : "off");
      gLastChannelFixMs = now;
    }
  }
}

void initEspNow() {
  if (gSafeMode || gEspNowDisabled) return;
  if (sEspNowRxQueue == nullptr) {
    sEspNowRxQueue = xQueueCreate(16, sizeof(EspNowRxPacket));
  }
  if (esp_now_init() != ESP_OK) {
    Serial.println("FATAL: esp_now_init failed");
    return;
  }
  applyWifiTxPower();
  esp_now_register_recv_cb(onEspNowRecv);
  esp_now_register_send_cb(onEspNowSend);
  gEspNowReady = true;
  ensureEspNowChannel();
  addBroadcastPeer();
  syncEspNowPeers();
  Serial.printf("[espnow] ready ch=%u mac=%s\n", (unsigned)currentEspNowChannel(),
                WiFi.macAddress().c_str());
  if (WiFi.status() == WL_CONNECTED && WiFi.channel() != (int)gEspNowChannel) {
    Serial.printf("[espnow] WARN: STA on ch%d but tag default ch%u — tag must match STA channel\n",
                  WiFi.channel(), (unsigned)gEspNowChannel);
  }
}

void disableEspNow() {
  if (gEspNowReady) {
    esp_now_deinit();
    gEspNowReady = false;
    Serial.println("[espnow] deactivated (esp_now_deinit)");
  }
  gEspNowDisabled = true;
  
  // Disable auto-reconnect and disconnect active connections to free the radio for scanning/setup
  Serial.println("[wifi] Settings opened: disabling auto-reconnect and disconnecting WiFi...");
  WiFi.setAutoReconnect(false);
  WiFi.disconnect();
}

void enableEspNow() {
  gEspNowDisabled = false;
  
  // Re-enable auto-reconnect when leaving setup
  Serial.println("[wifi] Settings closed/done: enabling auto-reconnect...");
  WiFi.setAutoReconnect(true);
  
  if (!gEspNowReady) {
    initEspNow();
  }
}

void syncEspNowPeers() {
  if (!gEspNowReady) return;
  addBroadcastPeer();
}

void removeEspNowPeer(const String& mac) {
  uint8_t addr[6];
  if (parseMac(mac, addr)) {
    removePeerMac(addr);
  }
}

bool startTagLearn() {
  // TagStoreLock protects gLearn String fields (lastMac, createdTagId) from
  // concurrent modification by rf433Task or onEspNowRecv callback.
  TagStoreLock lock;
  const uint32_t now = millis();
  gLearn.lastMac = "";
  gLearn.lastTagId = 0;
  gLearn.createdTagId = "";
  gLearn.deadlineMs = now + kLearnTimeoutMs;
  gLearn.active = true;  // Set active LAST so other tasks see consistent state
  ensureEspNowChannel();
  addBroadcastPeer();
  Serial.printf("[espnow] learn start ch=%u (tag must use same channel)\n",
                (unsigned)currentEspNowChannel());
  return true;
}

void stopTagLearn() {
  TagStoreLock lock;
  gLearn.active = false;
  gLearn.lastMac = "";
  gLearn.lastTagId = 0;
  gLearn.createdTagId = "";
  gLearn.deadlineMs = 0;
}

void stepTagLearn() {
  if (!gLearn.active) return;
  ensureEspNowChannel();
  const uint32_t now = millis();
  if ((int32_t)(now - gLearn.deadlineMs) >= 0) {
    stopTagLearn();
  }
}

static volatile bool sPendingImmediateAnnounce = false;

void triggerImmediateAnnounce() {
  sPendingImmediateAnnounce = true;
}

void stepReceiverAnnounce() {
  if (gSafeMode || gEspNowDisabled) return;
  if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) return; // Bypass during active WiFi scans
  if (!gEspNowReady) return;
  const uint32_t now = millis();
  const bool immediate = sPendingImmediateAnnounce;
  // Use fast interval when any tag was recently active, slow interval when idle
  const uint32_t baseInterval = gPrevAnyTagActive ? kAnnounceIntervalMs : kAnnounceIntervalIdleMs;
  if (immediate) {
    if ((uint32_t)(now - gLastAnnounceMs) < 25) return;
  } else {
    if ((uint32_t)(now - gLastAnnounceMs) < baseInterval) return;
  }

  // Smart RF Collision Avoidance: if 433MHz receiver is currently receiving an RF frame from remote,
  // defer ESP-NOW transmission by 100ms so Wi-Fi TX burst does not interfere with RF433 frame decoding.
  // Cap max deferral to 300ms so continuous RF noise won't block announcements indefinitely.
  static uint8_t rfDeferCount = 0;
  if (SYN480R::isReceiving() && rfDeferCount < 3) {
    rfDeferCount++;
    gLastAnnounceMs = now - kAnnounceIntervalMs + 100;
    return;
  }
  rfDeferCount = 0;

  sPendingImmediateAnnounce = false;
  gLastAnnounceMs = now;

  static const uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  ReceiverAnnounce msg = {};
  msg.magic[0] = kAnnounceMagic0;
  msg.magic[1] = kAnnounceMagic1;
  msg.version = kAnnounceVersion;
  msg.channel = currentEspNowChannel();

  // Protect multi-variable door state reads from concurrent modification
  TagStoreLock lock;

  // Set door status: 1 = Opening/Open/Stopped, 2 = Closing, 4 = Safety Sensor Blocked, 0 = Other/Closed
  uint8_t status = 0;
  if (gSafetySensorEnabled && gSafetySensorBlocked) {
    status = 4;
  } else if (gDoorState == DoorState::Closing) {
    status = 2;
  } else if (gDoorState == DoorState::Opening || gDoorState == DoorState::Open || gDoorState == DoorState::Stopped) {
    status = 1;
  }
  if (!gTagBuzzerEnabled) {
    status |= 0x80; // Bit 7: 1 = Buzzer Muted
  }
  msg.reserved = status;

  esp_now_send(broadcast, (uint8_t*)&msg, sizeof(msg));
}

void checkEspNowReceiverHealth() {
  if (gSafeMode || gEspNowDisabled) return;

  const int scanState = WiFi.scanComplete();
  if (scanState == WIFI_SCAN_RUNNING) return;

  // 1. If ESP-NOW was suspended for a scan, or failed to initialize,
  // and the scan is no longer running, recover/resume it in the background
  // only after a 15-second timeout has elapsed since the scan started.
  if (!gEspNowReady) {
    if ((uint32_t)(millis() - gWifiScanStartMs) >= 15000) {
      if (scanState >= 0) {
        Serial.printf("[wifi] Background scan cleanup: found %d networks\n", scanState);
        WiFi.scanDelete();
      }
      Serial.println("[espnow] Resuming ESP-NOW...");
      resumeEspNow();
    }
    return;
  }

  const uint32_t now = millis();
  static uint32_t lastCheckMs = 0;
  if ((uint32_t)(now - lastCheckMs) < 10000) return; // Health check every 10 seconds
  lastCheckMs = now;

  // 2. Verify that the broadcast peer exists and has the correct channel configuration
  addBroadcastPeer();
}

bool isEspNowReady() {
  return gEspNowReady;
}

void suspendEspNow() {
  if (gEspNowReady) {
    esp_now_deinit();
    gEspNowReady = false;
    Serial.println("[espnow] suspended (esp_now_deinit)");
  }
}

void resumeEspNow() {
  if (gSafeMode) {
    return; // Remain suspended while in Safe Mode
  }
  if (!gEspNowDisabled && !gEspNowReady) {
    Serial.println("[espnow] resuming ESP-NOW...");
    initEspNow();
  }
}
