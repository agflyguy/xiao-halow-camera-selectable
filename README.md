# Seeed XIAO ESP32-S3 Sense + Wio-WM6108 — camera (HaLow or Wi-Fi)

ESP-IDF **5.1.1** MJPEG camera server for the [Seeed Halo kit](https://wiki.seeedstudio.com/getting_started_with_wifi_halow_module_for_xiao/) (XIAO ESP32-S3 Sense + WM6108 HaLow module).

Network modes match the **HT-HC33** Arduino camera sketch (`ht-hc33-halow-camera.ino`): same credentials, link maintenance, and stream error handling.

| Build | Network | SSID / password | When to use |
|-------|---------|-----------------|-------------|
| **default** (`USE_WIFI=0`) | Wi-Fi **HaLow** STA | `grey-AP` / `heltec.org` | Classic HaLow AP (e.g. Heltec HT-H7608) |
| **Wi-Fi** (`USE_WIFI=1`) | 2.4 GHz **Wi-Fi** STA | `blue-2g` / `Dear!me2` | HAVEN / OpenMANET **point** AP |

Unlike the HT-HC33, the Xiao can also join OPENMANET HaLow mesh with Morse mm-iot — override SSID/passphrase at build time (see below).

## Prerequisites

```bash
git clone https://github.com/Seeed-Studio/mm-iot-esp32.git ~/mm-iot-esp32
export MMIOT_ROOT=~/mm-iot-esp32
. ~/esp/esp-idf/export.sh   # ESP-IDF v5.1.1
```

## Build and flash

**Run all commands from the `xiao-halow-camera/` folder** (not the repo root — `idf.py` there will fail with “CMakeLists.txt not found”).

```bash
cd xiao-halow-camera
source ./setenv.sh          # sets MMIOT_ROOT=~/mm-iot-esp32
. ~/esp/esp-idf/export.sh
idf.py set-target esp32s3

# HaLow — same as HT-HC33 USE_WIFI 0
idf.py build flash monitor

# 2.4 GHz Wi-Fi — same as HT-HC33 USE_WIFI 1
idf.py -DUSE_WIFI=1 build flash monitor
```

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

Seeed’s mm-iot fork example uses **`haven2` / `Dear!me2`** — match whatever your Pi node shows in **Network → Wireless**.

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
- **Stream**: skip bad frames instead of killing the handler (same idea as HT-HC33 `app_httpd.cpp`)

If video is still choppy, try one viewer tab, move closer to the AP, or use HaLow only for stills and Wi-Fi for live view.

## Hardware notes

- HaLow uses the **WM6108** module antenna — not the XIAO 2.4 GHz PCB antenna
- `sdkconfig.defaults` sets SPI pins and `CONFIG_MM_BCF_MF16858_US` for the Halo kit
- Country code **US** in `mm_app_loadconfig.c` — must match your HaLow AP region

## References

- [HT-HC33 camera sketch](../ht-hc33-halow-camera.ino)
- [Seeed mm-iot-esp32 web_camera_serve](https://github.com/Seeed-Studio/mm-iot-esp32/tree/main/examples/web_camera_serve)
- [Seeed HaLow XIAO wiki](https://wiki.seeedstudio.com/getting_started_with_wifi_halow_module_for_xiao/)
