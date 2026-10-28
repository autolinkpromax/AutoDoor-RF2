#pragma once

#include "config.h"
#include "types.h"

#include <Preferences.h>
#include <WebServer.h>
#include <vector>

extern WebServer gServer;
extern Preferences gPrefs;

#include "tags/TagRegistry.h"

extern std::vector<Tag>& gTags;
extern LearnState gLearn;

extern DoorState& gDoorState;
extern uint32_t& gDoorTransitionEndMs;
extern bool gPrevAnyTagActive;
extern uint32_t& gPendingCloseAtMs;
extern uint32_t gPrevActiveDelayMaxMs;
extern uint32_t gPrevActiveSafetyDelayMs;
extern String gActiveTriggerTagId;
extern bool& gIsManualOpenState;
extern bool& gAutoCloseArmed;
extern bool& gTagOpenSuppressed;
extern std::vector<String>& gTagBaselineActiveIds;

extern bool gApEnabled;
extern uint32_t gApLastHadStationsMs;
extern uint32_t gLastDoorAutoTxMs;

extern bool& gSafeMode;
extern bool& gSimMode;
extern bool& gMotorReverse;
extern uint32_t& gRelayPulseMs;
extern uint8_t& gEspNowChannel;
extern bool& gEspNowDisabled;

extern uint32_t gBootMs;
extern bool gSystemBootReady;
extern uint32_t gWifiScanStartMs;

extern bool gWifiStatic;
extern bool gTimeSynced;

extern std::vector<ScheduleEntry> gSchedules;
extern uint32_t gLastScheduleCheckMs;

extern bool gBlynkEnabled;
extern bool gBlynkConnected;
extern String gBlynkToken;
extern String gBlynkTemplateId;
extern String gBlynkDeviceName;
extern char gBlynkAuthBuf[64];

extern bool& gTagAutoEnabled;
extern bool& gTagBuzzerEnabled;

extern bool& gAutoRebootEnabled;
extern uint8_t& gAutoRebootHour;
extern uint8_t& gAutoRebootMinute;

extern String& gDeviceName;
extern String gWebPassword;

extern bool& gMagSensorEnabled;
extern bool gMagSensorStableClosed;
extern bool& gBuiltinSensorsEnabled;
extern bool& gBuiltinMagSwap;
extern bool gBuiltinMagOpen;
extern bool gBuiltinMagClose;
extern bool& gSafetySensorEnabled;
extern bool gSafetySensorBlocked;
extern uint32_t& gSafetyCloseDelayMs;
extern uint32_t& gSafetyReverseDelayMs;
extern uint32_t& gSafetyDebounceMs;
extern bool& gRemotePassageClose;
extern uint32_t& gRemotePassageDelayMs;
extern uint32_t& gSafetyCloseAtMs;
extern uint32_t& gDoorTransitionMs;
extern uint8_t& gMagSensorType;
extern uint8_t& gSafetySensorType;
extern uint32_t& gLastCloseOrClosingMs;

extern bool& gAutoRetryCloseEnabled;
extern uint8_t& gMaxCloseRetryCount;
extern uint32_t& gCloseRetryDelayMs;
extern uint8_t& gCloseRetryCount;
extern bool& gAutoCloseAborted;

extern bool& gDoubleClosePulseEnabled;
extern uint32_t& gDoubleCloseAtMs;

extern uint32_t& gTagOpenDurationMs;
extern uint32_t& gTagOpenStopAtMs;

// ── DiagEventLogger (OOP refactor Phase 1) ──
// Event logging is now encapsulated in DiagEventLogger class.
// The struct and free-function wrappers below maintain backward compatibility
// so that existing callers (door.cpp, http_api.cpp, etc.) compile unchanged.

#include "diagnostics/DiagEventLogger.h"

struct DoorEventLogEntry {
  char timeStr[12];
  char code[16];
  char level[10];
  char reasonStr[48];
  char message[256];
  char entityId[48];
  uint32_t timestampMs = 0;
  uint32_t epochSec = 0;
  uint16_t durationMs = 0;
  int16_t metricVal = 0;
  uint8_t sensorBits = 0;
  uint8_t flags = 0;
};

// Backward-compatible free-function wrappers (delegate to gDiagLogger)
inline void logDiagEvent(DoorEventCode code,
                  DoorEventLevel level = DoorEventLevel::Info,
                  DiagReasonCode reason = DiagReasonCode::None,
                  uint16_t durationMs = 0,
                  int16_t metricVal = 0,
                  const char* entity = nullptr,
                  uint8_t customSensorBits = 0xFF) {
  gDiagLogger.logDiagEvent(code, level, reason, durationMs, metricVal, entity, customSensorBits);
}

inline void logDoorEvent(DoorEventCode code, DoorEventLevel level = DoorEventLevel::Info, uint16_t param = 0, const char* extra = nullptr) {
  gDiagLogger.logDoorEvent(code, level, param, extra);
}
inline void logDoorEvent(DoorEventCode code, DoorEventLevel level, uint16_t param, const String& extra) {
  gDiagLogger.logDoorEvent(code, level, param, extra);
}
inline void logDoorEvent(const char* code, const char* level, const char* msg) {
  gDiagLogger.logDoorEvent(code, level, msg);
}
inline void logDoorEvent(const char* code, const char* level, const String& msg) {
  gDiagLogger.logDoorEvent(code, level, msg);
}

inline std::vector<CompactDiagEvent> getRecentCompactDoorEvents(size_t maxCount = 1000) {
  return gDiagLogger.getRecentCompactDoorEvents(maxCount);
}
inline std::vector<DoorEventLogEntry> getRecentDoorEvents(size_t maxCount = 1000) {
  return gDiagLogger.getRecentDoorEvents(maxCount);
}
inline DoorEventLogEntry formatEventEntry(const CompactDiagEvent& ev) {
  return gDiagLogger.formatEventEntry(ev);
}
inline void clearDoorEvents() {
  gDiagLogger.clear();
}
inline uint32_t getTotalEventsLogged() {
  return gDiagLogger.totalLogged();
}
inline uint8_t captureCurrentSensorBits() {
  return DiagEventLogger::captureCurrentSensorBits();
}
inline void copyUtf8Safe(char* dst, const char* src, size_t dstSize) {
  DiagEventLogger::copyUtf8Safe(dst, src, dstSize);
}

