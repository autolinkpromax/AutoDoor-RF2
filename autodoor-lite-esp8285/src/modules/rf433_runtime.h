#pragma once

#include <stdint.h>

namespace Rf433Runtime {

using RfCodeCallback = void (*)(uint32_t code);

void begin(uint8_t pinRx, RfCodeCallback cb);
void step();

}  // namespace Rf433Runtime
