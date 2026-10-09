#include "TagsApiController.h"
#include "app/globals.h"
#include "tags/TagRegistry.h"
#include "modules/esp_now_runtime.h"
#include <uri/UriBraces.h>

void TagsApiController::registerRoutes() {
  _server.on("/api/tags", HTTP_GET, [this]() {
    wrap([this]() { handleTagsList(); });
  });

  _server.on("/api/tags", HTTP_POST, [this]() {
    wrap([this]() { handleTagsCreate(); });
  });

  _server.on("/api/tags/learn", HTTP_POST, [this]() {
    wrap([this]() { handleTagLearn(); });
  });

  _server.on(UriBraces("/api/tags/{}"), HTTP_PUT, [this]() {
    wrap([this]() { handleTagUpdate(); });
  });

  _server.on(UriBraces("/api/tags/{}"), HTTP_DELETE, [this]() {
    wrap([this]() { handleTagDelete(); });
  });

  _server.on("/api/tag-auto", HTTP_POST, [this]() {
    wrap([this]() { handleTagAutoPost(); });
  });
}

void TagsApiController::handleTagsList() {
  TagStoreLock lock;
  const uint32_t now = millis();

  _server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  _server.send(200, "application/json", "");

  String chunk;
  chunk.reserve(1024);
  chunk = "{\"ok\":true,\"tags\":[";

  bool first = true;
  for (const auto& t : gTagRegistry.tags()) {
    if (!first) {
      chunk += ",";
    }
    first = false;

    const bool isRf = isRfTag(t);
    const uint32_t effectiveHoldMs = isRf ? kDefaultRfTagHoldMs : (t.tagHoldMs > 0 ? t.tagHoldMs : kDefaultTagHoldMs);
    const bool active = isTagActive(t, now);
    JsonDocument itemDoc;
    JsonObject o = itemDoc.to<JsonObject>();
    o["id"] = t.id;
    o["name"] = t.name;
    o["mac"] = t.mac;
    o["isRf"] = isRf;
    o["tagId"] = t.tagId;
    o["tagHoldMs"] = effectiveHoldMs;
    o["tagCloseDelayMs"] = t.tagCloseDelayMs;
    o["tagRssiMin"] = (int)t.tagRssiMin;
    o["tagSafetyCloseDelayMs"] = t.tagSafetyCloseDelayMs;
    o["tagOpenDurationMs"] = t.tagOpenDurationMs;
    o["rfRole"] = (int)t.rfRole;
    o["icon"] = t.icon;
    o["scheduleEnabled"] = t.scheduleEnabled;
    o["scheduleStartMin"] = t.scheduleStartMin;
    o["scheduleEndMin"] = t.scheduleEndMin;
    o["active"] = active;
    o["lastRssi"] = (int)t.lastRssi;
    o["rssiZone"] = tagRssiZone(t, now, active);
    if (t.lastSeenMs != 0) {
      o["lastSeenAgeMs"] = (uint32_t)(now - t.lastSeenMs);
      o["holdRemainingMs"] = active ? (int32_t)((int32_t)effectiveHoldMs - (int32_t)(now - t.lastSeenMs)) : 0;
    } else {
      o["lastSeenAgeMs"] = nullptr;
      o["holdRemainingMs"] = 0;
    }

    String itemStr;
    serializeJson(itemDoc, itemStr);
    chunk += itemStr;

    if (chunk.length() >= 1024) {
      _server.sendContent(chunk);
      chunk = "";
      chunk.reserve(1024);
      delay(1);
    }
  }

  chunk += "]}";
  _server.sendContent(chunk);
  _server.sendContent("");
}

void TagsApiController::handleTagsCreate() {
  TagStoreLock lock;
  if (gTagRegistry.count() >= TagRegistry::kMaxTags) {
    JsonDocument err;
    err["ok"] = false;
    err["error"] = "tag_limit_reached";
    sendJson(400, err);
    return;
  }
  JsonDocument body = parseBodyOrEmpty();
  const String name = String((const char*)(body["name"] | "Tag"));
  const String mac = String((const char*)(body["mac"] | ""));
  const uint32_t tagId = body["tagId"] | 0U;
  const String icon = body["icon"].isNull() ? "" : String((const char*)body["icon"]);
  if (mac.isEmpty()) {
    JsonDocument err;
    err["ok"] = false;
    err["error"] = "mac_required";
    sendJson(400, err);
    return;
  }
  uint8_t addr[6];
  if (!parseMac(mac, addr)) {
    JsonDocument err;
    err["ok"] = false;
    err["error"] = "bad_mac";
    sendJson(400, err);
    return;
  }
  Tag* t = createTag(name, mac, tagId, icon);
  if (!t) {
    JsonDocument err;
    err["ok"] = false;
    err["error"] = "duplicate_mac";
    sendJson(400, err);
    return;
  }
  const uint32_t hold = isRfTag(*t) ? kDefaultRfTagHoldMs : (body["tagHoldMs"] | t->tagHoldMs);
  const uint32_t closeDelay = body["tagCloseDelayMs"] | t->tagCloseDelayMs;
  const int8_t rssi = (int8_t)(int)(body["tagRssiMin"] | (int)t->tagRssiMin);
  const uint32_t safetyClose = body["tagSafetyCloseDelayMs"].isNull() ? t->tagSafetyCloseDelayMs : body["tagSafetyCloseDelayMs"].as<uint32_t>();
  const uint32_t openDuration = body["tagOpenDurationMs"].isNull() ? t->tagOpenDurationMs : body["tagOpenDurationMs"].as<uint32_t>();
  const uint8_t rfRole = body["rfRole"].isNull() ? 0 : (uint8_t)body["rfRole"].as<unsigned int>();
  const bool schedEnabled = body["scheduleEnabled"] | false;
  const uint16_t schedStart = (uint16_t)constrain((int)(body["scheduleStartMin"] | 0), 0, 1439);
  const uint16_t schedEnd = (uint16_t)constrain((int)(body["scheduleEndMin"] | 1439), 0, 1439);
  updateTag(t->id, name, hold, closeDelay, rssi, safetyClose, openDuration, rfRole, icon,
            schedEnabled, schedStart, schedEnd);
  syncEspNowPeers();
  saveTagsToPrefs();
  JsonDocument doc;
  doc["ok"] = true;
  doc["id"] = t->id;
  sendJson(200, doc);
}

void TagsApiController::handleTagUpdate() {
  TagStoreLock lock;
  const String id = _server.pathArg(0);
  JsonDocument body = parseBodyOrEmpty();
  Tag* t = findTagById(id);
  if (!t) {
    JsonDocument err;
    err["ok"] = false;
    err["error"] = "not_found";
    sendJson(404, err);
    return;
  }
  const String name = String((const char*)(body["name"] | t->name.c_str()));
  const uint32_t hold = isRfTag(*t) ? kDefaultRfTagHoldMs : (body["tagHoldMs"].isNull() ? t->tagHoldMs : body["tagHoldMs"].as<uint32_t>());
  const uint32_t closeDelay = body["tagCloseDelayMs"].isNull() ? t->tagCloseDelayMs : body["tagCloseDelayMs"].as<uint32_t>();
  const int8_t rssi = body["tagRssiMin"].isNull() ? t->tagRssiMin : (int8_t)body["tagRssiMin"].as<int>();
  const uint32_t safetyClose = body["tagSafetyCloseDelayMs"].isNull() ? t->tagSafetyCloseDelayMs : body["tagSafetyCloseDelayMs"].as<uint32_t>();
  const uint32_t openDuration = body["tagOpenDurationMs"].isNull() ? t->tagOpenDurationMs : body["tagOpenDurationMs"].as<uint32_t>();
  const uint8_t rfRole = body["rfRole"].isNull() ? t->rfRole : (uint8_t)body["rfRole"].as<unsigned int>();
  const String icon = body["icon"].isNull() ? t->icon : String((const char*)body["icon"]);
  const bool schedEnabled = body["scheduleEnabled"].isNull() ? t->scheduleEnabled : body["scheduleEnabled"].as<bool>();
  const uint16_t schedStart = body["scheduleStartMin"].isNull() ? t->scheduleStartMin : (uint16_t)constrain((int)body["scheduleStartMin"].as<int>(), 0, 1439);
  const uint16_t schedEnd = body["scheduleEndMin"].isNull() ? t->scheduleEndMin : (uint16_t)constrain((int)body["scheduleEndMin"].as<int>(), 0, 1439);
  updateTag(id, name, hold, closeDelay, rssi, safetyClose, openDuration, rfRole, icon,
            schedEnabled, schedStart, schedEnd);
  saveTagsToPrefs();
  JsonDocument doc;
  doc["ok"] = true;
  sendJson(200, doc);
}

void TagsApiController::handleTagDelete() {
  TagStoreLock lock;
  const String id = _server.pathArg(0);
  Tag* t = findTagById(id);
  if (!t) {
    JsonDocument err;
    err["ok"] = false;
    err["error"] = "not_found";
    sendJson(404, err);
    return;
  }
  const String mac = t->mac;
  deleteTag(id);
  removeEspNowPeer(mac);
  saveTagsToPrefs();
  JsonDocument doc;
  doc["ok"] = true;
  sendJson(200, doc);
}

void TagsApiController::handleTagLearn() {
  TagStoreLock lock;
  JsonDocument body = parseBodyOrEmpty();
  const String action = String((const char*)(body["action"] | "start"));
  if (action == "stop") {
    stopTagLearn();
  } else {
    startTagLearn();
  }
  JsonDocument doc;
  doc["ok"] = true;
  doc["active"] = gLearn.active;
  sendJson(200, doc);
}

void TagsApiController::handleTagAutoPost() {
  TagStoreLock lock;
  JsonDocument body = parseBodyOrEmpty();
  if (!body["enabled"].isNull()) {
    gTagAutoEnabled = body["enabled"].as<bool>();
    gPrefs.putBool("tagAutoEnabled", gTagAutoEnabled);
    if (!gTagAutoEnabled) {
      gPendingCloseAtMs = 0;
      gPrevAnyTagActive = false;
      gTagRegistry.clearSuppression();
    }
  }
  JsonDocument doc;
  doc["ok"] = true;
  doc["enabled"] = gTagAutoEnabled;
  sendJson(200, doc);
}
