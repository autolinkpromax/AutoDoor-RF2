#include "EventsApiController.h"
#include "modules/event_archive.h"
#include <time.h>

void EventsApiController::registerRoutes() {
  _server.on("/api/events", HTTP_GET, [this]() {
    wrap([this]() { handleEventsList(); });
  });

  _server.on("/api/events/clear", HTTP_POST, [this]() {
    wrap([this]() { handleEventsClear(); });
  });

  _server.on("/api/history/dates", HTTP_GET, [this]() {
    wrap([this]() { handleHistoryDates(); });
  });

  _server.on("/api/history", HTTP_GET, [this]() {
    wrap([this]() { handleHistoryGet(); });
  });

  _server.on("/api/history/save", HTTP_POST, [this]() {
    wrap([this]() { handleHistorySave(); });
  });

  _server.on("/api/history/clear", HTTP_POST, [this]() {
    wrap([this]() { handleHistoryClear(); });
  });

  _server.on("/api/history/export", HTTP_GET, [this]() {
    wrap([this]() { handleHistoryExport(); });
  });
}

void EventsApiController::appendEventJson(const CompactDiagEvent& ev, String& out) {
  DoorEventLogEntry entry = formatEventEntry(ev);
  JsonDocument doc;
  doc["time"] = (const char*)entry.timeStr;
  doc["epoch"] = entry.epochSec;
  doc["type"] = (const char*)entry.code;
  doc["level"] = (const char*)entry.level;
  doc["msg"] = (const char*)entry.message;
  if (entry.reasonStr[0] && strcmp(entry.reasonStr, "None") != 0) {
    doc["reason"] = (const char*)entry.reasonStr;
  }
  if (entry.durationMs > 0) doc["durationMs"] = entry.durationMs;
  if (entry.metricVal != 0) doc["metric"] = entry.metricVal;
  if (entry.entityId[0]) doc["entity"] = (const char*)entry.entityId;
  doc["sensorBits"] = entry.sensorBits;
  doc["flags"] = entry.flags;

  String item;
  serializeJson(doc, item);
  out += item;
}

void EventsApiController::streamEventsResponse(const String& dateStr, const std::vector<CompactDiagEvent>& events) {
  _server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  _server.send(200, "application/json", "");

  String chunk;
  chunk.reserve(1024);
  chunk = "{\"ok\":true,\"date\":\"";
  chunk += dateStr;
  chunk += "\",\"freeHeapKb\":";
  chunk += String((int)(ESP.getFreeHeap() / 1024));
  chunk += ",\"eventCount\":";
  chunk += String(events.size());
  chunk += ",\"events\":[";

  bool first = true;
  for (size_t i = 0; i < events.size(); ++i) {
    if (!first) {
      chunk += ",";
    }
    first = false;

    appendEventJson(events[i], chunk);

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

void EventsApiController::handleEventsList() {
  size_t limit = 100;
  if (_server.hasArg("limit")) {
    int l = _server.arg("limit").toInt();
    if (l > 0 && l <= 1000) limit = (size_t)l;
  }
  auto events = getRecentCompactDoorEvents(limit);
  streamEventsResponse("live", events);
}

void EventsApiController::handleEventsClear() {
  clearDoorEvents();
  JsonDocument doc;
  doc["ok"] = true;
  doc["message"] = "ล้างประวัติเหตุการณ์เรียบร้อยแล้ว";
  sendJson(200, doc);
}

void EventsApiController::handleHistoryDates() {
  auto dates = getArchivedDates();
  JsonDocument doc;
  doc["ok"] = true;
  doc["fsTotalKb"] = (unsigned)(getArchiveFsTotalBytes() / 1024);
  doc["fsUsedKb"] = (unsigned)(getArchiveFsUsedBytes() / 1024);
  JsonArray arr = doc["dates"].to<JsonArray>();
  for (const auto& d : dates) {
    JsonObject o = arr.add<JsonObject>();
    o["date"] = d.dateStr;
    o["formatted"] = d.formattedDate;
    o["eventCount"] = d.eventCount;
    o["fileSizeBytes"] = d.fileSizeBytes;
  }
  sendJson(200, doc);
}

void EventsApiController::handleHistoryGet() {
  String dateStr = _server.arg("date");
  if (dateStr.isEmpty()) {
    JsonDocument err;
    err["ok"] = false;
    err["error"] = "missing_date";
    sendJson(400, err);
    return;
  }
  auto events = readArchivedCompactEvents(dateStr);
  streamEventsResponse(dateStr, events);
}

void EventsApiController::handleHistorySave() {
  bool ok = archiveCurrentEventsToFlash();
  JsonDocument doc;
  doc["ok"] = ok;
  doc["message"] = ok ? "บันทึกประวัติเหตุการณ์ลง Flash สำเร็จ" : "บันทึกข้อมูลไม่สำเร็จ";
  sendJson(ok ? 200 : 500, doc);
}

void EventsApiController::handleHistoryClear() {
  clearAllArchivedLogs();
  JsonDocument doc;
  doc["ok"] = true;
  doc["message"] = "ล้างประวัติเหตุการณ์ย้อนหลังใน Flash ทั้งหมดแล้ว";
  sendJson(200, doc);
}

void EventsApiController::handleHistoryExport() {
  // Ensure live RAM events are archived to Flash first
  archiveCurrentEventsToFlash();

  _server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  _server.sendHeader("Content-Disposition", "attachment; filename=\"autodoor_history_180days.csv\"");
  _server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  _server.sendHeader("Pragma", "no-cache");
  _server.sendHeader("Expires", "0");
  _server.send(200, "text/csv; charset=utf-8", "");

  // Send UTF-8 BOM so Microsoft Excel renders Thai text properly
  String chunk;
  chunk.reserve(1024);
  chunk = "\xEF\xBB\xBF";
  chunk += "ลำดับ,วันที่,เวลา,ผู้เข้า/อุปกรณ์,ช่องทาง,คำสั่ง/เหตุการณ์,สัญญาณ(dBm),UnixTimestamp\n";

  auto dates = getArchivedDates();
  size_t rowNum = 1;

  for (const auto& d : dates) {
    auto records = readArchivedCompactAccessRecords(d.dateStr);
    for (const auto& rec : records) {
      DoorEventLogEntry entry = DiagEventLogger::formatAccessEntry(rec);

      char dateBuf[16] = {0};
      char timeBuf[16] = {0};
      time_t raw = (time_t)rec.epochSec;
      struct tm ti;
      localtime_r(&raw, &ti);
      snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d", ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday);
      snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", ti.tm_hour, ti.tm_min, ti.tm_sec);

      const char* channel = entry.reasonStr[0] ? entry.reasonStr : "ทั่วไป";

      String whoStr = entry.entityId;
      if (whoStr.isEmpty()) whoStr = "-";
      whoStr.replace("\"", "\"\"");

      String msgStr = entry.message;
      msgStr.replace("\"", "\"\"");

      chunk += String(rowNum++) + ",";
      chunk += String(dateBuf) + ",";
      chunk += String(timeBuf) + ",";
      chunk += "\"" + whoStr + "\",";
      chunk += "\"" + String(channel) + "\",";
      chunk += "\"" + msgStr + "\",";
      chunk += "-,";
      chunk += String(rec.epochSec) + "\n";

      if (chunk.length() >= 1024) {
        _server.sendContent(chunk);
        chunk = "";
        chunk.reserve(1024);
        delay(1);
        yield();
      }
    }
  }

  if (chunk.length() > 0) {
    _server.sendContent(chunk);
  }
  _server.sendContent("");
}

