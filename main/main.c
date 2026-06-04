/*
 * Seeed XIAO ESP32-S3 Sense + Wio-WM6108 (Halo kit)
 *
 * Dual network mode (same as HT-HC33 camera):
 *   USE_WIFI=0  HaLow STA — grey-AP / heltec.org (classic HaLow AP)
 *   USE_WIFI=1  2.4 GHz Wi-Fi — blue-2g / Dear!me2 (HAVEN point AP)
 *
 * Build:
 *   idf.py build                    # HaLow (default)
 *   idf.py -DUSE_WIFI=1 build       # 2.4 GHz Wi-Fi
 */

#include <stdio.h>
#include <string.h>

#include "mmosal.h"
#include "mm_app_common.h"
#include "app_wifi.h"
#include "camera_http.h"

#include "esp_camera.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifndef USE_WIFI
#define USE_WIFI 0
#endif

static const char *TAG = "xiao_camera";

#define CONFIG_XCLK_FREQ 20000000

static bool frame_has_jpeg_soi(const camera_fb_t *fb)
{
    return fb && fb->format == PIXFORMAT_JPEG && fb->len >= 2 && fb->buf[0] == 0xFF && fb->buf[1] == 0xD8;
}

/* OV3660 often logs NO-SOI at CIF; Seeed example uses VGA + PSRAM + fb_count 1 */
static void camera_warmup(void)
{
    int valid = 0;
    for (int i = 0; i < 50 && valid < 8; i++) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (frame_has_jpeg_soi(fb)) {
            valid++;
        }
        if (fb) {
            esp_camera_fb_return(fb);
        }
        vTaskDelay(pdMS_TO_TICKS(80));
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
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
        .fb_count = 1,
        /* QVGA: stable OV3660 + small enough for HaLow and Wi-Fi */
        .frame_size = FRAMESIZE_QVGA,
#if USE_WIFI
        .jpeg_quality = 14,
#else
        .jpeg_quality = 18,
#endif
    };
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        return err;
    }

    sensor_t *s = esp_camera_sensor_get();
    if (s) {
        s->set_vflip(s, 1);
        s->set_brightness(s, 1);
        s->set_saturation(s, 0);
    }

    camera_warmup();
    return ESP_OK;
}

static void print_ready(void)
{
    char ip[48];
#if USE_WIFI
    if (app_wifi_get_ip_addr(ip, sizeof(ip))) {
        printf("Camera ready — http://%s/\n\n", ip);
    }
#else
    if (app_wlan_get_ip_addr(ip, sizeof(ip))) {
        printf("Camera ready — http://%s/\n\n", ip);
    }
#endif
}

static void restart_camera_http(void)
{
    if (camera_http_init() == ESP_OK) {
        print_ready();
    } else {
        printf("Web server failed to restart — refresh after IP appears\n");
    }
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
            if (!camera_http_is_running()) {
#if USE_WIFI
                printf("[Wi-Fi] link up — starting web server\n");
#else
                printf("[HaLow] link up — starting web server\n");
#endif
                restart_camera_http();
            }
            link_was_up = true;
        } else {
            if (link_was_up || camera_http_is_running()) {
#if USE_WIFI
                printf("[Wi-Fi] link lost — stopping web server\n");
#else
                printf("[HaLow] link lost — stopping web server\n");
#endif
                camera_http_stop();
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
    print_ready();

    xTaskCreate(network_link_task, "net_link", 4096, NULL, 5, NULL);
}
