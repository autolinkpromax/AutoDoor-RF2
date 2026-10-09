#pragma once

#include <Arduino.h>

/**
 * @brief Abstract interface for executing door motion commands.
 * Implemented by DoorController / door logic bridge to decouple
 * engines (AutoCloseEngine, ReversalDeadband) from hardware actuation details.
 */
class IDoorExecutor {
public:
  virtual ~IDoorExecutor() = default;

  /**
   * @brief Execute a door slot action.
   * @param slot "open", "close", or "stop"
   * @param manualCommand True if triggered by user button/API
   * @param manualHold True if holding open permanently
   * @param bypassPreStop True if STOP deadband already completed
   * @return true if command was dispatched successfully
   */
  virtual bool executeSlot(const char* slot, bool manualCommand, bool manualHold, bool bypassPreStop = false) = 0;
  virtual bool executeSlot(const String& slot, bool manualCommand, bool manualHold, bool bypassPreStop = false) {
    return executeSlot(slot.c_str(), manualCommand, manualHold, bypassPreStop);
  }
};
