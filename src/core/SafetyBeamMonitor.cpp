#include "SafetyBeamMonitor.h"
#include "app/globals.h"

SafetyBeamMonitor::SafetyBeamMonitor(uint8_t pin)
  : _pin(pin) {}

void SafetyBeamMonitor::init() {
  pinMode(_pin, INPUT_PULLUP);
}

void SafetyBeamMonitor::reset() {
  _initialized = false;
  _lastRaw = false;
  _rawStableStartMs = 0;
  _debouncedBlocked = false;
  _prevBlocked = false;
  _passageCrossed = false;
}

SafetyBeamEvent SafetyBeamMonitor::update(uint32_t now, bool isClosing) {
  if (!gSafetySensorEnabled) {
    _debouncedBlocked = false;
    _prevBlocked = false;
    _passageCrossed = false;
    gSafetySensorBlocked = false;
    return SafetyBeamEvent::None;
  }

  const bool rawBlocked = (digitalRead(_pin) == (gSafetySensorType == 1 ? HIGH : LOW));

  if (!_initialized) {
    _initialized = true;
    _lastRaw = rawBlocked;
    _rawStableStartMs = now;
    _debouncedBlocked = rawBlocked;
    _prevBlocked = rawBlocked;
    gSafetySensorBlocked = rawBlocked;
  }

  // Configurable Debounce Filter:
  // Beam break: gSafetyDebounceMs (when closing: minimum 80ms to prevent motor inrush noise)
  // Beam clear: min(gSafetyDebounceMs, 80ms) for prompt clearance detection
  const uint32_t effectiveDebounceMs = (gSafetyDebounceMs > 0) ? gSafetyDebounceMs : kDefaultSafetyDebounceMs;
  const uint32_t breakDebounceMs = isClosing ? (effectiveDebounceMs < 80 ? 80 : effectiveDebounceMs) : effectiveDebounceMs;
  const uint32_t clearDebounceMs = (effectiveDebounceMs < 80) ? effectiveDebounceMs : 80;
  const uint32_t debounceThresholdMs = rawBlocked ? breakDebounceMs : clearDebounceMs;

  if (rawBlocked != _lastRaw) {
    _lastRaw = rawBlocked;
    _rawStableStartMs = now;
  } else if ((uint32_t)(now - _rawStableStartMs) >= debounceThresholdMs) {
    _debouncedBlocked = rawBlocked;
  }

  gSafetySensorBlocked = _debouncedBlocked;

  SafetyBeamEvent ev = SafetyBeamEvent::None;
  if (_debouncedBlocked && !_prevBlocked) {
    ev = SafetyBeamEvent::BeamJustBroken;
  } else if (_debouncedBlocked) {
    ev = SafetyBeamEvent::BeamBlocked;
  } else if (!_debouncedBlocked && _prevBlocked) {
    ev = SafetyBeamEvent::BeamJustCleared;
  } else {
    ev = SafetyBeamEvent::BeamClear;
  }

  _prevBlocked = _debouncedBlocked;
  return ev;
}

bool SafetyBeamMonitor::multiSample(uint8_t sensorType, uint8_t samples, uint32_t delayMs) {
  uint8_t blockedCount = 0;
  for (uint8_t i = 0; i < samples; ++i) {
    if (digitalRead(_pin) == (sensorType == 1 ? HIGH : LOW)) {
      blockedCount++;
    }
    delay(delayMs);
  }
  const bool isBlocked = (blockedCount >= (samples / 2 + 1));
  _debouncedBlocked = isBlocked;
  _prevBlocked = isBlocked;
  _lastRaw = isBlocked;
  _initialized = true;
  gSafetySensorBlocked = isBlocked;
  return isBlocked;
}
