#include "BaseController.h"
#include "modules/web_auth.h"

void BaseController::sendJson(int code, const JsonDocument& doc) {
  String out;
  serializeJson(doc, out);
  _server.send(code, "application/json", out);
}

JsonDocument BaseController::parseBodyOrEmpty() {
  JsonDocument doc;
  if (_server.hasArg("plain")) {
    deserializeJson(doc, _server.arg("plain"));
  }
  return doc;
}

void BaseController::wrap(HandlerFn fn, bool skipAuth) {
  if (!skipAuth && !ensureWebAuth()) return;
  fn();
}
