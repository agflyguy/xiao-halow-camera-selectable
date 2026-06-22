#pragma once

// *** Copy to camera_build_config.h and edit (camera_build_config.h is not in git) ***
//   cp main/camera_build_config.example.h main/camera_build_config.h
//
// Edit, save, then: idf.py build flash monitor

#define USE_WIFI 0
#define ENABLE_OV3660_SETTINGS 0
#define STREAM_RTSP 0

#define WIFI_SSID "your-2g-ap"
#define WIFI_PASSWORD "change-me"

#define HALOW_SSID "your-halow-ap"
#define HALOW_PASSPHRASE "change-me"
#define HALOW_COUNTRY "US"

#define USE_STATIC_IP 0
#define STATIC_IP "10.41.0.199"
#define STATIC_GATEWAY "10.41.0.4"
#define STATIC_NETMASK "255.255.0.0"
#define STATIC_DNS1 "10.41.0.4"
#define STATIC_DNS2 "0.0.0.0"

#define CAMERA_TITLE "my-camera"

#define ENABLE_HEARTBEAT_LED 1
#define HEARTBEAT_LED_GPIO 21
#define HEARTBEAT_LED_ON 0
#define HEARTBEAT_STABLE_MS 5000
#define HEARTBEAT_PULSE_MS 200
#define HEARTBEAT_PERIOD_MS 2000

#define WIFI_STREAM_SMOOTH_MODE 1
#define WIFI_STREAM_QVGA 1
#define WIFI_JPEG_QUALITY 16
#define WIFI_FRAME_PERIOD_MS 100
#define WIFI_SMOOTH_JPEG_QUALITY 22
#define WIFI_SMOOTH_FRAME_PERIOD_MS 180

#define CAM_VFLIP 1
#define CAM_HMIRROR 0
#define CAM_BRIGHTNESS 1
#define CAM_CONTRAST 0
#define CAM_SATURATION 0
