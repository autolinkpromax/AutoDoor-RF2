#include "rf433_runtime.h"

#include <SYN480R.h>

namespace {

Rf433Runtime::RfCodeCallback gRfCb = nullptr;

}  // namespace

namespace Rf433Runtime {

void begin(uint8_t pinRx, RfCodeCallback cb) {
  gRfCb = cb;
  SYN480R::begin(pinRx);
}

void step() {
  if (!gRfCb || !SYN480R::available()) {
    return;
  }
  const uint32_t code = SYN480R::read();
  if (code != 0) {
    gRfCb(code);
  }
}

}  // namespace Rf433Runtime
