#include "ReversalDeadband.h"

void ReversalDeadband::schedule(const char* slot, uint32_t targetAtMs, bool manualCommand, bool manualHold) {
  if (slot && strcmp(slot, "open") == 0) {
    _slot = "open";
  } else if (slot && strcmp(slot, "close") == 0) {
    _slot = "close";
  } else if (slot && strcmp(slot, "stop") == 0) {
    _slot = "stop";
  } else {
    _slot = "";
  }
  _targetAtMs = targetAtMs;
  _manualCommand = manualCommand;
  _manualHold = manualHold;
}

void ReversalDeadband::cancel() {
  _slot = "";
  _targetAtMs = 0;
  _manualCommand = false;
  _manualHold = false;
}

bool ReversalDeadband::consume(uint32_t now, const char*& outSlot, bool& outManual, bool& outHold) {
  if (!isDue(now)) {
    return false;
  }
  outSlot = _slot;
  outManual = _manualCommand;
  outHold = _manualHold;
  cancel();
  return true;
}

bool ReversalDeadband::consume(uint32_t now, String& outSlot, bool& outManual, bool& outHold) {
  const char* s = "";
  if (!consume(now, s, outManual, outHold)) {
    return false;
  }
  outSlot = s;
  return true;
}
