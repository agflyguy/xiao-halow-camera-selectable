#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Serial log with [YYYY-MM-DD HH:MM:SS] (UTC when synced) or [uptime HH:MM:SS] prefix. */
void app_log_printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif
