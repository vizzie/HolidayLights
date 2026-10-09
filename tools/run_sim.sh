#!/usr/bin/env bash
# Builds the desktop simulator ([env:native]) and launches it. Works from any
# directory. Uses `pio` from PATH if present, otherwise the copy the
# PlatformIO VSCode extension installs (which isn't put on PATH).
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if command -v pio >/dev/null 2>&1; then
  PIO="pio"
else
  PIO="${HOME}/.platformio/penv/bin/pio"
fi

if [ ! -x "$(command -v "$PIO")" ]; then
  echo "PlatformIO not found (looked for pio on PATH and at ${PIO})." >&2
  exit 1
fi

echo "Building simulator ..."
"$PIO" run -d "$PROJECT_DIR" -e native

# exec so Ctrl-C and the exit status go straight to the sim.
exec "$PROJECT_DIR/.pio/build/native/program"
