#pragma once

#include "BaseController.h"

/**
 * @brief Controller for ESP-NOW and 433MHz Tag management and Learn mode.
 */
class TagsApiController : public BaseController {
public:
  explicit TagsApiController(WebServer& server) : BaseController(server) {}

  void registerRoutes() override;

private:
  void handleTagsList();
  void handleTagsCreate();
  void handleTagUpdate();
  void handleTagDelete();
  void handleTagLearn();
  void handleTagAutoPost();
};
