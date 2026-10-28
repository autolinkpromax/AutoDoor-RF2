#pragma once

#include <Arduino.h>

bool parseIp(const String& s, IPAddress& out);
void applyStaConfig();
void snapshotStaIpToPrefs();
void clearHomeWifiPrefs();

void setupWifiApSta();
void setupWifiApStaPhased();  // Staged boot: SoftAP-only init, STA connect deferred to loop()
void applyWifiTxPower();
void applyWifiBootTxPower();      // Reduced TX power during early boot (kWifiBootTxPowerDbm)
void stepWifiBootPowerRestore();  // Restore kWifiTxPowerDbm once STA is up / timeout / Safe Mode
uint32_t wifiStaUpMs();           // ms since STA connected (0 while disconnected)
bool wifiStaConfigured();         // true if home Wi-Fi SSID is stored
void autoDetectEspNowChannelOnBoot();
bool persistEspNowChannel(uint8_t ch);
void stepEspNowChannelSync();
void stepWiFiReconnect();
void stepApIdle();
void stepDnsServer();
void startSoftAp();
String resolveApSsid();
void invalidateStaSsidCache();
void invalidateStaPassCache();
void invalidateApSettingsCache();

String sanitizeHostname(String name);
String resolveHostname();
bool startMdns();
