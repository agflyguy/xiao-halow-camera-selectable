#pragma once

// *** xiao-halow-camera-selectable — edit THIS file before build ***
// Path: main/camera_build_config.h
//
// Edit the #define lines below, save, then from project root:
//   idf.py build flash monitor
//
// USE_WIFI: 0 = HaLow AP (gray-M / grey-AP)   1 = 2.4 GHz Wi-Fi (blue-2g)
// ENABLE_OV3660_SETTINGS: 0 = stream-only UI   1 = full OV3660 settings panel
//
// STREAM_RTSP — pick ONE live video protocol (better performance than running both):
//   0 = HTTP MJPEG at /stream on port 80  (browser + ZoneMinder — recommended)
//   1 = RTSP on port 8554  (VLC; often too slow on ESP32 — prefer HTTP for ZoneMinder)
// Port 80 web UI + /capture still run in both modes; only live video path changes.

#define USE_WIFI 1
#define ENABLE_OV3660_SETTINGS 0
#define STREAM_RTSP 0

