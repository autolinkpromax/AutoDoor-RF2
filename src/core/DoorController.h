#pragma once

#include <Arduino.h>
#include <vector>
#include "app/types.h"
#include "core/IDoorExecutor.h"
#include "core/IDoorListener.h"
#include "core/SafetyBeamMonitor.h"
#include "hal/MagSensorReader.h"
#include "core/AutoCloseEngine.h"
#include "core/ReversalDeadband.h"
#include "hal/MotorActuator.h"
#include "tags/TagRegistry.h"

/**
 * @brief Master Door Controller (Coordinates Layers 1 - 5)
 *
 * Architecture:
 * - Layer 1: Sensors & Input Processing (FreeRTOS Queue, Reed/Limit sensors, Safety Beam)
 * - Layer 2: Tag Trigger Evaluation & Baseline Suppression
 * - Layer 3: Auto-Close Priority Engine (Priority 1 Safety, Priority 2 Fallback, Priority 3 Failsafe)
 * - Layer 4: Directional Reversal Deadband Actuator (2.4s Single STOP + Settling)
 * - Layer 5: Transition Timers & State Completion (Auto-Retry Close Circuit Breaker)
 */
class DoorController : public IDoorExecutor {
public:
  DoorController(MotorActuator& motor = gMotor,
                 TagRegistry& tagRegistry = gTagRegistry,
                 AutoCloseEngine& autoClose = gAutoCloseEngine);

  /**
   * @brief Boot initialization and sensor ground truth calibration.
   */
  void init();

  /**
   * @brief Initializes FreeRTOS command queue for thread-safe cross-task communication.
   */
  void initCommandQueue();

  /**
   * @brief Enqueue door action from FreeRTOS task or interrupt.
   */
  bool requestAction(DoorCmdAction action, bool manualCommand, bool manualHold = false, const char* tagId = nullptr, bool bypassPreStop = false, uint32_t customCloseDelayMs = 0);
  bool requestSlot(const char* slot, bool manualCommand, bool manualHold = false, const char* tagId = nullptr, bool bypassPreStop = false, uint32_t customCloseDelayMs = 0);
  bool requestSlot(const String& slot, bool manualCommand, bool manualHold = false, const char* tagId = nullptr, bool bypassPreStop = false, uint32_t customCloseDelayMs = 0) {
    return requestSlot(slot.c_str(), manualCommand, manualHold, tagId, bypassPreStop, customCloseDelayMs);
  }

  /**
   * @brief Direct pulse dispatch and directional deadband coordination (Layer 4).
   */
  bool executeSlot(const char* slot, bool manualCommand, bool manualHold = false, bool bypassPreStop = false) override;
  bool executeSlot(const String& slot, bool manualCommand, bool manualHold = false, bool bypassPreStop = false) override {
    return executeSlot(slot.c_str(), manualCommand, manualHold, bypassPreStop);
  }

  /**
   * @brief Main state machine step function (Executes Layers 1 through 5 in order).
   */
  void step();

  /**
   * @brief Synchronization helper when door reaches Closed state.
   */
  void noteDoorClosedSync(uint32_t now);

  /**
   * @brief State update helper when Closing pulse begins.
   */
  void noteClosingStarted(uint32_t now);

  /**
   * @brief Checks if door is in safe, quiescent state for restart.
   */
  bool isSafeToRestart() const;

  /**
   * @brief Checks if a directional reversal is pending.
   */
  bool isReversing() const;

  /**
   * @brief String representation of door state.
   */
  static const char* stateToString(DoorState s);

  /**
   * @brief Attach event observer listener.
   */
  void addListener(IDoorListener* listener);

  /**
   * @brief Detach event observer listener.
   */
  void removeListener(IDoorListener* listener);

  // Subsystem Accessors
  SafetyBeamMonitor& safetySensor() { return _safetySensor; }
  MagSensorReader& magSensor() { return _magSensor; }
  AutoCloseEngine& autoCloseEngine() { return _autoClose; }
  ReversalDeadband& reversal() { return _reversal; }
  DoorState state() const { return _doorState; }

  // Reference accessors for backward-compatible global variable binding
  DoorState& stateRef() { return _doorState; }
  uint32_t& transitionEndMsRef() { return _doorTransitionEndMs; }
  uint32_t& lastCloseOrClosingMsRef() { return _lastCloseOrClosingMs; }
  uint8_t& closeRetryCountRef() { return _closeRetryCount; }
  bool& isManualOpenStateRef() { return _isManualOpenState; }
  uint32_t& doubleCloseAtMsRef() { return _doubleCloseAtMs; }
  uint32_t& tagOpenDurationMsRef() { return _tagOpenDurationMs; }
  uint32_t& tagOpenStopAtMsRef() { return _tagOpenStopAtMs; }

private:
  void applyDoorCommandState(const char* slot, uint32_t now);
  bool pulseSlot(const char* slot, uint32_t durationMs = 0);
  uint32_t getEffectiveSafetyCloseDelay(uint32_t now);

  // Command Execution Helpers (executeSlot decomposition)
  bool executeOpen(bool manualCommand, bool manualHold, bool bypassPreStop, uint32_t now);
  bool executeStop(bool manualCommand, uint32_t now);
  bool executeClose(bool manualCommand, bool manualHold, bool bypassPreStop, uint32_t now);

  // Layer Processing Helpers (step decomposition)
  void processLayer1Sensors(uint32_t now);
  void processLayer2Tags(uint32_t now, bool& anyActiveTotal, bool& anyActiveNonSuppressed);
  void processLayer4Reversal(uint32_t now);
  void processLayer5Timers(uint32_t now);
  void processIdleSanitizer(uint32_t now, bool anyActiveTotal);

  // Component Dependencies
  MotorActuator& _motor;
  TagRegistry& _tagRegistry;
  AutoCloseEngine& _autoClose;
  SafetyBeamMonitor _safetySensor;
  MagSensorReader _magSensor;
  ReversalDeadband _reversal;

  // FreeRTOS Command Queue
  QueueHandle_t _cmdQueue{nullptr};

  // Observers
  std::vector<IDoorListener*> _listeners;

  // Door State Variables
  DoorState _doorState{DoorState::Closed};
  uint32_t _doorOpenedAtMs{0};
  uint32_t _doorTransitionEndMs{0};
  uint32_t _doorMotionStoppedAtMs{0};
  uint32_t _lastCloseOrClosingMs{0};
  uint32_t _lastStopPulseMs{0};
  uint32_t _lastSafeAbortMs{0};
  uint32_t _suppressionReleaseAtMs{0};

  uint32_t _doubleCloseAtMs{0};
  uint32_t _closeRetryAtMs{0};
  uint8_t _closeRetryCount{0};

  uint32_t _tagOpenDurationMs{0};
  uint32_t _tagOpenStopAtMs{0};
  bool _isManualOpenState{false};
};

extern DoorController gDoorController;
