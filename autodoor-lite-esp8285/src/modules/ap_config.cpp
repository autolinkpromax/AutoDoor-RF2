#include "ap_config.h"

#include <ESP8266WiFi.h>

#include "app/config.h"

namespace {

String gApSsid;

}  // namespace

namespace ApConfig {

bool detectConfigRequest(uint8_t bootButtonPin, uint32_t windowMs) {
  pinMode(bootButtonPin, INPUT_PULLUP);
  const uint32_t start = millis();
  bool pressed = false;
  while ((millis() - start) < windowMs) {
    if (digitalRead(bootButtonPin) == LOW) {
      pressed = true;
      break;
    }
    delay(10);
    yield();
  }
  return pressed;
}

bool beginConfigAP(uint8_t channel) {
  if (channel < 1 || channel > 13) {
    channel = kDefaultEspNowChannel;
  }

  uint32_t chip = ESP.getChipId();
  char suffix[7];
  snprintf(suffix, sizeof(suffix), "%06lX", static_cast<unsigned long>(chip));

  gApSsid = String(kApSsidPrefix) + suffix;

  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_AP_STA);
  WiFi.persistent(false);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  delay(100);
  wifi_set_channel(channel);
  bool ok = WiFi.softAP(gApSsid.c_str(), kApPassword, channel, false, 4);
  if (!ok) {
    delay(150);
    wifi_set_channel(channel);
    ok = WiFi.softAP(gApSsid.c_str(), kApPassword, channel, false, 4);
  }
  return ok;
}

String apSsid() { return gApSsid; }

}  // namespace ApConfig
