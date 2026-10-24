#pragma once

#include "BaseController.h"

/**
 * @brief Controller for system settings, NTP time sync, schedules, Blynk, and ESP-NOW state.
 */
class ConfigApiController : public BaseController {
public:
  explicit ConfigApiController(WebServer& server) : BaseController(server) {}

  void registerRoutes() override;

private:
  void handleSettingsGet();
  void handleSettingsPut();
  void handleTimeGet();
  void handleTimeSync();
  void handleScheduleGet();
  void handleSchedulePut();
  void handleBlynkGet();
  void handleBlynkPut();
  void handleMqttGet();
  void handleMqttPut();
  void handleEspNowDisable();
  void handleEspNowEnable();
};
