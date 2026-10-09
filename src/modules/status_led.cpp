#include "status_led.h"

#include "app/config.h"
#include "app/globals.h"

bool detectSafeModeBoot() {
  if (gPrefs.getBool("forceSafeMode", false)) {
    gPrefs.remove("forceSafeMode");
    Serial.println("[safe] SAFE MODE enabled via Web UI flag");
    return true;
  }

  pinMode(kBootBtnPin, INPUT_PULLUP);
  delay(40);
  if (digitalRead(kBootBtnPin) != LOW) return false;
  AD_LOG(Serial.println("[safe] boot held - keep 5s for safe mode"));
  const uint32_t t0 = millis();
  while ((uint32_t)(millis() - t0) < 5000) {
    if (digitalRead(kBootBtnPin) != LOW) return false;
    delay(50);
  }
  Serial.println("[safe] SAFE MODE enabled");
  return true;
}

void initStatusLed() {
  pinMode(kBootBtnPin, INPUT_PULLUP);
}

void stepStatusLed() {}
