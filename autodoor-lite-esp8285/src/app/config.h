#pragma once

#include <Arduino.h>

constexpr uint8_t kPinBootApButton = 0;
constexpr uint8_t kPinDoorOpen = 12;
constexpr uint8_t kPinDoorClose = 13;
constexpr uint8_t kPinRf433Rx = 14;

constexpr uint8_t kDefaultEspNowChannel = 9;
constexpr uint16_t kDefaultRelayPulseMs = 500;

constexpr uint32_t kBootApWindowMs = 5000;
constexpr uint32_t kDailyRestartMs = 24UL * 60UL * 60UL * 1000UL;

constexpr uint16_t kTagDefaultHoldMs = 3000;
constexpr uint16_t kTagMinHoldMs = 500;
constexpr uint16_t kTagMaxHoldMs = 60000;

constexpr uint8_t kDefaultTagSensitivity = 1;
constexpr uint8_t kMinTagSensitivity = 1;
constexpr uint8_t kMaxTagSensitivity = 10;

constexpr uint16_t kDefaultTagCloseDelayMs = 3000;
constexpr uint16_t kMaxTagCloseDelayMs = 60000;

constexpr uint8_t kMaxTags = 24;
constexpr const char* kApSsidPrefix = "ADLITE-";
constexpr const char* kApPassword = "12345678";
