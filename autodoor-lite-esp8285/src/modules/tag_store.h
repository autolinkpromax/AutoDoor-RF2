#pragma once

#include <stddef.h>
#include <stdint.h>

#include "app/config.h"
#include "app/types.h"

namespace TagStore {

bool begin();

const AppSettings& settings();
bool updateSettings(const AppSettings& s);

size_t tagCount();
const TagRecord* tagAt(size_t idx);

bool addTag(uint32_t tagId, const char* name, uint16_t holdMs);
bool removeTag(uint32_t tagId);
bool setTag(uint32_t tagId,
			const char* name,
			uint16_t holdMs,
			uint16_t closeDelayMs,
			int8_t tagRssiMin,
			int8_t tagPassRssi,
			uint16_t tagPassCloseDelayMs,
			bool enabled);

bool tagExists(uint32_t tagId);
void markSeen(uint32_t tagId, uint32_t nowMs, uint8_t sensitivity, const uint8_t* sourceMac, int8_t rssi);
void stepAging(uint32_t nowMs);
bool anyActiveTags();
const RuntimeTagState* runtimeAt(size_t idx);

String tagsToJson();

}  // namespace TagStore
