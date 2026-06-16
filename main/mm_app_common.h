/*
 * Copyright 2023 Morse Micro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>

void app_wlan_init(void);
/** @return false if join timed out (does not assert). */
bool app_wlan_start(void);
void app_wlan_stop(void);
void app_wlan_arp_send(void);

/** Scan for classic HaLow APs (same idea as HT-HC33 printHaLowScan). */
void app_wlan_print_scan(void);

/** Re-associate after link loss. @return true if DHCP/link is up again. */
bool app_wlan_reconnect(void);

bool app_wlan_link_is_up(void);
/** True if an IP was assigned (may still respond to ping after a brief mmipal down event). */
bool app_wlan_has_ip(void);
bool app_wlan_get_ip_addr(char *buf, unsigned buf_len);
void app_wlan_print_addresses(void);
