#pragma once

#include "app/types.h"

/**
 * @brief Observer interface for door state events.
 * Enables services (HTTP API, Status LED, ESP-NOW broadcast) to react to door state changes.
 */
class IDoorListener {
public:
  virtual ~IDoorListener() = default;

  /**
   * @brief Invoked whenever door state changes (e.g. Closed -> Opening).
   */
  virtual void onDoorStateChanged(DoorState oldState, DoorState newState) {}

  /**
   * @brief Invoked on physical door closure synchronization.
   */
  virtual void onDoorClosed() {}

  /**
   * @brief Invoked on emergency safe abort (obstacle during close).
   */
  virtual void onSafeAbort() {}
};
