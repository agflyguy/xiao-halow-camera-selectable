#include "camera_rtsp.h"

#include <new>
#include <stdio.h>
#include <string>

#include "CRtspSession.h"
#include "EspCameraStreamer.h"
#include "camera_build_config.h"

extern "C" {
#include "app_wifi.h"
#include "mm_app_common.h"
}

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "lwip/tcp.h"

static const char *TAG = "camera_rtsp";

#define RTSP_TASK_STACK_BYTES 24576
#define RTSP_PORT 8554
#define RTSP_PATH_PRESENTATION "mjpeg"
#define RTSP_PATH_STREAM "1"
#define CAMERA_WIDTH 160
#define CAMERA_HEIGHT 120

/* Pace between frames; RTP send time dominates on slow links. */
#if USE_WIFI
#define FRAME_PERIOD_MS 80
#else
#define FRAME_PERIOD_MS 400
#endif

static void tune_rtsp_client_socket(int sock)
{
    int sndbuf = 32768;
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));
    int nodelay = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
}

static TaskHandle_t s_rtsp_task = NULL;
static StackType_t *s_rtsp_stack = NULL;
static StaticTask_t s_rtsp_task_buf;
static int s_listen_sock = -1;
static volatile bool s_rtsp_stop_requested = false;
static volatile bool s_rtsp_running = false;

static bool get_device_ip(char *ip, size_t ip_len)
{
#if USE_WIFI
    return app_wifi_get_ip_addr(ip, ip_len);
#else
    return app_wlan_get_ip_addr(ip, ip_len);
#endif
}

static int create_listen_socket(void)
{
    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "socket() failed: errno %d", errno);
        return -1;
    }

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(RTSP_PORT);

    if (bind(listen_sock, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        ESP_LOGE(TAG, "bind(%d) failed: errno %d", RTSP_PORT, errno);
        close(listen_sock);
        return -1;
    }

    if (listen(listen_sock, 1) != 0) {
        ESP_LOGE(TAG, "listen() failed: errno %d", errno);
        close(listen_sock);
        return -1;
    }

    struct timeval tv = {.tv_sec = 0, .tv_usec = 10000};
    setsockopt(listen_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    return listen_sock;
}

static void rtsp_task(void *arg)
{
    (void)arg;

    s_listen_sock = create_listen_socket();
    if (s_listen_sock < 0) {
        s_rtsp_running = false;
        s_rtsp_task = NULL;
        vTaskDelete(NULL);
        return;
    }
    int listen_sock = s_listen_sock;

    EspCameraStreamer *streamer = nullptr;
    CRtspSession *session = nullptr;
    uint32_t last_frame_ms = 0;

    ESP_LOGI(TAG, "RTSP server listening on port %d", RTSP_PORT);

    while (!s_rtsp_stop_requested) {
        if (session) {
            /* Call session directly — CStreamer::handleRequests() deletes stopped
             * sessions itself, which would double-free if we also delete below. */
            session->handleRequests(0);

            if (session->m_stopped) {
                ESP_LOGI(TAG, "RTSP client disconnected");
                delete session;
                delete streamer;
                session = nullptr;
                streamer = nullptr;
                last_frame_ms = 0;
            } else {
                uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
                if (session->m_streaming &&
                    (now_ms > last_frame_ms + FRAME_PERIOD_MS || now_ms < last_frame_ms)) {
                    streamer->streamImage(now_ms);
                    last_frame_ms = now_ms;
                }
            }
        } else {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_sock =
                accept(listen_sock, (struct sockaddr *)&client_addr, &client_len);

            if (client_sock >= 0) {
                tune_rtsp_client_socket(client_sock);
                char ip[48];
                if (!get_device_ip(ip, sizeof(ip))) {
                    snprintf(ip, sizeof(ip), "0.0.0.0");
                }

                ESP_LOGI(TAG, "RTSP client connected — rtsp://%s:%d/%s/%s", ip, RTSP_PORT,
                         RTSP_PATH_PRESENTATION, RTSP_PATH_STREAM);

                char hostport[64];
                snprintf(hostport, sizeof(hostport), "%s:%d", ip, RTSP_PORT);

                streamer = new (std::nothrow) EspCameraStreamer(CAMERA_WIDTH, CAMERA_HEIGHT);
                if (!streamer) {
                    ESP_LOGE(TAG, "Out of memory for RTSP streamer");
                    close(client_sock);
                    continue;
                }
                streamer->setURI(hostport, RTSP_PATH_PRESENTATION, RTSP_PATH_STREAM);

                session = new (std::nothrow) CRtspSession(client_sock, streamer);
                if (!session) {
                    ESP_LOGE(TAG, "Out of memory for RTSP session");
                    delete streamer;
                    streamer = nullptr;
                    close(client_sock);
                    continue;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(session ? 2 : 10));
    }

    if (session) {
        delete session;
        session = nullptr;
    }
    if (streamer) {
        delete streamer;
        streamer = nullptr;
    }

    if (listen_sock >= 0) {
        close(listen_sock);
    }
    s_listen_sock = -1;
    s_rtsp_running = false;
    s_rtsp_task = NULL;
    vTaskDelete(NULL);
}

extern "C" esp_err_t camera_rtsp_init(void)
{
    if (s_rtsp_running || s_rtsp_task) {
        return ESP_OK;
    }

    char ip[48];
    if (!get_device_ip(ip, sizeof(ip))) {
        ESP_LOGW(TAG, "No IP yet — RTSP not started");
        return ESP_ERR_INVALID_STATE;
    }

    if (!s_rtsp_stack) {
        s_rtsp_stack = (StackType_t *)heap_caps_malloc(RTSP_TASK_STACK_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!s_rtsp_stack) {
            ESP_LOGE(TAG, "Failed to allocate internal RTSP task stack");
            return ESP_ERR_NO_MEM;
        }
    }

    s_rtsp_stop_requested = false;
    s_rtsp_running = true;

    s_rtsp_task = xTaskCreateStaticPinnedToCore(
        rtsp_task, "rtsp_srv", RTSP_TASK_STACK_BYTES / sizeof(StackType_t), NULL, 5,
        s_rtsp_stack, &s_rtsp_task_buf, tskNO_AFFINITY);
    if (!s_rtsp_task) {
        s_rtsp_running = false;
        ESP_LOGE(TAG, "Failed to create RTSP task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "RTSP started — rtsp://%s:%d/%s/%s", ip, RTSP_PORT, RTSP_PATH_PRESENTATION,
             RTSP_PATH_STREAM);
    return ESP_OK;
}

extern "C" void camera_rtsp_stop(void)
{
    if (!s_rtsp_running && !s_rtsp_task) {
        return;
    }

    s_rtsp_stop_requested = true;
    if (s_listen_sock >= 0) {
        close(s_listen_sock);
        s_listen_sock = -1;
    }

    for (int i = 0; i < 100 && s_rtsp_task; i++) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    s_rtsp_running = false;
    s_rtsp_task = NULL;
    ESP_LOGI(TAG, "RTSP stopped");
}

extern "C" bool camera_rtsp_is_running(void)
{
    return s_rtsp_running;
}
