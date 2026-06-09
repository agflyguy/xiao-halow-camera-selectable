#!/bin/sh
# Fix corrupt managed_components (iCloud / interrupted fullclean).
# Usage:  source ./setenv.sh && . ~/esp/esp-idf/export.sh && ./repair_deps.sh
cd "$(dirname "$0")"

force_remove() {
    target="$1"
    [ -e "$target" ] || return 0
    echo "  removing $target ..."
    chmod -R u+w "$target" 2>/dev/null || true
    rm -rf "$target" 2>/dev/null || true
    if [ -e "$target" ]; then
        # iCloud / locked files: second pass after a short pause
        sleep 2
        chmod -R u+w "$target" 2>/dev/null || true
        rm -rf "$target" 2>/dev/null || true
    fi
    if [ -e "$target" ]; then
        echo "WARNING: could not fully remove $target (iCloud or file lock)." >&2
        echo "  Close IDF monitor / ninja, or move project out of Documents/iCloud." >&2
        return 1
    fi
    return 0
}

echo "Removing managed_components and build (can take 30-60 s in iCloud Documents)..."
force_remove managed_components || true
force_remove build || true
force_remove "managed_components/espressif__esp32-camera 2" || true
force_remove "managed_components/espressif__esp_jpeg 2" || true

echo "Re-downloading components (idf.py reconfigure)..."
if ! idf.py reconfigure; then
    echo "ERROR: idf.py reconfigure failed." >&2
    exit 1
fi

cam_dir="managed_components/espressif__esp32-camera"
jpeg_inc="managed_components/espressif__esp_jpeg/include"
cam_drv="$cam_dir/driver"
cam_hash="$cam_dir/.component_hash"
cam_sums="$cam_dir/CHECKSUMS.json"
if [ ! -d "$jpeg_inc" ] || [ ! -d "$cam_drv" ] || [ ! -f "$cam_hash" ] || [ ! -f "$cam_sums" ]; then
    echo "ERROR: component download still incomplete." >&2
    echo "  Missing: jpeg include, esp32-camera driver, .component_hash, or CHECKSUMS.json" >&2
    echo "  - Move project out of iCloud-synced Documents if possible" >&2
    echo "  - Retry: ./repair_deps.sh" >&2
    exit 1
fi

echo "OK — dependencies repaired. Run: idf.py build flash monitor"
