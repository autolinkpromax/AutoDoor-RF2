#include "SystemApiController.h"
#include "app/config.h"
#include "app/globals.h"
#include "modules/door.h"
#include "modules/esp_now_runtime.h"
#include "modules/web_auth.h"
#include "modules/webhook_hooks.h"
#include "modules/outbound_webhook.h"
#include "modules/wifi_manager.h"
#include "tags/TagRegistry.h"
#include "diagnostics/DiagEventLogger.h"
#include "modules/maintenance.h"
#include "hal/MotorActuator.h"

#include <Update.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_task_wdt.h>
#include <WiFi.h>

void SystemApiController::registerRoutes() {
  _server.on("/api/health", HTTP_GET, [this]() {
    wrap([this]() { handleHealth(); }, true);
  });

  _server.on("/api/status", HTTP_GET, [this]() {
    wrap([this]() { handleStatus(); });
  });

  _server.on("/api/live", HTTP_GET, [this]() {
    wrap([this]() { handleLive(); });
  });

  _server.on("/api/login", HTTP_POST, [this]() {
    wrap([this]() { handleLogin(); }, true);
  });

  _server.on("/api/webauth", HTTP_GET, [this]() {
    wrap([this]() { handleWebAuthGet(); }, true);
  });

  _server.on("/api/webauth", HTTP_PUT, [this]() {
    wrap([this]() { handleWebAuthPut(); });
  });

  _server.on("/api/webauth", HTTP_POST, [this]() {
    wrap([this]() { handleWebAuthPut(); });
  });

  _server.on("/api/restart", HTTP_POST, [this]() {
    wrap([this]() { handleRestart(); });
  });

  _server.on("/api/safemode/enter", HTTP_POST, [this]() {
    wrap([this]() { handleEnterSafeMode(); });
  });

  _server.on("/api/safemode/exit", HTTP_POST, [this]() {
    wrap([this]() { handleExitSafeMode(); }, true);
  });

  _server.on(
      "/api/ota", HTTP_POST,
      [this]() { wrap([this]() { handleOtaFinish(); }); },
      [this]() { handleOtaUpload(); }
  );

  _server.on("/api/webhook", HTTP_GET, [this]() {
    wrap([this]() { handleWebhookGet(); });
  });

  _server.on("/api/webhook", HTTP_POST, [this]() {
    wrap([this]() { handleWebhookSave(); });
  });

  _server.on("/api/webhook/test", HTTP_POST, [this]() {
    wrap([this]() { handleWebhookTest(); });
  });

  _server.on("/api/maintenance/reset-service", HTTP_POST, [this]() {
    wrap([this]() { handleResetService(); });
  });

  _server.on("/api/maintenance/settings", HTTP_POST, [this]() {
    wrap([this]() { handleMaintenanceSettings(); });
  });

  _server.on("/api/system/factory-reset", HTTP_POST, [this]() {
    wrap([this]() { handleFactoryReset(); });
  });
}

void SystemApiController::handleHealth() {
  JsonDocument doc;
  doc["ok"] = true;
  doc["fwVersion"] = kFirmwareVersion;
  doc["buildDate"] = kFirmwareBuildDate;
  sendJson(200, doc);
}

void SystemApiController::handleStatus() {
  JsonDocument doc;
  {
    TagStoreLock lock;
    doc["ok"] = true;
    doc["fwVersion"] = kFirmwareVersion;
    doc["buildDate"] = kFirmwareBuildDate;
    doc["doorState"] = doorStateToString(gDoorState);
    doc["safeMode"] = gSafeMode;
    doc["tagCount"] = gTagRegistry.count();
    doc["apSsid"] = resolveApSsid();
    const String host = resolveHostname();
    doc["hostname"] = host;
    doc["localUrl"] = "http://" + host + ".local";
    doc["apEnabled"] = gApEnabled;
    doc["apStations"] = gApEnabled ? WiFi.softAPgetStationNum() : 0;
    doc["apIp"] = gApEnabled ? WiFi.softAPIP().toString() : "";
    doc["webAuthEnabled"] = !gWebPassword.isEmpty() && !gSafeMode;
    doc["staConnected"] = WiFi.status() == WL_CONNECTED;
    if (WiFi.status() == WL_CONNECTED) {
      doc["staIp"] = WiFi.localIP().toString();
      doc["staSsid"] = WiFi.SSID();
    }
    doc["espNowChannel"] = currentEspNowChannel();
    doc["espNowChannelConfigured"] = gEspNowChannel;
    doc["wifiChannel"] = WiFi.channel();
    doc["espNowDisabled"] = gEspNowDisabled;
    doc["relayPulseMs"] = gRelayPulseMs;
    doc["doorTransitionMs"] = gDoorTransitionMs;

    JsonObject maint = doc["maintenance"].to<JsonObject>();
    maint["totalCycles"] = gMaintenance.getTotalCycles();
    maint["serviceCycles"] = gMaintenance.getServiceCycles();
    maint["serviceThreshold"] = gMaintenance.getServiceThreshold();
    maint["serviceDue"] = gMaintenance.isServiceDue();
    maint["lastServiceDate"] = gMaintenance.getLastServiceDate();

    JsonObject ota = doc["ota"].to<JsonObject>();
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* next = esp_ota_get_next_update_partition(NULL);
    ota["runningPartition"] = running ? running->label : "unknown";
    ota["nextPartition"] = next ? next->label : "none";
    ota["otaValid"] = (next != NULL);
    ota["flashSize"] = ESP.getFlashChipSize();
    ota["freeSketchSpace"] = ESP.getFreeSketchSpace();
    ota["sketchSize"] = ESP.getSketchSize();
  }
  sendJson(200, doc);
}

void SystemApiController::handleLive() {
  JsonDocument doc;
  const uint32_t now = millis();
  doc["ok"] = true;
  doc["doorState"] = doorStateToString(gDoorState);
  doc["doorTransitionMs"] = gDoorTransitionMs;
  doc["learnActive"] = gLearn.active;
  if (gLearn.active) {
    doc["learnRemainingMs"] = (int32_t)((int32_t)gLearn.deadlineMs - (int32_t)now);
  } else {
    doc["learnRemainingMs"] = nullptr;
  }
  doc["learnCreatedTagId"] = gLearn.createdTagId;
  doc["learnLastMac"] = gLearn.lastMac;
  
  doc["magSensorEnabled"] = gMagSensorEnabled;
  doc["magSensorClosed"] = gMagSensorEnabled ? gMagSensorStableClosed : false;
  doc["closeRetryCount"] = gCloseRetryCount;
  doc["builtinSensorsEnabled"] = false;
  doc["builtinMagOpen"] = false;
  doc["builtinMagClose"] = false;
  doc["safetySensorEnabled"] = gSafetySensorEnabled;
  doc["safetySensorBlocked"] = gSafetySensorEnabled ? gSafetySensorBlocked : false;
  doc["safetyCloseDelayMs"] = gSafetyCloseDelayMs;
  doc["safetyReverseDelayMs"] = gSafetyReverseDelayMs;
  if (gSafetySensorEnabled && gSafetyCloseAtMs != 0) {
    doc["safetyCloseRemainingMs"] = (int32_t)((int32_t)gSafetyCloseAtMs - (int32_t)now);
  } else {
    doc["safetyCloseRemainingMs"] = nullptr;
  }

  bool any = false;
  size_t tagCount = 0;
  {
    TagStoreLock lock;
    tagCount = gTagRegistry.count();
    std::vector<const Tag*> liveTagList;
    liveTagList.reserve(10);

    // 1. Prioritize currently active tags
    for (const auto& t : gTagRegistry.tags()) {
      if (isTagActive(t, now)) {
        any = true;
        if (liveTagList.size() < 10) {
          liveTagList.push_back(&t);
        }
      }
    }

    // 2. Fill remaining slots up to 10 with recently seen tags (< 2 minutes)
    if (liveTagList.size() < 10) {
      for (const auto& t : gTagRegistry.tags()) {
        if (!isTagActive(t, now) && t.lastSeenMs != 0 && (uint32_t)(now - t.lastSeenMs) < 120000) {
          bool alreadyIn = false;
          for (const auto* p : liveTagList) {
            if (p == &t) { alreadyIn = true; break; }
          }
          if (!alreadyIn) {
            liveTagList.push_back(&t);
            if (liveTagList.size() >= 10) break;
          }
        }
      }
    }

    // 3. Ensure active trigger tag is always present while opening or open
    if (!gActiveTriggerTagId.isEmpty() && (gDoorState == DoorState::Opening || gDoorState == DoorState::Open)) {
      Tag* trigTag = findTagById(gActiveTriggerTagId);
      if (trigTag) {
        bool found = false;
        for (const auto* p : liveTagList) {
          if (p == trigTag) { found = true; break; }
        }
        if (!found) {
          liveTagList.insert(liveTagList.begin(), trigTag);
        }
      }
    }

    JsonArray tags = doc["tags"].to<JsonArray>();
    for (const auto* tagPtr : liveTagList) {
      const Tag& t = *tagPtr;
      const bool isRf = isRfTag(t);
      const uint32_t effectiveHoldMs = isRf ? kDefaultRfTagHoldMs : (t.tagHoldMs > 0 ? t.tagHoldMs : kDefaultTagHoldMs);
      const bool active = isTagActive(t, now);
      const bool isTrigger = (!gActiveTriggerTagId.isEmpty() && t.id == gActiveTriggerTagId && (gDoorState == DoorState::Opening || gDoorState == DoorState::Open));
      JsonObject o = tags.add<JsonObject>();
      o["id"] = t.id;
      o["name"] = t.name;
      o["mac"] = t.mac;
      o["isRf"] = isRf;
      o["active"] = active;
      o["isTriggerTag"] = isTrigger;
      o["tagHoldMs"] = effectiveHoldMs;
      o["tagCloseDelayMs"] = t.tagCloseDelayMs;
      o["tagRssiMin"] = (int)t.tagRssiMin;
      o["tagSafetyCloseDelayMs"] = t.tagSafetyCloseDelayMs;
      o["tagOpenDurationMs"] = t.tagOpenDurationMs;
      o["rfRole"] = (int)t.rfRole;
      o["lastRssi"] = (int)t.lastRssi;
      o["rssiZone"] = tagRssiZone(t, now, active);
      if (t.lastSeenMs != 0) {
        o["lastSeenAgeMs"] = (uint32_t)(now - t.lastSeenMs);
        o["holdRemainingMs"] =
            active ? (int32_t)((int32_t)effectiveHoldMs - (int32_t)(now - t.lastSeenMs)) : 0;
      }
    }
  } // TagStoreLock released

  doc["anyTagActive"] = any;
  doc["tagAutoEnabled"] = gTagAutoEnabled;
  doc["safeMode"] = gSafeMode;
  if (gActiveTriggerTagId.isEmpty()) {
    doc["activeTriggerTagId"] = nullptr;
  } else {
    doc["activeTriggerTagId"] = gActiveTriggerTagId;
  }
  if (gPendingCloseAtMs != 0) {
    doc["pendingCloseMs"] = (int32_t)((int32_t)gPendingCloseAtMs - (int32_t)now);
  } else {
    doc["pendingCloseMs"] = nullptr;
  }
  if (gSafetyCloseAtMs != 0) {
    doc["safetyCloseRemainingMs"] = (int32_t)((int32_t)gSafetyCloseAtMs - (int32_t)now);
  } else {
    doc["safetyCloseRemainingMs"] = nullptr;
  }
  doc["tagCount"] = tagCount;
  doc["tagTotalCount"] = tagCount;

  // Recent events kept in scope with deep-copied strings for ArduinoJson (25 latest for dashboard)
  std::vector<DoorEventLogEntry> recent = getRecentDoorEvents(25);
  JsonArray events = doc["recentEvents"].to<JsonArray>();
  for (const auto& ev : recent) {
    JsonObject obj = events.add<JsonObject>();
    obj["time"] = String(ev.timeStr);
    obj["type"] = String(ev.code);
    obj["level"] = String(ev.level);
    obj["msg"] = String(ev.message);
    if (ev.reasonStr[0]) obj["reason"] = String(ev.reasonStr);
    if (ev.durationMs > 0) obj["durationMs"] = ev.durationMs;
    if (ev.metricVal != 0) obj["metric"] = ev.metricVal;
    if (ev.entityId[0]) obj["entity"] = String(ev.entityId);
    obj["sensorBits"] = ev.sensorBits;
    obj["flags"] = ev.flags;
  }

  JsonObject maint = doc["maintenance"].to<JsonObject>();
  maint["totalCycles"] = gMaintenance.getTotalCycles();
  maint["serviceCycles"] = gMaintenance.getServiceCycles();
  maint["serviceThreshold"] = gMaintenance.getServiceThreshold();
  maint["serviceDue"] = gMaintenance.isServiceDue();
  maint["lastServiceDate"] = gMaintenance.getLastServiceDate();

  sendJson(200, doc);
}

void SystemApiController::handleLogin() {
  if (gSafeMode || gWebPassword.isEmpty()) {
    JsonDocument doc;
    doc["ok"] = true;
    doc["skip"] = true;
    sendJson(200, doc);
    return;
  }
  JsonDocument body = parseBodyOrEmpty();
  const String pass = body["password"].as<String>();
  if (pass != gWebPassword) {
    JsonDocument err;
    err["ok"] = false;
    err["error"] = "bad_password";
    sendJson(401, err);
    return;
  }
  const String sessionToken = createWebSession();
  _server.sendHeader("Set-Cookie",
                     "al_sess=" + sessionToken + "; Path=/; Max-Age=86400; HttpOnly; SameSite=Lax");
  JsonDocument doc;
  doc["ok"] = true;
  sendJson(200, doc);
}

void SystemApiController::handleWebAuthGet() {
  JsonDocument doc;
  doc["ok"] = true;
  doc["hasPassword"] = !gWebPassword.isEmpty() && !gSafeMode;
  doc["safeMode"] = gSafeMode;
  doc["authenticated"] = isWebAuthed();
  sendJson(200, doc);
}

void SystemApiController::handleWebAuthPut() {
  TagStoreLock lock;
  if (gSafeMode) {
    JsonDocument err;
    err["ok"] = false;
    err["error"] = "safe_mode";
    sendJson(403, err);
    return;
  }
  JsonDocument body = parseBodyOrEmpty();
  if (!body["password"].isNull()) {
    gWebPassword = body["password"].as<String>();
    gPrefs.putString("webPass", gWebPassword);
    revokeAllWebSessions();
    AD_LOG(Serial.printf("[auth] web password %s\n", gWebPassword.isEmpty() ? "cleared" : "updated"));
  }
  JsonDocument doc;
  doc["ok"] = true;
  doc["hasPassword"] = !gWebPassword.isEmpty();
  doc["safeMode"] = gSafeMode;
  doc["authenticated"] = isWebAuthed();
  sendJson(200, doc);
}

void SystemApiController::handleRestart() {
  gPrefs.remove("forceSafeMode");
  JsonDocument doc;
  doc["ok"] = true;
  doc["message"] = "กำลังรีสตาร์ท...";
  sendJson(200, doc);
  delay(200);
  ESP.restart();
}

void SystemApiController::handleEnterSafeMode() {
  TagStoreLock lock;
  gPrefs.putBool("forceSafeMode", true);
  JsonDocument doc;
  doc["ok"] = true;
  sendJson(200, doc);
  delay(200);
  ESP.restart();
}

void SystemApiController::handleExitSafeMode() {
  TagStoreLock lock;
  gPrefs.remove("forceSafeMode");
  gSafeMode = false;
  JsonDocument doc;
  doc["ok"] = true;
  doc["message"] = "กำลังออกจาก Safe Mode และรีสตาร์ท...";
  sendJson(200, doc);
  delay(200);
  ESP.restart();
}

void SystemApiController::handleOtaFinish() {
  if (!ensureWebAuth()) return;
  const bool success = !Update.hasError();
  JsonDocument doc;
  doc["ok"] = success;
  if (success) {
    doc["message"] = "อัปเดตเฟิร์มแวร์สำเร็จ กำลังรีสตาร์ท...";
    sendJson(200, doc);
    delay(500);
    ESP.restart();
  } else {
    doc["error"] = "OTA ล้มเหลว (รหัสข้อผิดพลาด: " + String(Update.getError()) + ")";
    sendJson(400, doc);
  }
}

void SystemApiController::handleOtaUpload() {
  if (!ensureWebAuth()) return;
  HTTPUpload& upload = _server.upload();
  esp_task_wdt_reset();
  if (upload.status == UPLOAD_FILE_START) {
    WiFi.setSleep(false);
    AD_LOG(Serial.printf("[ota] Start update: %s\n", upload.filename.c_str()));
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      AD_LOG(Update.printError(Serial));
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      AD_LOG(Update.printError(Serial));
    }
    esp_task_wdt_reset();
    vTaskDelay(1);
  } else if (upload.status == UPLOAD_FILE_END) {
    esp_task_wdt_reset();
    if (Update.end(true)) {
      AD_LOG(Serial.printf("[ota] Update success: %u bytes\n", upload.totalSize));
    } else {
      AD_LOG(Update.printError(Serial));
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    esp_task_wdt_reset();
    Update.end();
    AD_LOG(Serial.println("[ota] Update aborted"));
  }
}

void SystemApiController::handleWebhookGet() {
  ensureHookKey();
  const String host = hookHostIp();
  const String base = "http://" + host;
  const String blynkHost = gPrefs.getString("blynkHost", "sgp1.blynk.cloud");
  const String blynkToken = gPrefs.getString("blynkToken", "");
  const String blynkBase = "https://" + blynkHost + "/external/api/update?token=" + blynkToken;

  JsonDocument doc;
  doc["ok"] = true;
  doc["key"] = gHookKey;
  doc["host"] = host;
  doc["blynkHost"] = blynkHost;
  doc["blynkToken"] = blynkToken;
  JsonObject hooks = doc["hooks"].to<JsonObject>();
  hooks["open"] = base + "/hook/door/open?key=" + gHookKey;
  hooks["tagOpen"] = base + "/hook/door/tag_open?key=" + gHookKey;
  hooks["stop"] = base + "/hook/door/stop?key=" + gHookKey;
  hooks["close"] = base + "/hook/door/close?key=" + gHookKey;
  hooks["tagAutoToggle"] = base + "/hook/tag-auto/toggle?key=" + gHookKey;
  if (blynkToken.length() >= 8) {
    JsonObject blynk = doc["blynk"].to<JsonObject>();
    blynk["open"] = blynkBase + "&V0=1";
    blynk["stop"] = blynkBase + "&V1=1";
    blynk["close"] = blynkBase + "&V2=1";
    blynk["tagAutoToggle"] = blynkBase + "&V3=1";
  }

  JsonObject out = doc["outbound"].to<JsonObject>();
  out["enabled"] = gWebhookConfig.enabled;
  out["url"] = gWebhookConfig.url;
  out["lineEnabled"] = gWebhookConfig.lineEnabled;
  out["lineToken"] = gWebhookConfig.lineToken;
  out["lineTargetId"] = gWebhookConfig.lineTargetId;
  out["eventSafety"] = gWebhookConfig.eventSafety;
  out["eventOpenDuration"] = gWebhookConfig.eventOpenDuration;
  out["openDurationMinutes"] = gWebhookConfig.openDurationMinutes;
  out["lastStatus"] = getLastWebhookStatus();
  out["lastTimeMs"] = getLastWebhookTimeMs();

  sendJson(200, doc);
}

void SystemApiController::handleWebhookSave() {
  const JsonDocument body = parseBodyOrEmpty();
  OutboundWebhookConfig cfg = gWebhookConfig;
  if (!body["enabled"].isNull()) {
    cfg.enabled = body["enabled"].as<bool>();
  }
  if (!body["url"].isNull()) {
    cfg.url = body["url"].as<String>();
    cfg.url.trim();
  }
  if (!body["lineEnabled"].isNull()) {
    cfg.lineEnabled = body["lineEnabled"].as<bool>();
  }
  if (!body["lineToken"].isNull()) {
    cfg.lineToken = body["lineToken"].as<String>();
    cfg.lineToken.trim();
  }
  if (!body["lineTargetId"].isNull()) {
    cfg.lineTargetId = body["lineTargetId"].as<String>();
    cfg.lineTargetId.trim();
  }
  if (!body["eventSafety"].isNull()) {
    cfg.eventSafety = body["eventSafety"].as<bool>();
  }
  if (!body["eventOpenDuration"].isNull()) {
    cfg.eventOpenDuration = body["eventOpenDuration"].as<bool>();
  }
  if (!body["openDurationMinutes"].isNull()) {
    cfg.openDurationMinutes = (uint16_t)constrain(body["openDurationMinutes"].as<int>(), 1, 60);
  }
  saveOutboundWebhookConfig(cfg);

  JsonDocument doc;
  doc["ok"] = true;
  doc["enabled"] = cfg.enabled;
  doc["url"] = cfg.url;
  doc["lineEnabled"] = cfg.lineEnabled;
  doc["lineToken"] = cfg.lineToken;
  doc["lineTargetId"] = cfg.lineTargetId;
  doc["eventSafety"] = cfg.eventSafety;
  doc["eventOpenDuration"] = cfg.eventOpenDuration;
  doc["openDurationMinutes"] = cfg.openDurationMinutes;
  sendJson(200, doc);
}

void SystemApiController::handleWebhookTest() {
  const JsonDocument body = parseBodyOrEmpty();
  String customUrl = "";
  String tagName = "";
  if (!body["url"].isNull()) {
    customUrl = body["url"].as<String>();
    customUrl.trim();
  }
  if (!body["tagName"].isNull()) {
    tagName = body["tagName"].as<String>();
    tagName.trim();
  }

  const bool enqueued = triggerOutboundWebhookTest(
    customUrl.isEmpty() ? nullptr : customUrl.c_str(),
    tagName.isEmpty() ? nullptr : tagName.c_str()
  );

  String resolvedTarget = resolveWebhookUrl(customUrl.isEmpty() ? gWebhookConfig.url : customUrl);

  JsonDocument doc;
  doc["ok"] = enqueued;
  doc["target"] = resolvedTarget;
  doc["msg"] = enqueued ? "Enqueued test webhook" : "Queue full or not ready";
  sendJson(enqueued ? 200 : 409, doc);
}

void SystemApiController::handleResetService() {
  JsonDocument body = parseBodyOrEmpty();
  String dateStr = body["date"].as<String>();
  gMaintenance.resetService(dateStr);
  logDoorEvent(DoorEventCode::None, DoorEventLevel::Info, 0, "SERVICE_RESET");

  JsonDocument res;
  res["ok"] = true;
  res["serviceCycles"] = 0;
  res["lastServiceDate"] = gMaintenance.getLastServiceDate();
  res["serviceDue"] = false;
  sendJson(200, res);
}

void SystemApiController::handleMaintenanceSettings() {
  JsonDocument body = parseBodyOrEmpty();
  if (!body["threshold"].isNull()) {
    gMaintenance.setThreshold(body["threshold"].as<uint32_t>());
  }
  JsonDocument res;
  res["ok"] = true;
  res["serviceThreshold"] = gMaintenance.getServiceThreshold();
  sendJson(200, res);
}

void SystemApiController::handleFactoryReset() {
  Serial.printf("\n==================================================\n");
  Serial.printf("[reset] >>> WEB FACTORY RESET REQUESTED <<<\n");
  Serial.printf("[reset] Resetting all settings to factory default. TAGS PRESERVED.\n");
  Serial.printf("==================================================\n\n");

  gMotor.triggerChirp(4);
  logDoorEvent(DoorEventCode::ManualStop, DoorEventLevel::Warn, 0, "FACTORY_RESET");
  logDiagEvent(DoorEventCode::ManualStop, DoorEventLevel::Warn, DiagReasonCode::None, 0, 0, "FACTORY_RESET");

  // 1. Wipe Wi-Fi & Web Authentication
  clearHomeWifiPrefs();
  clearWebAuthPassword();

  // 2. Wipe AppConfig door and sensor parameters
  const char* keysToClear[] = {
    "simMode", "motorReverse", "deviceName", "espNowCh", "tagAutoEnabled", "tagBuzzerEn",
    "magSensEnabled", "builtinSensEn", "builtinMagSwap", "safeSensEnabled", "safeSensType",
    "safeCloseDelay", "safeRevDelay", "remPassClose", "remPassDelay", "doorTransMs",
    "magSensType", "autoRetryEn", "maxCloseRetry", "retryCloseDelay", "dblClose",
    "rebootEn", "rebootHour", "rebootMin", "safe_mode", "apUserEnabled", "apIdleMin",
    "blynkToken", "blynkHost", "blynkEn", "blynkDeviceName", "blynkTemplateId",
    "mqttHost", "mqttPort", "mqttUser", "mqttPass", "mqttTopicPrefix", "mqttDiscPrefix", "mqttEnabled",
    "whOutEn", "whOutUrl", "whLineEn", "whLineTok", "whLineTgt", "whEvSafety", "whEvOpenDur", "whOpenDurMin",
    "servCycles", "servThreshold", "servLastDate", "schedCount", "hostname"
  };
  for (const char* k : keysToClear) {
    gPrefs.remove(k);
  }
  for (int i = 0; i < 32; ++i) {
    char sKey[16];
    snprintf(sKey, sizeof(sKey), "sched_%d", i);
    gPrefs.remove(sKey);
  }

  // NOTE: Tag entries ("tag_ids", "t_*") and lifetime odometer ("maintTotal") are 100% PRESERVED!

  JsonDocument res;
  res["ok"] = true;
  res["msg"] = "Factory reset initiated. Device will restart into Setup Wizard.";
  sendJson(200, res);

  // Restart device safely after short delay
  _server.client().flush();
  delay(500);
  ESP.restart();
}

