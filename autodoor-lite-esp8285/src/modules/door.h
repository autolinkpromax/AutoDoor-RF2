#pragma once

#include <stdint.h>

#include "app/types.h"

namespace Door {

void begin(uint8_t pinOpen, uint8_t pinClose, uint16_t pulseMs);
void setPulseMs(uint16_t pulseMs);

void open();
void close();
void stop();
void step();

DoorState state();

}  // namespace Door
