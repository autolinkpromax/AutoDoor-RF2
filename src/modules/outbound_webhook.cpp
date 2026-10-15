#include "outbound_webhook.h"

#include "app/globals.h"
#include "core/DoorController.h"
#include "core/IDoorListener.h"
#include "hal/MotorActuator.h"
#include "modules/wifi_manager.h"
#include "tags/TagRegistry.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <time.h>

OutboundWebhookConfig gWebhookConfig;

namespace {

struct WebhookItem {
  char event[16];
  char state[16];
  char message[256];
  char tagId[16];
  char tagName[64];
  char targetUrl[128];
  uint32_t timestamp;
  bool isTest;
};

QueueHandle_t sQueue = nullptr;
TaskHandle_t sTaskHandle = nullptr;
int sLastStatus = 0;
uint32_t sLastTimeMs = 0;
static uint32_t sDoorOpenStartMs = 0;
static uint32_t sLastOpenWarnSentMin = 0;
static String sActiveDoorTagId = "";

void formatDiscordPayload(const WebhookItem& item, String& out) {
  JsonDocument doc;
  String emoji = "🚪";
  if (strcmp(item.event, "tag_open") == 0 || strcmp(item.event, "open") == 0) emoji = "🟢";
  else if (strcmp(item.event, "tag_closing") == 0 || strcmp(item.event, "closing") == 0) emoji = "🟡";
  else if (strcmp(item.event, "tag_close") == 0 || strcmp(item.event, "close") == 0) emoji = "🔒";
  else if (strcmp(item.event, "stop") == 0) emoji = "🛑";
  else if (strcmp(item.event, "safety") == 0) emoji = "⚠️";
  else if (strcmp(item.event, "open_warn") == 0) emoji = "⏳";
  else if (strcmp(item.event, "test") == 0) emoji = "🧪";
  else if (strcmp(item.event, "tag") == 0) emoji = "🏷️";

  doc["content"] = emoji + " **[AutoDoor]** " + item.message;
  serializeJson(doc, out);
}

// ntfy.sh expects plain-text body (or text/plain Content-Type) to display message directly
void formatNtfyPayload(const WebhookItem& item, String& out) {
  out = item.message;
}

bool isNtfyUrl(const String& url) {
  return url.indexOf("ntfy.sh/") >= 0 || url.indexOf("/ntfy/") >= 0 || url.indexOf("ntfy.") >= 0 || url.startsWith("https://ntfy/") || url.startsWith("http://ntfy/");
}

bool isLineTarget(const String& str) {
  if (str.startsWith("line:")) return true;
  if (str.length() >= 30 && (str.startsWith("U") || str.startsWith("C") || str.startsWith("R"))) {
    return true;
  }
  return false;
}

void formatLinePayload(const WebhookItem& item, const String& targetId, String& out) {
  JsonDocument doc;
  doc["to"] = targetId;
  JsonArray msgs = doc["messages"].to<JsonArray>();
  JsonObject msg = msgs.add<JsonObject>();
  msg["type"] = "text";

  String emoji = "🚪";
  if (strcmp(item.event, "tag_open") == 0 || strcmp(item.event, "open") == 0) emoji = "🟢";
  else if (strcmp(item.event, "tag_closing") == 0 || strcmp(item.event, "closing") == 0) emoji = "🟡";
  else if (strcmp(item.event, "tag_close") == 0 || strcmp(item.event, "close") == 0) emoji = "🔒";
  else if (strcmp(item.event, "stop") == 0) emoji = "🛑";
  else if (strcmp(item.event, "safety") == 0) emoji = "⚠️";
  else if (strcmp(item.event, "open_warn") == 0) emoji = "⏳";
  else if (strcmp(item.event, "test") == 0) emoji = "🧪";

  msg["text"] = emoji + " [AutoDoor] " + String(item.message);
  serializeJson(doc, out);
}

void formatGenericPayload(const WebhookItem& item, String& out) {
  JsonDocument doc;
  doc["event"] = item.event;
  doc["state"] = item.state;
  doc["message"] = item.message;
  doc["device"] = "AutoDoor-RF2";
  doc["timestamp"] = item.timestamp;
  if (item.tagId[0] != '\0') {
    doc["tagId"] = item.tagId;
    doc["tagName"] = item.tagName;
  }
  doc["isTest"] = item.isTest;
  serializeJson(doc, out);
}

void outboundWebhookTask(void* param) {
  WebhookItem item;
  while (true) {
    if (xQueueReceive(sQueue, &item, pdMS_TO_TICKS(1000)) == pdTRUE) {
      const String url = (item.targetUrl[0] != '\0') ? String(item.targetUrl) : gWebhookConfig.url;
      if (!item.isTest && item.targetUrl[0] == '\0' && !gWebhookConfig.enabled) {
        continue;
      }
      if (url.length() < 8) {
        continue;
      }

      if (WiFi.status() != WL_CONNECTED) {
        sLastStatus = -1;
        sLastTimeMs = millis();
        continue;
      }

      // Fast rate limit: ensure small gap (200ms) between consecutive HTTP calls so lwIP sockets recycle cleanly
      static uint32_t sLastDispatchMs = 0;
      uint32_t now = millis();
      if (sLastDispatchMs != 0 && (now - sLastDispatchMs < 200)) {
        vTaskDelay(pdMS_TO_TICKS(200 - (now - sLastDispatchMs)));
      }
      sLastDispatchMs = millis();

      // 1. Check if item target is LINE or if LINE is enabled
      const bool targetIsLine = isLineTarget(item.targetUrl);
      String lineTarget = "";
      if (targetIsLine) {
        lineTarget = String(item.targetUrl);
        if (lineTarget.startsWith("line:")) lineTarget = lineTarget.substring(5);
      } else if (gWebhookConfig.lineEnabled && !gWebhookConfig.lineTargetId.isEmpty()) {
        lineTarget = gWebhookConfig.lineTargetId;
      }

      // 2. Dispatch to LINE Messaging API if applicable
      if (!lineTarget.isEmpty() && gWebhookConfig.lineEnabled && gWebhookConfig.lineToken.length() >= 20) {
        String linePayload;
        formatLinePayload(item, lineTarget, linePayload);

        WiFiClientSecure client;
        client.setInsecure();
        client.setTimeout(2500);

        HTTPClient http;
        http.begin(client, "https://api.line.me/v2/bot/message/push");
        http.setTimeout(2500);
        http.addHeader("Content-Type", "application/json; charset=utf-8");
        http.addHeader("Authorization", "Bearer " + gWebhookConfig.lineToken);
        http.addHeader("User-Agent", "AutoDoor-RF2");
        int lineHttpCode = http.POST(linePayload);
        http.end();
        client.stop();

        sLastStatus = lineHttpCode;
        sLastTimeMs = millis();
        AD_LOG(Serial.printf("[wh_out] LINE push to %s -> http=%d\n", lineTarget.c_str(), lineHttpCode));
      }

      // 3. Dispatch to Webhook URL (ntfy / Discord / Generic) if applicable (and not exclusively a LINE target ID)
      if (!targetIsLine) {
        const String url = (item.targetUrl[0] != '\0') ? String(item.targetUrl) : gWebhookConfig.url;
        if ((item.isTest || item.targetUrl[0] != '\0' || gWebhookConfig.enabled) && url.length() >= 8) {
          String payload;
          const bool isDiscord = (url.indexOf("discord.com/api/webhooks") >= 0);
          const bool isNtfy    = isNtfyUrl(url);
          String contentType;

          if (isDiscord) {
            formatDiscordPayload(item, payload);
            contentType = "application/json; charset=utf-8";
          } else if (isNtfy) {
            formatNtfyPayload(item, payload);
            contentType = "text/plain; charset=utf-8";
          } else {
            formatGenericPayload(item, payload);
            contentType = "application/json; charset=utf-8";
          }

          int httpCode = -1;
          if (url.startsWith("https://")) {
            WiFiClientSecure client;
            client.setInsecure();
            client.setTimeout(2500);

            HTTPClient http;
            http.begin(client, url);
            http.setTimeout(2500);
            http.addHeader("Content-Type", contentType);
            http.addHeader("User-Agent", "AutoDoor-RF2");
            if (isNtfy) {
              http.addHeader("Title", "AutoDoor-RF2");
              if (strcmp(item.event, "tag_open") == 0) http.addHeader("Tags", "door,green_circle");
              else if (strcmp(item.event, "tag_closing") == 0) http.addHeader("Tags", "door,yellow_circle");
              else if (strcmp(item.event, "tag_close") == 0) http.addHeader("Tags", "door,lock");
              else if (strcmp(item.event, "safety") == 0) http.addHeader("Tags", "warning,rotating_light");
              else if (strcmp(item.event, "open_warn") == 0) http.addHeader("Tags", "hourglass,warning");
            }
            httpCode = http.POST(payload);
            http.end();
            client.stop();
          } else {
            WiFiClient client;
            client.setTimeout(2500);

            HTTPClient http;
            http.begin(client, url);
            http.setTimeout(2500);
            http.addHeader("Content-Type", contentType);
            http.addHeader("User-Agent", "AutoDoor-RF2");
            if (isNtfy) {
              http.addHeader("Title", "AutoDoor-RF2");
              if (strcmp(item.event, "tag_open") == 0) http.addHeader("Tags", "door,green_circle");
              else if (strcmp(item.event, "tag_closing") == 0) http.addHeader("Tags", "door,yellow_circle");
              else if (strcmp(item.event, "tag_close") == 0) http.addHeader("Tags", "door,lock");
              else if (strcmp(item.event, "safety") == 0) http.addHeader("Tags", "warning,rotating_light");
              else if (strcmp(item.event, "open_warn") == 0) http.addHeader("Tags", "hourglass,warning");
            }
            httpCode = http.POST(payload);
            http.end();
            client.stop();
          }

          sLastStatus = httpCode;
          sLastTimeMs = millis();
          AD_LOG(Serial.printf("[wh_out] event=%s -> http=%d\n", item.event, httpCode));
        }
      }
    }

    // Check door left open duration
    if (sDoorOpenStartMs != 0 && gDoorState != DoorState::Closed) {
      if (gWebhookConfig.enabled && gWebhookConfig.eventOpenDuration && gWebhookConfig.openDurationMinutes > 0) {
        uint32_t elapsedMs = millis() - sDoorOpenStartMs;
        uint32_t elapsedMin = elapsedMs / 60000UL;
        if (elapsedMin >= gWebhookConfig.openDurationMinutes &&
            (sLastOpenWarnSentMin == 0 || elapsedMin >= sLastOpenWarnSentMin + gWebhookConfig.openDurationMinutes)) {
          sLastOpenWarnSentMin = elapsedMin;
          char warnMsg[128];
          snprintf(warnMsg, sizeof(warnMsg), "⚠️ แจ้งเตือน: ประตูเปิดค้างไว้นานกว่า %u นาทีแล้ว!", (unsigned int)elapsedMin);
          triggerOutboundWebhook("open_warn", "open", warnMsg);
        }
      }
    }
  }
}

String resolveTagWebhookUrl(const String& customUrl) {
  String u = customUrl;
  u.trim();
  if (u.isEmpty()) return "";
  if (isLineTarget(u)) {
    if (u.startsWith("line:")) return u.substring(5);
    return u;
  }
  if (u.startsWith("http://") || u.startsWith("https://")) return u;
  return "https://ntfy.sh/" + u;
}

class OutboundDoorListener : public IDoorListener {
public:
  void onDoorStateChanged(DoorState oldState, DoorState newState) override {
    if (newState == DoorState::Opening) {
      if (sDoorOpenStartMs == 0) {
        sDoorOpenStartMs = millis();
      }
      sLastOpenWarnSentMin = 0;

      if (!gActiveTriggerTagId.isEmpty()) {
        sActiveDoorTagId = gActiveTriggerTagId;
        Tag* activeTag = gTagRegistry.findById(gActiveTriggerTagId);
        if (activeTag && activeTag->webhookEnabled && activeTag->webhookOpen) {
          String customUrl = resolveTagWebhookUrl(activeTag->webhookUrl);
          const char* targetUrl = customUrl.isEmpty() ? nullptr : customUrl.c_str();
          String msg = "🟢 ประตูเริ่มเปิด (โดย: " + activeTag->name + ")";
          triggerOutboundWebhook("tag_open", "opening", msg.c_str(), activeTag->id.c_str(), activeTag->name.c_str(), targetUrl);
        }
      }
    } else if (newState == DoorState::Closing) {
      if (sActiveDoorTagId.isEmpty() && !gActiveTriggerTagId.isEmpty()) {
        sActiveDoorTagId = gActiveTriggerTagId;
      }
      if (!sActiveDoorTagId.isEmpty()) {
        Tag* activeTag = gTagRegistry.findById(sActiveDoorTagId);
        if (activeTag && activeTag->webhookEnabled && activeTag->webhookClosing) {
          String customUrl = resolveTagWebhookUrl(activeTag->webhookUrl);
          const char* targetUrl = customUrl.isEmpty() ? nullptr : customUrl.c_str();
          String msg = "🟡 ประตูกำลังปิด (จาก: " + activeTag->name + ")";
          triggerOutboundWebhook("tag_closing", "closing", msg.c_str(), activeTag->id.c_str(), activeTag->name.c_str(), targetUrl);
        }
      }
    } else if (newState == DoorState::Closed) {
      sDoorOpenStartMs = 0;
      sLastOpenWarnSentMin = 0;

      if (!sActiveDoorTagId.isEmpty()) {
        Tag* activeTag = gTagRegistry.findById(sActiveDoorTagId);
        if (activeTag && activeTag->webhookEnabled && activeTag->webhookClose) {
          String customUrl = resolveTagWebhookUrl(activeTag->webhookUrl);
          const char* targetUrl = customUrl.isEmpty() ? nullptr : customUrl.c_str();
          String msg = "🔒 ประตูปิดสนิทเรียบร้อย (จาก: " + activeTag->name + ")";
          triggerOutboundWebhook("tag_close", "closed", msg.c_str(), activeTag->id.c_str(), activeTag->name.c_str(), targetUrl);
        }
        sActiveDoorTagId = "";
      }
    }
  }

  void onSafeAbort() override {
    if ((gWebhookConfig.enabled || gWebhookConfig.lineEnabled) && gWebhookConfig.eventSafety) {
      triggerOutboundWebhook("safety", "reversing", "⚠️ ตรวจพบสิ่งกีดขวาง! ลำแสงเซนเซอร์ Safety ถูกตัด ประตูถอยกลับอัตโนมัติ");
    }
  }
};

OutboundDoorListener sDoorListener;

} // namespace

void loadOutboundWebhookConfig() {
  gWebhookConfig.enabled = gPrefs.getBool("whOutEn", false);
  const String defaultUrl = "https://ntfy.sh/" + resolveApSsid();
  gWebhookConfig.url = gPrefs.getString("whOutUrl", defaultUrl.c_str());
  if (gWebhookConfig.url.isEmpty()) {
    gWebhookConfig.url = defaultUrl;
  }
  gWebhookConfig.lineEnabled = gPrefs.getBool("whLineEn", false);
  gWebhookConfig.lineToken = gPrefs.getString("whLineTok", "");
  gWebhookConfig.lineTargetId = gPrefs.getString("whLineTarget", "");
  gWebhookConfig.eventSafety = gPrefs.getBool("whEvSafe", true);
  gWebhookConfig.eventOpenDuration = gPrefs.getBool("whEvOpenDur", true);
  gWebhookConfig.openDurationMinutes = gPrefs.getUShort("whOpenDurMin", 5);
  if (gWebhookConfig.openDurationMinutes == 0) gWebhookConfig.openDurationMinutes = 5;
}

void saveOutboundWebhookConfig(const OutboundWebhookConfig& cfg) {
  gWebhookConfig = cfg;
  gPrefs.putBool("whOutEn", cfg.enabled);
  gPrefs.putString("whOutUrl", cfg.url);
  gPrefs.putBool("whLineEn", cfg.lineEnabled);
  gPrefs.putString("whLineTok", cfg.lineToken);
  gPrefs.putString("whLineTarget", cfg.lineTargetId);
  gPrefs.putBool("whEvSafe", cfg.eventSafety);
  gPrefs.putBool("whEvOpenDur", cfg.eventOpenDuration);
  gPrefs.putUShort("whOpenDurMin", cfg.openDurationMinutes);
}

void initOutboundWebhook() {
  loadOutboundWebhookConfig();

  if (!sQueue) {
    sQueue = xQueueCreate(4, sizeof(WebhookItem));
  }

  if (!sTaskHandle) {
    xTaskCreate(outboundWebhookTask, "whOutTask", 5120, NULL, 1, &sTaskHandle);
  }

  gDoorController.addListener(&sDoorListener);
  AD_LOG(Serial.println("[wh_out] Outbound webhook service initialized"));
}

bool triggerOutboundWebhook(const char* event, const char* state, const char* message, const char* tagId, const char* tagName, const char* customUrl) {
  if (!sQueue || !event) return false;
  const bool hasCustomUrl = (customUrl != nullptr && customUrl[0] != '\0');
  const bool hasWebhook = (hasCustomUrl || (gWebhookConfig.enabled && gWebhookConfig.url.length() >= 8));
  const bool hasLine = (gWebhookConfig.lineEnabled && gWebhookConfig.lineToken.length() >= 20);
  if (!hasWebhook && !hasLine) return false;

  // Filter check for general/global events
  if (!hasCustomUrl) {
    if (strcmp(event, "safety") == 0 && !gWebhookConfig.eventSafety) return false;
    if (strcmp(event, "open_warn") == 0 && !gWebhookConfig.eventOpenDuration) return false;
  }

  WebhookItem item;
  memset(&item, 0, sizeof(item));
  strncpy(item.event, event, sizeof(item.event) - 1);
  strncpy(item.state, state ? state : "", sizeof(item.state) - 1);
  strncpy(item.message, message ? message : "", sizeof(item.message) - 1);
  if (tagId) strncpy(item.tagId, tagId, sizeof(item.tagId) - 1);
  if (tagName) strncpy(item.tagName, tagName, sizeof(item.tagName) - 1);
  if (hasCustomUrl) strncpy(item.targetUrl, customUrl, sizeof(item.targetUrl) - 1);
  item.timestamp = (uint32_t)time(nullptr);
  item.isTest = false;

  return (xQueueSend(sQueue, &item, 0) == pdTRUE);
}

bool triggerOutboundWebhookTest() {
  if (!sQueue) return false;

  WebhookItem item;
  memset(&item, 0, sizeof(item));
  strncpy(item.event, "test", sizeof(item.event) - 1);
  strncpy(item.state, "test", sizeof(item.state) - 1);
  strncpy(item.message, "ทดสอบส่ง Webhook จาก AutoDoor-RF2 สำเร็จ! (ระบบเชื่อมต่อปกติ)", sizeof(item.message) - 1);
  item.timestamp = (uint32_t)time(nullptr);
  item.isTest = true;

  return (xQueueSend(sQueue, &item, 0) == pdTRUE);
}

int getLastWebhookStatus() {
  return sLastStatus;
}

uint32_t getLastWebhookTimeMs() {
  return sLastTimeMs;
}
