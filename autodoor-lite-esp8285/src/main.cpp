#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>

#include "app/config.h"
#include "app/types.h"
#include "modules/ap_config.h"
#include "modules/door.h"
#include "modules/esp_now_runtime.h"
#include "modules/health.h"
#include "modules/http_api.h"
#include "modules/rf433_runtime.h"
#include "modules/tag_store.h"

namespace {

ESP8266WebServer gServer(80);
bool gConfigMode = false;
bool gPrevAnyTag = false;
uint32_t gLastApLogMs = 0;
uint8_t gAppliedEspNowChannel = 0;
bool gLearnActive = false;
uint32_t gLearnDeadlineMs = 0;
uint16_t gLearnHoldMs = 3000;
uint32_t gLearnLastTagId = 0;
bool gLearnAdded = false;
uint32_t gPendingCloseAtMs = 0;
uint32_t gLastRssiDebugMs = 0;

bool startLearn(uint16_t holdMs, uint16_t timeoutSec) {
  gLearnHoldMs = constrain(holdMs, kTagMinHoldMs, kTagMaxHoldMs);
  uint16_t to = timeoutSec;
  if (to < 5) to = 5;
  if (to > 120) to = 120;
  gLearnDeadlineMs = millis() + static_cast<uint32_t>(to) * 1000UL;
  gLearnLastTagId = 0;
  gLearnAdded = false;
  gLearnActive = true;
  return true;
}

void stopLearn() {
  gLearnActive = false;
}

String learnStatusJson() {
  String out = "{";
  out += "\"active\":";
  out += (gLearnActive ? "true" : "false");
  out += ",\"lastTagId\":";
  out += String(gLearnLastTagId);
  out += ",\"added\":";
  out += (gLearnAdded ? "true" : "false");
  if (gLearnActive) {
    const int32_t leftMs = static_cast<int32_t>(gLearnDeadlineMs - millis());
    out += ",\"leftSec\":";
    out += String(leftMs > 0 ? leftMs / 1000 : 0);
  }
  out += "}";
  return out;
}

uint8_t autoChannelNow() {
  uint8_t ch = EspNowRuntime::autoSelectChannel();
  if (ch < 1 || ch > 13) {
    ch = kDefaultEspNowChannel;
  }

  AppSettings s = TagStore::settings();
  s.espNowChannel = ch;
  TagStore::updateSettings(s);

  EspNowRuntime::setChannel(ch);
  gAppliedEspNowChannel = ch;
  if (gConfigMode) {
    WiFi.softAPdisconnect(true);
    ApConfig::beginConfigAP(ch);
  }
  return ch;
}

void onTagSeen(uint32_t tagId, int8_t rssi, const uint8_t* sourceMac) {
  gLearnLastTagId = tagId;
  if (gLearnActive && !TagStore::tagExists(tagId)) {
    char name[20];
    snprintf(name, sizeof(name), "TAG-%08lX", static_cast<unsigned long>(tagId));
    if (TagStore::addTag(tagId, name, gLearnHoldMs)) {
      gLearnAdded = true;
      gLearnActive = false;
    }
  } else if (gLearnActive && TagStore::tagExists(tagId)) {
    gLearnAdded = true;
    gLearnActive = false;
  }
  const uint8_t sensitivity = TagStore::settings().tagSensitivity;
  TagStore::markSeen(tagId, millis(), sensitivity, sourceMac, rssi);
}

void onRfCode(uint32_t code) {
  const AppSettings& s = TagStore::settings();
  if (code == s.rfOpenCode && s.rfOpenCode != 0) {
    Door::open();
  } else if (code == s.rfCloseCode && s.rfCloseCode != 0) {
    Door::close();
  } else if (code == s.rfStopCode && s.rfStopCode != 0) {
    Door::stop();
  }
}

void startConfigMode(uint8_t channel) {
  if (gConfigMode) return;
  if (ApConfig::beginConfigAP(channel)) {
    gConfigMode = true;
    Serial.print("[ap] started ssid=");
    Serial.print(ApConfig::apSsid());
    Serial.print(" ip=");
    Serial.println(WiFi.softAPIP());
    HttpApi::begin(gServer, startLearn, stopLearn, learnStatusJson, autoChannelNow);
    gServer.begin();
  } else {
    Serial.println("[ap] failed to start");
    gConfigMode = false;
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(150);
  Serial.println();
  Serial.println("=== AutoDoor Lite ESP8285 (Offline) ===");

  if (!TagStore::begin()) {
    Serial.println("[fatal] TagStore init failed");
    delay(3000);
    ESP.restart();
  }

  autoChannelNow();
  const AppSettings& cfg = TagStore::settings();

  Door::begin(kPinDoorOpen, kPinDoorClose, cfg.relayPulseMs);
  Health::begin(kDailyRestartMs);

  startConfigMode(cfg.espNowChannel);

  if (!EspNowRuntime::begin(cfg.espNowChannel, onTagSeen)) {
    Serial.println("[warn] esp-now init failed");
  }
  gAppliedEspNowChannel = cfg.espNowChannel;

  Rf433Runtime::begin(kPinRf433Rx, onRfCode);

  Serial.printf("[boot] channel=%u pulseMs=%u tags=%u configMode=%s\n",
                static_cast<unsigned>(cfg.espNowChannel),
                static_cast<unsigned>(cfg.relayPulseMs),
                static_cast<unsigned>(TagStore::tagCount()),
                gConfigMode ? "true" : "false");
}

void loop() {
  const uint32_t now = millis();

  Health::step();
  Door::step();

  EspNowRuntime::step();
  Rf433Runtime::step();

  TagStore::stepAging(now);
  const uint8_t desiredChannel = TagStore::settings().espNowChannel;
  if (desiredChannel >= 1 && desiredChannel <= 13 && desiredChannel != gAppliedEspNowChannel) {
    EspNowRuntime::setChannel(desiredChannel);
    gAppliedEspNowChannel = desiredChannel;
    if (gConfigMode) {
      WiFi.softAPdisconnect(true);
      ApConfig::beginConfigAP(desiredChannel);
    }
  }

  const AppSettings& liveSettings = TagStore::settings();
  const bool anyTag = liveSettings.tagAutoEnabled && TagStore::anyActiveTags();
  if (anyTag && !gPrevAnyTag) {
    gPendingCloseAtMs = 0;
    if (!liveSettings.simMode) {
      Door::open();
    }
  }
  if (!anyTag && gPrevAnyTag) {
    const uint16_t delayMs = liveSettings.tagCloseDelayMs;
    gPendingCloseAtMs = millis() + delayMs;
  }
  if (!anyTag && gPendingCloseAtMs != 0 && static_cast<int32_t>(millis() - gPendingCloseAtMs) >= 0) {
    gPendingCloseAtMs = 0;
    if (!liveSettings.simMode) {
      Door::close();
    }
  }
  if (anyTag) {
    gPendingCloseAtMs = 0;
  }
  gPrevAnyTag = anyTag;

  if (gConfigMode) {
    if (gLearnActive && static_cast<int32_t>(millis() - gLearnDeadlineMs) >= 0) {
      gLearnActive = false;
    }
    gServer.handleClient();
    if ((now - gLastApLogMs) > 15000) {
      gLastApLogMs = now;
      Serial.printf("[ap] clients=%u freeHeap=%u\n", WiFi.softAPgetStationNum(), ESP.getFreeHeap());
    }
  }

  if ((now - gLastRssiDebugMs) >= 2000) {
    gLastRssiDebugMs = now;
    bool printed = false;
    for (size_t i = 0; i < TagStore::tagCount(); ++i) {
      const TagRecord* tag = TagStore::tagAt(i);
      const RuntimeTagState* rt = TagStore::runtimeAt(i);
      if (!tag || !rt) {
        continue;
      }
      printed = true;
      const uint32_t ageMs = (rt->lastSeenMs == 0) ? 0xFFFFFFFFUL : (now - rt->lastSeenMs);
      Serial.printf("[rssi][runtime] tagId=%lu enabled=%u active=%u rssi=%d seenStreak=%u ageMs=%lu\n",
                    static_cast<unsigned long>(tag->tagId),
                    static_cast<unsigned>(tag->enabled),
                    rt->active ? 1u : 0u,
                    static_cast<int>(rt->rssi),
                    static_cast<unsigned>(rt->seenStreak),
                    static_cast<unsigned long>(ageMs));
    }
    if (!printed) {
      Serial.println("[rssi][runtime] no tags configured");
    }
  }

  delay(1);
}
