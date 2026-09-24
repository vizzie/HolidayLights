#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include "effects.h"

struct DateRange {
  uint8_t startMonth;
  uint8_t startDay;
  uint8_t endMonth;
  uint8_t endDay;
};

struct Season {
  DateRange range;
  const TProgmemRGBPalette16 *palette;
  const EffectId *effects; // cycles through these, one at a time
  uint8_t numEffects;
};

// True if (month, day) falls within range, handling ranges that wrap the
// new year (e.g. Dec 31 - Jan 2).
bool dateInRange(uint8_t month, uint8_t day, const DateRange &range);

// The season for (month, day). Always returns a valid Season: when today
// doesn't fall inside any defined range (including before the real date
// has been fetched, i.e. month == 0), a default season is returned instead.
const Season &getActiveSeason(uint8_t month, uint8_t day);

extern const TProgmemRGBPalette16 DefaultPalette;
