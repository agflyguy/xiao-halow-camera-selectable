#include "app_wifi.h"
#include "app_log.h"
#include "camera_build_config.h"

#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"

static const char *TAG = "app_wifi";

/* Credentials from main/camera_build_config.h (same layout as HT-HC33) */

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static EventGroupHandle_t s_wifi_events;
static char ip_addr_str[16];
static bool s_sntp_started;

#if USE_STATIC_IP
static bool wifi_apply_static_ip(esp_netif_t *netif)
{
    esp_netif_ip_info_t info = {};
    esp_ip4_addr_t dns = {};

    if (esp_netif_str_to_ip4(STATIC_IP, &info.ip) != ESP_OK ||
        esp_netif_str_to_ip4(STATIC_GATEWAY, &info.gw) != ESP_OK ||
        esp_netif_str_to_ip4(STATIC_NETMASK, &info.netmask) != ESP_OK) {
        printf("Static IP: bad STATIC_* in camera_build_config.h\n");
        return false;
    }
    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(netif));
    ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &info));
    if (esp_netif_str_to_ip4(STATIC_DNS1, &dns) == ESP_OK && dns.addr != 0) {
        esp_netif_dns_info_t dns_info = { .ip = { .type = ESP_IPADDR_TYPE_V4, .u_ip.ip4 = dns } };
        ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info));
    }
    printf("IPv4: static %s gw %s\n", STATIC_IP, STATIC_GATEWAY);
    return true;
}
#endif

static void wifi_start_sntp_once(void)
{
    if (s_sntp_started) {
        return;
    }
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
    s_sntp_started = true;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_wifi_events, WIFI_CONNECTED_BIT);
        ip_addr_str[0] = '\0';
        app_log_printf("[Wi-Fi] disconnected — reconnecting...\n");
        esp_wifi_connect();
#if USE_STATIC_IP
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        snprintf(ip_addr_str, sizeof(ip_addr_str), STATIC_IP);
        ESP_LOGI(TAG, "IP: %s (static)", ip_addr_str);
        wifi_start_sntp_once();
        xEventGroupSetBits(s_wifi_events, WIFI_CONNECTED_BIT);
#endif
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        snprintf(ip_addr_str, sizeof(ip_addr_str), IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "IP: %s", ip_addr_str);
        wifi_start_sntp_once();
        xEventGroupSetBits(s_wifi_events, WIFI_CONNECTED_BIT);
    }
}

bool app_wifi_connect(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    s_wifi_events = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {};
    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, WIFI_PASSWORD, sizeof(wifi_config.sta.password) - 1);

    printf("Wi-Fi AP: \"%s\"\n", WIFI_SSID);
#if USE_STATIC_IP
    if (!wifi_apply_static_ip(sta_netif)) {
        return false;
    }
#else
    printf("IPv4: DHCP\n");
#endif
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    /* Camera DMA + JPEG capture stall when Wi-Fi modem sleep is enabled */
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    EventBits_t bits = xEventGroupWaitBits(s_wifi_events, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE, pdFALSE, pdMS_TO_TICKS(60000));
    if (bits & WIFI_CONNECTED_BIT) {
        return true;
    }
    printf("Wi-Fi timeout\n");
    return false;
}

bool app_wifi_link_is_up(void)
{
    return ip_addr_str[0] != '\0';
}

bool app_wifi_get_ip_addr(char *buf, unsigned buf_len)
{
    if (!app_wifi_link_is_up() || buf_len == 0) {
        return false;
    }
    strncpy(buf, ip_addr_str, buf_len - 1);
    buf[buf_len - 1] = '\0';
    return true;
}

void app_wifi_print_addresses(void)
{
    if (!app_wifi_link_is_up()) {
        printf("Network: Wi-Fi link down\n");
        return;
    }
    printf("\n========================================\n");
    printf(" %s — Wi-Fi mode (2.4 GHz)\n", CAMERA_TITLE);
    printf(" IP address:  %s\n", ip_addr_str);
    printf(" Camera URL:  http://%s/\n", ip_addr_str);
    printf("========================================\n\n");
}
