#include <Arduino.h>
#include <WiFi.h>

#include "app/config.h"
#include "app/AppConfig.h"
#include "app/globals.h"
#include "core/DoorController.h"
#include "modules/esp_now_runtime.h"
#include "modules/http_api.h"
#include "modules/local_panel.h"
#include "hal/MotorActuator.h"
#include "modules/status_led.h"
#include "tags/TagRegistry.h"
#include "modules/blynk_service.h"
#include "modules/ntp.h"
#include "modules/schedule.h"
#include "modules/daily_restart.h"
#include "modules/webhook_hooks.h"
#include "modules/web_auth.h"
#include "modules/wifi_manager.h"
#include "modules/rf433_runtime.h"
#include "modules/event_archive.h"
#include "modules/outbound_webhook.h"
#include "modules/mqtt_service.h"
#include "modules/maintenance.h"
#include <esp_task_wdt.h>


void setup() {
  Serial.begin(115200);
  delay(800);  // Extended PSU settle time: allows motor board AUX supply to reach stable voltage before WiFi radio turns on
  gBootMs = millis();
  Serial.println();
  Serial.println("=== AutoDoor ESP-NOW ===");

  esp_reset_reason_t resetReason = esp_reset_reason();
  if (resetReason == ESP_RST_TASK_WDT || resetReason == ESP_RST_WDT || resetReason == ESP_RST_INT_WDT) {
    Serial.printf("[wdt] RECOVERY: Board rebooted by Watchdog Timer (reason=%d)\n", (int)resetReason);
  }

  // Task Watchdog: 8-second timeout to protect against any deadlocks or infinite loops
  esp_task_wdt_config_t twdtConfig = {
    .timeout_ms = 8000,
    .idle_core_mask = (1 << 0),
    .trigger_panic = true,
  };
  esp_err_t twdtErr = esp_task_wdt_init(&twdtConfig);
  if (twdtErr == ESP_ERR_INVALID_STATE) {
    esp_task_wdt_reconfigure(&twdtConfig);
  }
  esp_task_wdt_add(NULL);
  Serial.println("[wdt] Task Watchdog Timer (8000ms) active for loopTask");

  gTagRegistry.init();
  gPrefs.begin("autodoor", false);

  // Load tags early to check if we have any tags in NVS
  gTagRegistry.loadFromPrefs();

  bool forcedSafeMode = detectSafeModeBoot();
  if (forcedSafeMode) {
    gSafeMode = true;
    Serial.println("[safe] SAFE MODE enabled via boot button hold (Wi-Fi credentials preserved)");
  } else if (gTagRegistry.empty()) {
    gSafeMode = true;
    Serial.println("[safe] SAFE MODE enabled automatically (no tags in system)");
  } else {
    gSafeMode = false;
  }

  loadWebAuthFromPrefs();
  gAppConfig.loadFromPrefs(gPrefs);

  autoDetectEspNowChannelOnBoot();
  loadSchedulesFromPrefs();
  setupBlynkFromPrefs();

  gDoorController.init();
  gDoorController.initCommandQueue();
  gMaintenance.begin(gPrefs);
  gDoorController.addListener(&gMaintenance);

  ensureHookKey();
  gMotor.init();
  initLocalPanel();
  initStatusLed();
  pinMode(kBootBtnPin, INPUT_PULLUP);
  Serial.printf("[boot-btn] Monitoring BOOT on GPIO %u (raw=%d)\n",
                kBootBtnPin, digitalRead(kBootBtnPin));
  initRf433();
  initEventArchive();
  initOutboundWebhook();
  gSystemBootReady = true;
  Serial.println("[boot] Core ready: door ground truth confirmed, RF433 receiver active");

  // ─── PHASE 1: WiFi Radio (SoftAP only, STA deferred) ───────────────────────
  // SoftAP เปิดก่อน TX Power 17dBm — ยังไม่ WiFi.begin() STA ทันที
  // STA connect จะเกิดใน stepWiFiReconnect() loop หลัง ~5 วินาที เมื่อ PSU เสถียรแล้ว
  Serial.println("[boot] Phase 1: WiFi SoftAP init (STA connect deferred)...");
  setupWifiApStaPhased();

  // ─── PHASE 2: ESP-NOW Init (ห่าง 400ms หลัง SoftAP) ────────────────────────
  // รอให้ WiFi radio settle ก่อน init ESP-NOW เพื่อป้องกัน peak current ซ้อนกัน
  delay(400);
  if (!gSafeMode) {
    Serial.println("[boot] Phase 2: ESP-NOW init...");
    initEspNow();
  } else {
    Serial.println("[safe] ESP-NOW suspended on boot (Safe Mode active)");
  }

  // ─── PHASE 3: WebServer + Cloud Services Config (ห่าง 200ms) ──────────────
  // WebServer เบา, MQTT/Blynk จะ connect เองใน loop() หลัง WiFi STA connected
  delay(200);
  setupWebServer();
  initMqttService();
  Serial.printf("[boot] wifi ch=%d espnow ch=%u\n", WiFi.channel(), (unsigned)currentEspNowChannel());

  Serial.printf("[boot] AP http://%s/\n", WiFi.softAPIP().toString().c_str());
  if (gSafeMode) {
    Serial.println("[safe] door auto logic disabled");
    triggerSafeModeAlertSound();
    gMotor.triggerChirp(2);
  }
  Serial.println("[boot] Staged boot complete: SoftAP+ESP-NOW ready. STA+Cloud pending in loop().");
}

struct ButtonChannel {
  uint8_t pin;
  const char* name;
  uint8_t lastRaw;
  uint8_t stableState;
  uint32_t lastDebounceMs;
  uint32_t pressStartMs;
  bool stopFired;
  bool safeFired;
  bool resetFired;
};

static ButtonChannel gBtnChannels[] = {
  { kBootBtnPin, "BOOT (GPIO 9)", HIGH, HIGH, 0, 0, false, false, false }
};


void stepBootButton() {
  const uint32_t now = millis();
  if (now < 1000) return;  // Settle time after boot

  // Periodic heartbeat log every 10 seconds (debug only — silenced in release build via AD_LOG)
  static uint32_t lastHeartbeatMs = 0;
  if ((uint32_t)(now - lastHeartbeatMs) >= 10000) {
    lastHeartbeatMs = now;
    AD_LOG(Serial.printf("[boot-btn] Heartbeat: GPIO 9=%d (doorState=%s)\n",
                  digitalRead(kBootBtnPin),
                  DoorController::stateToString(gDoorState)));
  }

  for (auto& btn : gBtnChannels) {
    const uint8_t raw = digitalRead(btn.pin);

    // Immediate log when raw pin state changes (edges)
    if (raw != btn.lastRaw) {
      btn.lastRaw = raw;
      btn.lastDebounceMs = now;
      Serial.printf("[boot-btn] %s raw change -> %d (%s)\n",
                    btn.name, raw, (raw == LOW) ? "LOW / Pressed" : "HIGH / Released");
    }

    // Stable debounce check (25ms stable reading)
    if ((uint32_t)(now - btn.lastDebounceMs) >= 25 && btn.stableState != raw) {
      btn.stableState = raw;
      if (raw == LOW) {
        // Confirmed Press Event
        btn.pressStartMs = now;
        btn.stopFired = true;
        btn.safeFired = false;
        btn.resetFired = false;

        Serial.printf("\n==================================================\n");
        Serial.printf("[boot-btn] >>> %s PRESSED -> STOP COMMAND SENT <<<\n", btn.name);
        Serial.printf("==================================================\n");

        // 1. Dispatch STOP command to door controller
        gDoorController.requestSlot("stop", true);

        // 2. Record event in Diag Event Logger and Door Event (visible in WebUI event log)
        logDoorEvent(DoorEventCode::ManualStop, DoorEventLevel::Info, btn.pin, "BOOT_BTN");
        logDiagEvent(DoorEventCode::ManualStop, DoorEventLevel::Info, DiagReasonCode::None, 0, btn.pin, "BOOT_BTN");
      } else {
        // Confirmed Release Event
        Serial.printf("[boot-btn] %s RELEASED\n", btn.name);
        btn.pressStartMs = 0;
        btn.stopFired = false;
        btn.safeFired = false;
        btn.resetFired = false;
      }
    }

    // Long press check (5 seconds hold -> fire STOP and enter Safe Mode safely without wiping Wi-Fi)
    if (btn.stableState == LOW && btn.stopFired && !btn.safeFired) {
      if ((uint32_t)(now - btn.pressStartMs) >= 5000) {
        btn.safeFired = true;
        // 1. Explicitly send STOP command before entering Safe Mode
        gDoorController.requestSlot("stop", true);

        // 2. Activate Safe Mode and start SoftAP (preserving Wi-Fi settings)
        if (!gSafeMode) {
          gSafeMode = true;
          suspendEspNow();
          startSoftAp();
          triggerSafeModeAlertSound();
          gMotor.triggerChirp(2);

          Serial.printf("\n==================================================\n");
          Serial.printf("[safe] >>> %s HELD 5s -> STOP FIRED & SAFE MODE ACTIVATED <<<\n", btn.name);
          Serial.printf("[safe] SoftAP active. Wi-Fi credentials safely preserved. ESP-NOW suspended.\n");
          Serial.printf("==================================================\n\n");
          logDoorEvent(DoorEventCode::ManualStop, DoorEventLevel::Warn, btn.pin, "SAFE_MODE");
          logDiagEvent(DoorEventCode::ManualStop, DoorEventLevel::Warn, DiagReasonCode::None, 0, btn.pin, "SAFE_MODE");
        }
      }
    }

    // Very Long press check (10 seconds hold -> Factory Reset Wi-Fi and Web Password)
    if (btn.stableState == LOW && btn.safeFired && !btn.resetFired) {
      if ((uint32_t)(now - btn.pressStartMs) >= 10000) {
        btn.resetFired = true;
        Serial.printf("\n==================================================\n");
        Serial.printf("[reset] >>> %s HELD 10s -> FACTORY RESET (WIFI & AUTH WIPED) <<<\n", btn.name);
        Serial.printf("==================================================\n\n");
        gMotor.triggerChirp(4);
        clearHomeWifiPrefs();
        clearWebAuthPassword();
        logDoorEvent(DoorEventCode::ManualStop, DoorEventLevel::Warn, btn.pin, "FACTORY_RESET");
        logDiagEvent(DoorEventCode::ManualStop, DoorEventLevel::Warn, DiagReasonCode::None, 0, btn.pin, "FACTORY_RESET");
        delay(600);
        ESP.restart();
      }
    }
  }
}


void loop() {
  esp_task_wdt_reset();
  stepEspNowRxProcessing();
  stepBootButton();
  gServer.handleClient();
  stepDnsServer();
  gMotor.step();
  stepTagLearn();

  stepEspNowChannelSync();
  checkEspNowReceiverHealth();
  ensureEspNowChannel();
  stepReceiverAnnounce();
  gDoorController.step();
  stepWiFiReconnect();
  stepApIdle();
  stepNtpAuto();
  stepDailyRestart();
  stepSchedule();
  stepEventArchive();
  stepBlynk();
  gTagRegistry.flushIfIdle();
  stepLocalPanel();
  stepStatusLed();
  // Drain any residual SYN480R available flag once per loop (rf433Task handles actual processing)
  stepRf433();
  vTaskDelay(1);
}
