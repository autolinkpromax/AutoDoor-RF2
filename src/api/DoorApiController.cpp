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
  const bool isManualHold = (strcmp(slot, "open") == 0 || strcmp(slot, "stop") == 0);
  const bool ok = requestDoorSlot(String(slot), true, isManualHold);
  if (ok && strcmp(slot, "open") == 0) {
    logDoorEvent(DoorEventCode::ManualOpen, DoorEventLevel::Info, 0, "WEB");
  }
  JsonDocument doc;
  doc["ok"] = ok;
  doc["doorState"] = doorStateToString(gDoorState);
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
