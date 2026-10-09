#pragma once

#include <WebServer.h>
#include "DoorApiController.h"
#include "TagsApiController.h"
#include "SystemApiController.h"
#include "NetworkApiController.h"
#include "ConfigApiController.h"
#include "EventsApiController.h"

/**
 * @brief Orchestrator for all HTTP API controllers and WebUI asset routes.
 */
class HttpApiServer {
public:
  static HttpApiServer& instance();

  explicit HttpApiServer(WebServer& server);

  /**
   * @brief Initialize headers, register all controller routes, and start web server.
   */
  void begin();

  /**
   * @brief Handle unknown routes and fallback to WebUI assets / index.
   */
  void handleNotFound();

private:
  WebServer& _server;

  DoorApiController _doorController;
  TagsApiController _tagsController;
  SystemApiController _systemController;
  NetworkApiController _networkController;
  ConfigApiController _configController;
  EventsApiController _eventsController;
};
