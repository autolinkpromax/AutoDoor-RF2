#pragma once

#include "core/DoorController.h"

// ── Backward Compatibility Facade (Phase 6) ──
// Inline wrappers delegating to gDoorController to ensure existing
// peripheral modules compile without disruption.

inline void initDoorCommandQueue() {
  gDoorController.initCommandQueue();
}

inline bool hasAnyActiveTag(uint32_t now) {
  return gTagRegistry.hasAnyActiveTag(now);
}

inline bool isDoorSafeToRestart() {
  return gDoorController.isSafeToRestart();
}

inline void initDoorSensorsOnBoot() {
  gDoorController.init();
}

inline bool requestDoorSlot(const String& slot, bool manualCommand, bool manualHold = false, const char* tagId = nullptr, bool bypassPreStop = false, uint32_t customCloseDelayMs = 0) {
  return gDoorController.requestSlot(slot, manualCommand, manualHold, tagId, bypassPreStop, customCloseDelayMs);
}

inline bool requestDoorAction(DoorCmdAction action, bool manualCommand, bool manualHold = false, const char* tagId = nullptr, bool bypassPreStop = false, uint32_t customCloseDelayMs = 0) {
  return gDoorController.requestAction(action, manualCommand, manualHold, tagId, bypassPreStop, customCloseDelayMs);
}

inline bool executeDoorSlot(const String& slot, bool manualCommand, bool manualHold = false, bool bypassPreStop = false) {
  return gDoorController.executeSlot(slot, manualCommand, manualHold, bypassPreStop);
}

inline void stepDoorLogic() {
  gDoorController.step();
}

inline const char* doorStateToString(DoorState s) {
  return DoorController::stateToString(s);
}

inline bool isDoorReversing() {
  return gDoorController.isReversing();
}
