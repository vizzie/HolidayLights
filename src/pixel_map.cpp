#include "pixel_map.h"

Point ledIndexToXY(uint16_t i) {
  for (uint8_t s = 0; s < NUM_SHAPES; s++) {
    const ShapeGeometry &g = SHAPES[s];
    uint16_t segTotal = (uint16_t)g.leftCount + g.topCount + g.rightCount;
    if (i < segTotal) {
      if (i < g.leftCount) {
        // up the left side
        return {g.xOffset, (int16_t)(g.yOffset + i)};
      }
      if (i < (uint16_t)g.leftCount + g.topCount) {
        // across the top
        uint16_t t = i - g.leftCount;
        return {(int16_t)(g.xOffset + t), (int16_t)(g.yOffset + g.leftCount)};
      }
      // down the right side
      uint16_t r = i - g.leftCount - g.topCount;
      return {(int16_t)(g.xOffset + g.topCount),
              (int16_t)(g.yOffset + g.leftCount - r)};
    }
    i -= segTotal;
  }
  return {-1, -1}; // index out of range
}

// Same as ledIndexToXY, but x has the gaps between openings squeezed out --
// each opening's x picks up immediately where the previous one's left off,
// with no space for the unlit jumper-wire gaps in between. Useful for
// effects (like the diagonal wipe) that should treat the three openings as
// one continuous strip rather than pausing while crossing physically real
// but invisible gap distance.
Point ledIndexToCompressedXY(uint16_t i) {
  int16_t compressedXOffset = 0;
  for (uint8_t s = 0; s < NUM_SHAPES; s++) {
    const ShapeGeometry &g = SHAPES[s];
    uint16_t segTotal = (uint16_t)g.leftCount + g.topCount + g.rightCount;
    if (i < segTotal) {
      if (i < g.leftCount) {
        return {compressedXOffset, (int16_t)(g.yOffset + i)};
      }
      if (i < (uint16_t)g.leftCount + g.topCount) {
        uint16_t t = i - g.leftCount;
        return {(int16_t)(compressedXOffset + t), (int16_t)(g.yOffset + g.leftCount)};
      }
      uint16_t r = i - g.leftCount - g.topCount;
      return {(int16_t)(compressedXOffset + g.topCount),
              (int16_t)(g.yOffset + g.leftCount - r)};
    }
    i -= segTotal;
    compressedXOffset += g.topCount;
  }
  return {-1, -1}; // index out of range
}

int16_t mappedCompressedWidth() {
  int16_t total = 0;
  for (uint8_t s = 0; s < NUM_SHAPES; s++) {
    total += SHAPES[s].topCount;
  }
  return total;
}

uint16_t totalMappedLeds() {
  uint16_t total = 0;
  for (uint8_t s = 0; s < NUM_SHAPES; s++) {
    total += SHAPES[s].leftCount + SHAPES[s].topCount + SHAPES[s].rightCount;
  }
  return total;
}

int16_t mappedWidth() {
  int16_t maxX = 0;
  for (uint8_t s = 0; s < NUM_SHAPES; s++) {
    int16_t right = SHAPES[s].xOffset + SHAPES[s].topCount;
    if (right > maxX) maxX = right;
  }
  return maxX;
}

int16_t mappedHeight() {
  int16_t maxY = 0;
  for (uint8_t s = 0; s < NUM_SHAPES; s++) {
    int16_t top = SHAPES[s].yOffset + SHAPES[s].leftCount;
    if (top > maxY) maxY = top;
  }
  return maxY;
}
