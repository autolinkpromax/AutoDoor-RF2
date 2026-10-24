#include "mqtt_service.h"

#include "app/globals.h"
#include "core/DoorController.h"
#include "core/IDoorListener.h"
#include "modules/door.h"
#include "modules/wifi_manager.h"
#include "tags/TagRegistry.h"

#include <WiFi.h>
#include <ArduinoJson.h>
#include <mqtt_client.h>

MqttConfig gMqttConfig;

namespace {

esp_mqtt_client_handle_t sMqttClient = nullptr;
bool sMqttConnected = false;
String sLastStatusText = "ไม่ได้เปิดใช้งาน";
String sDeviceId = "";
String sActiveTagId = "";

String getDeviceId() {
  if (sDeviceId.isEmpty()) {
    const uint64_t mac = ESP.getEfuseMac();
    char buf[24];
    snprintf(buf, sizeof(buf), "autodoor_%06x", (unsigned int)(mac & 0xFFFFFF));
    sDeviceId = String(buf);
  }
  return sDeviceId;
}

String getCmdTopic() {
  return gMqttConfig.topicPrefix + "/cmd";
}

String getStateTopic() {
  return gMqttConfig.topicPrefix + "/state";
}

String getAvailTopic() {
  return gMqttConfig.topicPrefix + "/availability";
}

String getSafetyTopic() {
  return gMqttConfig.topicPrefix + "/safety";
}

String getTagTopic() {
  return gMqttConfig.topicPrefix + "/last_tag";
}

void publishDiscovery() {
  if (!sMqttClient || !sMqttConnected) return;

  const String devId = getDeviceId();
  const String availTopic = getAvailTopic();
  const String discPrefix = gMqttConfig.discoveryPrefix.isEmpty() ? "homeassistant" : gMqttConfig.discoveryPrefix;

  // 1. Cover Entity (Garage Door)
  {
    JsonDocument doc;
    doc["name"] = "AutoDoor";
    doc["unique_id"] = devId + "_cover";
    doc["device_class"] = "garage";
    doc["command_topic"] = getCmdTopic();
    doc["state_topic"] = getStateTopic();
    doc["availability_topic"] = availTopic;
    doc["payload_open"] = "OPEN";
    doc["payload_close"] = "CLOSE";
    doc["payload_stop"] = "STOP";
    doc["state_open"] = "open";
    doc["state_opening"] = "opening";
    doc["state_closed"] = "closed";
    doc["state_closing"] = "closing";
    doc["state_stopped"] = "stopped";

    JsonObject dev = doc["device"].to<JsonObject>();
    JsonArray ids = dev["identifiers"].to<JsonArray>();
    ids.add(devId);
    dev["name"] = "AutoDoor-RF2";
    dev["model"] = "AutoDoor-RF2 ESP32-C3";
    dev["manufacturer"] = "AutoLink";
    dev["sw_version"] = "1.0.1";

    String payload;
    serializeJson(doc, payload);
    const String discTopic = discPrefix + "/cover/" + devId + "/config";
    esp_mqtt_client_publish(sMqttClient, discTopic.c_str(), payload.c_str(), payload.length(), 1, 1);
  }

  // 2. Binary Sensor (Safety Beam)
  {
    JsonDocument doc;
    doc["name"] = "AutoDoor Safety Beam";
    doc["unique_id"] = devId + "_safety";
    doc["device_class"] = "safety";
    doc["state_topic"] = getSafetyTopic();
    doc["availability_topic"] = availTopic;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";

    JsonObject dev = doc["device"].to<JsonObject>();
    JsonArray ids = dev["identifiers"].to<JsonArray>();
    ids.add(devId);

    String payload;
    serializeJson(doc, payload);
    const String discTopic = discPrefix + "/binary_sensor/" + devId + "_safety/config";
    esp_mqtt_client_publish(sMqttClient, discTopic.c_str(), payload.c_str(), payload.length(), 1, 1);
  }

  // 3. Sensor (Last Trigger Tag)
  {
    JsonDocument doc;
    doc["name"] = "AutoDoor Last Trigger";
    doc["unique_id"] = devId + "_tag";
    doc["icon"] = "mdi:car-key";
    doc["state_topic"] = getTagTopic();
    doc["availability_topic"] = availTopic;

    JsonObject dev = doc["device"].to<JsonObject>();
    JsonArray ids = dev["identifiers"].to<JsonArray>();
    ids.add(devId);

    String payload;
    serializeJson(doc, payload);
    const String discTopic = discPrefix + "/sensor/" + devId + "_tag/config";
    esp_mqtt_client_publish(sMqttClient, discTopic.c_str(), payload.c_str(), payload.length(), 1, 1);
  }

  // 4. Sensor (Door State Thai Text)
  {
    JsonDocument doc;
    doc["name"] = "สถานะประตู";
    doc["unique_id"] = devId + "_state_text";
    doc["icon"] = "mdi:garage";
    doc["state_topic"] = gMqttConfig.topicPrefix + "/state_text";
    doc["availability_topic"] = availTopic;

    JsonObject dev = doc["device"].to<JsonObject>();
    JsonArray ids = dev["identifiers"].to<JsonArray>();
    ids.add(devId);

    String payload;
    serializeJson(doc, payload);
    const String discTopic = discPrefix + "/sensor/" + devId + "_state_text/config";
    esp_mqtt_client_publish(sMqttClient, discTopic.c_str(), payload.c_str(), payload.length(), 1, 1);
  }

  // 5. Button (Open)
  {
    JsonDocument doc;
    doc["name"] = "เปิดประตู";
    doc["unique_id"] = devId + "_btn_open";
    doc["command_topic"] = getCmdTopic();
    doc["payload_press"] = "OPEN";
    doc["icon"] = "mdi:arrow-up-bold-box";
    doc["availability_topic"] = availTopic;

    JsonObject dev = doc["device"].to<JsonObject>();
    JsonArray ids = dev["identifiers"].to<JsonArray>();
    ids.add(devId);

    String payload;
    serializeJson(doc, payload);
    const String discTopic = discPrefix + "/button/" + devId + "_open/config";
    esp_mqtt_client_publish(sMqttClient, discTopic.c_str(), payload.c_str(), payload.length(), 1, 1);
  }

  // 6. Button (Stop)
  {
    JsonDocument doc;
    doc["name"] = "หยุดประตู";
    doc["unique_id"] = devId + "_btn_stop";
    doc["command_topic"] = getCmdTopic();
    doc["payload_press"] = "STOP";
    doc["icon"] = "mdi:stop-circle";
    doc["availability_topic"] = availTopic;

    JsonObject dev = doc["device"].to<JsonObject>();
    JsonArray ids = dev["identifiers"].to<JsonArray>();
    ids.add(devId);

    String payload;
    serializeJson(doc, payload);
    const String discTopic = discPrefix + "/button/" + devId + "_stop/config";
    esp_mqtt_client_publish(sMqttClient, discTopic.c_str(), payload.c_str(), payload.length(), 1, 1);
  }

  // 7. Button (Close)
  {
    JsonDocument doc;
    doc["name"] = "ปิดประตู";
    doc["unique_id"] = devId + "_btn_close";
    doc["command_topic"] = getCmdTopic();
    doc["payload_press"] = "CLOSE";
    doc["icon"] = "mdi:arrow-down-bold-box";
    doc["availability_topic"] = availTopic;

    JsonObject dev = doc["device"].to<JsonObject>();
    JsonArray ids = dev["identifiers"].to<JsonArray>();
    ids.add(devId);

    String payload;
    serializeJson(doc, payload);
    const String discTopic = discPrefix + "/button/" + devId + "_close/config";
    esp_mqtt_client_publish(sMqttClient, discTopic.c_str(), payload.c_str(), payload.length(), 1, 1);
  }

  AD_LOG(Serial.println("[mqtt] Home Assistant Auto-Discovery published"));
}

void handleIncomingCommand(const char* data, int len) {
  if (len <= 0) return;
  char cmd[16] = {0};
  const int copyLen = (len < (int)sizeof(cmd) - 1) ? len : (sizeof(cmd) - 1);
  memcpy(cmd, data, copyLen);
  cmd[copyLen] = '\0';

  // Trim whitespace
  char* p = cmd;
  while (*p == ' ' || *p == '\r' || *p == '\n') p++;
  char* end = p + strlen(p) - 1;
  while (end > p && (*end == ' ' || *end == '\r' || *end == '\n')) *end-- = '\0';

  Serial.printf("[mqtt] Received command: '%s'\n", p);

  if (strcasecmp(p, "OPEN") == 0) {
    gActiveTriggerTagId = "";
    requestDoorSlot("open", true, true);
  } else if (strcasecmp(p, "CLOSE") == 0) {
    gActiveTriggerTagId = "";
    requestDoorSlot("close", true, false);
  } else if (strcasecmp(p, "STOP") == 0) {
    gActiveTriggerTagId = "";
    requestDoorSlot("stop", true, true);
  }
}

void mqttEventHandler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
  switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED: {
      sMqttConnected = true;
      sLastStatusText = "เชื่อมต่อสำเร็จ (Connected)";
      Serial.printf("[mqtt] Connected to broker %s:%u\n", gMqttConfig.host.c_str(), gMqttConfig.port);

      // Publish Online availability
      const String availTopic = getAvailTopic();
      esp_mqtt_client_publish(sMqttClient, availTopic.c_str(), "online", 6, 1, 1);

      // Subscribe to command topic
      const String cmdTopic = getCmdTopic();
      esp_mqtt_client_subscribe(sMqttClient, cmdTopic.c_str(), 1);

      // Publish Auto-Discovery configs
      publishDiscovery();

      // Publish initial state
      publishMqttDoorState(DoorController::stateToString(gDoorState));
      publishMqttSafety(false);
      break;
    }
    case MQTT_EVENT_DISCONNECTED: {
      sMqttConnected = false;
      sLastStatusText = "หลุดการเชื่อมต่อ (Disconnected)";
      AD_LOG(Serial.println("[mqtt] Disconnected from broker"));
      break;
    }
    case MQTT_EVENT_DATA: {
      const String cmdTopic = getCmdTopic();
      if (event->topic_len == (int)cmdTopic.length() && strncmp(event->topic, cmdTopic.c_str(), event->topic_len) == 0) {
        handleIncomingCommand(event->data, event->data_len);
      }
      break;
    }
    case MQTT_EVENT_ERROR: {
      sLastStatusText = "เกิดข้อผิดพลาดในการเชื่อมต่อ (Error)";
      AD_LOG(Serial.println("[mqtt] Event Error"));
      break;
    }
    default:
      break;
  }
}

class MqttDoorListener : public IDoorListener {
public:
  void onDoorStateChanged(DoorState oldState, DoorState newState) override {
    if (!gMqttConfig.enabled || !sMqttConnected) return;

    if (newState == DoorState::Opening) {
      if (!gActiveTriggerTagId.isEmpty()) {
        sActiveTagId = gActiveTriggerTagId;
        Tag* tag = gTagRegistry.findById(gActiveTriggerTagId);
        if (tag) {
          publishMqttLastTag(tag->name.c_str());
        }
      } else {
        publishMqttLastTag("Manual / App");
      }
      publishMqttDoorState("opening");
      publishMqttSafety(false);
    } else if (newState == DoorState::Open) {
      publishMqttDoorState("open");
    } else if (newState == DoorState::Closing) {
      publishMqttDoorState("closing");
      publishMqttSafety(false);
    } else if (newState == DoorState::Closed) {
      publishMqttDoorState("closed");
      sActiveTagId = "";
    } else if (newState == DoorState::Stopped) {
      publishMqttDoorState("stopped");
    }
  }

  void onSafeAbort() override {
    if (!gMqttConfig.enabled || !sMqttConnected) return;
    publishMqttSafety(true);
  }
};

MqttDoorListener sMqttDoorListener;

} // namespace

void loadMqttConfig() {
  gMqttConfig.enabled = gPrefs.getBool("mqttEn", false);
  gMqttConfig.host = gPrefs.getString("mqttHost", "");
  gMqttConfig.port = gPrefs.getUShort("mqttPort", 1883);
  if (gMqttConfig.port == 0) gMqttConfig.port = 1883;
  gMqttConfig.user = gPrefs.getString("mqttUser", "");
  gMqttConfig.pass = gPrefs.getString("mqttPass", "");
  gMqttConfig.topicPrefix = gPrefs.getString("mqttTopic", "autodoor");
  if (gMqttConfig.topicPrefix.isEmpty()) gMqttConfig.topicPrefix = "autodoor";
  gMqttConfig.discoveryPrefix = gPrefs.getString("mqttDisc", "homeassistant");
  if (gMqttConfig.discoveryPrefix.isEmpty()) gMqttConfig.discoveryPrefix = "homeassistant";
}

void saveMqttConfig(const MqttConfig& cfg) {
  gMqttConfig = cfg;
  gPrefs.putBool("mqttEn", cfg.enabled);
  gPrefs.putString("mqttHost", cfg.host);
  gPrefs.putUShort("mqttPort", cfg.port);
  gPrefs.putString("mqttUser", cfg.user);
  gPrefs.putString("mqttPass", cfg.pass);
  gPrefs.putString("mqttTopic", cfg.topicPrefix);
  gPrefs.putString("mqttDisc", cfg.discoveryPrefix);

  // Restart MQTT client with new settings
  initMqttService();
}

bool isMqttConnected() {
  return sMqttConnected;
}

String getMqttStatusText() {
  if (!gMqttConfig.enabled) return "ปิดใช้งาน (Disabled)";
  return sLastStatusText;
}

void publishMqttDoorState(const char* stateStr) {
  if (!sMqttClient || !sMqttConnected || !stateStr) return;
  const String topic = getStateTopic();
  esp_mqtt_client_publish(sMqttClient, topic.c_str(), stateStr, strlen(stateStr), 1, 1);

  // Also publish state_text (Thai text)
  const String textTopic = gMqttConfig.topicPrefix + "/state_text";
  const char* thText = "ไม่ทราบสถานะ";
  if (strcasecmp(stateStr, "closed") == 0) thText = "ปิดสนิท";
  else if (strcasecmp(stateStr, "opening") == 0) thText = "กำลังเปิด...";
  else if (strcasecmp(stateStr, "open") == 0) thText = "เปิดสุด";
  else if (strcasecmp(stateStr, "closing") == 0) thText = "กำลังปิด...";
  else if (strcasecmp(stateStr, "stopped") == 0) thText = "หยุดชั่วคราว";
  esp_mqtt_client_publish(sMqttClient, textTopic.c_str(), thText, strlen(thText), 1, 1);
}

void publishMqttSafety(bool triggered) {
  if (!sMqttClient || !sMqttConnected) return;
  const String topic = getSafetyTopic();
  const char* val = triggered ? "ON" : "OFF";
  esp_mqtt_client_publish(sMqttClient, topic.c_str(), val, strlen(val), 1, 1);
}

void publishMqttLastTag(const char* tagName) {
  if (!sMqttClient || !sMqttConnected || !tagName) return;
  const String topic = getTagTopic();
  esp_mqtt_client_publish(sMqttClient, topic.c_str(), tagName, strlen(tagName), 1, 1);
}

String sanitizeMqttHost(String host) {
  host.trim();
  if (host.startsWith("http://")) host = host.substring(7);
  else if (host.startsWith("https://")) host = host.substring(8);
  else if (host.startsWith("mqtt://")) host = host.substring(7);
  else if (host.startsWith("mqtts://")) host = host.substring(8);
  int slashIdx = host.indexOf('/');
  if (slashIdx >= 0) host = host.substring(0, slashIdx);
  int colonIdx = host.indexOf(':');
  if (colonIdx >= 0) host = host.substring(0, colonIdx);
  host.trim();
  return host;
}

void initMqttService() {
  loadMqttConfig();

  static bool sListenerAdded = false;
  if (!sListenerAdded) {
    gDoorController.addListener(&sMqttDoorListener);
    sListenerAdded = true;
  }

  // Stop previous client if exists
  if (sMqttClient) {
    esp_mqtt_client_stop(sMqttClient);
    esp_mqtt_client_destroy(sMqttClient);
    sMqttClient = nullptr;
    sMqttConnected = false;
  }

  if (!gMqttConfig.enabled || gMqttConfig.host.isEmpty()) {
    sLastStatusText = "ปิดใช้งาน (Disabled)";
    return;
  }

  static String sHostHold;
  static String sUserHold;
  static String sPassHold;
  static String sLwtHold;

  sHostHold = sanitizeMqttHost(gMqttConfig.host);
  sUserHold = gMqttConfig.user;
  sPassHold = gMqttConfig.pass;
  sLwtHold = getAvailTopic();

  esp_mqtt_client_config_t mqtt_cfg = {};
  mqtt_cfg.broker.address.hostname = sHostHold.c_str();
  mqtt_cfg.broker.address.port = gMqttConfig.port;
  mqtt_cfg.broker.address.transport = MQTT_TRANSPORT_OVER_TCP;

  if (!sUserHold.isEmpty()) {
    mqtt_cfg.credentials.username = sUserHold.c_str();
    if (!sPassHold.isEmpty()) {
      mqtt_cfg.credentials.authentication.password = sPassHold.c_str();
    }
  }

  mqtt_cfg.session.last_will.topic = sLwtHold.c_str();
  mqtt_cfg.session.last_will.msg = "offline";
  mqtt_cfg.session.last_will.qos = 1;
  mqtt_cfg.session.last_will.retain = 1;
  mqtt_cfg.session.keepalive = 60;

  sMqttClient = esp_mqtt_client_init(&mqtt_cfg);
  if (sMqttClient) {
    esp_mqtt_client_register_event(sMqttClient, MQTT_EVENT_ANY, mqttEventHandler, NULL);
    esp_mqtt_client_start(sMqttClient);
    sLastStatusText = "กำลังเชื่อมต่อ... (Connecting)";
    Serial.printf("[mqtt] Started client for %s:%u\n", sHostHold.c_str(), gMqttConfig.port);
  } else {
    sLastStatusText = "สร้างการเชื่อมต่อไม่สำเร็จ (Init Failed)";
    Serial.println("[mqtt] ERROR: Failed to initialize esp_mqtt_client");
  }
}
