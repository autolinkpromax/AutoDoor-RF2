#include "daily_restart.h"

#include "app/config.h"
#include "app/globals.h"
#include "modules/door.h"
#include "tags/TagRegistry.h"

#include "modules/event_archive.h"

#include <esp_task_wdt.h>
#include <time.h>

namespace {

uint32_t gLastRestartCheckMs = 0;
constexpr uint32_t kRestartCheckIntervalMs = 30000;
static uint32_t gFallbackDeferStartMs = 0;
constexpr uint32_t kMaxFallbackDeferMs = 10UL * 60000UL;

void doScheduledRestart(const char* reason) {
  Serial.printf("[restart] %s\n", reason);
  Serial.println("[restart] Archiving logs before restart...");
  esp_task_wdt_reset();
  archiveCurrentEventsToFlash();
  esp_task_wdt_reset();
  delay(200);
  ESP.restart();
}

}  // namespace

void stepDailyRestart() {
  if (gSafeMode || !gAutoRebootEnabled) {
    return;
  }

  const uint32_t now = millis();
  if (gLastRestartCheckMs != 0 &&
      (int32_t)(now - gLastRestartCheckMs) < (int32_t)kRestartCheckIntervalMs) {
    return;
  }
  gLastRestartCheckMs = now;

  if (gTimeSynced) {
    struct tm ti;
    if (!getLocalTime(&ti)) {
      return;
    }
    if (ti.tm_hour != gAutoRebootHour || ti.tm_min < gAutoRebootMinute) {
      return;
    }
    const int ymd = (ti.tm_year + 1900) * 10000 + (ti.tm_mon + 1) * 100 + ti.tm_mday;
    TagStoreLock lock;
    const int lastYmd = gPrefs.getInt("lastRestartYmd", 0);
    if (lastYmd == ymd) {
      return;
    }

    // SAFETY INTERLOCK: Door MUST be confirmed closed, not moving, no active tags, and no safety beam breaks!
    if (!isDoorSafeToRestart()) {
      Serial.printf("[restart] Daily %02u:%02u ICT restart deferred: Door is busy/open/safety active\n",
                    (unsigned)gAutoRebootHour, (unsigned)gAutoRebootMinute);
      return;
    }

    gPrefs.putInt("lastRestartYmd", ymd);
    char reasonBuf[64];
    snprintf(reasonBuf, sizeof(reasonBuf), "daily %02u:%02u ICT (door confirmed closed)",
             (unsigned)gAutoRebootHour, (unsigned)gAutoRebootMinute);
    doScheduledRestart(reasonBuf);
    return;
  }

  if ((uint32_t)(now - gBootMs) >= kDailyRestartFallbackMs) {
    if (!isDoorSafeToRestart()) {
      if (gFallbackDeferStartMs == 0) {
        gFallbackDeferStartMs = now;
      } else if ((uint32_t)(now - gFallbackDeferStartMs) >= kMaxFallbackDeferMs) {
        Serial.println("[restart] FORCE restart: door not safe but fallback defer exceeded 10min");
        doScheduledRestart("uptime 24h forced (door defer timeout)");
        return;
      }
      Serial.println("[restart] 24h fallback restart deferred: Door is busy/open/safety active");
      return;
    }
    gFallbackDeferStartMs = 0;
    doScheduledRestart("uptime 24h (no NTP, door confirmed closed)");
  }
}

