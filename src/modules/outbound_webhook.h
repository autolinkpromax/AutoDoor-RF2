#pragma once

#include <Arduino.h>

struct OutboundWebhookConfig {
  bool enabled = false;
  String url = "";
  bool lineEnabled = false;
  String lineToken = "";
  String lineTargetId = "";
  bool eventSafety = true;
  bool eventOpenDuration = true;
  uint16_t openDurationMinutes = 5;
};

extern OutboundWebhookConfig gWebhookConfig;

void initOutboundWebhook();
void loadOutboundWebhookConfig();
void saveOutboundWebhookConfig(const OutboundWebhookConfig& cfg);
bool triggerOutboundWebhook(const char* event, const char* state, const char* message, const char* tagId = nullptr, const char* tagName = nullptr, const char* customUrl = nullptr);
bool triggerOutboundWebhookTest();
int getLastWebhookStatus();
uint32_t getLastWebhookTimeMs();
