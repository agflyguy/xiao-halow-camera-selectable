# xiao-halow-camera-selectable

ESP-IDF **5.1.1** MJPEG camera server for the [Seeed Halo kit](https://wiki.seeedstudio.com/getting_started_with_wifi_halow_module_for_xiao/) (XIAO ESP32-S3 Sense + WM6108 HaLow module).

**Build-time selectable:** HaLow or 2.4 GHz Wi-Fi; HTTP MJPEG (browser, ZoneMinder, VLC) or optional RTSP.

Release tag **`zm-vlc-curl-verified`** (*ZM + VLC + curl verified*) — Wi-Fi mode, ZoneMinder on `/stream`, VLC on `/video.mjpg`, `curl` to `/` and `/capture`.

```bash
git clone https://github.com/agflyguy/xiao-halow-camera-selectable.git
cd xiao-halow-camera-selectable
git checkout zm-vlc-curl-verified   # optional: exact tested release
```

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

| Define | When | Example |
|--------|------|---------|
| `CAMERA_TITLE` | Always | `garage-cam` — browser tab, web UI banner, serial log |
| `USE_STATIC_IP` | Always | `0` = DHCP, `1` = use `STATIC_*` below |
| `STATIC_IP` | `USE_STATIC_IP 1` | Unique per camera on mesh, e.g. `10.41.0.160` (gray) |
| `STATIC_GATEWAY` / `STATIC_NETMASK` / `STATIC_DNS1` / `STATIC_DNS2` | `USE_STATIC_IP 1` | Gateway/DNS **`10.41.0.4`** (brown); netmask `255.255.0.0` — see `haven-gateway/docs/NODE-IPS.md` |

Web UI loads title and IP from `GET /camera_info` JSON.

| Define | When | Example |
|--------|------|---------|
| `ENABLE_HEARTBEAT_LED` | Always | `1` = user LED blinks after stable link |
| `HEARTBEAT_LED_GPIO` | `ENABLE_HEARTBEAT_LED 1` | `21` (XIAO ESP32-S3 Sense) |
| `HEARTBEAT_STABLE_MS` | `ENABLE_HEARTBEAT_LED 1` | `5000` — wait before first blink |

| `STREAM_RTSP` | Live video protocol (pick **one** — better than running both) |
|---------------|---------------------------------------------------------------------|
| **0** (default) | **HTTP MJPEG** — browser `/stream`; ZoneMinder/VLC on `/video.mjpg` |
| **1** | **RTSP** `rtsp://<ip>:8554/mjpeg/1` — VLC / ZoneMinder (RTP-over-TCP) |

Port **80** web UI and `/capture` work in both modes. Only the live video path changes.

### Credentials (`main/camera_build_config.h`)

| Define | When | Example |
|--------|------|---------|
| `WIFI_SSID` / `WIFI_PASSWORD` | `USE_WIFI 1` | `blue-2g` / `change-me` |
| `HALOW_SSID` / `HALOW_PASSPHRASE` / `HALOW_COUNTRY` | `USE_WIFI 0` | `gray-M` / `change-me` / `US` |

HaLow can still be overridden at build: `idf.py -DHALOW_SSID=grey-AP -DHALOW_PASSPHRASE=heltec.org build`

### Camera defaults at boot

| Define | Typical range | Notes |
|--------|---------------|--------|
| `CAM_VFLIP` | 0 or 1 | Vertical flip |
| `CAM_HMIRROR` | 0 or 1 | Horizontal mirror |
| `CAM_BRIGHTNESS` | -2 to 2 | |
| `CAM_CONTRAST` | -2 to 2 | |
| `CAM_SATURATION` | -2 to 2 | |

With `ENABLE_OV3660_SETTINGS 1`, you can also adjust these live in the browser UI.

Unlike the HT-HC33, the Xiao can also join OPENMANET HaLow mesh with Morse mm-iot — override SSID/passphrase at build time (see below).

## Troubleshooting: CMake / `managed_components` errors

If you see:

`espressif__esp_jpeg/include is not a directory`

the auto-downloaded components are incomplete (common after `idf.py fullclean`, iCloud sync, or Finder duplicate folders named `"... 2"`).

```bash
cd xiao-halow-camera-selectable
source ./setenv.sh
. ~/esp/esp-idf/export.sh
./repair_deps.sh
idf.py build flash monitor
```

Avoid `idf.py fullclean` for normal rebuilds. Do not copy folders inside `managed_components/`.

## Troubleshooting: serial port busy

Flash or monitor fails with:

```text
Could not exclusively lock port /dev/cu.usbserial-0001
[Errno 35] Resource temporarily unavailable
```

**Cause:** another process already has the USB serial port open. This is **not** a browser issue. Common holders:

| Process | Typical source |
|---------|----------------|
| `esp_idf_monitor` / `idf.py monitor` | This project's terminal left running after `idf.py flash monitor` |
| `pio device monitor` | HT-HC33 PlatformIO monitor (same or another Cursor tab) |
| Arduino IDE **Serial Monitor** | Same port selected |
| Cursor / VS Code **Serial Monitor** panel | Open on that port |

**Same cable, different board:** XIAO and HT-HC33 often share the same `/dev/cu.usbserial-0001` when you swap boards on one cable. Stop the monitor for the *other* camera before flashing.

### Fix

1. **Stop the monitor** — `Ctrl+C` in any terminal running `idf.py monitor` or `pio device monitor`.
2. **Find what holds the port:**

   ```bash
   lsof /dev/cu.usbserial-0001
   ```

3. **Release it** (use PID from `lsof`):

   ```bash
   kill <PID>
   ```

4. **Flash, then monitor** — do not run `idf.py build flash monitor` as one chained command if upload keeps failing; flash first:

   ```bash
   idf.py -p /dev/cu.usbserial-0001 flash
   # wait for SUCCESS, then:
   idf.py -p /dev/cu.usbserial-0001 monitor
   ```

5. If still stuck: unplug USB, wait 2 s, replug; close Arduino IDE; retry `lsof`.

## Troubleshooting: `camera_build_config.h` not updating

**Edit this file only:**

`main/camera_build_config.h`

(not `ht-hc33-halow-camera_update/camera_build_config.h` — that is the Arduino sketch.)

After save, run from the project root:

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

**Run all commands from the project root** (where `CMakeLists.txt` lives).

**`idf.py build` alone does not update the board.** You must **`flash`** after every change or the device keeps running the old firmware.

```bash
cd xiao-halow-camera-selectable
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
 IP address:  10.41.x.x
 Camera URL:  http://10.41.x.x/
========================================

Camera ready — http://10.41.x.x/
```

Open that URL from a device on the **same network** as the camera.

## Wi-Fi stream tuning (`main/camera_build_config.h`)

If the live view is **choppy on 2.4 GHz Wi-Fi**, set:

```c
#define WIFI_STREAM_SMOOTH_MODE 1
```

**Smooth mode** (overrides manual tuning): **CIF** resolution, JPEG quality **22**, **~180 ms** between frames (~5–6 fps). Uses **3 PSRAM frame buffers** when PSRAM is available.

Or tune manually with `WIFI_STREAM_SMOOTH_MODE 0`:

| Setting | Smoother | Sharper (more bandwidth) |
|---------|----------|---------------------------|
| `WIFI_STREAM_QVGA` | **1** (320×240) | 0 (640×480 VGA) |
| `WIFI_JPEG_QUALITY` | **16–18** | 10–14 |
| `WIFI_FRAME_PERIOD_MS` | **100–120** | 80 |

Also:

- **`esp_wifi_set_ps(WIFI_PS_NONE)`** is enabled — required for stable MJPEG.
- **One viewer at a time** — browser *or* VLC *or* ZoneMinder, not all three.
- Move closer to the AP; weak RSSI causes stalls that look like choppiness.

## Behavior (aligned with HT-HC33)

- **Link recovery** (both modes): background task stops the web server when the link drops, reconnects (HaLow: `mmwlan` re-join; Wi-Fi: auto-reconnect with brief glitch debounce), then restarts HTTP when IP is back. Serial shows `[HaLow]` / `[Wi-Fi] link lost` and `link up`.
- **HaLow stream**: CIF, moderate JPEG quality, ~5 fps cap, skip slow sends instead of killing the stream
- **Wi-Fi stream**: smooth-mode CIF preset by default; PSRAM frame buffers when available
- **Web UI**: edit `main/camera_build_config.h` (`ENABLE_OV3660_SETTINGS=0` stream-only, `=1` full panel). Serial log shows `Web UI: full OV3660 settings` or `stream-only` at boot.
- **Stream** (`STREAM_RTSP=0`): ZoneMinder/VLC on `http://<ip>/video.mjpg`; browser on `http://<ip>/stream`
- **Stream** (`STREAM_RTSP=1`): `rtsp://<ip>:8554/mjpeg/1` only — HTTP `/stream` disabled

### VLC (HTTP MJPEG, `STREAM_RTSP 0`)

Browsers and VLC use the same HTTP stream (non-chunked multipart for VLC compatibility):

1. Use **`http://<camera-ip>/video.mjpg`** (not `/stream` — that path is for browsers)
2. **Media → Open Network Stream** → paste the full URL including `http://`
3. Command line: `vlc --demux=mjpeg "http://<camera-ip>/video.mjpg"`
4. Confirm the PC running VLC can open `http://<camera-ip>/capture` in a browser first

RTSP (`STREAM_RTSP 1`) is optional and usually slower than HTTP on this hardware.

### ZoneMinder (recommended: HTTP MJPEG, not RTSP)

RTSP on ESP32 is limited by the micro-rtsp stack (many small RTP-over-TCP packets per frame). **Use HTTP instead** — it is what works reliably on HaLow and Wi-Fi.

1. Keep `STREAM_RTSP 0` in `main/camera_build_config.h`
2. **Add Monitor** → **Source Type**: `Ffmpeg`
3. **Source Path**: `http://<camera-ip>/video.mjpg` (**recommended** — best output in testing)
4. **Maximum FPS**: ~5 on HaLow, ~8 on Wi-Fi (with smooth mode)
5. **Function**: `Monitor` or `Modect` as needed

If `/video.mjpg` is unavailable, try `http://<camera-ip>/stream` as a fallback.

### VLC via RTSP (optional, often slow)

Only if you need RTSP specifically: set `STREAM_RTSP 1`, flash, open `rtsp://<camera-ip>:8554/mjpeg/1`. Expect low FPS on HaLow; Wi-Fi is better but still slower than HTTP.

If video is still choppy, try one viewer at a time, move closer to the AP, or lower ZoneMinder FPS.

### Serial errors (what they mean)

| Message | Cause | Fix |
|---------|-------|-----|
| `cam_hal: NO-SOI` | OV3660 needs a few frames after init | Harmless at boot; suppressed in newer builds |
| `httpd_accept_conn: error in accept (23)` | Ran out of lwIP sockets | Use `STREAM_RTSP 0` **or** `1`, not both; close extra viewer tabs; run `idf.py reconfigure build flash` |
| `stream ended (ESP_FAIL)` | HaLow link slow or viewer closed tab | Normal on weak link; use one stream tab |

After changing `STREAM_RTSP` or `sdkconfig.defaults`, run **`idf.py reconfigure build flash`** so socket limits update.

## Hardware notes

- HaLow uses the **WM6108** module antenna — not the XIAO 2.4 GHz PCB antenna
- `sdkconfig.defaults` sets SPI pins and `CONFIG_MM_BCF_MF16858_US` for the Halo kit
- Country code **US** in `mm_app_loadconfig.c` — must match your HaLow AP region

## References

- [HT-HC33 camera sketch](../ht-hc33-halow-camera.ino)
- [Seeed mm-iot-esp32 web_camera_serve](https://github.com/Seeed-Studio/mm-iot-esp32/tree/main/examples/web_camera_serve)
- [Seeed HaLow XIAO wiki](https://wiki.seeedstudio.com/getting_started_with_wifi_halow_module_for_xiao/)
