#include "seasons.h"

const TProgmemRGBPalette16 NewYearPalette PROGMEM = {
    CRGB::Gold,   CRGB::White,  CRGB::Silver, CRGB::Gold,
    CRGB::White,  CRGB::Gold,   CRGB::Silver, CRGB::White,
    CRGB::Gold,   CRGB::Silver, CRGB::White,  CRGB::Gold,
    CRGB::Silver, CRGB::Gold,   CRGB::White,  CRGB::Silver};

const TProgmemRGBPalette16 ValentinesPalette PROGMEM = {
    CRGB::Red,     CRGB::HotPink, CRGB::White,   CRGB::Crimson,
    CRGB::HotPink, CRGB::Red,     CRGB::Crimson, CRGB::White,
    CRGB::Red,     CRGB::White,   CRGB::HotPink, CRGB::Crimson,
    CRGB::White,   CRGB::Crimson, CRGB::Red,     CRGB::HotPink};

const TProgmemRGBPalette16 StPatricksPalette PROGMEM = {
    CRGB::Green,      CRGB::Gold,       CRGB::ForestGreen, CRGB::LimeGreen,
    CRGB::Gold,       CRGB::ForestGreen,CRGB::Green,       CRGB::LimeGreen,
    CRGB::ForestGreen,CRGB::LimeGreen,  CRGB::Gold,        CRGB::Green,
    CRGB::LimeGreen,  CRGB::Green,      CRGB::ForestGreen, CRGB::Gold};

// Easter's exact date moves every year and computing it needs a real
// calendar algorithm, which isn't worth it here -- this is a fixed
// "spring pastels" window that covers Easter most years instead.
const TProgmemRGBPalette16 SpringPalette PROGMEM = {
    CRGB::LightPink,  CRGB::LightYellow, CRGB::Lavender,   CRGB::PaleGreen,
    CRGB::LightYellow,CRGB::Lavender,    CRGB::PaleGreen,  CRGB::LightPink,
    CRGB::Lavender,   CRGB::PaleGreen,   CRGB::LightPink,  CRGB::LightYellow,
    CRGB::PaleGreen,  CRGB::LightPink,   CRGB::LightYellow,CRGB::Lavender};

const TProgmemRGBPalette16 IndependenceDayPalette PROGMEM = {
    CRGB::Red,      CRGB::White, CRGB::RoyalBlue, CRGB::Red,
    CRGB::White,    CRGB::Red,   CRGB::RoyalBlue, CRGB::White,
    CRGB::RoyalBlue,CRGB::Red,   CRGB::White,     CRGB::RoyalBlue,
    CRGB::Red,      CRGB::White, CRGB::RoyalBlue, CRGB::White};

const TProgmemRGBPalette16 HalloweenPalette PROGMEM = {
    CRGB::OrangeRed, CRGB::Purple,    CRGB::DarkOrange, CRGB::DarkViolet,
    CRGB::Purple,    CRGB::OrangeRed, CRGB::DarkViolet, CRGB::DarkOrange,
    CRGB::OrangeRed, CRGB::DarkViolet,CRGB::Purple,     CRGB::DarkOrange,
    CRGB::DarkOrange,CRGB::Purple,    CRGB::OrangeRed,  CRGB::DarkViolet};

const TProgmemRGBPalette16 ThanksgivingPalette PROGMEM = {
    CRGB::OrangeRed,  CRGB::SaddleBrown, CRGB::Gold,       CRGB::DarkRed,
    CRGB::SaddleBrown,CRGB::Gold,        CRGB::DarkRed,    CRGB::OrangeRed,
    CRGB::Gold,       CRGB::DarkRed,     CRGB::OrangeRed,  CRGB::SaddleBrown,
    CRGB::DarkRed,    CRGB::OrangeRed,   CRGB::SaddleBrown,CRGB::Gold};

const TProgmemRGBPalette16 ChristmasPalette PROGMEM = {
    CRGB::Red,  CRGB::Red,   CRGB::Green, CRGB::Green,
    CRGB::Red,  CRGB::Gold,  CRGB::Green, CRGB::Red,
    CRGB::Green,CRGB::Red,   CRGB::Gold,  CRGB::Green,
    CRGB::Red,  CRGB::Green, CRGB::Red,   CRGB::Green};

// Used whenever today doesn't fall inside any defined season below.
const TProgmemRGBPalette16 DefaultPalette PROGMEM = {
    CRGB::White, CRGB::White, CRGB::White, CRGB::White,
    CRGB::White, CRGB::White, CRGB::White, CRGB::White,
    CRGB::White, CRGB::White, CRGB::White, CRGB::White,
    CRGB::White, CRGB::White, CRGB::White, CRGB::White};

// Add more holidays/seasons here as needed -- each is just a date range, a
// palette, and which effect to run. Kept in calendar order for readability;
// matching only depends on ranges not overlapping.
static const Season SEASONS[] = {
    {{12, 31, 1, 2},  &NewYearPalette,         EFFECT_SPARKLE}, // New Year
    {{2, 10, 2, 14},  &ValentinesPalette,      EFFECT_SPARKLE}, // Valentine's
    {{3, 15, 3, 17},  &StPatricksPalette,      EFFECT_CHASE},   // St. Patrick's
    {{3, 20, 4, 20},  &SpringPalette,          EFFECT_CHASE},   // Spring
    {{6, 28, 7, 5},   &IndependenceDayPalette, EFFECT_SPARKLE}, // Independence Day
    {{10, 1, 10, 31}, &HalloweenPalette,       EFFECT_SPARKLE}, // Halloween
    {{11, 1, 11, 27}, &ThanksgivingPalette,    EFFECT_CHASE},   // Thanksgiving
    {{12, 1, 12, 30}, &ChristmasPalette,       EFFECT_SPARKLE}, // Christmas
};
static const uint8_t NUM_SEASONS = sizeof(SEASONS) / sizeof(SEASONS[0]);

bool dateInRange(uint8_t month, uint8_t day, const DateRange &r) {
  uint16_t md = (uint16_t)month * 100 + day;
  uint16_t start = (uint16_t)r.startMonth * 100 + r.startDay;
  uint16_t end = (uint16_t)r.endMonth * 100 + r.endDay;
  if (start <= end) {
    return md >= start && md <= end;
  }
  return md >= start || md <= end; // range wraps the new year
}

const Season *getActiveSeason(uint8_t month, uint8_t day) {
  for (uint8_t i = 0; i < NUM_SEASONS; i++) {
    if (dateInRange(month, day, SEASONS[i].range)) {
      return &SEASONS[i];
    }
  }
  return nullptr;
}
