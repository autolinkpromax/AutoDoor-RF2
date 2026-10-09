#pragma once

#include <Arduino.h>
#include <vector>
#include "app/globals.h"
#include "app/types.h"

struct ArchiveDateInfo {
  String dateStr;        // e.g. "20260907"
  String formattedDate;  // e.g. "2026-09-07"
  uint16_t eventCount = 0;
  size_t fileSizeBytes = 0;
};

void initEventArchive();
void stepEventArchive();
bool archiveCurrentEventsToFlash();
std::vector<ArchiveDateInfo> getArchivedDates();
std::vector<DoorEventLogEntry> readArchivedEvents(const String& dateStr);
std::vector<CompactDiagEvent> readArchivedCompactEvents(const String& dateStr);
std::vector<CompactAccessRecord> readArchivedCompactAccessRecords(const String& dateStr);
size_t getArchiveFsTotalBytes();
size_t getArchiveFsUsedBytes();
void clearAllArchivedLogs();
void triggerArchiveOnOverflow();
