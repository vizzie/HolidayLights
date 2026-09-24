#include <Arduino.h>
#include <FastLED.h>
#include "pixel_map.h"
#include "current_date.h"
#include "seasons.h"

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
#define FAKE_DATE_MONTH 12
#define FAKE_DATE_DAY 15

CRGB leds[NUM_LEDS];

CurrentDate today = {0, 0, false};
const TProgmemRGBPalette16 *currentPalette = &DefaultPalette;
EffectId currentEffect = EFFECT_CHASE;

void applySeasonForToday() {
  if (!today.valid) {
    return;
  }
  const Season *season = getActiveSeason(today.month, today.day);
  if (season != nullptr) {
    currentPalette = season->palette;
    currentEffect = season->effect;
  } else {
    currentPalette = &DefaultPalette;
    currentEffect = EFFECT_CHASE;
  }
}

// ---- Linear effect: a chase along the wire, corner to corner ----
void renderChase(uint16_t frame) {
  fadeToBlackBy(leds, NUM_LEDS, 32);
  leds[frame % NUM_LEDS] = ColorFromPalette(*currentPalette, frame & 0xFF);
}

// ---- 2D effect: falling sparkles ----
#define NUM_SPARKLES 8
struct Sparkle {
  int16_t x, y;
  uint8_t brightness;
  uint8_t paletteIndex;
  bool active;
};
Sparkle sparkles[NUM_SPARKLES];

void updateSparkles() {
  for (uint8_t i = 0; i < NUM_SPARKLES; i++) {
    Sparkle &s = sparkles[i];
    if (!s.active) {
      if (random8() < 20) {
        s.x = random16(mappedWidth() + 1);
        s.y = mappedHeight();
        s.brightness = 255;
        s.paletteIndex = random8();
        s.active = true;
      }
      continue;
    }
    s.y -= 1;
    s.brightness = qsub8(s.brightness, 40);
    if (s.y < 0 || s.brightness == 0) {
      s.active = false;
    }
  }
}

void renderSparkles() {
  fadeToBlackBy(leds, NUM_LEDS, 40);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    Point p = ledIndexToXY(i);
    for (uint8_t j = 0; j < NUM_SPARKLES; j++) {
      Sparkle &s = sparkles[j];
      if (s.active && p.x == s.x && p.y == s.y) {
        leds[i] = ColorFromPalette(*currentPalette, s.paletteIndex, s.brightness);
      }
    }
  }
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

  if (currentEffect == EFFECT_CHASE) {
    renderChase(frame);
  } else {
    updateSparkles();
    renderSparkles();
  }

  FastLED.show();
  frame++;
  delay(30);
}
