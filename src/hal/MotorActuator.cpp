#include "MotorActuator.h"
#include "app/config.h"
#include "app/globals.h"
#include <soc/soc.h>
#include <soc/gpio_reg.h>
#include <esp_timer.h>

// Global instance for Composition Root / backward compatibility
MotorActuator gMotor;

MotorActuator::MotorActuator(uint8_t openPin, uint8_t closePin)
  : _openPin(openPin), _closePin(closePin),
    _openMask(1UL << openPin), _closeMask(1UL << closePin),
    _bothMask((1UL << openPin) | (1UL << closePin)) {
  _mux = portMUX_INITIALIZER_UNLOCKED;
}

MotorActuator::~MotorActuator() {
  forceShutdown();
  if (_hwTimer != nullptr) {
    esp_timer_delete(_hwTimer);
    _hwTimer = nullptr;
  }
}

// Simultaneous atomic register drive on GPIO pins to eliminate pin skew.
// Active HIGH on open/close pin; both LOW = idle / stop pulse release
void MotorActuator::driveBoardSignals(bool openActive, bool closeActive) {
  if (gSimMode) {
    REG_WRITE(GPIO_OUT_W1TC_REG, _bothMask);
  } else {
    const uint32_t openMask = gMotorReverse ? _closeMask : _openMask;
    const uint32_t closeMask = gMotorReverse ? _openMask : _closeMask;
    uint32_t setMask = 0;
    uint32_t clrMask = 0;
    if (openActive) setMask |= openMask; else clrMask |= openMask;
    if (closeActive) setMask |= closeMask; else clrMask |= closeMask;
    // Clear inactive pins first, then set active pins in single atomic register write
    if (clrMask) REG_WRITE(GPIO_OUT_W1TC_REG, clrMask);
    if (setMask) REG_WRITE(GPIO_OUT_W1TS_REG, setMask);
  }
}

void MotorActuator::drivePhaseSignals(MotorPhase phase) {
  if (phase == MotorPhase::PulsingOpen) {
    driveBoardSignals(true, false);
  } else if (phase == MotorPhase::PulsingClose) {
    driveBoardSignals(false, true);
  } else if (phase == MotorPhase::PulsingStop) {
    driveBoardSignals(true, true);
  } else {
    driveBoardSignals(false, false);
  }
}

// Hardware timer callback: guaranteed de-energize even if loopTask is frozen, blocked, or crashed
void IRAM_ATTR MotorActuator::onHardwareTimer(void* arg) {
  MotorActuator* self = static_cast<MotorActuator*>(arg);
  if (!self) return;

  REG_WRITE(GPIO_OUT_W1TC_REG, self->_bothMask);

  portENTER_CRITICAL_ISR(&self->_mux);
  self->_phase = MotorPhase::Idle;
  self->_stage = MotorStage::Idle;
  portEXIT_CRITICAL_ISR(&self->_mux);
}

bool MotorActuator::startPulse(MotorPhase phase, uint32_t customDurationMs) {
  bool wasPreempted = false;
  if (_hwTimer != nullptr) {
    esp_timer_stop(_hwTimer);
  }

  portENTER_CRITICAL(&_mux);
  if (_phase != MotorPhase::Idle || _stage != MotorStage::Idle) {
    driveBoardSignals(false, false);
    _phase = MotorPhase::Idle;
    _stage = MotorStage::Idle;
    wasPreempted = true;
  }
  const uint32_t now = millis();
  _pulseGen = _pulseGen + 1;  // increment generation BEFORE setting phase

  // STOP is always a single sustained pulse per specification Rule #4/#6.
  // Open/Close use double-pulse (Pulse1 -> Gap -> Pulse2).
  const bool isSinglePulse = (phase == MotorPhase::PulsingStop);
  uint32_t pulseDuration;
  if (customDurationMs > 0) {
    pulseDuration = customDurationMs;
  } else if (isSinglePulse) {
    pulseDuration = kStopPulseMs;       // 1200ms single sustained STOP pulse
  } else {
    pulseDuration = kDoublePulseWidthMs; // 300ms per press for Open/Close double-pulse
  }
  // Hard ceiling fail-safe: Never allow any pulse to exceed 1500ms under any circumstances
  if (pulseDuration > 1500) {
    pulseDuration = 1500;
  }
  _pulseWidthMs = pulseDuration;
  _stageEndMs = now + pulseDuration;
  _phase = phase;
  _stage = MotorStage::Pulse1;

  // Drive hardware signals atomically inside critical section to eliminate preemption race
  drivePhaseSignals(phase);
  portEXIT_CRITICAL(&_mux);

  // Arm hardware timer for guaranteed de-assertion
  if (_hwTimer != nullptr) {
    esp_timer_start_once(_hwTimer, (uint64_t)(pulseDuration + 200) * 1000ULL);
  }

  if (wasPreempted) {
    Serial.println("[gpio] previous pulse preempted by new command");
  }

  if (phase == MotorPhase::PulsingOpen) {
    const uint8_t pin = gMotorReverse ? _closePin : _openPin;
    Serial.printf("[gpio] OPEN double-pulse START -> Press 1/2: GPIO %u = HIGH (%lu ms, gap %lu ms, Press 2/2 %lu ms)\n",
                  pin, (unsigned long)pulseDuration, (unsigned long)kDoublePulseGapMs, (unsigned long)pulseDuration);
  } else if (phase == MotorPhase::PulsingClose) {
    const uint8_t pin = gMotorReverse ? _openPin : _closePin;
    Serial.printf("[gpio] CLOSE double-pulse START -> Press 1/2: GPIO %u = HIGH (%lu ms, gap %lu ms, Press 2/2 %lu ms)\n",
                  pin, (unsigned long)pulseDuration, (unsigned long)kDoublePulseGapMs, (unsigned long)pulseDuration);
  } else if (phase == MotorPhase::PulsingStop) {
    Serial.printf("[gpio] STOP single-pulse -> GPIO %u+%u = HIGH (%lu ms)\n",
                  _openPin, _closePin, (unsigned long)pulseDuration);
  }
  return true;
}

void MotorActuator::init() {
  pinMode(_openPin, OUTPUT);
  pinMode(_closePin, OUTPUT);
  digitalWrite(_openPin, LOW);
  digitalWrite(_closePin, LOW);
  driveBoardSignals(false, false);

  if (_hwTimer == nullptr) {
    const esp_timer_create_args_t timerArgs = {
      .callback = &MotorActuator::onHardwareTimer,
      .arg = this,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "relay_auto_off"
    };
    esp_err_t err = esp_timer_create(&timerArgs, &_hwTimer);
    if (err == ESP_OK) {
      Serial.println("[gpio] Hardware timer auto-off initialized (fail-safe protection active)");
    } else {
      Serial.printf("[gpio] ERROR creating hardware timer: %d\n", err);
    }
  }

  Serial.printf("[gpio] Relay initialized (OpenPin=GPIO %u, ClosePin=GPIO %u, doublePulseWidth=%lu ms, gap=%lu ms)\n",
                _openPin, _closePin, (unsigned long)kDoublePulseWidthMs, (unsigned long)kDoublePulseGapMs);
}

void MotorActuator::forceShutdown() {
  if (_hwTimer != nullptr) {
    esp_timer_stop(_hwTimer);
  }
  portENTER_CRITICAL(&_mux);
  driveBoardSignals(false, false);
  _phase = MotorPhase::Idle;
  _stage = MotorStage::Idle;
  _chirpRemaining = 0;
  portEXIT_CRITICAL(&_mux);
  Serial.printf("[gpio] EMERGENCY: Forced relay cutoff -> GPIO %u & GPIO %u = LOW\n", _openPin, _closePin);
}

void MotorActuator::step() {
  if (_chirpRemaining > 0) {
    const uint32_t now = millis();
    if ((int32_t)(now - _chirpNextMs) >= 0) {
      if (!_chirpStateOn) {
        portENTER_CRITICAL(&_mux);
        driveBoardSignals(true, true);
        _phase = MotorPhase::PulsingStop;
        portEXIT_CRITICAL(&_mux);
        _chirpStateOn = true;
        _chirpNextMs = now + 140;
      } else {
        portENTER_CRITICAL(&_mux);
        driveBoardSignals(false, false);
        _phase = MotorPhase::Idle;
        _stage = MotorStage::Idle;
        portEXIT_CRITICAL(&_mux);
        _chirpStateOn = false;
        _chirpRemaining--;
        _chirpNextMs = now + 140;
      }
    }
    return;
  }

  // Fast path: When motor is Idle (99.9% of loop iterations), exit immediately without critical section
  if (_phase == MotorPhase::Idle) {
    return;
  }

  portENTER_CRITICAL(&_mux);
  MotorPhase phase = _phase;
  MotorStage stage = _stage;
  uint32_t stageEnd = _stageEndMs;
  uint32_t pulseWidth = _pulseWidthMs;
  uint8_t gen = _pulseGen;
  portEXIT_CRITICAL(&_mux);

  if (phase == MotorPhase::Idle || stage == MotorStage::Idle) return;

  const uint32_t now = millis();
  if ((int32_t)(now - stageEnd) >= 0) {
    if (stage == MotorStage::Pulse1) {
      if (_hwTimer != nullptr) {
        esp_timer_stop(_hwTimer);
      }

      // STOP: Single pulse only — go directly to Idle (no Gap/Pulse2)
      // Spec Rule #4: "ห้ามส่งสัญญาณ STOP เบิ้ล 2 ครั้ง" / Single STOP Pulse 1200ms
      if (phase == MotorPhase::PulsingStop) {
        bool terminated = false;
        portENTER_CRITICAL(&_mux);
        if (_pulseGen == gen) {
          driveBoardSignals(false, false);
          _phase = MotorPhase::Idle;
          _stage = MotorStage::Idle;
          terminated = true;
        }
        portEXIT_CRITICAL(&_mux);
        if (terminated) {
          Serial.printf("[gpio] STOP single-pulse COMPLETE (%lu ms) -> GPIO %u & GPIO %u = LOW\n",
                        (unsigned long)pulseWidth, _openPin, _closePin);
        }
        return;
      }

      // Open/Close: Transition from Pulse 1 to Gap (0.3s interval / LOW)
      portENTER_CRITICAL(&_mux);
      if (_pulseGen == gen) {
        driveBoardSignals(false, false);
        _stage = MotorStage::Gap;
        _stageEndMs = now + kDoublePulseGapMs;
      }
      portEXIT_CRITICAL(&_mux);
      const char* phaseName = (phase == MotorPhase::PulsingOpen) ? "OPEN" : "CLOSE";
      Serial.printf("[gpio] %s double-pulse -> GAP interval (%lu ms)\n", phaseName, (unsigned long)kDoublePulseGapMs);
    } else if (stage == MotorStage::Gap) {
      // Transition from Gap to Pulse 2 (HIGH) — Open/Close only
      portENTER_CRITICAL(&_mux);
      if (_pulseGen == gen) {
        drivePhaseSignals(phase);
        _stage = MotorStage::Pulse2;
        _stageEndMs = now + pulseWidth;
      }
      portEXIT_CRITICAL(&_mux);
      if (_hwTimer != nullptr) {
        esp_timer_start_once(_hwTimer, (uint64_t)(pulseWidth + 200) * 1000ULL);
      }
      if (phase == MotorPhase::PulsingOpen) {
        const uint8_t pin = gMotorReverse ? _closePin : _openPin;
        Serial.printf("[gpio] OPEN double-pulse -> Press 2/2: GPIO %u = HIGH (%lu ms)\n", pin, (unsigned long)pulseWidth);
      } else if (phase == MotorPhase::PulsingClose) {
        const uint8_t pin = gMotorReverse ? _openPin : _closePin;
        Serial.printf("[gpio] CLOSE double-pulse -> Press 2/2: GPIO %u = HIGH (%lu ms)\n", pin, (unsigned long)pulseWidth);
      }
    } else if (stage == MotorStage::Pulse2) {
      // Transition from Pulse 2 to Idle (Complete) — Open/Close only
      if (_hwTimer != nullptr) {
        esp_timer_stop(_hwTimer);
      }
      bool terminated = false;
      portENTER_CRITICAL(&_mux);
      if (_pulseGen == gen) {
        driveBoardSignals(false, false);
        _phase = MotorPhase::Idle;
        _stage = MotorStage::Idle;
        terminated = true;
      }
      portEXIT_CRITICAL(&_mux);
      if (terminated) {
        const char* phaseName = (phase == MotorPhase::PulsingOpen) ? "OPEN" : "CLOSE";
        Serial.printf("[gpio] %s double-pulse COMPLETE -> GPIO %u & GPIO %u = LOW\n", phaseName, _openPin, _closePin);
      }
    }
  }
}

bool MotorActuator::pulseOpen(uint32_t durationMs) { return startPulse(MotorPhase::PulsingOpen, durationMs); }
bool MotorActuator::pulseClose(uint32_t durationMs) { return startPulse(MotorPhase::PulsingClose, durationMs); }
bool MotorActuator::pulseStop(uint32_t durationMs) { return startPulse(MotorPhase::PulsingStop, durationMs); }

bool MotorActuator::isBusy() const {
  return _phase != MotorPhase::Idle || _stage != MotorStage::Idle || _chirpRemaining > 0;
}

void MotorActuator::triggerChirp(uint8_t count) {
  if (count == 0) return;
  _chirpRemaining = count;
  _chirpNextMs = millis();
  _chirpStateOn = false;
}
