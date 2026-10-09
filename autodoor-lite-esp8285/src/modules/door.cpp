#include "door.h"

#include <Arduino.h>

namespace {

uint8_t gPinOpen = 255;
uint8_t gPinClose = 255;
uint16_t gPulseMs = 500;
uint32_t gPulseEndAtMs = 0;
DoorState gState = DoorState::Stopped;

void driveIdle() {
  digitalWrite(gPinOpen, HIGH);
  digitalWrite(gPinClose, HIGH);
}

}  // namespace

namespace Door {

void begin(uint8_t pinOpen, uint8_t pinClose, uint16_t pulseMs) {
  gPinOpen = pinOpen;
  gPinClose = pinClose;
  pinMode(gPinOpen, OUTPUT);
  pinMode(gPinClose, OUTPUT);
  setPulseMs(pulseMs);
  driveIdle();
}

void setPulseMs(uint16_t pulseMs) { gPulseMs = constrain(pulseMs, 100, 5000); }

void open() {
  if (gPinOpen == 255) {
    return;
  }
  driveIdle();
  digitalWrite(gPinOpen, LOW);
  gPulseEndAtMs = millis() + gPulseMs;
  gState = DoorState::Opening;
}

void close() {
  if (gPinClose == 255) {
    return;
  }
  driveIdle();
  digitalWrite(gPinClose, LOW);
  gPulseEndAtMs = millis() + gPulseMs;
  gState = DoorState::Closing;
}

void stop() {
  driveIdle();
  gPulseEndAtMs = 0;
  gState = DoorState::Stopped;
}

void step() {
  if (gPulseEndAtMs == 0) {
    return;
  }
  if ((int32_t)(millis() - gPulseEndAtMs) >= 0) {
    driveIdle();
    if (gState == DoorState::Opening) {
      gState = DoorState::Opened;
    } else if (gState == DoorState::Closing) {
      gState = DoorState::Closed;
    }
    gPulseEndAtMs = 0;
  }
}

DoorState state() { return gState; }

}  // namespace Door
