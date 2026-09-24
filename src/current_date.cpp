#include "current_date.h"
#include <Bridge.h>
#include <Process.h>

static uint32_t lastRefreshMs = 0;
static bool hasRunOnce = false;

void currentDateBegin() {
  Bridge.begin();
}

static bool queryLinuxDate(CurrentDate &out) {
  Process date;
  date.begin("date");
  date.addParameter("+%m-%d");
  date.run(); // blocks until the Linux-side process exits

  char buf[8]; // "MM-DD" + newline + NUL
  uint8_t len = 0;
  while (date.available() && len < sizeof(buf) - 1) {
    buf[len++] = (char)date.read();
  }
  buf[len] = '\0';

  if (len < 5) {
    return false;
  }

  out.month = (uint8_t)atoi(buf);
  out.day = (uint8_t)atoi(buf + 3);
  out.valid = (out.month >= 1 && out.month <= 12 && out.day >= 1 && out.day <= 31);
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
