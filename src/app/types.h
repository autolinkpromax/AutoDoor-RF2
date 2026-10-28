#pragma once

#include "app/config.h"

#include <Arduino.h>
#include <vector>

#pragma pack(push, 1)
struct TagBeacon {
  uint8_t magic[2];
  uint8_t version;
  uint32_t tagId;
  uint8_t seq;
};
#pragma pack(pop)

constexpr uint8_t kBeaconMagic0 = 0x41;
constexpr uint8_t kBeaconMagic1 = 0x44;
constexpr uint8_t kBeaconVersion = 1;

constexpr uint8_t kAnnounceMagic0 = 0x41;
constexpr uint8_t kAnnounceMagic1 = 0x52;
constexpr uint8_t kAnnounceVersion = 1;

#pragma pack(push, 1)
struct ReceiverAnnounce {
  uint8_t magic[2];
  uint8_t version;
  uint8_t channel;
  uint8_t reserved;
};
#pragma pack(pop)

enum class RfButtonRole : uint8_t {
  Auto = 0,       // Open and auto-close (normal / toggle)
  HoldOpen = 1,   // Open and stay open (disarms auto-close)
  Close = 2,      // Immediate close (cancels hold open, resets to auto)
  Stop = 3,       // Emergency stop
  AutoOpen = 4    // Open only and auto-close (press to open only, no toggle to close)
};

struct Tag {
  String id;
  String name;
  String mac;
  uint8_t macBin[6] = {0};
  uint32_t tagId = 0;
  uint32_t tagHoldMs = kDefaultTagHoldMs;
  uint32_t tagCloseDelayMs = kDefaultTagCloseDelayMs;
  int8_t tagRssiMin = kDefaultTagRssiMin;
  uint8_t rfRole = 0; // RfButtonRole: 0 = Auto, 1 = HoldOpen, 2 = Close, 3 = Stop
  uint32_t tagSafetyCloseDelayMs = kDefaultTagSafetyCloseDelayMs;
  uint32_t tagOpenDurationMs = 0; // 0 = full open default, >0 = partial open (e.g. motorcycle/pedestrian)
  String icon; // "auto", "suv", "sedan", "pickup", "bike", "scooter", "van", "rf", "bicycle", "person"
  bool scheduleEnabled = false;
  uint16_t scheduleStartMin = 0;   // 00:00 (0..1439)
  uint16_t scheduleEndMin = 1439;  // 23:59 (0..1439)
  uint32_t lastSeenMs = 0;
  int8_t lastRssi = 0;
  float smoothedRssi = -127.0f;
  bool dirty = false;
  bool webhookEnabled = false;
  bool webhookOpen = true;
  bool webhookClosing = true;
  bool webhookClose = true;
  String webhookUrl = "";
};

struct ScheduleEntry {
  String id;
  bool enabled = false;
  String action = "open";
  uint8_t hour = 0;
  uint8_t minute = 0;
  uint8_t daysMask = 0x7F;
  int lastFiredYmd = 0;
};

struct LearnState {
  volatile bool active = false;
  uint32_t deadlineMs = 0;
  String lastMac;
  uint32_t lastTagId = 0;
  String createdTagId;
};

enum class DoorState : uint8_t {
  Unknown = 0,
  Open = 1,
  Closed = 2,
  Stopped = 3,
  Opening = 4,
  Closing = 5
};

enum class DoorCmdAction : uint8_t {
  Open = 1,
  Close = 2,
  Stop = 3
};

struct DoorCommand {
  DoorCmdAction action;
  bool isManual;
  bool manualHold;
  bool bypassPreStop = false;
  char tagId[16] = {0};
  uint32_t customCloseDelayMs = 0;
};

enum class DoorEventLevel : uint8_t {
  Info = 0,
  Success = 1,
  Warn = 2,
  Danger = 3
};

enum class DoorEventCode : uint8_t {
  None = 0,

  // State & Sync (0x10 - 0x1F)
  StateClosed   = 0x10,
  MagClosed     = 0x11,
  MagOpen       = 0x12,
  LimitClosed   = 0x13,
  LimitOpen     = 0x14,

  // Control & Motor (0x20 - 0x2F)
  ManualOpen    = 0x20,
  ManualStop    = 0x21,
  ManualClose   = 0x22,
  AutoClose     = 0x23,
  CloseStep1    = 0x24,
  Reversal      = 0x25,
  DoubleClose   = 0x26,
  RetryClose    = 0x27,
  RetryFail     = 0x28,
  Failsafe      = 0x29,
  PedestrianStop= 0x2A,

  // Safety Sensor (0x30 - 0x3F)
  BeamCut       = 0x30,
  SafeAbort     = 0x31,
  PassArm       = 0x32,
  SafeClose     = 0x33,
  CloseBlock    = 0x34,
  PassSkip      = 0x35,

  // Tag & RF Events (0x40 - 0x4F)
  TagOpen       = 0x40,
  FallClose     = 0x41,
  RfOpen        = 0x42,
  RfClose       = 0x43,
  RfHold        = 0x44,
  RfUnhold      = 0x45,
  RfUnknown     = 0x46
};

enum class DiagReasonCode : uint8_t {
  None                 = 0x00,
  NormalCycle          = 0x01,
  TravelTimeout        = 0x02,
  MotorStallWarn       = 0x03,
  
  // Triggers & Origins (0x10 - 0x1F)
  SrcTagRssi           = 0x10,
  SrcRfBtn             = 0x11,
  SrcRfHold            = 0x12,
  SrcWebUi             = 0x13,
  SrcBlynk             = 0x14,
  SrcSchedule          = 0x15,
  SrcWebhook           = 0x16,

  // Safety & Obstacle (0x20 - 0x2F)
  ObstacleDuringClose  = 0x20,
  PassageCrossed       = 0x21,
  BeamStillBlocked     = 0x22,
  PassSkipSafeMode     = 0x23,
  PassSkipManual       = 0x24,
  PassSkipClosed       = 0x25,
  MidwayDirectionChange= 0x26,

  // Auto-Close & Rules (0x30 - 0x3F)
  AutoCloseFallback    = 0x30,
  AutoCloseSafety      = 0x31,
  FailsafeOvertime     = 0x32,
  RetryCloseUnlocked   = 0x33,
  RetryExhausted       = 0x34,
  DeadbandInterlock    = 0x35,

  // Suppressions (0x40 - 0x4F)
  TagSuppressedHold    = 0x40,
  ReversalInterlock    = 0x41
};

// Sensor bitmasks (1 Byte)
constexpr uint8_t SENSOR_BIT_BEAM        = (1 << 0);
constexpr uint8_t SENSOR_BIT_MAG_CLOSED  = (1 << 1);
constexpr uint8_t SENSOR_BIT_LIMIT_OPEN  = (1 << 2);
constexpr uint8_t SENSOR_BIT_LIMIT_CLOSE = (1 << 3);
constexpr uint8_t SENSOR_BIT_BUZZER      = (1 << 4);
constexpr uint8_t DOOR_STATE_SHIFT       = 5;
constexpr uint8_t DOOR_STATE_MASK        = (0x07 << DOOR_STATE_SHIFT);

// Flags bitmasks (1 Byte)
constexpr uint8_t LEVEL_MASK             = 0x03;
constexpr uint8_t FLAG_MANUAL            = (1 << 2);
constexpr uint8_t FLAG_SAFEMODE          = (1 << 3);
constexpr uint8_t FLAG_REVERSAL          = (1 << 4);
constexpr uint8_t FLAG_AUTOCLOSE_ARMED   = (1 << 5);
constexpr uint8_t FLAG_EPOCH_TIME        = (1 << 6);

// Entity Dictionary Encodings (1 Byte)
enum EntityPredefined : uint8_t {
  ENTITY_NONE      = 0,
  ENTITY_AUTO      = 1,
  ENTITY_SAFETY    = 2,
  ENTITY_REMOTE    = 3,
  ENTITY_BOOT_BTN  = 4,
  ENTITY_SAFE_MODE = 5,
  ENTITY_SCHEDULE  = 6,
  ENTITY_WEB       = 7,
  ENTITY_BLYNK     = 8,
  ENTITY_FAILSAFE  = 9,
  ENTITY_PREDEFINED_COUNT = 10,

  ENTITY_TAG_BASE  = 10,
  ENTITY_TAG_MAX   = 73,

  ENTITY_POOL_BASE = 74,
  ENTITY_POOL_MAX  = 255
};

#pragma pack(push, 1)
struct CompactDiagEvent {
  uint32_t       timestamp = 0;   // 4B: epochSec if FLAG_EPOCH_TIME, else uptime seconds
  DoorEventCode  code = DoorEventCode::None; // 1B
  DiagReasonCode reason = DiagReasonCode::None; // 1B
  uint8_t        sensorBits = 0;  // 1B: sensor snapshot & doorState
  uint8_t        flags = 0;       // 1B: level (bits 0..1), manual, safeMode, reversal, autoclose, epochTime
  uint16_t       durationMs = 0;  // 2B: duration in ms (0..65535)
  int8_t         metricVal = 0;   // 1B: RSSI or pin or small metric (-128..127)
  uint8_t        entityRef = 0;   // 1B: Entity dictionary / tag index / string pool
};
#pragma pack(pop)

static_assert(sizeof(CompactDiagEvent) == 12, "CompactDiagEvent must be exactly 12 bytes");
using CompactDoorEvent = CompactDiagEvent;

enum class AccessMethod : uint8_t {
  EspNow      = 0,
  RfRemote    = 1,
  WebUi       = 2,
  BlynkApp    = 3,
  Schedule    = 4,
  External    = 5,
  BootButton  = 6,
  Unknown     = 7
};

enum class AccessAction : uint8_t {
  AutoOpen    = 0,
  HoldOpen    = 1,
  Close       = 2,
  Stop        = 3,
  SafeAbort   = 4,
  Failsafe    = 5
};

// Predefined special entity tokens for CompactAccessRecord
constexpr uint16_t ACCESS_TAG_RF_BASE    = 4000; // 4001..4080: RF Remotes #1..#80
constexpr uint16_t ACCESS_TAG_WEB        = 65530; // Web UI Dashboard
constexpr uint16_t ACCESS_TAG_BLYNK      = 65531; // Blynk Cloud / Mobile App
constexpr uint16_t ACCESS_TAG_SCHEDULE   = 65532; // Schedule Auto Open
constexpr uint16_t ACCESS_TAG_EXTERNAL   = 65533; // External Switch / Magnetic Sensor
constexpr uint16_t ACCESS_TAG_BOOT_BTN   = 65534; // Onboard Boot Button
constexpr uint16_t ACCESS_TAG_UNKNOWN    = 65535; // Unknown Entity

// Ultra-minimal 6-byte packed binary access record:
// Exact Unix timestamp (4B) + 16-bit Tag ID (2B)
#pragma pack(push, 1)
struct CompactAccessRecord {
  uint32_t epochSec = 0;   // 4B: Unix timestamp (วินาทีสากล ระบุ วัน/เดือน/ปี/ชั่วโมง/นาที/วินาที)
  uint16_t tagId = 0;      // 2B: รหัสแท็ก/รีโมท (0..65535)
};
#pragma pack(pop)
static_assert(sizeof(CompactAccessRecord) == 6, "CompactAccessRecord must be exactly 6 bytes");

// Unpacked in-memory representation for API / JSON presentation
struct AccessLogRecord {
  uint32_t epochSec = 0;
  uint16_t tagId = 0;
  char     who[48] = {0};
  char     type[16] = {0};     // "rf", "car", "bike", "web", "app"
  char     subtitle[48] = {0}; // "รีโมทพกพา", "รถยนต์ SUV", "รถมอเตอร์ไซค์"
};

