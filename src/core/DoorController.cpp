#include "DoorController.h"

#include "app/config.h"
#include "app/globals.h"
#include "modules/esp_now_runtime.h"

#include <algorithm>

DoorController gDoorController;

// Global reference bindings to guarantee backward compatibility with legacy callers
DoorState& gDoorState = gDoorController.stateRef();
uint32_t& gDoorTransitionEndMs = gDoorController.transitionEndMsRef();
uint32_t& gLastCloseOrClosingMs = gDoorController.lastCloseOrClosingMsRef();
uint8_t& gCloseRetryCount = gDoorController.closeRetryCountRef();
bool& gIsManualOpenState = gDoorController.isManualOpenStateRef();
uint32_t& gDoubleCloseAtMs = gDoorController.doubleCloseAtMsRef();
uint32_t& gTagOpenDurationMs = gDoorController.tagOpenDurationMsRef();
uint32_t& gTagOpenStopAtMs = gDoorController.tagOpenStopAtMsRef();

namespace {
constexpr uint32_t kSuppressionReleaseIdleMs = 1200;
}

DoorController::DoorController(MotorActuator& motor,
                               TagRegistry& tagRegistry,
                               AutoCloseEngine& autoClose)
  : _motor(motor),
    _tagRegistry(tagRegistry),
    _autoClose(autoClose) {
}

void DoorController::init() {
  _magSensor.init();
  _safetySensor.init();

  _autoClose.setOnPassageReset([]() {
    gDoorController.safetySensor().clearPassage();
  });
  _autoClose.setOnFallbackSuccess([]() {
    gActiveTriggerTagId = "";
    gDoorController.safetySensor().clearPassage();
  });

  // Internal pull-up and hardware RC debounce filter settling delay
  delay(50);

  const uint32_t now = millis();
  _doorState = DoorState::Closed;
  _doorOpenedAtMs = 0;
  _doorTransitionEndMs = 0;
  _reversal.cancel();
  _closeRetryCount = 0;
  _closeRetryAtMs = 0;
  _isManualOpenState = false;
  gActiveTriggerTagId = "";
  _safetySensor.clearPassage();
  _tagRegistry.setSuppressed(true);

  if (gMagSensorEnabled) {
    bool rawClosed = _magSensor.multiSampleExtMag(gMagSensorType);
    if (rawClosed) {
      noteDoorClosedSync(now);
    } else {
      _doorState = DoorState::Open;
      _doorOpenedAtMs = now;
      _doorMotionStoppedAtMs = now;
      if (!gSafeMode && gTagAutoEnabled) {
        _autoClose.armPendingClose(now + 5000);
      }
    }
    Serial.printf("[door] boot sensor ground truth: ext mag %s -> Door %s\n",
                  rawClosed ? "CLOSED" : "OPEN",
                  stateToString(_doorState));
  } else {
    noteDoorClosedSync(now);
    Serial.println("[door] boot ground truth: default CLOSED (auto mode restored)");
  }

  if (gSafetySensorEnabled) {
    _safetySensor.multiSample(gSafetySensorType);
  }
}

void DoorController::initCommandQueue() {
  if (_cmdQueue == nullptr) {
    _cmdQueue = xQueueCreate(16, sizeof(DoorCommand));
  }
}

bool DoorController::requestAction(DoorCmdAction action, bool manualCommand, bool manualHold, const char* tagId) {
  if (tagId && tagId[0] != '\0') {
    TagStoreLock lock;
    Tag* t = _tagRegistry.findById(tagId);
    if (t) {
      gActiveTriggerTagId = t->id;
      gPrevActiveDelayMaxMs = (t->tagCloseDelayMs > 0) ? t->tagCloseDelayMs : kDefaultTagCloseDelayMs;
      gPrevActiveSafetyDelayMs = (t->tagSafetyCloseDelayMs > 0) ? t->tagSafetyCloseDelayMs : gSafetyCloseDelayMs;
      _tagOpenDurationMs = t->tagOpenDurationMs;
    }
  } else if (manualCommand) {
    gActiveTriggerTagId = "";
  }
  initCommandQueue();
  if (_cmdQueue == nullptr) return false;
  DoorCommand cmd;
  cmd.action = action;
  cmd.isManual = manualCommand;
  cmd.manualHold = manualHold;
  if (tagId && tagId[0] != '\0') {
    strncpy(cmd.tagId, tagId, sizeof(cmd.tagId) - 1);
    cmd.tagId[sizeof(cmd.tagId) - 1] = '\0';
  } else {
    cmd.tagId[0] = '\0';
  }
  return xQueueSend(_cmdQueue, &cmd, 0) == pdTRUE;
}

bool DoorController::requestSlot(const char* slot, bool manualCommand, bool manualHold, const char* tagId) {
  if (!slot) return false;
  if (strcmp(slot, "open") == 0) return requestAction(DoorCmdAction::Open, manualCommand, manualHold, tagId);
  if (strcmp(slot, "close") == 0) return requestAction(DoorCmdAction::Close, manualCommand, manualHold, tagId);
  if (strcmp(slot, "stop") == 0) return requestAction(DoorCmdAction::Stop, manualCommand, manualHold, tagId);
  return false;
}

void DoorController::applyDoorCommandState(const char* slot, uint32_t now) {
  if (!slot) return;
  DoorState prev = _doorState;
  if (strcmp(slot, "open") == 0) {
    if (_doorState != DoorState::Open) {
      _doorState = DoorState::Opening;
      _doorTransitionEndMs = now + gDoorTransitionMs;
      _doorMotionStoppedAtMs = 0;
      if (_doorOpenedAtMs == 0) {
        _doorOpenedAtMs = now;
      }
    } else {
      // Door is already fully Open: maintain Open state, do NOT downgrade to Opening
      _doorMotionStoppedAtMs = 0;
      if (_doorOpenedAtMs == 0) {
        _doorOpenedAtMs = now;
      }
    }
  } else if (strcmp(slot, "close") == 0) {
    _doorState = DoorState::Closing;
    _doorTransitionEndMs = now + gDoorTransitionMs;
    _doorOpenedAtMs = 0;
    _doorMotionStoppedAtMs = 0;
    _lastCloseOrClosingMs = now;
  } else if (strcmp(slot, "stop") == 0) {
    _doorState = DoorState::Stopped;
    _doorTransitionEndMs = 0;
    _doorOpenedAtMs = 0;
    _doorMotionStoppedAtMs = now;
  }

  if (prev != _doorState) {
    for (auto* l : _listeners) {
      if (l) l->onDoorStateChanged(prev, _doorState);
    }
  }
  triggerImmediateAnnounce();
}

bool DoorController::pulseSlot(const char* slot, uint32_t durationMs) {
  if (!slot) return false;
  if (strcmp(slot, "open") == 0) return _motor.pulseOpen(durationMs);
  if (strcmp(slot, "close") == 0) return _motor.pulseClose(durationMs);
  if (strcmp(slot, "stop") == 0) return _motor.pulseStop(durationMs);
  return false;
}

uint32_t DoorController::getEffectiveSafetyCloseDelay(uint32_t now) {
  return _tagRegistry.getEffectiveSafetyCloseDelay(now, gActiveTriggerTagId, gPrevActiveSafetyDelayMs, gSafetyCloseDelayMs);
}

void DoorController::noteDoorClosedSync(uint32_t now) {
  uint32_t closeDurMs = (_lastCloseOrClosingMs > 0) ? (uint32_t)(now - _lastCloseOrClosingMs) : 0;
  DoorState prev = _doorState;
  _doorState = DoorState::Closed;
  _doorTransitionEndMs = 0;
  _doorOpenedAtMs = 0;
  _autoClose.disarmAll();
  _reversal.cancel();
  _closeRetryCount = 0;
  _closeRetryAtMs = 0;
  _autoClose.resetCircuitBreaker();
  _lastCloseOrClosingMs = now;
  _isManualOpenState = false;
  gActiveTriggerTagId = "";
  _safetySensor.clearPassage();
  _lastSafeAbortMs = 0;
  _tagOpenStopAtMs = 0;
  _tagOpenDurationMs = 0;
  _doorMotionStoppedAtMs = now;

  _tagRegistry.snapshotBaseline(now);
  _suppressionReleaseAtMs = 0;
  triggerImmediateAnnounce();

  if (prev != _doorState) {
    for (auto* l : _listeners) {
      if (l) {
        l->onDoorStateChanged(prev, _doorState);
        l->onDoorClosed();
      }
    }
  }

  logDiagEvent(DoorEventCode::StateClosed,
               DoorEventLevel::Info,
               DiagReasonCode::NormalCycle,
               (uint16_t)min((uint32_t)65535, closeDurMs),
               (int16_t)(ESP.getFreeHeap() / 1024),
               "AUTO");
}

void DoorController::noteClosingStarted(uint32_t now) {
  _autoClose.disarmAll();
  _isManualOpenState = false;
  _safetySensor.clearPassage();
  _tagOpenStopAtMs = 0;
  gActiveTriggerTagId = "";

  _tagRegistry.snapshotBaseline(now);
  _suppressionReleaseAtMs = 0;
}

bool DoorController::executeOpen(bool manualCommand, bool manualHold, bool bypassPreStop, uint32_t now) {
  _closeRetryCount = 0;
  _closeRetryAtMs = 0;
  _doubleCloseAtMs = 0;

  // If already opening, ignore duplicate auto requests (allow manual re-pulse)
  if (_doorState == DoorState::Opening && !manualCommand) {
    return true;
  }
  // If an open reversal sequence is already running, avoid restarting it
  if (_reversal.isReversingToOpen()) {
    return true;
  }

  // If currently Closing: Stop first (GPIO 6+7 = HIGH 1200ms), apply 1.2s settling deadband (2.4s total), then reverse to Open
  if (_doorState == DoorState::Closing && !bypassPreStop) {
    pulseSlot("stop");
    _lastStopPulseMs = now;
    _doorMotionStoppedAtMs = now;
    _doorState = DoorState::Stopped;
    _doorTransitionEndMs = 0;
    _reversal.schedule("open", now + kStopPulseMs + kReverseDeadbandMs, manualCommand, manualHold);
    _autoClose.disarmAll();
    _doubleCloseAtMs = 0;
    _tagOpenStopAtMs = 0;
    _tagRegistry.clearSuppression();
    _isManualOpenState = manualCommand && manualHold;
    triggerImmediateAnnounce();
    Serial.printf("[door] OPEN interrupted CLOSING -> STOP (GPIO 6+7 = HIGH %lu ms) with %lu ms deadband before OPEN\n",
                  (unsigned long)kStopPulseMs, (unsigned long)kReverseDeadbandMs);
    logDoorEvent(DoorEventCode::Reversal, DoorEventLevel::Warn, 0);
    return true;
  }

  // If currently Stopped: enforce settling deadband if STOP pulse or settling time is still active
  const uint32_t deadbandMs = kStopPulseMs + kReverseDeadbandMs;
  if (_doorState == DoorState::Stopped && !bypassPreStop && _doorMotionStoppedAtMs != 0 &&
      (int32_t)(now - _doorMotionStoppedAtMs) < (int32_t)deadbandMs) {
    _reversal.schedule("open", _doorMotionStoppedAtMs + deadbandMs, manualCommand, manualHold);
    _autoClose.disarmAll();
    _tagOpenStopAtMs = 0;
    _doubleCloseAtMs = 0;
    triggerImmediateAnnounce();
    Serial.printf("[door] OPEN from STOP scheduled after settling deadband (%lu ms remaining)\n",
                  (unsigned long)(_doorMotionStoppedAtMs + deadbandMs - now));
    return true;
  }

  _reversal.cancel();
  const uint32_t openPulseWidth = bypassPreStop ? kReversePulseMs : gRelayPulseMs;
  const bool ok = pulseSlot("open", openPulseWidth);
  if (ok) {
    gLastDoorAutoTxMs = now;
    applyDoorCommandState("open", now);
    _suppressionReleaseAtMs = 0;
    _isManualOpenState = manualCommand && manualHold;
    _safetySensor.clearPassage();

    // Start auto-close countdown or pedestrian partial open countdown immediately from OPEN command
    if (!_isManualOpenState && !gSafeMode && gTagAutoEnabled && _tagOpenDurationMs > 0) {
      _tagOpenStopAtMs = now + _tagOpenDurationMs;
      uint32_t delayMs = (gPrevActiveDelayMaxMs > 0) ? gPrevActiveDelayMaxMs : kDefaultTagCloseDelayMs;
      _autoClose.armPendingClose(now + _tagOpenDurationMs + delayMs);
      Serial.printf("[door] open started -> pedestrian partial open armed for %lu ms, auto-close in %lu ms\n",
                    (unsigned long)_tagOpenDurationMs, (unsigned long)(_tagOpenDurationMs + delayMs));
    } else if (!_isManualOpenState && !gSafeMode && gTagAutoEnabled) {
      _tagOpenStopAtMs = 0;
      uint32_t delayMs = (gPrevActiveDelayMaxMs > 0) ? gPrevActiveDelayMaxMs : kDefaultTagCloseDelayMs;
      _autoClose.armPendingClose(now + delayMs);
      Serial.printf("[door] open started -> auto-close countdown armed for %lu ms\n", (unsigned long)delayMs);
    } else {
      _tagOpenStopAtMs = 0;
      _autoClose.disarmPendingClose();
    }

    if (manualCommand) {
      _tagRegistry.clearSuppression();
    }
    Serial.printf("[door] OPEN pulse executed -> GPIO %u = HIGH (%lu ms, manual=%d hold=%d manualOpenState=%d, bypassPreStop=%d)\n",
                  gMotorReverse ? kBoardClosePin : kBoardOpenPin, (unsigned long)openPulseWidth,
                  manualCommand, manualHold, _isManualOpenState, bypassPreStop);
  }
  return ok;
}

bool DoorController::executeStop(bool manualCommand, uint32_t now) {
  _closeRetryCount = 0;
  _closeRetryAtMs = 0;
  _doubleCloseAtMs = 0;
  _reversal.cancel();

  const bool ok = pulseSlot("stop");
  if (ok) {
    _lastStopPulseMs = now;
    gLastDoorAutoTxMs = now;
    applyDoorCommandState("stop", now);
    if (manualCommand) {
      _isManualOpenState = true;  // Manual stop holds door stopped indefinitely (inhibits auto-close)
      _tagRegistry.snapshotBaseline(now);
      _tagRegistry.setSuppressed(true);
      logDoorEvent(DoorEventCode::ManualStop, DoorEventLevel::Info);
    } else {
      _isManualOpenState = false;
    }
    _autoClose.disarmAll();
    _tagOpenStopAtMs = 0;
    Serial.printf("[door] STOP pulse executed -> GPIO %u+%u = HIGH (1200 ms, manual=%d, manualHold=%d)\n",
                  kBoardOpenPin, kBoardClosePin, manualCommand, _isManualOpenState);
  }
  return ok;
}

bool DoorController::executeClose(bool manualCommand, bool manualHold, bool bypassPreStop, uint32_t now) {
  // Safety check: Cannot close while beam is blocked
  if (gSafetySensorEnabled && _safetySensor.isBlocked()) {
    Serial.println("[door] CLOSE rejected: safety beam is currently blocked");
    logDoorEvent(DoorEventCode::CloseBlock, DoorEventLevel::Warn, 0);
    return false;
  }

  const DoorState prevSt = _doorState;

  // If already closed, nothing to do
  if (_doorState == DoorState::Closed) {
    return true;
  }
  // If already closing, ignore duplicate auto close requests (allow manual re-pulse)
  if (_doorState == DoorState::Closing && !manualCommand) {
    return true;
  }
  // If a close deadband sequence is already running, avoid restarting it
  if (_reversal.isReversingToClose()) {
    return true;
  }

  const uint32_t deadbandMs = kStopPulseMs + kReverseDeadbandMs; // 2400 ms total settling time
  const bool isStoppedOverDeadband = (_doorState == DoorState::Stopped &&
                                      _doorMotionStoppedAtMs != 0 &&
                                      (int32_t)(now - _doorMotionStoppedAtMs) >= (int32_t)deadbandMs);

  // Step 1: Pre-Stop & 2.4s Deadband before closing
  // Bypassed if bypassPreStop is true OR if the door has already been in Stopped state for >= 2.4s
  if (!bypassPreStop && !isStoppedOverDeadband) {
    // Send single STOP pulse (GPIO 6+7 = HIGH 1200ms) to reset motor latch,
    // then apply 1.2s settling deadband (2.4s total) before firing physical CLOSE pulse
    pulseSlot("stop");
    _lastStopPulseMs = now;
    _doorMotionStoppedAtMs = now;
    _doorState = DoorState::Stopped;
    _doorTransitionEndMs = 0;

    _reversal.schedule("close", now + deadbandMs, manualCommand, manualHold);
    _autoClose.disarmAll();
    _tagOpenStopAtMs = 0;
    _doubleCloseAtMs = 0;
    triggerImmediateAnnounce();

    if (prevSt == DoorState::Opening) {
      Serial.printf("[door] CLOSE interrupted OPENING -> STOP (GPIO 6+7 = HIGH %lu ms) with %lu ms deadband before CLOSE\n",
                    (unsigned long)kStopPulseMs, (unsigned long)kReverseDeadbandMs);
      logDoorEvent(DoorEventCode::Reversal, DoorEventLevel::Info, 2);
    } else {
      Serial.printf("[door] CLOSE initiated from %s -> STOP (GPIO 6+7 = HIGH %lu ms) with %lu ms deadband before CLOSE\n",
                    stateToString(prevSt), (unsigned long)kStopPulseMs, (unsigned long)kReverseDeadbandMs);
      logDoorEvent(DoorEventCode::CloseStep1, DoorEventLevel::Info, manualCommand ? 1 : 0);
    }
    return true;
  }

  if (isStoppedOverDeadband && !bypassPreStop) {
    Serial.printf("[door] CLOSE directly executed (already in STOPPED state for %lu ms >= %lu ms, bypassing Pre-Stop)\n",
                  (unsigned long)(now - _doorMotionStoppedAtMs), (unsigned long)deadbandMs);
  }

  // Step 2: Fired after deadband expires OR directly when already stopped >= 2.4s -> Send physical CLOSE pulse
  _reversal.cancel();
  const uint32_t closePulseWidth = (bypassPreStop || prevSt == DoorState::Stopped || prevSt == DoorState::Opening) ? kReversePulseMs : gRelayPulseMs;
  const bool ok = pulseSlot("close", closePulseWidth);
  if (ok) {
    gLastDoorAutoTxMs = now;
    applyDoorCommandState("close", now);
    noteClosingStarted(now);
    if (gDoubleClosePulseEnabled) {
      _doubleCloseAtMs = now + 3500;
    }
    Serial.printf("[door] CLOSE pulse executed -> GPIO %u = HIGH (%lu ms, manual=%d, bypassPreStop=%d, directFromStopped=%d)\n",
                  gMotorReverse ? kBoardOpenPin : kBoardClosePin, (unsigned long)closePulseWidth, manualCommand, bypassPreStop, isStoppedOverDeadband);
    if (manualCommand) {
      logDoorEvent(DoorEventCode::ManualClose, DoorEventLevel::Info);
    } else {
      logDoorEvent(DoorEventCode::AutoClose, DoorEventLevel::Success);
    }
  }
  return ok;
}

bool DoorController::executeSlot(const char* slot, bool manualCommand, bool manualHold, bool bypassPreStop) {
  if (!slot) return false;
  TagStoreLock lock;
  const uint32_t now = millis();

  // Rate limiting for auto commands (avoid relay chatter)
  if ((uint32_t)(now - gLastDoorAutoTxMs) < 250 && !manualCommand) {
    return false;
  }

  const bool isOpen = (strcmp(slot, "open") == 0);
  if (manualCommand || isOpen) {
    _autoClose.resetCircuitBreaker();
  }

  if (isOpen)                  return executeOpen(manualCommand, manualHold, bypassPreStop, now);
  if (strcmp(slot, "stop") == 0)  return executeStop(manualCommand, now);
  if (strcmp(slot, "close") == 0) return executeClose(manualCommand, manualHold, bypassPreStop, now);

  return false;
}

void DoorController::processLayer1Sensors(uint32_t now) {
  // 1.1 Process Thread-Safe Door Commands from FreeRTOS Queue
  if (_cmdQueue != nullptr) {
    DoorCommand cmd;
    while (xQueueReceive(_cmdQueue, &cmd, 0) == pdTRUE) {
      const char* slot = (cmd.action == DoorCmdAction::Open)  ? "open" :
                         (cmd.action == DoorCmdAction::Close) ? "close" : "stop";
      if (cmd.tagId[0] != '\0') {
        TagStoreLock lock;
        Tag* t = _tagRegistry.findById(cmd.tagId);
        if (t) {
          gActiveTriggerTagId = t->id;
          gPrevActiveDelayMaxMs = (t->tagCloseDelayMs > 0) ? t->tagCloseDelayMs : kDefaultTagCloseDelayMs;
          gPrevActiveSafetyDelayMs = (t->tagSafetyCloseDelayMs > 0) ? t->tagSafetyCloseDelayMs : gSafetyCloseDelayMs;
          _tagOpenDurationMs = t->tagOpenDurationMs;
        }
      } else if (cmd.isManual) {
        gActiveTriggerTagId = "";
      }
      executeSlot(slot, cmd.isManual, cmd.manualHold);
    }
  }

  // 1.2 & 1.3 Magnetic & Limit Sensor Processing (Ground Truth Position Sensing)
  MagSensorEvent magEv = _magSensor.update(now, _doorState);
  if (magEv == MagSensorEvent::ReachedClosed) {
    uint32_t closeDurMs = (_lastCloseOrClosingMs > 0) ? (uint32_t)(now - _lastCloseOrClosingMs) : 0;
    noteDoorClosedSync(now);
    logDiagEvent(DoorEventCode::MagClosed,
                 DoorEventLevel::Info,
                 DiagReasonCode::NormalCycle,
                 (uint16_t)min((uint32_t)65535, closeDurMs),
                 (int16_t)(ESP.getFreeHeap() / 1024),
                 "MAG");
    AD_LOG(Serial.println("[door] mag sensor -> sync CLOSED (auto mode restored)"));
  } else if (magEv == MagSensorEvent::ExternallyOpened) {
    uint32_t openDurMs = (_doorOpenedAtMs > 0) ? (uint32_t)(now - _doorOpenedAtMs) : 0;
    _doorState = DoorState::Open;
    _doorOpenedAtMs = now;
    _doorMotionStoppedAtMs = now;
    _doorTransitionEndMs = 0;
    _isManualOpenState = true;  // Hold open permanently for external / house remote
    _autoClose.disarmAll();
    _tagRegistry.snapshotBaseline(now);
    triggerImmediateAnnounce();
    logDiagEvent(DoorEventCode::MagOpen,
                 DoorEventLevel::Info,
                 DiagReasonCode::NormalCycle,
                 (uint16_t)min((uint32_t)65535, openDurMs),
                 (int16_t)(ESP.getFreeHeap() / 1024),
                 "REMOTE");
    AD_LOG(Serial.println("[door] mag sensor -> opened externally (house remote) -> HOLD OPEN"));
  } else if (magEv == MagSensorEvent::ReachedOpen) {
    uint32_t openDurMs = (_doorOpenedAtMs > 0) ? (uint32_t)(now - _doorOpenedAtMs) : 0;
    const bool openedExternally = (_doorState == DoorState::Closed || _doorState == DoorState::Stopped);
    if (openedExternally) {
      _isManualOpenState = true;  // Opened externally -> Hold open permanently
      _autoClose.disarmAll();
      _tagRegistry.snapshotBaseline(now);
    }
    _doorState = DoorState::Open;
    _doorTransitionEndMs = 0;
    _doorOpenedAtMs = now;
    _doorMotionStoppedAtMs = now;
    triggerImmediateAnnounce();
    logDiagEvent(DoorEventCode::LimitOpen,
                 DoorEventLevel::Info,
                 DiagReasonCode::NormalCycle,
                 (uint16_t)min((uint32_t)65535, openDurMs),
                 (int16_t)(ESP.getFreeHeap() / 1024),
                 openedExternally ? "REMOTE" : "LIMIT");
    Serial.println("[door] builtin open limit -> sync OPEN (manual hold)");
  }

  // 1.4 Safety Sensor Logic (Beam Obstacle & Anti-Pinch)
  const bool isClosingForDebounce = (_doorState == DoorState::Closing || _reversal.isReversingToClose());
  SafetyBeamEvent safetyEv = _safetySensor.update(now, isClosingForDebounce);

  if (gSafetySensorEnabled) {
    const bool isClosing = (_doorState == DoorState::Closing || _reversal.isReversingToClose());

    if (_safetySensor.isBlocked()) {
      // ── Beam is Blocked ──
      if (safetyEv == SafetyBeamEvent::BeamJustBroken) {
        AD_LOG(Serial.println("[safety] beam broken (obstacle detected)"));
        logDiagEvent(DoorEventCode::BeamCut,
                     DoorEventLevel::Warn,
                     DiagReasonCode::ObstacleDuringClose,
                     0,
                     0,
                     "BEAM");
        triggerImmediateAnnounce();

        // When door is Opening and vehicle cuts safety beam -> STOP IMMEDIATELY!
        if (_doorState == DoorState::Opening) {
          gLastDoorAutoTxMs = 0; // Bypass 250ms rate limit for emergency safety stop
          executeSlot("stop", false, false);
          AD_LOG(Serial.println("[safety] vehicle crossed beam while opening -> STOP immediately"));
        }
      }

      // Disarm all auto-close countdowns while obstacle is in the doorway
      _autoClose.disarmAll();
      _tagOpenStopAtMs = 0;

      // If door is Closing when obstacle appears -> EMERGENCY STOP & REVERSE TO OPEN!
      if (isClosing) {
        AD_LOG(Serial.println("[safety] obstacle during close -> STOP and reverse to OPEN"));
        logDiagEvent(DoorEventCode::SafeAbort,
                     DoorEventLevel::Danger,
                     DiagReasonCode::ObstacleDuringClose,
                     2400,
                     0,
                     "BEAM");
        _lastSafeAbortMs = now;
        _isManualOpenState = false;
        for (auto* l : _listeners) {
          if (l) l->onSafeAbort();
        }
        executeSlot("open", false, false);
      }

      // Mark that a passage has crossed the safety beam only when door is not closed
      if (_doorState != DoorState::Closed) {
        _safetySensor.setPassageCrossed(true);
      }
    } else {
      // ── Beam is Clear ──
      if (_safetySensor.hasPassageCrossed()) {
        if (gSafeMode) {
          logDiagEvent(DoorEventCode::PassSkip, DoorEventLevel::Warn, DiagReasonCode::PassSkipSafeMode, 1, 0, "SAFETY");
          _safetySensor.clearPassage();
        } else if (!gTagAutoEnabled) {
          logDiagEvent(DoorEventCode::PassSkip, DoorEventLevel::Warn, DiagReasonCode::PassSkipClosed, 2, 0, "SAFETY");
          _safetySensor.clearPassage();
        } else if (_isManualOpenState) {
          // Manual Hold Mode (Hold Open / Stop): strictly hold state, never auto-close on beam passage
          // Door must stay held indefinitely without disturbance until user commands Close and door finishes closing
          logDiagEvent(DoorEventCode::PassSkip, DoorEventLevel::Warn, DiagReasonCode::PassSkipManual, 3, 0, "SAFETY");
          _safetySensor.clearPassage();
          AD_LOG(Serial.println("[safety] manual hold mode active -> beam passage ignored (door remains held)"));
        } else if (_doorState == DoorState::Closed) {
          logDiagEvent(DoorEventCode::PassSkip, DoorEventLevel::Warn, DiagReasonCode::PassSkipClosed, 4, 0, "SAFETY");
          _safetySensor.clearPassage();
        } else if (_doorState == DoorState::Open || _doorState == DoorState::Opening || _doorState == DoorState::Stopped) {
          if (_autoClose.safetyCloseAtMs() == 0) {
            uint32_t effectiveSafetyDelay = getEffectiveSafetyCloseDelay(now);
            if (_lastSafeAbortMs != 0 && (uint32_t)(now - _lastSafeAbortMs) < 20000) {
              effectiveSafetyDelay = gSafetyReverseDelayMs;
            }
            uint32_t baseTimeMs = now;
            if (_reversal.isReversingToOpen() && (int32_t)(_reversal.targetAtMs() - now) > 0) {
              baseTimeMs = _reversal.targetAtMs();
            }
            uint32_t targetCloseAtMs = baseTimeMs + effectiveSafetyDelay;
            const uint32_t stoppedRefMs = (_doorMotionStoppedAtMs != 0) ? _doorMotionStoppedAtMs :
                                          (_lastStopPulseMs != 0) ? _lastStopPulseMs : 0;
            if (stoppedRefMs != 0) {
              uint32_t deadbandEndMs = stoppedRefMs + kStopPulseMs + kReverseDeadbandMs;
              if ((int32_t)(deadbandEndMs - targetCloseAtMs) > 0) {
                targetCloseAtMs = deadbandEndMs;
              }
            }
            _autoClose.armSafetyClose(targetCloseAtMs);
            Serial.printf("[safety] passage clear -> auto-close scheduled in %lu ms (safety delay = %lu ms)\n",
                          (unsigned long)(_autoClose.safetyCloseAtMs() - now), (unsigned long)effectiveSafetyDelay);
            logDiagEvent(DoorEventCode::PassArm,
                         DoorEventLevel::Info,
                         DiagReasonCode::PassageCrossed,
                         (uint16_t)effectiveSafetyDelay,
                         0,
                         "SAFETY");
          }
        }
      }
    }
  } else {
    _autoClose.disarmSafetyClose();
    _safetySensor.clearPassage();
  }
}

void DoorController::processLayer2Tags(uint32_t now, bool& anyActiveTotal, bool& anyActiveNonSuppressed) {
  anyActiveTotal = false;
  anyActiveNonSuppressed = false;
  bool hasActiveTagSeen = false;
  uint32_t activeDelayMax = kDefaultTagCloseDelayMs;
  uint32_t activeSafetyDelay = gSafetyCloseDelayMs;
  uint32_t activeOpenDuration = 0;
  uint32_t latestSeenMs = 0;

  for (const auto& t : _tagRegistry.tags()) {
    if (TagRegistry::isRf(t)) continue; // RF remotes are manual button commands, NEVER auto-presence proximity tags!
    if (isTagActive(t, now)) {
      if (!TagRegistry::isTagInSchedule(t)) {
        static uint32_t lastSchedWarnMs = 0;
        if ((uint32_t)(now - lastSchedWarnMs) >= 5000) {
          lastSchedWarnMs = now;
          AD_LOG(Serial.printf("[door] Tag '%s' in range but rejected (outside active hours)\n",
                               t.name.isEmpty() ? t.id.c_str() : t.name.c_str()));
        }
        continue;
      }
      anyActiveTotal = true;
      if (!_tagRegistry.wasInBaseline(t.id)) {
        anyActiveNonSuppressed = true;
        if (!hasActiveTagSeen || t.lastSeenMs >= latestSeenMs) {
          latestSeenMs = t.lastSeenMs;
          hasActiveTagSeen = true;
          activeDelayMax = t.tagCloseDelayMs;
          activeSafetyDelay = t.tagSafetyCloseDelayMs;
          activeOpenDuration = t.tagOpenDurationMs;
          gActiveTriggerTagId = t.id;
        }
      }
    }
  }

  if (hasActiveTagSeen) {
    gPrevActiveDelayMaxMs = activeDelayMax;
    gPrevActiveSafetyDelayMs = activeSafetyDelay;
    _tagOpenDurationMs = activeOpenDuration;
  }

  // Case A: Door is currently opening under partial open (e.g. initiated by RF pedestrian open)
  // and an active unsuppressed tag wanting full open (activeOpenDuration == 0) arrives:
  // Cancel the partial open stop timer so the door continues opening all the way to full open!
  if (_doorState == DoorState::Opening && _tagOpenStopAtMs != 0 && hasActiveTagSeen && activeOpenDuration == 0 && anyActiveNonSuppressed) {
    _tagOpenStopAtMs = 0;
    _tagOpenDurationMs = 0;
    uint32_t delayMs = (activeDelayMax > 0) ? activeDelayMax : kDefaultTagCloseDelayMs;
    _autoClose.armPendingClose(now + delayMs);
    Serial.printf("[door] full-open tag '%s' detected while opening -> cancelled partial stop, opening to full limit\n",
                  gActiveTriggerTagId.c_str());
    logDiagEvent(DoorEventCode::TagOpen,
                 DoorEventLevel::Info,
                 DiagReasonCode::SrcTagRssi,
                 0,
                 0,
                 "FULL_OPEN_OVERRIDE");
    triggerImmediateAnnounce();
  }

  // Handle Automatic Opening when an unsuppressed tag approaches
  if (!gSafeMode && gTagAutoEnabled) {
    if (anyActiveNonSuppressed) {
      _suppressionReleaseAtMs = 0;

      // Do NOT auto-open if safety sensor has already armed a passage-close timer, or reversal is pending
      const bool isReversingToClose = _reversal.isReversingToClose();
      const bool isReversingToOpen = _reversal.isReversingToOpen();
      const bool isSafetyClosePending = (gSafetySensorEnabled && _safetySensor.hasPassageCrossed() &&
                                         (_autoClose.safetyCloseAtMs() != 0 || _safetySensor.isBlocked()));
      const bool safetyCloseArmed = isSafetyClosePending || isReversingToClose || isReversingToOpen;
      if (!safetyCloseArmed) {
        bool shouldOpen = false;
        if (_doorState == DoorState::Closed) {
          if (!_tagRegistry.isSuppressed()) {
            shouldOpen = true;
          } else if (_tagRegistry.anyNewlyActiveTag(now)) {
            shouldOpen = true;
            _tagRegistry.clearSuppression();
            _isManualOpenState = false;
          }
        } else if (_doorState == DoorState::Stopped && !_isManualOpenState) {
          // Case B: Door stopped mid-way (pedestrian partial open) and an active unsuppressed tag wanting full open arrives:
          // Resume opening to full open!
          if (activeOpenDuration == 0) {
            if (!_tagRegistry.isSuppressed()) {
              shouldOpen = true;
            } else if (_tagRegistry.anyNewlyActiveTag(now)) {
              shouldOpen = true;
              _tagRegistry.clearSuppression();
            }
          }
        }
        if (shouldOpen) {
          if (executeSlot("open", false, false)) {
            Tag* trigTag = _tagRegistry.findById(gActiveTriggerTagId);
            const char* tagDesc = (trigTag && !trigTag->name.isEmpty()) ? trigTag->name.c_str() :
                                  (gActiveTriggerTagId.isEmpty() ? "active" : gActiveTriggerTagId.c_str());
            AD_LOG(Serial.println("[door] auto-opening (tag active)"));
            logDiagEvent(DoorEventCode::TagOpen,
                         DoorEventLevel::Success,
                         DiagReasonCode::SrcTagRssi,
                         0,
                         trigTag ? (int16_t)trigTag->lastRssi : 0,
                         tagDesc);
          }
        }
      }
    } else {
      // Manage Tag baseline suppression release when all tags have left
      if (_doorState == DoorState::Closing) {
        _tagRegistry.setSuppressed(true);
        _suppressionReleaseAtMs = 0;
      } else if (_tagRegistry.isSuppressed() && !anyActiveTotal) {
        if (_suppressionReleaseAtMs == 0) {
          _suppressionReleaseAtMs = now + kSuppressionReleaseIdleMs;
        } else if ((int32_t)(now - _suppressionReleaseAtMs) >= 0) {
          _tagRegistry.clearSuppression();
          _suppressionReleaseAtMs = 0;
          AD_LOG(Serial.println("[door] reset tag suppression (all tags inactive)"));
        }
      } else {
        _suppressionReleaseAtMs = 0;
      }
    }
  }
}

void DoorController::processLayer4Reversal(uint32_t now) {
  if (_reversal.isDue(now)) {
    const char* targetSlot = "";
    bool manual = false;
    bool hold = false;
    _reversal.consume(now, targetSlot, manual, hold);
    Serial.printf("[door] reversal deadband (%lu ms) expired -> executing %s\n",
                  (unsigned long)(kStopPulseMs + kReverseDeadbandMs), targetSlot);
    executeSlot(targetSlot, manual, hold, true /* bypassPreStop */);
  }
}

void DoorController::processLayer5Timers(uint32_t now) {
  // 5.0 Double Close Pulse #2 Execution (Confirmation pulse after 3 seconds)
  if (_doubleCloseAtMs != 0 && (int32_t)(now - _doubleCloseAtMs) >= 0) {
    _doubleCloseAtMs = 0;
    const bool isClosedPhysically = (gMagSensorEnabled && gMagSensorStableClosed);
    const bool isSafetyBlocked = gSafetySensorEnabled && _safetySensor.isBlocked();
    if (!isClosedPhysically && !isSafetyBlocked && !gSafeMode && (_doorState == DoorState::Closing || _doorState == DoorState::Open || _doorState == DoorState::Stopped)) {
      pulseSlot("close");
      logDiagEvent(DoorEventCode::DoubleClose,
                   DoorEventLevel::Info,
                   DiagReasonCode::NormalCycle,
                   (uint16_t)gRelayPulseMs,
                   0,
                   "PULSE2");
      Serial.println("[door] double close pulse #2 executed (3s delay)");
    }
  }

  // 5.1 Auto-Retry Close Scheduler Execution
  if (_closeRetryAtMs != 0 && (int32_t)(now - _closeRetryAtMs) >= 0) {
    _closeRetryAtMs = 0;
    const bool isClosedPhysically = (gMagSensorEnabled && gMagSensorStableClosed);
    if (!isClosedPhysically && !gSafeMode && (_doorState == DoorState::Stopped || _doorState == DoorState::Open)) {
      Serial.printf("[door] auto-retry CLOSE: executing attempt #%u (delay expired)\n", _closeRetryCount);
      executeSlot("close", false, false);
    } else {
      _closeRetryCount = 0;
    }
  }

  // 5.15 Pedestrian Partial Open Timer Expiry (Stop door mid-way, then arm pause delay)
  if ((_doorState == DoorState::Opening || _doorState == DoorState::Open) && _tagOpenStopAtMs != 0 && (int32_t)(now - _tagOpenStopAtMs) >= 0) {
    pulseSlot("stop");
    _lastStopPulseMs = now;
    _doorMotionStoppedAtMs = now;
    _doorState = DoorState::Stopped;
    _doorTransitionEndMs = 0;
    _tagOpenStopAtMs = 0;

    uint32_t pauseDelayMs = (gPrevActiveDelayMaxMs > 0) ? gPrevActiveDelayMaxMs : kDefaultTagCloseDelayMs;
    _autoClose.armPendingClose(now + pauseDelayMs);

    Serial.printf("[door] pedestrian open duration (%lu ms) reached -> STOP pulse sent, auto-close scheduled in %lu ms\n",
                  (unsigned long)_tagOpenDurationMs, (unsigned long)pauseDelayMs);
    logDiagEvent(DoorEventCode::PedestrianStop,
                 DoorEventLevel::Info,
                 DiagReasonCode::NormalCycle,
                 (uint16_t)(_tagOpenDurationMs / 1000),
                 (int16_t)(pauseDelayMs / 1000),
                 "PEDESTRIAN");
    triggerImmediateAnnounce();
  }

  // 5.2 Transition Timer Expiry
  if ((_doorState == DoorState::Opening || _doorState == DoorState::Closing) && _doorTransitionEndMs != 0) {
    const uint32_t graceMs = (_doorState == DoorState::Closing && gMagSensorEnabled) ? 3000 : 0;
    if ((int32_t)(now - (_doorTransitionEndMs + graceMs)) >= 0) {
      if (_doorState == DoorState::Opening) {
        if (_doorOpenedAtMs == 0) _doorOpenedAtMs = now;
        _doorState = DoorState::Open;
        _doorMotionStoppedAtMs = now;
        _doorTransitionEndMs = 0;
        if (!_isManualOpenState && !gSafeMode && gTagAutoEnabled && _tagOpenStopAtMs == 0) {
          uint32_t delayMs = (gPrevActiveDelayMaxMs > 0) ? gPrevActiveDelayMaxMs : kDefaultTagCloseDelayMs;
          _autoClose.armPendingClose(now + delayMs);
          Serial.printf("[door] opening transition completed -> OPEN, auto-close armed in %lu ms\n", (unsigned long)delayMs);
        }
        triggerImmediateAnnounce();
        AD_LOG(Serial.println("[door] opening transition completed -> OPEN"));
      } else if (_doorState == DoorState::Closing) {
        const bool hasPhysicalClosedSensor = gMagSensorEnabled;
        const bool isClosedPhysically = (gMagSensorEnabled && gMagSensorStableClosed);

        if (hasPhysicalClosedSensor && !isClosedPhysically && gAutoRetryCloseEnabled) {
          if (_closeRetryCount < gMaxCloseRetryCount) {
            _closeRetryCount++;
            _doorState = DoorState::Stopped;
            _doorTransitionEndMs = 0;
            _closeRetryAtMs = now + gCloseRetryDelayMs;
            Serial.printf("[door] physical sensor NOT closed after travel timer -> auto-retry #%u in %lu ms\n",
                          _closeRetryCount, (unsigned long)gCloseRetryDelayMs);
            logDiagEvent(DoorEventCode::RetryClose,
                         DoorEventLevel::Warn,
                         DiagReasonCode::RetryCloseUnlocked,
                         (uint16_t)_closeRetryCount,
                         0,
                         "RETRY");
            triggerImmediateAnnounce();
          } else {
            // Exceeded max retry attempts -> Trip circuit breaker!
            _doorState = DoorState::Stopped;
            _doorTransitionEndMs = 0;
            _closeRetryAtMs = 0;
            _autoClose.tripCircuitBreaker();
            Serial.printf("[door] ALERT: Auto-retry close failed after %u attempts! Tripping circuit breaker to prevent infinite motor cycling.\n",
                          _closeRetryCount);
            logDiagEvent(DoorEventCode::RetryFail,
                         DoorEventLevel::Danger,
                         DiagReasonCode::RetryExhausted,
                         (uint16_t)_closeRetryCount,
                         0,
                         "CIRCUIT_BREAKER");
            triggerImmediateAnnounce();
          }
        } else {
          _closeRetryCount = 0;
          _closeRetryAtMs = 0;
          noteDoorClosedSync(now);
          AD_LOG(Serial.println("[door] closing transition completed -> CLOSED"));
        }
      }
    }
  }

  // Maintain opened timestamp while open
  if (_doorState == DoorState::Open && _doorOpenedAtMs == 0) {
    _doorOpenedAtMs = now;
  }
}

void DoorController::processIdleSanitizer(uint32_t now, bool anyActiveTotal) {
  if (_doorState == DoorState::Closed && (uint32_t)(now - _lastCloseOrClosingMs) >= 15000) {
    if (!anyActiveTotal) {
      if (_safetySensor.hasPassageCrossed() || _autoClose.isArmed() ||
          _reversal.isReversing() || _tagRegistry.isSuppressed() || !_tagRegistry.baselineActiveIds().empty() ||
          !gActiveTriggerTagId.isEmpty() || _isManualOpenState || _closeRetryCount != 0 || _closeRetryAtMs != 0) {
        _safetySensor.clearPassage();
        _autoClose.disarmAll();
        _reversal.cancel();
        _closeRetryCount = 0;
        _closeRetryAtMs = 0;
        _isManualOpenState = false;
        _tagRegistry.clearSuppression();
        gActiveTriggerTagId = "";
        _suppressionReleaseAtMs = 0;
        AD_LOG(Serial.println("[door] idle sanitizer: state fully refreshed (door closed >15s)"));
      }
    }
  }
}

void DoorController::step() {
  TagStoreLock lock;
  const uint32_t now = millis();

  // Layer 1: Sensors & Input Processing
  processLayer1Sensors(now);

  // Layer 2: Tag & RF Trigger Evaluation
  bool anyActiveTotal = false;
  bool anyActiveNonSuppressed = false;
  processLayer2Tags(now, anyActiveTotal, anyActiveNonSuppressed);

  // Layer 3: Auto-Close Decision Engine & Timers
  AutoCloseContext closeCtx{
    now,
    _doorState,
    gSafeMode,
    gTagAutoEnabled,
    _isManualOpenState,
    gSafetySensorEnabled,
    _safetySensor.isBlocked(),
    _safetySensor.hasPassageCrossed(),
    anyActiveNonSuppressed,
    _doorOpenedAtMs,
    (gPrevActiveDelayMaxMs > 0) ? gPrevActiveDelayMaxMs : kDefaultTagCloseDelayMs,
    _reversal.isReversing()
  };
  _autoClose.update(closeCtx, this);

  // Layer 4: Directional Reversal Deadband Actuator
  processLayer4Reversal(now);

  // Layer 5: Transition Timer & State Completion & Auto-Retry Close
  processLayer5Timers(now);

  // Idle Sanitizer: Clean runtime state when door has been CLOSED for >= 15 seconds
  processIdleSanitizer(now, anyActiveTotal);

  gPrevAnyTagActive = (gSafeMode || !gTagAutoEnabled) ? false : anyActiveTotal;
}

bool DoorController::isSafeToRestart() const {
  TagStoreLock lock;
  const uint32_t now = millis();
  if (_doorState != DoorState::Closed) return false;
  if (_motor.isBusy()) return false;
  if (_reversal.isReversing()) return false;
  if (_autoClose.isArmed()) return false;
  if (gSafetySensorEnabled && _safetySensor.isBlocked()) return false;
  if (_tagRegistry.hasAnyActiveTag(now)) return false;
  return true;
}

bool DoorController::isReversing() const {
  return _reversal.isReversing();
}

const char* DoorController::stateToString(DoorState s) {
  switch (s) {
    case DoorState::Open:    return "open";
    case DoorState::Closed:  return "closed";
    case DoorState::Stopped: return "stopped";
    case DoorState::Opening: return "opening";
    case DoorState::Closing: return "closing";
    default:                 return "closed";
  }
}

void DoorController::addListener(IDoorListener* listener) {
  if (listener) {
    _listeners.push_back(listener);
  }
}

void DoorController::removeListener(IDoorListener* listener) {
  if (listener) {
    _listeners.erase(
      std::remove(_listeners.begin(), _listeners.end(), listener),
      _listeners.end()
    );
  }
}
