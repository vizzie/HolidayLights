#pragma once
#include <Arduino.h>
#include <FastLED.h>

struct DateRange {
  uint8_t startMonth;
  uint8_t startDay;
  uint8_t endMonth;
  uint8_t endDay;
};

enum EffectId : uint8_t {
  EFFECT_CHASE,
  EFFECT_SPARKLE,
};

struct Season {
  DateRange range;
  const TProgmemRGBPalette16 *palette;
  EffectId effect;
};

// True if (month, day) falls within range, handling ranges that wrap the
// new year (e.g. Dec 31 - Jan 2).
bool dateInRange(uint8_t month, uint8_t day, const DateRange &range);

// The active season for (month, day), or nullptr if none matches -- callers
// should fall back to DefaultPalette in that case.
const Season *getActiveSeason(uint8_t month, uint8_t day);

extern const TProgmemRGBPalette16 DefaultPalette;
