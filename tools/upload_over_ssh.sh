#!/usr/bin/env bash
# Uploads a compiled .hex to an Arduino Yun over SSH, using the flashing
# scripts that ship on the Yun's own Linux side (merge-sketch-with-bootloader.lua
# and run-avrdude) since avrdude has no network transport of its own.
set -euo pipefail

HEX_FILE="$1"
YUN_HOST="${YUN_HOST:-10.68.1.8}"
YUN_USER="${YUN_USER:-root}"
REMOTE_HEX="/tmp/sketch.hex"

echo "Copying $HEX_FILE to ${YUN_USER}@${YUN_HOST}:${REMOTE_HEX} ..."
# -O: legacy SCP protocol — dropbear_2011.54 on the Yun has no sftp-server.
scp -O -o BatchMode=yes "$HEX_FILE" "${YUN_USER}@${YUN_HOST}:${REMOTE_HEX}"

echo "Flashing on ${YUN_HOST} ..."
ssh -o BatchMode=yes "${YUN_USER}@${YUN_HOST}" "
  set -e
  if [ -x /usr/bin/merge-sketch-with-bootloader.lua ]; then
    /usr/bin/merge-sketch-with-bootloader.lua '${REMOTE_HEX}'
  fi
  /usr/bin/run-avrdude '${REMOTE_HEX}'
"
