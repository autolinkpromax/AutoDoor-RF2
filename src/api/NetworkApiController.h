#pragma once

#include "BaseController.h"
#include <WiFi.h>

/**
 * @brief Controller for WiFi Station, WiFi Scan, and SoftAP management.
 */
class NetworkApiController : public BaseController {
public:
  explicit NetworkApiController(WebServer& server) : BaseController(server) {}

  void registerRoutes() override;

private:
  int _wifiScanCount{0};
  bool _wifiScanDone{false};
  uint32_t _wifiScanStartMs{0};

  void handleWifiScanStart();
  void handleWifiScanStatus();
  void handleWifiConnect();
  void handleWifiStatus();
  void handleWifiStatic();
  void handleApGet();
  void handleApPut();

  static String getWifiStatusString(wl_status_t status);
};
