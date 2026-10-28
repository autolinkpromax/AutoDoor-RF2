#pragma once

#include "BaseController.h"

/**
 * @brief Controller for Door actuation endpoints, emergency stop, and webhooks.
 */
class DoorApiController : public BaseController {
public:
  explicit DoorApiController(WebServer& server) : BaseController(server) {}

  void registerRoutes() override;

private:
  void handleDoorPost();
  void handleEmergencyStop();
  void handleHookDoorSlot(const char* slot);
  void handleHookDoorTagOpen();
  void handleHookTagAutoToggle();
  bool hookKeyValid();
};
