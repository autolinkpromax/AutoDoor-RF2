#include "AppConfig.h"

AppConfig gAppConfig;

// Reference aliases ensuring transparent access for any code using legacy globals
bool& gSafeMode = gAppConfig.safeModeRef();
bool& gSimMode = gAppConfig.simModeRef();
bool& gMotorReverse = gAppConfig.motorReverseRef();
uint32_t& gRelayPulseMs = gAppConfig.relayPulseMsRef();
uint8_t& gEspNowChannel = gAppConfig.espNowChannelRef();
bool& gEspNowDisabled = gAppConfig.espNowDisabledRef();

String& gDeviceName = gAppConfig.deviceNameRef();
bool& gTagAutoEnabled = gAppConfig.tagAutoEnabledRef();
bool& gTagBuzzerEnabled = gAppConfig.tagBuzzerEnabledRef();

bool& gMagSensorEnabled = gAppConfig.magSensorEnabledRef();
uint8_t& gMagSensorType = gAppConfig.magSensorTypeRef();
bool& gBuiltinSensorsEnabled = gAppConfig.builtinSensorsEnabledRef();
bool& gBuiltinMagSwap = gAppConfig.builtinMagSwapRef();

bool& gSafetySensorEnabled = gAppConfig.safetySensorEnabledRef();
uint8_t& gSafetySensorType = gAppConfig.safetySensorTypeRef();
uint32_t& gSafetyCloseDelayMs = gAppConfig.safetyCloseDelayMsRef();
uint32_t& gSafetyReverseDelayMs = gAppConfig.safetyReverseDelayMsRef();

bool& gRemotePassageClose = gAppConfig.remotePassageCloseRef();
uint32_t& gRemotePassageDelayMs = gAppConfig.remotePassageDelayMsRef();

uint32_t& gDoorTransitionMs = gAppConfig.doorTransitionMsRef();
bool& gAutoRetryCloseEnabled = gAppConfig.autoRetryCloseEnabledRef();
uint8_t& gMaxCloseRetryCount = gAppConfig.maxCloseRetryCountRef();
uint32_t& gCloseRetryDelayMs = gAppConfig.closeRetryDelayMsRef();
bool& gDoubleClosePulseEnabled = gAppConfig.doubleClosePulseEnabledRef();

bool& gAutoRebootEnabled = gAppConfig.autoRebootEnabledRef();
uint8_t& gAutoRebootHour = gAppConfig.autoRebootHourRef();
uint8_t& gAutoRebootMinute = gAppConfig.autoRebootMinuteRef();

void AppConfig::loadFromPrefs(Preferences& prefs) {
  _relayPulseMs = kDefaultRelayPulseMs;
  _simMode = prefs.getBool("simMode", false);
  _motorReverse = prefs.getBool("motorReverse", false);
  _deviceName = prefs.getString("deviceName", "AutoDoor");
  _espNowChannel = prefs.getUChar("espNowCh", kEspNowChannel);

  _tagAutoEnabled = prefs.getBool("tagAutoEnabled", true);
  _tagBuzzerEnabled = prefs.getBool("tagBuzzerEn", true);

  _magSensorEnabled = prefs.getBool("magSensEnabled", false);
#if AUTODOOR_FEATURE_BUILTIN_MAG
  _builtinSensorsEnabled = prefs.getBool("builtinSensEn", false);
  if (_builtinSensorsEnabled && _magSensorEnabled) {
    _magSensorEnabled = false;
    prefs.putBool("magSensEnabled", false);
  }
  _builtinMagSwap = prefs.getBool("builtinMagSwap", false);
#else
  _builtinSensorsEnabled = false;
  _builtinMagSwap = false;
  prefs.putBool("builtinSensEn", false);
#endif

  _safetySensorEnabled = prefs.getBool("safeSensEnabled", false);
  _safetyCloseDelayMs = prefs.getUInt("safeCloseDelay", 3000);
  _safetyReverseDelayMs = prefs.getUInt("safeRevDelay", 5000);
  _remotePassageClose = prefs.getBool("remPassClose", false);
  _remotePassageDelayMs = prefs.getUInt("remPassDelay", 5000);
  _doorTransitionMs = prefs.getUInt("doorTransMs", 5000);
  _magSensorType = prefs.getUChar("magSensType", 0);
  _safetySensorType = prefs.getUChar("safeSensType", 0);
  _autoRetryCloseEnabled = prefs.getBool("autoRetryEn", true);
  _maxCloseRetryCount = prefs.getUChar("maxCloseRetry", 3);
  _closeRetryDelayMs = prefs.getUInt("retryCloseDelay", 3000);
  _doubleClosePulseEnabled = prefs.getBool("dblClose", true);

  _autoRebootEnabled = prefs.getBool("rebootEn", true);
  _autoRebootHour = prefs.getUChar("rebootHour", kDailyRestartHour);
  _autoRebootMinute = prefs.getUChar("rebootMin", 0);
}

void AppConfig::saveToPrefs(Preferences& prefs) {
  prefs.putBool("simMode", _simMode);
  prefs.putBool("motorReverse", _motorReverse);
  prefs.putString("deviceName", _deviceName);
  prefs.putUChar("espNowCh", _espNowChannel);
  prefs.putBool("tagAutoEnabled", _tagAutoEnabled);
  prefs.putBool("tagBuzzerEn", _tagBuzzerEnabled);

  prefs.putBool("magSensEnabled", _magSensorEnabled);
  prefs.putBool("builtinSensEn", _builtinSensorsEnabled);
  prefs.putBool("builtinMagSwap", _builtinMagSwap);

  prefs.putBool("safeSensEnabled", _safetySensorEnabled);
  prefs.putUChar("safeSensType", _safetySensorType);
  prefs.putUInt("safeCloseDelay", _safetyCloseDelayMs);
  prefs.putUInt("safeRevDelay", _safetyReverseDelayMs);
  prefs.putBool("remPassClose", _remotePassageClose);
  prefs.putUInt("remPassDelay", _remotePassageDelayMs);
  prefs.putUInt("doorTransMs", _doorTransitionMs);
  prefs.putUChar("magSensType", _magSensorType);
  prefs.putBool("autoRetryEn", _autoRetryCloseEnabled);
  prefs.putUChar("maxCloseRetry", _maxCloseRetryCount);
  prefs.putUInt("retryCloseDelay", _closeRetryDelayMs);
  prefs.putBool("dblClose", _doubleClosePulseEnabled);

  prefs.putBool("rebootEn", _autoRebootEnabled);
  prefs.putUChar("rebootHour", _autoRebootHour);
  prefs.putUChar("rebootMin", _autoRebootMinute);
}
