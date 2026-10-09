#pragma once

#include <Arduino.h>

#pragma pack(push, 1)
struct TagBeacon {
  uint8_t magic0;
  uint8_t magic1;
  uint8_t version;
  uint32_t tagId;
  uint8_t seq;
};

struct TagBeaconV2 {
  uint8_t magic0;
  uint8_t magic1;
  uint8_t version;
  uint32_t tagId;
  uint8_t seq;
  int8_t metricRssi;
};

constexpr uint8_t kTagBeaconMagic0 = 0x41;
constexpr uint8_t kTagBeaconMagic1 = 0x44;
constexpr uint8_t kTagBeaconVersionV1 = 1;
constexpr uint8_t kTagBeaconVersionV2 = 2;

struct ReceiverAnnounce {
  uint8_t magic0;
  uint8_t magic1;
  uint8_t version;
  uint8_t channel;
  uint8_t seq;
};
#pragma pack(pop)

static_assert(sizeof(TagBeacon) == 8, "TagBeacon layout mismatch");
static_assert(sizeof(TagBeaconV2) == 9, "TagBeaconV2 layout mismatch");
static_assert(sizeof(ReceiverAnnounce) == 5, "ReceiverAnnounce layout mismatch");

constexpr uint8_t kReceiverAnnounceMagic0 = 0x41;
constexpr uint8_t kReceiverAnnounceMagic1 = 0x52;
constexpr uint8_t kReceiverAnnounceVersion = 1;

struct TagRecord {
  uint32_t tagId;
  char name[20];
  uint16_t holdMs;
  uint16_t closeDelayMs;
  int8_t tagRssiMin;
  int8_t tagPassRssi;
  uint16_t tagPassCloseDelayMs;
  uint8_t enabled;
};

struct AppSettings {
  uint8_t espNowChannel;
  uint16_t relayPulseMs;
  uint8_t tagSensitivity;
  uint16_t tagCloseDelayMs;
  uint8_t tagAutoEnabled;
  uint8_t simMode;
  uint32_t rfOpenCode;
  uint32_t rfCloseCode;
  uint32_t rfStopCode;
};

struct RuntimeTagState {
  uint32_t tagId;
  uint32_t lastSeenMs;
  uint8_t seenStreak;
  int8_t rssi;
  uint8_t sourceMac[6];
  bool active;
};

enum class DoorState : uint8_t {
  Stopped = 0,
  Opening = 1,
  Closing = 2,
  Opened = 3,
  Closed = 4,
};
