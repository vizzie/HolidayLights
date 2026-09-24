#pragma once
#include <Arduino.h>

// One entry per opening, left to right: window 1, window 2, door.
// Counts follow the physical wiring order: up the left side, across the
// top, down the right side. Coordinates are in "LED pitch" units, not
// inches — the strip has uniform LED spacing, so index differences within
// a run are already proportional to real-world distance.
//
struct ShapeGeometry {
  uint8_t leftCount;
  uint8_t topCount;
  uint8_t rightCount;
  int16_t xOffset; // x position of the left edge
  int16_t yOffset; // y position of the bottom edge (usually 0)
};

// xOffset = previous shape's xOffset + its topCount (width) + 30-LED gap.
// yOffset: windows sit 11 LEDs above the floor; the door goes to the floor.
static const ShapeGeometry SHAPES[] = {
    {47, 26, 47, 0, 11},    // window 1
    {48, 25, 47, 56, 11},   // window 2: 0 + 26 + 30
    {61, 29, 60, 111, 0},   // door: 56 + 25 + 30
};
static const uint8_t NUM_SHAPES = sizeof(SHAPES) / sizeof(SHAPES[0]);

struct Point {
  int16_t x;
  int16_t y;
};

// Maps a physical LED index to its (x, y) position, computed on the fly
// from the shape geometry above rather than a stored per-LED table — 390
// LEDs x 4 bytes would eat more than half the 32u4's RAM on top of the LED
// buffer itself.
Point ledIndexToXY(uint16_t i);

uint16_t totalMappedLeds();
int16_t mappedWidth();
int16_t mappedHeight();
