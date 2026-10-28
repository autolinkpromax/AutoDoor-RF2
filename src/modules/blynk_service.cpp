#include "blynk_service.h"

#include "app/globals.h"
#include "app/config.h"
#include "modules/wifi_manager.h"
#include "modules/door.h"
#include "hal/MotorActuator.h"
#include "diagnostics/DiagEventLogger.h"

#include <WiFi.h>

#define BLYNK_TEMPLATE_ID "TMPL000000"
#define BLYNK_TEMPLATE_NAME "AutoDoor"
#define BLYNK_AUTH_TOKEN "00000000000000000000000000000000"
#define BLYNK_HEARTBEAT 30  // Extend ping interval to 30s (saves 66%-90% standby bandwidth)
#define BLYNK_NO_BUILTIN    // Reduce memory footprint
#include <BlynkSimpleEsp32.h>

namespace {

uint32_t gBlynkLastConnectAttemptMs = 0;
bool gBlynkConfigured = false;
bool gBlynkNeedForceSync = false;
String gBlynkLastDoorText = "";
int gBlynkLastSafetyState = -2;
int gBlynkLastMagState = -2;

}  // namespace

void setupBlynkFromPrefs() {
  gBlynkEnabled = gPrefs.getBool("blynkEnabled", false);
  gBlynkToken = gPrefs.getString("blynkToken", "");
  gBlynkTemplateId = gPrefs.getString("blynkTemplateId", "");
  gBlynkDeviceName = gPrefs.getString("blynkDeviceName", "AutoDoor");
  gBlynkAuthBuf[0] = '\0';
  if (gBlynkToken.length() > 0 && gBlynkToken.length() < sizeof(gBlynkAuthBuf)) {
    strncpy(gBlynkAuthBuf, gBlynkToken.c_str(), sizeof(gBlynkAuthBuf) - 1);
  }
  gBlynkConfigured = false;
  gBlynkNeedForceSync = true;
}

void stepBlynk() {
  if (gSafeMode || !gBlynkEnabled || gBlynkAuthBuf[0] == '\0' || WiFi.status() != WL_CONNECTED) {
    if (gBlynkConnected) {
      Blynk.disconnect();
      gBlynkConnected = false;
    }
    gBlynkConfigured = false;
    return;
  }

  // Staged boot: let STA settle before opening the Blynk TCP connection (peak current spacing)
  if (wifiStaUpMs() < kStagedBlynkDelayMs) return;

  const uint32_t now = millis();

  if (!gBlynkConfigured) {
    Blynk.config(gBlynkAuthBuf);
    gBlynkConfigured = true;
    gBlynkLastConnectAttemptMs = 0;
  }

  if (!Blynk.connected()) {
    gBlynkConnected = false;
    // Retry connection every 30 seconds (or immediately on boot) with low 100ms timeout to avoid loop latency
    const bool doorMoving = (gDoorState == DoorState::Opening || gDoorState == DoorState::Closing || isDoorReversing());
    if (gBlynkLastConnectAttemptMs == 0 || (uint32_t)(now - gBlynkLastConnectAttemptMs) >= 30000) {
      if (doorMoving) {
        // Do not block loop task while door is in motion
        return;
      }
      gBlynkLastConnectAttemptMs = now;
      if (WiFi.localIP() != INADDR_NONE) {
        // Gate behind motor idle to prevent blocking loop during relay pulse timing (Fix C3)
        if (!gMotor.isBusy()) {
          AD_LOG(Serial.println("[blynk] Connecting to Blynk Cloud (timeout 100ms)..."));
          Blynk.connect(100);
        }
      }
    }
    return;
  }

  if (!gBlynkConnected) {
    gBlynkConnected = true;
    gBlynkNeedForceSync = true;
  }

  // Run Blynk.run() while connected to process incoming button commands instantly (<50ms)
  // Gate behind motor idle to prevent timing interference with relay pulses (Fix C4)
  if (!gMotor.isBusy()) {
    Blynk.run();
  }

  static uint32_t lastWriteMs = 0;

  // Rate-limiting interlock: enforce at least 100ms gap between consecutive virtualWrite calls
  if ((uint32_t)(now - lastWriteMs) < 100) {
    return;
  }

  int currentSafetyState = gSafetySensorEnabled ? (gSafetySensorBlocked ? 1 : 0) : -1;

  // Physical magnetic sensor verification:
  // If magnetic sensor is enabled, door is reported as "ปิดสนิท" ONLY if magnet is actually detected.
  bool hasMagSensor = gMagSensorEnabled;
  bool isClosedPhysically = hasMagSensor
                              ? gMagSensorStableClosed
                              : (gDoorState == DoorState::Closed);

  const char* doorThai = "ปิดสนิท";
  if (gDoorState == DoorState::Opening) {
    doorThai = "กำลังเปิด";
  } else if (gDoorState == DoorState::Closing) {
    doorThai = "กำลังปิด";
  } else if (gDoorState == DoorState::Stopped) {
    doorThai = "หยุด";
  } else {
    if (hasMagSensor) {
      doorThai = isClosedPhysically ? "ปิดสนิท" : "เปิดอยู่";
    } else {
      doorThai = (gDoorState == DoorState::Closed) ? "ปิดสนิท" : "เปิดอยู่";
    }
  }

  // Periodic heartbeat sync every 30 minutes (1,800,000 ms) as a safety net
  // (Reduces standby quota consumption from ~130k down to ~1.4k messages/month, saving 97%+)
  static uint32_t lastPeriodicSyncMs = 0;
  if (lastPeriodicSyncMs == 0) {
    lastPeriodicSyncMs = now;
  } else if ((uint32_t)(now - lastPeriodicSyncMs) >= 1800000UL) {
    lastPeriodicSyncMs = now;
    gBlynkNeedForceSync = true;
  }

  // Send V4 (Door Status) when state changes or upon reconnect/heartbeat force sync
  if (gBlynkNeedForceSync || strcmp(doorThai, gBlynkLastDoorText.c_str()) != 0) {
    Blynk.virtualWrite(V4, doorThai);
    gBlynkLastDoorText = doorThai;
    lastWriteMs = now;
    gBlynkNeedForceSync = false;
    return;
  }

  // Send V3 (Magnetic Sensor Status: 1 = Closed/ปิดสนิท, 0 = Open/เปิด, -1 = Disabled)
  int currentMagState = hasMagSensor ? (isClosedPhysically ? 1 : 0) : -1;
  if (currentMagState != gBlynkLastMagState) {
    Blynk.virtualWrite(V3, currentMagState);
    gBlynkLastMagState = currentMagState;
    lastWriteMs = now;
    return;
  }

  // Send V6 (Safety Sensor) ONLY when safety state changes
  if (currentSafetyState != gBlynkLastSafetyState) {
    Blynk.virtualWrite(V6, currentSafetyState); // V6: 1 = มีสิ่งกีดขวาง, 0 = ปกติ, -1 = ปิดใช้งาน
    gBlynkLastSafetyState = currentSafetyState;
    lastWriteMs = now;
    return;
  }
}

void disconnectBlynk() {
  Blynk.disconnect();
  gBlynkConnected = false;
  gBlynkConfigured = false;
}

BLYNK_CONNECTED() {
  Serial.println("[blynk] Connected to Blynk Cloud successfully!");
  gBlynkNeedForceSync = true;
  gBlynkLastDoorText = "";
  gBlynkLastSafetyState = -2;
  gBlynkLastMagState = -2;
}

BLYNK_WRITE(V0) {
  int val = param.asInt();
  Serial.printf("[blynk] V0 (Open) rx: %d\n", val);
  if (val != 0) {
    if (requestDoorSlot("open", true, true)) {
      logDoorEvent(DoorEventCode::ManualOpen, DoorEventLevel::Info, 0, "BLYNK");
    }
  }
}

BLYNK_WRITE(V1) {
  int val = param.asInt();
  Serial.printf("[blynk] V1 (Stop) rx: %d\n", val);
  if (val != 0) {
    requestDoorSlot("stop", true, true);
  }
}

BLYNK_WRITE(V2) {
  int val = param.asInt();
  Serial.printf("[blynk] V2 (Close) rx: %d\n", val);
  if (val != 0) {
    requestDoorSlot("close", true);
  }
}
BLYNK_WRITE(V7) {
  String paramStr = param.asStr();
  Serial.printf("[blynk] V7 (Tag Auto Open) rx: '%s'\n", paramStr.c_str());
  if (paramStr.isEmpty() || paramStr == "0") return;

  // Extract optional tag_id and custom delay
  String tagId = "";
  uint32_t customDelaySec = 0;

  int sepIdx = paramStr.indexOf(':');
  if (sepIdx < 0) sepIdx = paramStr.indexOf(',');

  if (sepIdx >= 0) {
    tagId = paramStr.substring(0, sepIdx);
    customDelaySec = paramStr.substring(sepIdx + 1).toInt();
  } else {
    // Check if matches an existing Tag ID in registry
    bool found = false;
    {
      TagStoreLock lock;
      Tag* existingTag = gTagRegistry.findById(paramStr);
      if (existingTag != nullptr) {
        tagId = paramStr;
        found = true;
      }
    }
    if (!found) {
      // Check if pure number (delay in seconds)
      bool isNumber = true;
      for (unsigned int i = 0; i < paramStr.length(); i++) {
        if (!isDigit(paramStr[i])) { isNumber = false; break; }
      }
      if (isNumber) {
        uint32_t n = paramStr.toInt();
        if (n > 1) {
          customDelaySec = n;
        }
      } else {
        tagId = paramStr;
      }
    }
  }

  if (tagId == "1" || tagId.equalsIgnoreCase("auto") || tagId.equalsIgnoreCase("default")) {
    tagId = "";
  }

  String tagLabel = tagId.isEmpty() ? "Blynk Webhook" : tagId;
  uint32_t tagCloseDelay = 60000; // 60s fallback auto-close
  uint32_t tagSafetyDelay = gSafetyCloseDelayMs;
  uint32_t tagOpenDuration = 0;

  if (!tagId.isEmpty()) {
    TagStoreLock lock;
    Tag* t = gTagRegistry.findById(tagId);
    if (t) {
      tagLabel = !t->name.isEmpty() ? t->name : t->id;
      if (t->tagCloseDelayMs > 0) tagCloseDelay = t->tagCloseDelayMs;
      if (t->tagSafetyCloseDelayMs > 0) tagSafetyDelay = t->tagSafetyCloseDelayMs;
      tagOpenDuration = t->tagOpenDurationMs;
    }
  }

  if (customDelaySec > 0 && customDelaySec <= 3600) {
    tagCloseDelay = customDelaySec * 1000;
  }

  gActiveTriggerTagId = tagId.isEmpty() ? "WEBHOOK" : tagId;
  gPrevActiveDelayMaxMs = tagCloseDelay;
  gPrevActiveSafetyDelayMs = tagSafetyDelay;
  gDoorController.tagOpenDurationMsRef() = tagOpenDuration;

  uint32_t now = millis();
  bool ok = false;
  if (gDoorState == DoorState::Open) {
    gDoorController.isManualOpenStateRef() = false;
    gDoorController.safetySensor().clearPassage();
    gDoorController.autoCloseEngine().armPendingClose(now + tagCloseDelay);
    ok = true;
  } else {
    ok = requestDoorSlot("open", false /* manualCommand = false */, false /* manualHold = false */, gActiveTriggerTagId.c_str());
  }

  if (ok) {
    logDiagEvent(DoorEventCode::TagOpen, DoorEventLevel::Success, DiagReasonCode::SrcBlynk, 0, 0, tagLabel.c_str());
    Serial.printf("[blynk] tag_open executed (tag='%s', fallback=%lu ms, safetyDelay=%lu ms)\n",
                  tagLabel.c_str(), (unsigned long)tagCloseDelay, (unsigned long)tagSafetyDelay);
  }
}

// Note: V3 is read-only (Magnetic Sensor status publish only — no write handler needed)
