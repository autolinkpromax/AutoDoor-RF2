#pragma once

#include "BaseController.h"
#include "diagnostics/DiagEventLogger.h"
#include <vector>

/**
 * @brief Controller for realtime diagnostics event log and historical LittleFS flash archive.
 */
class EventsApiController : public BaseController {
public:
  explicit EventsApiController(WebServer& server) : BaseController(server) {}

  void registerRoutes() override;

private:
  void handleEventsList();
  void handleEventsClear();
  void handleHistoryDates();
  void handleHistoryGet();
  void handleHistorySave();
  void handleHistoryClear();
  void handleHistoryExport();

  void appendEventJson(const CompactDiagEvent& ev, String& out);
  void streamEventsResponse(const String& dateStr, const std::vector<CompactDiagEvent>& events);
};
