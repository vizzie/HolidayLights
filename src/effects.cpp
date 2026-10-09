#include "effects.h"
#include "pixel_map.h"

// ---- Linear: multiple independent pixels chasing along the wire ----
// Each has its own fixed-point position/velocity (avoids float math on the
// 32u4) and bounces off both ends. When two land on the same LED in the
// same frame, their colors multiply together instead of one overwriting
// the other -- since these are discrete LEDs, "overlap" can only mean
// "same index, same frame," there's no partial/sub-pixel overlap to blend.
#define MIN_CHASE_PIXELS 2
#define MAX_CHASE_PIXELS 6
#define CHASE_POS_SHIFT 4 // fixed-point: 16 sub-positions per LED

struct ChasePixel {
  int16_t position; // fixed-point, 0..(numLeds-1)<<CHASE_POS_SHIFT
  int16_t velocity; // fixed-point units/frame; sign = direction
  uint8_t paletteIndex;
};
static ChasePixel chasePixels[MAX_CHASE_PIXELS];
static uint8_t numChasePixels = 0;

static void startChasePixels(uint16_t numLeds) {
  numChasePixels = MIN_CHASE_PIXELS + random8(MAX_CHASE_PIXELS - MIN_CHASE_PIXELS + 1);
  int16_t maxPos = (int16_t)((numLeds - 1) << CHASE_POS_SHIFT);
  for (uint8_t k = 0; k < numChasePixels; k++) {
    ChasePixel &p = chasePixels[k];
    // Spread starting positions evenly so they don't all bunch up at spawn.
    p.position = (numChasePixels > 1)
                     ? (int16_t)((int32_t)maxPos * k / (numChasePixels - 1))
                     : 0;
    uint8_t speed = 4 + random8(21); // 0.25 - 1.5 px/frame
    p.velocity = random8(2) ? (int16_t)speed : (int16_t)(-speed);
    p.paletteIndex = (uint8_t)((255UL * k) / numChasePixels);
  }
}

static void renderChase(CRGB *leds, uint16_t numLeds,
                         const TProgmemRGBPalette16 &palette, bool justStarted) {
  if (justStarted) {
    startChasePixels(numLeds);
  }

  fadeToBlackBy(leds, numLeds, 32);

  int16_t maxPos = (int16_t)((numLeds - 1) << CHASE_POS_SHIFT);

  for (uint8_t k = 0; k < numChasePixels; k++) {
    ChasePixel &p = chasePixels[k];
    uint16_t idx = (uint16_t)(p.position >> CHASE_POS_SHIFT);
    CRGB color = ColorFromPalette(palette, p.paletteIndex);

    // Add into whatever's already there -- another pixel's head drawn
    // earlier this frame, or a still-fading trail from a recent pass --
    // instead of overwriting it, so overlapping tails brighten/blend
    // together instead of one replacing the other. Saturates at 255 per
    // channel (CRGB::operator+=), so it can't wrap around and glitch.
    leds[idx] += color;

    p.position += p.velocity;
    if (p.position < 0) {
      p.position = -p.position;
      p.velocity = (int16_t)(-p.velocity);
    } else if (p.position > maxPos) {
      p.position = (int16_t)(2 * maxPos - p.position);
      p.velocity = (int16_t)(-p.velocity);
    }
  }
}

// ---- 2D: dripping sparkles ----
// Two layers on top of a fading background:
//  - Drips start at an opening's top corner and run down its left or right
//    side all the way to the bottom, flickering as they fall. They walk the
//    strip by LED index rather than (x, y): going down the left side is
//    index - 1, going down the right side is index + 1.
//  - Twinkles flash random LEDs anywhere on the strip and fade out. They
//    need no state -- the per-frame fade does the decay.
#define NUM_DRIPS 10
#define DRIP_SPAWN_CHANCE 8    // out of 256, per idle drip slot per frame
#define TWINKLES_PER_FRAME 3   // max new twinkles per frame
#define TWINKLE_CHANCE 160     // out of 256, per twinkle attempt

struct Drip {
  uint16_t ledIndex;
  int8_t direction;   // -1: down a left side, +1: down a right side
  uint8_t remaining;  // LEDs left to travel before reaching the bottom
  uint8_t speed;      // fraction of an LED per frame, out of 256
  uint8_t subStep;    // accumulates speed; steps one LED on overflow
  uint8_t paletteIndex;
  bool active;
};
static Drip drips[NUM_DRIPS];

static void spawnDrip(Drip &d) {
  uint8_t shape = random8(NUM_SHAPES);
  uint16_t start = 0;
  for (uint8_t s = 0; s < shape; s++) {
    start += (uint16_t)SHAPES[s].leftCount + SHAPES[s].topCount + SHAPES[s].rightCount;
  }
  const ShapeGeometry &g = SHAPES[shape];
  if (random8(2)) {
    d.ledIndex = start + g.leftCount - 1; // top of the left side
    d.direction = -1;
    d.remaining = g.leftCount - 1;
  } else {
    d.ledIndex = start + g.leftCount + g.topCount; // top of the right side
    d.direction = 1;
    d.remaining = g.rightCount - 1;
  }
  d.speed = 128 + random8(128); // 0.5 - 1 LED/frame
  d.subStep = 0;
  d.paletteIndex = random8();
  d.active = true;
}

static void renderSparkles(CRGB *leds, uint16_t numLeds,
                            const TProgmemRGBPalette16 &palette) {
  fadeToBlackBy(leds, numLeds, 40);

  for (uint8_t k = 0; k < TWINKLES_PER_FRAME; k++) {
    if (random8() < TWINKLE_CHANCE) {
      leds[random16(numLeds)] = ColorFromPalette(palette, random8());
    }
  }

  for (uint8_t i = 0; i < NUM_DRIPS; i++) {
    Drip &d = drips[i];
    if (!d.active) {
      if (random8() < DRIP_SPAWN_CHANCE) {
        spawnDrip(d);
      }
      continue;
    }
    // Random flicker instead of a steady fade, so it keeps sparkling all
    // the way down; the background fade leaves a short trail behind it.
    leds[d.ledIndex] = ColorFromPalette(palette, d.paletteIndex, 160 + random8(96));

    uint16_t acc = (uint16_t)d.subStep + d.speed;
    d.subStep = (uint8_t)acc;
    if (acc > 255) {
      if (d.remaining == 0) {
        d.active = false; // reached the bottom
      } else {
        d.ledIndex += d.direction;
        d.remaining--;
      }
    }
  }
}

// ---- Shared wipe state: step through the palette one color at a time ----
// Each wipe sweeps the next palette color over the previous one, so the
// sequence goes color[0] -> color[1] -> color[2] ... and wraps back to the
// start. Step 16 across the 0-255 index range lands exactly on each of the
// palette's 16 defined colors in turn.
static const uint8_t WIPE_COLOR_STEP = 16;

struct WipeState {
  int16_t threshold;
  uint16_t holdCounter;
  uint8_t colorFromIndex;
  uint8_t colorToIndex;
  bool started;
};

// Advances to the next palette color, skipping entries identical to the
// current one -- several palettes repeat a color back to back (Red, Red,
// Orange, Orange, ...), and wiping a color over itself just looks stalled.
static void advanceWipeColors(WipeState &w, const TProgmemRGBPalette16 &palette) {
  w.colorFromIndex = w.colorToIndex;
  CRGB from = ColorFromPalette(palette, w.colorFromIndex);
  for (uint8_t tries = 0; tries < 16; tries++) {
    w.colorToIndex += WIPE_COLOR_STEP;
    CRGB to = ColorFromPalette(palette, w.colorToIndex);
    if (to.r != from.r || to.g != from.g || to.b != from.b) {
      break;
    }
  }
  w.threshold = 0;
  w.holdCounter = 0;
}

// ---- 2D: hard-edge wipe up the y-axis ----
static const uint16_t VERTICAL_WIPE_HOLD_FRAMES = 40; // hold at the top before the next wipe
static WipeState vertWipe = {0, 0, 0, 0, false};

static void renderVerticalWipe(CRGB *leds, uint16_t numLeds,
                                const TProgmemRGBPalette16 &palette, uint16_t frame) {
  if (!vertWipe.started) {
    vertWipe.started = true;
    advanceWipeColors(vertWipe, palette);
  }

  int16_t height = mappedHeight();
  CRGB colorFrom = ColorFromPalette(palette, vertWipe.colorFromIndex);
  CRGB colorTo = ColorFromPalette(palette, vertWipe.colorToIndex);
  for (uint16_t i = 0; i < numLeds; i++) {
    Point p = ledIndexToXY(i);
    leds[i] = (p.y <= vertWipe.threshold) ? colorTo : colorFrom;
  }

  if (vertWipe.threshold < height) {
    if (frame & 1) { // one row every other frame
      vertWipe.threshold++;
    }
  } else if (vertWipe.holdCounter < VERTICAL_WIPE_HOLD_FRAMES) {
    vertWipe.holdCounter++;
  } else {
    advanceWipeColors(vertWipe, palette);
  }
}

// ---- 2D: hard-edge wipe along a 45-degree diagonal, corner to corner ----
// Each wipe starts from a randomly chosen corner and sweeps to the opposite
// one, then holds briefly before picking a new corner and advancing to the
// next palette color.
static const uint16_t DIAGONAL_WIPE_HOLD_FRAMES = 20;

static WipeState diagWipe = {0, 0, 0, 0, false};
static int16_t diagWipeMax = 0;
static uint8_t diagWipeCorner = 0; // 0: BL->TR, 1: TR->BL, 2: BR->TL, 3: TL->BR

// Distance from `p` to the wipe's starting corner, measured along the
// chosen 45-degree diagonal -- 0 at the start corner, diagWipeMax at the
// opposite one.
static int16_t diagonalDistance(const Point &p, uint8_t cornerIndex,
                                 int16_t maxWidth, int16_t maxHeight) {
  switch (cornerIndex) {
  case 0:
    return p.x + p.y; // bottom-left -> top-right
  case 1:
    return (maxWidth - p.x) + (maxHeight - p.y); // top-right -> bottom-left
  case 2:
    return (maxWidth - p.x) + p.y; // bottom-right -> top-left
  default:
    return p.x + (maxHeight - p.y); // top-left -> bottom-right
  }
}

static void startNextDiagonalWipe(const TProgmemRGBPalette16 &palette) {
  diagWipeCorner = random8(4);
  diagWipeMax = mappedCompressedWidth() + mappedHeight();
  advanceWipeColors(diagWipe, palette);
}

static void renderDiagonalWipe(CRGB *leds, uint16_t numLeds,
                                const TProgmemRGBPalette16 &palette, uint16_t frame) {
  (void)frame; // progress is tracked in diagWipe, not the global frame clock
  if (!diagWipe.started) {
    diagWipe.started = true;
    startNextDiagonalWipe(palette);
  }

  CRGB colorFrom = ColorFromPalette(palette, diagWipe.colorFromIndex);
  CRGB colorTo = ColorFromPalette(palette, diagWipe.colorToIndex);
  // Compressed x -- the wipe treats the three openings as one continuous
  // strip, without the physically real but unlit gaps between them eating
  // into the sweep's travel (see ledIndexToCompressedXY).
  int16_t maxWidth = mappedCompressedWidth();
  int16_t maxHeight = mappedHeight();
  for (uint16_t i = 0; i < numLeds; i++) {
    Point p = ledIndexToCompressedXY(i);
    int16_t d = diagonalDistance(p, diagWipeCorner, maxWidth, maxHeight);
    leds[i] = (d <= diagWipe.threshold) ? colorTo : colorFrom;
  }

  if (diagWipe.threshold < diagWipeMax) {
    diagWipe.threshold++;
  } else if (diagWipe.holdCounter < DIAGONAL_WIPE_HOLD_FRAMES) {
    diagWipe.holdCounter++;
  } else {
    startNextDiagonalWipe(palette);
  }
}

// ---- Linear: 10-pixel stripes chasing along the wire ----
// Each stripe is the next of the palette's 16 colors in turn, with
// back-to-back repeats (Red, Red, ...) collapsed so neighboring stripes
// always differ instead of merging into one double-width stripe.
//
// Speed drifts: it eases toward a randomly chosen target by one small step
// per frame, and picks a new target once it arrives, so the stripes gently
// speed up and slow down without ever lurching. Speeds are in 1/256ths of
// an LED per frame.
static const uint8_t STRIPE_MIN_SPEED = 32;  // ~0.13 LED/frame, ~4 LEDs/s
static const uint8_t STRIPE_MAX_SPEED = 255; // 1 LED/frame, ~30 LEDs/s

static void renderStripes(CRGB *leds, uint16_t numLeds,
                           const TProgmemRGBPalette16 &palette) {
  const uint16_t stripeWidth = 10;
  static uint16_t offset = 0; // own counter, so it wraps cleanly at a stripe-cycle boundary
  static uint8_t subStep = 0;
  static uint8_t speed = 128;
  static uint8_t targetSpeed = 128;

  CRGB colors[16];
  uint8_t numColors = 0;
  for (uint8_t k = 0; k < 16; k++) {
    CRGB c = ColorFromPalette(palette, k * 16);
    if (numColors == 0 || c.r != colors[numColors - 1].r ||
        c.g != colors[numColors - 1].g || c.b != colors[numColors - 1].b) {
      colors[numColors++] = c;
    }
  }
  // The sequence repeats, so the last color also borders the first.
  if (numColors > 1 && colors[numColors - 1].r == colors[0].r &&
      colors[numColors - 1].g == colors[0].g && colors[numColors - 1].b == colors[0].b) {
    numColors--;
  }

  uint16_t cycleLength = numColors * stripeWidth;
  offset %= cycleLength;
  for (uint16_t i = 0; i < numLeds; i++) {
    uint16_t block = ((i + offset) % cycleLength) / stripeWidth;
    leds[i] = colors[block];
  }

  // One step per frame: a full min-to-max swing takes ~5 seconds.
  if (speed < targetSpeed) {
    speed++;
  } else if (speed > targetSpeed) {
    speed--;
  } else {
    targetSpeed = STRIPE_MIN_SPEED + random8(STRIPE_MAX_SPEED - STRIPE_MIN_SPEED + 1);
  }
  uint16_t acc = (uint16_t)subStep + speed;
  subStep = (uint8_t)acc;
  offset += acc >> 8;
}

// ---- 2D: slow-drifting vertical gradient, bottom to top ----
static void renderGradient(CRGB *leds, uint16_t numLeds,
                            const TProgmemRGBPalette16 &palette, uint16_t frame) {
  int16_t height = mappedHeight();
  uint8_t phase = (uint8_t)(frame / 4); // drift through the palette, ~30s per full cycle
  for (uint16_t i = 0; i < numLeds; i++) {
    Point p = ledIndexToXY(i);
    uint8_t index = (uint8_t)map(p.y, 0, height, 0, 255) + phase;
    leds[i] = ColorFromPalette(palette, index);
  }
}

// ---- Solid: flat color, no motion -- mainly for testing/calibration ----
static CRGB solidColor = CRGB::White;

void setSolidColor(CRGB color) {
  solidColor = color;
}

static void renderSolid(CRGB *leds, uint16_t numLeds) {
  fill_solid(leds, numLeds, solidColor);
}

void renderEffect(EffectId effect, CRGB *leds, uint16_t numLeds,
                   const TProgmemRGBPalette16 &palette, uint16_t frame) {
  static EffectId lastEffect = (EffectId)0xFF; // sentinel: no effect run yet
  bool justStarted = (effect != lastEffect);
  lastEffect = effect;

  switch (effect) {
  case EFFECT_CHASE:
    renderChase(leds, numLeds, palette, justStarted);
    break;
  case EFFECT_SPARKLE:
    renderSparkles(leds, numLeds, palette);
    break;
  case EFFECT_VERTICAL_WIPE:
    renderVerticalWipe(leds, numLeds, palette, frame);
    break;
  case EFFECT_DIAGONAL_WIPE:
    renderDiagonalWipe(leds, numLeds, palette, frame);
    break;
  case EFFECT_STRIPES:
    renderStripes(leds, numLeds, palette);
    break;
  case EFFECT_GRADIENT:
    renderGradient(leds, numLeds, palette, frame);
    break;
  case EFFECT_SOLID:
    renderSolid(leds, numLeds);
    break;
  }
}
