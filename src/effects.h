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
};

// Renders one frame of `effect` into `leds`. Effects that need two
// reference colors sample them from `palette`; `frame` is the animation
// clock and is expected to increment by 1 on every call.
void renderEffect(EffectId effect, CRGB *leds, uint16_t numLeds,
                   const TProgmemRGBPalette16 &palette, uint16_t frame);
