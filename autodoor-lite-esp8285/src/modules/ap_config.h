#pragma once

#include <stdint.h>
#include <Arduino.h>

namespace ApConfig {

bool detectConfigRequest(uint8_t bootButtonPin, uint32_t windowMs);
bool beginConfigAP(uint8_t channel);
String apSsid();

}  // namespace ApConfig
