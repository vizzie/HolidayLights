#include <Arduino.h>
#include <FastLED.h>
#include <Bridge.h>
#include "pixel_map.h"
#include "current_date.h"
#include "seasons.h"
#include "effects.h"

// Strip configuration — adjust to match your hardware.
#define DATA_PIN 6
#define NUM_LEDS 390
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

#define BRIGHTNESS 50 // 0-255

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

// Waits up to ~3 minutes (36 x 5s) for the Yun's clock to sync at power-up.
const uint8_t DATE_RETRY_LIMIT = 36;
const uint16_t DATE_RETRY_MS = 5000;

CRGB leds[NUM_LEDS];

CurrentDate today = {0, 0, false, 0};
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

// Publishes status to the Yun's Linux side via Bridge.put(). View it at
// http://arduino.local/data/get (all keys, JSON) or /data/get/<key>. Needs
// "REST API access" set to open on the Yun's web config page (or enter the
// Yun password when prompted). Each put is a blocking Bridge round trip, so
// this is only called on changes, never per frame.
void publishStatus() {
  char date[8];
  snprintf(date, sizeof(date), "%02u-%02u", today.month, today.day);
  Bridge.put("date", date);
  Bridge.put("date_valid", today.valid ? "yes" : "no");
  Bridge.put("season", activeSeason->name);
  // Stack buffers, not String: with RAM this tight, a failed heap
  // allocation silently yields an empty String.
  char num[12];
  snprintf(num, sizeof(num), "%u", (unsigned)currentEffect);
  Bridge.put("effect", num);
  snprintf(num, sizeof(num), "%u", (unsigned)effectIndex);
  Bridge.put("effect_index", num);
  ltoa((long)(millis() / 1000UL), num, 10);
  Bridge.put("uptime_s", num);
}

uint16_t frame = 0;

void setup() {
#ifdef FAKE_DATE_MONTH
  today.month = FAKE_DATE_MONTH;
  today.day = FAKE_DATE_DAY;
  today.valid = true;
#else
  currentDateBegin();
  // The clock may not be NTP-synced yet at power-up, so keep retrying until
  // it reads a sane date. If it never does, today stays invalid and the
  // Default season is used.
  for (uint8_t attempt = 0; attempt < DATE_RETRY_LIMIT; attempt++) {
    currentDateUpdate(today, 0); // 0 = force a read
    if (today.valid) {
      break;
    }
    delay(DATE_RETRY_MS);
  }
#endif
  applySeasonForToday();

#ifdef FORCE_SOLID_COLOR
  setSolidColor(FORCE_SOLID_COLOR);
#endif

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

#ifndef FAKE_DATE_MONTH
  publishStatus();
#endif
}

void loop() {
  if (millis() - effectCycleStartMs >= EFFECT_CYCLE_MS) {
    effectIndex = (effectIndex + 1) % activeSeason->numEffects;
    currentEffect = activeSeason->effects[effectIndex];
    effectCycleStartMs = millis();
#ifndef FAKE_DATE_MONTH
    publishStatus();
#endif
  }

#ifdef FORCE_EFFECT
  currentEffect = FORCE_EFFECT;
#endif

  renderEffect(currentEffect, leds, NUM_LEDS, *currentPalette, frame);

  FastLED.show();
  frame++;
  delay(30);
}
