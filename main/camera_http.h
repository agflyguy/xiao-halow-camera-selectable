#pragma once

#include <stdbool.h>

#include "esp_err.h"

esp_err_t camera_http_init(void);
void camera_http_stop(void);
bool camera_http_is_running(void);
