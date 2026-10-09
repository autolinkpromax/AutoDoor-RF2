#include "web_auth.h"

#include "app/globals.h"
#include "tags/TagRegistry.h"

#include <ArduinoJson.h>
#include <vector>

namespace {

constexpr size_t kMaxWebSessions = 16;
std::vector<String> gWebSessions;

int b64CharVal(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;
}

String base64Decode(const String& in) {
  String out;
  out.reserve((in.length() * 3) / 4);
  uint32_t acc = 0;
  int bits = 0;
  for (size_t i = 0; i < in.length(); i++) {
    const char c = in[i];
    if (c == '=' || c == '\n' || c == '\r') break;
    const int v = b64CharVal(c);
    if (v < 0) continue;
    acc = (acc << 6) | (uint32_t)v;
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      out += char((acc >> bits) & 0xFF);
    }
  }
  return out;
}

void saveWebSessionsToPrefs() {
  TagStoreLock lock;
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (const auto& token : gWebSessions) {
    arr.add(token);
  }
  String out;
  serializeJson(doc, out);
  gPrefs.putString("webSess", out);
}

String sessionFromCookieHeader() {
  if (!gServer.hasHeader("Cookie")) return {};
  const String cookie = gServer.header("Cookie");
  const int i = cookie.indexOf("al_sess=");
  if (i < 0) return {};
  int start = i + 8;
  int end = cookie.indexOf(';', start);
  if (end < 0) end = cookie.length();
  return cookie.substring(start, end);
}

bool sessionTokenValid(const String& token) {
  if (token.isEmpty()) return false;
  for (const auto& existing : gWebSessions) {
    if (existing == token) return true;
  }
  return false;
}

}  // namespace

void loadWebAuthFromPrefs() {
  TagStoreLock lock;
  gWebPassword = gPrefs.getString("webPass", "");
  gWebSessions.clear();

  const String raw = gPrefs.getString("webSess", "");
  if (raw.isEmpty()) return;

  JsonDocument doc;
  if (!deserializeJson(doc, raw) && doc.is<JsonArrayConst>()) {
    for (JsonVariantConst v : doc.as<JsonArrayConst>()) {
      const String token = v.as<String>();
      if (!token.isEmpty()) gWebSessions.push_back(token);
    }
    return;
  }

  if (raw.length() >= 8) {
    gWebSessions.push_back(raw);
    saveWebSessionsToPrefs();
  }
}

String createWebSession() {
  const String token = makeId();
  gWebSessions.push_back(token);
  while (gWebSessions.size() > kMaxWebSessions) {
    gWebSessions.erase(gWebSessions.begin());
  }
  saveWebSessionsToPrefs();
  AD_LOG(Serial.printf("[auth] session created (active=%u)\n", (unsigned)gWebSessions.size()));
  return token;
}

void revokeAllWebSessions() {
  TagStoreLock lock;
  gWebSessions.clear();
  gPrefs.remove("webSess");
  AD_LOG(Serial.println("[auth] all sessions revoked"));
}

bool cookieSessionValid() {
  if (gWebSessions.empty()) return false;
  return sessionTokenValid(sessionFromCookieHeader());
}

bool isWebAuthed() {
  if (gSafeMode || gWebPassword.isEmpty()) return true;
  const String uri = gServer.uri();
  if (uri.startsWith("/hook/")) return true;
  if (cookieSessionValid()) return true;
  if (gServer.hasHeader("Authorization")) {
    String auth = gServer.header("Authorization");
    if (auth.startsWith("Basic ")) {
      const String decoded = base64Decode(auth.substring(6));
      const int colon = decoded.indexOf(':');
      const String pass = (colon >= 0) ? decoded.substring(colon + 1) : decoded;
      if (pass == gWebPassword) return true;
    }
  }
  return false;
}

bool ensureWebAuth() {
  if (isWebAuthed()) return true;
  gServer.send(401, "application/json", "{\"ok\":false,\"error\":\"auth_required\"}");
  return false;
}
