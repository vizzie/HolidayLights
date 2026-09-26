// Minimal stand-in for Arduino.h, used only by the native/[env:native]
// desktop simulator build (see sim_main.cpp). The real firmware (env:yun)
// uses the actual Arduino core instead -- this shim exists purely so
// effects.cpp/pixel_map.cpp/seasons.cpp (which #include <Arduino.h> for
// map() and the fixed-width int types) compile on a desktop with no
// Arduino framework present at all.
#pragma once
#include <cstdint>
#include <cstdlib>

inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
