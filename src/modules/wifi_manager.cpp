#include "wifi_manager.h"

#include "app/config.h"
#include "app/globals.h"
#include "modules/esp_now_runtime.h"
#include "tags/TagRegistry.h"

#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>

namespace {

uint32_t gStaReconnectBackoffMs = 5000;
uint32_t gStaLastReconnectAttemptMs = 0;
constexpr uint32_t kStaReconnectBackoffMinMs = 5000;
constexpr uint32_t kStaReconnectBackoffMaxMs = 300000;
constexpr char kDefaultApPassword[] = "12345678";

String sCachedStaSsid;
bool sStaSsidLoaded = false;

String sCachedStaPass;
bool sStaPassLoaded = false;

// AP settings cache — loaded once on first access, invalidated via invalidateApSettingsCache()
bool sApUserEnabled = true;
bool sApUserEnabledLoaded = false;
uint8_t sApIdleMinutes = 1;
bool sApIdleMinutesLoaded = false;

const String& getCachedStaSsid() {
  if (!sStaSsidLoaded) {
    sCachedStaSsid = gPrefs.getString("wifiSsid", "");
    sStaSsidLoaded = true;
  }
  return sCachedStaSsid;
}

const String& getCachedStaPass() {
  if (!sStaPassLoaded) {
    sCachedStaPass = gPrefs.getString("wifiPass", "");
    sStaPassLoaded = true;
  }
  return sCachedStaPass;
}

bool apUserEnabled() {
  if (!sApUserEnabledLoaded) {
    sApUserEnabled = gPrefs.getBool("apUserEnabled", true);
    sApUserEnabledLoaded = true;
  }
  return sApUserEnabled;
}

uint8_t apIdleMinutes() {
  if (!sApIdleMinutesLoaded) {
    sApIdleMinutes = constrain(gPrefs.getUChar("apIdleMin", 1), 1, 120);
    sApIdleMinutesLoaded = true;
  }
  return sApIdleMinutes;
}

bool isValidApPassword(const String& pass) {
  return pass.length() >= 8 && pass.length() <= 63;
}

String resolveApPassword() {
  String custom = gPrefs.getString("apPass", "");
  if (isValidApPassword(custom)) return custom;
  if (!custom.isEmpty()) {
    Serial.println("[ap] stored password invalid — using default");
  }
  custom = kDefaultApPassword;
  gPrefs.putString("apPass", custom);
  return custom;
}

}  // namespace

bool parseIp(const String& s, IPAddress& out) {
  return out.fromString(s);
}

void applyStaConfig() {
  if (gSafeMode) {
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    return;
  }
  gWifiStatic = gPrefs.getBool("wifiStatic", false);
  if (!gWifiStatic) {
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    return;
  }

  const String ipStr = gPrefs.getString("wifiIp", "");
  const String gwStr = gPrefs.getString("wifiGw", "");
  const String maskStr = gPrefs.getString("wifiMask", "");
  const String dnsStr = gPrefs.getString("wifiDns", "");
  if (ipStr.isEmpty() || gwStr.isEmpty() || maskStr.isEmpty()) return;

  IPAddress ip, gw, mask, dns;
  if (!parseIp(ipStr, ip) || !parseIp(gwStr, gw) || !parseIp(maskStr, mask)) return;
  if (dnsStr.isEmpty()) {
    dns = gw;
  } else if (!parseIp(dnsStr, dns)) {
    dns = gw;
  }
  WiFi.config(ip, gw, mask, dns);
}

void snapshotStaIpToPrefs() {
  if (WiFi.status() != WL_CONNECTED) return;
  const String newIp = WiFi.localIP().toString();
  if (gPrefs.getString("wifiIp", "") != newIp) {
    gPrefs.putString("wifiIp", newIp);
    gPrefs.putString("wifiGw", WiFi.gatewayIP().toString());
    gPrefs.putString("wifiMask", WiFi.subnetMask().toString());
    gPrefs.putString("wifiDns", WiFi.dnsIP().toString());
  }
}

void clearHomeWifiPrefs() {
  gPrefs.remove("wifiSsid");
  gPrefs.remove("wifiPass");
  gPrefs.remove("wifiStatic");
  gPrefs.remove("wifiIp");
  gPrefs.remove("wifiGw");
  gPrefs.remove("wifiMask");
  gPrefs.remove("wifiDns");

  gWifiStatic = false;
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.disconnect(true, true);
  gStaReconnectBackoffMs = kStaReconnectBackoffMinMs;
  gStaLastReconnectAttemptMs = 0;
  sCachedStaSsid = "";
  sStaSsidLoaded = true;
  sCachedStaPass = "";
  sStaPassLoaded = true;

  Serial.println("[safe] cleared home wifi credentials/static-ip prefs");
}

uint8_t pickLeastCongestedChannel(int networkCount) {
  float channelScore[14] = {0};
  for (int i = 0; i < networkCount; ++i) {
    const int ch = WiFi.channel(i);
    if (ch >= 1 && ch <= 13) {
      const int rssi = WiFi.RSSI(i);
      // Weight power contribution of each network based on signal strength
      // RSSI to linear scale: e.g. -100dBm = 1 unit, -90dBm = 10 units, etc.
      float power = pow(10.0f, (rssi + 100.0f) / 10.0f);
      channelScore[ch] += power;
    }
  }
  uint8_t bestCh = kEspNowChannel;
  float minScore = 1e12f;
  for (int ch = 1; ch <= 13; ++ch) {
    if (channelScore[ch] < minScore) {
      minScore = channelScore[ch];
      bestCh = (uint8_t)ch;
    }
  }
  return bestCh;
}

bool persistEspNowChannel(uint8_t ch) {
  TagStoreLock lock;
  ch = constrain(ch, (uint8_t)1, (uint8_t)13);
  if (ch == gEspNowChannel) return false;
  const uint8_t prev = gEspNowChannel;
  gEspNowChannel = ch;
  gPrefs.putUChar("espNowCh", gEspNowChannel);
  esp_wifi_set_channel(gEspNowChannel, WIFI_SECOND_CHAN_NONE);
  if (gApEnabled) {
    startSoftAp();
  }
  syncEspNowPeers();
  Serial.printf("[ch] updated %u -> %u (saved)\n", (unsigned)prev, (unsigned)gEspNowChannel);
  return true;
}

void autoDetectEspNowChannelOnBoot() {
  const uint8_t saved = gEspNowChannel;
  // If we already have a saved, valid ESP-NOW channel (1..14), skip the blocking Wi-Fi scan on boot.
  // stepEspNowChannelSync() in loop() will automatically sync channels once connected to home Wi-Fi.
  if (saved >= 1 && saved <= 14) {
    Serial.printf("[ch] boot: using saved channel %u (skip blocking scan for fast boot)\n", saved);
    return;
  }

  uint8_t detected = saved;

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  const String staSsid = gPrefs.getString("wifiSsid", "");
  const int n = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/true);
  bool ssidFound = false;

  if (!staSsid.isEmpty() && n > 0) {
    int bestRssi = -127;
    for (int i = 0; i < n; ++i) {
      if (WiFi.SSID(i) != staSsid) continue;
      ssidFound = true;
      const int rssi = WiFi.RSSI(i);
      if (rssi > bestRssi) {
        bestRssi = rssi;
        detected = (uint8_t)WiFi.channel(i);
      }
    }
    if (ssidFound) {
      Serial.printf("[ch] boot scan: STA \"%s\" on ch%u\n", staSsid.c_str(), (unsigned)detected);
    }
  }

  if (staSsid.isEmpty() || !ssidFound) {
    detected = pickLeastCongestedChannel(n);
    Serial.printf("[ch] boot scan: least congested ch%u (networks=%d)\n", (unsigned)detected, n);
  }

  WiFi.scanDelete();

  if (detected != saved) {
    persistEspNowChannel(detected);
  } else {
    Serial.printf("[ch] boot scan: ch%u unchanged\n", (unsigned)detected);
  }
}

void stepEspNowChannelSync() {
  if (gEspNowDisabled) return;
  if (!isEspNowReady()) return; // Bypass if ESP-NOW is suspended (e.g. during/after WiFi scan)
  
  const uint32_t now = millis();
  static uint32_t lastSyncCheckMs = 0;
  if ((uint32_t)(now - lastSyncCheckMs) < 5000) return;
  lastSyncCheckMs = now;

  if (WiFi.status() != WL_CONNECTED) return;
  const int ch = WiFi.channel();
  if (ch < 1 || ch > 13) return;
  if ((uint8_t)ch == gEspNowChannel) return;
  Serial.printf("[ch] STA connected on ch%d — syncing espnow\n", ch);
  persistEspNowChannel((uint8_t)ch);
}

static DNSServer sDnsServer;
static bool sDnsRunning = false;

static void stopDnsServer() {
  if (sDnsRunning) {
    sDnsServer.stop();
    sDnsRunning = false;
    AD_LOG(Serial.println("[dns] Captive portal DNS server stopped"));
  }
}

String resolveApSsid() {
  const String custom = gPrefs.getString("apSsid", "");
  if (!custom.isEmpty()) return custom;
  const uint64_t mac = ESP.getEfuseMac();
  char ssid[32];
  snprintf(ssid, sizeof(ssid), "%s%04X", kApSsidPrefix, (uint32_t)(mac & 0xFFFFu));
  return String(ssid);
}

void startSoftAp() {
  if (!gSafeMode && !apUserEnabled()) {
    if (gApEnabled) {
      stopDnsServer();
      WiFi.softAPdisconnect(true);
      gApEnabled = false;
      AD_LOG(Serial.println("[ap] disabled by user setting"));
    }
    return;
  }
  if (gApEnabled) {
    stopDnsServer();
    WiFi.softAPdisconnect(true);
    gApEnabled = false;
    delay(80);
  }
  const String ssid = resolveApSsid();
  if (gSafeMode) {
    gApEnabled = WiFi.softAP(ssid.c_str(), nullptr, gEspNowChannel);
    Serial.printf("[ap] SAFE MODE open AP ssid=%s ch=%u ip=%s\n", ssid.c_str(),
                  (unsigned)gEspNowChannel, WiFi.softAPIP().toString().c_str());
  } else {
    const String pass = resolveApPassword();
    gApEnabled = WiFi.softAP(ssid.c_str(), pass.c_str(), gEspNowChannel);
    Serial.printf("[ap] ssid=%s pass=%s ch=%u ip=%s\n", ssid.c_str(), pass.c_str(),
                  (unsigned)gEspNowChannel, WiFi.softAPIP().toString().c_str());
  }
  if (gApEnabled) {
    if (!sDnsRunning) {
      sDnsServer.setErrorReplyCode(DNSReplyCode::NoError);
      sDnsServer.start(53, "*", WiFi.softAPIP());
      sDnsRunning = true;
      AD_LOG(Serial.println("[dns] Captive portal DNS server started on port 53"));
    }
  }
  gApLastHadStationsMs = millis();
}

void stepDnsServer() {
  if (sDnsRunning) {
    sDnsServer.processNextRequest();
  }
}

void stepApIdle() {
  if (!gApEnabled || gSafeMode) return;
  
  const uint32_t now = millis();
  static uint32_t lastCheckMs = 0;
  if ((uint32_t)(now - lastCheckMs) < 5000) return;
  lastCheckMs = now;

  if (getCachedStaSsid().isEmpty()) return;
  if (WiFi.softAPgetStationNum() > 0) {
    gApLastHadStationsMs = now;
    return;
  }
  if (gApLastHadStationsMs == 0) gApLastHadStationsMs = now;
  const uint32_t idleMs = (uint32_t)apIdleMinutes() * 60000UL;
  if ((uint32_t)(now - gApLastHadStationsMs) >= idleMs) {
    stopDnsServer();
    WiFi.softAPdisconnect(true);
    gApEnabled = false;
    AD_LOG(Serial.printf("[ap] auto-off idle %u min\n", (unsigned)apIdleMinutes()));
  }
}

void invalidateStaSsidCache() {
  sStaSsidLoaded = false;
}

void invalidateStaPassCache() {
  sCachedStaPass = "";
  sStaPassLoaded = false;
}

void invalidateApSettingsCache() {
  sApUserEnabledLoaded = false;
  sApIdleMinutesLoaded = false;
}

void stepWiFiReconnect() {
  if (gEspNowDisabled) return;
  if (!isEspNowReady()) return; // Bypass if ESP-NOW is suspended (e.g. during/after WiFi scan)
  
  const uint32_t now = millis();
  static uint32_t lastCheckMs = 0;
  if ((uint32_t)(now - lastCheckMs) < 3000) return;
  lastCheckMs = now;

  const wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    gStaReconnectBackoffMs = kStaReconnectBackoffMinMs;
    return;
  }

  const String& staSsid = getCachedStaSsid();
  if (staSsid.isEmpty()) return;

  if ((int32_t)(now - gStaLastReconnectAttemptMs) < (int32_t)gStaReconnectBackoffMs) return;
  gStaLastReconnectAttemptMs = now;

  // Fallback to DHCP if Fix IP connection repeatedly fails (reached maximum backoff)
  if (gWifiStatic && gStaReconnectBackoffMs >= kStaReconnectBackoffMaxMs) {
    Serial.println("[wifi] Fix IP reconnect failed repeatedly -> falling back to DHCP safety net");
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  } else {
    applyStaConfig();
  }

  const String& staPass = getCachedStaPass();
  WiFi.begin(staSsid.c_str(), staPass.c_str());
  esp_wifi_set_channel(gEspNowChannel, WIFI_SECOND_CHAN_NONE);

  gStaReconnectBackoffMs = min(gStaReconnectBackoffMs * 2, kStaReconnectBackoffMaxMs);
}

void applyWifiTxPower() {
  WiFi.setTxPower(static_cast<wifi_power_t>(kWifiTxPower));
}

String sanitizeHostname(String name) {
  name.trim();
  name.toLowerCase();
  String clean = "";
  for (size_t i = 0; i < name.length() && clean.length() < 32; i++) {
    char c = name[i];
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
      clean += c;
    } else if (c == '-' || c == '_' || c == ' ') {
      if (clean.length() > 0 && clean[clean.length() - 1] != '-') {
        clean += '-';
      }
    }
  }
  while (clean.length() > 0 && clean[0] == '-') {
    clean.remove(0, 1);
  }
  while (clean.length() > 0 && clean[clean.length() - 1] == '-') {
    clean.remove(clean.length() - 1);
  }
  return clean;
}

String resolveHostname() {
  const String custom = gPrefs.getString("hostname", "");
  if (!custom.isEmpty()) {
    const String clean = sanitizeHostname(custom);
    if (!clean.isEmpty()) return clean;
  }
  String defHost = sanitizeHostname(resolveApSsid());
  if (defHost.isEmpty()) defHost = "autodoor";
  return defHost;
}

bool startMdns() {
  const String host = resolveHostname();
  WiFi.setHostname(host.c_str());
  MDNS.end();
  if (!MDNS.begin(host.c_str())) {
    Serial.printf("[mdns] failed to start responder for '%s'\n", host.c_str());
    return false;
  }
  MDNS.addService("http", "tcp", 80);
  Serial.printf("[mdns] responder active: http://%s.local/\n", host.c_str());
  return true;
}

void setupWifiApSta() {
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  applyWifiTxPower();

  const String host = resolveHostname();
  WiFi.setHostname(host.c_str());
  startSoftAp();

  const String& staSsid = getCachedStaSsid();
  const String& staPass = getCachedStaPass();
  if (!staSsid.isEmpty()) {
    applyStaConfig();
    WiFi.begin(staSsid.c_str(), staPass.c_str());
  }
  gWifiStatic = gPrefs.getBool("wifiStatic", false);
  esp_wifi_set_channel(gEspNowChannel, WIFI_SECOND_CHAN_NONE);

  startMdns();
}
