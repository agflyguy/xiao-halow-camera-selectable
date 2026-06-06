#!/bin/sh
# Fix corrupt managed_components (iCloud / interrupted fullclean).
# Usage:  source ./setenv.sh && . ~/esp/esp-idf/export.sh && ./repair_deps.sh
set -e
cd "$(dirname "$0")"

echo "Removing managed_components and build..."
rm -rf managed_components build
rm -rf "managed_components/espressif__esp32-camera 2" 2>/dev/null || true
rm -rf "managed_components/espressif__esp_jpeg 2" 2>/dev/null || true

echo "Re-downloading components..."
idf.py reconfigure

jpeg_inc="managed_components/espressif__esp_jpeg/include"
cam_drv="managed_components/espressif__esp32-camera/driver"
if [ ! -d "$jpeg_inc" ] || [ ! -d "$cam_drv" ]; then
    echo "ERROR: component download still incomplete." >&2
    echo "  - Move project out of iCloud-synced Documents if possible" >&2
    echo "  - Retry: ./repair_deps.sh" >&2
    exit 1
fi

echo "OK — dependencies repaired. Run: idf.py build flash monitor"
