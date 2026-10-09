#include "NetworkApiController.h"
#include "app/globals.h"
#include "modules/esp_now_runtime.h"
#include "modules/wifi_manager.h"

void NetworkApiController::registerRoutes() {
  _server.on("/api/wifi/scan", HTTP_GET, [this]() {
    wrap([this]() { handleWifiScanStart(); });
  });

  _server.on("/api/wifi/scan/status", HTTP_GET, [this]() {
    wrap([this]() { handleWifiScanStatus(); });
  });

  _server.on("/api/wifi/connect", HTTP_POST, [this]() {
    wrap([this]() { handleWifiConnect(); });
  });

  _server.on("/api/wifi/status", HTTP_GET, [this]() {
    wrap([this]() { handleWifiStatus(); });
  });

  _server.on("/api/wifi/static", HTTP_POST, [this]() {
    wrap([this]() { handleWifiStatic(); });
  });

  _server.on("/api/ap", HTTP_GET, [this]() {
    wrap([this]() { handleApGet(); });
  });

  _server.on("/api/ap", HTTP_PUT, [this]() {
    wrap([this]() { handleApPut(); });
  });

  _server.on("/api/ap", HTTP_POST, [this]() {
    wrap([this]() { handleApPut(); });
  });
}

void NetworkApiController::handleWifiScanStart() {
  const int scanState = WiFi.scanComplete();
  if (scanState == WIFI_SCAN_RUNNING) {
    Serial.println("[wifi] scan already running. ignoring request.");
    JsonDocument doc;
    doc["ok"] = true;
    doc["started"] = true;
    sendJson(200, doc);
    return;
  }

  // 1. Ensure STA mode is active (ESP32 scanNetworks requires WIFI_MODE_STA or WIFI_MODE_AP_STA)
  if (!(WiFi.getMode() & WIFI_MODE_STA)) {
    WiFi.enableSTA(true);
    delay(50);
  }

  // 2. Suspend ESP-NOW and set global scan start timer so background loop doesn't prematurely resume
  Serial.println("[wifi] scan requested. suspending ESP-NOW...");
  suspendEspNow();
  gWifiScanStartMs = millis();
  _wifiScanStartMs = millis();
  _wifiScanDone = false;
  delay(50);
  
  // 3. Start async scan
  _wifiScanCount = WiFi.scanNetworks(/*async=*/true, /*show_hidden=*/true);
  Serial.printf("[wifi] scan started. scanNetworks status: %d\n", _wifiScanCount);

  if (_wifiScanCount == WIFI_SCAN_FAILED) {
    WiFi.scanDelete();
    delay(50);
    _wifiScanCount = WiFi.scanNetworks(/*async=*/true, /*show_hidden=*/false);
    Serial.printf("[wifi] scan retry status: %d\n", _wifiScanCount);
  }

  JsonDocument doc;
  if (_wifiScanCount == WIFI_SCAN_FAILED) {
    resumeEspNow();
    doc["ok"] = false;
    doc["error"] = "scan_failed";
    sendJson(500, doc);
  } else {
    doc["ok"] = true;
    doc["started"] = true;
    sendJson(200, doc);
  }
}

void NetworkApiController::handleWifiScanStatus() {
  const int n = WiFi.scanComplete();
  Serial.printf("[wifi] scan status poll: scanComplete status = %d\n", n);
  
  if (n == WIFI_SCAN_RUNNING) {
    JsonDocument doc;
    doc["ok"] = true;
    doc["done"] = false;
    sendJson(200, doc);
    return;
  }
  
  // Resume ESP-NOW since the scan is finished (succeeded or failed)
  resumeEspNow();
  
  JsonDocument doc;
  if (n == WIFI_SCAN_FAILED) {
    Serial.println("[wifi] scan failed (-2)");
    doc["ok"] = false;
    doc["done"] = true;
    doc["error"] = "scan_failed";
    sendJson(200, doc);
    return;
  }
  
  doc["ok"] = true;
  doc["done"] = true;
  JsonArray arr = doc["networks"].to<JsonArray>();
  const int count = n >= 0 ? min(n, 24) : 0;
  Serial.printf("[wifi] scan finished. found %d networks. showing up to %d\n", n, count);
  for (int i = 0; i < count; ++i) {
    JsonObject o = arr.add<JsonObject>();
    o["ssid"] = WiFi.SSID(i);
    o["rssi"] = WiFi.RSSI(i);
    o["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    Serial.printf("   - SSID: '%s', RSSI: %d, Secure: %s\n", 
                  WiFi.SSID(i).c_str(), WiFi.RSSI(i), 
                  (WiFi.encryptionType(i) != WIFI_AUTH_OPEN) ? "yes" : "no");
  }
  if (n >= 0) {
    WiFi.scanDelete();
  }
  sendJson(200, doc);
}

void NetworkApiController::handleWifiConnect() {
  TagStoreLock lock;
  JsonDocument body = parseBodyOrEmpty();
  const String ssid = String((const char*)(body["ssid"] | ""));
  const String password = String((const char*)(body["password"] | ""));
  if (ssid.isEmpty()) {
    JsonDocument err;
    err["ok"] = false;
    err["error"] = "ssid_required";
    sendJson(400, err);
    return;
  }
  Serial.printf("[wifi] connect requested. SSID: '%s'\n", ssid.c_str());

  const String oldSsid = gPrefs.getString("wifiSsid", "");
  if (oldSsid != ssid) {
    // Reset Fix IP to DHCP whenever switching to a new Wi-Fi network to prevent subnet mismatch
    gPrefs.putBool("wifiStatic", false);
    gWifiStatic = false;
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    Serial.println("[wifi] New SSID detected -> resetting Fix IP to DHCP for new network");
  }

  gPrefs.putString("wifiSsid", ssid);
  gPrefs.putString("wifiPass", password);
  invalidateStaSsidCache();
  invalidateStaPassCache();
  
  // Enable auto-reconnect so the driver can manage channel switching and connection events
  Serial.println("[wifi] enabling auto-reconnect for connection attempt...");
  WiFi.setAutoReconnect(true);
  
  // Disconnect any active/pending STA connection first to prevent "is connecting" error
  Serial.println("[wifi] disconnecting current sta before calling begin...");
  WiFi.disconnect();
  delay(100);
  
  applyStaConfig();
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.println("[wifi] WiFi.begin called.");
  
  JsonDocument doc;
  doc["ok"] = true;
  sendJson(200, doc);
}

String NetworkApiController::getWifiStatusString(wl_status_t status) {
  switch (status) {
    case WL_CONNECTED:
      return "CONNECTED";
    case WL_NO_SSID_AVAIL:
      return "SSID_NOT_FOUND";
    case WL_CONNECT_FAILED:
      return "CONNECT_FAILED";
    case WL_CONNECTION_LOST:
      return "CONNECTION_LOST";
    case WL_DISCONNECTED:
      return "DISCONNECTED";
    case WL_IDLE_STATUS:
      return "CONNECTING";
    case WL_SCAN_COMPLETED:
      return "SCAN_COMPLETED";
    default:
      return "UNKNOWN";
  }
}

void NetworkApiController::handleWifiStatus() {
  wl_status_t status = WiFi.status();
  Serial.printf("[wifi] status poll. WiFi.status = %d (%s), IP = %s, SSID = %s\n",
                (int)status, getWifiStatusString(status).c_str(),
                WiFi.localIP().toString().c_str(), WiFi.SSID().c_str());
                
  JsonDocument doc;
  doc["ok"] = true;
  doc["status"] = (int)status;
  doc["statusName"] = getWifiStatusString(status);
  
  String thDesc = "กำลังเชื่อมต่อ...";
  if (status == WL_CONNECTED) {
    thDesc = "เชื่อมต่อสำเร็จ (IP: " + WiFi.localIP().toString() + ")";
  } else if (status == WL_NO_SSID_AVAIL) {
    thDesc = "ไม่พบเครือข่าย Wi-Fi ที่ระบุ";
  } else if (status == WL_CONNECT_FAILED) {
    thDesc = "เชื่อมต่อล้มเหลว (รหัสผ่านผิดพลาด)";
  } else if (status == WL_DISCONNECTED) {
    thDesc = "ไม่ได้เชื่อมต่อ";
  } else if (status == WL_IDLE_STATUS) {
    thDesc = "กำลังเริ่มเชื่อมต่อ...";
  }
  doc["statusThai"] = thDesc;
  doc["ip"] = WiFi.localIP().toString();
  doc["ssid"] = WiFi.SSID();
  const String host = resolveHostname();
  doc["hostname"] = host;
  doc["localUrl"] = "http://" + host + ".local";
  sendJson(200, doc);
}

void NetworkApiController::handleWifiStatic() {
  TagStoreLock lock;
  JsonDocument body = parseBodyOrEmpty();

  const bool enabled = body["enabled"] | false;
  JsonDocument doc;
  if (enabled) {
    if (WiFi.status() != WL_CONNECTED) {
      doc["ok"] = false;
      doc["error"] = "sta_not_connected";
      sendJson(400, doc);
      return;
    }
    snapshotStaIpToPrefs();
    gPrefs.putBool("wifiStatic", true);
    gWifiStatic = true;
    applyStaConfig();
    WiFi.disconnect();
    delay(100);
    const String staSsid = gPrefs.getString("wifiSsid", "");
    const String staPass = gPrefs.getString("wifiPass", "");
    WiFi.begin(staSsid.c_str(), staPass.c_str());
  } else {
    gPrefs.putBool("wifiStatic", false);
    gWifiStatic = false;
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.disconnect();
    delay(100);
    const String staSsid = gPrefs.getString("wifiSsid", "");
    const String staPass = gPrefs.getString("wifiPass", "");
    if (!staSsid.isEmpty()) {
      WiFi.begin(staSsid.c_str(), staPass.c_str());
    }
  }
  doc["ok"] = true;
  doc["wifiStatic"] = gPrefs.getBool("wifiStatic", false);
  doc["wifiIp"] = gPrefs.getString("wifiIp", "");
  sendJson(200, doc);
}

void NetworkApiController::handleApGet() {
  JsonDocument doc;
  doc["ok"] = true;
  doc["ssid"] = gPrefs.getString("apSsid", "");
  doc["effectiveSsid"] = resolveApSsid();
  doc["passwordSet"] = !gPrefs.getString("apPass", "").isEmpty();
  doc["enabled"] = gPrefs.getBool("apUserEnabled", true);
  doc["idleMin"] = gPrefs.getUChar("apIdleMin", 1);
  doc["apEnabled"] = gApEnabled;
  doc["apStations"] = gApEnabled ? WiFi.softAPgetStationNum() : 0;
  doc["apIp"] = gApEnabled ? WiFi.softAPIP().toString() : "";
  doc["safeMode"] = gSafeMode;
  sendJson(200, doc);
}

void NetworkApiController::handleApPut() {
  TagStoreLock lock;
  JsonDocument body = parseBodyOrEmpty();
  if (!body["enabled"].isNull()) {
    gPrefs.putBool("apUserEnabled", body["enabled"].as<bool>());
  }
  if (!body["idleMin"].isNull()) {
    const uint8_t m = constrain(body["idleMin"].as<int>(), 1, 120);
    gPrefs.putUChar("apIdleMin", m);
  }
  invalidateApSettingsCache(); // Refresh cached apUserEnabled/apIdleMinutes on next access
  if (!body["ssid"].isNull()) {
    String ssid = body["ssid"].as<String>();
    ssid.trim();
    if (ssid.length() > 32) {
      JsonDocument err;
      err["ok"] = false;
      err["error"] = "ap_ssid_too_long";
      sendJson(400, err);
      return;
    }
    gPrefs.putString("apSsid", ssid);
  }
  if (!body["password"].isNull()) {
    const String pass = body["password"].as<String>();
    if (!pass.isEmpty() && pass.length() < 8) {
      JsonDocument err;
      err["ok"] = false;
      err["error"] = "ap_pass_short";
      sendJson(400, err);
      return;
    }
    if (!pass.isEmpty()) {
      gPrefs.putString("apPass", pass);
    }
  }
  startSoftAp();
  gApLastHadStationsMs = millis();
  JsonDocument doc;
  doc["ok"] = true;
  doc["effectiveSsid"] = resolveApSsid();
  doc["apEnabled"] = gApEnabled;
  doc["apStations"] = gApEnabled ? WiFi.softAPgetStationNum() : 0;
  sendJson(200, doc);
}
