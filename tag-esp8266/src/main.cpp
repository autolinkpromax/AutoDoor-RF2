#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <EEPROM.h>
#include <user_interface.h>

// ── Defaults (overridable via build flags) ──────────────────────────
#ifndef TAG_ESPNOW_CHANNEL
#define TAG_ESPNOW_CHANNEL 9
#endif

// ── Debug ───────────────────────────────────────────────────────────
#if defined(TAG_DEBUG_SERIAL) && TAG_DEBUG_SERIAL
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...) ((void)0)
#define DEBUG_PRINTLN(...) ((void)0)
#endif

// ── Timing constants ────────────────────────────────────────────────
constexpr uint32_t kBeaconIntervalScan  = 60;    // loop tick: scanning (fast hop)
constexpr uint32_t kBeaconIntervalLock  = 80;    // loop tick: locked (high responsiveness)
constexpr uint8_t  kBurstCountScan      = 5;     // sends per burst: scanning — 5 packets เพิ่มโอกาส hub ได้ยินก่อน hop ช่อง
constexpr uint8_t  kBurstCountLock      = 3;     // sends per burst: locked
constexpr uint8_t  kBootBurstCount      = 6;     // boot-time burst sends
constexpr uint32_t kBootSettleMs        = 400;   // delay before channel scan
constexpr uint32_t kReceiverTimeoutMs   = 5000;  // how long we stay locked
constexpr uint32_t kChannelHopMs        = 60;    // dwell per scanned channel
constexpr uint32_t kWakeScanWindowMs    = 3000;  // max awake before sleep
constexpr uint32_t kSleepDurationMs     = 0;     // light-sleep disabled (keep active for vehicle)
constexpr uint32_t kPowerSaveDelayMs    = 10 * 60 * 1000; // 10 min grace
constexpr uint32_t kStatLogIntervalMs   = 3000;  // status print interval
constexpr uint8_t  kChannelMin          = 1;
constexpr uint8_t  kChannelMax          = 13;

// ── Protocol magic ──────────────────────────────────────────────────
constexpr uint8_t kBeaconMagic0   = 0x41; // 'A'
constexpr uint8_t kBeaconMagic1   = 0x44; // 'D'
constexpr uint8_t kAnnounceMagic0 = 0x41; // 'A'
constexpr uint8_t kAnnounceMagic1 = 0x52; // 'R'
constexpr uint8_t kBeaconVersion  = 1;
constexpr uint8_t kAnnounceVer    = 1;

// ── Packet structures ───────────────────────────────────────────────
#pragma pack(push, 1)
struct TagBeacon {
  uint8_t  magic[2];
  uint8_t  version;
  uint32_t tagId;
  uint8_t  seq;
};

struct ReceiverAnnounce {
  uint8_t magic[2];
  uint8_t version;
  uint8_t channel;
  uint8_t reserved;
};
#pragma pack(pop)

// ── Global state ────────────────────────────────────────────────────
uint8_t         gBroadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
TagBeacon       gBeacon{};
uint8_t         gSeq            = 0;
uint8_t         gWifiChannel    = TAG_ESPNOW_CHANNEL;
uint32_t        gLastRxMs       = 0;  // last time hub was seen
uint32_t        gLastHopMs      = 0;  // last channel hop timestamp
uint8_t         gScanChannel    = 1;  // next channel to scan
volatile uint8_t  gTargetChannel  = 0;
volatile bool     gHubTick        = false;
volatile uint8_t  gDoorStatus     = 0;  // door state from hub: 0=idle, 1=opening, 2=closing, 3=about to close
volatile bool     gBuzzerEnabled  = true;
bool            gScanning       = false;
uint32_t        gLastWakeMs     = 0;  // last wake-up timestamp
uint32_t        gOkSends        = 0;
uint32_t        gFailSends      = 0;

// ── Top 3 Channel Frequency Cache ──────────────────────────────────
uint8_t gTopChannels[3] = {TAG_ESPNOW_CHANNEL, 1, 6};
uint8_t gTopCounts[3]   = {1, 0, 0};
uint8_t gScanBgCh       = 1;
uint8_t gScanStep       = 0; // 0=Top1, 1=Top2, 2=Top3, 3=Bg

// ══════════════════════════════════════════════════════════════════════
//  Internal helpers
// ══════════════════════════════════════════════════════════════════════
namespace {

// ── Weighted Top 3 Multi-Home Scan: 75% Top 3 channels, 25% background 1..13 ──
// Pattern: [Top 1] -> [Top 2] -> [Top 3] -> [Scan Ch X] -> repeat
uint8_t getNextScanChannel() {
  uint8_t ch = gTopChannels[0];
  switch (gScanStep) {
    case 0: ch = gTopChannels[0]; break;
    case 1: ch = gTopChannels[1]; break;
    case 2: ch = gTopChannels[2]; break;
    case 3:
      ch = gScanBgCh++;
      if (gScanBgCh > kChannelMax) gScanBgCh = kChannelMin;
      break;
  }
  gScanStep = (gScanStep + 1) % 4;
  if (ch < kChannelMin || ch > kChannelMax) ch = TAG_ESPNOW_CHANNEL;
  return ch;
}

void loadEepromTopChannels() {
  EEPROM.begin(16);
  if (EEPROM.read(0) == 0xA5) {
    uint8_t lastCh = EEPROM.read(1);
    if (lastCh >= kChannelMin && lastCh <= kChannelMax) {
      gWifiChannel = lastCh;
    }
    for (int i = 0; i < 3; ++i) {
      uint8_t ch = EEPROM.read(2 + i);
      if (ch >= kChannelMin && ch <= kChannelMax) {
        gTopChannels[i] = ch;
        gTopCounts[i]   = EEPROM.read(5 + i);
      }
    }
    DEBUG_PRINTF("[eeprom] loaded ch=%u Top3: #1=%u(#%u) #2=%u(#%u) #3=%u(#%u)\n",
      gWifiChannel, gTopChannels[0], gTopCounts[0], gTopChannels[1], gTopCounts[1], gTopChannels[2], gTopCounts[2]);
  } else {
    EEPROM.write(0, 0xA5);
    EEPROM.write(1, gWifiChannel);
    EEPROM.write(2, gTopChannels[0]);
    EEPROM.write(3, gTopChannels[1]);
    EEPROM.write(4, gTopChannels[2]);
    EEPROM.write(5, gTopCounts[0]);
    EEPROM.write(6, gTopCounts[1]);
    EEPROM.write(7, gTopCounts[2]);
    EEPROM.commit();
  }
}

void recordChannelHit(uint8_t channel) {
  channel = constrain(channel, kChannelMin, kChannelMax);

  int foundIdx = -1;
  for (int i = 0; i < 3; ++i) {
    if (gTopChannels[i] == channel) {
      foundIdx = i;
      break;
    }
  }

  if (foundIdx >= 0) {
    if (gTopCounts[foundIdx] < 255) gTopCounts[foundIdx]++;
  } else {
    int minIdx = 0;
    for (int i = 1; i < 3; ++i) {
      if (gTopCounts[i] < gTopCounts[minIdx]) minIdx = i;
    }
    gTopChannels[minIdx] = channel;
    gTopCounts[minIdx] = 1;
  }

  // Sort Top 3 by count descending
  for (int i = 0; i < 2; ++i) {
    for (int j = i + 1; j < 3; ++j) {
      if (gTopCounts[j] > gTopCounts[i]) {
        uint8_t tc = gTopChannels[i]; gTopChannels[i] = gTopChannels[j]; gTopChannels[j] = tc;
        uint8_t cnt = gTopCounts[i]; gTopCounts[i] = gTopCounts[j]; gTopCounts[j] = cnt;
      }
    }
  }

  if (EEPROM.read(0) != 0xA5 || EEPROM.read(1) != channel ||
      EEPROM.read(2) != gTopChannels[0] || EEPROM.read(3) != gTopChannels[1] || EEPROM.read(4) != gTopChannels[2]) {
    EEPROM.write(0, 0xA5);
    EEPROM.write(1, channel);
    EEPROM.write(2, gTopChannels[0]);
    EEPROM.write(3, gTopChannels[1]);
    EEPROM.write(4, gTopChannels[2]);
    EEPROM.write(5, gTopCounts[0]);
    EEPROM.write(6, gTopCounts[1]);
    EEPROM.write(7, gTopCounts[2]);
    EEPROM.commit();
    DEBUG_PRINTF("[ch] Top3 saved: #1=%u(#%u) #2=%u(#%u) #3=%u(#%u)\n",
      gTopChannels[0], gTopCounts[0], gTopChannels[1], gTopCounts[1], gTopChannels[2], gTopCounts[2]);
  }
}

// ── ISR (IRAM): called from WiFi irq context ────────────────────────
void IRAM_ATTR onEspNowRecv(uint8_t* mac, uint8_t* data, uint8_t len) {
  (void)mac;
  if (len < 5) return;
  if (data[0] != kAnnounceMagic0 || data[1] != kAnnounceMagic1 || data[2] != kAnnounceVer) return;
  uint8_t ch = data[3];
  if (ch >= kChannelMin && ch <= kChannelMax) {
    gTargetChannel = ch;
    uint8_t raw = data[4];
    gDoorStatus = raw & 0x7F;             // bits 0-6: door status
    gBuzzerEnabled = ((raw & 0x80) == 0); // bit 7: 0 = enabled, 1 = muted
    gHubTick = true;
  }
}

void onEspNowSend(uint8_t* mac, uint8_t status) {
  (void)mac;
  (void)status;
}

// ── Peer helpers ────────────────────────────────────────────────────
bool ensureBroadcastPeer(uint8_t channel) {
  if (esp_now_set_peer_channel(gBroadcastMac, channel) == 0) return true;
  esp_now_del_peer(gBroadcastMac);
  return esp_now_add_peer(gBroadcastMac, ESP_NOW_ROLE_COMBO, channel, nullptr, 0) == 0;
}

// ── One-time radio init (called once at boot) ───────────────────────
void initRadio() {
  system_update_cpu_freq(80);  // 80 MHz — saves ~15 mA vs 160 MHz
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setPhyMode(WIFI_PHY_MODE_11B);  // 11b (DSSS @ 1Mbps): sensitivity ดีกว่า 11N ~6-8 dB = ระยะไกลขึ้น 2-3x
  WiFi.setOutputPower(20.5);  // Max output power for ESP8285

  esp_now_init();
  esp_now_register_recv_cb(onEspNowRecv);
  esp_now_register_send_cb(onEspNowSend);
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  wifi_set_channel(gWifiChannel);
  delay(10);
  ensureBroadcastPeer(gWifiChannel);
}

// ── Brief CPU boost for TX burst, restore after ─────────────────────
inline void cpuBoost()  { system_update_cpu_freq(160); }
inline void cpuNormal() { system_update_cpu_freq(80);  }

// ── Send beacon burst ───────────────────────────────────────────────
void sendBeaconBurst(uint8_t count) {
  cpuBoost();
  for (uint8_t i = 0; i < count; ++i) {
    gBeacon.seq = gSeq++;
    if (esp_now_send(gBroadcastMac, (uint8_t*)&gBeacon, sizeof(gBeacon)) == 0) {
      ++gOkSends;
    } else {
      ++gFailSends;
    }
    delay(2);
    yield();
  }
  cpuNormal();
}

// ── Channel switching ───────────────────────────────────────────────
void applyChannel(uint8_t channel, bool persist) {
  channel = constrain(channel, kChannelMin, kChannelMax);

  if (channel != gWifiChannel) {
    [[maybe_unused]] const uint8_t prev = gWifiChannel;
    gWifiChannel = channel;
    wifi_set_channel(gWifiChannel);
    delay(5);  // 5ms เพียงพอตาม ESP8266 SDK spec (ลดจาก 10ms เพื่อลด scan blackout)
    ensureBroadcastPeer(gWifiChannel);
    DEBUG_PRINTF("[ch] %u -> %u\n", prev, gWifiChannel);
  }

  if (persist) {
    recordChannelHit(channel);
  }
}

// ── Lightweight radio restore after sleep ───────────────────────────
void restoreRadio() {
  WiFi.forceSleepWake();
  delay(1);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.setPhyMode(WIFI_PHY_MODE_11B);  // 11b ต้องตรงกับ initRadio() — ระยะไกลขึ้น 2-3x
  WiFi.setOutputPower(20.5);

  esp_now_init();
  esp_now_register_recv_cb(onEspNowRecv);
  esp_now_register_send_cb(onEspNowSend);
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  wifi_set_channel(gWifiChannel);
  delay(10);
  ensureBroadcastPeer(gWifiChannel);
}

// ── Software light-sleep ────────────────────────────────────────────
[[maybe_unused]] void enterLightSleep() {
  DEBUG_PRINTF("[pwr] sleep %lums\n", (unsigned long)kSleepDurationMs);

  esp_now_deinit();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  WiFi.forceSleepBegin();

  delay(kSleepDurationMs);

  restoreRadio();
  gLastHopMs = 0;  // allow immediate scan
  gLastWakeMs = millis();
}

// ── Hub-announce handler (called from loop) ─────────────────────────
void stepHubSync(uint32_t now) {
  if (!gHubTick) return;
  gHubTick = false;

  if (gLastRxMs == 0 || gScanning) {
    DEBUG_PRINTLN("[hub] found — lock");
    gScanning = false;
  }
  gLastRxMs = now;
  gLastWakeMs = now; // reset sleep timer

  uint8_t target = gTargetChannel;
  if (target >= kChannelMin && target <= kChannelMax) {
    applyChannel(target, true);
  }
}

// ── Channel scanner ─────────────────────────────────────────────────
void stepChannelHop(uint32_t now) {
  if (now < kBootSettleMs) return;
  if (gLastRxMs != 0 && (now - gLastRxMs) < kReceiverTimeoutMs) return;
  if (now < gLastHopMs) return; // clock skew guard

  if (!gScanning) {
    gScanning = true;
    DEBUG_PRINTF("[ch] scan start ch=%u\n", gWifiChannel);
  }
  if ((now - gLastHopMs) < kChannelHopMs) return;
  gLastHopMs = now;

  uint8_t nextCh = getNextScanChannel();
  applyChannel(nextCh, false);
}

// ── Periodic status log ─────────────────────────────────────────────
void stepStatusLog(uint32_t now) {
  static uint32_t lastLog = 0;
  if ((now - lastLog) < kStatLogIntervalMs) return;
  lastLog = now;
  DEBUG_PRINTF("[stat] ch=%u rx_age=%lu ms ok=%lu fail=%lu scan=%d Top3:#1=%u #2=%u #3=%u\n",
               gWifiChannel,
               gLastRxMs ? (unsigned long)(now - gLastRxMs) : 999999UL,
               (unsigned long)gOkSends, (unsigned long)gFailSends,
               gScanning ? 1 : 0,
               gTopChannels[0], gTopChannels[1], gTopChannels[2]);
}

// ── Power-save manager ──────────────────────────────────────────────
void stepPowerSave(uint32_t now) {
  // Keep radio awake continuously for maximum drive-in responsiveness.
  // Aggressive light-sleep is disabled to prevent 3-second blackout periods while vehicle is moving.
  (void)now;
}

// ── Buzzer sequencer ────────────────────────────────────────────────
#ifndef TAG_BUZZER_PIN
#define TAG_BUZZER_PIN 3 // GPIO3 (RX pin on ESP-01/ESP-01S, safe at boot)
#endif

constexpr uint8_t kBuzzerPin = TAG_BUZZER_PIN;

// โครงสร้าง {ความถี่ (Hz), ระยะเวลา (ms), หน่วงเวลาพัก (ms)}
const uint16_t SOUND_7ELEVEN[][3] = {
  {3500, 220, 40},   // โน้ต ติ๊ง
  {500,  750, 500}   // โน้ต หน่อง
};

enum class BuzzerPattern : uint8_t {
  None = 0,
  Opening,
  Closing
};

BuzzerPattern gCurrentPattern = BuzzerPattern::None;
uint8_t gLastDoorStatus = 0;

uint32_t gBuzzerNextToggleMs = 0;
bool gBuzzerActive = false;
uint8_t gBuzzerStep = 0;
uint8_t gBuzzerRepeatCount = 0;

void setBuzzerPattern(BuzzerPattern pattern) {
  if (gCurrentPattern == pattern) return;
  gCurrentPattern = pattern;
  gBuzzerNextToggleMs = 0;
  gBuzzerActive = false;
  gBuzzerStep = 0;
  gBuzzerRepeatCount = 0;
  noTone(kBuzzerPin);
  digitalWrite(kBuzzerPin, LOW);
}

void updateBuzzerPattern(uint32_t now) {
  bool locked = (gLastRxMs != 0 && (now - gLastRxMs) < kReceiverTimeoutMs);
  if (!locked || !gBuzzerEnabled) {
    setBuzzerPattern(BuzzerPattern::None);
    return;
  }

  uint8_t status = gDoorStatus;
  if (status != gLastDoorStatus) {
    uint8_t prev = gLastDoorStatus;
    gLastDoorStatus = status;
    DEBUG_PRINTF("[buzzer] state change: %u -> %u\n", prev, status);

    if (status == 1) {
      // Play Opening chime only when transitioning into Opening/Open from Closed/Closing/Stopped.
      // Do NOT play chime when transitioning back from Safety Blocked (status 4).
      if (prev != 1 && prev != 4) {
        setBuzzerPattern(BuzzerPattern::Opening);
      }
    } else if (status == 2 || status == 3) {
      // Play Closing chime only when transitioning into Closing / About to Close
      if (prev != 2 && prev != 3) {
        setBuzzerPattern(BuzzerPattern::Closing);
      }
    } else {
      // Any other state (0=Closed/Idle, 4=Safety Blocked, etc.) -> Silence
      setBuzzerPattern(BuzzerPattern::None);
    }
  }
}

void updateBuzzer(uint32_t now) {
  if (gCurrentPattern == BuzzerPattern::None) {
    if (gBuzzerActive) {
      noTone(kBuzzerPin);
      digitalWrite(kBuzzerPin, LOW);
      gBuzzerActive = false;
    }
    return;
  }

  if (now < gBuzzerNextToggleMs) return;

  switch (gCurrentPattern) {
    case BuzzerPattern::Opening:
      // 7-Eleven Chime: {3500Hz, 220ms, 40ms pause} -> {500Hz, 750ms} (เล่น 2 ครั้ง ห่างกัน 0.8 วินาที)
      if (gBuzzerStep == 0) {
        tone(kBuzzerPin, SOUND_7ELEVEN[0][0]);
        gBuzzerActive = true;
        gBuzzerNextToggleMs = now + SOUND_7ELEVEN[0][1];
        gBuzzerStep = 1;
      } else if (gBuzzerStep == 1) {
        noTone(kBuzzerPin);
        digitalWrite(kBuzzerPin, LOW);
        gBuzzerActive = false;
        gBuzzerNextToggleMs = now + SOUND_7ELEVEN[0][2];
        gBuzzerStep = 2;
      } else if (gBuzzerStep == 2) {
        tone(kBuzzerPin, SOUND_7ELEVEN[1][0]);
        gBuzzerActive = true;
        gBuzzerNextToggleMs = now + SOUND_7ELEVEN[1][1];
        gBuzzerStep = 3;
      } else {
        noTone(kBuzzerPin);
        digitalWrite(kBuzzerPin, LOW);
        gBuzzerActive = false;
        gBuzzerRepeatCount++;
        if (gBuzzerRepeatCount < 2) {
          gBuzzerStep = 0;
          gBuzzerNextToggleMs = now + 800; // gap 0.8s (800ms) between repeats
        } else {
          gCurrentPattern = BuzzerPattern::None;
        }
      }
      break;

    case BuzzerPattern::Closing:
      // Descending Warning Chime (High -> Low: 1318Hz -> 880Hz) - repeated 6 times
      if (gBuzzerStep == 0) {
        tone(kBuzzerPin, 1318);
        gBuzzerActive = true;
        gBuzzerNextToggleMs = now + 80;
        gBuzzerStep = 1;
      } else if (gBuzzerStep == 1) {
        noTone(kBuzzerPin);
        digitalWrite(kBuzzerPin, LOW);
        gBuzzerActive = false;
        gBuzzerNextToggleMs = now + 30;
        gBuzzerStep = 2;
      } else if (gBuzzerStep == 2) {
        tone(kBuzzerPin, 880);
        gBuzzerActive = true;
        gBuzzerNextToggleMs = now + 140;
        gBuzzerStep = 3;
      } else {
        noTone(kBuzzerPin);
        digitalWrite(kBuzzerPin, LOW);
        gBuzzerActive = false;
        gBuzzerRepeatCount++;
        if (gBuzzerRepeatCount < 6) {
          gBuzzerStep = 0;
          gBuzzerNextToggleMs = now + 120; // gap between repeats
        } else {
          gCurrentPattern = BuzzerPattern::None;
        }
      }
      break;

    default:
      noTone(kBuzzerPin);
      digitalWrite(kBuzzerPin, LOW);
      gBuzzerActive = false;
      gCurrentPattern = BuzzerPattern::None;
      break;
  }
}

}  // namespace

// ══════════════════════════════════════════════════════════════════════
//  Arduino entry points
// ══════════════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("=== AutoDoor Tag ESP-01S v3 ===");

  pinMode(kBuzzerPin, OUTPUT);
  digitalWrite(kBuzzerPin, LOW);

  loadEepromTopChannels();

  const uint32_t chipId = ESP.getChipId();
  gBeacon.magic[0] = kBeaconMagic0;
  gBeacon.magic[1] = kBeaconMagic1;
  gBeacon.version   = kBeaconVersion;
  gBeacon.tagId     = chipId;
  gBeacon.seq       = 0;

  initRadio(); // one-time init

  gLastRxMs    = 0;
  gScanChannel = 1;
  gLastWakeMs  = millis();

  // Fire boot burst immediately
  int ok = 0, fail = 0;
  for (uint8_t i = 0; i < kBootBurstCount; ++i) {
    gBeacon.seq = gSeq++;
    int err = esp_now_send(gBroadcastMac, (uint8_t*)&gBeacon, sizeof(gBeacon));
    if (err == 0) { ++ok; ++gOkSends; }
    else          { ++fail; ++gFailSends; }
    delay(2);  // 2ms = เพียงพอให้ radio flush (เทียบกับ sendBeaconBurst), ลดจาก 10ms ประหยัด 48ms/boot
  }
  Serial.printf("[boot] burst ok=%d fail=%d\n", ok, fail);

  if (fail >= kBootBurstCount) gScanning = true;
}

void loop() {
  const uint32_t now = millis();

  // ── Adaptive timing: locked → slow tick, scanning → fast tick ─────
  const bool locked = (gLastRxMs != 0 && (now - gLastRxMs) < kReceiverTimeoutMs);
  const uint32_t interval = locked ? kBeaconIntervalLock : kBeaconIntervalScan;
  const uint8_t  burst   = locked ? kBurstCountLock : kBurstCountScan;

  stepHubSync(now);
  stepChannelHop(now);
  sendBeaconBurst(burst);
  stepStatusLog(now);
  stepPowerSave(now);

  // Non-blocking wait — CPU at 80 MHz, radio on (ESP-NOW RX active)
  while ((uint32_t)(millis() - now) < interval) {
    stepHubSync(millis());
    updateBuzzerPattern(millis());
    updateBuzzer(millis());
    yield();
  }
}
