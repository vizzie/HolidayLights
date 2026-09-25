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
    CRGB::Red, CRGB::Red, CRGB::Orange, CRGB::Orange,
    CRGB::Yellow, CRGB::Yellow, CRGB::Green, CRGB::Green,
    CRGB::Cyan, CRGB::Cyan, CRGB::Blue, CRGB::Blue,
    CRGB::Purple, CRGB::Purple, CRGB::Magenta, CRGB::Magenta};

// Each season cycles through its own list of effects, one at a time --
// add/remove/reorder freely, order doesn't matter beyond that.
static const EffectId NewYearEffects[] = {EFFECT_SPARKLE, EFFECT_VERTICAL_WIPE, EFFECT_GRADIENT};
static const EffectId ValentinesEffects[] = {EFFECT_SPARKLE, EFFECT_GRADIENT};
static const EffectId StPatricksEffects[] = {EFFECT_CHASE, EFFECT_STRIPES};
static const EffectId SpringEffects[] = {EFFECT_GRADIENT, EFFECT_DIAGONAL_WIPE};
static const EffectId IndependenceEffects[] = {EFFECT_CHASE, EFFECT_SPARKLE, EFFECT_STRIPES, EFFECT_VERTICAL_WIPE, EFFECT_GRADIENT, EFFECT_DIAGONAL_WIPE};
static const EffectId HalloweenEffects[] = {EFFECT_SPARKLE, EFFECT_DIAGONAL_WIPE};
static const EffectId ThanksgivingEffects[] = {EFFECT_CHASE, EFFECT_GRADIENT};
static const EffectId ChristmasEffects[] = {EFFECT_SPARKLE, EFFECT_CHASE, EFFECT_VERTICAL_WIPE, EFFECT_STRIPES};
static const EffectId DefaultEffects[] = {EFFECT_CHASE, EFFECT_SPARKLE, EFFECT_STRIPES, EFFECT_VERTICAL_WIPE, EFFECT_GRADIENT, EFFECT_DIAGONAL_WIPE};

#define EFFECT_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

// Add more holidays/seasons here as needed -- each is just a date range, a
// palette, and which effects to cycle through. Kept in calendar order for
// readability; matching only depends on ranges not overlapping.
static const Season SEASONS[] = {
    {{12, 31, 1, 2},  &NewYearPalette,         NewYearEffects,       EFFECT_COUNT(NewYearEffects)},
    {{2, 10, 2, 14},  &ValentinesPalette,      ValentinesEffects,    EFFECT_COUNT(ValentinesEffects)},
    {{3, 15, 3, 17},  &StPatricksPalette,      StPatricksEffects,    EFFECT_COUNT(StPatricksEffects)},
    {{3, 20, 4, 20},  &SpringPalette,          SpringEffects,        EFFECT_COUNT(SpringEffects)},
    {{6, 28, 7, 5},   &IndependenceDayPalette, IndependenceEffects,  EFFECT_COUNT(IndependenceEffects)},
    {{10, 1, 10, 31}, &HalloweenPalette,       HalloweenEffects,     EFFECT_COUNT(HalloweenEffects)},
    {{11, 1, 11, 27}, &ThanksgivingPalette,    ThanksgivingEffects,  EFFECT_COUNT(ThanksgivingEffects)},
    {{12, 1, 12, 30}, &ChristmasPalette,       ChristmasEffects,     EFFECT_COUNT(ChristmasEffects)},
};
static const uint8_t NUM_SEASONS = sizeof(SEASONS) / sizeof(SEASONS[0]);

static const Season DefaultSeason = {{0, 0, 0, 0}, &DefaultPalette, DefaultEffects, EFFECT_COUNT(DefaultEffects)};

bool dateInRange(uint8_t month, uint8_t day, const DateRange &r) {
  uint16_t md = (uint16_t)month * 100 + day;
  uint16_t start = (uint16_t)r.startMonth * 100 + r.startDay;
  uint16_t end = (uint16_t)r.endMonth * 100 + r.endDay;
  if (start <= end) {
    return md >= start && md <= end;
  }
  return md >= start || md <= end; // range wraps the new year
}

const Season &getActiveSeason(uint8_t month, uint8_t day) {
  for (uint8_t i = 0; i < NUM_SEASONS; i++) {
    if (dateInRange(month, day, SEASONS[i].range)) {
      return SEASONS[i];
    }
  }
  return DefaultSeason; // also covers month == 0, i.e. before the real date arrives
}
