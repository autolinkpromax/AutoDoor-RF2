#pragma once

#include <Arduino.h>

struct MqttConfig {
  bool enabled = false;
  String host = "";
  uint16_t port = 1883;
  String user = "";
  String pass = "";
  String topicPrefix = "autodoor";
  String discoveryPrefix = "homeassistant";
};

extern MqttConfig gMqttConfig;

void initMqttService();
void loadMqttConfig();
void saveMqttConfig(const MqttConfig& cfg);
bool isMqttConnected();
String getMqttStatusText();
void publishMqttDoorState(const char* stateStr);
void publishMqttSafety(bool triggered);
void publishMqttLastTag(const char* tagName);
