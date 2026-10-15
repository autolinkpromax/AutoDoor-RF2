#include "TagRegistry.h"

#include "app/config.h"
#include "app/globals.h"
#include "modules/door.h"
#include "modules/esp_now_runtime.h"

#include <ArduinoJson.h>
#include <esp_random.h>

// Global instance for Composition Root / backward compatibility
TagRegistry gTagRegistry;
SemaphoreHandle_t gTagStoreMutex = nullptr;

// Reference aliases ensuring transparent access for any code using legacy globals
std::vector<Tag>& gTags = gTagRegistry.tags();
bool& gTagOpenSuppressed = gTagRegistry.suppressionRef();
std::vector<String>& gTagBaselineActiveIds = gTagRegistry.baselineActiveIds();

namespace {

constexpr uint32_t kTagsFlushIdleMs = 500;

#pragma pack(push, 1)
struct TagDiskEntryV1 {
  uint32_t id;                 // 4 bytes: 32-bit random number (converted to/from 8-char hex string)
  char name[80];               // 80 bytes: up to 26 UTF-8 Thai characters (or 79 ASCII)
  uint8_t mac[6];              // 6 bytes: binary MAC address
  uint8_t isRf;                // 1 byte: 1 for RF, 0 for ESP-NOW
  uint32_t tagId;              // 4 bytes
  uint32_t tagHoldMs;          // 4 bytes
  uint32_t tagCloseDelayMs;    // 4 bytes
  int8_t tagRssiMin;           // 1 byte
  uint8_t rfRole;              // 1 byte: 0 = Auto, 1 = HoldOpen, 2 = Close, 3 = Stop
  uint32_t tagOpenDurationMs;  // 4 bytes: 0 = full open default, >0 = partial open (e.g. motorcycle/pedestrian)
  uint32_t tagSafetyCloseDelayMs; // 4 bytes
};

struct TagDiskEntryV2 {
  uint32_t id;                 // 4 bytes: 32-bit random number (converted to/from 8-char hex string)
  char name[80];               // 80 bytes: up to 26 UTF-8 Thai characters (or 79 ASCII)
  uint8_t mac[6];              // 6 bytes: binary MAC address
  uint8_t isRf;                // 1 byte: 1 for RF, 0 for ESP-NOW
  uint32_t tagId;              // 4 bytes
  uint32_t tagHoldMs;          // 4 bytes
  uint32_t tagCloseDelayMs;    // 4 bytes
  int8_t tagRssiMin;           // 1 byte
  uint8_t rfRole;              // 1 byte: 0 = Auto, 1 = HoldOpen, 2 = Close, 3 = Stop
  uint32_t tagOpenDurationMs;  // 4 bytes: 0 = full open default, >0 = partial open (e.g. motorcycle/pedestrian)
  uint32_t tagSafetyCloseDelayMs; // 4 bytes
  char icon[16];               // 16 bytes: "auto", "suv", "sedan", "pickup", "bike", "scooter", "van", "rf", "bicycle", "person"
};

struct TagDiskEntryV3 {
  uint32_t id;                 // 4 bytes: 32-bit random number (converted to/from 8-char hex string)
  char name[80];               // 80 bytes: up to 26 UTF-8 Thai characters (or 79 ASCII)
  uint8_t mac[6];              // 6 bytes: binary MAC address
  uint8_t isRf;                // 1 byte: 1 for RF, 0 for ESP-NOW
  uint32_t tagId;              // 4 bytes
  uint32_t tagHoldMs;          // 4 bytes
  uint32_t tagCloseDelayMs;    // 4 bytes
  int8_t tagRssiMin;           // 1 byte
  uint8_t rfRole;              // 1 byte: 0 = Auto, 1 = HoldOpen, 2 = Close, 3 = Stop
  uint32_t tagOpenDurationMs;  // 4 bytes: 0 = full open default, >0 = partial open (e.g. motorcycle/pedestrian)
  uint32_t tagSafetyCloseDelayMs; // 4 bytes
  char icon[16];               // 16 bytes: "auto", "suv", "sedan", "pickup", "bike", "scooter", "van", "rf", "bicycle", "person"
  uint8_t scheduleEnabled;     // 1 byte: 0 = disabled (24/7), 1 = enabled
  uint16_t scheduleStartMin;   // 2 bytes: minute of day (0..1439)
  uint16_t scheduleEndMin;     // 2 bytes: minute of day (0..1439)
};

struct TagDiskEntryV4 {
  uint32_t id;                 // 4 bytes: 32-bit random number (converted to/from 8-char hex string)
  char name[80];               // 80 bytes: up to 26 UTF-8 Thai characters (or 79 ASCII)
  uint8_t mac[6];              // 6 bytes: binary MAC address
  uint8_t isRf;                // 1 byte: 1 for RF, 0 for ESP-NOW
  uint32_t tagId;              // 4 bytes
  uint32_t tagHoldMs;          // 4 bytes
  uint32_t tagCloseDelayMs;    // 4 bytes
  int8_t tagRssiMin;           // 1 byte
  uint8_t rfRole;              // 1 byte: 0 = Auto, 1 = HoldOpen, 2 = Close, 3 = Stop
  uint32_t tagOpenDurationMs;  // 4 bytes: 0 = full open default, >0 = partial open (e.g. motorcycle/pedestrian)
  uint32_t tagSafetyCloseDelayMs; // 4 bytes
  char icon[16];               // 16 bytes: "auto", "suv", "sedan", "pickup", "bike", "scooter", "van", "rf", "bicycle", "person"
  uint8_t scheduleEnabled;     // 1 byte: 0 = disabled (24/7), 1 = enabled
  uint16_t scheduleStartMin;   // 2 bytes: minute of day (0..1439)
  uint16_t scheduleEndMin;     // 2 bytes: minute of day (0..1439)
  uint8_t webhookEnabled;      // 1 byte: 0 = disabled, 1 = enabled
  char webhookUrl[96];         // 96 bytes: custom topic or URL
};

struct TagDiskEntryV5 {
  uint32_t id;                 // 4 bytes: 32-bit random number (converted to/from 8-char hex string)
  char name[80];               // 80 bytes: up to 26 UTF-8 Thai characters (or 79 ASCII)
  uint8_t mac[6];              // 6 bytes: binary MAC address
  uint8_t isRf;                // 1 byte: 1 for RF, 0 for ESP-NOW
  uint32_t tagId;              // 4 bytes
  uint32_t tagHoldMs;          // 4 bytes
  uint32_t tagCloseDelayMs;    // 4 bytes
  int8_t tagRssiMin;           // 1 byte
  uint8_t rfRole;              // 1 byte: 0 = Auto, 1 = HoldOpen, 2 = Close, 3 = Stop
  uint32_t tagOpenDurationMs;  // 4 bytes: 0 = full open default, >0 = partial open (e.g. motorcycle/pedestrian)
  uint32_t tagSafetyCloseDelayMs; // 4 bytes
  char icon[16];               // 16 bytes: "auto", "suv", "sedan", "pickup", "bike", "scooter", "van", "rf", "bicycle", "person"
  uint8_t scheduleEnabled;     // 1 byte: 0 = disabled (24/7), 1 = enabled
  uint16_t scheduleStartMin;   // 2 bytes: minute of day (0..1439)
  uint16_t scheduleEndMin;     // 2 bytes: minute of day (0..1439)
  uint8_t webhookEnabled;      // 1 byte: 0 = disabled, 1 = enabled
  uint8_t webhookOpen;         // 1 byte: 0 = disabled, 1 = enabled
  uint8_t webhookClose;        // 1 byte: 0 = disabled, 1 = enabled
  char webhookUrl[96];         // 96 bytes: custom topic or URL
};

struct TagDiskEntry {
  uint32_t id;                 // 4 bytes: 32-bit random number (converted to/from 8-char hex string)
  char name[80];               // 80 bytes: up to 26 UTF-8 Thai characters (or 79 ASCII)
  uint8_t mac[6];              // 6 bytes: binary MAC address
  uint8_t isRf;                // 1 byte: 1 for RF, 0 for ESP-NOW
  uint32_t tagId;              // 4 bytes
  uint32_t tagHoldMs;          // 4 bytes
  uint32_t tagCloseDelayMs;    // 4 bytes
  int8_t tagRssiMin;           // 1 byte
  uint8_t rfRole;              // 1 byte: 0 = Auto, 1 = HoldOpen, 2 = Close, 3 = Stop
  uint32_t tagOpenDurationMs;  // 4 bytes: 0 = full open default, >0 = partial open (e.g. motorcycle/pedestrian)
  uint32_t tagSafetyCloseDelayMs; // 4 bytes
  char icon[16];               // 16 bytes: "auto", "suv", "sedan", "pickup", "bike", "scooter", "van", "rf", "bicycle", "person"
  uint8_t scheduleEnabled;     // 1 byte: 0 = disabled (24/7), 1 = enabled
  uint16_t scheduleStartMin;   // 2 bytes: minute of day (0..1439)
  uint16_t scheduleEndMin;     // 2 bytes: minute of day (0..1439)
  uint8_t webhookEnabled;      // 1 byte: 0 = disabled, 1 = enabled
  uint8_t webhookOpen;         // 1 byte: 0 = disabled, 1 = enabled
  uint8_t webhookClosing;      // 1 byte: 0 = disabled, 1 = enabled
  uint8_t webhookClose;        // 1 byte: 0 = disabled, 1 = enabled
  char webhookUrl[96];         // 96 bytes: custom topic or URL
};
#pragma pack(pop)

uint32_t parseId(const String& idStr) {
  char* endptr = nullptr;
  uint32_t val = (uint32_t)strtoul(idStr.c_str(), &endptr, 16);
  if (val == 0 && idStr != "00000000" && idStr != "0") {
    val = 0;
    for (char c : idStr) {
      val = val * 33 + c;
    }
  }
  return val;
}

inline uint64_t macToKey(const uint8_t mac[6]) {
  uint64_t k = 0;
  memcpy(&k, mac, 6);
  return k;
}

}  // namespace

// ============================================================================
// TagStoreLock RAII Implementation
// ============================================================================

namespace {
static uint32_t sConsecutiveLockTimeouts = 0;
constexpr uint32_t kMaxConsecutiveLockTimeouts = 5;
}  // namespace

TagStoreLock::TagStoreLock(uint32_t waitMs) {
  gTagRegistry.init();
  if (gTagStoreMutex != nullptr) {
    locked = (xSemaphoreTakeRecursive(gTagStoreMutex, pdMS_TO_TICKS(waitMs)) == pdTRUE);
    if (!locked) {
      sConsecutiveLockTimeouts++;
      Serial.printf("[tag_store] WARNING: TagStoreLock timeout (%lu ms)! Consecutive=%u\n",
                    (unsigned long)waitMs, (unsigned)sConsecutiveLockTimeouts);
      if (sConsecutiveLockTimeouts >= kMaxConsecutiveLockTimeouts) {
        Serial.println("[tag_store] CRITICAL: TagStoreLock deadlock detected (5 consecutive timeouts)! Forcing restart...");
        delay(100);
        ESP.restart();
      }
    } else {
      sConsecutiveLockTimeouts = 0;
    }
  }
}

TagStoreLock::~TagStoreLock() {
  if (locked && gTagStoreMutex != nullptr) {
    xSemaphoreGiveRecursive(gTagStoreMutex);
    locked = false;
  }
}

// ============================================================================
// TagRegistry Implementation
// ============================================================================

TagRegistry::TagRegistry() {
  _tags.reserve(32);
  for (size_t s = 0; s < kHashSlots; ++s) {
    _rfTable[s].tagIdx = 0xFFFF;
    _rfTable[s].code = 0;
    _macTable[s].tagIdx = 0xFFFF;
    _macTable[s].macLow = 0;
    _macTable[s].macHigh = 0;
    _idTable[s].tagIdx = 0xFFFF;
    _idTable[s].idVal = 0;
  }
}

TagRegistry::~TagRegistry() {
  if (_mutex != nullptr) {
    vSemaphoreDelete(_mutex);
    _mutex = nullptr;
    gTagStoreMutex = nullptr;
  }
}

void TagRegistry::init() {
  if (_mutex == nullptr) {
    _mutex = xSemaphoreCreateRecursiveMutex();
    gTagStoreMutex = _mutex;
  }
}

void TagRegistry::insertRfSlot(uint32_t code, uint16_t tagIdx) {
  if (code == 0) return;
  uint16_t idx = hashFib32(code);
  for (size_t probe = 0; probe < kHashSlots; ++probe) {
    if (_rfTable[idx].tagIdx == 0xFFFF || _rfTable[idx].code == code) {
      _rfTable[idx].code = code;
      _rfTable[idx].tagIdx = tagIdx;
      return;
    }
    idx = (idx + 1) & kHashMask;
  }
}

void TagRegistry::insertMacSlot(const uint8_t mac[6], uint16_t tagIdx) {
  if (!mac) return;
  const uint16_t macHigh = ((uint16_t)mac[0] << 8) | mac[1];
  const uint32_t macLow = ((uint32_t)mac[2] << 24) | ((uint32_t)mac[3] << 16) | ((uint32_t)mac[4] << 8) | mac[5];
  uint16_t idx = hashFibMac(macHigh, macLow);
  for (size_t probe = 0; probe < kHashSlots; ++probe) {
    if (_macTable[idx].tagIdx == 0xFFFF || (_macTable[idx].macLow == macLow && _macTable[idx].macHigh == macHigh)) {
      _macTable[idx].macLow = macLow;
      _macTable[idx].macHigh = macHigh;
      _macTable[idx].tagIdx = tagIdx;
      return;
    }
    idx = (idx + 1) & kHashMask;
  }
}

void TagRegistry::insertIdSlot(uint32_t idVal, uint16_t tagIdx) {
  // Note: idVal == 0 is a valid (rare) id; tagIdx == 0xFFFF is the empty sentinel
  uint16_t idx = hashFib32(idVal);
  for (size_t probe = 0; probe < kHashSlots; ++probe) {
    if (_idTable[idx].tagIdx == 0xFFFF || _idTable[idx].idVal == idVal) {
      _idTable[idx].idVal = idVal;
      _idTable[idx].tagIdx = tagIdx;
      return;
    }
    idx = (idx + 1) & kHashMask;
  }
}

void TagRegistry::rebuildIndexes() {
  for (size_t s = 0; s < kHashSlots; ++s) {
    _rfTable[s].tagIdx = 0xFFFF;
    _rfTable[s].code = 0;
    _macTable[s].tagIdx = 0xFFFF;
    _macTable[s].macLow = 0;
    _macTable[s].macHigh = 0;
    _idTable[s].tagIdx = 0xFFFF;
    _idTable[s].idVal = 0;
  }

  const size_t total = _tags.size();
  for (size_t i = 0; i < total; ++i) {
    auto& t = _tags[i];
    uint16_t tagIdx = (uint16_t)i;

    uint32_t idVal = parseId(t.id);
    insertIdSlot(idVal, tagIdx);

    insertMacSlot(t.macBin, tagIdx);

    if (t.tagId != 0) {
      insertRfSlot(t.tagId, tagIdx);
      if ((t.tagId & 0x00FFFFFF) != t.tagId) {
        insertRfSlot(t.tagId & 0x00FFFFFF, tagIdx);
      }
    }

    // Also register alternative RF pseudo-MAC encodings for RF tags
    if (isRf(t)) {
      uint32_t code = t.tagId;
      if (code != 0) {
        uint8_t addr5[6] = {'R', (uint8_t)(code >> 24), (uint8_t)(code >> 16), (uint8_t)(code >> 8), (uint8_t)code, 0};
        uint8_t addr4[6] = {'R', 0, (uint8_t)(code >> 24), (uint8_t)(code >> 16), (uint8_t)(code >> 8), (uint8_t)code};
        uint8_t addr3[6] = {'R', 0, 0, (uint8_t)(code >> 16), (uint8_t)(code >> 8), (uint8_t)code};
        insertMacSlot(addr5, tagIdx);
        insertMacSlot(addr4, tagIdx);
        insertMacSlot(addr3, tagIdx);
      }
    }
  }
}

// ── String & MAC Utility ──

String TagRegistry::makeId() {
  char buf[9];
  snprintf(buf, sizeof(buf), "%08lx", (unsigned long)esp_random());
  return String(buf);
}

bool TagRegistry::parseMac(const String& s, uint8_t out[6]) {
  if (s.startsWith("RF:") || s.startsWith("rf:") || s.startsWith("Rf:") || s.startsWith("rF:")) {
    unsigned int a = 0, b = 0, c = 0, d = 0, e = 0;
    if (sscanf(s.c_str() + 3, "%02X:%02X:%02X:%02X:%02X", &a, &b, &c, &d, &e) == 5) {
      out[0] = 'R';
      out[1] = (uint8_t)a;
      out[2] = (uint8_t)b;
      out[3] = (uint8_t)c;
      out[4] = (uint8_t)d;
      out[5] = (uint8_t)e;
      return true;
    } else if (sscanf(s.c_str() + 3, "%02X:%02X:%02X:%02X", &b, &c, &d, &e) == 4) {
      out[0] = 'R';
      out[1] = 0;
      out[2] = (uint8_t)b;
      out[3] = (uint8_t)c;
      out[4] = (uint8_t)d;
      out[5] = (uint8_t)e;
      return true;
    } else if (sscanf(s.c_str() + 3, "%02X:%02X:%02X", &c, &d, &e) == 3) {
      out[0] = 'R';
      out[1] = 0;
      out[2] = 0;
      out[3] = (uint8_t)c;
      out[4] = (uint8_t)d;
      out[5] = (uint8_t)e;
      return true;
    }
    return false;
  }
  unsigned int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0;
  if (sscanf(s.c_str(), "%02X:%02X:%02X:%02X:%02X:%02X", &a, &b, &c, &d, &e, &f) != 6) {
    return false;
  }
  out[0] = (uint8_t)a;
  out[1] = (uint8_t)b;
  out[2] = (uint8_t)c;
  out[3] = (uint8_t)d;
  out[4] = (uint8_t)e;
  out[5] = (uint8_t)f;
  return true;
}

String TagRegistry::macToString(const uint8_t mac[6]) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3],
           mac[4], mac[5]);
  return String(buf);
}

// ── Private No-Lock Lookup Helpers (caller MUST hold TagStoreLock) ──
// Bug fix: eliminates double recursive-mutex acquisition that occurred when public
// methods called each other (e.g. getCopyById -> findById, create -> findByMacStr -> findByMac).
// Also raises probe limit from 16 to kProbeLimit=32 to handle dense 256-tag tables.

Tag* TagRegistry::_findByIdNoLock(uint32_t idVal) {
  // idVal == 0 is valid (rare edge case); tagIdx == 0xFFFF is the empty sentinel
  uint16_t idx = hashFib32(idVal);
  for (size_t probe = 0; probe < kProbeLimit; ++probe) {
    const auto& slot = _idTable[idx];
    if (slot.tagIdx == 0xFFFF) break;
    if (slot.idVal == idVal && slot.tagIdx < _tags.size()) {
      return &_tags[slot.tagIdx];
    }
    idx = (idx + 1) & kHashMask;
  }
  return nullptr;
}

Tag* TagRegistry::_findByMacNoLock(uint16_t macHigh, uint32_t macLow) {
  uint16_t idx = hashFibMac(macHigh, macLow);
  for (size_t probe = 0; probe < kProbeLimit; ++probe) {
    const auto& slot = _macTable[idx];
    if (slot.tagIdx == 0xFFFF) break;
    if (slot.macLow == macLow && slot.macHigh == macHigh && slot.tagIdx < _tags.size()) {
      return &_tags[slot.tagIdx];
    }
    idx = (idx + 1) & kHashMask;
  }
  return nullptr;
}

Tag* TagRegistry::_findByRfCodeNoLock(uint32_t code) {
  if (code == 0) return nullptr;
  const uint32_t code24 = code & 0x00FFFFFF;

  // 1. Exact code search
  uint16_t idx = hashFib32(code);
  for (size_t probe = 0; probe < kProbeLimit; ++probe) {
    const auto& slot = _rfTable[idx];
    if (slot.tagIdx == 0xFFFF) break;
    if (slot.code == code && slot.tagIdx < _tags.size()) {
      return &_tags[slot.tagIdx];
    }
    idx = (idx + 1) & kHashMask;
  }

  // 2. 24-bit masked fallback (for receivers that strip upper byte)
  if (code != code24) {
    idx = hashFib32(code24);
    for (size_t probe = 0; probe < kProbeLimit; ++probe) {
      const auto& slot = _rfTable[idx];
      if (slot.tagIdx == 0xFFFF) break;
      if (slot.code == code24 && slot.tagIdx < _tags.size()) {
        return &_tags[slot.tagIdx];
      }
      idx = (idx + 1) & kHashMask;
    }
  }

  return nullptr;
}

// ── Zero-Latency Flat Hash Table Lookups (O(1) in ~15-40 ns) ──
// Each public method acquires the lock exactly ONCE, then calls the no-lock helper.

Tag* TagRegistry::findById(const String& id) {
  TagStoreLock lock;
  if (id.isEmpty() || _tags.empty()) return nullptr;
  return _findByIdNoLock(parseId(id));
}

Tag* TagRegistry::findByMac(const uint8_t mac[6]) {
  TagStoreLock lock;
  if (!mac || _tags.empty()) return nullptr;
  const uint16_t macHigh = ((uint16_t)mac[0] << 8) | mac[1];
  const uint32_t macLow  = ((uint32_t)mac[2] << 24) | ((uint32_t)mac[3] << 16) | ((uint32_t)mac[4] << 8) | mac[5];
  return _findByMacNoLock(macHigh, macLow);
}

Tag* TagRegistry::findByMacStr(const String& mac) {
  TagStoreLock lock;
  uint8_t addr[6];
  if (!parseMac(mac, addr)) return nullptr;
  const uint16_t macHigh = ((uint16_t)addr[0] << 8) | addr[1];
  const uint32_t macLow  = ((uint32_t)addr[2] << 24) | ((uint32_t)addr[3] << 16) | ((uint32_t)addr[4] << 8) | addr[5];
  return _findByMacNoLock(macHigh, macLow);
}

Tag* TagRegistry::findByRfCode(uint32_t code) {
  TagStoreLock lock;
  if (code == 0 || _tags.empty()) return nullptr;
  return _findByRfCodeNoLock(code);
}

bool TagRegistry::getCopyById(const String& id, Tag& out) {
  TagStoreLock lock;
  if (id.isEmpty() || _tags.empty()) return false;
  Tag* t = _findByIdNoLock(parseId(id));
  if (t) { out = *t; return true; }
  return false;
}

bool TagRegistry::getCopyByMac(const uint8_t mac[6], Tag& out) {
  TagStoreLock lock;
  if (!mac || _tags.empty()) return false;
  const uint16_t macHigh = ((uint16_t)mac[0] << 8) | mac[1];
  const uint32_t macLow  = ((uint32_t)mac[2] << 24) | ((uint32_t)mac[3] << 16) | ((uint32_t)mac[4] << 8) | mac[5];
  Tag* t = _findByMacNoLock(macHigh, macLow);
  if (t) { out = *t; return true; }
  return false;
}

bool TagRegistry::getCopyByRfCode(uint32_t code, Tag& out) {
  TagStoreLock lock;
  if (code == 0 || _tags.empty()) return false;
  Tag* t = _findByRfCodeNoLock(code);
  if (t) { out = *t; return true; }
  return false;
}

// ── Classification & Activity ──

bool TagRegistry::isRf(const Tag& t) {
  return t.mac.startsWith("RF:") || t.mac.startsWith("rf:") || t.mac.startsWith("Rf:") ||
         t.mac.startsWith("rF:") || (t.macBin[0] == 'R');
}

bool TagRegistry::isActive(const Tag& t, uint32_t now) {
  if (t.lastSeenMs == 0) return false;
  uint32_t holdMs = isRf(t) ? kDefaultRfTagHoldMs : ((t.tagHoldMs > 0) ? t.tagHoldMs : kDefaultTagHoldMs);
  return (uint32_t)(now - t.lastSeenMs) <= holdMs;
}

bool TagRegistry::isTagInSchedule(const Tag& t) {
  if (!t.scheduleEnabled) return true;
  if (!gTimeSynced) {
    static uint32_t lastNtpWarnMs = 0;
    const uint32_t now = millis();
    if ((uint32_t)(now - lastNtpWarnMs) >= 5000) {
      lastNtpWarnMs = now;
      AD_LOG(Serial.printf("[tag] Tag '%s' has active time restriction, but NTP time is not synced -> access rejected\n",
                           t.name.isEmpty() ? t.id.c_str() : t.name.c_str()));
    }
    return false;
  }
  struct tm ti;
  if (!getLocalTime(&ti, 0)) return false;
  const uint16_t nowMin = (uint16_t)(ti.tm_hour * 60 + ti.tm_min);
  if (t.scheduleStartMin <= t.scheduleEndMin) {
    return (nowMin >= t.scheduleStartMin && nowMin <= t.scheduleEndMin);
  } else {
    // Overnight window: e.g. 22:00 to 06:00
    return (nowMin >= t.scheduleStartMin || nowMin <= t.scheduleEndMin);
  }
}

const char* TagRegistry::tagRssiZone(const Tag& t, uint32_t now, bool active) {
  if (t.lastSeenMs == 0) return "out";
  if (isRf(t)) {
    return active ? "in_range" : "out";
  }

  const uint32_t ageMs = (uint32_t)(now - t.lastSeenMs);
  const int8_t holdTh = (int8_t)(t.tagRssiMin - kTagRssiHysteresisDb);

  if (active) {
    return (t.lastRssi >= holdTh) ? "in_range" : "departing";
  }
  if (ageMs > 10000) return "out";
  if (t.lastRssi >= t.tagRssiMin) return "approaching";
  if (ageMs < 3000) return "departing";
  return "out";
}

bool TagRegistry::hasAnyActiveTag(uint32_t now) const {
  for (const auto& t : _tags) {
    if (isActive(t, now)) {
      return true;
    }
  }
  return false;
}

// ── Mutation / CRUD ──

Tag* TagRegistry::create(const String& name, const String& mac, uint32_t tagId, const String& icon) {
  TagStoreLock lock;
  if (_tags.size() >= kMaxTags) {
    Serial.printf("[tag_store] ERROR: Maximum capacity reached (%u tags)\n", (unsigned int)kMaxTags);
    return nullptr;
  }
  // Duplicate MAC check: use no-lock helper (lock already held above)
  uint8_t checkAddr[6];
  if (parseMac(mac, checkAddr)) {
    const uint16_t mH = ((uint16_t)checkAddr[0] << 8) | checkAddr[1];
    const uint32_t mL = ((uint32_t)checkAddr[2] << 24) | ((uint32_t)checkAddr[3] << 16) | ((uint32_t)checkAddr[4] << 8) | checkAddr[5];
    if (_findByMacNoLock(mH, mL)) return nullptr;
  }
  Tag t;
  t.id = makeId();
  t.name = name.length() ? name : "Tag " + mac.substring(mac.length() - 5);
  t.mac = mac;
  parseMac(mac, t.macBin);
  t.tagId = tagId;
  t.icon = icon;
  if (isRf(t)) {
    t.tagHoldMs = kDefaultRfTagHoldMs;
  }
  t.tagSafetyCloseDelayMs = (gSafetyCloseDelayMs > 0) ? gSafetyCloseDelayMs : kDefaultTagSafetyCloseDelayMs;
  t.dirty = true;
  const uint32_t newIdVal = parseId(t.id);  // capture before push_back
  _tags.push_back(t);
  rebuildIndexes();
  markDirty();
  if (gSafeMode) {
    gSafeMode = false;
    Serial.println("[safe] Tag added -> Auto-clearing Safe Mode");
    resumeEspNow();
  }
  // Return pointer via no-lock helper (lock still held)
  return _findByIdNoLock(newIdVal);
}

bool TagRegistry::update(const String& id, const String& name, uint32_t tagHoldMs, uint32_t tagCloseDelayMs,
                         int8_t tagRssiMin,
                         uint32_t tagSafetyCloseDelayMs,
                         uint32_t tagOpenDurationMs,
                         uint8_t rfRole,
                         const String& icon,
                         bool scheduleEnabled,
                         uint16_t scheduleStartMin,
                         uint16_t scheduleEndMin,
                         bool webhookEnabled,
                         bool webhookOpen,
                         bool webhookClosing,
                         bool webhookClose,
                         const String& webhookUrl) {
  TagStoreLock lock;
  // Use no-lock helper: lock is already acquired above
  Tag* t = _findByIdNoLock(parseId(id));
  if (!t) return false;
  bool changed = false;
  if (name.length() && t->name != name) { t->name = name; changed = true; }
  uint32_t targetHoldMs = isRf(*t) ? kDefaultRfTagHoldMs : tagHoldMs;
  if (targetHoldMs >= 1000 && t->tagHoldMs != targetHoldMs) {
    t->tagHoldMs = targetHoldMs;
    changed = true;
  }
  if (t->tagCloseDelayMs != tagCloseDelayMs) { t->tagCloseDelayMs = tagCloseDelayMs; changed = true; }
  if (t->tagRssiMin != tagRssiMin) { t->tagRssiMin = (int8_t)constrain((int)tagRssiMin, -120, 0); changed = true; }
  if (t->tagSafetyCloseDelayMs != tagSafetyCloseDelayMs) { t->tagSafetyCloseDelayMs = tagSafetyCloseDelayMs; changed = true; }
  if (t->tagOpenDurationMs != tagOpenDurationMs) { t->tagOpenDurationMs = tagOpenDurationMs; changed = true; }
  if (t->rfRole != rfRole) { t->rfRole = rfRole; changed = true; }
  if (!icon.isEmpty() && t->icon != icon) { t->icon = icon; changed = true; }
  if (t->scheduleEnabled != scheduleEnabled) { t->scheduleEnabled = scheduleEnabled; changed = true; }
  if (t->scheduleStartMin != scheduleStartMin) { t->scheduleStartMin = scheduleStartMin; changed = true; }
  if (t->scheduleEndMin != scheduleEndMin) { t->scheduleEndMin = scheduleEndMin; changed = true; }
  if (t->webhookEnabled != webhookEnabled) { t->webhookEnabled = webhookEnabled; changed = true; }
  if (t->webhookOpen != webhookOpen) { t->webhookOpen = webhookOpen; changed = true; }
  if (t->webhookClosing != webhookClosing) { t->webhookClosing = webhookClosing; changed = true; }
  if (t->webhookClose != webhookClose) { t->webhookClose = webhookClose; changed = true; }
  if (t->webhookUrl != webhookUrl) { t->webhookUrl = webhookUrl; changed = true; }

  if (changed) {
    t->dirty = true;
    markDirty();
  }
  return true;
}

bool TagRegistry::remove(const String& id) {
  TagStoreLock lock;
  for (auto it = _tags.begin(); it != _tags.end(); ++it) {
    if (it->id == id) {
      _tags.erase(it);
      rebuildIndexes();
      markDirty();
      return true;
    }
  }
  return false;
}

void TagRegistry::noteSeen(Tag& t, int8_t rssi, uint32_t now) {
  TagStoreLock lock;
  t.lastSeenMs = now;
  t.lastRssi = rssi;
}

// ── Baseline Suppression Domain ──

bool TagRegistry::wasInBaseline(const char* id) const {
  if (!id || !*id) return false;
  for (const auto& x : _baselineActiveIds) {
    if (x == id) return true;
  }
  return false;
}

bool TagRegistry::wasInBaseline(const String& id) const {
  for (const auto& x : _baselineActiveIds) {
    if (x == id) return true;
  }
  return false;
}

void TagRegistry::snapshotBaseline(uint32_t now) {
  _baselineActiveIds.clear();
  bool anyActive = false;
  for (const auto& t : _tags) {
    if (!isRf(t) && isActive(t, now)) {
      _baselineActiveIds.push_back(t.id);
      anyActive = true;
    }
  }
  _openSuppressed = anyActive;
}

void TagRegistry::clearSuppression() {
  _openSuppressed = false;
  _baselineActiveIds.clear();
}

bool TagRegistry::anyNewlyActiveTag(uint32_t now) const {
  for (const auto& t : _tags) {
    if (isActive(t, now) && !wasInBaseline(t.id)) {
      return true;
    }
  }
  return false;
}

uint32_t TagRegistry::getEffectiveSafetyCloseDelay(uint32_t now,
                                                  const String& activeTriggerTagId,
                                                  uint32_t prevActiveSafetyDelayMs,
                                                  uint32_t fallbackDelayMs) const {
  if (!activeTriggerTagId.isEmpty()) {
    // Use no-lock helper: callers (door.cpp) hold TagStoreLock before calling this
    Tag* trigTag = const_cast<TagRegistry*>(this)->_findByIdNoLock(parseId(activeTriggerTagId));
    if (trigTag) {
      return trigTag->tagSafetyCloseDelayMs;
    }
  }
  if (prevActiveSafetyDelayMs > 0) {
    return prevActiveSafetyDelayMs;
  }
  for (const auto& t : _tags) {
    if (isActive(t, now)) {
      return t.tagSafetyCloseDelayMs;
    }
  }
  return fallbackDelayMs;
}

// ── NVS Persistence ──

void TagRegistry::markDirty() {
  _tagsDirty = true;
  _tagsDirtyMs = millis();
}

void TagRegistry::flushIfIdle() {
  TagStoreLock lock;
  if (!_tagsDirty) return;
  const uint32_t now = millis();
  if ((uint32_t)(now - _tagsDirtyMs) < kTagsFlushIdleMs) return;
  saveToPrefs();
}

void TagRegistry::loadFromPrefs() {
  TagStoreLock lock;
  _tags.clear();

  size_t idsLen = gPrefs.getBytesLength("tag_ids");
  if (idsLen > 0) {
    size_t tagCount = idsLen / sizeof(uint32_t);
    _tags.reserve(tagCount > 32 ? (tagCount > kMaxTags ? kMaxTags : tagCount) : 32);
    std::vector<uint32_t> ids(tagCount);
    gPrefs.getBytes("tag_ids", ids.data(), idsLen);
    for (uint32_t id : ids) {
      if (_tags.size() >= kMaxTags) {
        Serial.printf("[tag_store] WARN: Reached max tag capacity (%u), skipping remaining\n", (unsigned int)kMaxTags);
        break;
      }
      char key[16];
      snprintf(key, sizeof(key), "t_%08lx", (unsigned long)id);
      size_t tagLen = gPrefs.getBytesLength(key);
      if (tagLen == sizeof(TagDiskEntry)) {
        TagDiskEntry entry;
        gPrefs.getBytes(key, &entry, sizeof(TagDiskEntry));

        Tag t;
        char idBuf[16];
        snprintf(idBuf, sizeof(idBuf), "%08lx", (unsigned long)entry.id);
        t.id = String(idBuf);

        char nameBuf[81];
        memcpy(nameBuf, entry.name, 80);
        nameBuf[80] = '\0';
        t.name = String(nameBuf);

        memcpy(t.macBin, entry.mac, 6);
        if (entry.isRf) {
          char macBuf[24];
          snprintf(macBuf, sizeof(macBuf), "RF:%02X:%02X:%02X:%02X:%02X", entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5]);
          t.mac = String(macBuf);
        } else {
          t.mac = macToString(t.macBin);
        }

        t.tagId = entry.tagId;
        t.tagHoldMs = entry.isRf ? kDefaultRfTagHoldMs : ((entry.tagHoldMs > 0) ? entry.tagHoldMs : kDefaultTagHoldMs);
        t.tagCloseDelayMs = entry.tagCloseDelayMs;
        t.tagRssiMin = entry.tagRssiMin;
        t.tagSafetyCloseDelayMs = entry.tagSafetyCloseDelayMs;
        t.tagOpenDurationMs = entry.tagOpenDurationMs;
        t.rfRole = entry.rfRole;

        char iconBuf[17];
        memcpy(iconBuf, entry.icon, 16);
        iconBuf[16] = '\0';
        t.icon = String(iconBuf);

        t.scheduleEnabled = (entry.scheduleEnabled != 0);
        t.scheduleStartMin = entry.scheduleStartMin;
        t.scheduleEndMin = (entry.scheduleEndMin == 0 && entry.scheduleStartMin == 0 && !t.scheduleEnabled) ? 1439 : entry.scheduleEndMin;
        t.webhookEnabled = (entry.webhookEnabled != 0);
        t.webhookOpen = (entry.webhookOpen != 0);
        t.webhookClosing = (entry.webhookClosing != 0);
        t.webhookClose = (entry.webhookClose != 0);
        char whBuf[97];
        memcpy(whBuf, entry.webhookUrl, 96);
        whBuf[96] = '\0';
        t.webhookUrl = String(whBuf);

        _tags.push_back(t);
      } else if (tagLen == sizeof(TagDiskEntryV5)) {
        TagDiskEntryV5 entryV5;
        gPrefs.getBytes(key, &entryV5, sizeof(TagDiskEntryV5));

        Tag t;
        char idBuf[16];
        snprintf(idBuf, sizeof(idBuf), "%08lx", (unsigned long)entryV5.id);
        t.id = String(idBuf);

        char nameBuf[81];
        memcpy(nameBuf, entryV5.name, 80);
        nameBuf[80] = '\0';
        t.name = String(nameBuf);

        memcpy(t.macBin, entryV5.mac, 6);
        if (entryV5.isRf) {
          char macBuf[24];
          snprintf(macBuf, sizeof(macBuf), "RF:%02X:%02X:%02X:%02X:%02X", entryV5.mac[1], entryV5.mac[2], entryV5.mac[3], entryV5.mac[4], entryV5.mac[5]);
          t.mac = String(macBuf);
        } else {
          t.mac = macToString(t.macBin);
        }

        t.tagId = entryV5.tagId;
        t.tagHoldMs = entryV5.isRf ? kDefaultRfTagHoldMs : ((entryV5.tagHoldMs > 0) ? entryV5.tagHoldMs : kDefaultTagHoldMs);
        t.tagCloseDelayMs = entryV5.tagCloseDelayMs;
        t.tagRssiMin = entryV5.tagRssiMin;
        t.tagSafetyCloseDelayMs = entryV5.tagSafetyCloseDelayMs;
        t.tagOpenDurationMs = entryV5.tagOpenDurationMs;
        t.rfRole = entryV5.rfRole;

        char iconBuf[17];
        memcpy(iconBuf, entryV5.icon, 16);
        iconBuf[16] = '\0';
        t.icon = String(iconBuf);

        t.scheduleEnabled = (entryV5.scheduleEnabled != 0);
        t.scheduleStartMin = entryV5.scheduleStartMin;
        t.scheduleEndMin = (entryV5.scheduleEndMin == 0 && entryV5.scheduleStartMin == 0 && !t.scheduleEnabled) ? 1439 : entryV5.scheduleEndMin;
        t.webhookEnabled = (entryV5.webhookEnabled != 0);
        t.webhookOpen = (entryV5.webhookOpen != 0);
        t.webhookClosing = true;
        t.webhookClose = (entryV5.webhookClose != 0);
        char whBuf[97];
        memcpy(whBuf, entryV5.webhookUrl, 96);
        whBuf[96] = '\0';
        t.webhookUrl = String(whBuf);
        t.dirty = true;

        _tags.push_back(t);
      } else if (tagLen == sizeof(TagDiskEntryV4)) {
        TagDiskEntryV4 entryV4;
        gPrefs.getBytes(key, &entryV4, sizeof(TagDiskEntryV4));

        Tag t;
        char idBuf[16];
        snprintf(idBuf, sizeof(idBuf), "%08lx", (unsigned long)entryV4.id);
        t.id = String(idBuf);

        char nameBuf[81];
        memcpy(nameBuf, entryV4.name, 80);
        nameBuf[80] = '\0';
        t.name = String(nameBuf);

        memcpy(t.macBin, entryV4.mac, 6);
        if (entryV4.isRf) {
          char macBuf[24];
          snprintf(macBuf, sizeof(macBuf), "RF:%02X:%02X:%02X:%02X:%02X", entryV4.mac[1], entryV4.mac[2], entryV4.mac[3], entryV4.mac[4], entryV4.mac[5]);
          t.mac = String(macBuf);
        } else {
          t.mac = macToString(t.macBin);
        }

        t.tagId = entryV4.tagId;
        t.tagHoldMs = entryV4.isRf ? kDefaultRfTagHoldMs : ((entryV4.tagHoldMs > 0) ? entryV4.tagHoldMs : kDefaultTagHoldMs);
        t.tagCloseDelayMs = entryV4.tagCloseDelayMs;
        t.tagRssiMin = entryV4.tagRssiMin;
        t.tagSafetyCloseDelayMs = entryV4.tagSafetyCloseDelayMs;
        t.tagOpenDurationMs = entryV4.tagOpenDurationMs;
        t.rfRole = entryV4.rfRole;

        char iconBuf[17];
        memcpy(iconBuf, entryV4.icon, 16);
        iconBuf[16] = '\0';
        t.icon = String(iconBuf);

        t.scheduleEnabled = (entryV4.scheduleEnabled != 0);
        t.scheduleStartMin = entryV4.scheduleStartMin;
        t.scheduleEndMin = (entryV4.scheduleEndMin == 0 && entryV4.scheduleStartMin == 0 && !t.scheduleEnabled) ? 1439 : entryV4.scheduleEndMin;
        t.webhookEnabled = (entryV4.webhookEnabled != 0);
        t.webhookOpen = true;
        t.webhookClosing = true;
        t.webhookClose = true;
        char whBuf[97];
        memcpy(whBuf, entryV4.webhookUrl, 96);
        whBuf[96] = '\0';
        t.webhookUrl = String(whBuf);
        t.dirty = true;

        _tags.push_back(t);
      } else if (tagLen == sizeof(TagDiskEntryV3)) {
        TagDiskEntryV3 entryV3;
        gPrefs.getBytes(key, &entryV3, sizeof(TagDiskEntryV3));

        Tag t;
        char idBuf[16];
        snprintf(idBuf, sizeof(idBuf), "%08lx", (unsigned long)entryV3.id);
        t.id = String(idBuf);

        char nameBuf[81];
        memcpy(nameBuf, entryV3.name, 80);
        nameBuf[80] = '\0';
        t.name = String(nameBuf);

        memcpy(t.macBin, entryV3.mac, 6);
        if (entryV3.isRf) {
          char macBuf[24];
          snprintf(macBuf, sizeof(macBuf), "RF:%02X:%02X:%02X:%02X:%02X", entryV3.mac[1], entryV3.mac[2], entryV3.mac[3], entryV3.mac[4], entryV3.mac[5]);
          t.mac = String(macBuf);
        } else {
          t.mac = macToString(t.macBin);
        }

        t.tagId = entryV3.tagId;
        t.tagHoldMs = entryV3.isRf ? kDefaultRfTagHoldMs : ((entryV3.tagHoldMs > 0) ? entryV3.tagHoldMs : kDefaultTagHoldMs);
        t.tagCloseDelayMs = entryV3.tagCloseDelayMs;
        t.tagRssiMin = entryV3.tagRssiMin;
        t.tagSafetyCloseDelayMs = entryV3.tagSafetyCloseDelayMs;
        t.tagOpenDurationMs = entryV3.tagOpenDurationMs;
        t.rfRole = entryV3.rfRole;

        char iconBuf[17];
        memcpy(iconBuf, entryV3.icon, 16);
        iconBuf[16] = '\0';
        t.icon = String(iconBuf);

        t.scheduleEnabled = (entryV3.scheduleEnabled != 0);
        t.scheduleStartMin = entryV3.scheduleStartMin;
        t.scheduleEndMin = (entryV3.scheduleEndMin == 0 && entryV3.scheduleStartMin == 0 && !t.scheduleEnabled) ? 1439 : entryV3.scheduleEndMin;
        t.webhookEnabled = true;
        t.webhookOpen = true;
        t.webhookClosing = true;
        t.webhookClose = true;
        t.webhookUrl = "";
        t.dirty = true;

        _tags.push_back(t);
      } else if (tagLen == sizeof(TagDiskEntryV2)) {
        TagDiskEntryV2 entryV2;
        gPrefs.getBytes(key, &entryV2, sizeof(TagDiskEntryV2));

        Tag t;
        char idBuf[16];
        snprintf(idBuf, sizeof(idBuf), "%08lx", (unsigned long)entryV2.id);
        t.id = String(idBuf);

        char nameBuf[81];
        memcpy(nameBuf, entryV2.name, 80);
        nameBuf[80] = '\0';
        t.name = String(nameBuf);

        memcpy(t.macBin, entryV2.mac, 6);
        if (entryV2.isRf) {
          char macBuf[24];
          snprintf(macBuf, sizeof(macBuf), "RF:%02X:%02X:%02X:%02X:%02X", entryV2.mac[1], entryV2.mac[2], entryV2.mac[3], entryV2.mac[4], entryV2.mac[5]);
          t.mac = String(macBuf);
        } else {
          t.mac = macToString(t.macBin);
        }

        t.tagId = entryV2.tagId;
        t.tagHoldMs = entryV2.isRf ? kDefaultRfTagHoldMs : ((entryV2.tagHoldMs > 0) ? entryV2.tagHoldMs : kDefaultTagHoldMs);
        t.tagCloseDelayMs = entryV2.tagCloseDelayMs;
        t.tagRssiMin = entryV2.tagRssiMin;
        t.tagSafetyCloseDelayMs = entryV2.tagSafetyCloseDelayMs;
        t.tagOpenDurationMs = entryV2.tagOpenDurationMs;
        t.rfRole = entryV2.rfRole;

        char iconBuf[17];
        memcpy(iconBuf, entryV2.icon, 16);
        iconBuf[16] = '\0';
        t.icon = String(iconBuf);

        t.scheduleEnabled = false;
        t.scheduleStartMin = 0;
        t.scheduleEndMin = 1439;
        t.dirty = true;

        _tags.push_back(t);
      } else if (tagLen == sizeof(TagDiskEntryV1)) {
        TagDiskEntryV1 entryV1;
        gPrefs.getBytes(key, &entryV1, sizeof(TagDiskEntryV1));

        Tag t;
        char idBuf[16];
        snprintf(idBuf, sizeof(idBuf), "%08lx", (unsigned long)entryV1.id);
        t.id = String(idBuf);

        char nameBuf[81];
        memcpy(nameBuf, entryV1.name, 80);
        nameBuf[80] = '\0';
        t.name = String(nameBuf);

        memcpy(t.macBin, entryV1.mac, 6);
        if (entryV1.isRf) {
          char macBuf[24];
          snprintf(macBuf, sizeof(macBuf), "RF:%02X:%02X:%02X:%02X:%02X", entryV1.mac[1], entryV1.mac[2], entryV1.mac[3], entryV1.mac[4], entryV1.mac[5]);
          t.mac = String(macBuf);
        } else {
          t.mac = macToString(t.macBin);
        }

        t.tagId = entryV1.tagId;
        t.tagHoldMs = entryV1.isRf ? kDefaultRfTagHoldMs : ((entryV1.tagHoldMs > 0) ? entryV1.tagHoldMs : kDefaultTagHoldMs);
        t.tagCloseDelayMs = entryV1.tagCloseDelayMs;
        t.tagRssiMin = entryV1.tagRssiMin;
        t.tagSafetyCloseDelayMs = entryV1.tagSafetyCloseDelayMs;
        t.tagOpenDurationMs = entryV1.tagOpenDurationMs;
        t.rfRole = entryV1.rfRole;
        t.icon = "";
        t.scheduleEnabled = false;
        t.scheduleStartMin = 0;
        t.scheduleEndMin = 1439;
        t.dirty = true;

        _tags.push_back(t);
      } else if (tagLen == 105) {
        struct OldTagDiskEntry105 {
          uint32_t id;
          char name[80];
          uint8_t mac[6];
          uint8_t isRf;
          uint32_t tagId;
          uint32_t tagHoldMs;
          uint32_t tagCloseDelayMs;
          int8_t tagRssiMin;
          int8_t tagPassRssi;
          uint32_t tagPassCloseDelayMs;
        } oldEntry105;
        gPrefs.getBytes(key, &oldEntry105, sizeof(OldTagDiskEntry105));

        Tag t;
        char idBuf[16];
        snprintf(idBuf, sizeof(idBuf), "%08lx", (unsigned long)oldEntry105.id);
        t.id = String(idBuf);
        char nameBuf[81];
        memcpy(nameBuf, oldEntry105.name, 80);
        nameBuf[80] = '\0';
        t.name = String(nameBuf);
        memcpy(t.macBin, oldEntry105.mac, 6);
        if (oldEntry105.isRf) {
          char macBuf[24];
          snprintf(macBuf, sizeof(macBuf), "RF:%02X:%02X:%02X:%02X:%02X", oldEntry105.mac[1], oldEntry105.mac[2], oldEntry105.mac[3], oldEntry105.mac[4], oldEntry105.mac[5]);
          t.mac = String(macBuf);
        } else {
          t.mac = macToString(t.macBin);
        }
        t.tagId = oldEntry105.tagId;
        t.tagHoldMs = (oldEntry105.tagHoldMs > 0) ? oldEntry105.tagHoldMs : (oldEntry105.isRf ? kDefaultRfTagHoldMs : kDefaultTagHoldMs);
        t.tagCloseDelayMs = oldEntry105.tagCloseDelayMs;
        t.tagRssiMin = oldEntry105.tagRssiMin;
        t.tagSafetyCloseDelayMs = kDefaultTagSafetyCloseDelayMs;
        t.dirty = true;
        _tags.push_back(t);
      } else if (tagLen == 285) {
        struct OldTagDiskEntry256 {
          uint32_t id;
          char name[256];
          uint8_t mac[6];
          uint8_t isRf;
          uint32_t tagId;
          uint32_t tagHoldMs;
          uint32_t tagCloseDelayMs;
          int8_t tagRssiMin;
          int8_t tagPassRssi;
          uint32_t tagPassCloseDelayMs;
        } oldEntry;
        gPrefs.getBytes(key, &oldEntry, sizeof(OldTagDiskEntry256));

        Tag t;
        char idBuf[16];
        snprintf(idBuf, sizeof(idBuf), "%08lx", (unsigned long)oldEntry.id);
        t.id = String(idBuf);
        char nameBuf[257];
        memcpy(nameBuf, oldEntry.name, 256);
        nameBuf[256] = '\0';
        t.name = String(nameBuf);
        memcpy(t.macBin, oldEntry.mac, 6);
        if (oldEntry.isRf) {
          char macBuf[24];
          snprintf(macBuf, sizeof(macBuf), "RF:%02X:%02X:%02X:%02X:%02X", oldEntry.mac[1], oldEntry.mac[2], oldEntry.mac[3], oldEntry.mac[4], oldEntry.mac[5]);
          t.mac = String(macBuf);
        } else {
          t.mac = macToString(t.macBin);
        }
        t.tagId = oldEntry.tagId;
        t.tagHoldMs = (oldEntry.tagHoldMs > 0) ? oldEntry.tagHoldMs : (oldEntry.isRf ? kDefaultRfTagHoldMs : kDefaultTagHoldMs);
        t.tagCloseDelayMs = oldEntry.tagCloseDelayMs;
        t.tagRssiMin = oldEntry.tagRssiMin;
        t.tagSafetyCloseDelayMs = kDefaultTagSafetyCloseDelayMs;
        t.dirty = true;
        _tags.push_back(t);
      } else if (tagLen >= sizeof(TagDiskEntryV1)) {
        // Safe generic fallback for any unknown size >= 109 bytes
        TagDiskEntry fallback = {};
        fallback.webhookEnabled = 1;
        fallback.webhookOpen = 1;
        fallback.webhookClosing = 1;
        fallback.webhookClose = 1;
        size_t readLen = (tagLen > sizeof(TagDiskEntry)) ? sizeof(TagDiskEntry) : tagLen;
        gPrefs.getBytes(key, &fallback, readLen);

        Tag t;
        char idBuf[16];
        snprintf(idBuf, sizeof(idBuf), "%08lx", (unsigned long)fallback.id);
        t.id = String(idBuf);
        char nameBuf[81];
        memcpy(nameBuf, fallback.name, 80);
        nameBuf[80] = '\0';
        t.name = String(nameBuf);
        memcpy(t.macBin, fallback.mac, 6);
        if (fallback.isRf) {
          char macBuf[24];
          snprintf(macBuf, sizeof(macBuf), "RF:%02X:%02X:%02X:%02X:%02X", fallback.mac[1], fallback.mac[2], fallback.mac[3], fallback.mac[4], fallback.mac[5]);
          t.mac = String(macBuf);
        } else {
          t.mac = macToString(t.macBin);
        }
        t.tagId = fallback.tagId;
        t.tagHoldMs = fallback.isRf ? kDefaultRfTagHoldMs : ((fallback.tagHoldMs > 0) ? fallback.tagHoldMs : kDefaultTagHoldMs);
        t.tagCloseDelayMs = fallback.tagCloseDelayMs;
        t.tagRssiMin = fallback.tagRssiMin;
        t.tagSafetyCloseDelayMs = (fallback.tagSafetyCloseDelayMs > 0) ? fallback.tagSafetyCloseDelayMs : kDefaultTagSafetyCloseDelayMs;
        t.tagOpenDurationMs = fallback.tagOpenDurationMs;
        t.rfRole = fallback.rfRole;
        char iconBuf[17];
        memcpy(iconBuf, fallback.icon, 16);
        iconBuf[16] = '\0';
        t.icon = String(iconBuf);
        t.scheduleEnabled = (fallback.scheduleEnabled != 0);
        t.scheduleStartMin = fallback.scheduleStartMin;
        t.scheduleEndMin = (fallback.scheduleEndMin == 0 && fallback.scheduleStartMin == 0 && !t.scheduleEnabled) ? 1439 : fallback.scheduleEndMin;
        t.webhookEnabled = (fallback.webhookEnabled != 0);
        t.webhookOpen = (fallback.webhookOpen != 0);
        t.webhookClosing = (fallback.webhookClosing != 0);
        t.webhookClose = (fallback.webhookClose != 0);
        char whBuf[97];
        memcpy(whBuf, fallback.webhookUrl, 96);
        whBuf[96] = '\0';
        t.webhookUrl = String(whBuf);
        t.dirty = true;
        _tags.push_back(t);
        Serial.printf("[tag_store] Recovered tag %s via generic fallback (tagLen=%u)\n", t.id.c_str(), (unsigned int)tagLen);
      }
    }
    rebuildIndexes();
    Serial.printf("[tag_store] Loaded %d tags. Index rebuilt (O(1) Hash Map + MRU Cache active).\n", (int)_tags.size());
  } else {
    // Migration from old schema (tags_bin)
    size_t len = gPrefs.getBytesLength("tags_bin");
    if (len > 0) {
      struct OldTagDiskEntry {
        uint32_t id;
        char name[16];
        uint8_t mac[6];
        uint8_t isRf;
        uint32_t tagId;
        uint32_t tagHoldMs;
        uint32_t tagCloseDelayMs;
        int8_t tagRssiMin;
        int8_t tagPassRssi;
        uint32_t tagPassCloseDelayMs;
      };

      std::vector<uint8_t> buffer(len);
      gPrefs.getBytes("tags_bin", buffer.data(), len);
      size_t count = len / sizeof(OldTagDiskEntry);
      const OldTagDiskEntry* entries = reinterpret_cast<const OldTagDiskEntry*>(buffer.data());
      for (size_t i = 0; i < count; ++i) {
        const auto& entry = entries[i];
        Tag t;
        char idBuf[16];
        snprintf(idBuf, sizeof(idBuf), "%08lx", (unsigned long)entry.id);
        t.id = String(idBuf);

        char nameBuf[17];
        memcpy(nameBuf, entry.name, 16);
        nameBuf[16] = '\0';
        t.name = String(nameBuf);

        memcpy(t.macBin, entry.mac, 6);
        if (entry.isRf) {
          char macBuf[24];
          snprintf(macBuf, sizeof(macBuf), "RF:%02X:%02X:%02X:%02X:%02X", entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5]);
          t.mac = String(macBuf);
        } else {
          t.mac = macToString(t.macBin);
        }

        t.tagId = entry.tagId;
        t.tagHoldMs = entry.tagHoldMs;
        t.tagCloseDelayMs = entry.tagCloseDelayMs;
        t.tagRssiMin = entry.tagRssiMin;
        t.tagSafetyCloseDelayMs = kDefaultTagSafetyCloseDelayMs;
        t.dirty = true;

        _tags.push_back(t);
      }

      saveToPrefs();
      gPrefs.remove("tags_bin");
      Serial.println("[tag_store] Migrated tags_bin to individual NVS keys.");
    } else {
      // JSON fallback migration
      const String raw = gPrefs.getString("tags", "");
      if (!raw.isEmpty()) {
        JsonDocument doc;
        if (deserializeJson(doc, raw) == DeserializationError::Ok) {
          JsonArray arr = doc["tags"].as<JsonArray>();
          if (!arr.isNull()) {
            for (JsonObject o : arr) {
              Tag t;
              t.id = o["id"] | "";
              t.name = o["name"] | "Tag";
              t.mac = o["mac"] | "";
              t.tagId = o["tagId"] | 0U;
              t.tagHoldMs = o["tagHoldMs"] | kDefaultTagHoldMs;
              t.tagCloseDelayMs = o["tagCloseDelayMs"] | kDefaultTagCloseDelayMs;
              t.tagRssiMin = (int8_t)(int)(o["tagRssiMin"] | (int)kDefaultTagRssiMin);
              t.tagSafetyCloseDelayMs = o["tagSafetyCloseDelayMs"] | kDefaultTagSafetyCloseDelayMs;
              t.tagOpenDurationMs = o["tagOpenDurationMs"] | 0U;
              t.rfRole = (uint8_t)(o["rfRole"] | 0);
              t.icon = o["icon"] | "";
              if (t.id.length() && t.mac.length()) {
                parseMac(t.mac, t.macBin);
                t.dirty = true;
                _tags.push_back(t);
              }
            }
          }
        }
        if (!_tags.empty()) {
          saveToPrefs();
          gPrefs.remove("tags");
          Serial.println("[tag_store] Migrated old JSON tags to individual NVS keys.");
        }
      }
    }
  }
  _tagsLoaded = true;
}

void TagRegistry::saveToPrefs() {
  TagStoreLock lock;
  if (!_tagsLoaded) {
    Serial.println("[tag_store] WARNING: Refusing to save tags before loadFromPrefs() completes.");
    return;
  }
  if (_tags.empty()) {
    size_t idsLen = gPrefs.getBytesLength("tag_ids");
    if (idsLen > 0) {
      std::vector<uint32_t> ids(idsLen / sizeof(uint32_t));
      gPrefs.getBytes("tag_ids", ids.data(), idsLen);
      for (uint32_t id : ids) {
        char key[16];
        snprintf(key, sizeof(key), "t_%08lx", (unsigned long)id);
        gPrefs.remove(key);
      }
    }
    gPrefs.remove("tag_ids");
    _tagsDirty = false;
    return;
  }

  std::vector<uint32_t> currentIds;
  currentIds.reserve(_tags.size());

  size_t oldLen = gPrefs.getBytesLength("tag_ids");
  std::vector<uint32_t> oldIds;
  if (oldLen > 0) {
    oldIds.resize(oldLen / sizeof(uint32_t));
    gPrefs.getBytes("tag_ids", oldIds.data(), oldLen);
  }

  for (auto& t : _tags) {
    uint32_t idVal = parseId(t.id);
    currentIds.push_back(idVal);

    if (t.dirty) {
      TagDiskEntry entry = {};
      entry.id = idVal;
      strncpy(entry.name, t.name.c_str(), sizeof(entry.name) - 1);
      entry.name[sizeof(entry.name) - 1] = '\0';
      memcpy(entry.mac, t.macBin, 6);
      entry.isRf = (t.mac.startsWith("RF:") || t.mac.startsWith("rf:")) ? 1 : 0;
      entry.tagId = t.tagId;
      entry.tagHoldMs = t.tagHoldMs;
      entry.tagCloseDelayMs = t.tagCloseDelayMs;
      entry.tagRssiMin = t.tagRssiMin;
      entry.rfRole = t.rfRole;
      entry.tagOpenDurationMs = t.tagOpenDurationMs;
      entry.tagSafetyCloseDelayMs = t.tagSafetyCloseDelayMs;
      strncpy(entry.icon, t.icon.c_str(), sizeof(entry.icon) - 1);
      entry.icon[sizeof(entry.icon) - 1] = '\0';
      entry.scheduleEnabled = t.scheduleEnabled ? 1 : 0;
      entry.scheduleStartMin = t.scheduleStartMin;
      entry.scheduleEndMin = t.scheduleEndMin;
      entry.webhookEnabled = t.webhookEnabled ? 1 : 0;
      entry.webhookOpen = t.webhookOpen ? 1 : 0;
      entry.webhookClosing = t.webhookClosing ? 1 : 0;
      entry.webhookClose = t.webhookClose ? 1 : 0;
      strncpy(entry.webhookUrl, t.webhookUrl.c_str(), sizeof(entry.webhookUrl) - 1);
      entry.webhookUrl[sizeof(entry.webhookUrl) - 1] = '\0';

      char key[16];
      snprintf(key, sizeof(key), "t_%08lx", (unsigned long)idVal);
      size_t written = gPrefs.putBytes(key, &entry, sizeof(TagDiskEntry));
      if (written == sizeof(TagDiskEntry)) {
        t.dirty = false;
        Serial.printf("[tag_store] Saved dirty tag %s (%s) to NVS.\n", t.id.c_str(), t.name.c_str());
      } else {
        Serial.printf("[tag_store] ERROR: Failed to save tag %s (%s) to NVS (written=%d/%d)\n",
                      t.id.c_str(), t.name.c_str(), (int)written, (int)sizeof(TagDiskEntry));
      }
    }
  }

  for (uint32_t oid : oldIds) {
    bool found = false;
    for (uint32_t cid : currentIds) {
      if (oid == cid) {
        found = true;
        break;
      }
    }
    if (!found) {
      char key[16];
      snprintf(key, sizeof(key), "t_%08lx", (unsigned long)oid);
      gPrefs.remove(key);
      Serial.printf("[tag_store] Deleted NVS key for tag ID: %08lx\n", (unsigned long)oid);
    }
  }

  size_t bytesToWrite = currentIds.size() * sizeof(uint32_t);
  gPrefs.putBytes("tag_ids", currentIds.data(), bytesToWrite);
  _tagsDirty = false;
  Serial.printf("[tag_store] Index updated. Total tags: %d\n", (int)_tags.size());
}
