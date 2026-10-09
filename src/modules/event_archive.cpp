#include "event_archive.h"

#include <LittleFS.h>
#include <time.h>
#include <algorithm>
#include <vector>

#include "app/config.h"
#include "app/globals.h"
#include "app/types.h"
#include "diagnostics/DiagEventLogger.h"

namespace {

constexpr uint32_t kArchiveMagic = 0x41444C47; // "ADLG" (AutoDoor Log)
constexpr uint16_t kArchiveVersion = 6; // V6: Ultra-Minimal 6-Byte Access Records (epochSec 4B + tagId 2B)
constexpr const char* kLogDir = "/logs";
constexpr uint32_t kPeriodicArchiveIntervalMs = 1800000UL; // 30 minutes (48 writes/day, ~5.7yr Flash lifespan) [Fix M8]
constexpr uint16_t kDefaultMaxDays = 180; // Retain logs for up to 180 days (6 months)
constexpr size_t kMinFreeSpaceBytes = 24576; // 24 KB minimum safety margin
constexpr size_t kMaxEventsPerDay = 1000;    // Limit per day to protect Flash and RAM
// kUtcOffsetSec removed: getYmdFromEpoch now uses localtime_r() which respects ICT-7 TZ from ntp.cpp [Fix]


struct __attribute__((packed)) ArchiveHeader {
  uint32_t magic;
  uint16_t version;
  uint16_t recordCount;
};
static_assert(sizeof(ArchiveHeader) == 8, "ArchiveHeader must be 8 bytes");

// Legacy V5 struct (8 bytes)
struct __attribute__((packed)) CompactAccessRecordV5 {
  uint32_t epochSec = 0;
  uint16_t tagId = 0;
  uint8_t  actionMethod = 0;
  int8_t   rssi = 0;
};
static_assert(sizeof(CompactAccessRecordV5) == 8, "CompactAccessRecordV5 must be 8 bytes");

// Legacy V4 struct (6 bytes)
struct __attribute__((packed)) CompactAccessRecordV4 {
  uint32_t epochSec = 0;
  uint16_t tagId : 12;
  uint16_t method : 2;
  uint16_t action : 2;
};
static_assert(sizeof(CompactAccessRecordV4) == 6, "CompactAccessRecordV4 must be 6 bytes");

// Legacy V1 struct (32 bytes)
struct __attribute__((packed)) CompactDiagEventV1 {
  uint32_t timestampMs = 0;
  uint32_t epochSec = 0;
  DoorEventCode code = DoorEventCode::None;
  DiagReasonCode reason = DiagReasonCode::None;
  uint8_t sensorBits = 0;
  uint8_t flags = 0;
  uint16_t durationMs = 0;
  int16_t metricVal = 0;
  char entityId[16] = {0};
};
static_assert(sizeof(CompactDiagEventV1) == 32, "CompactDiagEventV1 must be 32 bytes");

// Legacy V2 struct (40 bytes)
struct __attribute__((packed)) CompactDiagEventV2 {
  uint32_t timestampMs = 0;
  uint32_t epochSec = 0;
  DoorEventCode code = DoorEventCode::None;
  DiagReasonCode reason = DiagReasonCode::None;
  uint8_t sensorBits = 0;
  uint8_t flags = 0;
  uint16_t durationMs = 0;
  int16_t metricVal = 0;
  char entityId[24] = {0};
};
static_assert(sizeof(CompactDiagEventV2) == 40, "CompactDiagEventV2 must be 40 bytes");

CompactDiagEvent convertV1ToV3(const CompactDiagEventV1& v1) {
  CompactDiagEvent e;
  if (v1.epochSec > 0) {
    e.timestamp = v1.epochSec;
    e.flags = v1.flags | FLAG_EPOCH_TIME;
  } else {
    e.timestamp = v1.timestampMs / 1000;
    e.flags = v1.flags & ~FLAG_EPOCH_TIME;
  }
  e.code = v1.code;
  e.reason = v1.reason;
  e.sensorBits = v1.sensorBits;
  e.durationMs = v1.durationMs;
  e.metricVal = (int8_t)constrain(v1.metricVal, -128, 127);
  e.entityRef = DiagEventLogger::encodeEntity(v1.entityId);
  return e;
}

CompactDiagEvent convertV2ToV3(const CompactDiagEventV2& v2) {
  CompactDiagEvent e;
  if (v2.epochSec > 0) {
    e.timestamp = v2.epochSec;
    e.flags = v2.flags | FLAG_EPOCH_TIME;
  } else {
    e.timestamp = v2.timestampMs / 1000;
    e.flags = v2.flags & ~FLAG_EPOCH_TIME;
  }
  e.code = v2.code;
  e.reason = v2.reason;
  e.sensorBits = v2.sensorBits;
  e.durationMs = v2.durationMs;
  e.metricVal = (int8_t)constrain(v2.metricVal, -128, 127);
  e.entityRef = DiagEventLogger::encodeEntity(v2.entityId);
  return e;
}

CompactAccessRecord convertV3ToAccessRecord(const CompactDiagEvent& ev) {
  CompactAccessRecord rec;
  rec.epochSec = ev.timestamp;
  if (ev.code == DoorEventCode::TagOpen) {
    rec.tagId = ev.entityRef;
  } else if (ev.code == DoorEventCode::RfHold || ev.code == DoorEventCode::RfOpen) {
    rec.tagId = (ev.entityRef > 0 && ev.entityRef <= 80) ? (ACCESS_TAG_RF_BASE + ev.entityRef) : ev.entityRef;
  } else if (ev.code == DoorEventCode::ManualOpen) {
    rec.tagId = (ev.entityRef == ENTITY_BLYNK) ? ACCESS_TAG_BLYNK :
                (ev.entityRef == ENTITY_SCHEDULE) ? ACCESS_TAG_SCHEDULE :
                (ev.entityRef == ENTITY_BOOT_BTN) ? ACCESS_TAG_BOOT_BTN : ACCESS_TAG_WEB;
  } else {
    rec.tagId = ACCESS_TAG_EXTERNAL;
  }
  return rec;
}

uint32_t sLastArchiveCheckMs = 0;
uint32_t sLastArchivedTotalEvents = 0;
uint32_t sLastPruneYmd = 0;
bool sFsMounted = false;

// Convert epoch seconds to date integer (YYYYMMDD) with zero heap allocation
uint32_t getYmdFromEpoch(uint32_t epochSec) {
  if (epochSec == 0) {
    if (gTimeSynced) {
      struct tm ti;
      if (getLocalTime(&ti)) {
        return (uint32_t)(ti.tm_year + 1900) * 10000 + (ti.tm_mon + 1) * 100 + ti.tm_mday;
      }
    }
    return 19700101;
  }
  // Use localtime_r() which respects TZ env "ICT-7" set by configTzTime() in ntp.cpp.
  // Previously used gmtime_r + manual kUtcOffsetSec; after configTzTime fix, localtime_r
  // already returns correct ICT time so manual offset is no longer needed.
  time_t raw = (time_t)epochSec;
  struct tm ti;
  localtime_r(&raw, &ti);
  return (uint32_t)(ti.tm_year + 1900) * 10000 + (ti.tm_mon + 1) * 100 + ti.tm_mday;
}

void formatYmdString(uint32_t ymd, char* out, size_t outSize) {
  snprintf(out, outSize, "%08lu", (unsigned long)ymd);
}

String formatDisplayDate(const String& ymd) {
  if (ymd.length() == 8) {
    return ymd.substring(0, 4) + "-" + ymd.substring(4, 6) + "-" + ymd.substring(6, 8);
  }
  return ymd;
}

// 64-bit unique key for instant exact deduplication
inline uint64_t accessRecordKey(const CompactAccessRecord& r) {
  return ((uint64_t)r.epochSec << 16) | (uint64_t)r.tagId;
}

// Check if two consecutive events are identical duplicates (e.g. repeated bounce within 8s)
inline bool isConsecutiveDuplicate(const CompactAccessRecord& prev, const CompactAccessRecord& curr) {
  if (prev.tagId != curr.tagId) return false;
  return (curr.epochSec >= prev.epochSec && (curr.epochSec - prev.epochSec) <= 8);
}

struct DayGroup {
  uint32_t ymd = 0;
  std::vector<CompactAccessRecord> records;
};

} // namespace

static volatile bool sNeedOverflowArchive = false;

void triggerArchiveOnOverflow() {
  sNeedOverflowArchive = true;
}

void initEventArchive() {
  if (sFsMounted) return;
  // Mount LittleFS using the dedicated "spiffs" 192KB partition, format on first boot if unformatted
  if (!LittleFS.begin(true, "/littlefs", 10, "spiffs")) {
    Serial.println("[archive] ERROR: LittleFS mount failed on partition 'spiffs'");
    return;
  }
  sFsMounted = true;
  Serial.printf("[archive] LittleFS mounted successfully: Total=%lu KB, Used=%lu KB\n",
                (unsigned long)(LittleFS.totalBytes() / 1024),
                (unsigned long)(LittleFS.usedBytes() / 1024));

  if (!LittleFS.exists(kLogDir)) {
    LittleFS.mkdir(kLogDir);
  }
}

size_t getArchiveFsTotalBytes() {
  return sFsMounted ? LittleFS.totalBytes() : 0;
}

size_t getArchiveFsUsedBytes() {
  return sFsMounted ? LittleFS.usedBytes() : 0;
}

void pruneOldArchives(uint16_t maxDays) {
  if (!sFsMounted) return;

  File root = LittleFS.open(kLogDir);
  if (!root) return;
  if (!root.isDirectory()) {
    root.close();
    return;
  }

  std::vector<String> dateFiles;
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String name = file.name();
      int lastSlash = name.lastIndexOf('/');
      if (lastSlash >= 0) name = name.substring(lastSlash + 1);
      if (name.endsWith(".dat") && name.length() == 12) { // "YYYYMMDD.dat"
        dateFiles.push_back(name.substring(0, 8));
      }
    }
    file.close();
    file = root.openNextFile();
  }
  root.close();

  // Sort descending by date (newest first)
  std::sort(dateFiles.begin(), dateFiles.end(), std::greater<String>());

  // Remove oldest files beyond allowed days
  if (dateFiles.size() > maxDays) {
    for (size_t i = maxDays; i < dateFiles.size(); ++i) {
      String filePath = String(kLogDir) + "/" + dateFiles[i] + ".dat";
      LittleFS.remove(filePath);
      Serial.printf("[archive] Pruned old log archive: %s\n", filePath.c_str());
    }
  }

  // Verify minimum safety free space (check once to minimize Flash traversal)
  size_t total = LittleFS.totalBytes();
  size_t used = LittleFS.usedBytes();
  if (total > used && (total - used) < kMinFreeSpaceBytes && !dateFiles.empty()) {
    for (int i = (int)dateFiles.size() - 1; i >= 0; --i) {
      if (total - LittleFS.usedBytes() >= kMinFreeSpaceBytes * 2) break;
      String filePath = String(kLogDir) + "/" + dateFiles[i] + ".dat";
      LittleFS.remove(filePath);
      Serial.printf("[archive] Space low -> removed archive: %s\n", filePath.c_str());
    }
  }
}

bool archiveCurrentEventsToFlash() {
  if (!sFsMounted) {
    initEventArchive();
    if (!sFsMounted) return false;
  }

  // Fetch 6-byte binary access records directly from in-memory ring buffer (100% 12-bit tagId fidelity)
  auto inMem = gDiagLogger.getRecentCompactAccessRecords(kMaxEventsPerDay);
  if (inMem.empty()) {
    return true;
  }

  // Sort chronologically ascending (oldest first)
  std::sort(inMem.begin(), inMem.end(), [](const CompactAccessRecord& a, const CompactAccessRecord& b) {
    return a.epochSec < b.epochSec;
  });

  // Group by date (uint32_t YYYYMMDD) without heap String allocations
  // Ring buffer almost always has only 1 or 2 distinct dates
  std::vector<DayGroup> dayGroups;
  for (const auto& rec : inMem) {
    uint32_t ymd = getYmdFromEpoch(rec.epochSec);
    DayGroup* targetGroup = nullptr;
    for (auto& dg : dayGroups) {
      if (dg.ymd == ymd) {
        targetGroup = &dg;
        break;
      }
    }
    if (!targetGroup) {
      dayGroups.push_back({ymd, {}});
      targetGroup = &dayGroups.back();
    }
    targetGroup->records.push_back(rec);
  }

  bool allOk = true;
  for (auto& group : dayGroups) {
    char dateBuf[16];
    formatYmdString(group.ymd, dateBuf, sizeof(dateBuf));
    const String filePath = String(kLogDir) + "/" + dateBuf + ".dat";
    const String tempPath = String(kLogDir) + "/temp.dat";

    std::vector<CompactAccessRecord> merged;
    std::vector<uint64_t> seenKeys;

    // Read existing file if it already exists
    if (LittleFS.exists(filePath)) {
      File f = LittleFS.open(filePath, "r");
      if (f) {
        ArchiveHeader hdr;
        if (f.read((uint8_t*)&hdr, sizeof(hdr)) == sizeof(hdr) && hdr.magic == kArchiveMagic) {
          if (hdr.version == 6) {
            merged.resize(hdr.recordCount);
            size_t readBytes = f.read((uint8_t*)merged.data(), hdr.recordCount * sizeof(CompactAccessRecord));
            if (readBytes != hdr.recordCount * sizeof(CompactAccessRecord)) {
              merged.resize(readBytes / sizeof(CompactAccessRecord));
            }
          } else if (hdr.version == 5) {
            std::vector<CompactAccessRecordV5> v5Records(hdr.recordCount);
            size_t readBytes = f.read((uint8_t*)v5Records.data(), hdr.recordCount * sizeof(CompactAccessRecordV5));
            size_t actualCount = readBytes / sizeof(CompactAccessRecordV5);
            merged.reserve(merged.size() + actualCount);
            for (size_t i = 0; i < actualCount; ++i) {
              CompactAccessRecord rec;
              rec.epochSec = v5Records[i].epochSec;
              rec.tagId = v5Records[i].tagId;
              merged.push_back(rec);
            }
          } else if (hdr.version == 4) {
            std::vector<CompactAccessRecordV4> v4Records(hdr.recordCount);
            size_t readBytes = f.read((uint8_t*)v4Records.data(), hdr.recordCount * sizeof(CompactAccessRecordV4));
            size_t actualCount = readBytes / sizeof(CompactAccessRecordV4);
            merged.reserve(merged.size() + actualCount);
            for (size_t i = 0; i < actualCount; ++i) {
              CompactAccessRecord rec;
              rec.epochSec = v4Records[i].epochSec;
              rec.tagId = v4Records[i].tagId;
              merged.push_back(rec);
            }
          } else if (hdr.version == 3) {
            std::vector<CompactDiagEvent> v3Records(hdr.recordCount);
            size_t readBytes = f.read((uint8_t*)v3Records.data(), hdr.recordCount * sizeof(CompactDiagEvent));
            size_t actualCount = readBytes / sizeof(CompactDiagEvent);
            merged.reserve(actualCount);
            for (size_t i = 0; i < actualCount; ++i) {
              merged.push_back(convertV3ToAccessRecord(v3Records[i]));
            }
          } else if (hdr.version == 1) {
            std::vector<CompactDiagEventV1> v1Records(hdr.recordCount);
            size_t readBytes = f.read((uint8_t*)v1Records.data(), hdr.recordCount * sizeof(CompactDiagEventV1));
            size_t actualCount = readBytes / sizeof(CompactDiagEventV1);
            merged.reserve(actualCount);
            for (size_t i = 0; i < actualCount; ++i) {
              merged.push_back(convertV3ToAccessRecord(convertV1ToV3(v1Records[i])));
            }
          } else if (hdr.version == 2) {
            std::vector<CompactDiagEventV2> v2Records(hdr.recordCount);
            size_t readBytes = f.read((uint8_t*)v2Records.data(), hdr.recordCount * sizeof(CompactDiagEventV2));
            size_t actualCount = readBytes / sizeof(CompactDiagEventV2);
            merged.reserve(actualCount);
            for (size_t i = 0; i < actualCount; ++i) {
              merged.push_back(convertV3ToAccessRecord(convertV2ToV3(v2Records[i])));
            }
          }
        }
        f.close();
      }
    }

    const size_t existingCount = merged.size();

    // Populate existing keys
    seenKeys.reserve(existingCount + group.records.size());
    for (const auto& r : merged) {
      seenKeys.push_back(accessRecordKey(r));
    }
    std::sort(seenKeys.begin(), seenKeys.end());

    CompactAccessRecord lastAdded{};
    bool hasLastAdded = !merged.empty();
    if (hasLastAdded) {
      lastAdded = merged.back();
    }

    // Append new unique events (deduplicating using binary search on seenKeys and checking consecutive spam)
    for (const auto& rec : group.records) {
      const uint64_t key = accessRecordKey(rec);
      if (std::binary_search(seenKeys.begin(), seenKeys.end(), key)) {
        continue;
      }
      if (hasLastAdded && isConsecutiveDuplicate(lastAdded, rec)) {
        continue;
      }
      seenKeys.insert(std::lower_bound(seenKeys.begin(), seenKeys.end(), key), key);
      merged.push_back(rec);
      lastAdded = rec;
      hasLastAdded = true;
    }

    // If no new unique events were added, skip writing to Flash to eliminate Flash wear!
    if (merged.size() == existingCount) {
      continue;
    }

    // Sort chronologically (oldest to newest)
    std::sort(merged.begin(), merged.end(), [](const CompactAccessRecord& a, const CompactAccessRecord& b) {
      return a.epochSec < b.epochSec;
    });

    // Cap at kMaxEventsPerDay to protect Flash space
    if (merged.size() > kMaxEventsPerDay) {
      merged.erase(merged.begin(), merged.begin() + (merged.size() - kMaxEventsPerDay));
    }

    // Write to temporary file with atomic Block Write
    File tf = LittleFS.open(tempPath, "w");
    if (!tf) {
      Serial.printf("[archive] ERROR: Failed to open %s for writing\n", tempPath.c_str());
      allOk = false;
      continue;
    }

    ArchiveHeader outHdr;
    outHdr.magic = kArchiveMagic;
    outHdr.version = kArchiveVersion;
    outHdr.recordCount = (uint16_t)merged.size();

    tf.write((const uint8_t*)&outHdr, sizeof(outHdr));
    if (!merged.empty()) {
      tf.write((const uint8_t*)merged.data(), merged.size() * sizeof(CompactAccessRecord));
    }
    tf.flush();
    tf.close();

    // Atomic replace
    LittleFS.remove(filePath);
    if (LittleFS.rename(tempPath, filePath)) {
      Serial.printf("[archive] Saved %u unique compact access records to %s (%u new, %u total bytes)\n",
                    outHdr.recordCount, filePath.c_str(),
                    (unsigned)(merged.size() - existingCount),
                    (unsigned)(sizeof(outHdr) + merged.size() * sizeof(CompactAccessRecord)));
    } else {
      Serial.printf("[archive] ERROR: Rename %s -> %s failed\n", tempPath.c_str(), filePath.c_str());
      allOk = false;
    }
  }

  sLastArchivedTotalEvents = getTotalEventsLogged();

  // Run pruning only once per day (or on boot) instead of every 10 minutes!
  time_t nowSec = time(nullptr);
  uint32_t currentYmd = getYmdFromEpoch((uint32_t)nowSec);
  if (currentYmd != sLastPruneYmd) {
    sLastPruneYmd = currentYmd;
    pruneOldArchives(kDefaultMaxDays);
  }

  return allOk;
}

void stepEventArchive() {
  // Never write to Flash while the door is actively moving (opening or closing)
  // Ensures 100% of CPU time and bus bandwidth is dedicated to motor relays and safety sensors
  if (gDoorState == DoorState::Opening || gDoorState == DoorState::Closing) {
    return;
  }

  const uint32_t now = millis();

  // 1. Immediate flush if RAM buffer reached limit or overflow was triggered
  if (sNeedOverflowArchive) {
    sNeedOverflowArchive = false;
    Serial.println("[archive] RAM ring buffer reached limit -> Auto-saving to Flash now!");
    if (archiveCurrentEventsToFlash()) {
      sLastArchivedTotalEvents = getTotalEventsLogged();
      sLastArchiveCheckMs = now;
    }
  }

  if (sLastArchiveCheckMs == 0) {
    sLastArchiveCheckMs = now;
    return;
  }

  // 2. Periodic 10-minute batch check: flush unique events to Flash every 10 minutes
  if ((uint32_t)(now - sLastArchiveCheckMs) >= kPeriodicArchiveIntervalMs) {
    sLastArchiveCheckMs = now;
    const uint32_t currentTotal = getTotalEventsLogged();
    if (currentTotal != sLastArchivedTotalEvents) {
      if (archiveCurrentEventsToFlash()) {
        sLastArchivedTotalEvents = currentTotal;
      }
    }
  }
}

std::vector<ArchiveDateInfo> getArchivedDates() {
  std::vector<ArchiveDateInfo> list;
  if (!sFsMounted) return list;

  File root = LittleFS.open(kLogDir);
  if (!root) return list;
  if (!root.isDirectory()) {
    root.close();
    return list;
  }

  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String name = file.name();
      int lastSlash = name.lastIndexOf('/');
      if (lastSlash >= 0) name = name.substring(lastSlash + 1);
      if (name.endsWith(".dat") && name.length() == 12) { // "YYYYMMDD.dat"
        String dateStr = name.substring(0, 8);
        ArchiveDateInfo info;
        info.dateStr = dateStr;
        info.formattedDate = formatDisplayDate(dateStr);
        info.fileSizeBytes = file.size();

        ArchiveHeader hdr;
        if (file.read((uint8_t*)&hdr, sizeof(hdr)) == sizeof(hdr) && hdr.magic == kArchiveMagic) {
          info.eventCount = hdr.recordCount;
        }
        list.push_back(info);
      }
    }
    file.close();
    file = root.openNextFile();
  }
  root.close();

  // Sort descending by date (newest first)
  std::sort(list.begin(), list.end(), [](const ArchiveDateInfo& a, const ArchiveDateInfo& b) {
    return a.dateStr > b.dateStr;
  });

  return list;
}

std::vector<CompactAccessRecord> readArchivedCompactAccessRecords(const String& dateStr) {
  std::vector<CompactAccessRecord> result;
  if (!sFsMounted || dateStr.isEmpty()) return result;

  String cleanDate = dateStr;
  cleanDate.replace("-", "");
  String filePath = String(kLogDir) + "/" + cleanDate + ".dat";
  if (!LittleFS.exists(filePath)) {
    return result;
  }

  File f = LittleFS.open(filePath, "r");
  if (!f) return result;

  ArchiveHeader hdr;
  if (f.read((uint8_t*)&hdr, sizeof(hdr)) != sizeof(hdr) || hdr.magic != kArchiveMagic || (hdr.version < 1 || hdr.version > 6)) {
    f.close();
    return result;
  }

  if (hdr.version == 6) {
    result.resize(hdr.recordCount);
    size_t readBytes = f.read((uint8_t*)result.data(), hdr.recordCount * sizeof(CompactAccessRecord));
    f.close();
    if (readBytes != hdr.recordCount * sizeof(CompactAccessRecord)) {
      result.resize(readBytes / sizeof(CompactAccessRecord));
    }
    std::reverse(result.begin(), result.end());
  } else if (hdr.version == 5) {
    std::vector<CompactAccessRecordV5> v5Records(hdr.recordCount);
    size_t readBytes = f.read((uint8_t*)v5Records.data(), hdr.recordCount * sizeof(CompactAccessRecordV5));
    f.close();
    size_t actualCount = readBytes / sizeof(CompactAccessRecordV5);
    result.reserve(actualCount);
    for (int i = (int)actualCount - 1; i >= 0; --i) {
      CompactAccessRecord rec;
      rec.epochSec = v5Records[i].epochSec;
      rec.tagId = v5Records[i].tagId;
      result.push_back(rec);
    }
  } else if (hdr.version == 4) {
    std::vector<CompactAccessRecordV4> v4Records(hdr.recordCount);
    size_t readBytes = f.read((uint8_t*)v4Records.data(), hdr.recordCount * sizeof(CompactAccessRecordV4));
    f.close();
    size_t actualCount = readBytes / sizeof(CompactAccessRecordV4);
    result.reserve(actualCount);
    for (int i = (int)actualCount - 1; i >= 0; --i) {
      CompactAccessRecord rec;
      rec.epochSec = v4Records[i].epochSec;
      rec.tagId = v4Records[i].tagId;
      result.push_back(rec);
    }
  } else if (hdr.version == 3) {
    std::vector<CompactDiagEvent> v3Records(hdr.recordCount);
    size_t readBytes = f.read((uint8_t*)v3Records.data(), hdr.recordCount * sizeof(CompactDiagEvent));
    f.close();
    size_t actualCount = readBytes / sizeof(CompactDiagEvent);
    result.reserve(actualCount);
    for (int i = (int)actualCount - 1; i >= 0; --i) {
      result.push_back(convertV3ToAccessRecord(v3Records[i]));
    }
  } else if (hdr.version == 1) {
    std::vector<CompactDiagEventV1> v1Records(hdr.recordCount);
    size_t readBytes = f.read((uint8_t*)v1Records.data(), hdr.recordCount * sizeof(CompactDiagEventV1));
    f.close();
    size_t actualCount = readBytes / sizeof(CompactDiagEventV1);
    result.reserve(actualCount);
    for (int i = (int)actualCount - 1; i >= 0; --i) {
      result.push_back(convertV3ToAccessRecord(convertV1ToV3(v1Records[i])));
    }
  } else if (hdr.version == 2) {
    std::vector<CompactDiagEventV2> v2Records(hdr.recordCount);
    size_t readBytes = f.read((uint8_t*)v2Records.data(), hdr.recordCount * sizeof(CompactDiagEventV2));
    f.close();
    size_t actualCount = readBytes / sizeof(CompactDiagEventV2);
    result.reserve(actualCount);
    for (int i = (int)actualCount - 1; i >= 0; --i) {
      result.push_back(convertV3ToAccessRecord(convertV2ToV3(v2Records[i])));
    }
  }

  return result;
}

std::vector<CompactDiagEvent> readArchivedCompactEvents(const String& dateStr) {
  auto records = readArchivedCompactAccessRecords(dateStr);
  std::vector<CompactDiagEvent> result;
  result.reserve(records.size());
  for (const auto& rec : records) {
    CompactDiagEvent ev;
    ev.timestamp = rec.epochSec;
    ev.flags = FLAG_EPOCH_TIME;
    ev.durationMs = 0;
    ev.metricVal = 0;
    if (rec.tagId == ACCESS_TAG_WEB) {
      ev.code = DoorEventCode::ManualOpen;
      ev.entityRef = (uint8_t)ENTITY_WEB;
    } else if (rec.tagId == ACCESS_TAG_BLYNK) {
      ev.code = DoorEventCode::ManualOpen;
      ev.entityRef = (uint8_t)ENTITY_BLYNK;
    } else if (rec.tagId == ACCESS_TAG_SCHEDULE) {
      ev.code = DoorEventCode::ManualOpen;
      ev.entityRef = (uint8_t)ENTITY_SCHEDULE;
    } else if (rec.tagId == ACCESS_TAG_BOOT_BTN) {
      ev.code = DoorEventCode::ManualOpen;
      ev.entityRef = (uint8_t)ENTITY_BOOT_BTN;
    } else if (rec.tagId > ACCESS_TAG_RF_BASE && rec.tagId <= ACCESS_TAG_RF_BASE + 80) {
      ev.code = DoorEventCode::RfOpen;
      ev.entityRef = (uint8_t)(rec.tagId - ACCESS_TAG_RF_BASE);
    } else if (rec.tagId == ACCESS_TAG_EXTERNAL) {
      ev.code = DoorEventCode::MagOpen;
      ev.entityRef = (uint8_t)ENTITY_SAFETY;
    } else {
      ev.code = DoorEventCode::TagOpen;
      ev.entityRef = (uint8_t)(rec.tagId < 255 ? rec.tagId : 255);
    }
    result.push_back(ev);
  }
  return result;
}

std::vector<DoorEventLogEntry> readArchivedEvents(const String& dateStr) {
  auto records = readArchivedCompactAccessRecords(dateStr);
  std::vector<DoorEventLogEntry> result;
  result.reserve(records.size());
  for (const auto& rec : records) {
    result.push_back(DiagEventLogger::formatAccessEntry(rec));
  }
  return result;
}

void clearAllArchivedLogs() {
  if (!sFsMounted) return;
  File root = LittleFS.open(kLogDir);
  if (!root) return;
  if (!root.isDirectory()) {
    root.close();
    return;
  }

  File file = root.openNextFile();
  while (file) {
    String p = file.path();
    file.close();
    LittleFS.remove(p);
    file = root.openNextFile();
  }
  root.close();
  Serial.println("[archive] Cleared all archived log files");
}
