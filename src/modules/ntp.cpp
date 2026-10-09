#include "ntp.h"

#include "app/globals.h"

#include <WiFi.h>
#include <time.h>

namespace {

bool gNtpConfigured = false;
uint32_t gNtpLastCheckMs = 0;
uint32_t gNtpLastResyncMs = 0;
constexpr uint32_t kNtpCheckIntervalMs = 200;
constexpr uint32_t kNtpResyncIntervalMs = 86400000UL;
constexpr int kNtpMaxAttempts = 40;
int gNtpAttemptCount = 0;

void ensureNtpConfigured() {
  if (gNtpConfigured) return;
  setenv("TZ", "ICT-7", 1);
  tzset();
  // Use Thailand timezone (ICT-7: UTC+7, no DST) and Thailand NTP pool servers
  configTzTime("ICT-7", "th.pool.ntp.org", "asia.pool.ntp.org", "pool.ntp.org");
  gNtpConfigured = true;
}

bool tryReadSyncedTime() {
  struct tm ti;
  if (!getLocalTime(&ti, 0)) {
    return false;
  }
  gTimeSynced = true;
  Serial.printf("[ntp] synced %04d-%02d-%02d %02d:%02d:%02d\n", ti.tm_year + 1900, ti.tm_mon + 1,
                ti.tm_mday, ti.tm_hour, ti.tm_min, ti.tm_sec);
  return true;
}

void requestNtpResync() {
  ensureNtpConfigured();
  configTzTime("ICT-7", "th.pool.ntp.org", "asia.pool.ntp.org", "pool.ntp.org");
}

}  // namespace

bool syncTimeFromNtp() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  ensureNtpConfigured();
  if (tryReadSyncedTime()) {
    gNtpLastResyncMs = millis();
    return true;
  }
  return false;
}

void stepNtpAuto() {
  const uint32_t now = millis();
  static uint32_t lastLoopCheckMs = 0;
  if ((uint32_t)(now - lastLoopCheckMs) < 5000) return;
  lastLoopCheckMs = now;

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  ensureNtpConfigured();

  if (gTimeSynced) {
    if (gNtpLastResyncMs == 0) {
      gNtpLastResyncMs = now;
      return;
    }
    if ((uint32_t)(now - gNtpLastResyncMs) >= kNtpResyncIntervalMs) {
      gNtpLastResyncMs = now;
      requestNtpResync();
    }
    return;
  }

  if ((int32_t)(now - gNtpLastCheckMs) < (int32_t)kNtpCheckIntervalMs) {
    return;
  }
  gNtpLastCheckMs = now;

  if (tryReadSyncedTime()) {
    gNtpLastResyncMs = now;
    return;
  }

  gNtpAttemptCount++;
  if (gNtpAttemptCount >= kNtpMaxAttempts) {
    gNtpAttemptCount = 0;
  }
}
