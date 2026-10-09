#pragma once

#include "app/types.h"

#include <Arduino.h>
#include <vector>

// Forward declaration
struct DoorEventLogEntry;

// ============================================================================
// DiagEventLogger: Thread-safe Ultra-Compact 8-Byte Ring Buffer for Access Logs
// ============================================================================
// Stores exclusively access/entry events (Who entered, at what time, RSSI)
// Each record is exactly 8 bytes (Unix Epoch 4B + Tag ID 2B + Method/Action 1B + RSSI 1B).
// Capacity: 500 entries = only 4,000 bytes (4.0 KB RAM)

class DiagEventLogger {
public:
  static constexpr size_t kMaxEvents = 500;

  DiagEventLogger();

  // ── Primary Access Logging API ──
  void logAccessEvent(uint16_t tagId);
  void logAccessEvent(AccessMethod method, AccessAction action, uint16_t tagId, int8_t rssi = 0);

  // ── Compatibility Logging API (Filters exclusively for real access events) ──
  void logDiagEvent(DoorEventCode code,
                    DoorEventLevel level = DoorEventLevel::Info,
                    DiagReasonCode reason = DiagReasonCode::None,
                    uint16_t durationMs = 0,
                    int16_t metricVal = 0,
                    const char* entity = nullptr,
                    uint8_t customSensorBits = 0xFF);

  // ── Convenience overloads (preserving existing call patterns) ──
  void logDoorEvent(DoorEventCode code, DoorEventLevel level = DoorEventLevel::Info, uint16_t param = 0, const char* extra = nullptr);
  void logDoorEvent(DoorEventCode code, DoorEventLevel level, uint16_t param, const String& extra);
  void logDoorEvent(const char* code, const char* level, const char* msg);
  void logDoorEvent(const char* code, const char* level, const String& msg);

  // ── Ultra-Compact 6-Byte Binary Access Log API ──
  static void encodeAccessRecord(const AccessLogRecord& plain, CompactAccessRecord& compact);
  static void decodeAccessRecord(const CompactAccessRecord& compact, AccessLogRecord& plain);

  std::vector<CompactAccessRecord> getRecentCompactAccessRecords(size_t maxCount = 500);
  std::vector<AccessLogRecord> getRecentAccessRecords(size_t maxCount = 500);
  static DoorEventLogEntry formatAccessEntry(const AccessLogRecord& rec);
  static DoorEventLogEntry formatAccessEntry(const CompactAccessRecord& rec);

  // ── Retrieval API (Backward-compatible wrappers) ──
  std::vector<CompactDiagEvent> getRecentCompactDoorEvents(size_t maxCount = 500);
  std::vector<DoorEventLogEntry> getRecentDoorEvents(size_t maxCount = 500);
  DoorEventLogEntry formatEventEntry(const CompactDiagEvent& ev);

  // ── Management ──
  void clear();
  uint32_t totalLogged();

  // ── Sensor snapshot (reads current global sensor state) ──
  static uint8_t captureCurrentSensorBits();

  // ── Entity / Tag ID Resolution ──
  static uint16_t encodeTagId(const char* entity);
  static void resolveTagString(uint16_t tagId, char* out, size_t outSize);
  static void resolveDeviceInfo(uint16_t tagId, char* outName, size_t outNameSize,
                                char* outType, size_t outTypeSize,
                                char* outSubtitle, size_t outSubSize);
  static inline uint8_t encodeEntity(const char* entity) { return (uint8_t)(encodeTagId(entity) & 0xFF); }
  static inline void resolveEntityString(uint8_t entityRef, char* out, size_t outSize) { resolveTagString(entityRef, out, outSize); }

  // ── Utility ──
  static void copyUtf8Safe(char* dst, const char* src, size_t dstSize);

private:
  CompactAccessRecord _ringBuffer[kMaxEvents];
  size_t _head = 0;
  size_t _count = 0;
  uint32_t _totalLogged = 0;
  portMUX_TYPE _mux;

  // ── Internal helpers (formatting, string conversion) ──
  static const char* levelToString(DoorEventLevel lvl);
  static const char* eventCodeToString(DoorEventCode code);
  static DoorEventCode stringToEventCode(const char* s);
  static const char* reasonToString(DiagReasonCode code);
  static DoorEventLevel stringToLevel(const char* s);
  static void formatEventTime(const CompactDiagEvent& ev, char* out, size_t outSize);
  static uint8_t captureCurrentFlags(DoorEventLevel level);
  static String formatEventMessage(const CompactDiagEvent& ev);
};

// Global instance (Composition Root will own this; extern for backward compatibility)
extern DiagEventLogger gDiagLogger;
