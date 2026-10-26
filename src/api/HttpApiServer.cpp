#include "HttpApiServer.h"
#include "app/globals.h"
#include "webui_serve.h"

HttpApiServer& HttpApiServer::instance() {
  static HttpApiServer sInstance(gServer);
  return sInstance;
}

HttpApiServer::HttpApiServer(WebServer& server)
    : _server(server),
      _doorController(server),
      _tagsController(server),
      _systemController(server),
      _networkController(server),
      _configController(server),
      _eventsController(server) {}

void HttpApiServer::begin() {
  const char* headerKeys[] = {"Cookie", "Authorization", "Host"};
  _server.collectHeaders(headerKeys, 3);

  _doorController.registerRoutes();
  _tagsController.registerRoutes();
  _systemController.registerRoutes();
  _networkController.registerRoutes();
  _configController.registerRoutes();
  _eventsController.registerRoutes();

  _server.onNotFound([this]() {
    handleNotFound();
  });

  _server.begin();
  AD_LOG(Serial.println("[http] server started"));
}

void HttpApiServer::handleNotFound() {
  if (tryServeWebUiAsset()) return;
  if (_server.method() == HTTP_GET && _server.uri() == "/") {
    handleWebUiIndex();
    return;
  }

  // Captive Portal Redirect for SoftAP clients
  if (gApEnabled && _server.client().localIP() == WiFi.softAPIP()) {
    const String host = _server.hostHeader();
    const String apIp = WiFi.softAPIP().toString();
    const String uri = _server.uri();

    if ((!host.isEmpty() && host != apIp && !host.startsWith("autolink")) ||
        uri == "/hotspot-detect.html" || uri == "/generate_204" || uri == "/gen_204" ||
        uri == "/connecttest.txt" || uri == "/ncsi.txt" || uri == "/mobile/status.php") {
      _server.sendHeader("Location", String("http://") + apIp + "/", true);
      _server.send(302, "text/plain", "");
      return;
    }
  }

  _server.send(404, "text/plain", "not found");
}
