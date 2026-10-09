#pragma once

#include <stdint.h>

namespace EspNowRuntime {

using TagSeenCallback = void (*)(uint32_t tagId, int8_t rssi, const uint8_t* sourceMac);

bool begin(uint8_t channel, TagSeenCallback cb);
void setChannel(uint8_t channel);
uint8_t currentChannel();
uint8_t autoSelectChannel();
void step();

}  // namespace EspNowRuntime
