#include "http_api.h"
#include "api/HttpApiServer.h"

/**
 * @brief Legacy entry point for WebServer setup, delegating to the OOP HttpApiServer.
 */
void setupWebServer() {
  HttpApiServer::instance().begin();
}
