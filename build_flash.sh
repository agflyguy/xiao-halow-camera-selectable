#!/bin/bash
# idf.py build alone does NOT update the board — this builds AND flashes.
set -e
cd "$(dirname "$0")"
if [ -f ./setenv.sh ]; then
    # shellcheck disable=SC1091
    source ./setenv.sh
fi
if [ -f "$HOME/esp/esp-idf/export.sh" ]; then
    # shellcheck disable=SC1091
    . "$HOME/esp/esp-idf/export.sh"
fi
echo "=== build (watch for: XIAO camera USE_WIFI=... ENABLE_OV3660_SETTINGS=...) ==="
idf.py build
echo ""
echo "=== flash + monitor (required after every code/config change) ==="
if [ -n "$1" ]; then
    idf.py -p "$1" flash monitor
else
    idf.py flash monitor
fi
