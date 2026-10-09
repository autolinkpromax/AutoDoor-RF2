#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>

extern "C" {
#include <espnow.h>
#include <user_interface.h>
}

#ifndef TAG_CHANNEL
#define TAG_CHANNEL 9
#endif

constexpr uint32_t kBeaconIntervalMs = 100;
constexpr uint8_t kBurstCount = 3;
constexpr uint8_t kBootBurstCount = 6;
constexpr uint32_t kBootSettleMs = 400;
constexpr uint32_t kReceiverTimeoutMs = 5000;
constexpr uint32_t kChannelHopMs = 100;

constexpr uint8_t kBeaconMagic0 = 0x41;
constexpr uint8_t kBeaconMagic1 = 0x44;
constexpr uint8_t kBeaconVersion = 1;

#pragma pack(push, 1)
struct TagBeacon {
  uint8_t magic[2];
  uint8_t version;
  uint32_t tagId;
  uint8_t seq;
  int8_t rssi;
};

struct ReceiverAnnounce {
  uint8_t magic[2];
  uint8_t version;
  uint8_t channel;
  uint8_t reserved;
};
#pragma pack(pop)

uint8_t gBroadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
TagBeacon gBeacon{};
uint8_t gSeq = 0;
uint8_t gWifiChannel = TAG_CHANNEL;
uint32_t gLastReceiverSeenMs = 0;
uint32_t gLastChannelHopMs = 0;
uint8_t gScanChannel = 1;
int8_t gLastPromiscRssi = -127;
uint8_t gLastPromiscSrcMac[6] = {0, 0, 0, 0, 0, 0};
bool gLastPromiscHasMac = false;
uint32_t gLastPromiscMs = 0;
int8_t gLastAnnounceRssi = -127;
uint32_t gLastAnnounceRssiMs = 0;

volatile uint8_t gTargetChannel = 0;
volatile bool gHubSeenTick = false;

uint32_t gTotalOkSends = 0;
uint32_t gTotalFailSends = 0;
uint32_t gConsecutiveFailBursts = 0;
bool gIsScanning = false;

namespace {

void onPromiscuousRecv(uint8_t* buf, uint16_t len) {
  if (!buf || len < 12 + 24) {
    return;
  }
  const uint8_t* frame = buf + 12;
  memcpy(gLastPromiscSrcMac, frame + 10, sizeof(gLastPromiscSrcMac));
  gLastPromiscHasMac = true;
  gLastPromiscRssi = static_cast<int8_t>(buf[0]);
  gLastPromiscMs = millis();
}

void onEspNowSend(uint8_t* mac, uint8_t status) {
  (void)mac;
  (void)status;
}

bool updateBroadcastPeer(uint8_t channel) {
  int err = esp_now_set_peer_channel(gBroadcastMac, channel);
  if (err != 0) {
    esp_now_del_peer(gBroadcastMac);
    return esp_now_add_peer(gBroadcastMac, ESP_NOW_ROLE_SLAVE, channel, nullptr, 0) == 0;
  }
  return true;
}

void applyChannel(uint8_t channel, bool persist) {
  channel = constrain(channel, (uint8_t)1, (uint8_t)13);
  uint8_t savedCh = EEPROM.read(0);
  if (channel == gWifiChannel && (!persist || savedCh == channel)) return;

  const uint8_t prev = gWifiChannel;
  gWifiChannel = channel;

  wifi_promiscuous_enable(1);
  wifi_set_channel(gWifiChannel);
  wifi_promiscuous_enable(1);

  delay(5);
  updateBroadcastPeer(gWifiChannel);

  if (persist && savedCh != channel) {
    EEPROM.write(0, channel);
    EEPROM.commit();
    Serial.printf("[ch] %u -> %u (saved, hub locked)\n", (unsigned)prev, (unsigned)gWifiChannel);
  }
}

void IRAM_ATTR onEspNowRecv(uint8_t* mac_addr, uint8_t* data, uint8_t len) {
  if (len < 5) return;
  if (data[0] != 0x41 || data[1] != 0x52 || data[2] != 1) return;

  uint8_t announceCh = data[3];
  if (announceCh >= 1 && announceCh <= 13) {
    if (gLastPromiscHasMac && (millis() - gLastPromiscMs <= 1200) &&
        memcmp(mac_addr, gLastPromiscSrcMac, 6) == 0) {
      gLastAnnounceRssi = gLastPromiscRssi;
    } else {
      gLastAnnounceRssi = -55; // Valid fallback RSSI when announce frame is received
    }
    gLastAnnounceRssiMs = millis();
    gTargetChannel = announceCh;
    gHubSeenTick = true;
  }
}

void sendBeaconBurst() {
  bool anyOk = false;
  for (uint8_t i = 0; i < kBurstCount; ++i) {
    const uint32_t now = millis();
    gBeacon.rssi = ((now - gLastAnnounceRssiMs) <= 5000) ? gLastAnnounceRssi : -127;
    gBeacon.seq = gSeq++;
    int err = esp_now_send(gBroadcastMac, reinterpret_cast<uint8_t*>(&gBeacon), sizeof(gBeacon));
    if (err == 0) {
      anyOk = true;
      gTotalOkSends++;
    } else {
      gTotalFailSends++;
      static uint32_t lastErrLogMs = 0;
      if (millis() - lastErrLogMs >= 3000) {
        lastErrLogMs = millis();
        Serial.printf("[espnow] send err=%d seq=%u ch=%u\n", err, gBeacon.seq, gWifiChannel);
      }
    }
    if (i + 1 < kBurstCount) {
      delay(8);
    }
  }

  if (anyOk) {
    gConsecutiveFailBursts = 0;
  } else {
    gConsecutiveFailBursts++;
    if (gConsecutiveFailBursts >= 30) {
      Serial.println("[espnow] 30 consecutive failed bursts! auto-recovery");
      gConsecutiveFailBursts = 0;

      esp_now_deinit();
      delay(50);
      if (esp_now_init() == 0) {
        esp_now_register_recv_cb(onEspNowRecv);
        esp_now_register_send_cb(onEspNowSend);
        esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
        updateBroadcastPeer(gWifiChannel);
      }
    }
  }
}

void stepChannelHop() {
  const uint32_t now = millis();
  if (now < kBootSettleMs) return;
  if (gLastReceiverSeenMs != 0 && (uint32_t)(now - gLastReceiverSeenMs) < kReceiverTimeoutMs) {
    return;
  }
  gIsScanning = true;

  if ((uint32_t)(now - gLastChannelHopMs) < kChannelHopMs) return;
  gLastChannelHopMs = now;

  applyChannel(gScanChannel, false);
  if (gScanChannel >= 13) {
    gScanChannel = 1;
  } else {
    gScanChannel++;
  }
}

void stepHubSync() {
  if (gHubSeenTick) {
    gHubSeenTick = false;

    const uint32_t now = millis();
    gLastReceiverSeenMs = now;
    gIsScanning = false;

    uint8_t target = gTargetChannel;
    static uint32_t sLastAnnounceLogMs = 0;
    if (now - sLastAnnounceLogMs >= 500) {
      sLastAnnounceLogMs = now;
      Serial.printf("[tag][announce] ch=%u promiscSeen=%s promiscAge=%lu announceRssi=%d announceAge=%lu\n",
                    (unsigned)target,
                    gLastPromiscHasMac ? "yes" : "no",
                    (unsigned long)(now - gLastPromiscMs),
                    (int)gLastAnnounceRssi,
                    (unsigned long)(now - gLastAnnounceRssiMs));
    }
    if (target >= 1 && target <= 13) {
      applyChannel(target, true);
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("=== AutoDoor Lite Tag ESP8285 (Compat) ===");

  EEPROM.begin(4);
  gWifiChannel = EEPROM.read(0);
  if (gWifiChannel < 1 || gWifiChannel > 13) {
    gWifiChannel = TAG_CHANNEL;
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setPhyMode(WIFI_PHY_MODE_11N);
  delay(100);

  const uint32_t chipId = ESP.getChipId();
  gBeacon.magic[0] = kBeaconMagic0;
  gBeacon.magic[1] = kBeaconMagic1;
  gBeacon.version = kBeaconVersion;
  gBeacon.tagId = chipId;
  gBeacon.seq = 0;

  if (esp_now_init() != 0) {
    Serial.println("FATAL: esp_now_init failed");
    return;
  }

  esp_now_register_recv_cb(onEspNowRecv);
  esp_now_register_send_cb(onEspNowSend);
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  wifi_set_promiscuous_rx_cb(onPromiscuousRecv);

  wifi_promiscuous_enable(1);
  wifi_set_channel(gWifiChannel);
  wifi_promiscuous_enable(1);
  delay(5);

  if (!updateBroadcastPeer(gWifiChannel)) {
    Serial.println("FATAL: esp_now_add_peer failed");
    return;
  }

  gLastReceiverSeenMs = 0;
  gScanChannel = 1;

  int okSends = 0;
  int failSends = 0;
  for (uint8_t i = 0; i < kBootBurstCount; ++i) {
    gBeacon.seq = gSeq++;
    int err = esp_now_send(gBroadcastMac, reinterpret_cast<uint8_t*>(&gBeacon), sizeof(gBeacon));
    if (err == 0) {
      okSends++;
      gTotalOkSends++;
    } else {
      failSends++;
      gTotalFailSends++;
    }
    delay(12);
  }
  Serial.printf("[boot] first bursts ok=%d fail=%d\n", okSends, failSends);
  Serial.printf("[boot] tagId=%lu ch=%u\n", (unsigned long)gBeacon.tagId, (unsigned)gWifiChannel);
}

void loop() {
  stepHubSync();
  stepChannelHop();
  sendBeaconBurst();
  delay(kBeaconIntervalMs);
}
