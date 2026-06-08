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
echo "=== reconfigure + build (watch for: STREAM_RTSP=0 or 1) ==="
idf.py reconfigure build
echo ""
echo "=== binary check (HTTP-only build must NOT contain 'RTSP started') ==="
if strings build/xiao_halow_camera.bin | grep -q "RTSP started"; then
    echo "WARNING: binary includes RTSP — camera_build_config.h has STREAM_RTSP=1"
else
    echo "OK: HTTP-only binary (no RTSP server)"
fi
if strings build/xiao_halow_camera.bin | grep -q "STREAM MODE: HTTP ONLY"; then
    echo "OK: boot banner says HTTP ONLY"
fi
echo ""
echo "=== flash + monitor (required after every code/config change) ==="
if [ -n "$1" ]; then
    idf.py -p "$1" flash monitor
else
    idf.py flash monitor
fi
