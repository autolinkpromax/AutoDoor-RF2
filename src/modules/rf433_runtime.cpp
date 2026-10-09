#include "rf433_runtime.h"

#include "app/config.h"
#include "app/globals.h"
#include "tags/TagRegistry.h"
#include "modules/door.h"
#include "hal/MotorActuator.h"
#include "modules/esp_now_runtime.h"
#include "core/AutoCloseEngine.h"

#include <SYN480R.h>

namespace {

QueueHandle_t gRfCmdQueue = NULL;

// ── Learn Mode candidate verification (NO mutex needed — local static state only) ──
// Returns true if candidate is verified (2+ identical burst frames within 500ms).
static bool verifyLearnCandidate(uint32_t value, uint32_t now) {
  static uint32_t sLearnCandidateValue = 0;
  static uint8_t  sLearnCandidateHits  = 0;
  static uint32_t sLearnCandidateLastMs = 0;

  if (value != sLearnCandidateValue || (uint32_t)(now - sLearnCandidateLastMs) > 500) {
    sLearnCandidateValue = value;
    sLearnCandidateHits  = 1;
    sLearnCandidateLastMs = now;
    AD_LOG(Serial.printf("[rf433] candidate candidate=0x%08X hit=1\n", value));
    return false;
  }

  sLearnCandidateHits++;
  sLearnCandidateLastMs = now;
  if (sLearnCandidateHits < 2) return false;

  // Verified — reset for next learn cycle
  sLearnCandidateValue = 0;
  sLearnCandidateHits  = 0;
  return true;
}



void processRfCode(uint32_t value) {
  if (!gSystemBootReady) return;
  if (value == 0 || value == 0x00FFFFFF || value == 0xFFFFFFFF) return;

  const uint32_t now = millis();

  // ── Learn Mode Path ──
  // Candidate verification runs OUTSIDE mutex — only acquires lock after confirmed.
  if (gLearn.active && (int32_t)(now - gLearn.deadlineMs) < 0) {
    if (!verifyLearnCandidate(value, now)) return;

    // Format pseudo-MAC: RF:00:XX:XX:XX:XX (using 4 bytes of 32-bit value)
    char macBuf[18];
    snprintf(macBuf, sizeof(macBuf), "RF:00:%02X:%02X:%02X:%02X",
             (uint8_t)(value >> 24),
             (uint8_t)(value >> 16),
             (uint8_t)(value >> 8),
             (uint8_t)value);
    String rfMac(macBuf);

    Serial.printf("[rf433] learning verified rx value=0x%08X mac=%s\n", value, rfMac.c_str());

    // Lock mutex only for the actual tag creation / NVS write
    TagStoreLock lock;
    Tag* tag = findTagByMacStr(rfMac);
    if (!tag) {
      tag = createTag("", rfMac, value);
      if (!tag) {
        tag = findTagByMacStr(rfMac);
      }
    }
    if (tag) {
      gLearn.lastMac = rfMac;
      gLearn.lastTagId = value;
      gLearn.createdTagId = tag->id;
      gLearn.active = false; // Stop learn mode immediately after learning 1 tag
      if (gSafeMode) {
        gSafeMode = false;
        Serial.println("[safe] RF Tag learned -> Auto-clearing Safe Mode");
        resumeEspNow();
      }
      saveTagsToPrefs();
      Serial.printf("[rf433] learned successfully id=%s mac=%s\n", tag->id.c_str(), rfMac.c_str());
    } else {
      Serial.printf("[rf433] failed to create tag for mac=%s\n", rfMac.c_str());
    }
    return;
  }

  // ── Normal Operation Path ──
  // Check registered tags directly under mutex:
  // For known paired remotes, we execute on the very first cleanly-decoded 24-bit frame!
  // (Requiring 2 identical frames within 120ms dropped valid clicks while the motor was moving
  //  due to brush/relay electrical hash on 433MHz).
  {
    // RAII lock to prevent mutex leak on any early-return path (Fix H3)
    TagStoreLock lock;
    if (!lock.isLocked()) {
      Serial.println("[rf433] WARN: mutex timeout 500ms — dropped RF frame");
      return;
    }

    Tag* tag = findTagByRfCode(value);
    if (tag) {
      if (!TagRegistry::isTagInSchedule(*tag)) {
        static uint32_t lastSchedWarnMs = 0;
        if ((uint32_t)(now - lastSchedWarnMs) >= 2000) {
          lastSchedWarnMs = now;
          Serial.printf("[rf433] Tag '%s' (0x%08X) ignored (outside active hours)\n",
                        tag->name.c_str(), value);
        }
        return;  // TagStoreLock destructor safely releases mutex
      }
      noteTagSeen(*tag, -50, now);

      static uint32_t holdPressStartMs = 0;
      static uint32_t lastFrameMs = 0;
      static uint32_t currentHoldCode = 0;
      static bool longPressTriggered = false;

      bool isNewPressStart = false;

      // Track continuous RF frame stream (button held down) - 350ms threshold between taps
      // Only default/Auto role (0) uses 4s long-press for hold-open toggle
      if (value == currentHoldCode && (uint32_t)(now - lastFrameMs) < 350) {
        lastFrameMs = now;
        if ((tag->rfRole == 0 || tag->rfRole == (uint8_t)RfButtonRole::AutoOpen) && !longPressTriggered && (uint32_t)(now - holdPressStartMs) >= 4000) {
          longPressTriggered = true;
          String tagName = tag->name;

          if (!gIsManualOpenState) {
            // Lock open (Hold Mode ON)
            requestDoorSlot("open", true, true, tag->id.c_str());
            logDoorEvent(DoorEventCode::RfHold, DoorEventLevel::Warn, 4, tagName.isEmpty() ? tag->id : tagName);
            Serial.println("[rf433] 4s long press -> MANUAL HOLD OPEN");
          } else {
            // Unlock & Close (Hold Mode OFF)
            requestDoorSlot("close", true, false, tag->id.c_str());
            logDoorEvent(DoorEventCode::RfUnhold, DoorEventLevel::Info, 0, tagName.isEmpty() ? tag->id : tagName);
            Serial.println("[rf433] 4s long press -> UNLOCK & CLOSE");
          }
          triggerImmediateAnnounce();
        }
      } else {
        // New press start
        currentHoldCode = value;
        holdPressStartMs = now;
        lastFrameMs = now;
        longPressTriggered = false;
        isNewPressStart = true;
      }

      static uint32_t lastExecGlobalMs = 0;
      static uint32_t lastExecTagValue = 0;
      // Execute command ONLY on new press start (300ms debounce between distinct presses).
      // Continuous frames while holding button (<350ms gap) accumulate towards long press and NEVER repeat commands.
      if (isNewPressStart && !longPressTriggered && (value != lastExecTagValue || (uint32_t)(now - lastExecGlobalMs) >= 300)) {
        lastExecGlobalMs = now;
        lastExecTagValue = value;

        String tagName = tag->name;
        Serial.printf("[rf433] tag seen: %s (0x%08X) role=%u\n", tagName.c_str(), value, (unsigned int)tag->rfRole);

        if (gSafeMode) {
          gSafeMode = false;
          Serial.println("[safe] Known RF remote pressed -> Auto-clearing Safe Mode");
          resumeEspNow();
        }

        const uint8_t role = tag->rfRole;
        const String eventName = tagName.isEmpty() ? tag->id : tagName;

        if (role == (uint8_t)RfButtonRole::HoldOpen) {
          // ── Dedicated Hold Open Button (Role 1): Exact same as Web UI Open (isManualHold = true) ──
          requestDoorSlot("open", true, true, tag->id.c_str());
          logDoorEvent(DoorEventCode::RfHold, DoorEventLevel::Warn, 0, eventName);
          AD_LOG(Serial.println("[rf433] button role HoldOpen -> request OPEN & HOLD (exact same as Web UI)"));
          triggerImmediateAnnounce();
        } else if (role == (uint8_t)RfButtonRole::Close) {
          // ── Dedicated Close Button (Role 2): Exact same as Web UI Close ──
          requestDoorSlot("close", true, false, tag->id.c_str());
          logDoorEvent(DoorEventCode::RfClose, DoorEventLevel::Success, 0, eventName);
          AD_LOG(Serial.println("[rf433] button role Close -> request CLOSE"));
          triggerImmediateAnnounce();
        } else if (role == (uint8_t)RfButtonRole::Stop) {
          // ── Dedicated Stop Button (Role 3): Exact same as Web UI Stop ──
          requestDoorSlot("stop", true, true, tag->id.c_str());
          logDoorEvent(DoorEventCode::ManualStop, DoorEventLevel::Warn, 0, eventName);
          AD_LOG(Serial.println("[rf433] button role Stop -> request STOP"));
          triggerImmediateAnnounce();
        } else if (role == (uint8_t)RfButtonRole::AutoOpen) {
          // ── Dedicated AutoOpen (Role 4): เปิดอัตโนมัติ (Auto / Open) ──
          // กดแล้วส่งสัญญาณเปิดประตูเสมอ (OPEN pulse) พร้อมรีเซ็ตตัวนับเวลาปิดอัตโนมัติตามค่าของ Tag
          const uint32_t delayMs = (tag->tagCloseDelayMs > 0) ? tag->tagCloseDelayMs : kDefaultTagCloseDelayMs;
          gPrevActiveDelayMaxMs = delayMs;
          gActiveTriggerTagId = tag->id;
          gDoorController.tagOpenDurationMsRef() = tag->tagOpenDurationMs;
          gIsManualOpenState = false;
          gAutoCloseEngine.resetCircuitBreaker();
          if (gTagAutoEnabled && !gSafeMode) {
            uint32_t totalDelayMs = delayMs;
            if (tag->tagOpenDurationMs > 0) {
              totalDelayMs += tag->tagOpenDurationMs;
            }
            gAutoCloseEngine.armPendingClose(now + totalDelayMs);
          }

          requestDoorSlot("open", true, false, tag->id.c_str());
          logDoorEvent(DoorEventCode::RfOpen, DoorEventLevel::Success, 0, eventName);
          Serial.printf("[rf433] button role AutoOpen -> sent OPEN pulse & reset countdown to %lu ms (%s)\n",
                        (unsigned long)delayMs, eventName.c_str());
          triggerImmediateAnnounce();
        } else {
          // ── Default Role 0: Auto / Smart Toggle Command ──
          if (gDoorState == DoorState::Closed || gDoorState == DoorState::Stopped) {
            const uint32_t delayMs = (tag->tagCloseDelayMs > 0) ? tag->tagCloseDelayMs : kDefaultTagCloseDelayMs;
            gPrevActiveDelayMaxMs = delayMs;
            gActiveTriggerTagId = tag->id;
            gDoorController.tagOpenDurationMsRef() = tag->tagOpenDurationMs;
            gIsManualOpenState = false;
            gAutoCloseEngine.resetCircuitBreaker();
            if (gTagAutoEnabled && !gSafeMode) {
              uint32_t totalDelayMs = delayMs;
              if (tag->tagOpenDurationMs > 0) {
                totalDelayMs += tag->tagOpenDurationMs;
              }
              gAutoCloseEngine.armPendingClose(now + totalDelayMs);
            }
            requestDoorSlot("open", true, false, tag->id.c_str());
            logDoorEvent(DoorEventCode::RfOpen, DoorEventLevel::Success, 0, eventName);
            AD_LOG(Serial.println("[rf433] remote button -> request OPEN (Auto)"));
          } else if (gDoorState == DoorState::Open) {
            requestDoorSlot("close", true, false, tag->id.c_str());
            logDoorEvent(DoorEventCode::RfClose, DoorEventLevel::Success, 0, eventName);
            AD_LOG(Serial.println("[rf433] remote button -> request CLOSE"));
          } else if (gDoorState == DoorState::Opening) {
            requestDoorSlot("close", true, false, tag->id.c_str());
            logDoorEvent(DoorEventCode::RfClose, DoorEventLevel::Warn, 1, eventName);
          } else if (gDoorState == DoorState::Closing) {
            requestDoorSlot("open", true, false, tag->id.c_str());
            logDoorEvent(DoorEventCode::RfOpen, DoorEventLevel::Warn, 1, eventName);
          }
        }
      }
    } else {
      // Unpaired / Unknown RF codes: ignore silently to prevent foreign RF noise interference
      AD_LOG(Serial.printf("[rf433] rx code: 0x%08X (unpaired/ignored)\n", value));
    }
  }  // TagStoreLock released here by RAII destructor
}

#include <esp_task_wdt.h>

void rf433Task(void* pvParameters) {
  (void)pvParameters;
  esp_task_wdt_add(NULL);
  uint32_t value = 0;
  for (;;) {
    if (xQueueReceive(gRfCmdQueue, &value, pdMS_TO_TICKS(1000)) == pdTRUE) {
      processRfCode(value);
    }
    esp_task_wdt_reset();
  }
}

}  // namespace

static DRAM_ATTR volatile uint32_t sLastIsrValue = 0;
static DRAM_ATTR volatile uint32_t sLastIsrMs = 0;

void IRAM_ATTR postRfCodeFromISR(uint32_t value) {
  if (gRfCmdQueue == NULL) return;

  const uint32_t nowMs = millis();
  // Filter out identical burst repeat frames arriving within 8ms
  // (allows all legitimate fast burst repeat frames through queue with high frequency)
  // (Reduced from 20ms: EV1527 frame takes ~15-20ms, 8ms filters only rapid echo reflections
  //  while passing through every legitimate repeat frame for faster responsiveness.)
  if (!gLearn.active && value == sLastIsrValue && (uint32_t)(nowMs - sLastIsrMs) < 8) {
    return;
  }
  sLastIsrValue = value;
  sLastIsrMs = nowMs;

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(gRfCmdQueue, &value, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

void initRf433() {
  initTagStoreMutex();
  gRfCmdQueue = xQueueCreate(32, sizeof(uint32_t));
  // Priority 1 (equal to loopTask and standard tasks to ensure sequential ordered dispatch), 6KB stack.
  // Increased from 4KB to 6KB for safety margin with String ops, logDoorEvent, and Serial.printf (Fix M1)
  xTaskCreate(rf433Task, "rf433Task", 6144, NULL, 1, NULL);
  vTaskDelay(pdMS_TO_TICKS(10));
  SYN480R::begin(kRf433RxPin);
  Serial.printf("[rf433] Priority 1 FreeRTOS task (6KB stack) & 32-item ISR queue initialized on pin %u\n", kRf433RxPin);
}

void stepRf433() {
  // Clear any residual available flag from library to avoid duplicate processing
  // (rf433Task handles the sequential queue processing directly from ISR)
  if (SYN480R::available()) {
    SYN480R::read();
  }
}
