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
