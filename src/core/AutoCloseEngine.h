#pragma once

#include <Arduino.h>
#include "app/types.h"
#include "core/IDoorExecutor.h"

/**
 * @brief Context structure passed to AutoCloseEngine for decision evaluation.
 */
struct AutoCloseContext {
  uint32_t now;
  DoorState doorState;
  bool isSafeMode;
  bool isTagAutoEnabled;
  bool isManualOpenState;
  bool isSafetySensorEnabled;
  bool isSafetySensorBlocked;
  bool hasPassageCrossed;
  bool anyActiveNonSuppressed;
  uint32_t doorOpenedAtMs;
  uint32_t fallbackDelayMs;
  bool isReversing;
};

/**
 * @brief Auto-Close Priority Engine (Layer 3)
 *
 * Implements the 3-Tier Priority Auto-Close hierarchy:
 * - Priority 1: Safety Sensor Auto-Close (Primary authority when passage occurred).
 *   Active tags in the garage must NEVER delay or prevent this safety close!
 * - Priority 2: Tag Departure & Fallback Auto-Close (Evaluates Open & Stopped states).
 * - Priority 3: Failsafe Max Open Timeout Protection (3 minutes open/stopped protection).
 */
class AutoCloseEngine {
public:
  using CallbackFn = void (*)();

  AutoCloseEngine() = default;

  /**
   * @brief Evaluates all 3 auto-close priorities and executes close command when due.
   */
  void update(const AutoCloseContext& ctx, IDoorExecutor* executor);

  /**
   * @brief Arm safety sensor auto-close timer (Priority 1).
   * Takes primary authority over pending fallback countdown.
   */
  void armSafetyClose(uint32_t targetCloseAtMs);

  /**
   * @brief Disarm safety sensor auto-close timer.
   */
  void disarmSafetyClose();

  /**
   * @brief Arm normal tag fallback auto-close countdown (Priority 2).
   */
  void armPendingClose(uint32_t targetCloseAtMs);

  /**
   * @brief Disarm normal tag fallback countdown.
   */
  void disarmPendingClose();

  /**
   * @brief Disarm all auto-close timers.
   */
  void disarmAll();

  /**
   * @brief Trip circuit breaker after excessive auto-retry failures.
   */
  void tripCircuitBreaker();

  /**
   * @brief Reset circuit breaker (on manual command or open pulse).
   */
  void resetCircuitBreaker();

  bool isAborted() const { return _autoCloseAborted; }
  bool isArmed() const { return _autoCloseArmed || _safetyCloseAtMs != 0 || _pendingCloseAtMs != 0; }
  bool isFallbackArmed() const { return _autoCloseArmed; }

  uint32_t safetyCloseAtMs() const { return _safetyCloseAtMs; }
  uint32_t pendingCloseAtMs() const { return _pendingCloseAtMs; }

  int32_t safetyRemainingMs(uint32_t now) const {
    return _safetyCloseAtMs ? (int32_t)((int32_t)_safetyCloseAtMs - (int32_t)now) : 0;
  }

  int32_t pendingRemainingMs(uint32_t now) const {
    return _pendingCloseAtMs ? (int32_t)((int32_t)_pendingCloseAtMs - (int32_t)now) : 0;
  }

  // Callbacks for notifying door controller / state
  void setOnPassageReset(CallbackFn fn) { _onPassageReset = fn; }
  void setOnFallbackSuccess(CallbackFn fn) { _onFallbackSuccess = fn; }

  // References for backward compatibility binding with globals
  uint32_t& pendingCloseAtMsRef() { return _pendingCloseAtMs; }
  uint32_t& safetyCloseAtMsRef() { return _safetyCloseAtMs; }
  bool& autoCloseArmedRef() { return _autoCloseArmed; }
  bool& autoCloseAbortedRef() { return _autoCloseAborted; }

private:
  uint32_t _pendingCloseAtMs{0};
  uint32_t _safetyCloseAtMs{0};
  uint32_t _lastFailsafeAtMs{0};
  bool _autoCloseArmed{false};
  bool _autoCloseAborted{false};

  CallbackFn _onPassageReset{nullptr};
  CallbackFn _onFallbackSuccess{nullptr};
};

extern AutoCloseEngine gAutoCloseEngine;
