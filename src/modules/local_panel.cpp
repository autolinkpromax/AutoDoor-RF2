#include "local_panel.h"

#include "app/config.h"
#if AUTODOOR_FEATURE_LOCAL_PANEL
#include "app/globals.h"
#include "modules/door.h"
#include "modules/esp_now_runtime.h"
#include "tags/TagRegistry.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include <cstring>

namespace {

constexpr uint8_t kOledWidth = 128;
constexpr uint8_t kOledHeight = 64;
constexpr uint8_t kOledAddr = 0x3C;
constexpr uint32_t kOledRefreshMs = 250;
constexpr uint32_t kClosingBlinkMs = 300;
constexpr uint16_t kBuzzerFreqHz = 2700;
constexpr uint32_t kBeepOnMs = 60;
constexpr uint32_t kBeepGapMs = 80;
constexpr uint32_t kBeepPauseMs = 450;

Adafruit_SSD1306 gOled(kOledWidth, kOledHeight, &Wire, -1);
bool gOledReady = false;
bool gBuzzerReady = false;
uint32_t gLastOledMs = 0;
uint32_t gLastClosingBlinkMs = 0;
bool gClosingBlinkOn = false;
uint32_t gBuzzerPhaseMs = 0;
uint8_t gBuzzerPhase = 0;

bool isDoorOpenForPanel() {
  return gDoorState == DoorState::Open || gDoorState == DoorState::Opening;
}

bool isDoorClosedForPanel() {
  return gDoorState == DoorState::Closed || gDoorState == DoorState::Closing;
}

const char* doorStateLabelShort() {
  switch (gDoorState) {
    case DoorState::Open:
      return "OPEN";
    case DoorState::Opening:
      return "OPENING";
    case DoorState::Closed:
      return "CLOSED";
    case DoorState::Closing:
      return "CLOSING";
    case DoorState::Stopped:
      return "STOP";
    default:
      return "CLOSED";
  }
}

struct TagPanelInfo {
  bool hasTag = false;
  String tagName;
  bool isRf = false;
  uint32_t tagHoldMs = 0;
  int8_t tagRssiMin = 0;
  int8_t tagPassRssi = 0;
  bool active = false;
  bool hasRssi = false;
  int8_t bestRssi = -127;
  uint32_t holdRemainMs = 0;
  const char* zone = "out";
};

const char* zoneLabelShort(const char* zone) {
  if (!zone) return "OUT";
  if (strcmp(zone, "in_range") == 0) return "IN RANGE";
  if (strcmp(zone, "approaching") == 0) return "APPROACH";
  if (strcmp(zone, "departing") == 0) return "DEPART";
  return "OUT";
}

TagPanelInfo collectTagPanelInfo(uint32_t now) {
  TagStoreLock lock;
  TagPanelInfo info;
  for (const auto& t : gTagRegistry.tags()) {
    const bool active = isTagActive(t, now);
    const char* zone = tagRssiZone(t, now, active);
    const bool detected = active || strcmp(zone, "out") != 0;
    if (!detected) continue;

    const bool stronger = !info.hasRssi || t.lastRssi > info.bestRssi;
    if (stronger) {
      info.hasTag = true;
      info.tagName = t.name;
      info.isRf = isRfTag(t);
      info.tagHoldMs = info.isRf ? 5000 : t.tagHoldMs;
      info.tagRssiMin = t.tagRssiMin;
      info.tagPassRssi = t.tagPassRssi;
      info.bestRssi = t.lastRssi;
      info.hasRssi = t.lastSeenMs != 0;
      info.zone = zone;
      info.active = active;
      if (active && t.lastSeenMs != 0) {
        const uint32_t age = (uint32_t)(now - t.lastSeenMs);
        const uint32_t holdMs = info.tagHoldMs;
        info.holdRemainMs = (age < holdMs) ? (holdMs - age) : 0;
      } else {
        info.holdRemainMs = 0;
      }
    }
  }
  return info;
}

bool isDoorClosingSequence(uint32_t now, const TagPanelInfo& info) {
  if (info.active) return false;
  if (gDoorState == DoorState::Closing) return true;
  if (gPendingCloseAtMs != 0 && (int32_t)(now - gPendingCloseAtMs) < 0) return true;
  return false;
}

uint32_t gFinishLineNextToggleMs = 0;
uint8_t gFinishLineStep = 0;
bool gFinishLineActive = false;

uint32_t gSafeSoundNextToggleMs = 0;
uint8_t gSafeSoundStep = 0;
bool gSafeSoundActive = false;

void buzzerToneFreq(uint32_t freq) {
  if (!gBuzzerReady) return;
  ledcWriteTone(kBuzzerPin, freq);
}

void buzzerTone(bool on) {
  buzzerToneFreq(on ? kBuzzerFreqHz : 0);
}

bool shouldBuzzerBeep(const TagPanelInfo& info) {
  if (!gTagAutoEnabled || !info.active || !info.hasTag || !info.hasRssi) return false;
  return info.bestRssi >= info.tagPassRssi;
}

void stepBuzzer(uint32_t now, const TagPanelInfo& info) {
  if (gSafeSoundActive) {
    if (now < gSafeSoundNextToggleMs) return;
    switch (gSafeSoundStep) {
      case 1:
        buzzerToneFreq(2700);
        gSafeSoundNextToggleMs = now + 120;
        gSafeSoundStep = 2;
        break;
      case 2:
        buzzerToneFreq(0);
        gSafeSoundNextToggleMs = now + 80;
        gSafeSoundStep = 3;
        break;
      case 3:
        buzzerToneFreq(2700);
        gSafeSoundNextToggleMs = now + 120;
        gSafeSoundStep = 4;
        break;
      default:
        buzzerToneFreq(0);
        gSafeSoundActive = false;
        gSafeSoundStep = 0;
        break;
    }
    return;
  }

  if (gFinishLineActive) {
    if (now < gFinishLineNextToggleMs) return;
    switch (gFinishLineStep) {
      case 1:
        buzzerToneFreq(784);
        gFinishLineNextToggleMs = now + 90;
        gFinishLineStep = 2;
        break;
      case 2:
        buzzerToneFreq(1047);
        gFinishLineNextToggleMs = now + 90;
        gFinishLineStep = 3;
        break;
      case 3:
        buzzerToneFreq(1318);
        gFinishLineNextToggleMs = now + 90;
        gFinishLineStep = 4;
        break;
      case 4:
        buzzerToneFreq(1568);
        gFinishLineNextToggleMs = now + 120;
        gFinishLineStep = 5;
        break;
      case 5:
        buzzerToneFreq(0);
        gFinishLineNextToggleMs = now + 30;
        gFinishLineStep = 6;
        break;
      case 6:
        buzzerToneFreq(2093);
        gFinishLineNextToggleMs = now + 350;
        gFinishLineStep = 7;
        break;
      default:
        buzzerToneFreq(0);
        gFinishLineActive = false;
        gFinishLineStep = 0;
        break;
    }
    return;
  }

  if (!shouldBuzzerBeep(info)) {
    buzzerTone(false);
    gBuzzerPhase = 0;
    gBuzzerPhaseMs = now;
    return;
  }

  const uint32_t elapsed = (uint32_t)(now - gBuzzerPhaseMs);
  switch (gBuzzerPhase) {
    case 0:
      buzzerTone(true);
      if (elapsed >= kBeepOnMs) {
        gBuzzerPhase = 1;
        gBuzzerPhaseMs = now;
      }
      break;
    case 1:
      buzzerTone(false);
      if (elapsed >= kBeepGapMs) {
        gBuzzerPhase = 2;
        gBuzzerPhaseMs = now;
      }
      break;
    case 2:
      buzzerTone(true);
      if (elapsed >= kBeepOnMs) {
        gBuzzerPhase = 3;
        gBuzzerPhaseMs = now;
      }
      break;
    default:
      buzzerTone(false);
      if (elapsed >= kBeepPauseMs) {
        gBuzzerPhase = 0;
        gBuzzerPhaseMs = now;
      }
      break;
  }
}

void updatePanelLeds(uint32_t now, const TagPanelInfo& info) {
  if (kTagDetectLedPin != 255) {
    digitalWrite(kTagDetectLedPin, info.active ? HIGH : LOW);
  }

  if (kDoorClosingBlinkPin == 255) return;

  if (!isDoorClosingSequence(now, info)) {
    digitalWrite(kDoorClosingBlinkPin, LOW);
    gClosingBlinkOn = false;
    return;
  }
  if ((uint32_t)(now - gLastClosingBlinkMs) >= kClosingBlinkMs) {
    gLastClosingBlinkMs = now;
    gClosingBlinkOn = !gClosingBlinkOn;
    digitalWrite(kDoorClosingBlinkPin, gClosingBlinkOn ? HIGH : LOW);
  }
}

void printTruncated(Adafruit_SSD1306& oled, const String& s, uint8_t maxChars) {
  String t = s;
  if (t.length() > maxChars) {
    t = t.substring(0, maxChars - 1);
    t += '~';
  }
  oled.print(t.c_str());
}

void drawOled(const TagPanelInfo& info) {
  if (!gOledReady) return;

  if (gSafeMode) {
    gOled.clearDisplay();
    gOled.setTextSize(2);
    gOled.setTextColor(SSD1306_WHITE);
    gOled.setCursor(10, 15);
    gOled.print("SAFE MODE");
    gOled.setTextSize(1);
    gOled.setCursor(10, 45);
    gOled.print("Door Stopped");
    gOled.display();
    return;
  }

  gOled.clearDisplay();
  gOled.setTextSize(1);
  gOled.setTextColor(SSD1306_WHITE);

  gOled.setCursor(0, 0);
  gOled.print("Door ");
  gOled.println(doorStateLabelShort());
  gOled.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  if (info.hasTag) {
    gOled.setCursor(0, 14);
    printTruncated(gOled, info.tagName, 16);

    gOled.setCursor(0, 24);
    gOled.print(zoneLabelShort(info.zone));

    gOled.setCursor(0, 36);
    if (info.hasRssi) {
      gOled.setTextSize(2);
      gOled.print((int)info.bestRssi);
      gOled.setTextSize(1);
      gOled.println(" dBm");
    } else {
      gOled.println("RSSI ---");
    }

    gOled.setCursor(0, 54);
    if (info.active && info.holdRemainMs > 0) {
      const uint32_t holdMs = info.tagHoldMs;
      gOled.print("Lost ");
      gOled.print((unsigned)((info.holdRemainMs + 999) / 1000));
      gOled.print("s / hold ");
      gOled.print((unsigned)((holdMs + 999) / 1000));
      gOled.print("s");
    } else if (info.hasRssi && info.bestRssi < info.tagPassRssi) {
      gOled.print("PASSED < ");
      gOled.print((int)info.tagPassRssi);
    } else {
      gOled.print("Open >= ");
      gOled.print((int)info.tagRssiMin);
      gOled.print(" dBm");
    }
  } else {
    gOled.setCursor(0, 22);
    gOled.println("No tag");
    gOled.setCursor(0, 36);
    gOled.print("CH ");
    gOled.print((unsigned)currentEspNowChannel());
  }

  if (gSafeMode) {
    gOled.setCursor(96, 0);
    gOled.print("SAFE");
  } else if (info.hasTag && isDoorClosedForPanel() && info.active) {
    gOled.setCursor(96, 14);
    gOled.print("WAIT");
  }

  gOled.display();
}

}  // namespace

void triggerGateFinishLineSound() {
  gFinishLineStep = 1;
  gFinishLineNextToggleMs = millis();
  gFinishLineActive = true;
}

void triggerSafeModeAlertSound() {
  gSafeSoundStep = 1;
  gSafeSoundNextToggleMs = millis();
  gSafeSoundActive = true;
}

void initLocalPanel() {
  if (kTagDetectLedPin != 255) {
    pinMode(kTagDetectLedPin, OUTPUT);
    digitalWrite(kTagDetectLedPin, LOW);
  }
  if (kDoorClosingBlinkPin != 255) {
    pinMode(kDoorClosingBlinkPin, OUTPUT);
    digitalWrite(kDoorClosingBlinkPin, LOW);
  }

  if (kBuzzerPin != 255) {
    gBuzzerReady = ledcAttach(kBuzzerPin, kBuzzerFreqHz, 8);
    buzzerTone(false);
    if (gBuzzerReady) {
      Serial.println("[panel] buzzer ready (tagPassRssi per tag)");
    } else {
      Serial.println("[panel] buzzer init failed");
    }
  }

  Wire.begin(kOledSdaPin, kOledSclPin);
  gOledReady = gOled.begin(SSD1306_SWITCHCAPVCC, kOledAddr);
  if (!gOledReady) {
    Serial.println("[panel] OLED init failed");
    return;
  }
  gOled.setRotation(0);
  gOled.cp437(true);
  gOled.clearDisplay();
  gOled.display();
  Serial.printf("[panel] OLED ready SDA=%u SCL=%u\n", (unsigned)kOledSdaPin, (unsigned)kOledSclPin);
}

void stepLocalPanel() {
  const uint32_t now = millis();
  static uint32_t lastPanelStepMs = 0;
  if ((uint32_t)(now - lastPanelStepMs) < 20) return;
  lastPanelStepMs = now;

  const TagPanelInfo info = collectTagPanelInfo(now);
  updatePanelLeds(now, info);
  stepBuzzer(now, info);

  if (!gOledReady) return;
  if ((uint32_t)(now - gLastOledMs) < kOledRefreshMs) return;
  gLastOledMs = now;
  drawOled(info);
}
#else

void initLocalPanel() {
  if (kBuzzerPin != 255) {
    pinMode(kBuzzerPin, OUTPUT);
  }
}

void stepLocalPanel() {}

void triggerGateFinishLineSound() {}

void triggerSafeModeAlertSound() {
  if (kBuzzerPin != 255) {
    tone(kBuzzerPin, 2700, 100);
    delay(150);
    tone(kBuzzerPin, 2700, 100);
  }
}

#endif
