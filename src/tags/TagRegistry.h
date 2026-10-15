#pragma once

#include "app/config.h"
#include "app/types.h"

#include <Arduino.h>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class TagRegistry;

// ============================================================================
// TagStoreLock: RAII Recursive Mutex Guard for Tag Storage & NVS
// ============================================================================
// Thread-Safety Contract:
// Tag storage is protected by a recursive mutex (gTagStoreMutex).
// FreeRTOS tasks (rf433Task, loopTask, webServer) must hold this lock
// when accessing or mutating Tag instances to avoid race conditions.

struct TagStoreLock {
  bool locked = false;
  explicit TagStoreLock(uint32_t waitMs = 500);
  ~TagStoreLock();
  bool isLocked() const { return locked; }
};

// ============================================================================
// TagRegistry: Encapsulated Tag domain with two-tier lookup & baseline suppression
// ============================================================================

class TagRegistry {
public:
  static constexpr size_t kMaxTags = 256;
  static constexpr size_t kHashSlots = 512;
  static constexpr uint32_t kHashMask = kHashSlots - 1; // 0x1FF
  static constexpr size_t kProbeLimit = 32;              // Max probe steps (load factor <= 50%)

  struct FastRfSlot {
    uint32_t code = 0;
    uint16_t tagIdx = 0xFFFF; // 0xFFFF = empty
    uint16_t reserved = 0;
  };

  struct FastMacSlot {
    uint32_t macLow = 0;      // mac[2..5]
    uint16_t macHigh = 0;     // mac[0..1]
    uint16_t tagIdx = 0xFFFF; // 0xFFFF = empty
  };

  struct FastIdSlot {
    uint32_t idVal = 0;
    uint16_t tagIdx = 0xFFFF; // 0xFFFF = empty
    uint16_t reserved = 0;
  };

  TagRegistry();
  ~TagRegistry();

  // Non-copyable
  TagRegistry(const TagRegistry&) = delete;
  TagRegistry& operator=(const TagRegistry&) = delete;

  // ── Lifecycle & Initialization ──
  void init();
  void loadFromPrefs();
  void saveToPrefs();
  void flushIfIdle();
  void markDirty();
  bool isLoaded() const { return _tagsLoaded; }

  // ── Mutex Lock ──
  TagStoreLock acquireLock(uint32_t waitMs = 500) { return TagStoreLock(waitMs); }
  SemaphoreHandle_t mutex() const { return _mutex; }

  // ── Lookups (Thread-safe: caller should hold TagStoreLock for stable pointers) ──
  Tag* findById(const String& id);
  Tag* findByMac(const uint8_t mac[6]);
  Tag* findByMacStr(const String& mac);
  Tag* findByRfCode(uint32_t code);

  // Safe copy lookups
  bool getCopyById(const String& id, Tag& out);
  bool getCopyByMac(const uint8_t mac[6], Tag& out);
  bool getCopyByRfCode(uint32_t code, Tag& out);

  // ── Tag Classification & Activity Evaluation ──
  static bool isRf(const Tag& t);
  static bool isActive(const Tag& t, uint32_t now);
  static bool isTagInSchedule(const Tag& t);
  static const char* tagRssiZone(const Tag& t, uint32_t now, bool active);
  bool hasAnyActiveTag(uint32_t now) const;

  // ── Mutation / CRUD ──
  Tag* create(const String& name, const String& mac, uint32_t tagId, const String& icon = "");
  bool update(const String& id, const String& name, uint32_t tagHoldMs, uint32_t tagCloseDelayMs,
              int8_t tagRssiMin,
              uint32_t tagSafetyCloseDelayMs = kDefaultTagSafetyCloseDelayMs,
              uint32_t tagOpenDurationMs = 0,
              uint8_t rfRole = 0,
              const String& icon = "",
              bool scheduleEnabled = false,
              uint16_t scheduleStartMin = 0,
              uint16_t scheduleEndMin = 1439,
              bool webhookEnabled = false,
              bool webhookOpen = true,
              bool webhookClosing = true,
              bool webhookClose = true,
              const String& webhookUrl = "");
  bool remove(const String& id);
  void noteSeen(Tag& t, int8_t rssi, uint32_t now);

  // ── Collection Access ──
  std::vector<Tag>& tags() { return _tags; }
  const std::vector<Tag>& tags() const { return _tags; }
  size_t count() const { return _tags.size(); }
  bool empty() const { return _tags.empty(); }

  // ── Baseline Suppression Domain (Transferred from door.cpp) ──
  void snapshotBaseline(uint32_t now);
  void clearSuppression();
  void setSuppressed(bool suppressed) { _openSuppressed = suppressed; }
  bool isSuppressed() const { return _openSuppressed; }
  bool& suppressionRef() { return _openSuppressed; }
  bool wasInBaseline(const String& id) const;
  bool wasInBaseline(const char* id) const;
  bool anyNewlyActiveTag(uint32_t now) const;
  const std::vector<String>& baselineActiveIds() const { return _baselineActiveIds; }
  std::vector<String>& baselineActiveIds() { return _baselineActiveIds; }

  // Effective safety close delay evaluation
  uint32_t getEffectiveSafetyCloseDelay(uint32_t now,
                                       const String& activeTriggerTagId = "",
                                       uint32_t prevActiveSafetyDelayMs = 0,
                                       uint32_t fallbackDelayMs = 0) const;

  // ── Static Utility Methods ──
  static String makeId();
  static bool parseMac(const String& s, uint8_t out[6]);
  static String macToString(const uint8_t mac[6]);

private:
  void rebuildIndexes();

  // No-lock lookup helpers — caller MUST hold TagStoreLock before calling
  Tag* _findByIdNoLock(uint32_t idVal);
  Tag* _findByMacNoLock(uint16_t macHigh, uint32_t macLow);
  Tag* _findByRfCodeNoLock(uint32_t code);

  std::vector<Tag> _tags;
  SemaphoreHandle_t _mutex = nullptr;

  // Zero-Latency Flat Open-Addressing Hash Tables (Fibonacci Hashing, Load Factor <= 50%)
  FastRfSlot  _rfTable[kHashSlots];
  FastMacSlot _macTable[kHashSlots];
  FastIdSlot  _idTable[kHashSlots];

  void insertRfSlot(uint32_t code, uint16_t tagIdx);
  void insertMacSlot(const uint8_t mac[6], uint16_t tagIdx);
  void insertIdSlot(uint32_t idVal, uint16_t tagIdx);

  static inline uint16_t hashFib32(uint32_t val) {
    return (uint16_t)((val * 0x9E3779B9U) >> 23);
  }

  static inline uint16_t hashFibMac(uint16_t macHigh, uint32_t macLow) {
    uint32_t k = macLow ^ ((uint32_t)macHigh << 16);
    return (uint16_t)((k * 0x9E3779B9U) >> 23);
  }

  bool _tagsLoaded = false;
  bool _tagsDirty = false;
  uint32_t _tagsDirtyMs = 0;

  // Baseline suppression state
  std::vector<String> _baselineActiveIds;
  bool _openSuppressed = false;

  friend struct TagStoreLock;
};

// Global instance for Composition Root / backward compatibility
extern TagRegistry gTagRegistry;
extern SemaphoreHandle_t gTagStoreMutex;

// Backward-compatible initialization helper
inline void initTagStoreMutex() {
  gTagRegistry.init();
}

// ── Free-function wrappers delegating to gTagRegistry (Backward Compatibility) ──
inline Tag* findTagById(const String& id) { return gTagRegistry.findById(id); }
inline Tag* findTagByMac(const uint8_t mac[6]) { return gTagRegistry.findByMac(mac); }
inline Tag* findTagByMacStr(const String& mac) { return gTagRegistry.findByMacStr(mac); }
inline Tag* findTagByRfCode(uint32_t code) { return gTagRegistry.findByRfCode(code); }

inline bool getTagCopyById(const String& id, Tag& out) { return gTagRegistry.getCopyById(id, out); }
inline bool getTagCopyByMac(const uint8_t mac[6], Tag& out) { return gTagRegistry.getCopyByMac(mac, out); }
inline bool getTagCopyByRfCode(uint32_t code, Tag& out) { return gTagRegistry.getCopyByRfCode(code, out); }

inline bool isRfTag(const Tag& t) { return TagRegistry::isRf(t); }
inline bool isTagActive(const Tag& t, uint32_t now) { return TagRegistry::isActive(t, now); }
inline const char* tagRssiZone(const Tag& t, uint32_t now, bool active) { return TagRegistry::tagRssiZone(t, now, active); }

inline void loadTagsFromPrefs() { gTagRegistry.loadFromPrefs(); }
inline void saveTagsToPrefs() { gTagRegistry.saveToPrefs(); }
inline void flushTagsIfIdle() { gTagRegistry.flushIfIdle(); }

inline Tag* createTag(const String& name, const String& mac, uint32_t tagId, const String& icon = "") {
  return gTagRegistry.create(name, mac, tagId, icon);
}
inline bool updateTag(const String& id, const String& name, uint32_t tagHoldMs, uint32_t tagCloseDelayMs,
                      int8_t tagRssiMin,
                      uint32_t tagSafetyCloseDelayMs = kDefaultTagSafetyCloseDelayMs,
                      uint32_t tagOpenDurationMs = 0,
                      uint8_t rfRole = 0,
                      const String& icon = "",
                      bool scheduleEnabled = false,
                      uint16_t scheduleStartMin = 0,
                      uint16_t scheduleEndMin = 1439,
                      bool webhookEnabled = false,
                      bool webhookOpen = true,
                      bool webhookClosing = true,
                      bool webhookClose = true,
                      const String& webhookUrl = "") {
  return gTagRegistry.update(id, name, tagHoldMs, tagCloseDelayMs, tagRssiMin, tagSafetyCloseDelayMs, tagOpenDurationMs, rfRole, icon, scheduleEnabled, scheduleStartMin, scheduleEndMin, webhookEnabled, webhookOpen, webhookClosing, webhookClose, webhookUrl);
}
inline bool deleteTag(const String& id) { return gTagRegistry.remove(id); }
inline void noteTagSeen(Tag& t, int8_t rssi, uint32_t now) { gTagRegistry.noteSeen(t, rssi, now); }

inline String makeId() { return TagRegistry::makeId(); }
inline bool parseMac(const String& s, uint8_t out[6]) { return TagRegistry::parseMac(s, out); }
inline String macToString(const uint8_t mac[6]) { return TagRegistry::macToString(mac); }
