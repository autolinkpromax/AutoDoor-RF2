#pragma once

#include <WebServer.h>
#include <ArduinoJson.h>
#include <functional>

/**
 * @brief Base controller providing shared JSON and authentication helpers.
 */
class BaseController {
public:
  using HandlerFn = std::function<void()>;

  explicit BaseController(WebServer& server) : _server(server) {}
  virtual ~BaseController() = default;

  /**
   * @brief Register route endpoints on the WebServer.
   */
  virtual void registerRoutes() = 0;

protected:
  WebServer& _server;

  void sendJson(int code, const JsonDocument& doc);
  JsonDocument parseBodyOrEmpty();
  void wrap(HandlerFn fn, bool skipAuth = false);
};
