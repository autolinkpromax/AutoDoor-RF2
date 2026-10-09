#include "schedule.h"

#include "app/config.h"
#include "app/globals.h"
#include "modules/door.h"
#include "tags/TagRegistry.h"

#include <ArduinoJson.h>
#include <time.h>

namespace {

void loadSchedFiredState() {
  const String json = gPrefs.getString("schedFired", "");
  if (json.isEmpty()) return;
  JsonDocument doc;
  if (deserializeJson(doc, json)) return;
  for (auto& e : gSchedules) {
    JsonVariantConst v = doc[e.id.c_str()];
    if (!v.isNull()) {
      e.lastFiredYmd = v.as<int>();
    }
  }
}

void saveSchedFiredState() {
  TagStoreLock lock;
  JsonDocument doc;
  for (const auto& e : gSchedules) {
    if (e.lastFiredYmd > 0) {
      doc[e.id.c_str()] = e.lastFiredYmd;
    }
  }
  String out;
  serializeJson(doc, out);
  gPrefs.putString("schedFired", out);
}

}  // namespace

void loadSchedulesFromPrefs() {
  TagStoreLock lock;
  gSchedules.clear();
  const String json = gPrefs.getString("schedules", "");
  if (json.isEmpty()) return;

  JsonDocument doc;
  if (deserializeJson(doc, json)) return;
  for (JsonObject o : doc.as<JsonArray>()) {
    ScheduleEntry e;
    e.id = String((const char*)(o["id"] | ""));
    if (e.id.isEmpty()) continue;
    e.enabled = o["enabled"] | false;
    e.action = String((const char*)(o["action"] | "open"));
    if (e.action != "open" && e.action != "close") {
      e.action = "open";
    }
    e.hour = (uint8_t)constrain((int)(o["hour"] | 0), 0, 23);
    e.minute = (uint8_t)constrain((int)(o["minute"] | 0), 0, 59);
    if (o["days"].is<JsonArrayConst>()) {
      e.daysMask = 0;
      for (JsonVariantConst dv : o["days"].as<JsonArrayConst>()) {
        const int day = dv.as<int>();
        if (day >= 0 && day <= 6) {
          e.daysMask |= (uint8_t)(1u << day);
        }
      }
      if (e.daysMask == 0) e.daysMask = 0x7F;
    } else {
      e.daysMask = (uint8_t)(o["daysMask"] | 0x7F);
    }
    gSchedules.push_back(e);
  }
  loadSchedFiredState();
}

void saveSchedulesToPrefs() {
  TagStoreLock lock;
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
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
  String out;
  serializeJson(doc, out);
  gPrefs.putString("schedules", out);
}

void stepSchedule() {
  if (gSafeMode || !gTimeSynced || gSchedules.empty()) {
    return;
  }
  const uint32_t nowMs = millis();
  if ((uint32_t)(nowMs - gLastScheduleCheckMs) < 1000UL) {
    return;
  }
  gLastScheduleCheckMs = nowMs;

  struct tm ti;
  if (!getLocalTime(&ti)) return;

  const int ymd = (ti.tm_year + 1900) * 10000 + (ti.tm_mon + 1) * 100 + ti.tm_mday;
  const int wday = ti.tm_wday;
  const int nowMin = ti.tm_hour * 60 + ti.tm_min;

  static bool sNtpFirstSyncHandled = false;
  if (!sNtpFirstSyncHandled) {
    sNtpFirstSyncHandled = true;
    for (auto& e : gSchedules) {
      if (e.lastFiredYmd == 0) {
        e.lastFiredYmd = ymd;
      }
    }
    saveSchedFiredState();
    AD_LOG(Serial.println("[schedule] Initialized un-fired schedules on NTP sync"));
    return;
  }

  for (auto& e : gSchedules) {
    if (!e.enabled) continue;
    if (!(e.daysMask & (1u << wday))) continue;
    const int schedMin = e.hour * 60 + e.minute;
    if (abs(nowMin - schedMin) > 0) continue;
    if (ti.tm_sec > 30) continue;
    if (e.lastFiredYmd == ymd) continue;
    e.lastFiredYmd = ymd;
    saveSchedFiredState();
    AD_LOG(Serial.printf("[schedule] fire id=%s action=%s\n", e.id.c_str(), e.action.c_str()));
    requestDoorSlot(e.action, true);
  }
}
