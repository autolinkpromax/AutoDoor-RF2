#pragma once

#include "BaseController.h"

/**
 * @brief Controller for system health, status, authentication, OTA upload, reboot, and safe mode.
 */
class SystemApiController : public BaseController {
public:
  explicit SystemApiController(WebServer& server) : BaseController(server) {}

  void registerRoutes() override;

private:
  void handleHealth();
  void handleStatus();
  void handleLive();
  void handleLogin();
  void handleWebAuthGet();
  void handleWebAuthPut();
  void handleRestart();
  void handleEnterSafeMode();
  void handleExitSafeMode();
  void handleOtaFinish();
  void handleOtaUpload();
  void handleWebhookGet();
  void handleWebhookSave();
  void handleWebhookTest();
  void handleResetService();
  void handleMaintenanceSettings();
  void handleFactoryReset();
};
