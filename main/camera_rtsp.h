#pragma once

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t camera_rtsp_init(void);
void camera_rtsp_stop(void);
bool camera_rtsp_is_running(void);

#ifdef __cplusplus
}
#endif
