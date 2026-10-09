#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "app/config.h"

/**
 * @brief Application Configuration Subsystem (Phase 7)
 *
 * Encapsulates persistent NVS settings, runtime operating modes,
 * sensor parameters, and door timing configurations.
 */
class AppConfig {
public:
  AppConfig() = default;

  /**
   * @brief Load all configuration settings from NVS Preferences.
   */
  void loadFromPrefs(Preferences& prefs);

  /**
   * @brief Save all current configuration settings to NVS Preferences.
   */
  void saveToPrefs(Preferences& prefs);

  // ── Operating Modes ──
  bool isSafeMode() const { return _safeMode; }
  void setSafeMode(bool val) { _safeMode = val; }
  bool& safeModeRef() { return _safeMode; }

  bool isSimMode() const { return _simMode; }
  void setSimMode(bool val) { _simMode = val; }
  bool& simModeRef() { return _simMode; }

  bool isMotorReverse() const { return _motorReverse; }
  void setMotorReverse(bool val) { _motorReverse = val; }
  bool& motorReverseRef() { return _motorReverse; }

  uint32_t relayPulseMs() const { return _relayPulseMs; }
  void setRelayPulseMs(uint32_t val) { _relayPulseMs = val; }
  uint32_t& relayPulseMsRef() { return _relayPulseMs; }

  // ── Wireless & ESP-NOW ──
  uint8_t espNowChannel() const { return _espNowChannel; }
  void setEspNowChannel(uint8_t val) { _espNowChannel = val; }
  uint8_t& espNowChannelRef() { return _espNowChannel; }

  bool isEspNowDisabled() const { return _espNowDisabled; }
  void setEspNowDisabled(bool val) { _espNowDisabled = val; }
  bool& espNowDisabledRef() { return _espNowDisabled; }

  // ── Device Identity & Tags ──
  const String& deviceName() const { return _deviceName; }
  void setDeviceName(const String& val) { _deviceName = val; }
  String& deviceNameRef() { return _deviceName; }

  bool isTagAutoEnabled() const { return _tagAutoEnabled; }
  void setTagAutoEnabled(bool val) { _tagAutoEnabled = val; }
  bool& tagAutoEnabledRef() { return _tagAutoEnabled; }

  bool isTagBuzzerEnabled() const { return _tagBuzzerEnabled; }
  void setTagBuzzerEnabled(bool val) { _tagBuzzerEnabled = val; }
  bool& tagBuzzerEnabledRef() { return _tagBuzzerEnabled; }

  // ── Magnetic Position Sensors ──
  bool isMagSensorEnabled() const { return _magSensorEnabled; }
  void setMagSensorEnabled(bool val) { _magSensorEnabled = val; }
  bool& magSensorEnabledRef() { return _magSensorEnabled; }

  uint8_t magSensorType() const { return _magSensorType; }
  void setMagSensorType(uint8_t val) { _magSensorType = val; }
  uint8_t& magSensorTypeRef() { return _magSensorType; }

  bool isBuiltinSensorsEnabled() const { return _builtinSensorsEnabled; }
  void setBuiltinSensorsEnabled(bool val) { _builtinSensorsEnabled = val; }
  bool& builtinSensorsEnabledRef() { return _builtinSensorsEnabled; }

  bool isBuiltinMagSwap() const { return _builtinMagSwap; }
  void setBuiltinMagSwap(bool val) { _builtinMagSwap = val; }
  bool& builtinMagSwapRef() { return _builtinMagSwap; }

  // ── Safety Infrared Beam ──
  bool isSafetySensorEnabled() const { return _safetySensorEnabled; }
  void setSafetySensorEnabled(bool val) { _safetySensorEnabled = val; }
  bool& safetySensorEnabledRef() { return _safetySensorEnabled; }

  uint8_t safetySensorType() const { return _safetySensorType; }
  void setSafetySensorType(uint8_t val) { _safetySensorType = val; }
  uint8_t& safetySensorTypeRef() { return _safetySensorType; }

  uint32_t safetyCloseDelayMs() const { return _safetyCloseDelayMs; }
  void setSafetyCloseDelayMs(uint32_t val) { _safetyCloseDelayMs = val; }
  uint32_t& safetyCloseDelayMsRef() { return _safetyCloseDelayMs; }

  uint32_t safetyReverseDelayMs() const { return _safetyReverseDelayMs; }
  void setSafetyReverseDelayMs(uint32_t val) { _safetyReverseDelayMs = val; }
  uint32_t& safetyReverseDelayMsRef() { return _safetyReverseDelayMs; }

  // ── Passage Close ──
  bool isRemotePassageClose() const { return _remotePassageClose; }
  void setRemotePassageClose(bool val) { _remotePassageClose = val; }
  bool& remotePassageCloseRef() { return _remotePassageClose; }

  uint32_t remotePassageDelayMs() const { return _remotePassageDelayMs; }
  void setRemotePassageDelayMs(uint32_t val) { _remotePassageDelayMs = val; }
  uint32_t& remotePassageDelayMsRef() { return _remotePassageDelayMs; }

  // ── Motor Timing & Auto-Retry ──
  uint32_t doorTransitionMs() const { return _doorTransitionMs; }
  void setDoorTransitionMs(uint32_t val) { _doorTransitionMs = val; }
  uint32_t& doorTransitionMsRef() { return _doorTransitionMs; }

  bool isAutoRetryCloseEnabled() const { return _autoRetryCloseEnabled; }
  void setAutoRetryCloseEnabled(bool val) { _autoRetryCloseEnabled = val; }
  bool& autoRetryCloseEnabledRef() { return _autoRetryCloseEnabled; }

  uint8_t maxCloseRetryCount() const { return _maxCloseRetryCount; }
  void setMaxCloseRetryCount(uint8_t val) { _maxCloseRetryCount = val; }
  uint8_t& maxCloseRetryCountRef() { return _maxCloseRetryCount; }

  uint32_t closeRetryDelayMs() const { return _closeRetryDelayMs; }
  void setCloseRetryDelayMs(uint32_t val) { _closeRetryDelayMs = val; }
  uint32_t& closeRetryDelayMsRef() { return _closeRetryDelayMs; }

  bool isDoubleClosePulseEnabled() const { return _doubleClosePulseEnabled; }
  void setDoubleClosePulseEnabled(bool val) { _doubleClosePulseEnabled = val; }
  bool& doubleClosePulseEnabledRef() { return _doubleClosePulseEnabled; }

  // ── Scheduled Auto Reboot ──
  bool isAutoRebootEnabled() const { return _autoRebootEnabled; }
  void setAutoRebootEnabled(bool val) { _autoRebootEnabled = val; }
  bool& autoRebootEnabledRef() { return _autoRebootEnabled; }

  uint8_t autoRebootHour() const { return _autoRebootHour; }
  void setAutoRebootHour(uint8_t val) { _autoRebootHour = val; }
  uint8_t& autoRebootHourRef() { return _autoRebootHour; }

  uint8_t autoRebootMinute() const { return _autoRebootMinute; }
  void setAutoRebootMinute(uint8_t val) { _autoRebootMinute = val; }
  uint8_t& autoRebootMinuteRef() { return _autoRebootMinute; }

private:
  bool _safeMode{false};
  bool _simMode{false};
  bool _motorReverse{false};
  uint32_t _relayPulseMs{kDefaultRelayPulseMs};
  uint8_t _espNowChannel{kEspNowChannel};
  bool _espNowDisabled{false};

  String _deviceName{"AutoDoor"};
  bool _tagAutoEnabled{true};
  bool _tagBuzzerEnabled{true};

  bool _magSensorEnabled{false};
  uint8_t _magSensorType{0};
  bool _builtinSensorsEnabled{false};
  bool _builtinMagSwap{false};

  bool _safetySensorEnabled{false};
  uint8_t _safetySensorType{0};
  uint32_t _safetyCloseDelayMs{3000};
  uint32_t _safetyReverseDelayMs{5000};

  bool _remotePassageClose{false};
  uint32_t _remotePassageDelayMs{5000};

  uint32_t _doorTransitionMs{5000};
  bool _autoRetryCloseEnabled{true};
  uint8_t _maxCloseRetryCount{3};
  uint32_t _closeRetryDelayMs{3000};
  bool _doubleClosePulseEnabled{true};

  bool _autoRebootEnabled{true};
  uint8_t _autoRebootHour{kDailyRestartHour};
  uint8_t _autoRebootMinute{0};
};

extern AppConfig gAppConfig;
