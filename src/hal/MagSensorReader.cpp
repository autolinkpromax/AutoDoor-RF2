#include "MagSensorReader.h"
#include "app/globals.h"

MagSensorReader::MagSensorReader(uint8_t extMagPin, uint8_t builtinOpenPin, uint8_t builtinClosePin)
  : _extMagPin(extMagPin), _builtinOpenPin(builtinOpenPin), _builtinClosePin(builtinClosePin) {}

void MagSensorReader::init() {
  pinMode(_extMagPin, INPUT_PULLUP);
}

MagSensorEvent MagSensorReader::update(uint32_t now, DoorState currentDoorState) {
  if (!gMagSensorEnabled) {
    return MagSensorEvent::None;
  }

  MagSensorEvent result = MagSensorEvent::None;

  // 1. External Magnetic Sensor (100ms Debounce)
  const bool rawClosed = (digitalRead(_extMagPin) == (gMagSensorType == 1 ? HIGH : LOW));

  if (!_extInitialized) {
    _extInitialized = true;
    _lastRawExtClosed = rawClosed;
    _rawExtStableStartMs = now;
    _debouncedExtClosed = rawClosed;
  }

  if (rawClosed != _lastRawExtClosed) {
    _lastRawExtClosed = rawClosed;
    _rawExtStableStartMs = now;
  } else if ((uint32_t)(now - _rawExtStableStartMs) >= 100) {
    if (!_debouncedExtClosed && rawClosed) {
      result = MagSensorEvent::ReachedClosed;
    } else if (_debouncedExtClosed && !rawClosed) {
      if (currentDoorState == DoorState::Closed) {
        result = MagSensorEvent::ExternallyOpened;
      }
    }
    _debouncedExtClosed = rawClosed;
  }
  gMagSensorStableClosed = _debouncedExtClosed;

  return result;
}

bool MagSensorReader::multiSampleExtMag(uint8_t sensorType, uint8_t samples, uint32_t delayMs) {
  uint8_t closedCount = 0;
  for (uint8_t i = 0; i < samples; ++i) {
    if (digitalRead(_extMagPin) == (sensorType == 1 ? HIGH : LOW)) {
      closedCount++;
    }
    delay(delayMs);
  }
  const bool rawClosed = (closedCount >= (samples / 2 + 1));
  _debouncedExtClosed = rawClosed;
  _lastRawExtClosed = rawClosed;
  _extInitialized = true;
  gMagSensorStableClosed = rawClosed;
  return rawClosed;
}

void MagSensorReader::multiSampleBuiltin(bool swap, bool& outOpen, bool& outClose, uint8_t samples, uint32_t delayMs) {
  (void)swap;
  (void)samples;
  (void)delayMs;
  outOpen = false;
  outClose = false;
  _builtinOpen = false;
  _builtinClose = false;
  gBuiltinMagOpen = false;
  gBuiltinMagClose = false;
}
