/*
 * Copyright 2023 Morse Micro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "mmosal.h"
#include "mmhal.h"
#include "mmwlan.h"
#include "mmipal.h"
#include "lwip/ip_addr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mm_app_common.h"
#include "mm_app_loadconfig.h"
#include "camera_build_config.h"

static struct mmosal_semb *link_established;
static bool link_up;
static bool stack_ready;
static char ip_addr_str[MMIPAL_IPADDR_STR_MAXLEN];
static char gateway_str[MMIPAL_IPADDR_STR_MAXLEN];
static char netmask_str[MMIPAL_IPADDR_STR_MAXLEN];
static uint32_t ip_addr_u32;
static uint32_t gw_addr_u32;
static uint8_t mac_addr[MMWLAN_MAC_ADDR_LEN];

static void sta_status_callback(enum mmwlan_sta_state sta_state)
{
    switch (sta_state) {
    case MMWLAN_STA_DISABLED:
        printf("[HaLow] STA disabled\n");
        break;
    case MMWLAN_STA_CONNECTING:
        printf("[HaLow] connecting...\n");
        break;
    case MMWLAN_STA_CONNECTED:
        printf("[HaLow] link up\n");
        break;
    }
}

static void link_status_callback(const struct mmipal_link_status *link_status)
{
    uint32_t time_ms = mmosal_get_time_ms();
    if (link_status->link_state == MMIPAL_LINK_UP) {
        printf("[HaLow] IP: %s  gateway %s\n", link_status->ip_addr, link_status->gateway);

        (void)mmosal_safer_strcpy(ip_addr_str, link_status->ip_addr, sizeof(ip_addr_str));
        (void)mmosal_safer_strcpy(gateway_str, link_status->gateway, sizeof(gateway_str));
        (void)mmosal_safer_strcpy(netmask_str, link_status->netmask, sizeof(netmask_str));

        ip_addr_u32 = ipaddr_addr(link_status->ip_addr);
        gw_addr_u32 = ipaddr_addr(link_status->gateway);
        link_up = true;
        mmosal_semb_give(link_established);
        app_wlan_arp_send();
    } else {
        printf("[HaLow] link down (%lu ms)\n", (unsigned long)time_ms);
        link_up = false;
        if (link_established) {
            while (mmosal_semb_wait(link_established, 0)) {
            }
        }
    }
}

void app_wlan_init(void)
{
    enum mmwlan_status status;
    struct mmwlan_version version;

    if (link_established == NULL) {
        link_established = mmosal_semb_create("link_established");
    }

    if (stack_ready) {
        return;
    }

    mmhal_init();
    mmwlan_init();
    mmwlan_set_channel_list(load_channel_list());

    struct mmipal_init_args mmipal_init_args = MMIPAL_INIT_ARGS_DEFAULT;
    load_mmipal_init_args(&mmipal_init_args);
    if (mmipal_init(&mmipal_init_args) != MMIPAL_SUCCESS) {
        printf("mmipal_init failed\n");
        MMOSAL_ASSERT(false);
    }
    mmipal_set_link_status_callback(link_status_callback);

    status = mmwlan_get_version(&version);
    MMOSAL_ASSERT(status == MMWLAN_SUCCESS);
    printf("Morse FW %s | morselib %s | chip 0x%lx\n",
           version.morse_fw_version, version.morselib_version, version.morse_chip_id);

    status = mmwlan_get_mac_addr(mac_addr);
    MMOSAL_ASSERT(status == MMWLAN_SUCCESS);
    printf("HaLow MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

    stack_ready = true;
}

static struct mmosal_semb *scan_done;
static bool target_ssid_seen;
static bool prescan_done;

#define SCAN_MAX_UNIQUE 32

typedef struct {
    char ssid[MMWLAN_SSID_MAXLEN + 1];
    int8_t rssi;
} scan_entry_t;

static scan_entry_t scan_table[SCAN_MAX_UNIQUE];
static int scan_unique;

static void scan_reset(const char *target)
{
    (void)target;
    scan_unique = 0;
    target_ssid_seen = false;
}

static void scan_trim_ssid(char *ssid)
{
    size_t len = strlen(ssid);
    while (len > 0 && (ssid[len - 1] == ' ' || ssid[len - 1] == '\t')) {
        ssid[--len] = '\0';
    }
}

static bool scan_rssi_valid(int rssi)
{
    return rssi < 0 && rssi >= -120;
}

static int8_t scan_pick_rssi(int8_t current, int candidate)
{
    if (!scan_rssi_valid(candidate)) {
        return current;
    }
    if (!scan_rssi_valid(current)) {
        return (int8_t)candidate;
    }
    return (candidate > current) ? (int8_t)candidate : current;
}

static void scan_note_ap(const char *ssid, int rssi, const char *target)
{
    char key[MMWLAN_SSID_MAXLEN + 1];
    (void)mmosal_safer_strcpy(key, ssid, sizeof(key));
    scan_trim_ssid(key);

    for (int i = 0; i < scan_unique; i++) {
        if (strcmp(scan_table[i].ssid, key) == 0) {
            scan_table[i].rssi = scan_pick_rssi(scan_table[i].rssi, rssi);
            return;
        }
    }
    if (scan_unique >= SCAN_MAX_UNIQUE) {
        return;
    }
    (void)mmosal_safer_strcpy(scan_table[scan_unique].ssid, key, sizeof(scan_table[0].ssid));
    scan_table[scan_unique].rssi = scan_rssi_valid(rssi) ? (int8_t)rssi : (int8_t)-128;
    scan_unique++;
    if (target && strcmp(key, target) == 0) {
        target_ssid_seen = true;
    }
}

static void scan_print_results(void)
{
    for (int i = 0; i < scan_unique; i++) {
        if (scan_rssi_valid(scan_table[i].rssi)) {
            printf("    %-32s  RSSI %d\n", scan_table[i].ssid, (int)scan_table[i].rssi);
        } else {
            printf("    %-32s  RSSI n/a\n", scan_table[i].ssid);
        }
    }
}

static void scan_rx_callback(const struct mmwlan_scan_result *result, void *arg)
{
    const char *target = (const char *)arg;
    if (result->ssid_len == 0) {
        return;
    }
    char ssid[MMWLAN_SSID_MAXLEN + 1];
    size_t len = result->ssid_len;
    if (len > MMWLAN_SSID_MAXLEN) {
        len = MMWLAN_SSID_MAXLEN;
    }
    memcpy(ssid, result->ssid, len);
    ssid[len] = '\0';
    scan_note_ap(ssid, (int)result->rssi, target);
}

static void scan_complete_callback(enum mmwlan_scan_state state, void *arg)
{
    (void)state;
    (void)arg;
    if (scan_done) {
        mmosal_semb_give(scan_done);
    }
}

void app_wlan_print_scan(void)
{
    const char *target = load_halow_ssid_default();

    /* Scan with STA off — join/reconnect while associated can miss APs. */
    (void)mmwlan_sta_disable();
    vTaskDelay(pdMS_TO_TICKS(800));

    scan_reset(target);
    if (scan_done == NULL) {
        scan_done = mmosal_semb_create("scan_done");
    }

    printf("Scanning HaLow APs (looking for \"%s\")...\n", target);
    struct mmwlan_scan_req scan_req = MMWLAN_SCAN_REQ_INIT;
    scan_req.scan_rx_cb = scan_rx_callback;
    scan_req.scan_complete_cb = scan_complete_callback;
    scan_req.scan_cb_arg = (void *)target;

    enum mmwlan_status status = mmwlan_scan_request(&scan_req);
    if (status != MMWLAN_SUCCESS) {
        printf("  Scan start failed (%d)\n", (int)status);
        return;
    }

    mmosal_semb_wait(scan_done, 45000);

    scan_print_results();

    if (scan_unique == 0) {
        printf("  No HaLow APs heard\n");
    } else {
        printf("  %d AP(s)\n", scan_unique);
    }
    if (!target_ssid_seen) {
        printf("  WARNING: \"%s\" was NOT in the scan.\n", target);
        printf("  OPENMANET mesh ID (haven) is often NOT the joinable SSID.\n");
        printf("  Use the SSID listed above (e.g. grey-AP) or your LuCI HaLow network name.\n");
        printf("  Rebuild: idf.py -DHALOW_SSID=grey-AP -DHALOW_PASSPHRASE=heltec.org build\n");
    }
}

static bool join_halow(bool first_join)
{
    enum mmwlan_status status;
    struct mmwlan_sta_args sta_args = MMWLAN_STA_ARGS_INIT;

    load_mmwlan_sta_args(&sta_args);
    load_mmwlan_settings();

    if (first_join) {
        printf("HaLow target: \"%s\"  security: SAE  country: US\n", sta_args.ssid);
        if (!prescan_done) {
            app_wlan_print_scan();
            prescan_done = true;
        }
        if (!target_ssid_seen) {
            printf("Join may fail — target SSID not visible in scan.\n");
        }
        printf("Joining HaLow (30-90 s)...\n");
    } else {
        printf("[HaLow] reconnecting...\n");
    }

    link_up = false;
    ip_addr_str[0] = '\0';
    if (link_established) {
        while (mmosal_semb_wait(link_established, 0)) {
        }
    }

    status = mmwlan_sta_enable(&sta_args, sta_status_callback);
    if (status != MMWLAN_SUCCESS) {
        return false;
    }

    return mmosal_semb_wait(link_established, first_join ? 120000 : 60000);
}

bool app_wlan_start(void)
{
    app_wlan_init();
    if (!join_halow(true)) {
        printf("HaLow timeout — check grey-AP is up and in range.\n");
        return false;
    }
    return true;
}

bool app_wlan_reconnect(void)
{
    (void)mmwlan_sta_disable();
    return join_halow(false);
}

void app_wlan_stop(void)
{
    mmwlan_sta_disable();
    mmwlan_shutdown();
}

bool app_wlan_link_is_up(void)
{
    return link_up && ip_addr_str[0] != '\0';
}

bool app_wlan_has_ip(void)
{
    return ip_addr_str[0] != '\0' && ip_addr_u32 != 0;
}

bool app_wlan_get_ip_addr(char *buf, unsigned buf_len)
{
    if (buf_len == 0 || ip_addr_str[0] == '\0') {
        return false;
    }
    (void)mmosal_safer_strcpy(buf, ip_addr_str, buf_len);
    return true;
}

void app_wlan_arp_send(void)
{
    if (!link_up) {
        return;
    }

    uint8_t arp_packet[] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5],
        0x08, 0x06,
        0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01,
        mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5],
        ((uint8_t *)&ip_addr_u32)[0], ((uint8_t *)&ip_addr_u32)[1],
        ((uint8_t *)&ip_addr_u32)[2], ((uint8_t *)&ip_addr_u32)[3],
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        ((uint8_t *)&gw_addr_u32)[0], ((uint8_t *)&gw_addr_u32)[1],
        ((uint8_t *)&gw_addr_u32)[2], ((uint8_t *)&gw_addr_u32)[3],
    };

    enum mmwlan_status status = mmwlan_tx(arp_packet, sizeof(arp_packet));
    if (status != MMWLAN_SUCCESS) {
        printf("ARP keepalive TX failed: %d\n", (int)status);
    }
}

void app_wlan_print_addresses(void)
{
    if (!app_wlan_link_is_up()) {
        printf("Network: link down\n");
        return;
    }
    printf("\n========================================\n");
    printf(" %s — HaLow mode (classic AP)\n", CAMERA_TITLE);
    printf(" IP address:  %s\n", ip_addr_str);
    printf(" Netmask:     %s\n", netmask_str);
    printf(" Gateway:     %s\n", gateway_str);
    printf(" Camera URL:  http://%s/\n", ip_addr_str);
    printf("========================================\n\n");
}
