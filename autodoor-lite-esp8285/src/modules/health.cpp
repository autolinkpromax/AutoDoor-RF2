#include "health.h"

#include <Arduino.h>

namespace {

uint32_t gBootMs = 0;
uint32_t gDailyRestartMs = 0;
uint32_t gLastStepMs = 0;
uint8_t gLowHeapCount = 0;

}  // namespace

namespace Health {

void begin(uint32_t dailyRestartMs) {
  gBootMs = millis();
  gLastStepMs = gBootMs;
  gDailyRestartMs = dailyRestartMs;
  ESP.wdtEnable(4000);
}

void step() {
  ESP.wdtFeed();

  const uint32_t now = millis();
  const uint32_t gap = now - gLastStepMs;
  gLastStepMs = now;

  if (gap > 15000) {
    ESP.restart();
  }

  const uint32_t freeHeap = ESP.getFreeHeap();
  if (freeHeap < 7000) {
    if (++gLowHeapCount >= 8) {
      ESP.restart();
    }
  } else {
    gLowHeapCount = 0;
  }

  if (gDailyRestartMs > 0 && (now - gBootMs) >= gDailyRestartMs) {
    ESP.restart();
  }
}

}  // namespace Health
