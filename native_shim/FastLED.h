// Minimal stand-in for FastLED, used only by the native/[env:native] desktop
// simulator build (see sim_main.cpp). The real firmware (env:yun) links the
// real FastLED library instead. This exists because FastLED's own "native"
// stub platform (as of 3.10.5) has an upstream bug (an undefined
// BusTraits<Bus::STUB> template specialization) that fails to compile
// outside FastLED's own internal test harness. Rather than chase that, this
// implements just the handful of symbols effects.cpp/pixel_map.cpp/
// seasons.cpp actually call, closely matching real FastLED's behavior
// (same palette interpolation scheme, same saturating math) so the preview
// looks right without depending on the real library at all.
#pragma once
#include <cstdint>
#include <cstdlib>

#define PROGMEM

struct CRGB {
  uint8_t r, g, b;

  constexpr CRGB() : r(0), g(0), b(0) {}
  constexpr CRGB(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}
  constexpr CRGB(uint32_t hexcode)
      : r((uint8_t)(hexcode >> 16)), g((uint8_t)(hexcode >> 8)), b((uint8_t)hexcode) {}

  CRGB &operator+=(const CRGB &rhs) {
    uint16_t nr = (uint16_t)r + rhs.r;
    uint16_t ng = (uint16_t)g + rhs.g;
    uint16_t nb = (uint16_t)b + rhs.b;
    r = nr > 255 ? 255 : (uint8_t)nr;
    g = ng > 255 ? 255 : (uint8_t)ng;
    b = nb > 255 ? 255 : (uint8_t)nb;
    return *this;
  }

  static const CRGB Red, Green, Blue, White, Black, Gold, Silver, HotPink, Crimson,
      ForestGreen, LimeGreen, LightPink, LightYellow, Lavender, PaleGreen, RoyalBlue,
      OrangeRed, Purple, DarkViolet, SaddleBrown, DarkRed, Cyan, DarkOrange, Magenta,
      Orange, Yellow;
};

inline constexpr CRGB CRGB::Red = 0xFF0000;
inline constexpr CRGB CRGB::Green = 0x008000;
inline constexpr CRGB CRGB::Blue = 0x0000FF;
inline constexpr CRGB CRGB::White = 0xFFFFFF;
inline constexpr CRGB CRGB::Black = 0x000000;
inline constexpr CRGB CRGB::Gold = 0xFFD700;
inline constexpr CRGB CRGB::Silver = 0xC0C0C0;
inline constexpr CRGB CRGB::HotPink = 0xFF69B4;
inline constexpr CRGB CRGB::Crimson = 0xDC143C;
inline constexpr CRGB CRGB::ForestGreen = 0x228B22;
inline constexpr CRGB CRGB::LimeGreen = 0x32CD32;
inline constexpr CRGB CRGB::LightPink = 0xFFB6C1;
inline constexpr CRGB CRGB::LightYellow = 0xFFFFE0;
inline constexpr CRGB CRGB::Lavender = 0xE6E6FA;
inline constexpr CRGB CRGB::PaleGreen = 0x98FB98;
inline constexpr CRGB CRGB::RoyalBlue = 0x4169E1;
inline constexpr CRGB CRGB::OrangeRed = 0xFF4500;
inline constexpr CRGB CRGB::Purple = 0x800080;
inline constexpr CRGB CRGB::DarkViolet = 0x9400D3;
inline constexpr CRGB CRGB::SaddleBrown = 0x8B4513;
inline constexpr CRGB CRGB::DarkRed = 0x8B0000;
inline constexpr CRGB CRGB::Cyan = 0x00FFFF;
inline constexpr CRGB CRGB::DarkOrange = 0xFF8C00;
inline constexpr CRGB CRGB::Magenta = 0xFF00FF;
inline constexpr CRGB CRGB::Orange = 0xFFA500;
inline constexpr CRGB CRGB::Yellow = 0xFFFF00;

using TProgmemRGBPalette16 = CRGB[16];

inline uint8_t qsub8(uint8_t a, uint8_t b) {
  return a > b ? (uint8_t)(a - b) : 0;
}

inline uint8_t random8() {
  return (uint8_t)(rand() & 0xFF);
}
inline uint8_t random8(uint8_t max) {
  return max == 0 ? 0 : (uint8_t)(rand() % max);
}
inline uint16_t random16(uint16_t max) {
  return max == 0 ? 0 : (uint16_t)(rand() % max);
}

inline void fill_solid(CRGB *leds, uint16_t numLeds, const CRGB &color) {
  for (uint16_t i = 0; i < numLeds; i++) {
    leds[i] = color;
  }
}

inline void fadeToBlackBy(CRGB *leds, uint16_t numLeds, uint8_t fadeBy) {
  uint16_t scale = 255 - fadeBy;
  for (uint16_t i = 0; i < numLeds; i++) {
    leds[i].r = (uint8_t)(((uint16_t)leds[i].r * scale) >> 8);
    leds[i].g = (uint8_t)(((uint16_t)leds[i].g * scale) >> 8);
    leds[i].b = (uint8_t)(((uint16_t)leds[i].b * scale) >> 8);
  }
}

// Same interpolation scheme as real FastLED: index's top 4 bits select
// which of the 16 stored colors to start from, the bottom 4 bits blend
// linearly toward the next one.
inline CRGB ColorFromPalette(const TProgmemRGBPalette16 &pal, uint8_t index,
                              uint8_t brightness = 255) {
  uint8_t hi4 = index >> 4;
  uint8_t lo4 = index & 0x0F;
  const CRGB &a = pal[hi4];
  const CRGB &b = pal[(hi4 + 1) & 0x0F];
  uint8_t blendAmt = (uint8_t)(lo4 * 17); // 0..255 across the 16 sub-steps

  CRGB result((uint8_t)(a.r + (((int)(b.r - a.r) * blendAmt) >> 8)),
              (uint8_t)(a.g + (((int)(b.g - a.g) * blendAmt) >> 8)),
              (uint8_t)(a.b + (((int)(b.b - a.b) * blendAmt) >> 8)));

  if (brightness != 255) {
    result.r = (uint8_t)(((uint16_t)result.r * brightness) >> 8);
    result.g = (uint8_t)(((uint16_t)result.g * brightness) >> 8);
    result.b = (uint8_t)(((uint16_t)result.b * brightness) >> 8);
  }
  return result;
}
