#pragma once
#include <Arduino.h>
#include <FastLED.h>

enum EffectId : uint8_t {
  EFFECT_CHASE,
  EFFECT_SPARKLE,
  EFFECT_VERTICAL_WIPE,
  EFFECT_DIAGONAL_WIPE,
  EFFECT_STRIPES,
  EFFECT_GRADIENT,
  EFFECT_SOLID, // holds a single flat color -- mainly for testing
};

// Renders one frame of `effect` into `leds`. Effects that need two
// reference colors sample them from `palette`; `frame` is the animation
// clock and is expected to increment by 1 on every call.
void renderEffect(EffectId effect, CRGB *leds, uint16_t numLeds,
                   const TProgmemRGBPalette16 &palette, uint16_t frame);

// Sets the flat color EFFECT_SOLID holds (defaults to white). Takes a
// literal CRGB rather than a palette index -- for testing/calibration you
// usually want an exact, known color, not whatever a season's palette maps
// to.
void setSolidColor(CRGB color);
