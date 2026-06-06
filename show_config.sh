#!/usr/bin/env bash
# Print camera_build_config.h and what CMake last read (run from xiao-halow-camera/)
set -e
cd "$(dirname "$0")"
CFG="main/camera_build_config.h"
echo "=== $CFG ==="
grep -E '^#define (USE_WIFI|ENABLE_OV3660_SETTINGS)' "$CFG" || true
echo ""
if [[ -f build/CMakeCache.txt ]]; then
  echo "=== Last CMake configure (grep build log or re-run) ==="
  echo "Run: idf.py build 2>&1 | grep 'XIAO camera'"
else
  echo "(no build/ yet — run idf.py build)"
fi
