#pragma once

#include <Arduino.h>
#include "app/config.h"

/**
 * @brief Directional Reversal Deadband Actuator (Layer 4)
 *
 * Enforces motor protection during directional reversals between Open and Close:
 * Total 2.4s Deadband: Single STOP pulse (GPIO 6+7 = HIGH for 1200ms) followed by
 * 1.2s settling deadband before firing the reverse command pulse.
 */
class ReversalDeadband {
public:
  ReversalDeadband() = default;

  /**
   * @brief Schedules a reversal action after deadband expiration.
   */
  void schedule(const char* slot, uint32_t targetAtMs, bool manualCommand, bool manualHold);
  void schedule(const String& slot, uint32_t targetAtMs, bool manualCommand, bool manualHold) {
    schedule(slot.c_str(), targetAtMs, manualCommand, manualHold);
  }

  /**
   * @brief Cancels any pending reversal.
   */
  void cancel();

  /**
   * @brief Checks if a directional reversal is currently pending.
   */
  bool isReversing() const { return _targetAtMs != 0; }

  /**
   * @brief Checks if a reversal is pending for a specific slot.
   */
  bool isReversingTo(const char* target) const {
    if (_targetAtMs == 0 || !_slot || !target) return false;
    return (_slot == target || strcmp(_slot, target) == 0);
  }

  bool isReversingToClose() const { return isReversingTo("close"); }
  bool isReversingToOpen() const { return isReversingTo("open"); }

  const char* slot() const { return _slot ? _slot : ""; }
  uint32_t targetAtMs() const { return _targetAtMs; }
  bool isManual() const { return _manualCommand; }
  bool isHold() const { return _manualHold; }

  /**
   * @brief Checks if the deadband target timestamp has passed.
   */
  bool isDue(uint32_t now) const {
    return _targetAtMs != 0 && (int32_t)(now - _targetAtMs) >= 0;
  }

  /**
   * @brief Consumes the pending reversal if due, returning the target command and clearing state.
   */
  bool consume(uint32_t now, const char*& outSlot, bool& outManual, bool& outHold);
  bool consume(uint32_t now, String& outSlot, bool& outManual, bool& outHold);

  // References for backward compatibility binding
  uint32_t& targetAtMsRef() { return _targetAtMs; }
  bool& manualRef() { return _manualCommand; }
  bool& holdRef() { return _manualHold; }

private:
  const char* _slot{""};
  uint32_t _targetAtMs{0};
  bool _manualCommand{false};
  bool _manualHold{false};
};
