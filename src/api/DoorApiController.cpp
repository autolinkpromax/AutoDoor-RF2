#include "DoorApiController.h"
#include "app/globals.h"
#include "modules/door.h"
#include "modules/webhook_hooks.h"
#include "hal/MotorActuator.h"
#include "diagnostics/DiagEventLogger.h"

void DoorApiController::registerRoutes() {
  _server.on("/api/door", HTTP_POST, [this]() {
    wrap([this]() { handleDoorPost(); });
  });

  _server.on("/api/emergency_stop", HTTP_ANY, [this]() {
    handleEmergencyStop();
  });

  _server.on("/hook/door/open", HTTP_GET, [this]() {
    wrap([this]() { handleHookDoorSlot("open"); }, true);
  });

  _server.on("/hook/door/tag_open", HTTP_GET, [this]() {
    wrap([this]() { handleHookDoorTagOpen(); }, true);
  });

  _server.on("/hook/door/stop", HTTP_GET, [this]() {
    wrap([this]() { handleHookDoorSlot("stop"); }, true);
  });

  _server.on("/hook/door/close", HTTP_GET, [this]() {
    wrap([this]() { handleHookDoorSlot("close"); }, true);
  });

  _server.on("/hook/tag-auto/toggle", HTTP_GET, [this]() {
    wrap([this]() { handleHookTagAutoToggle(); }, true);
  });
}

void DoorApiController::handleDoorPost() {
  JsonDocument body = parseBodyOrEmpty();
  const String slot = String((const char*)(body["slot"] | ""));
  if (slot != "open" && slot != "close" && slot != "stop") {
    JsonDocument err;
    err["ok"] = false;
    err["error"] = "bad_slot";
    sendJson(400, err);
    return;
  }
  gActiveTriggerTagId = "";
  const bool isManualHold = (slot == "open" || slot == "stop");
  const bool force = body["force"] | body["bypassPreStop"] | false;
  const bool ok = force ? executeDoorSlot(slot, true, isManualHold, true /* bypassPreStop */)
                        : requestDoorSlot(slot, true, isManualHold);
  if (ok && slot == "open") {
    logDoorEvent(DoorEventCode::ManualOpen, DoorEventLevel::Info, 0, "WEB");
  }
  JsonDocument doc;
  doc["ok"] = ok;
  doc["doorState"] = doorStateToString(gDoorState);
  sendJson(ok ? 200 : 409, doc);
}

void DoorApiController::handleEmergencyStop() {
  gMotor.forceShutdown();
  executeDoorSlot("stop", true);
  JsonDocument doc;
  doc["ok"] = true;
  doc["msg"] = "Emergency Stop: all relays cut off immediately";
  sendJson(200, doc);
}

bool DoorApiController::hookKeyValid() {
  ensureHookKey();
  const String key = _server.arg("key");
  return key.length() > 0 && key == gHookKey;
}

void DoorApiController::handleHookDoorSlot(const char* slot) {
  if (!hookKeyValid()) {
    JsonDocument doc;
    doc["ok"] = false;
    doc["error"] = "invalid_key";
    sendJson(403, doc);
    return;
  }
  gActiveTriggerTagId = "";

  uint32_t holdSec = 0;
  if (_server.hasArg("delay")) holdSec = _server.arg("delay").toInt();
  else if (_server.hasArg("hold")) holdSec = _server.arg("hold").toInt();
  else if (_server.hasArg("fallback")) holdSec = _server.arg("fallback").toInt();

  const bool isManualHold = (strcmp(slot, "open") == 0 && holdSec == 0) || strcmp(slot, "stop") == 0;
  const bool ok = requestDoorSlot(String(slot), true, isManualHold);
  if (ok && strcmp(slot, "open") == 0) {
    logDoorEvent(DoorEventCode::ManualOpen, DoorEventLevel::Info, 0, "WEB");
    if (holdSec > 0 && holdSec <= 86400) {
      uint32_t now = millis();
      gDoorController.isManualOpenStateRef() = false;
      gDoorController.autoCloseEngine().armPendingClose(now + (holdSec * 1000));
    }
  }
  JsonDocument doc;
  doc["ok"] = ok;
  doc["doorState"] = doorStateToString(gDoorState);
  if (holdSec > 0) doc["holdSec"] = holdSec;
  sendJson(ok ? 200 : 409, doc);
}

void DoorApiController::handleHookDoorTagOpen() {
  if (!hookKeyValid()) {
    JsonDocument doc;
    doc["ok"] = false;
    doc["error"] = "invalid_key";
    sendJson(403, doc);
    return;
  }

  // Identify tag if specified
  String tagId = _server.hasArg("tag_id") ? _server.arg("tag_id") : (_server.hasArg("tag") ? _server.arg("tag") : "");
  String tagLabel = "Webhook";
  uint32_t tagCloseDelay = 60000; // 60s fallback auto-close
  uint32_t tagSafetyDelay = gSafetyCloseDelayMs;
  uint32_t tagOpenDuration = 0;

  if (!tagId.isEmpty()) {
    TagStoreLock lock;
    Tag* t = gTagRegistry.findById(tagId);
    if (t) {
      if (!t->name.isEmpty()) tagLabel = t->name;
      else tagLabel = t->id;
      if (t->tagCloseDelayMs > 0) tagCloseDelay = t->tagCloseDelayMs;
      if (t->tagSafetyCloseDelayMs > 0) tagSafetyDelay = t->tagSafetyCloseDelayMs;
      tagOpenDuration = t->tagOpenDurationMs;
    }
  }

  // Fallback auto-close delay override if supplied
  if (_server.hasArg("delay")) {
    uint32_t dSec = _server.arg("delay").toInt();
    if (dSec > 0 && dSec <= 3600) {
      tagCloseDelay = dSec * 1000;
    }
  } else if (_server.hasArg("fallback")) {
    uint32_t fSec = _server.arg("fallback").toInt();
    if (fSec > 0 && fSec <= 3600) {
      tagCloseDelay = fSec * 1000;
    }
  } else if (_server.hasArg("hold")) {
    uint32_t hSec = _server.arg("hold").toInt();
    if (hSec > 0 && hSec <= 3600) {
      tagCloseDelay = hSec * 1000;
    }
  }

  gActiveTriggerTagId = tagId.isEmpty() ? "WEBHOOK" : tagId;
  gPrevActiveDelayMaxMs = tagCloseDelay;
  gPrevActiveSafetyDelayMs = tagSafetyDelay;
  gDoorController.tagOpenDurationMsRef() = tagOpenDuration;

  bool ok = false;
  uint32_t now = millis();

  if (gDoorState == DoorState::Open) {
    // Already open: refresh fallback auto-close timer and tag session without redundant relay pulse
    gDoorController.isManualOpenStateRef() = false;
    gDoorController.safetySensor().clearPassage();
    gDoorController.autoCloseEngine().armPendingClose(now + tagCloseDelay);
    ok = true;
  } else {
    // Closed, Closing, or Stopped: request Open with manualHold = false (Tag Auto Mode)
    ok = requestDoorSlot("open", false /* manualCommand = false */, false /* manualHold = false */, gActiveTriggerTagId.c_str());
  }

  if (ok) {
    logDiagEvent(DoorEventCode::TagOpen, DoorEventLevel::Success, DiagReasonCode::SrcWebhook, 0, 0, tagLabel.c_str());
    Serial.printf("[webhook] tag_open triggered (tag='%s', fallback=%lu ms, safetyDelay=%lu ms)\n",
                  tagLabel.c_str(), (unsigned long)tagCloseDelay, (unsigned long)tagSafetyDelay);
  }

  JsonDocument doc;
  doc["ok"] = ok;
  doc["mode"] = "tag_open";
  doc["doorState"] = doorStateToString(gDoorState);
  doc["tagId"] = gActiveTriggerTagId;
  doc["tagName"] = tagLabel;
  doc["fallbackSec"] = tagCloseDelay / 1000;
  sendJson(ok ? 200 : 409, doc);
}

void DoorApiController::handleHookTagAutoToggle() {
  TagStoreLock lock;
  if (!hookKeyValid()) {
    JsonDocument doc;
    doc["ok"] = false;
    doc["error"] = "invalid_key";
    sendJson(403, doc);
    return;
  }
  if (_server.hasArg("value")) {
    const String val = _server.arg("value");
    gTagAutoEnabled = (val == "1" || val == "true");
  } else {
    gTagAutoEnabled = !gTagAutoEnabled;
  }
  gPrefs.putBool("tagAutoEnabled", gTagAutoEnabled);
  if (!gTagAutoEnabled) {
    gPendingCloseAtMs = 0;
    gPrevAnyTagActive = false;
  }
  JsonDocument doc;
  doc["ok"] = true;
  doc["tagAutoEnabled"] = gTagAutoEnabled;
  sendJson(200, doc);
}
