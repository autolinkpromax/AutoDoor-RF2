#include "webui_serve.h"

#include "app/globals.h"
#include "webui_assets.h"

#include <WebServer.h>

namespace {

const WebUiAsset* findAsset(const String& path) {
  for (size_t i = 0; i < kWebUiAssetCount; ++i) {
    if (path.equals(kWebUiAssets[i].path)) {
      return &kWebUiAssets[i];
    }
  }
  return nullptr;
}

void sendAsset(const WebUiAsset& asset) {
  const String ifNoneMatch = gServer.header("If-None-Match");
  if (ifNoneMatch.length() > 0 && ifNoneMatch == asset.etag) {
    gServer.send(304);
    return;
  }
  gServer.sendHeader("ETag", asset.etag);
  if (asset.immutable) {
    gServer.sendHeader("Cache-Control", "public, max-age=31536000, immutable");
  } else {
    gServer.sendHeader("Cache-Control", "no-cache");
  }
  gServer.send_P(200, asset.mime, (const char*)asset.data, asset.len);
}

}  // namespace

void handleWebUiIndex() {
  const WebUiAsset* index = findAsset("index.html");
  if (!index) {
    gServer.send(500, "text/plain", "index missing");
    return;
  }
  sendAsset(*index);
}

bool tryServeWebUiAsset() {
  if (gServer.method() != HTTP_GET) return false;
  String uri = gServer.uri();
  if (!uri.startsWith("/assets/")) return false;
  String path = uri.substring(8);
  if (path.length() == 0) return false;
  const WebUiAsset* asset = findAsset(path);
  if (!asset) return false;
  sendAsset(*asset);
  return true;
}
