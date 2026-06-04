#pragma once

#include <stdbool.h>

/** Connect to 2.4 GHz AP (same credentials as HT-HC33 USE_WIFI 1). Blocks until IP or timeout. */
bool app_wifi_connect(void);

bool app_wifi_link_is_up(void);

bool app_wifi_get_ip_addr(char *buf, unsigned buf_len);

void app_wifi_print_addresses(void);
