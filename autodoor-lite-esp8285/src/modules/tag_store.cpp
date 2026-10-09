#include "tag_store.h"

#include <EEPROM.h>
#include <string.h>

namespace {

constexpr uint16_t kEepromSize = 4096;
constexpr uint32_t kBlobMagic = 0x4154444CUL;  // ATDL
constexpr uint16_t kBlobVersion = 1;

struct PersistBlob {
  uint32_t magic;
  uint16_t version;
  uint16_t tagCount;
  AppSettings settings;
  TagRecord tags[kMaxTags];
  uint32_t checksum;
};

PersistBlob gBlob{};
RuntimeTagState gRuntime[kMaxTags]{};

uint32_t fnv1a32(const uint8_t* data, size_t len) {
  uint32_t hash = 2166136261UL;
  for (size_t i = 0; i < len; ++i) {
    hash ^= data[i];
    hash *= 16777619UL;
  }
  return hash;
}

void resetDefaults() {
  memset(&gBlob, 0, sizeof(gBlob));
  gBlob.magic = kBlobMagic;
  gBlob.version = kBlobVersion;
  gBlob.tagCount = 0;
  gBlob.settings.espNowChannel = kDefaultEspNowChannel;
  gBlob.settings.relayPulseMs = kDefaultRelayPulseMs;
  gBlob.settings.tagSensitivity = kDefaultTagSensitivity;
  gBlob.settings.tagCloseDelayMs = kDefaultTagCloseDelayMs;
  gBlob.settings.tagAutoEnabled = 1;
  gBlob.settings.simMode = 0;
  gBlob.settings.rfOpenCode = 0;
  gBlob.settings.rfCloseCode = 0;
  gBlob.settings.rfStopCode = 0;
  memset(gRuntime, 0, sizeof(gRuntime));
}

void refreshRuntime() {
  memset(gRuntime, 0, sizeof(gRuntime));
  for (size_t i = 0; i < gBlob.tagCount && i < kMaxTags; ++i) {
    gRuntime[i].tagId = gBlob.tags[i].tagId;
    gRuntime[i].lastSeenMs = 0;
    gRuntime[i].rssi = -127;
    gRuntime[i].active = false;
  }
}

bool saveBlob() {
  gBlob.checksum = fnv1a32(reinterpret_cast<const uint8_t*>(&gBlob),
                           sizeof(PersistBlob) - sizeof(gBlob.checksum));
  EEPROM.put(0, gBlob);
  return EEPROM.commit();
}

int findIndexByTagId(uint32_t tagId) {
  for (size_t i = 0; i < gBlob.tagCount; ++i) {
    if (gBlob.tags[i].tagId == tagId) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

}  // namespace

namespace TagStore {

bool begin() {
  EEPROM.begin(kEepromSize);
  EEPROM.get(0, gBlob);
  const uint32_t checksum = fnv1a32(reinterpret_cast<const uint8_t*>(&gBlob),
                                    sizeof(PersistBlob) - sizeof(gBlob.checksum));
  if (gBlob.magic != kBlobMagic || gBlob.version != kBlobVersion || gBlob.tagCount > kMaxTags ||
      gBlob.checksum != checksum) {
    resetDefaults();
    if (!saveBlob()) {
      return false;
    }
  }
  refreshRuntime();
  return true;
}

const AppSettings& settings() { return gBlob.settings; }

bool updateSettings(const AppSettings& s) {
  gBlob.settings = s;
  if (gBlob.settings.espNowChannel < 1 || gBlob.settings.espNowChannel > 13) {
    gBlob.settings.espNowChannel = kDefaultEspNowChannel;
  }
  if (gBlob.settings.relayPulseMs < 100 || gBlob.settings.relayPulseMs > 5000) {
    gBlob.settings.relayPulseMs = kDefaultRelayPulseMs;
  }
  if (gBlob.settings.tagSensitivity < kMinTagSensitivity ||
      gBlob.settings.tagSensitivity > kMaxTagSensitivity) {
    gBlob.settings.tagSensitivity = kDefaultTagSensitivity;
  }
  if (gBlob.settings.tagCloseDelayMs > kMaxTagCloseDelayMs) {
    gBlob.settings.tagCloseDelayMs = kDefaultTagCloseDelayMs;
  }
  gBlob.settings.tagAutoEnabled = gBlob.settings.tagAutoEnabled ? 1 : 0;
  gBlob.settings.simMode = gBlob.settings.simMode ? 1 : 0;
  return saveBlob();
}

size_t tagCount() { return gBlob.tagCount; }

const TagRecord* tagAt(size_t idx) {
  if (idx >= gBlob.tagCount) {
    return nullptr;
  }
  return &gBlob.tags[idx];
}

bool addTag(uint32_t tagId, const char* name, uint16_t holdMs) {
  if (tagId == 0 || gBlob.tagCount >= kMaxTags || findIndexByTagId(tagId) >= 0) {
    return false;
  }
  TagRecord& t = gBlob.tags[gBlob.tagCount++];
  t.tagId = tagId;
  t.enabled = 1;
  t.holdMs = constrain(holdMs, kTagMinHoldMs, kTagMaxHoldMs);
  t.closeDelayMs = kDefaultTagCloseDelayMs;
  t.tagRssiMin = -100;
  t.tagPassRssi = -90;
  t.tagPassCloseDelayMs = 3000;
  memset(t.name, 0, sizeof(t.name));
  if (name && name[0]) {
    strncpy(t.name, name, sizeof(t.name) - 1);
  } else {
    snprintf(t.name, sizeof(t.name), "TAG-%08lX", static_cast<unsigned long>(tagId));
  }
  refreshRuntime();
  return saveBlob();
}

bool removeTag(uint32_t tagId) {
  const int idx = findIndexByTagId(tagId);
  if (idx < 0) {
    return false;
  }
  for (size_t i = static_cast<size_t>(idx); i + 1 < gBlob.tagCount; ++i) {
    gBlob.tags[i] = gBlob.tags[i + 1];
  }
  --gBlob.tagCount;
  refreshRuntime();
  return saveBlob();
}

bool setTag(uint32_t tagId,
            const char* name,
            uint16_t holdMs,
            uint16_t closeDelayMs,
            int8_t tagRssiMin,
            int8_t tagPassRssi,
            uint16_t tagPassCloseDelayMs,
            bool enabled) {
  const int idx = findIndexByTagId(tagId);
  if (idx < 0) {
    return false;
  }
  TagRecord& t = gBlob.tags[idx];
  t.holdMs = constrain(holdMs, kTagMinHoldMs, kTagMaxHoldMs);
  t.closeDelayMs = constrain(closeDelayMs, (uint16_t)0, kMaxTagCloseDelayMs);
  t.tagRssiMin = constrain(tagRssiMin, (int8_t)-127, (int8_t)0);
  t.tagPassRssi = constrain(tagPassRssi, (int8_t)-127, (int8_t)0);
  t.tagPassCloseDelayMs = constrain(tagPassCloseDelayMs, (uint16_t)0, kMaxTagCloseDelayMs);
  t.enabled = enabled ? 1 : 0;
  if (name && name[0]) {
    memset(t.name, 0, sizeof(t.name));
    strncpy(t.name, name, sizeof(t.name) - 1);
  }
  return saveBlob();
}

bool tagExists(uint32_t tagId) { return findIndexByTagId(tagId) >= 0; }

void markSeen(uint32_t tagId, uint32_t nowMs, uint8_t sensitivity, const uint8_t* sourceMac, int8_t rssi) {
  const int idx = findIndexByTagId(tagId);
  if (idx < 0 || !gBlob.tags[idx].enabled) {
    return;
  }
  if (sensitivity < kMinTagSensitivity) sensitivity = kMinTagSensitivity;
  if (sensitivity > kMaxTagSensitivity) sensitivity = kMaxTagSensitivity;
  const TagRecord& tag = gBlob.tags[idx];
  gRuntime[idx].tagId = tagId;
  gRuntime[idx].lastSeenMs = nowMs;
  gRuntime[idx].rssi = rssi;
  if (sourceMac) {
    memcpy(gRuntime[idx].sourceMac, sourceMac, sizeof(gRuntime[idx].sourceMac));
  }
  if (rssi != -127 && rssi < tag.tagRssiMin) {
    gRuntime[idx].seenStreak = 0;
    return;
  }
  if (gRuntime[idx].seenStreak < 255) {
    gRuntime[idx].seenStreak++;
  }
  if (gRuntime[idx].seenStreak >= sensitivity) {
    gRuntime[idx].active = true;
  }
}

void stepAging(uint32_t nowMs) {
  for (size_t i = 0; i < gBlob.tagCount; ++i) {
    if (!gBlob.tags[i].enabled || !gRuntime[i].active) {
      continue;
    }
    const uint32_t elapsed = nowMs - gRuntime[i].lastSeenMs;
    const uint16_t closeDelay = gBlob.tags[i].closeDelayMs > 0 ? gBlob.tags[i].closeDelayMs : gBlob.settings.tagCloseDelayMs;
    if (elapsed > (gRuntime[i].rssi >= gBlob.tags[i].tagPassRssi ? gBlob.tags[i].tagPassCloseDelayMs : closeDelay)) {
      gRuntime[i].active = false;
      gRuntime[i].seenStreak = 0;
    }
  }
}

bool anyActiveTags() {
  for (size_t i = 0; i < gBlob.tagCount; ++i) {
    if (gBlob.tags[i].enabled && gRuntime[i].active) {
      return true;
    }
  }
  return false;
}

const RuntimeTagState* runtimeAt(size_t idx) {
  if (idx >= gBlob.tagCount) {
    return nullptr;
  }
  return &gRuntime[idx];
}

String tagsToJson() {
  String out = "[";
  for (size_t i = 0; i < gBlob.tagCount; ++i) {
    if (i) {
      out += ',';
    }
    out += "{\"tagId\":";
    out += String(gBlob.tags[i].tagId);
    out += ",\"name\":\"";
    out += String(gBlob.tags[i].name);
    out += "\",\"holdMs\":";
    out += String(gBlob.tags[i].holdMs);
    out += ",\"closeDelayMs\":";
    out += String(gBlob.tags[i].closeDelayMs);
    out += ",\"tagRssiMin\":";
    out += String(gBlob.tags[i].tagRssiMin);
    out += ",\"tagPassRssi\":";
    out += String(gBlob.tags[i].tagPassRssi);
    out += ",\"tagPassCloseDelayMs\":";
    out += String(gBlob.tags[i].tagPassCloseDelayMs);
    out += ",\"enabled\":";
    out += String(gBlob.tags[i].enabled ? "true" : "false");
    out += ",\"active\":";
    out += String(gRuntime[i].active ? "true" : "false");
    out += ",\"rssi\":";
    out += String(gRuntime[i].rssi);
    out += '}';
  }
  out += "]";
  return out;
}

}  // namespace TagStore
