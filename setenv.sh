#!/bin/sh
# Source before idf.py:  . ./setenv.sh
export MMIOT_ROOT="${MMIOT_ROOT:-$HOME/mm-iot-esp32}"
if [ ! -d "$MMIOT_ROOT/framework/morselib" ]; then
  echo "ERROR: mm-iot-esp32 not found at $MMIOT_ROOT" >&2
  echo "  git clone https://github.com/Seeed-Studio/mm-iot-esp32.git ~/mm-iot-esp32" >&2
  return 1 2>/dev/null || exit 1
fi
echo "MMIOT_ROOT=$MMIOT_ROOT"
