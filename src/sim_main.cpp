// Desktop simulator. Runs the exact same effects.cpp/pixel_map.cpp/
// seasons.cpp as the real firmware -- only the entry point differs: instead
// of driving a physical WS2812 strip, this opens a real window and draws a
// dot per LED at its real (x, y). Uses SDL2 (brew install sdl2) rather than
// a terminal, since terminal escape-code rendering turned out to be
// unreliable across terminal emulators (VSCode's integrated terminal in
// particular) -- a real window sidesteps that whole class of problems.
#include <SDL.h>
#include <cstdio>
#include "pixel_map.h"
#include "seasons.h"
#include "effects.h"

#define NUM_LEDS 390
#define DOT_SIZE 6 // pixels per LED, full resolution -- no compression needed like the terminal version
#define MARGIN 50  // pixels of blank space around the outside of the display

CRGB leds[NUM_LEDS];

// Change these to preview a different date without waiting for the
// calendar -- same idea as FAKE_DATE_MONTH/DAY in the real main.cpp.
static uint8_t simMonth = 9;
static uint8_t simDay = 15;

static const Season *activeSeason = nullptr;
static const TProgmemRGBPalette16 *currentPalette = &DefaultPalette;
static EffectId currentEffect = EFFECT_CHASE;
static uint8_t effectIndex = 0;

static void applySeasonForToday() {
  activeSeason = &getActiveSeason(simMonth, simDay);
  currentPalette = activeSeason->palette;
  effectIndex = 0;
  currentEffect = activeSeason->effects[0];
}

static void advanceEffect() {
  effectIndex = (effectIndex + 1) % activeSeason->numEffects;
  currentEffect = activeSeason->effects[effectIndex];
}

static const char *effectName(EffectId effect) {
  switch (effect) {
  case EFFECT_CHASE:
    return "Chase";
  case EFFECT_SPARKLE:
    return "Sparkle";
  case EFFECT_VERTICAL_WIPE:
    return "Vertical Wipe";
  case EFFECT_DIAGONAL_WIPE:
    return "Diagonal Wipe";
  case EFFECT_STRIPES:
    return "Stripes";
  case EFFECT_GRADIENT:
    return "Gradient";
  case EFFECT_SOLID:
    return "Solid";
  }
  return "?";
}

static void updateWindowTitle(SDL_Window *window) {
  char title[128];
  snprintf(title, sizeof(title), "HolidayLights sim -- %s (press any key for next)",
           effectName(currentEffect));
  SDL_SetWindowTitle(window, title);
}

int main() {
  applySeasonForToday();

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }

  int windowW = (mappedWidth() + 1) * DOT_SIZE + 2 * MARGIN;
  int windowH = (mappedHeight() + 1) * DOT_SIZE + 2 * MARGIN;

  SDL_Window *window = SDL_CreateWindow("HolidayLights sim", SDL_WINDOWPOS_CENTERED,
                                         SDL_WINDOWPOS_CENTERED, windowW, windowH, 0);
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  updateWindowTitle(window);

  int16_t maxHeight = mappedHeight();
  uint16_t frame = 0;
  bool running = true;

  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      } else if (event.type == SDL_KEYDOWN) {
        advanceEffect();
        updateWindowTitle(window);
      }
    }

    renderEffect(currentEffect, leds, NUM_LEDS, *currentPalette, frame);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Dark-gray fill behind each opening's bounding box, so the window/door
    // shapes read as actual openings rather than just floating LED dots.
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    for (uint8_t s = 0; s < NUM_SHAPES; s++) {
      const ShapeGeometry &shape = SHAPES[s];
      SDL_Rect shapeRect;
      shapeRect.x = MARGIN + shape.xOffset * DOT_SIZE;
      shapeRect.y = MARGIN + (maxHeight - (shape.yOffset + shape.leftCount)) * DOT_SIZE;
      shapeRect.w = shape.topCount * DOT_SIZE;
      shapeRect.h = shape.leftCount * DOT_SIZE;
      SDL_RenderFillRect(renderer, &shapeRect);
    }

    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      Point p = ledIndexToXY(i);
      if (p.x < 0 || p.y < 0) {
        continue;
      }
      SDL_Rect rect;
      rect.x = MARGIN + p.x * DOT_SIZE;
      rect.y = MARGIN + (maxHeight - p.y) * DOT_SIZE; // flip: p.y=0 is the floor, screen y=0 is the top
      rect.w = DOT_SIZE;
      rect.h = DOT_SIZE;
      CRGB c = leds[i];
      SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
      SDL_RenderFillRect(renderer, &rect);
    }
    SDL_RenderPresent(renderer);

    frame++;
    SDL_Delay(30); // matches the real firmware's ~30ms frame delay
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
