#include "current_date.h"
#include <Bridge.h>
#include <Process.h>

static uint32_t lastRefreshMs = 0;
static bool hasRunOnce = false;

void currentDateBegin() {
  Bridge.begin();
}

// The Yun has no battery-backed clock: at power-up it starts from a stale
// saved time until NTP syncs, so an early read can return an old date. Any
// year before this firmware was built can't be real, so reject it (the
// build year comes from __DATE__, e.g. "Nov 16 2026").
static uint16_t minSaneYear() {
  return (uint16_t)atoi(__DATE__ + 7);
}

static bool queryLinuxDate(CurrentDate &out) {
  Process date;
  date.begin("date");
  date.addParameter("+%Y-%m-%d");
  date.run(); // blocks until the Linux-side process exits

  char buf[12]; // "YYYY-MM-DD" + newline + NUL
  uint8_t len = 0;
  while (date.available() && len < sizeof(buf) - 1) {
    buf[len++] = (char)date.read();
  }
  buf[len] = '\0';

  if (len < 10) {
    return false;
  }

  out.year = (uint16_t)atoi(buf);
  out.month = (uint8_t)atoi(buf + 5);
  out.day = (uint8_t)atoi(buf + 8);
  out.valid = (out.year >= minSaneYear() && out.month >= 1 && out.month <= 12 &&
               out.day >= 1 && out.day <= 31);
  return out.valid;
}

bool currentDateUpdate(CurrentDate &out, uint32_t refreshIntervalMs) {
  uint32_t now = millis();
  if (hasRunOnce && (now - lastRefreshMs < refreshIntervalMs)) {
    return false;
  }
  lastRefreshMs = now;
  hasRunOnce = true;
  return queryLinuxDate(out);
}
