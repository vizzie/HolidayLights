#include <Arduino.h>
#include <FastLED.h>
#include "pixel_map.h"
#include "current_date.h"
#include "seasons.h"
#include "effects.h"

// Strip configuration — adjust to match your hardware.
#define DATA_PIN 6
#define NUM_LEDS 390
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

#define BRIGHTNESS 128 // 0-255

// Uncomment to preview a season's look without waiting on the real
// calendar date. Skips the Bridge/Linux date lookup entirely (and its
// up-to-a-minute setup() delay), so it's also the fast path for iterating
// on palettes/effects. Comment back out before flashing for real use.
// #define FAKE_DATE_MONTH 7
// #define FAKE_DATE_DAY 4

// Uncomment to force a specific effect regardless of season/cycling, for
// previewing one effect at a time on the real hardware. Comment back out
// afterward.
// #define FORCE_EFFECT EFFECT_SOLID

// Only used when FORCE_EFFECT is EFFECT_SOLID -- the color it holds.
// #define FORCE_SOLID_COLOR CRGB::White

// Uncomment to shrink the effect-cycle interval for previewing (e.g. 5000 =
// 5 seconds instead of 5 minutes). Comment back out before real use.
// #define FORCE_EFFECT_CYCLE_MS 5000UL

#ifdef FORCE_EFFECT_CYCLE_MS
const uint32_t EFFECT_CYCLE_MS = FORCE_EFFECT_CYCLE_MS;
#else
const uint32_t EFFECT_CYCLE_MS = 5UL * 60UL * 1000UL; // advance every 5 minutes
#endif

CRGB leds[NUM_LEDS];

CurrentDate today = {0, 0, false};
const Season *activeSeason = nullptr;
const TProgmemRGBPalette16 *currentPalette = &DefaultPalette;
EffectId currentEffect = EFFECT_CHASE;
uint8_t effectIndex = 0;
uint32_t effectCycleStartMs = 0;

void applySeasonForToday() {
  activeSeason = &getActiveSeason(today.month, today.day);
  currentPalette = activeSeason->palette;
  effectIndex = 0;
  currentEffect = activeSeason->effects[0];
  effectCycleStartMs = millis();
}

uint16_t frame = 0;

void setup() {
#ifdef FAKE_DATE_MONTH
  today.month = FAKE_DATE_MONTH;
  today.day = FAKE_DATE_DAY;
  today.valid = true;
#else
  currentDateBegin();
  currentDateUpdate(today, 0); // force an initial read before the first render
#endif
  applySeasonForToday();

#ifdef FORCE_SOLID_COLOR
  setSolidColor(FORCE_SOLID_COLOR);
#endif

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

void loop() {
#ifndef FAKE_DATE_MONTH
  if (currentDateUpdate(today, 3600000UL)) { // re-check the date hourly
    applySeasonForToday();
  }
#endif

  if (millis() - effectCycleStartMs >= EFFECT_CYCLE_MS) {
    effectIndex = (effectIndex + 1) % activeSeason->numEffects;
    currentEffect = activeSeason->effects[effectIndex];
    effectCycleStartMs = millis();
  }

#ifdef FORCE_EFFECT
  currentEffect = FORCE_EFFECT;
#endif

  renderEffect(currentEffect, leds, NUM_LEDS, *currentPalette, frame);

  FastLED.show();
  frame++;
  delay(30);
}
