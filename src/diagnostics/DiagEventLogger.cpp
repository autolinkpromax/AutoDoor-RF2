#include "DiagEventLogger.h"
#include "app/globals.h"
#include "modules/door.h"
#include "modules/event_archive.h"
#include "tags/TagRegistry.h"

// ============================================================================
// DiagEventLogger implementation: Ultra-Compact Binary Access Log
// ============================================================================

DiagEventLogger gDiagLogger;

namespace {

constexpr size_t kEntityPoolSlots = 16;
constexpr size_t kEntitySlotMaxLen = 16;
char sEntityPool[kEntityPoolSlots][kEntitySlotMaxLen] = {{0}};
uint8_t sEntityPoolHead = 0;

} // namespace

DiagEventLogger::DiagEventLogger() {
  memset(_ringBuffer, 0, sizeof(_ringBuffer));
  _mux = portMUX_INITIALIZER_UNLOCKED;
}

// ============================================================================
// ULTRA-COMPACT 6-BYTE BINARY PACKING
// ============================================================================

void DiagEventLogger::encodeAccessRecord(const AccessLogRecord& plain, CompactAccessRecord& compact) {
  compact.epochSec = plain.epochSec;
  compact.tagId = plain.tagId;
}

void DiagEventLogger::decodeAccessRecord(const CompactAccessRecord& compact, AccessLogRecord& plain) {
  plain.epochSec = compact.epochSec;
  plain.tagId = compact.tagId;
  resolveDeviceInfo(compact.tagId, plain.who, sizeof(plain.who),
                    plain.type, sizeof(plain.type),
                    plain.subtitle, sizeof(plain.subtitle));
}

// ============================================================================
// STRING CONVERSION HELPERS
// ============================================================================

const char* DiagEventLogger::levelToString(DoorEventLevel lvl) {
  switch (lvl) {
    case DoorEventLevel::Success: return "success";
    case DoorEventLevel::Warn:    return "warn";
    case DoorEventLevel::Danger:  return "danger";
    case DoorEventLevel::Info:
    default:                      return "info";
  }
}

const char* DiagEventLogger::eventCodeToString(DoorEventCode code) {
  switch (code) {
    case DoorEventCode::StateClosed: return "STATE_CLOSE";
    case DoorEventCode::MagClosed:   return "MAG_CLOSE";
    case DoorEventCode::MagOpen:     return "MAG_OPEN";
    case DoorEventCode::LimitClosed: return "LIMIT_CLOSE";
    case DoorEventCode::LimitOpen:   return "LIMIT_OPEN";
    case DoorEventCode::ManualOpen:  return "MANUAL_OPEN";
    case DoorEventCode::ManualStop:  return "MANUAL_STOP";
    case DoorEventCode::ManualClose: return "MANUAL_CLOSE";
    case DoorEventCode::AutoClose:   return "AUTO_CLOSE";
    case DoorEventCode::CloseStep1:  return "CLOSE_STEP1";
    case DoorEventCode::Reversal:    return "REVERSAL";
    case DoorEventCode::DoubleClose: return "DOUBLE_CLOSE";
    case DoorEventCode::RetryClose:  return "RETRY_CLOSE";
    case DoorEventCode::RetryFail:   return "RETRY_FAIL";
    case DoorEventCode::Failsafe:    return "FAILSAFE";
    case DoorEventCode::PedestrianStop: return "PEDESTRIAN_STOP";
    case DoorEventCode::BeamCut:     return "BEAM_CUT";
    case DoorEventCode::SafeAbort:   return "SAFE_ABORT";
    case DoorEventCode::PassArm:     return "PASS_ARM";
    case DoorEventCode::SafeClose:   return "SAFE_CLOSE";
    case DoorEventCode::CloseBlock:  return "CLOSE_BLOCK";
    case DoorEventCode::PassSkip:    return "PASS_SKIP";
    case DoorEventCode::TagOpen:     return "TAG_OPEN";
    case DoorEventCode::FallClose:   return "FALL_CLOSE";
    case DoorEventCode::RfOpen:      return "RF_OPEN";
    case DoorEventCode::RfClose:     return "RF_CLOSE";
    case DoorEventCode::RfHold:      return "RF_HOLD";
    case DoorEventCode::RfUnhold:    return "RF_UNHOLD";
    case DoorEventCode::RfUnknown:   return "RF_UNKN";
    default:                         return "INFO";
  }
}

DoorEventCode DiagEventLogger::stringToEventCode(const char* s) {
  if (!s) return DoorEventCode::None;
  if (strcmp(s, "STATE_CLOSE") == 0) return DoorEventCode::StateClosed;
  if (strcmp(s, "MAG_CLOSE") == 0)   return DoorEventCode::MagClosed;
  if (strcmp(s, "MAG_OPEN") == 0)     return DoorEventCode::MagOpen;
  if (strcmp(s, "LIMIT_CLOSE") == 0) return DoorEventCode::LimitClosed;
  if (strcmp(s, "LIMIT_OPEN") == 0)   return DoorEventCode::LimitOpen;
  if (strcmp(s, "MANUAL_OPEN") == 0)  return DoorEventCode::ManualOpen;
  if (strcmp(s, "MANUAL_STOP") == 0)  return DoorEventCode::ManualStop;
  if (strcmp(s, "MANUAL_CLOSE") == 0) return DoorEventCode::ManualClose;
  if (strcmp(s, "AUTO_CLOSE") == 0)   return DoorEventCode::AutoClose;
  if (strcmp(s, "CLOSE_STEP1") == 0)  return DoorEventCode::CloseStep1;
  if (strcmp(s, "REVERSAL") == 0)    return DoorEventCode::Reversal;
  if (strcmp(s, "DOUBLE_CLOSE") == 0) return DoorEventCode::DoubleClose;
  if (strcmp(s, "RETRY_CLOSE") == 0)  return DoorEventCode::RetryClose;
  if (strcmp(s, "RETRY_FAIL") == 0)   return DoorEventCode::RetryFail;
  if (strcmp(s, "FAILSAFE") == 0)    return DoorEventCode::Failsafe;
  if (strcmp(s, "PEDESTRIAN_STOP") == 0) return DoorEventCode::PedestrianStop;
  if (strcmp(s, "BEAM_CUT") == 0)     return DoorEventCode::BeamCut;
  if (strcmp(s, "SAFE_ABORT") == 0)   return DoorEventCode::SafeAbort;
  if (strcmp(s, "PASS_ARM") == 0)     return DoorEventCode::PassArm;
  if (strcmp(s, "SAFE_CLOSE") == 0)   return DoorEventCode::SafeClose;
  if (strcmp(s, "CLOSE_BLOCK") == 0)  return DoorEventCode::CloseBlock;
  if (strcmp(s, "PASS_SKIP") == 0)    return DoorEventCode::PassSkip;
  if (strcmp(s, "TAG_OPEN") == 0)     return DoorEventCode::TagOpen;
  if (strcmp(s, "FALL_CLOSE") == 0)   return DoorEventCode::FallClose;
  if (strcmp(s, "RF_OPEN") == 0)      return DoorEventCode::RfOpen;
  if (strcmp(s, "RF_CLOSE") == 0)     return DoorEventCode::RfClose;
  if (strcmp(s, "RF_HOLD") == 0)      return DoorEventCode::RfHold;
  if (strcmp(s, "RF_UNHOLD") == 0)    return DoorEventCode::RfUnhold;
  if (strcmp(s, "RF_UNKN") == 0)      return DoorEventCode::RfUnknown;
  return DoorEventCode::None;
}

const char* DiagEventLogger::reasonToString(DiagReasonCode code) {
  switch (code) {
    case DiagReasonCode::NormalCycle:         return "NORMAL_CYCLE";
    case DiagReasonCode::TravelTimeout:       return "TRAVEL_TIMEOUT";
    case DiagReasonCode::MotorStallWarn:      return "MOTOR_STALL_WARN";
    case DiagReasonCode::SrcTagRssi:          return "SRC_TAG";
    case DiagReasonCode::SrcRfBtn:            return "SRC_RF_BTN";
    case DiagReasonCode::SrcRfHold:           return "SRC_RF_HOLD";
    case DiagReasonCode::SrcWebUi:            return "SRC_WEB";
    case DiagReasonCode::SrcBlynk:            return "SRC_BLYNK";
    case DiagReasonCode::SrcSchedule:         return "SRC_SCHEDULE";
    case DiagReasonCode::SrcWebhook:          return "SRC_WEBHOOK";
    case DiagReasonCode::ObstacleDuringClose: return "OBSTACLE_DURING_CLOSE";
    case DiagReasonCode::PassageCrossed:      return "PASSAGE_CROSSED";
    case DiagReasonCode::BeamStillBlocked:    return "BEAM_BLOCKED";
    case DiagReasonCode::PassSkipSafeMode:    return "SKIP_SAFEMODE";
    case DiagReasonCode::PassSkipManual:      return "SKIP_MANUAL";
    case DiagReasonCode::PassSkipClosed:      return "SKIP_CLOSED";
    case DiagReasonCode::MidwayDirectionChange: return "MIDWAY_DIRECTION_CHANGE";
    case DiagReasonCode::AutoCloseFallback:   return "AUTOCLOSE_FALLBACK";
    case DiagReasonCode::AutoCloseSafety:     return "AUTOCLOSE_SAFETY";
    case DiagReasonCode::FailsafeOvertime:    return "FAILSAFE_3MIN";
    case DiagReasonCode::RetryCloseUnlocked:  return "RETRY_UNLOCKED";
    case DiagReasonCode::RetryExhausted:      return "RETRY_EXHAUSTED";
    case DiagReasonCode::DeadbandInterlock:   return "DEADBAND_WAIT";
    case DiagReasonCode::TagSuppressedHold:   return "TAG_SUPPRESSED";
    case DiagReasonCode::ReversalInterlock:   return "REVERSAL_INTERLOCK";
    default:                                  return "";
  }
}

DoorEventLevel DiagEventLogger::stringToLevel(const char* s) {
  if (!s) return DoorEventLevel::Info;
  if (strcmp(s, "success") == 0) return DoorEventLevel::Success;
  if (strcmp(s, "warn") == 0)    return DoorEventLevel::Warn;
  if (strcmp(s, "danger") == 0)  return DoorEventLevel::Danger;
  return DoorEventLevel::Info;
}

// ============================================================================
// FORMATTING HELPERS
// ============================================================================

void DiagEventLogger::formatEventTime(const CompactDiagEvent& ev, char* out, size_t outSize) {
  if (ev.flags & FLAG_EPOCH_TIME) {
    time_t raw = (time_t)ev.timestamp;
    struct tm ti;
    localtime_r(&raw, &ti);
    snprintf(out, outSize, "%02d:%02d:%02d", ti.tm_hour, ti.tm_min, ti.tm_sec);
  } else {
    uint32_t sec = ev.timestamp;
    snprintf(out, outSize, "+%lum%02lus", (unsigned long)(sec / 60), (unsigned long)(sec % 60));
  }
}

uint8_t DiagEventLogger::captureCurrentFlags(DoorEventLevel level) {
  uint8_t f = ((uint8_t)level & LEVEL_MASK);
  if (gIsManualOpenState) f |= FLAG_MANUAL;
  if (gSafeMode)          f |= FLAG_SAFEMODE;
  if (isDoorReversing())  f |= FLAG_REVERSAL;
  if (gAutoCloseArmed || gSafetyCloseAtMs != 0 || gPendingCloseAtMs != 0) f |= FLAG_AUTOCLOSE_ARMED;
  return f;
}

uint16_t DiagEventLogger::encodeTagId(const char* entity) {
  if (!entity || !entity[0]) return ACCESS_TAG_UNKNOWN;
  if (strcmp(entity, "WEB") == 0) return ACCESS_TAG_WEB;
  if (strcmp(entity, "BLYNK") == 0) return ACCESS_TAG_BLYNK;
  if (strcmp(entity, "SCHEDULE") == 0) return ACCESS_TAG_SCHEDULE;
  if (strcmp(entity, "BOOT_BTN") == 0) return ACCESS_TAG_BOOT_BTN;
  if (strcmp(entity, "SAFETY") == 0 || strcmp(entity, "MAG") == 0) return ACCESS_TAG_EXTERNAL;

  // Check if it's RF Remote (e.g. "REMOTE", "RF-1", "Remote #2")
  if (strncasecmp(entity, "REMOTE", 6) == 0 || strncasecmp(entity, "RF", 2) == 0) {
    int rNum = 1;
    const char* p = entity;
    while (*p && !isdigit(*p)) p++;
    if (*p) rNum = atoi(p);
    if (rNum < 1) rNum = 1;
    if (rNum > 80) rNum = 80;
    return (uint16_t)(ACCESS_TAG_RF_BASE + rNum);
  }

  // Search registered tags (by tag ID or tag Name)
  const auto& tags = gTagRegistry.tags();
  for (size_t i = 0; i < tags.size() && i < ACCESS_TAG_RF_BASE; ++i) {
    if (tags[i].id.equalsIgnoreCase(entity) || tags[i].name.equalsIgnoreCase(entity)) {
      return (uint16_t)(i + 1);
    }
  }

  // If entity matches tagId numeric string
  for (size_t i = 0; i < tags.size() && i < ACCESS_TAG_RF_BASE; ++i) {
    if (String(tags[i].tagId) == entity) {
      return (uint16_t)(i + 1);
    }
  }

  return ACCESS_TAG_UNKNOWN;
}

void DiagEventLogger::resolveDeviceInfo(uint16_t tagId, char* outName, size_t outNameSize,
                                        char* outType, size_t outTypeSize,
                                        char* outSub, size_t outSubSize) {
  if (!outName || outNameSize == 0) return;
  outName[0] = '\0';
  if (outType && outTypeSize > 0) outType[0] = '\0';
  if (outSub && outSubSize > 0) outSub[0] = '\0';

  if (tagId == ACCESS_TAG_WEB) {
    copyUtf8Safe(outName, "Web Console", outNameSize);
    if (outType) copyUtf8Safe(outType, "web", outTypeSize);
    if (outSub) copyUtf8Safe(outSub, "เว็บคอนโซล", outSubSize);
    return;
  }
  if (tagId == ACCESS_TAG_BLYNK) {
    copyUtf8Safe(outName, "Blynk App", outNameSize);
    if (outType) copyUtf8Safe(outType, "app", outTypeSize);
    if (outSub) copyUtf8Safe(outSub, "แอปมือถือ", outSubSize);
    return;
  }
  if (tagId == ACCESS_TAG_SCHEDULE) {
    copyUtf8Safe(outName, "ตารางเวลา", outNameSize);
    if (outType) copyUtf8Safe(outType, "schedule", outTypeSize);
    if (outSub) copyUtf8Safe(outSub, "ตั้งเวลาอัตโนมัติ", outSubSize);
    return;
  }
  if (tagId == ACCESS_TAG_BOOT_BTN) {
    copyUtf8Safe(outName, "ปุ่มกดหน้าตู้", outNameSize);
    if (outType) copyUtf8Safe(outType, "btn", outTypeSize);
    if (outSub) copyUtf8Safe(outSub, "สวิตช์หน้าตู้", outSubSize);
    return;
  }
  if (tagId == ACCESS_TAG_EXTERNAL) {
    copyUtf8Safe(outName, "สวิตช์ภายนอก", outNameSize);
    if (outType) copyUtf8Safe(outType, "ext", outTypeSize);
    if (outSub) copyUtf8Safe(outSub, "เซ็นเซอร์ภายนอก", outSubSize);
    return;
  }

  // RF Remotes
  if (tagId > ACCESS_TAG_RF_BASE && tagId <= ACCESS_TAG_RF_BASE + 80) {
    snprintf(outName, outNameSize, "รีโมท RF #%u", tagId - ACCESS_TAG_RF_BASE);
    if (outType) copyUtf8Safe(outType, "rf", outTypeSize);
    if (outSub) copyUtf8Safe(outSub, "รีโมทพกพา", outSubSize);
    return;
  }

  // Registered tags in TagRegistry
  if (tagId >= 1 && tagId <= ACCESS_TAG_RF_BASE) {
    const size_t idx = tagId - 1;
    const auto& tags = gTagRegistry.tags();
    if (idx < tags.size()) {
      const auto& t = tags[idx];
      const char* nameStr = !t.name.isEmpty() ? t.name.c_str() : t.id.c_str();
      copyUtf8Safe(outName, nameStr, outNameSize);

      String nLower = String(nameStr);
      nLower.toLowerCase();

      if (!t.icon.isEmpty() && t.icon != "auto") {
        if (outType) copyUtf8Safe(outType, t.icon.c_str(), outTypeSize);
        if (outSub) {
          if (t.icon == "suv") copyUtf8Safe(outSub, "รถยนต์ SUV", outSubSize);
          else if (t.icon == "sedan") copyUtf8Safe(outSub, "รถเก๋ง", outSubSize);
          else if (t.icon == "pickup") copyUtf8Safe(outSub, "รถกระบะ", outSubSize);
          else if (t.icon == "bike") copyUtf8Safe(outSub, "รถมอเตอร์ไซค์", outSubSize);
          else if (t.icon == "scooter") copyUtf8Safe(outSub, "รถสกู๊ตเตอร์", outSubSize);
          else if (t.icon == "van") copyUtf8Safe(outSub, "รถตู้", outSubSize);
          else if (t.icon == "rf") copyUtf8Safe(outSub, "รีโมท RF", outSubSize);
          else if (t.icon == "bicycle") copyUtf8Safe(outSub, "รถจักรยาน", outSubSize);
          else if (t.icon == "person") copyUtf8Safe(outSub, "คนเดิน", outSubSize);
          else copyUtf8Safe(outSub, "ยานพาหนะ", outSubSize);
        }
        return;
      }

      if (TagRegistry::isRf(t)) {
        if (outType) copyUtf8Safe(outType, "rf", outTypeSize);
        if (outSub) copyUtf8Safe(outSub, "รีโมทพกพา", outSubSize);
      } else if (nLower.indexOf("มอไซ") >= 0 || nLower.indexOf("มอเตอร์ไซค์") >= 0 ||
                 nLower.indexOf("bike") >= 0 || nLower.indexOf("wave") >= 0 ||
                 nLower.indexOf("scooter") >= 0 || nLower.indexOf("pcx") >= 0 ||
                 nLower.indexOf("adv") >= 0 || nLower.indexOf("forza") >= 0 ||
                 nLower.indexOf("click") >= 0) {
        if (outType) copyUtf8Safe(outType, "bike", outTypeSize);
        if (outSub) copyUtf8Safe(outSub, "รถมอเตอร์ไซค์", outSubSize);
      } else {
        if (outType) copyUtf8Safe(outType, "car", outTypeSize);
        if (nLower.indexOf("everest") >= 0 || nLower.indexOf("suv") >= 0 || nLower.indexOf("fortuner") >= 0) {
          if (outSub) copyUtf8Safe(outSub, "รถยนต์ SUV", outSubSize);
        } else if (nLower.indexOf("dmax") >= 0 || nLower.indexOf("revo") >= 0 || nLower.indexOf("vigo") >= 0 || nLower.indexOf("truck") >= 0) {
          if (outSub) copyUtf8Safe(outSub, "รถกระบะ", outSubSize);
        } else {
          if (outSub) copyUtf8Safe(outSub, "รถยนต์ส่วนบุคคล", outSubSize);
        }
      }
      return;
    }
  }

  snprintf(outName, outNameSize, "แท็ก #%u", tagId);
  if (outType) copyUtf8Safe(outType, "rf", outTypeSize);
  if (outSub) copyUtf8Safe(outSub, "อุปกรณ์พกพา", outSubSize);
}

void DiagEventLogger::resolveTagString(uint16_t tagId, char* out, size_t outSize) {
  char dummyType[16], dummySub[32];
  resolveDeviceInfo(tagId, out, outSize, dummyType, sizeof(dummyType), dummySub, sizeof(dummySub));
}

String DiagEventLogger::formatEventMessage(const CompactDiagEvent& ev) {
  char entityBuf[32];
  resolveTagString(ev.entityRef, entityBuf, sizeof(entityBuf));
  const char* entity = entityBuf;
  switch (ev.code) {
    case DoorEventCode::StateClosed:
      if (ev.durationMs > 0) {
        char buf[128];
        snprintf(buf, sizeof(buf), "[สถานะ] ประตูปิดสนิท (เวลาวิ่งปิด %.1fs)", (float)ev.durationMs / 1000.0f);
        return String(buf);
      }
      return "[สถานะ] ประตูปิดสนิท (คืนโหมด Auto)";

    case DoorEventCode::MagClosed:
      if (ev.durationMs > 0) {
        char buf[128];
        snprintf(buf, sizeof(buf), "[แม่เหล็ก] ตรวจพบประตูปิดสนิท (เวลาวิ่งปิด %.1fs)", (float)ev.durationMs / 1000.0f);
        return String(buf);
      }
      return "[แม่เหล็ก] ตรวจพบประตูปิดสนิท (ซิงค์สถานะ CLOSED)";

    case DoorEventCode::MagOpen:
      if (strcmp(entity, "REMOTE") == 0) {
        if (ev.durationMs > 0) {
          char buf[128];
          snprintf(buf, sizeof(buf), "[รีโมทประตู] ตรวจพบประตูเปิดออก (เวลาวิ่งเปิด %.1fs)", (float)ev.durationMs / 1000.0f);
          return String(buf);
        }
        return "[รีโมทประตู] ตรวจพบประตูเปิดออก (เปิดค้างโหมดมือ)";
      }
      if (ev.durationMs > 0) {
        char buf[128];
        snprintf(buf, sizeof(buf), "[แม่เหล็ก] ตรวจพบประตูเปิดออก (เวลาวิ่งเปิด %.1fs)", (float)ev.durationMs / 1000.0f);
        return String(buf);
      }
      return "[แม่เหล็ก] ตรวจพบประตูเปิดออก (ซิงค์สถานะ OPEN)";

    case DoorEventCode::LimitClosed:
      if (ev.durationMs > 0) {
        char buf[128];
        snprintf(buf, sizeof(buf), "[ลิมิตสวิตช์] ชนลิมิตปิดสนิท (เวลาวิ่งปิด %.1fs)", (float)ev.durationMs / 1000.0f);
        return String(buf);
      }
      return "[ลิมิตสวิตช์] ชนลิมิตปิดสนิท (ซิงค์สถานะ CLOSED)";

    case DoorEventCode::LimitOpen:
      if (strcmp(entity, "REMOTE") == 0) {
        if (ev.durationMs > 0) {
          char buf[128];
          snprintf(buf, sizeof(buf), "[รีโมทประตู] ชนลิมิตเปิดสุด (เวลาวิ่งเปิด %.1fs)", (float)ev.durationMs / 1000.0f);
          return String(buf);
        }
        return "[รีโมทประตู] ชนลิมิตเปิดสุด (เปิดค้างโหมดมือ)";
      }
      if (ev.durationMs > 0) {
        char buf[128];
        snprintf(buf, sizeof(buf), "[ลิมิตสวิตช์] ชนลิมิตเปิดสุด (เวลาวิ่งเปิด %.1fs)", (float)ev.durationMs / 1000.0f);
        return String(buf);
      }
      return "[ลิมิตสวิตช์] ชนลิมิตเปิดสุด (ซิงค์สถานะ OPEN)";

    case DoorEventCode::ManualOpen:
      return "[คำสั่งมือ] สั่งเปิด -> GPIO 6 = HIGH (500 ms)";

    case DoorEventCode::ManualStop:
      return "[คำสั่งมือ] สั่งหยุด -> GPIO 6+7 = HIGH (1200 ms)";

    case DoorEventCode::ManualClose:
      return "[คำสั่งมือ] สั่งปิด -> GPIO 7 = HIGH (500 ms)";

    case DoorEventCode::AutoClose:
      return "[อัตโนมัติ] สั่งปิด -> GPIO 7 = HIGH (500 ms)";

    case DoorEventCode::CloseStep1:
      return (ev.flags & FLAG_MANUAL)
        ? "[คำสั่งมือ] สั่งปิด -> ยิง STOP (GPIO 6+7 = HIGH 1200ms) -> รอ Deadband 1.2s"
        : "[อัตโนมัติ] สั่งปิด -> ยิง STOP (GPIO 6+7 = HIGH 1200ms) -> รอ Deadband 1.2s";

    case DoorEventCode::Reversal:
      if (ev.reason == DiagReasonCode::MidwayDirectionChange || ev.durationMs == 2) {
        return "[สลับทิศทาง] ประตูกำลังเปิด -> สลับปิดกลางคัน (ยิง STOP 1200ms -> Deadband 1.2s)";
      }
      if (ev.reason == DiagReasonCode::ObstacleDuringClose || ev.durationMs == 1) {
        return "[ดีดกลับฉุกเฉิน] ลำแสงถูกตัด -> สลับเปิดทันที (ยิง STOP 1200ms -> Deadband 1.2s)";
      }
      return "[สลับทิศทาง] สลับทิศทางมอเตอร์ -> ยิง STOP (1200ms) -> Deadband 1.2s";

    case DoorEventCode::DoubleClose:
      return "[คำสั่งปิดย้ำ] ยิงปิดรอบ 2 -> GPIO 7 = HIGH (500 ms)";

    case DoorEventCode::RetryClose:
      return String("[ลองปิดซ้ำ] รอบที่ ") + (ev.durationMs ? ev.durationMs : 1) + " -> ยิงปิด GPIO 7 = HIGH (500 ms)";

    case DoorEventCode::RetryFail:
      return String("[ลองปิดซ้ำ] ล้มเหลวครบ ") + (ev.durationMs ? ev.durationMs : 3) + " รอบ: ประตูปิดไม่สนิท";

    case DoorEventCode::Failsafe:
      return "[เซฟตี้ฉุกเฉิน] เปิดค้างเกิน 3 นาที -> สั่งปิด GPIO 7 = HIGH (500 ms)";

    case DoorEventCode::PedestrianStop: {
      uint32_t openSec = ev.durationMs;
      uint32_t pauseSec = ev.metricVal > 0 ? (uint32_t)ev.metricVal : 0;
      char buf[128];
      snprintf(buf, sizeof(buf), "[เปิดช่องรถเล็ก] วิ่งเปิด %lu วิ -> สั่งหยุดรอ %lu วิ ก่อนปิด", (unsigned long)openSec, (unsigned long)pauseSec);
      return String(buf);
    }

    case DoorEventCode::BeamCut: {
      uint8_t st = ((ev.sensorBits & DOOR_STATE_MASK) >> DOOR_STATE_SHIFT);
      const char* stStr = (st == 1) ? "open" : (st == 2) ? "closed" : (st == 3) ? "stopped" : (st == 4) ? "opening" : (st == 5) ? "closing" : "unknown";
      return String("[เซ็นเซอร์] ลำแสงถูกตัด (สถานะ: ") + stStr + ")";
    }

    case DoorEventCode::SafeAbort:
      return "[ฉุกเฉิน] ลำแสงถูกตัดขณะปิด -> สั่งเปิด GPIO 6 = HIGH (500 ms)";

    case DoorEventCode::PassArm:
      return String("[เซ็นเซอร์] ลำแสงโล่ง -> นับถอยหลังปิดใน ") + ev.durationMs + " ms";

    case DoorEventCode::SafeClose:
      return "[เซ็นเซอร์] ครบเวลากันหนีบ -> สั่งปิด GPIO 7 = HIGH (500 ms)";

    case DoorEventCode::CloseBlock:
      return (ev.durationMs == 1 || ev.reason == DiagReasonCode::BeamStillBlocked)
        ? "[อัตโนมัติ] ถึงเวลาปิดแต่ถูกระงับ: มีสิ่งกีดขวาง"
        : "[ความปลอดภัย] ปฏิเสธการปิด: มีสิ่งกีดขวาง";

    case DoorEventCode::PassSkip:
      if (ev.reason == DiagReasonCode::PassSkipSafeMode || ev.durationMs == 1) return "[เซ็นเซอร์] ลำแสงโล่ง แต่ข้ามการปิด (อยู่ใน Safe Mode)";
      if (ev.reason == DiagReasonCode::PassSkipClosed || ev.durationMs == 2)   return "[เซ็นเซอร์] ลำแสงโล่ง แต่ข้ามการปิด (ปิดโหมดอัตโนมัติ)";
      if (ev.reason == DiagReasonCode::PassSkipManual || ev.durationMs == 3)   return "[เซ็นเซอร์] ลำแสงโล่ง แต่ข้ามการปิด (เปิดค้างโหมดมือ / รีโมทประตู)";
      return "[เซ็นเซอร์] ลำแสงโล่ง แต่ข้ามการปิด (ระบบคิดว่าประตูปิดอยู่)";

    case DoorEventCode::TagOpen: {
      String desc = entity[0] ? String(entity) : "active";
      if (ev.metricVal < 0) {
        desc += " " + String(ev.metricVal) + "dBm";
      }
      return String("[แท็ก] รถเข้าใกล้ -> สั่งเปิด GPIO 6 = HIGH (500 ms) (") + desc + ")";
    }

    case DoorEventCode::FallClose:
      return "[อัตโนมัติ] ครบเวลานับถอยหลังปิด (รอบเดียวถ้วน) -> สั่งปิด";

    case DoorEventCode::RfOpen:
      return String("[รีโมท] กดปุ่มเปิด -> สั่งเปิด GPIO 6 = HIGH (500 ms) (") + (entity[0] ? entity : "RF") + ")";

    case DoorEventCode::RfClose:
      return (ev.durationMs == 1)
        ? String("[รีโมท] สั่งปิดกลางทาง -> GPIO 7 = HIGH (500 ms) (") + (entity[0] ? entity : "RF") + ")"
        : String("[รีโมท] กดปุ่มปิด -> สั่งปิด GPIO 7 = HIGH (500 ms) (") + (entity[0] ? entity : "RF") + ")";

    case DoorEventCode::RfHold:
      return (ev.durationMs == 4)
        ? String("[รีโมท] กดค้าง 4 วิ -> ล็อกเปิดค้าง (Buzzer 3 ครั้ง) [") + (entity[0] ? entity : "RF") + "]"
        : String("[รีโมท] กดปุ่มเปิดค้าง -> ล็อกเปิดค้าง (Buzzer 2 ครั้ง) [") + (entity[0] ? entity : "RF") + "]";

    case DoorEventCode::RfUnhold:
      return String("[รีโมท] กดค้าง 4 วิ -> ปลดล็อกและสั่งปิดประตู [") + (entity[0] ? entity : "RF") + "]";

    case DoorEventCode::RfUnknown:
      return String("[รีโมท] พบคลื่น RF แปลกปลอม (") + (entity[0] ? entity : "0x00") + ") ยังไม่จับคู่";

    default:
      return entity[0] ? String(entity) : "Event";
  }
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void DiagEventLogger::copyUtf8Safe(char* dst, const char* src, size_t dstSize) {
  if (!dst || dstSize == 0) return;
  if (!src || !src[0]) {
    dst[0] = '\0';
    return;
  }
  const size_t maxLen = dstSize - 1;
  size_t i = 0;
  size_t validLen = 0;
  while (src[i] && i < maxLen) {
    const uint8_t c = (uint8_t)src[i];
    size_t charLen = 1;
    if ((c & 0x80) == 0) {
      charLen = 1;
    } else if ((c & 0xE0) == 0xC0) {
      charLen = 2;
    } else if ((c & 0xF0) == 0xE0) {
      charLen = 3;
    } else if ((c & 0xF8) == 0xF0) {
      charLen = 4;
    } else {
      break;
    }
    if (i + charLen > maxLen) {
      break;
    }
    i += charLen;
    validLen = i;
  }
  memcpy(dst, src, validLen);
  dst[validLen] = '\0';
}

uint8_t DiagEventLogger::captureCurrentSensorBits() {
  uint8_t bits = 0;
  if (gSafetySensorEnabled && gSafetySensorBlocked) {
    bits |= SENSOR_BIT_BEAM;
  }
  if (gMagSensorEnabled && gMagSensorStableClosed) {
    bits |= SENSOR_BIT_MAG_CLOSED;
  }
  uint8_t st = (uint8_t)gDoorState;
  bits |= ((st & 0x07) << DOOR_STATE_SHIFT);
  return bits;
}

// ============================================================================
// PRIMARY LOGGING API
// ============================================================================

void DiagEventLogger::logAccessEvent(uint16_t tagId) {
  uint32_t timeVal = 0;
  if (gTimeSynced) {
    time_t raw;
    time(&raw);
    timeVal = (uint32_t)raw;
  } else {
    timeVal = millis() / 1000;
  }

  CompactAccessRecord rec;
  rec.epochSec = timeVal;
  rec.tagId = tagId;

  portENTER_CRITICAL(&_mux);
  // Smart Debounce: if same tagId within 8 seconds, ignore repeat
  if (_count > 0) {
    size_t lastIdx = (_head + kMaxEvents - 1) % kMaxEvents;
    const CompactAccessRecord& last = _ringBuffer[lastIdx];
    if (last.tagId == rec.tagId && rec.epochSec >= last.epochSec && (rec.epochSec - last.epochSec) <= 8) {
      portEXIT_CRITICAL(&_mux);
      return;
    }
  }

  const bool isBufferFull = (_count >= kMaxEvents - 1);
  _ringBuffer[_head] = rec;
  _head = (_head + 1) % kMaxEvents;
  if (_count < kMaxEvents) {
    _count++;
  }
  _totalLogged++;
  portEXIT_CRITICAL(&_mux);

  if (isBufferFull) {
    triggerArchiveOnOverflow();
  }

  char whoBuf[48] = {0};
  resolveTagString(rec.tagId, whoBuf, sizeof(whoBuf));
  Serial.printf("[access_log][6B] Entry: tagId=%u who='%s' time=%lu\n",
                rec.tagId, whoBuf, (unsigned long)rec.epochSec);
}

void DiagEventLogger::logAccessEvent(AccessMethod method, AccessAction action, uint16_t tagId, int8_t rssi) {
  (void)method;
  (void)action;
  (void)rssi;
  logAccessEvent(tagId);
}

void DiagEventLogger::logDiagEvent(DoorEventCode code,
                                    DoorEventLevel level,
                                    DiagReasonCode reason,
                                    uint16_t durationMs,
                                    int16_t metricVal,
                                    const char* entity,
                                    uint8_t customSensorBits) {
  (void)level;
  (void)reason;
  (void)durationMs;
  (void)metricVal;
  (void)customSensorBits;

  // Filter exclusively for real door opening events
  if (code == DoorEventCode::TagOpen || code == DoorEventCode::RfOpen || code == DoorEventCode::RfHold) {
    logAccessEvent(encodeTagId(entity));
  } else if (code == DoorEventCode::ManualOpen) {
    if (!entity || !entity[0] || strcmp(entity, "UNKNOWN") == 0) return;
    if (strcmp(entity, "BLYNK") == 0) {
      logAccessEvent(ACCESS_TAG_BLYNK);
    } else if (strcmp(entity, "SCHEDULE") == 0) {
      logAccessEvent(ACCESS_TAG_SCHEDULE);
    } else if (strcmp(entity, "BOOT_BTN") == 0) {
      logAccessEvent(ACCESS_TAG_BOOT_BTN);
    } else {
      logAccessEvent(ACCESS_TAG_WEB);
    }
  } else if (code == DoorEventCode::MagOpen) {
    logAccessEvent(ACCESS_TAG_EXTERNAL);
  }
  // All other codes (BEAM_CUT, PASS_SKIP, PASS_ARM, SAFE_CLOSE, AUTO_CLOSE,
  // MAG_CLOSE, LIMIT_CLOSE, CLOSE_STEP1, DOUBLE_CLOSE, RETRY_CLOSE) are ignored
}

// ============================================================================
// CONVENIENCE OVERLOADS
// ============================================================================

void DiagEventLogger::logDoorEvent(DoorEventCode code, DoorEventLevel level, uint16_t param, const char* extra) {
  DiagReasonCode rc = DiagReasonCode::None;
  if (code == DoorEventCode::Reversal) {
    if (param == 1) rc = DiagReasonCode::ObstacleDuringClose;
    else if (param == 2) rc = DiagReasonCode::MidwayDirectionChange;
  }
  logDiagEvent(code, level, rc, param, 0, extra);
}

void DiagEventLogger::logDoorEvent(DoorEventCode code, DoorEventLevel level, uint16_t param, const String& extra) {
  logDoorEvent(code, level, param, extra.c_str());
}

void DiagEventLogger::logDoorEvent(const char* code, const char* level, const char* msg) {
  DoorEventCode c = stringToEventCode(code);
  DoorEventLevel lvl = stringToLevel(level);
  if (c != DoorEventCode::None) {
    logDoorEvent(c, lvl, 0, msg);
  } else {
    logDoorEvent(DoorEventCode::None, lvl, 0, code ? code : msg);
  }
}

void DiagEventLogger::logDoorEvent(const char* code, const char* level, const String& msg) {
  logDoorEvent(code, level, msg.c_str());
}

// ============================================================================
// RETRIEVAL API
// ============================================================================

std::vector<CompactAccessRecord> DiagEventLogger::getRecentCompactAccessRecords(size_t maxCount) {
  std::vector<CompactAccessRecord> list;
  portENTER_CRITICAL(&_mux);
  const size_t toCopy = (_count < maxCount) ? _count : maxCount;
  list.reserve(toCopy);
  for (size_t i = 0; i < toCopy; ++i) {
    size_t idx = (_head + kMaxEvents - 1 - i) % kMaxEvents;
    list.push_back(_ringBuffer[idx]);
  }
  portEXIT_CRITICAL(&_mux);
  return list;
}

std::vector<AccessLogRecord> DiagEventLogger::getRecentAccessRecords(size_t maxCount) {
  auto rawList = getRecentCompactAccessRecords(maxCount);
  std::vector<AccessLogRecord> list;
  list.reserve(rawList.size());
  for (const auto& c : rawList) {
    AccessLogRecord plain;
    decodeAccessRecord(c, plain);
    list.push_back(plain);
  }
  return list;
}

DoorEventLogEntry DiagEventLogger::formatAccessEntry(const CompactAccessRecord& rec) {
  AccessLogRecord plain;
  decodeAccessRecord(rec, plain);
  return formatAccessEntry(plain);
}

DoorEventLogEntry DiagEventLogger::formatAccessEntry(const AccessLogRecord& rec) {
  DoorEventLogEntry entry;
  memset(&entry, 0, sizeof(entry));

  entry.epochSec = rec.epochSec;
  entry.timestampMs = rec.epochSec * 1000;
  entry.flags = FLAG_EPOCH_TIME;

  time_t t = (time_t)rec.epochSec;
  struct tm tmInfo;
  localtime_r(&t, &tmInfo);
  snprintf(entry.timeStr, sizeof(entry.timeStr), "%02d:%02d:%02d",
           tmInfo.tm_hour, tmInfo.tm_min, tmInfo.tm_sec);

  copyUtf8Safe(entry.code, rec.type[0] ? rec.type : "rf", sizeof(entry.code));
  copyUtf8Safe(entry.level, "success", sizeof(entry.level));
  copyUtf8Safe(entry.message, rec.who[0] ? rec.who : "ไม่ระบุ", sizeof(entry.message));
  copyUtf8Safe(entry.entityId, rec.who[0] ? rec.who : "ไม่ระบุ", sizeof(entry.entityId));
  copyUtf8Safe(entry.reasonStr, rec.subtitle[0] ? rec.subtitle : "อุปกรณ์", sizeof(entry.reasonStr));

  return entry;
}

std::vector<CompactDiagEvent> DiagEventLogger::getRecentCompactDoorEvents(size_t maxCount) {
  auto records = getRecentAccessRecords(maxCount);
  std::vector<CompactDiagEvent> list;
  list.reserve(records.size());
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
    } else if (rec.tagId == ACCESS_TAG_EXTERNAL) {
      ev.code = DoorEventCode::MagOpen;
      ev.entityRef = (uint8_t)ENTITY_SAFETY;
    } else if (rec.tagId > ACCESS_TAG_RF_BASE && rec.tagId <= ACCESS_TAG_RF_BASE + 80) {
      ev.code = DoorEventCode::RfOpen;
      ev.entityRef = (uint8_t)(rec.tagId - ACCESS_TAG_RF_BASE);
    } else {
      ev.code = DoorEventCode::TagOpen;
      ev.entityRef = (uint8_t)(rec.tagId < 255 ? rec.tagId : 255);
    }
    list.push_back(ev);
  }
  return list;
}

DoorEventLogEntry DiagEventLogger::formatEventEntry(const CompactDiagEvent& ev) {
  AccessLogRecord rec;
  memset(&rec, 0, sizeof(rec));
  rec.epochSec = ev.timestamp;

  if (ev.entityRef == (uint8_t)ENTITY_WEB || ev.code == DoorEventCode::ManualOpen) {
    rec.tagId = ACCESS_TAG_WEB;
  } else if (ev.entityRef == (uint8_t)ENTITY_BLYNK) {
    rec.tagId = ACCESS_TAG_BLYNK;
  } else if (ev.entityRef == (uint8_t)ENTITY_SCHEDULE) {
    rec.tagId = ACCESS_TAG_SCHEDULE;
  } else if (ev.entityRef == (uint8_t)ENTITY_BOOT_BTN) {
    rec.tagId = ACCESS_TAG_BOOT_BTN;
  } else {
    rec.tagId = ev.entityRef;
    if (ev.code == DoorEventCode::RfHold || ev.code == DoorEventCode::RfOpen) {
      if (ev.entityRef > 0 && ev.entityRef <= 80) {
        rec.tagId = ACCESS_TAG_RF_BASE + ev.entityRef;
      }
    }
  }

  resolveDeviceInfo(rec.tagId, rec.who, sizeof(rec.who), rec.type, sizeof(rec.type), rec.subtitle, sizeof(rec.subtitle));
  return formatAccessEntry(rec);
}

std::vector<DoorEventLogEntry> DiagEventLogger::getRecentDoorEvents(size_t maxCount) {
  auto records = getRecentAccessRecords(maxCount);
  std::vector<DoorEventLogEntry> list;
  list.reserve(records.size());
  for (const auto& rec : records) {
    list.push_back(formatAccessEntry(rec));
  }
  return list;
}

// ============================================================================
// MANAGEMENT
// ============================================================================

void DiagEventLogger::clear() {
  portENTER_CRITICAL(&_mux);
  _head = 0;
  _count = 0;
  portEXIT_CRITICAL(&_mux);
  Serial.println("[event] cleared all door events");
}

uint32_t DiagEventLogger::totalLogged() {
  portENTER_CRITICAL(&_mux);
  const uint32_t count = _totalLogged;
  portEXIT_CRITICAL(&_mux);
  return count;
}
