#include "webhook_hooks.h"

#include "app/globals.h"
#include "tags/TagRegistry.h"

#include <WiFi.h>
#include <esp_random.h>

String gHookKey;

void ensureHookKey() {
  TagStoreLock lock;
  gHookKey = gPrefs.getString("hookKey", "");
  if (gHookKey.length() < 32) {
    char buf[65];
    snprintf(buf, sizeof(buf), "%08lx%08lx%08lx%08lx",
             (unsigned long)esp_random(),
             (unsigned long)esp_random(),
             (unsigned long)esp_random(),
             (unsigned long)esp_random());
    gHookKey = String(buf);
    gPrefs.putString("hookKey", gHookKey);
  }
}


String hookHostIp() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }
  return WiFi.softAPIP().toString();
}
