#pragma once

#include <Arduino.h>

bool parseIp(const String& s, IPAddress& out);
void applyStaConfig();
void snapshotStaIpToPrefs();
void clearHomeWifiPrefs();

void setupWifiApSta();
void setupWifiApStaPhased();  // Staged boot: SoftAP-only init, STA connect deferred to loop()
void applyWifiTxPower();
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
