#pragma once

// *** xiao-halow-camera-selectable — edit THIS file before build ***
// Path: main/camera_build_config.h
//
// Edit the #define lines below, save, then from project root:
//   idf.py build flash monitor
//
// Network mode
//   USE_WIFI 0 = HaLow AP    1 = 2.4 GHz Wi-Fi (HAVEN point AP)
//   ENABLE_OV3660_SETTINGS 0 = stream-only UI   1 = full OV3660 settings panel
//
// STREAM_RTSP — pick ONE live video protocol (better performance than running both):
//   0 = HTTP MJPEG (browser + ZoneMinder + VLC — recommended)
//   1 = RTSP on port 8554 (VLC; often too slow — prefer HTTP for ZoneMinder)
//
// Live video (HTTP MJPEG, STREAM_RTSP=0):
//   ZoneMinder (recommended): http://<ip>/video.mjpg
//   VLC / ffmpeg:             http://<ip>/video.mjpg
//   Browser:                  http://<ip>/stream

#define USE_WIFI 1
#define ENABLE_OV3660_SETTINGS 0
#define STREAM_RTSP 0

// --- 2.4 GHz Wi-Fi credentials (USE_WIFI=1) ---
#define WIFI_SSID "blue-2g"
#define WIFI_PASSWORD "Dear!me2"

// --- HaLow credentials (USE_WIFI=0) — override at build with -DHALOW_SSID=... if needed ---
#define HALOW_SSID "gray-M"
#define HALOW_PASSPHRASE "Dear!me2"
#define HALOW_COUNTRY "US"

// --- Wi-Fi stream tuning (USE_WIFI=1, STREAM_RTSP=0) ---
// WIFI_STREAM_SMOOTH_MODE 1 = low-bandwidth preset (CIF, ~5–6 fps) — best if still choppy
// WIFI_STREAM_SMOOTH_MODE 0 = use the manual settings below
#define WIFI_STREAM_SMOOTH_MODE 1

#define WIFI_STREAM_QVGA 1       // 1 = QVGA   0 = VGA (ignored when SMOOTH_MODE=1)
#define WIFI_JPEG_QUALITY 16     // 10–20 (ignored when SMOOTH_MODE=1)
#define WIFI_FRAME_PERIOD_MS 100 // ms between frames (ignored when SMOOTH_MODE=1)

// Smooth preset (only when WIFI_STREAM_SMOOTH_MODE=1):
#define WIFI_SMOOTH_JPEG_QUALITY 22
#define WIFI_SMOOTH_FRAME_PERIOD_MS 180

// --- OV3660 defaults at boot (OV3660 range is usually -2 to +2) ---
#define CAM_VFLIP 1
#define CAM_HMIRROR 0
#define CAM_BRIGHTNESS 1
#define CAM_CONTRAST 0
#define CAM_SATURATION 0
