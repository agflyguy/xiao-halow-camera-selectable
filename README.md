# Seeed XIAO ESP32-S3 Sense + Wio-WM6108 — camera (HaLow or Wi-Fi)

ESP-IDF **5.1.1** MJPEG camera server for the [Seeed Halo kit](https://wiki.seeedstudio.com/getting_started_with_wifi_halow_module_for_xiao/) (XIAO ESP32-S3 Sense + WM6108 HaLow module).

Network modes match the **HT-HC33 / HT-HD01** Arduino camera sketch (`ht-hc33-halow-camera.ino`): same `camera_build_config.h` layout, credentials, link maintenance, and stream error handling.

Edit **`main/camera_build_config.h`** before build (same layout as HT-HC33 Arduino):

| `USE_WIFI` | Network | SSID / password | When to use |
|------------|---------|-----------------|-------------|
| **0** (default) | Wi-Fi **HaLow** STA | `grey-AP` / `heltec.org` | Classic HaLow AP (e.g. Heltec HT-H7608) |
| **1** | 2.4 GHz **Wi-Fi** STA | `blue-2g` / `change-me` | HAVEN / OpenMANET **point** AP |

| `ENABLE_OV3660_SETTINGS` | Web UI |
|--------------------------|--------|
| **0** | Stream-only (Get Still + Start Stream) |
| **1** (default) | Full OV3660 panel (“Toggle OV3660 settings”) |

Unlike the HT-HC33, the Xiao can also join OPENMANET HaLow mesh with Morse mm-iot — override SSID/passphrase at build time (see below).

## Troubleshooting: CMake / `managed_components` errors

If you see:

`espressif__esp_jpeg/include is not a directory`

the auto-downloaded components are incomplete (common after `idf.py fullclean`, iCloud sync, or Finder duplicate folders named `"... 2"`).

```bash
cd xiao-halow-camera
source ./setenv.sh
. ~/esp/esp-idf/export.sh
./repair_deps.sh
idf.py build flash monitor
```

Avoid `idf.py fullclean` for normal rebuilds. Do not copy folders inside `managed_components/`.

## Troubleshooting: `camera_build_config.h` not updating

**Edit this file only:**

`xiao-halow-camera/main/camera_build_config.h`

(not `ht-hc33-halow-camera_update/camera_build_config.h` — that is the Arduino sketch.)

After save, run from `xiao-halow-camera/`:

```bash
idf.py build flash monitor
```

Check the CMake line during build:

`XIAO camera USE_WIFI=… ENABLE_OV3660_SETTINGS=…`

After flash, serial must match:

- `USE_WIFI 0` → `HaLow mode` and `HTTP servers started — web UI: full OV3660 settings` or `stream-only`
- `USE_WIFI 1` → `Wi-Fi mode` and the same web UI line

If CMake shows the right values but serial does not, you did not flash the new binary. If CMake shows wrong values, run once:

```bash
idf.py reconfigure build flash monitor
```

Projects under iCloud-synced `Documents/` can lag — save the header, confirm the two `#define` lines on disk, then build.

## Prerequisites

```bash
git clone https://github.com/Seeed-Studio/mm-iot-esp32.git ~/mm-iot-esp32
export MMIOT_ROOT=~/mm-iot-esp32
. ~/esp/esp-idf/export.sh   # ESP-IDF v5.1.1
```

## Build and flash

**Run all commands from the `xiao-halow-camera/` folder** (not the repo root — `idf.py` there will fail with “CMakeLists.txt not found”).

**`idf.py build` alone does not update the board.** You must **`flash`** after every change or the device keeps running the old firmware.

```bash
cd xiao-halow-camera
source ./setenv.sh          # sets MMIOT_ROOT=~/mm-iot-esp32
. ~/esp/esp-idf/export.sh
idf.py set-target esp32s3

# Edit main/camera_build_config.h (the two #define lines), then:
idf.py build flash monitor

# Or use the helper script (same thing):
chmod +x build_flash.sh
./build_flash.sh /dev/cu.usbmodem*    # optional: serial port
```

After flash, serial shows **`Firmware compiled: <date> <time>`** — that timestamp must change when you rebuild and reflash. You can also verify the binary on disk before flashing:

```bash
strings build/xiao_halow_camera.bin | grep "stream stopped"
```

CMake prints what it read, for example:

`XIAO camera USE_WIFI=1 ENABLE_OV3660_SETTINGS=1`

If that line does not match your header, you are editing the wrong file or an old build tree — see **Config not updating** below.

### OPENMANET / HAVEN on HaLow

**Important:** On many HAVEN gates, `haven` / `havenmesh` are the **802.11s mesh ID and key**, not the SSID a camera STA joins. Your serial log already showed a joinable network named **`grey-AP`** (which gave IP `10.41.x.x`).

1. Flash and watch the **scan list** — use the SSID that actually appears.
2. Rebuild with that name (no quotes in `idf.py`):

```bash
# If scan shows grey-AP (common on Heltec/HAVEN):
idf.py fullclean
idf.py -DHALOW_SSID=grey-AP -DHALOW_PASSPHRASE=heltec.org build flash monitor

# If your LuCI mesh really broadcasts SSID "haven":
idf.py fullclean
idf.py -DOPENMANET_MESH=1 build flash monitor
# same as:
idf.py -DHALOW_SSID=haven -DHALOW_PASSPHRASE=havenmesh build flash monitor
```

Always run **`idf.py fullclean`** when changing `-DHALOW_*` so CMake picks up new credentials.

Seeed’s mm-iot fork example uses **`haven2` / `change-me`** — match whatever your Pi node shows in **Network → Wireless**.

## After boot

Serial monitor prints the assigned IP and URL, for example:

```
========================================
 XIAO camera — HaLow mode (classic AP)
 IP address:  192.168.x.x
 Camera URL:  http://192.168.x.x/
========================================

Camera ready — http://192.168.x.x/
```

Open that URL from a device on the **same network** as the camera.

## Behavior (aligned with HT-HC33)

- **Link recovery** (both modes): background task stops the web server when the link drops, reconnects (HaLow: `mmwlan` re-join; Wi-Fi: auto-reconnect), then restarts HTTP when IP is back. Serial shows `[HaLow]` / `[Wi-Fi] link lost` and `link up`.
- **HaLow stream**: CIF, moderate JPEG quality, ~5 fps cap, skip/slow down on send errors
- **Wi-Fi stream**: VGA (not SVGA), ~12 fps cap — less stutter than maxing bandwidth
- **Web UI**: edit `main/camera_build_config.h` (`ENABLE_OV3660_SETTINGS=0` stream-only, `=1` full panel). Serial log shows `Web UI: full OV3660 settings` or `stream-only` at boot.
- **Stream**: MJPEG on port **81** (`/stream`); main page on port **80**

If video is still choppy, try one viewer tab, move closer to the AP, or use HaLow only for stills and Wi-Fi for live view.

## Hardware notes

- HaLow uses the **WM6108** module antenna — not the XIAO 2.4 GHz PCB antenna
- `sdkconfig.defaults` sets SPI pins and `CONFIG_MM_BCF_MF16858_US` for the Halo kit
- Country code **US** in `mm_app_loadconfig.c` — must match your HaLow AP region

## References

- [HT-HC33 camera sketch](../ht-hc33-halow-camera.ino)
- [Seeed mm-iot-esp32 web_camera_serve](https://github.com/Seeed-Studio/mm-iot-esp32/tree/main/examples/web_camera_serve)
- [Seeed HaLow XIAO wiki](https://wiki.seeedstudio.com/getting_started_with_wifi_halow_module_for_xiao/)
