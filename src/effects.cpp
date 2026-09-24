#include "effects.h"
#include "pixel_map.h"

// Two fixed sample points on the active palette, used as the "from"/"to"
// colors for wipes and stripes so those effects stay on-theme with
// whichever season picked the palette, instead of hardcoding literal colors.
static const uint8_t COLOR_FROM_INDEX = 0;
static const uint8_t COLOR_TO_INDEX = 170;
static const uint8_t STRIPE_ALT_INDEX = 128;

// ---- Linear: a single pixel chasing along the wire ----
static void renderChase(CRGB *leds, uint16_t numLeds,
                         const TProgmemRGBPalette16 &palette, uint16_t frame) {
  fadeToBlackBy(leds, numLeds, 32);
  leds[frame % numLeds] = ColorFromPalette(palette, frame & 0xFF);
}

// ---- 2D: falling sparkles ----
#define NUM_SPARKLES 8
struct Sparkle {
  int16_t x, y;
  uint8_t brightness;
  uint8_t paletteIndex;
  bool active;
};
static Sparkle sparkles[NUM_SPARKLES];

static void updateSparkles() {
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

static void renderSparkles(CRGB *leds, uint16_t numLeds,
                            const TProgmemRGBPalette16 &palette) {
  fadeToBlackBy(leds, numLeds, 40);
  for (uint16_t i = 0; i < numLeds; i++) {
    Point p = ledIndexToXY(i);
    for (uint8_t j = 0; j < NUM_SPARKLES; j++) {
      Sparkle &s = sparkles[j];
      if (s.active && p.x == s.x && p.y == s.y) {
        leds[i] = ColorFromPalette(palette, s.paletteIndex, s.brightness);
      }
    }
  }
}

// ---- 2D: hard-edge wipe up the y-axis ----
static void renderVerticalWipe(CRGB *leds, uint16_t numLeds,
                                const TProgmemRGBPalette16 &palette, uint16_t frame) {
  int16_t height = mappedHeight();
  int16_t cyclePause = 20; // frames to hold at the top before restarting
  int16_t threshold = (frame / 2) % (height + cyclePause);
  CRGB colorFrom = ColorFromPalette(palette, COLOR_FROM_INDEX);
  CRGB colorTo = ColorFromPalette(palette, COLOR_TO_INDEX);
  for (uint16_t i = 0; i < numLeds; i++) {
    Point p = ledIndexToXY(i);
    leds[i] = (p.y <= threshold) ? colorTo : colorFrom;
  }
}

// ---- 2D: hard-edge wipe along a 45-degree diagonal ----
static void renderDiagonalWipe(CRGB *leds, uint16_t numLeds,
                                const TProgmemRGBPalette16 &palette, uint16_t frame) {
  int16_t maxDiag = mappedWidth() + mappedHeight();
  int16_t cyclePause = 20;
  int16_t threshold = (frame / 2) % (maxDiag + cyclePause);
  CRGB colorFrom = ColorFromPalette(palette, COLOR_FROM_INDEX);
  CRGB colorTo = ColorFromPalette(palette, COLOR_TO_INDEX);
  for (uint16_t i = 0; i < numLeds; i++) {
    Point p = ledIndexToXY(i);
    int16_t diag = p.x + p.y;
    leds[i] = (diag <= threshold) ? colorTo : colorFrom;
  }
}

// ---- Linear: 10-pixel alternating stripes chasing along the wire ----
static void renderStripes(CRGB *leds, uint16_t numLeds,
                           const TProgmemRGBPalette16 &palette, uint16_t frame) {
  const uint16_t stripeWidth = 10;
  CRGB colorA = ColorFromPalette(palette, COLOR_FROM_INDEX);
  CRGB colorB = ColorFromPalette(palette, STRIPE_ALT_INDEX);
  for (uint16_t i = 0; i < numLeds; i++) {
    uint16_t block = (i + frame) / stripeWidth;
    leds[i] = (block % 2 == 0) ? colorA : colorB;
  }
}

// ---- 2D: slow-drifting vertical gradient, bottom to top ----
static void renderGradient(CRGB *leds, uint16_t numLeds,
                            const TProgmemRGBPalette16 &palette, uint16_t frame) {
  int16_t height = mappedHeight();
  uint8_t phase = (uint8_t)(frame / 8); // slow drift through the palette
  for (uint16_t i = 0; i < numLeds; i++) {
    Point p = ledIndexToXY(i);
    uint8_t index = (uint8_t)map(p.y, 0, height, 0, 255) + phase;
    leds[i] = ColorFromPalette(palette, index);
  }
}

void renderEffect(EffectId effect, CRGB *leds, uint16_t numLeds,
                   const TProgmemRGBPalette16 &palette, uint16_t frame) {
  switch (effect) {
  case EFFECT_CHASE:
    renderChase(leds, numLeds, palette, frame);
    break;
  case EFFECT_SPARKLE:
    updateSparkles();
    renderSparkles(leds, numLeds, palette);
    break;
  case EFFECT_VERTICAL_WIPE:
    renderVerticalWipe(leds, numLeds, palette, frame);
    break;
  case EFFECT_DIAGONAL_WIPE:
    renderDiagonalWipe(leds, numLeds, palette, frame);
    break;
  case EFFECT_STRIPES:
    renderStripes(leds, numLeds, palette, frame);
    break;
  case EFFECT_GRADIENT:
    renderGradient(leds, numLeds, palette, frame);
    break;
  }
}
