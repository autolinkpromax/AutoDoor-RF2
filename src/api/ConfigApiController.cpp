#include "ConfigApiController.h"
#include "app/config.h"
#include "app/globals.h"
#include "modules/blynk_service.h"
#include "modules/mqtt_service.h"
#include "modules/door.h"
#include "modules/esp_now_runtime.h"
#include "modules/ntp.h"
#include "modules/schedule.h"
#include "modules/wifi_manager.h"
#include "tags/TagRegistry.h"
#include <WiFi.h>
#include <time.h>

void ConfigApiController::registerRoutes() {
  _server.on("/api/settings", HTTP_GET, [this]() {
    wrap([this]() { handleSettingsGet(); });
  });

  _server.on("/api/settings", HTTP_PUT, [this]() {
    wrap([this]() { handleSettingsPut(); });
  });

  _server.on("/api/time", HTTP_GET, [this]() {
    wrap([this]() { handleTimeGet(); });
  });

  _server.on("/api/time/sync", HTTP_POST, [this]() {
    wrap([this]() { handleTimeSync(); });
  });

  _server.on("/api/schedule", HTTP_GET, [this]() {
    wrap([this]() { handleScheduleGet(); });
  });

  _server.on("/api/schedule", HTTP_PUT, [this]() {
    wrap([this]() { handleSchedulePut(); });
  });

  _server.on("/api/blynk", HTTP_GET, [this]() {
    wrap([this]() { handleBlynkGet(); });
  });

  _server.on("/api/blynk", HTTP_PUT, [this]() {
    wrap([this]() { handleBlynkPut(); });
  });

  _server.on("/api/mqtt", HTTP_GET, [this]() {
    wrap([this]() { handleMqttGet(); });
  });

  _server.on("/api/mqtt", HTTP_PUT, [this]() {
    wrap([this]() { handleMqttPut(); });
  });

  _server.on("/api/mqtt", HTTP_POST, [this]() {
    wrap([this]() { handleMqttPut(); });
  });

  _server.on("/api/espnow/disable", HTTP_POST, [this]() {
    wrap([this]() { handleEspNowDisable(); });
  });

  _server.on("/api/espnow/enable", HTTP_POST, [this]() {
    wrap([this]() { handleEspNowEnable(); });
  });
}

void ConfigApiController::handleSettingsGet() {
  TagStoreLock lock;
  JsonDocument doc;
  doc["ok"] = true;
  doc["staConnected"] = (WiFi.status() == WL_CONNECTED);
  doc["staIp"] = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "";
  doc["staSsid"] = WiFi.SSID();
  doc["wifiStatic"] = gPrefs.getBool("wifiStatic", false);
  doc["wifiIp"] = gPrefs.getString("wifiIp", "");
  doc["wifiSsid"] = gPrefs.getString("wifiSsid", "");
  doc["tagAutoEnabled"] = gTagAutoEnabled;
  doc["tagBuzzerEnabled"] = gTagBuzzerEnabled;
  doc["simMode"] = gSimMode;
  doc["motorReverse"] = gMotorReverse;
  doc["magSensorEnabled"] = gMagSensorEnabled;
  doc["magSensorType"] = gMagSensorType;
  doc["autoRetryCloseEnabled"] = gAutoRetryCloseEnabled;
  doc["maxCloseRetryCount"] = gMaxCloseRetryCount;
  doc["closeRetryDelayMs"] = gCloseRetryDelayMs;
  doc["doubleClosePulse"] = gDoubleClosePulseEnabled;
  doc["builtinSensorsEnabled"] = false;
  doc["builtinMagSwap"] = false;
  doc["safetySensorEnabled"] = gSafetySensorEnabled;
  doc["safetySensorType"] = gSafetySensorType;
  doc["safetyCloseDelayMs"] = gSafetyCloseDelayMs;
  doc["safetyReverseDelayMs"] = gSafetyReverseDelayMs;
  doc["remotePassageClose"] = gRemotePassageClose;
  doc["remotePassageDelayMs"] = gRemotePassageDelayMs;
  doc["doorTransitionMs"] = gDoorTransitionMs;
  doc["timeSynced"] = gTimeSynced;
  doc["deviceName"] = gDeviceName;
  const String host = resolveHostname();
  doc["hostname"] = host;
  doc["defaultHostname"] = sanitizeHostname(resolveApSsid());
  doc["localUrl"] = "http://" + host + ".local";
  doc["tagCount"] = gTagRegistry.count();
  doc["autoRebootEnabled"] = gAutoRebootEnabled;
  doc["autoRebootHour"] = gAutoRebootHour;
  doc["autoRebootMinute"] = gAutoRebootMinute;
  struct tm ti;
  if (getLocalTime(&ti)) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d", ti.tm_year + 1900, ti.tm_mon + 1,
             ti.tm_mday, ti.tm_hour, ti.tm_min, ti.tm_sec);
    doc["localTime"] = buf;
  } else {
    doc["localTime"] = nullptr;
  }
  sendJson(200, doc);
}

void ConfigApiController::handleSettingsPut() {
  TagStoreLock lock;
  JsonDocument body = parseBodyOrEmpty();
  if (!body["doorTransitionMs"].isNull()) {
    gDoorTransitionMs = constrain(body["doorTransitionMs"].as<uint32_t>(), 1000U, 60000U);
    gPrefs.putUInt("doorTransMs", gDoorTransitionMs);
  }
  if (!body["simMode"].isNull()) {
    gSimMode = body["simMode"].as<bool>();
    gPrefs.putBool("simMode", gSimMode);
  }
  if (!body["tagBuzzerEnabled"].isNull()) {
    gTagBuzzerEnabled = body["tagBuzzerEnabled"].as<bool>();
    gPrefs.putBool("tagBuzzerEn", gTagBuzzerEnabled);
    triggerImmediateAnnounce();
  }
  if (!body["motorReverse"].isNull()) {
    gMotorReverse = body["motorReverse"].as<bool>();
    gPrefs.putBool("motorReverse", gMotorReverse);
  }
  if (!body["magSensorEnabled"].isNull()) {
    gMagSensorEnabled = body["magSensorEnabled"].as<bool>();
    gPrefs.putBool("magSensEnabled", gMagSensorEnabled);
  }
  if (!body["magSensorType"].isNull()) {
    gMagSensorType = constrain(body["magSensorType"].as<uint8_t>(), 0U, 1U);
    gPrefs.putUChar("magSensType", gMagSensorType);
  }
  if (!body["autoRetryCloseEnabled"].isNull()) {
    gAutoRetryCloseEnabled = body["autoRetryCloseEnabled"].as<bool>();
    gPrefs.putBool("autoRetryEn", gAutoRetryCloseEnabled);
  }
  if (!body["maxCloseRetryCount"].isNull()) {
    gMaxCloseRetryCount = constrain(body["maxCloseRetryCount"].as<uint8_t>(), (uint8_t)1, (uint8_t)5);
    gPrefs.putUChar("maxCloseRetry", gMaxCloseRetryCount);
  }
  if (!body["closeRetryDelayMs"].isNull()) {
    gCloseRetryDelayMs = constrain(body["closeRetryDelayMs"].as<uint32_t>(), 1000U, 30000U);
    gPrefs.putUInt("retryCloseDelay", gCloseRetryDelayMs);
  }
  if (!body["doubleClosePulse"].isNull()) {
    gDoubleClosePulseEnabled = body["doubleClosePulse"].as<bool>();
    gPrefs.putBool("dblClose", gDoubleClosePulseEnabled);
  }
#if AUTODOOR_FEATURE_BUILTIN_MAG
  if (!body["builtinSensorsEnabled"].isNull()) {
    gBuiltinSensorsEnabled = body["builtinSensorsEnabled"].as<bool>();
    gPrefs.putBool("builtinSensEn", gBuiltinSensorsEnabled);
    if (gBuiltinSensorsEnabled) {
      gMagSensorEnabled = false;
      gPrefs.putBool("magSensEnabled", false);
    }
  }
  if (!body["builtinMagSwap"].isNull()) {
    gBuiltinMagSwap = body["builtinMagSwap"].as<bool>();
    gPrefs.putBool("builtinMagSwap", gBuiltinMagSwap);
  }
#else
  gBuiltinSensorsEnabled = false;
  gBuiltinMagSwap = false;
#endif
  if (!body["safetySensorEnabled"].isNull()) {
    gSafetySensorEnabled = body["safetySensorEnabled"].as<bool>();
    gPrefs.putBool("safeSensEnabled", gSafetySensorEnabled);
  }
  if (!body["safetySensorType"].isNull()) {
    gSafetySensorType = constrain(body["safetySensorType"].as<uint8_t>(), 0U, 1U);
    gPrefs.putUChar("safeSensType", gSafetySensorType);
  }
  if (!body["safetyCloseDelayMs"].isNull()) {
    gSafetyCloseDelayMs = body["safetyCloseDelayMs"].as<uint32_t>();
    gPrefs.putUInt("safeCloseDelay", gSafetyCloseDelayMs);
  }
  if (!body["safetyReverseDelayMs"].isNull()) {
    gSafetyReverseDelayMs = body["safetyReverseDelayMs"].as<uint32_t>();
    gPrefs.putUInt("safeRevDelay", gSafetyReverseDelayMs);
  }
  if (!body["remotePassageClose"].isNull()) {
    gRemotePassageClose = body["remotePassageClose"].as<bool>();
    gPrefs.putBool("remPassClose", gRemotePassageClose);
  }
  if (!body["remotePassageDelayMs"].isNull()) {
    gRemotePassageDelayMs = constrain(body["remotePassageDelayMs"].as<uint32_t>(), 1000U, 60000U);
    gPrefs.putUInt("remPassDelay", gRemotePassageDelayMs);
  }
  if (!body["espNowChannel"].isNull()) {
    const uint8_t ch = constrain(body["espNowChannel"].as<int>(), 1, 13);
    persistEspNowChannel(ch);
  }
  if (!body["deviceName"].isNull()) {
    String deviceName = body["deviceName"].as<String>();
    deviceName.trim();
    if (deviceName.length() > 0 && deviceName.length() <= 32) {
      gDeviceName = deviceName;
      gPrefs.putString("deviceName", gDeviceName);
    }
  }
  if (!body["hostname"].isNull()) {
    String h = body["hostname"].as<String>();
    h = sanitizeHostname(h);
    if (h.length() > 0 && h.length() <= 32) {
      gPrefs.putString("hostname", h);
    } else {
      gPrefs.remove("hostname");
    }
    startMdns();
  }
  if (!body["autoRebootEnabled"].isNull()) {
    gAutoRebootEnabled = body["autoRebootEnabled"].as<bool>();
    gPrefs.putBool("rebootEn", gAutoRebootEnabled);
  }
  if (!body["autoRebootHour"].isNull()) {
    gAutoRebootHour = constrain(body["autoRebootHour"].as<uint8_t>(), (uint8_t)0, (uint8_t)23);
    gPrefs.putUChar("rebootHour", gAutoRebootHour);
  }
  if (!body["autoRebootMinute"].isNull()) {
    gAutoRebootMinute = constrain(body["autoRebootMinute"].as<uint8_t>(), (uint8_t)0, (uint8_t)59);
    gPrefs.putUChar("rebootMin", gAutoRebootMinute);
  }
  if (!body["autoRebootTime"].isNull()) {
    const char* tStr = body["autoRebootTime"].as<const char*>();
    if (tStr && strlen(tStr) >= 3) {
      int h = 0, m = 0;
      if (sscanf(tStr, "%d:%d", &h, &m) >= 1) {
        gAutoRebootHour = (uint8_t)constrain(h, 0, 23);
        gAutoRebootMinute = (uint8_t)constrain(m, 0, 59);
        gPrefs.putUChar("rebootHour", gAutoRebootHour);
        gPrefs.putUChar("rebootMin", gAutoRebootMinute);
      }
    }
  }
  JsonDocument doc;
  doc["ok"] = true;
  const String host = resolveHostname();
  doc["hostname"] = host;
  doc["defaultHostname"] = sanitizeHostname(resolveApSsid());
  doc["localUrl"] = "http://" + host + ".local";
  sendJson(200, doc);
}

void ConfigApiController::handleTimeGet() {
  JsonDocument doc;
  doc["ok"] = true;
  doc["synced"] = gTimeSynced;
  struct tm ti;
  if (getLocalTime(&ti)) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d", ti.tm_year + 1900, ti.tm_mon + 1,
             ti.tm_mday, ti.tm_hour, ti.tm_min, ti.tm_sec);
    doc["localTime"] = buf;
  } else {
    doc["localTime"] = nullptr;
  }
  sendJson(200, doc);
}

void ConfigApiController::handleTimeSync() {
  const bool ok = syncTimeFromNtp();
  JsonDocument doc;
  doc["ok"] = ok;
  doc["synced"] = gTimeSynced;
  if (!ok) {
    doc["pending"] = true;
  }
  sendJson(ok ? 200 : 202, doc);
}

void ConfigApiController::handleScheduleGet() {
  JsonDocument doc;
  doc["ok"] = true;
  JsonArray arr = doc["schedules"].to<JsonArray>();
  for (const auto& e : gSchedules) {
    JsonObject o = arr.add<JsonObject>();
    o["id"] = e.id;
    o["enabled"] = e.enabled;
    o["action"] = e.action;
    o["hour"] = e.hour;
    o["minute"] = e.minute;
    JsonArray days = o["days"].to<JsonArray>();
    for (int d = 0; d < 7; ++d) {
      if (e.daysMask & (1u << d)) {
        days.add(d);
      }
    }
  }
  sendJson(200, doc);
}

void ConfigApiController::handleSchedulePut() {
  JsonDocument body = parseBodyOrEmpty();
  if (!body["schedules"].is<JsonArray>()) {
    JsonDocument err;
    err["ok"] = false;
    err["error"] = "schedules_required";
    sendJson(400, err);
    return;
  }
  gSchedules.clear();
  for (JsonObject o : body["schedules"].as<JsonArray>()) {
    ScheduleEntry e;
    e.id = String((const char*)(o["id"] | ""));
    if (e.id.isEmpty()) {
      e.id = makeId();
    }
    e.enabled = o["enabled"] | false;
    e.action = String((const char*)(o["action"] | "open"));
    if (e.action != "open" && e.action != "close") {
      e.action = "open";
    }
    e.hour = (uint8_t)constrain((int)(o["hour"] | 0), 0, 23);
    e.minute = (uint8_t)constrain((int)(o["minute"] | 0), 0, 59);
    e.daysMask = 0;
    if (o["days"].is<JsonArray>()) {
      for (JsonVariant dv : o["days"].as<JsonArray>()) {
        const int day = dv.as<int>();
        if (day >= 0 && day <= 6) {
          e.daysMask |= (uint8_t)(1u << day);
        }
      }
    }
    if (e.daysMask == 0) {
      e.daysMask = 0x7F;
    }
    gSchedules.push_back(e);
  }
  saveSchedulesToPrefs();
  JsonDocument doc;
  doc["ok"] = true;
  sendJson(200, doc);
}

void ConfigApiController::handleBlynkGet() {
  JsonDocument doc;
  doc["ok"] = true;
  doc["enabled"] = gPrefs.getBool("blynkEnabled", false);
  doc["token"] = gPrefs.getString("blynkToken", "");
  doc["templateId"] = gPrefs.getString("blynkTemplateId", "");
  doc["deviceName"] = gPrefs.getString("blynkDeviceName", "AutoDoor");
  doc["connected"] = gBlynkConnected;
  sendJson(200, doc);
}

void ConfigApiController::handleBlynkPut() {
  TagStoreLock lock;
  JsonDocument body = parseBodyOrEmpty();
  if (!body["enabled"].isNull()) {
    gPrefs.putBool("blynkEnabled", body["enabled"].as<bool>());
  }
  if (!body["token"].isNull()) {
    const String tok = body["token"].as<String>();
    if (tok.length() >= 8) {
      gPrefs.putString("blynkToken", tok);
    }
  }
  if (!body["templateId"].isNull()) {
    gPrefs.putString("blynkTemplateId", body["templateId"].as<String>());
  }
  if (!body["deviceName"].isNull()) {
    gPrefs.putString("blynkDeviceName", body["deviceName"].as<String>());
  }
  setupBlynkFromPrefs();
  if (gBlynkEnabled && gBlynkAuthBuf[0] != '\0') {
    disconnectBlynk();
  }
  JsonDocument doc;
  doc["ok"] = true;
  doc["enabled"] = gBlynkEnabled;
  doc["token"] = gBlynkToken;
  doc["templateId"] = gBlynkTemplateId;
  doc["deviceName"] = gBlynkDeviceName;
  doc["connected"] = gBlynkConnected;
  sendJson(200, doc);
}

void ConfigApiController::handleMqttGet() {
  JsonDocument doc;
  doc["ok"] = true;
  doc["enabled"] = gMqttConfig.enabled;
  doc["host"] = gMqttConfig.host;
  doc["port"] = gMqttConfig.port;
  doc["user"] = gMqttConfig.user;
  doc["pass"] = gMqttConfig.pass;
  doc["topicPrefix"] = gMqttConfig.topicPrefix;
  doc["discoveryPrefix"] = gMqttConfig.discoveryPrefix;
  doc["connected"] = isMqttConnected();
  doc["status"] = getMqttStatusText();
  sendJson(200, doc);
}

void ConfigApiController::handleMqttPut() {
  TagStoreLock lock;
  JsonDocument body = parseBodyOrEmpty();
  MqttConfig cfg = gMqttConfig;
  if (!body["enabled"].isNull()) {
    cfg.enabled = body["enabled"].as<bool>();
  }
  if (!body["host"].isNull()) {
    cfg.host = body["host"].as<String>();
    cfg.host.trim();
  }
  if (!body["port"].isNull()) {
    cfg.port = (uint16_t)constrain(body["port"].as<int>(), 1, 65535);
  }
  if (!body["user"].isNull()) {
    cfg.user = body["user"].as<String>();
    cfg.user.trim();
  }
  if (!body["pass"].isNull()) {
    cfg.pass = body["pass"].as<String>();
  }
  if (!body["topicPrefix"].isNull()) {
    String t = body["topicPrefix"].as<String>();
    t.trim();
    if (!t.isEmpty()) cfg.topicPrefix = t;
  }
  if (!body["discoveryPrefix"].isNull()) {
    String d = body["discoveryPrefix"].as<String>();
    d.trim();
    if (!d.isEmpty()) cfg.discoveryPrefix = d;
  }
  saveMqttConfig(cfg);

  JsonDocument doc;
  doc["ok"] = true;
  doc["enabled"] = gMqttConfig.enabled;
  doc["host"] = gMqttConfig.host;
  doc["port"] = gMqttConfig.port;
  doc["user"] = gMqttConfig.user;
  doc["pass"] = gMqttConfig.pass;
  doc["topicPrefix"] = gMqttConfig.topicPrefix;
  doc["discoveryPrefix"] = gMqttConfig.discoveryPrefix;
  doc["connected"] = isMqttConnected();
  doc["status"] = getMqttStatusText();
  sendJson(200, doc);
}

void ConfigApiController::handleEspNowDisable() {
  disableEspNow();
  JsonDocument doc;
  doc["ok"] = true;
  doc["espNowDisabled"] = gEspNowDisabled;
  sendJson(200, doc);
}

void ConfigApiController::handleEspNowEnable() {
  enableEspNow();
  JsonDocument doc;
  doc["ok"] = true;
  doc["espNowDisabled"] = gEspNowDisabled;
  sendJson(200, doc);
}
