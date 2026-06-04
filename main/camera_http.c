#include "camera_http.h"

#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifndef USE_WIFI
#define USE_WIFI 0
#endif

static const char *TAG = "camera_http";

#if USE_WIFI
#define FRAME_PERIOD_MS 120
#else
#define FRAME_PERIOD_MS 280
#endif

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static httpd_handle_t s_server;
static bool s_running;

static bool jpeg_has_soi(const uint8_t *buf, size_t len)
{
    return len >= 2 && buf[0] == 0xFF && buf[1] == 0xD8;
}

static void stream_end(httpd_req_t *req)
{
    httpd_resp_send_chunk(req, NULL, 0);
}

static esp_err_t favicon_handler(httpd_req_t *req)
{
    httpd_resp_set_status(req, "204 No Content");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t jpg_stream_httpd_handler(httpd_req_t *req)
{
    camera_fb_t *fb = NULL;
    esp_err_t res;
    size_t jpg_len;
    uint8_t *jpg_buf;
    char part_buf[64];
    uint32_t frame_idx = 0;

    ESP_LOGI(TAG, "stream client connected");

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK) {
        return res;
    }
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "Connection", "close");

    while (true) {
        fb = esp_camera_fb_get();
        if (!fb) {
            vTaskDelay(pdMS_TO_TICKS(30));
            continue;
        }

        if (fb->format != PIXFORMAT_JPEG) {
            bool ok = frame2jpg(fb, 80, &jpg_buf, &jpg_len);
            esp_camera_fb_return(fb);
            fb = NULL;
            if (!ok) {
                vTaskDelay(pdMS_TO_TICKS(30));
                continue;
            }
        } else {
            jpg_len = fb->len;
            jpg_buf = fb->buf;
        }

        if (!jpeg_has_soi(jpg_buf, jpg_len)) {
            if (fb) {
                esp_camera_fb_return(fb);
                fb = NULL;
            } else {
                free(jpg_buf);
            }
            vTaskDelay(pdMS_TO_TICKS(30));
            continue;
        }

        res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        if (res == ESP_OK) {
            size_t hlen = snprintf(part_buf, sizeof(part_buf), _STREAM_PART, (unsigned)jpg_len);
            res = httpd_resp_send_chunk(req, part_buf, hlen);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char *)jpg_buf, jpg_len);
        }

        if (fb) {
            if (fb->format != PIXFORMAT_JPEG) {
                free(jpg_buf);
            }
            esp_camera_fb_return(fb);
            fb = NULL;
        }

        /* Client closed tab / TCP dead — must exit, not spin on send errors */
        if (res != ESP_OK) {
            ESP_LOGI(TAG, "stream ended (%s)", esp_err_to_name(res));
            stream_end(req);
            break;
        }

        frame_idx++;
        if ((frame_idx % 60) == 0) {
            ESP_LOGI(TAG, "streaming, last frame %u KB", (unsigned)(jpg_len / 1024));
        }

        vTaskDelay(pdMS_TO_TICKS(FRAME_PERIOD_MS));
    }

    ESP_LOGI(TAG, "stream client disconnected");
    return ESP_OK;
}

static httpd_uri_t uri_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = jpg_stream_httpd_handler,
};

static httpd_uri_t uri_favicon = {
    .uri = "/favicon.ico",
    .method = HTTP_GET,
    .handler = favicon_handler,
};

esp_err_t camera_http_init(void)
{
    if (s_running) {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 12288;
    config.send_wait_timeout = 10;
    config.recv_wait_timeout = 5;
    config.lru_purge_enable = true;
    config.max_open_sockets = 5;
    config.max_uri_handlers = 8;
    config.core_id = tskNO_AFFINITY;

    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        return err;
    }
    if (httpd_register_uri_handler(s_server, &uri_get) != ESP_OK ||
        httpd_register_uri_handler(s_server, &uri_favicon) != ESP_OK) {
        httpd_stop(s_server);
        s_server = NULL;
        return ESP_FAIL;
    }
    s_running = true;
    ESP_LOGI(TAG, "HTTP server on port %d", config.server_port);
    return ESP_OK;
}

void camera_http_stop(void)
{
    if (!s_running) {
        return;
    }
    httpd_stop(s_server);
    s_server = NULL;
    s_running = false;
    ESP_LOGI(TAG, "HTTP server stopped");
}

bool camera_http_is_running(void)
{
    return s_running;
}
