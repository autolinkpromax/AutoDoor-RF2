#pragma once

#include <Arduino.h>
#include <esp_timer.h>
#include "app/config.h"

// ============================================================================
// MotorActuator: Hardware abstraction & state machine for door motor relays
// ============================================================================
// Encapsulates the double-pulse relay control logic (Pulse 1 -> Gap -> Pulse 2),
// atomic register writes on GPIO pins, FreeRTOS critical sections, and hardware
// timer fail-safe cutoff.
//
// Invariants:
// - Motor Control GPIO Mapping:
//     OPEN:  GPIO 6 = HIGH (500 ms)
//     CLOSE: GPIO 7 = HIGH (500 ms, or 800 ms after reverse/stop)
//     STOP:  GPIO 6 & GPIO 7 = HIGH simultaneously via atomic register write (1200 ms)
//     IDLE:  GPIO 6 & GPIO 7 = LOW
// - Atomic register writes (GPIO_OUT_W1TS_REG / GPIO_OUT_W1TC_REG) eliminate pin skew.
// - Hardware timer (esp_timer) ensures guaranteed de-energize even if loopTask hangs.

enum class MotorPhase : uint8_t {
  Idle = 0,
  PulsingOpen,
  PulsingClose,
  PulsingStop
};

enum class MotorStage : uint8_t {
  Idle = 0,
  Pulse1,
  Gap,
  Pulse2
};

class MotorActuator {
public:
  explicit MotorActuator(uint8_t openPin = kBoardOpenPin, uint8_t closePin = kBoardClosePin);
  ~MotorActuator();

  // Non-copyable
  MotorActuator(const MotorActuator&) = delete;
  MotorActuator& operator=(const MotorActuator&) = delete;

  // Lifecycle
  void init();
  void step();

  // Primary Motor Actions
  bool pulseOpen(uint32_t durationMs = 0);
  bool pulseClose(uint32_t durationMs = 0);
  bool pulseStop(uint32_t durationMs = 0);

  // Status & Protection
  bool isBusy() const;
  void forceShutdown();
  void triggerChirp(uint8_t count);

  // State inspection
  MotorPhase phase() const { return _phase; }
  MotorStage stage() const { return _stage; }
  uint8_t openPin() const { return _openPin; }
  uint8_t closePin() const { return _closePin; }

  // Backward-compatibility aliases (matching legacy relay.h signatures)
  bool pulseRelayOpen(uint32_t durationMs = 0) { return pulseOpen(durationMs); }
  bool pulseRelayClose(uint32_t durationMs = 0) { return pulseClose(durationMs); }
  bool pulseRelayStop(uint32_t durationMs = 0) { return pulseStop(durationMs); }
  bool isRelayBusy() const { return isBusy(); }
  void forceRelayCutoff() { forceShutdown(); }
  void triggerMultiStopChirp(uint8_t count) { triggerChirp(count); }

private:
  const uint8_t _openPin;
  const uint8_t _closePin;
  const uint32_t _openMask;
  const uint32_t _closeMask;
  const uint32_t _bothMask;

  volatile MotorPhase _phase = MotorPhase::Idle;
  volatile MotorStage _stage = MotorStage::Idle;
  volatile uint32_t _stageEndMs = 0;
  volatile uint32_t _pulseWidthMs = 0;
  volatile uint8_t _pulseGen = 0;  // generation counter to guard against race conditions
  portMUX_TYPE _mux;

  esp_timer_handle_t _hwTimer = nullptr;

  uint8_t _chirpRemaining = 0;
  uint32_t _chirpNextMs = 0;
  bool _chirpStateOn = false;

  void driveBoardSignals(bool openActive, bool closeActive);
  void drivePhaseSignals(MotorPhase phase);
  bool startPulse(MotorPhase phase, uint32_t customDurationMs = 0);

  static void IRAM_ATTR onHardwareTimer(void* arg);
};

// Global instance for Composition Root / backward compatibility
extern MotorActuator gMotor;
