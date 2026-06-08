/*
 * Seeed XIAO ESP32-S3 Sense + Wio-WM6108 (Halo kit)
 *
 * Network mode and web UI: edit main/camera_build_config.h (not the HT-HC33 Arduino copy).
 */

#include <stdio.h>
#include <string.h>

#include "mmosal.h"
#include "mm_app_common.h"
#include "app_wifi.h"
#include "camera_http.h"
#include "camera_build_config.h"
#if STREAM_RTSP
#include "camera_rtsp.h"
#endif

#include "esp_camera.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "xiao_camera";

#define CONFIG_XCLK_FREQ 20000000

static bool frame_has_jpeg_soi(const camera_fb_t *fb)
{
    return fb && fb->format == PIXFORMAT_JPEG && fb->len >= 2 && fb->buf[0] == 0xFF && fb->buf[1] == 0xD8;
}

/* OV3660 can log NO-SOI for a few frames after init; discard until JPEG is stable */
static void camera_warmup(void)
{
    vTaskDelay(pdMS_TO_TICKS(200));
    int valid = 0;
    for (int i = 0; i < 60 && valid < 10; i++) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (frame_has_jpeg_soi(fb)) {
            valid++;
        }
        if (fb) {
            esp_camera_fb_return(fb);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    printf("Camera warmup: %d valid JPEG frame(s)\n", valid);
}

static esp_err_t init_camera(void)
{
    camera_config_t camera_config = {
        .pin_pwdn = -1,
        .pin_reset = -1,
        .pin_xclk = 10,
        .pin_sccb_sda = 40,
        .pin_sccb_scl = 39,
        .pin_d7 = 48,
        .pin_d6 = 11,
        .pin_d5 = 12,
        .pin_d4 = 14,
        .pin_d3 = 16,
        .pin_d2 = 18,
        .pin_d1 = 17,
        .pin_d0 = 15,
        .pin_vsync = 38,
        .pin_href = 47,
        .pin_pclk = 13,
        .xclk_freq_hz = CONFIG_XCLK_FREQ,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_JPEG,
#if USE_WIFI
        .grab_mode = CAMERA_GRAB_LATEST,
        .fb_count = 2,
#else
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
        .fb_count = 1,
#endif
#if STREAM_RTSP
        /* RTSP: smaller frames — micro-rtsp sends many RTP packets per JPEG */
        .frame_size = FRAMESIZE_QQVGA,
        .jpeg_quality = USE_WIFI ? 30 : 38,
#elif USE_WIFI
        .frame_size = FRAMESIZE_QVGA,
        .jpeg_quality = 14,
#else
        .frame_size = FRAMESIZE_QVGA,
        .jpeg_quality = 18,
#endif
    };
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        return err;
    }

    /* Hide benign cam_hal "NO-SOI" warnings during startup capture */
    esp_log_level_set("cam_hal", ESP_LOG_ERROR);
    esp_log_level_set("s3 ll_cam", ESP_LOG_ERROR);

    sensor_t *s = esp_camera_sensor_get();
    if (s) {
        s->set_vflip(s, 1);
        s->set_brightness(s, 1);
        s->set_saturation(s, 0);
    }

    camera_warmup();
    return ESP_OK;
}

static void print_ov3660_ui_mode(void)
{
#if ENABLE_OV3660_SETTINGS
    printf("OV3660 menu: ON  (ENABLE_OV3660_SETTINGS=1, full settings panel)\n");
#else
    printf("OV3660 menu: OFF (ENABLE_OV3660_SETTINGS=0, stream-only UI)\n");
#endif
}

static void print_ready(void)
{
    char ip[48];
#if USE_WIFI
    if (app_wifi_get_ip_addr(ip, sizeof(ip))) {
        printf("Camera ready — http://%s/\n", ip);
#if STREAM_RTSP
        printf("Live stream (RTSP) — rtsp://%s:8554/mjpeg/1\n", ip);
        printf("  VLC / ZoneMinder: RTSP URL above (server uses RTP-over-TCP)\n");
#else
        printf("Live stream (HTTP) — http://%s/stream\n", ip);
        printf("VLC / ffmpeg — http://%s/video.mjpg\n", ip);
#endif
        print_ov3660_ui_mode();
        printf("\n");
    }
#else
    if (app_wlan_get_ip_addr(ip, sizeof(ip))) {
        printf("Camera ready — http://%s/\n", ip);
#if STREAM_RTSP
        printf("Live stream (RTSP) — rtsp://%s:8554/mjpeg/1\n", ip);
        printf("  VLC / ZoneMinder: RTSP URL above (server uses RTP-over-TCP)\n");
#else
        printf("Live stream (HTTP) — http://%s/stream\n", ip);
        printf("VLC / ffmpeg — http://%s/video.mjpg\n", ip);
#endif
        print_ov3660_ui_mode();
        printf("\n");
    }
#endif
}

static void restart_streaming_services(void)
{
    bool ok = camera_http_init() == ESP_OK;
#if STREAM_RTSP
    if (camera_rtsp_init() != ESP_OK) {
        ok = false;
    }
#endif
    if (ok) {
        print_ready();
    } else {
        printf("Streaming services failed to restart — retry after IP appears\n");
    }
}

static void stop_streaming_services(void)
{
    camera_http_stop();
#if STREAM_RTSP
    camera_rtsp_stop();
#endif
}

static bool streaming_services_running(void)
{
    if (!camera_http_is_running()) {
        return false;
    }
#if STREAM_RTSP
    return camera_rtsp_is_running();
#else
    return true;
#endif
}

/* Same pattern as HT-HC33 maintainHaLowLink(): stop HTTP when down, restart when up */
static void network_link_task(void *arg)
{
    (void)arg;
    bool link_was_up = false;

    while (true) {
#if USE_WIFI
        bool link_up = app_wifi_link_is_up();
#else
        bool link_up = app_wlan_link_is_up();
#endif

        if (link_up) {
            if (!streaming_services_running()) {
#if USE_WIFI
                printf("[Wi-Fi] link up — starting streaming services\n");
#else
                printf("[HaLow] link up — starting streaming services\n");
#endif
                restart_streaming_services();
            }
            link_was_up = true;
        } else {
            if (link_was_up || streaming_services_running()) {
#if USE_WIFI
                printf("[Wi-Fi] link lost — stopping streaming services\n");
#else
                printf("[HaLow] link lost — stopping streaming services\n");
#endif
                stop_streaming_services();
                link_was_up = false;
            }
#if !USE_WIFI
            if (!link_up && app_wlan_reconnect()) {
                app_wlan_print_addresses();
            }
#endif
        }

#if !USE_WIFI
        app_wlan_arp_send();
#endif
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void)
{
    printf("\n========================================\n");
    printf(" Seeed XIAO ESP32-S3 Sense + WM6108\n");
#if USE_WIFI
    printf(" Camera — Wi-Fi mode (HAVEN / 2.4 GHz)\n");
#else
    printf(" Camera — HaLow mode (classic HaLow AP)\n");
    printf(" HAVEN mesh \"haven\" is not used in this mode.\n");
#endif
    printf(" ESP-IDF 5.1.1\n");
    printf(" Firmware compiled: %s %s\n", __DATE__, __TIME__);
    printf(" camera_build_config.h: USE_WIFI=%d ENABLE_OV3660_SETTINGS=%d STREAM_RTSP=%d\n",
           USE_WIFI, ENABLE_OV3660_SETTINGS, STREAM_RTSP);
#if STREAM_RTSP
    printf(" >>> STREAM MODE: RTSP ONLY (port 8554, VLC / ZoneMinder) <<<\n");
    printf(" >>> HTTP /stream is DISABLED <<<\n");
#else
    printf(" >>> STREAM MODE: HTTP ONLY (port 80 /stream) <<<\n");
    printf(" >>> RTSP is NOT compiled in <<<\n");
#endif
#if ENABLE_OV3660_SETTINGS
    printf(" Web UI: full OV3660 settings\n");
#else
    printf(" Web UI: stream-only (no settings panel)\n");
#endif
    printf("========================================\n\n");

#if USE_WIFI
    if (!app_wifi_connect()) {
        return;
    }
    app_wifi_print_addresses();
#else
    if (!app_wlan_start()) {
        printf("HaLow connect failed — fix AP/credentials, then reset.\n");
        return;
    }
    app_wlan_print_addresses();
#endif

    /* Required before httpd_start (Seeed web_camera_serve); Wi-Fi mode may already have a loop. */
    esp_err_t ev = esp_event_loop_create_default();
    if (ev != ESP_OK && ev != ESP_ERR_INVALID_STATE) {
        printf("Event loop init failed: %s\n", esp_err_to_name(ev));
        return;
    }

    esp_err_t err = init_camera();
    if (err != ESP_OK) {
        printf("Camera init failed: %s\n", esp_err_to_name(err));
        return;
    }

    if (camera_http_init() != ESP_OK) {
        printf("Camera web server failed to start\n");
        return;
    }
#if STREAM_RTSP
    if (camera_rtsp_init() != ESP_OK) {
        printf("RTSP server failed to start\n");
    }
#endif
    print_ready();

    xTaskCreate(network_link_task, "net_link", 4096, NULL, 5, NULL);
}
