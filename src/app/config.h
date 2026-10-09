#pragma once

#include <Arduino.h>

#ifndef AUTODOOR_FEATURE_LOCAL_PANEL
#define AUTODOOR_FEATURE_LOCAL_PANEL 0
#endif

#ifndef AUTODOOR_FEATURE_STATUS_LED
#define AUTODOOR_FEATURE_STATUS_LED 1
#endif

#ifndef AUTODOOR_FEATURE_SAFE_BUTTON_LED
#define AUTODOOR_FEATURE_SAFE_BUTTON_LED 1
#endif

#ifndef AUTODOOR_FEATURE_BUILTIN_MAG
#define AUTODOOR_FEATURE_BUILTIN_MAG 0
#endif

// Board control: idle/stop = both LOW; open = pin6 HIGH; close = pin7 HIGH
constexpr uint8_t kBoardOpenPin = 6;
constexpr uint8_t kBoardClosePin = 7;
constexpr uint8_t kBootBtnPin = 9;         // Primary BOOT button on GPIO 9 (Active LOW, ESP32-C3 hardware BOOT)
constexpr uint8_t kBootBtnAuxPin = 255;    // Disabled (Unused in revised hardware pinout)
constexpr uint8_t kBootBtnAltPin = 255;    // Disabled
constexpr uint8_t kOledSdaPin = 255;
constexpr uint8_t kOledSclPin = 255;
constexpr uint8_t kTagDetectLedPin = 255;  // Disabled (GPIO 9 dedicated exclusively to BOOT button)
constexpr uint8_t kDoorClosingBlinkPin = 255; // Disabled (Unused in revised hardware pinout)
constexpr uint8_t kBuzzerPin = 255;        // Disabled (Unused in revised hardware pinout)
constexpr uint8_t kRf433RxPin = 0;         // RF433 RX Data on GPIO 0
constexpr uint8_t kMagSensorPin = 3;       // External Magnetic Reed Switch on GPIO 3
constexpr uint8_t kBuiltinMagOpenPin = 255;  // Disabled (Unused in revised hardware pinout)
constexpr uint8_t kBuiltinMagClosePin = 255; // Disabled (Unused in revised hardware pinout)
constexpr uint8_t kSafetySensorPin = 2;    // Infrared Safety Beam Sensor on GPIO 2


constexpr uint8_t kDailyRestartHour = 3;
constexpr uint32_t kDailyRestartFallbackMs = 86400000UL;

constexpr const char* kApSsidPrefix = "AutoLink";
constexpr uint8_t kEspNowChannel = 9;
// ==============================================================================
// Wi-Fi / ESP-NOW TX Power Configuration (จุดปรับกำลังส่ง Wi-Fi จุดเดียวในระบบ)
// ==============================================================================
// ESP32-C3 Wi-Fi power range: 10.0 dBm (40) ถึง 19.5 dBm (78) (สเต็ปละ 0.25 dBm)
//
// ตารางอ้างอิงค่า dBm:
//   19.5 dBm -> ค่า 78 (กำลังส่งสูงสุด)
//   19.0 dBm -> ค่า 76
//   18.5 dBm -> ค่า 74
//   18.0 dBm -> ค่า 72 (ค่าแนะนำ: สัญญาณแรง เสถียร บอร์ดไม่ร้อน)
//   17.0 dBm -> ค่า 68
//   15.0 dBm -> ค่า 60
//   13.0 dBm -> ค่า 52
//   10.0 dBm -> ค่า 40 (กำลังส่งต่ำสุด)
//
// ปรับค่า dBm ได้โดยตรงที่ kWifiTxPowerDbm บรรทัดนี้ (ระบบคำนวณสเต็ปให้อัตโนมัติ):
constexpr float kWifiTxPowerDbm = 18.0f;
constexpr int8_t kWifiTxPower = static_cast<int8_t>(kWifiTxPowerDbm * 4.0f + 0.5f);


constexpr const char* kFirmwareVersion = "1.0.0";
constexpr const char* kFirmwareBuildDate = __DATE__ " " __TIME__;

constexpr uint32_t kDefaultRelayPulseMs = 300;
// Double-pulse motor control: 2 presses separated by 0.3s (300ms)
constexpr uint32_t kDoublePulseGapMs = 300;       // Gap between pulse 1 and pulse 2 (0.3s)
constexpr uint32_t kDoublePulseWidthMs = 300;     // Duration of each pulse press (300ms)
constexpr uint32_t kStopPulseMs = 1200;          // Deadband reference for STOP before reverse
constexpr uint32_t kReverseDeadbandMs = 1200;    // Settling delay (1.2s) after STOP before reverse command (gear protection & motor MCU pause clearance)
constexpr uint32_t kReversePulseMs = 400;        // Pulse width for reverse command (CLOSE/OPEN) after STOP
constexpr uint32_t kLearnTimeoutMs = 30000;

// Default tag tuning (new tags / Web UI)
constexpr uint32_t kDefaultTagHoldMs = 20000;
constexpr uint32_t kDefaultRfTagHoldMs = 1000;
constexpr uint32_t kDefaultTagCloseDelayMs = 10000;
constexpr int8_t kDefaultTagRssiMin = -100;
constexpr int8_t kTagRssiHysteresisDb = 8;

// Standard preset (Web UI "มาตรฐาน")
constexpr uint32_t kStandardTagHoldMs = 2000;
constexpr uint32_t kStandardTagCloseDelayMs = 3000;
constexpr int8_t kStandardTagRssiMin = -95;

// RSSI below this after peak = car passed gate (close even if tag still active)
constexpr int8_t kDefaultTagPassRssi = -90;
constexpr uint32_t kDefaultTagPassCloseDelayMs = 5000;
constexpr uint32_t kDefaultTagSafetyCloseDelayMs = 3000;
constexpr uint32_t kStandardTagSafetyCloseDelayMs = 3000;
constexpr uint32_t kDefaultSafetyReverseDelayMs = 5000;

#if defined(AUTODOOR_VERBOSE_LOG) && AUTODOOR_VERBOSE_LOG
#define AD_LOG(stmt) \
  do {               \
    stmt;            \
  } while (0)
#else
#define AD_LOG(stmt) ((void)0)
#endif
