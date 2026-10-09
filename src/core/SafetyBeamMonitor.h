#pragma once

#include "app/config.h"
#include <Arduino.h>

// ============================================================================
// SafetyBeamMonitor: Photocell safety sensor debounce & passage tracking
// ============================================================================
// Encapsulates the anti-pinch optical safety beam sensor logic.
// Features:
// - Dynamic asymmetric debouncing (80ms when closing/reversing to reject motor
//   inrush / vibration noise; 20ms when stationary/opening for fast response).
// - Multi-sampling during boot for stable initial state detection.
// - Passage tracking: records when a vehicle crossed the beam during the current
//   open session to coordinate auto-close priority.

enum class SafetyBeamEvent : uint8_t {
  None = 0,
  BeamJustBroken,        // Beam transitioned from clear -> blocked (obstacle entered)
  BeamBlocked,           // Beam remains continuously blocked
  BeamJustCleared,       // Beam transitioned from blocked -> clear (obstacle left)
  BeamClear              // Beam remains continuously clear
};

class SafetyBeamMonitor {
public:
  explicit SafetyBeamMonitor(uint8_t pin = kSafetySensorPin);

  void init();

  // Primary update cycle (returns event and updates internal debounced status)
  SafetyBeamEvent update(uint32_t now, bool isClosing);

  // Status queries
  bool isBlocked() const { return _debouncedBlocked; }
  bool wasPrevBlocked() const { return _prevBlocked; }
  bool hasPassageCrossed() const { return _passageCrossed; }
  void setPassageCrossed(bool crossed) { _passageCrossed = crossed; }
  void clearPassage() { _passageCrossed = false; }
  bool& passageCrossedRef() { return _passageCrossed; }
  void reset();

  // Boot multi-sampling
  bool multiSample(uint8_t sensorType, uint8_t samples = 5, uint32_t delayMs = 5);

private:
  const uint8_t _pin;
  bool _initialized = false;
  bool _lastRaw = false;
  uint32_t _rawStableStartMs = 0;
  bool _debouncedBlocked = false;
  bool _prevBlocked = false;
  bool _passageCrossed = false;
};
