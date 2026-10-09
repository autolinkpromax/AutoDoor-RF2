#pragma once

#include "app/config.h"
#include "app/types.h"
#include <Arduino.h>

// ============================================================================
// MagSensorReader: External Magnetic Sensor & Built-in Limit Switch Reader
// ============================================================================
// Encapsulates:
// 1. External magnetic reed switch on door frame (Pin 3 `kMagSensorPin`) with
//    100ms stable debounce.
// 2. Built-in motor limit switches (Pins 4 & 5 `kBuiltinMagOpenPin`, `kBuiltinMagClosePin`).
// 3. Pin-change diagnostic logging for built-in limit switches.

enum class MagSensorEvent : uint8_t {
  None = 0,
  ReachedClosed,       // Magnet or limit confirms door reached fully CLOSED
  ExternallyOpened,    // External source (house remote / manual switch) opened door from Closed
  ReachedOpen          // Built-in limit switch confirms door reached fully OPEN
};

class MagSensorReader {
public:
  explicit MagSensorReader(uint8_t extMagPin = kMagSensorPin,
                           uint8_t builtinOpenPin = kBuiltinMagOpenPin,
                           uint8_t builtinClosePin = kBuiltinMagClosePin);

  void init();
  MagSensorEvent update(uint32_t now, DoorState currentDoorState);

  // Status queries
  bool isExtMagStableClosed() const { return _debouncedExtClosed; }
  bool isBuiltinOpen() const { return _builtinOpen; }
  bool isBuiltinClose() const { return _builtinClose; }

  // Boot multi-sampling
  bool multiSampleExtMag(uint8_t sensorType, uint8_t samples = 5, uint32_t delayMs = 5);
  void multiSampleBuiltin(bool swap, bool& outOpen, bool& outClose, uint8_t samples = 5, uint32_t delayMs = 5);

private:
  const uint8_t _extMagPin;
  const uint8_t _builtinOpenPin;
  const uint8_t _builtinClosePin;

  // External Mag Sensor debounce state
  bool _extInitialized = false;
  bool _lastRawExtClosed = false;
  uint32_t _rawExtStableStartMs = 0;
  bool _debouncedExtClosed = false;

  // Built-in limit switches state
  bool _builtinOpen = false;
  bool _builtinClose = false;
  int _prevRawOpenPin = -1;
  int _prevRawClosePin = -1;
};
