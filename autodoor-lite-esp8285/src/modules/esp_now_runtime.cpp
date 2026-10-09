#include "esp_now_runtime.h"

#include <ESP8266WiFi.h>

extern "C" {
#include <espnow.h>
#include <user_interface.h>
}

#include "app/types.h"

namespace {

EspNowRuntime::TagSeenCallback gTagSeenCb = nullptr;
uint8_t gCurrentChannel = 1;
uint32_t gLastAnnounceMs = 0;
uint8_t gAnnounceSeq = 0;
int8_t gLastPromiscRssi = -127;
uint32_t gLastPromiscMs = 0;
uint8_t gLastPromiscSrcMac[6] = {0, 0, 0, 0, 0, 0};
bool gLastPromiscHasMac = false;

const char* macToStr(const uint8_t* mac, char* out, size_t outLen) {
  if (!mac || !out || outLen < 18) {
    return "??:??:??:??:??:??";
  }
  snprintf(out, outLen, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4],
           mac[5]);
  return out;
}

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

void onEspNowRecv(uint8_t* mac, uint8_t* data, uint8_t len) {
  if (!data || len < static_cast<uint8_t>(sizeof(TagBeacon))) {
    return;
  }

  const TagBeacon* b = reinterpret_cast<const TagBeacon*>(data);
  if (b->magic0 != kTagBeaconMagic0 || b->magic1 != kTagBeaconMagic1 || b->tagId == 0) {
    return;
  }
  const bool isV1 = (b->version == kTagBeaconVersionV1 && len >= static_cast<uint8_t>(sizeof(TagBeacon)));
  const bool isV2 = (b->version == kTagBeaconVersionV2 && len >= static_cast<uint8_t>(sizeof(TagBeaconV2)));
  if (!isV1 && !isV2) {
    return;
  }

  if (gTagSeenCb) {
    int8_t rssi = -127;
    const uint32_t now = millis();
    int8_t payloadMetricRssi = -127;
    if (isV2) {
      const TagBeaconV2* b2 = reinterpret_cast<const TagBeaconV2*>(data);
      payloadMetricRssi = b2->metricRssi;
      if (payloadMetricRssi < -127) payloadMetricRssi = -127;
      if (payloadMetricRssi > 0) payloadMetricRssi = 0;
    }

    if (payloadMetricRssi > -127) {
      rssi = payloadMetricRssi;
    }

    const bool promiscFresh = gLastPromiscHasMac && (now - gLastPromiscMs <= 1200);
    const bool promiscMacMatch = promiscFresh && (memcmp(mac, gLastPromiscSrcMac, 6) == 0);
    if (rssi <= -127 && promiscMacMatch) {
      rssi = gLastPromiscRssi;
    }

    static uint32_t sLastRxLogMs = 0;
    if (now - sLastRxLogMs >= 250) {
      sLastRxLogMs = now;
      char srcMacStr[18];
      char promiscMacStr[18];
      Serial.printf("[espnow][rx] tagId=%lu seq=%u ver=%u len=%u src=%s rssi=%d payloadRssi=%d promiscFresh=%s promiscMatch=%s promiscSrc=%s age=%lu\n",
                    static_cast<unsigned long>(b->tagId),
                    static_cast<unsigned>(b->seq),
            static_cast<unsigned>(b->version),
                    static_cast<unsigned>(len),
                    macToStr(mac, srcMacStr, sizeof(srcMacStr)),
                    static_cast<int>(rssi),
            static_cast<int>(payloadMetricRssi),
                    promiscFresh ? "yes" : "no",
                    promiscMacMatch ? "yes" : "no",
                    macToStr(gLastPromiscSrcMac, promiscMacStr, sizeof(promiscMacStr)),
                    static_cast<unsigned long>(now - gLastPromiscMs));
    }

    static uint32_t sLastPromiscWarnMs = 0;
    if (rssi <= -127 && !promiscFresh && (now - sLastPromiscWarnMs >= 3000)) {
      sLastPromiscWarnMs = now;
      Serial.println("[espnow][warn] promisc RSSI unavailable, report as unknown (-127)");
    }

    gTagSeenCb(b->tagId, rssi, mac);
  }
}

void sendAnnounce() {
  ReceiverAnnounce ann{};
  ann.magic0 = kReceiverAnnounceMagic0;
  ann.magic1 = kReceiverAnnounceMagic1;
  ann.version = kReceiverAnnounceVersion;
  ann.channel = gCurrentChannel;
  ann.seq = gAnnounceSeq++;

  uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_send(broadcastMac, reinterpret_cast<uint8_t*>(&ann), sizeof(ann));
}

}  // namespace

namespace EspNowRuntime {

bool begin(uint8_t channel, TagSeenCallback cb) {
  gTagSeenCb = cb;

  if (channel < 1 || channel > 13) {
    channel = 1;
  }
  gCurrentChannel = channel;
  wifi_set_channel(channel);

  if (esp_now_init() != 0) {
    return false;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(onEspNowRecv);
  wifi_set_promiscuous_rx_cb(onPromiscuousRecv);
  wifi_promiscuous_enable(1);
  gLastAnnounceMs = 0;
  gAnnounceSeq = 0;
  return true;
}

void setChannel(uint8_t channel) {
  if (channel < 1 || channel > 13) {
    return;
  }
  gCurrentChannel = channel;
  wifi_set_channel(channel);
}

uint8_t currentChannel() { return gCurrentChannel; }

uint8_t autoSelectChannel() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(20);

  int found = WiFi.scanNetworks();
  if (found <= 0) {
    return gCurrentChannel >= 1 && gCurrentChannel <= 13 ? gCurrentChannel : 9;
  }

  float score[14] = {0.0f};
  for (int i = 0; i < found; ++i) {
    int ch = WiFi.channel(i);
    if (ch < 1 || ch > 13) {
      continue;
    }
    int rssi = WiFi.RSSI(i);
    float p = powf(10.0f, (rssi + 100.0f) / 10.0f);
    score[ch] += p;
  }

  uint8_t best = 1;
  float bestScore = score[1];
  for (uint8_t ch = 2; ch <= 13; ++ch) {
    if (score[ch] < bestScore) {
      bestScore = score[ch];
      best = ch;
    }
  }
  WiFi.scanDelete();
  return best;
}

void step() {
  const uint32_t now = millis();
  if ((now - gLastAnnounceMs) >= 1200) {
    gLastAnnounceMs = now;
    sendAnnounce();
  }
}

}  // namespace EspNowRuntime
