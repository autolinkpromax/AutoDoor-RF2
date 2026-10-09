#pragma once

#include <stdint.h>

namespace Health {

void begin(uint32_t dailyRestartMs);
void step();

}  // namespace Health
