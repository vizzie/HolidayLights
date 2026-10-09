#include "effects.h"
#include "pixel_map.h"

static bool sameColor(const CRGB &a, const CRGB &b) {
  return a.r == b.r && a.g == b.g && a.b == b.b;
}

// 0 -> 255 -> 0 over one 0-255 cycle of t; a cheap smooth-ish oscillator.
static uint8_t triangle8(uint8_t t) {
  return t < 128 ? (uint8_t)(t * 2) : (uint8_t)((255 - t) * 2);
}

// Collects the palette indices (0, 16, ... 240) whose colors pass `test`,
// so effects can pick e.g. "the warm colors" from whatever palette the
// season uses. Falls back to every index if none pass. Returns the count.
typedef bool (*ColorTest)(const CRGB &c);
static uint8_t collectPaletteIndices(const TProgmemRGBPalette16 &palette, ColorTest test,
                                     uint8_t out[16]) {
  uint8_t n = 0;
  for (uint8_t k = 0; k < 16; k++) {
    if (test(ColorFromPalette(palette, k * 16))) {
      out[n++] = k * 16;
    }
  }
  if (n == 0) {
    for (uint8_t k = 0; k < 16; k++) {
      out[n++] = k * 16;
    }
  }
  return n;
}

static bool isWarm(const CRGB &c) { return c.r > c.b && c.r >= c.g; }
static bool isPurple(const CRGB &c) { return c.b > c.g && c.b >= c.r; }

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

static void spawnDrip(Drip &d, uint8_t paletteIndex) {
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
  d.paletteIndex = paletteIndex;
  d.active = true;
}

// Moves a drip down its side at its own speed; deactivates it at the bottom.
static void advanceDrip(Drip &d) {
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
        spawnDrip(d, random8());
      }
      continue;
    }
    // Random flicker instead of a steady fade, so it keeps sparkling all
    // the way down; the background fade leaves a short trail behind it.
    leds[d.ledIndex] = ColorFromPalette(palette, d.paletteIndex, 160 + random8(96));
    advanceDrip(d);
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
    if (!sameColor(ColorFromPalette(palette, w.colorToIndex), from)) {
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
    if (numColors == 0 || !sameColor(c, colors[numColors - 1])) {
      colors[numColors++] = c;
    }
  }
  // The sequence repeats, so the last color also borders the first.
  if (numColors > 1 && sameColor(colors[numColors - 1], colors[0])) {
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

// ---- 2D: haunted-house lightning storm ----
// Every LED flickers like candlelight in the palette's warm colors, each at
// its own pace. Then lightning strikes: a first flash, a moment of
// darkness, then a stronger second flash. Purple slime oozes down from the
// tops of the openings until it covers the whole strip, sits there for a
// few seconds, then slowly fades back into the candlelight. The next strike
// comes 10-30 seconds after that. The flash is literal white -- no palette
// color reads as lightning.
#define STORM_MIN_GAP_FRAMES 333  // ~10s at ~30ms/frame
#define STORM_MAX_GAP_FRAMES 1000 // ~30s
#define SLIME_SPEED 5             // 1/16ths of a row per frame: top to bottom in ~8s
#define SLIME_MAX_LAG 15          // rows some sides trail behind others, for an uneven drip
#define SLIME_HOLD_FRAMES 167     // ~5s fully covered
#define SLIME_FADE_FRAMES 133     // ~4s fading back to candlelight

// Flash brightness for each frame of a strike. After the last entry the
// flash decays smoothly instead.
static const uint8_t STRIKE_LEVELS[] = {180, 90, 0, 0, 0, 255};
static const uint8_t STRIKE_LENGTH = sizeof(STRIKE_LEVELS);

enum StormPhase : uint8_t { STORM_CANDLES, STORM_STRIKE, SLIME_FALL, SLIME_HOLD, SLIME_FADE };

struct StormState {
  uint8_t phase;
  uint16_t timer;    // frames to the next strike, or frames into the current phase
  uint16_t progress; // slime depth from the top, in 1/16ths of a row
  uint8_t flash;
};
static StormState storm = {STORM_CANDLES, 0, 0, 0};

// Linear blend from a to b; amount 0 = all a, 255 = (almost) all b.
static CRGB mixColor(const CRGB &a, const CRGB &b, uint8_t amount) {
  return CRGB((uint8_t)(a.r + (((int16_t)b.r - a.r) * amount >> 8)),
              (uint8_t)(a.g + (((int16_t)b.g - a.g) * amount >> 8)),
              (uint8_t)(a.b + (((int16_t)b.b - a.b) * amount >> 8)));
}

static void renderStorm(CRGB *leds, uint16_t numLeds, const TProgmemRGBPalette16 &palette,
                        uint16_t frame, bool justStarted) {
  if (justStarted) {
    storm.phase = STORM_CANDLES;
    storm.timer = 100; // first strike after ~3s, so it shows up quickly
    storm.flash = 0;
  }

  bool blackout = false;
  switch (storm.phase) {
  case STORM_CANDLES:
    if (--storm.timer == 0) {
      storm.phase = STORM_STRIKE;
    }
    break;
  case STORM_STRIKE:
    storm.flash = STRIKE_LEVELS[storm.timer];
    blackout = (storm.flash == 0); // the dark gap between the two flashes
    if (++storm.timer == STRIKE_LENGTH) {
      storm.phase = SLIME_FALL;
      storm.progress = 0;
    }
    break;
  case SLIME_FALL:
    storm.progress += SLIME_SPEED;
    if (storm.progress >= (uint16_t)(mappedHeight() + SLIME_MAX_LAG + 1) * 16) {
      storm.phase = SLIME_HOLD;
      storm.timer = 0;
    }
    break;
  case SLIME_HOLD:
    if (++storm.timer == SLIME_HOLD_FRAMES) {
      storm.phase = SLIME_FADE;
      storm.timer = 0;
    }
    break;
  case SLIME_FADE:
    if (++storm.timer == SLIME_FADE_FRAMES) {
      storm.phase = STORM_CANDLES;
      storm.timer = STORM_MIN_GAP_FRAMES +
                    random16(STORM_MAX_GAP_FRAMES - STORM_MIN_GAP_FRAMES + 1);
    }
    break;
  }
  if (storm.phase != STORM_STRIKE) {
    storm.flash = (uint8_t)(((uint16_t)storm.flash * 220) >> 8);
  }

  bool slime = (storm.phase == SLIME_FALL || storm.phase == SLIME_HOLD ||
                storm.phase == SLIME_FADE);
  uint8_t fadeAmount = (storm.phase == SLIME_FADE)
                           ? (uint8_t)((uint32_t)storm.timer * 255 / SLIME_FADE_FRAMES)
                           : 0;
  int16_t height = mappedHeight();
  int16_t slimeRows = (int16_t)(storm.progress / 16);
  uint8_t warm[16];
  uint8_t numWarm = collectPaletteIndices(palette, isWarm, warm);
  uint8_t purples[16];
  uint8_t numPurples = collectPaletteIndices(palette, isPurple, purples);

  for (uint16_t i = 0; i < numLeds; i++) {
    // Cheap per-LED hash gives each LED its own fixed colors, flicker
    // speeds and phase, so no per-LED state is needed.
    uint8_t h = (uint8_t)(i * 167 + 71);
    CRGB candle = CRGB::Black;
    if (!blackout) {
      uint8_t flicker = 60 + triangle8((uint8_t)(frame * (3 + (h & 3))) + h) / 4 +
                        triangle8((uint8_t)(frame * (7 + (h >> 6))) + (uint8_t)(h * 3)) / 5;
      if (random8() < 3) {
        flicker /= 2; // occasional sputter
      }
      candle = ColorFromPalette(palette, warm[h % numWarm], flicker);
    }

    CRGB c = candle;
    if (slime) {
      Point p = ledIndexToXY(i);
      // Each vertical side (one x per side) gets its own lag, so the slime
      // runs down the sides unevenly instead of as one flat line.
      uint8_t lag = (uint8_t)((p.x * 37 + 11) % (SLIME_MAX_LAG + 1));
      int16_t reach = slimeRows - lag; // rows covered on this side
      int16_t depth = height - p.y;    // rows below the top
      if (depth <= reach) {
        // Slow wet shimmer; the leading edge is a brighter, heavier drop.
        uint8_t level = 150 + triangle8((uint8_t)(frame + h)) / 5;
        if (storm.phase == SLIME_FALL && reach - depth < 2) {
          level = 255;
        }
        CRGB goo = ColorFromPalette(palette, purples[h % numPurples], level);
        c = mixColor(goo, candle, fadeAmount);
      }
    }
    c += CRGB(storm.flash, storm.flash, storm.flash);
    leds[i] = c;
  }
}

// ---- 2D: heartbeat ----
// The whole house pulses in the palette's reddest color with a lub-dub
// rhythm that slowly speeds up, then flatlines into darkness for a few
// seconds before starting again.
#define HEART_START_PERIOD 45     // frames per beat at the start (~44 bpm)
#define HEART_END_PERIOD 16       // frames per beat at the end (~125 bpm)
#define HEART_DUB_DELAY 7         // frames from "lub" to "dub"
#define HEART_DECAY 35            // brightness lost per frame after each thump
#define HEART_GLOW 24             // resting brightness between beats
#define HEART_FLATLINE_FRAMES 120 // ~3.6s of darkness before restarting

struct HeartState {
  uint8_t period;
  uint8_t beatFrame;
  uint8_t flatline; // frames of darkness left; 0 while beating
};
static HeartState heart = {HEART_START_PERIOD, 0, 0};

// Brightness `t` frames after a thump that peaked at `peak`.
static uint8_t thump(uint8_t t, uint8_t peak) {
  uint16_t drop = (uint16_t)t * HEART_DECAY;
  return drop < peak ? (uint8_t)(peak - drop) : 0;
}

static void renderHeartbeat(CRGB *leds, uint16_t numLeds, const TProgmemRGBPalette16 &palette,
                            bool justStarted) {
  if (justStarted) {
    heart.period = HEART_START_PERIOD;
    heart.beatFrame = 0;
    heart.flatline = 0;
  }

  uint8_t level = 0;
  if (heart.flatline > 0) {
    if (--heart.flatline == 0) {
      heart.period = HEART_START_PERIOD;
      heart.beatFrame = 0;
    }
  } else {
    uint8_t t = heart.beatFrame;
    uint8_t lub = thump(t, 255);
    uint8_t dub = (t >= HEART_DUB_DELAY) ? thump(t - HEART_DUB_DELAY, 170) : 0;
    level = lub > dub ? lub : dub;
    if (level < HEART_GLOW) {
      level = HEART_GLOW;
    }
    if (++heart.beatFrame >= heart.period) {
      heart.beatFrame = 0;
      if (heart.period > HEART_END_PERIOD) {
        heart.period--; // each beat a little faster
      } else {
        heart.flatline = HEART_FLATLINE_FRAMES;
      }
    }
  }

  // Reddest palette entry: most red relative to green and blue.
  uint8_t redIndex = 0;
  int16_t bestScore = -1000;
  for (uint8_t k = 0; k < 16; k++) {
    CRGB c = ColorFromPalette(palette, k * 16);
    int16_t score = (int16_t)c.r - c.g - c.b;
    if (score > bestScore) {
      bestScore = score;
      redIndex = k * 16;
    }
  }
  fill_solid(leds, numLeds, ColorFromPalette(palette, redIndex, level));
}

// ---- 2D: passing ghost ----
// A soft pale glow drifts slowly across the house at its real (x, y), so it
// vanishes while crossing the unlit gaps between openings and reappears in
// the next one. It bobs up and down near the tops of the openings as it
// goes, then waits a few seconds and crosses again in a random direction.
// The house stays faintly lit in the palette's colors behind it.
#define GHOST_RADIUS 14     // LEDs
#define GHOST_SPEED 80      // 1/256ths of an LED per frame, ~14s to cross
#define GHOST_BACKGROUND 30 // brightness of the faint palette glow behind it

struct GhostState {
  int16_t x;
  uint8_t subStep;
  int8_t direction;
  uint16_t waitFrames; // frames until the next crossing; 0 while crossing
};
static GhostState ghost = {0, 0, 1, 0};

static void renderGhost(CRGB *leds, uint16_t numLeds, const TProgmemRGBPalette16 &palette,
                        uint16_t frame, bool justStarted) {
  int16_t width = mappedWidth();
  if (justStarted) {
    ghost.waitFrames = 30;
  }

  if (ghost.waitFrames > 0) {
    if (--ghost.waitFrames == 0) {
      ghost.direction = random8(2) ? 1 : -1;
      ghost.x = (ghost.direction > 0) ? -GHOST_RADIUS : width + GHOST_RADIUS;
      ghost.subStep = 0;
    }
  } else {
    uint16_t acc = (uint16_t)ghost.subStep + GHOST_SPEED;
    ghost.subStep = (uint8_t)acc;
    ghost.x += (acc >> 8) * ghost.direction;
    if (ghost.x < -GHOST_RADIUS || ghost.x > width + GHOST_RADIUS) {
      ghost.waitFrames = 30 + random16(120); // ~1-5s before the next pass
    }
  }

  int16_t ghostY = 44 + triangle8((uint8_t)(frame * 2)) / 16; // bobs over ~4s
  const int16_t radiusSq = GHOST_RADIUS * GHOST_RADIUS;
  bool visible = (ghost.waitFrames == 0);
  uint8_t wisp = 200 + random8(56); // slight shimmer

  for (uint16_t i = 0; i < numLeds; i++) {
    uint8_t h = (uint8_t)(i * 167 + 71);
    CRGB c = ColorFromPalette(palette, h, GHOST_BACKGROUND);
    if (visible) {
      Point p = ledIndexToXY(i);
      int16_t dx = p.x - ghost.x;
      int16_t dy = p.y - ghostY;
      if (dx > -GHOST_RADIUS && dx < GHOST_RADIUS && dy > -GHOST_RADIUS && dy < GHOST_RADIUS) {
        int16_t distSq = dx * dx + dy * dy;
        if (distSq < radiusSq) {
          uint8_t level = (uint8_t)(255 - (uint16_t)distSq * 255 / radiusSq);
          level = (uint8_t)(((uint16_t)level * level) >> 8); // soften the edge
          level = (uint8_t)(((uint16_t)level * wisp) >> 8);
          // Pale blue-white.
          c += CRGB((uint8_t)(((uint16_t)level * 200) >> 8),
                    (uint8_t)(((uint16_t)level * 220) >> 8), level);
        }
      }
    }
    leds[i] = c;
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
  case EFFECT_STORM:
    renderStorm(leds, numLeds, palette, frame, justStarted);
    break;
  case EFFECT_HEARTBEAT:
    renderHeartbeat(leds, numLeds, palette, justStarted);
    break;
  case EFFECT_GHOST:
    renderGhost(leds, numLeds, palette, frame, justStarted);
    break;
  case EFFECT_SOLID:
    renderSolid(leds, numLeds);
    break;
  }
}
