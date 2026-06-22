#pragma once

#include <stdbool.h>

/** Built-in status LED — short pulse after link + IP + HTTP stable. */
void camera_heartbeat_init(void);
void camera_heartbeat_set_status(bool link_up, bool have_ip, bool services_up);
