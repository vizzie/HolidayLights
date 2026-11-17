#pragma once
#include <Arduino.h>

struct CurrentDate {
  uint8_t month; // 1-12
  uint8_t day;   // 1-31
  bool valid;    // false until a read passes the sane-year check
  uint16_t year; // full year, e.g. 2026
};

// Starts the Bridge to the Yun's Linux side. Call once in setup(). Blocks
// until the Linux side responds, which can take from a few seconds up to
// ~60s after power-on while Linux itself is still booting.
void currentDateBegin();

// Re-queries the Linux side's (NTP-synced) date, but at most once per
// refreshIntervalMs -- shelling out to a Linux process is slow and
// shouldn't happen every frame. Safe to call from every loop() iteration.
// Returns true and updates `out` only when a fresh, valid read happens.
// A read with an implausibly old year (clock not yet NTP-synced) is invalid.
bool currentDateUpdate(CurrentDate &out, uint32_t refreshIntervalMs = 3600000UL);
