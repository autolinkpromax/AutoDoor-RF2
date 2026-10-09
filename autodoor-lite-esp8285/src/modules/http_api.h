#pragma once

#include <ESP8266WebServer.h>

namespace HttpApi {

using LearnStartFn = bool (*)(uint16_t holdMs, uint16_t timeoutSec);
using LearnStopFn = void (*)();
using LearnStatusFn = String (*)();
using AutoChannelFn = uint8_t (*)();

void begin(ESP8266WebServer& server,
		   LearnStartFn startFn,
		   LearnStopFn stopFn,
		   LearnStatusFn statusFn,
		   AutoChannelFn autoChannelFn);

}  // namespace HttpApi
