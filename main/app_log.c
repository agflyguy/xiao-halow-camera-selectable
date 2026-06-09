#include "app_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "esp_timer.h"

static void app_log_format_time(char *buf, size_t len)
{
    time_t now = time(NULL);

    /* Roughly after 2020-01-01 — SNTP or other time sync has run. */
    if (now >= 1577836800) {
        struct tm tm_utc;
        gmtime_r(&now, &tm_utc);
        snprintf(buf, len, "%04d-%02d-%02d %02d:%02d:%02d UTC",
                 tm_utc.tm_year + 1900, tm_utc.tm_mon + 1, tm_utc.tm_mday,
                 tm_utc.tm_hour, tm_utc.tm_min, tm_utc.tm_sec);
        return;
    }

    int64_t sec = esp_timer_get_time() / 1000000;
    int h = (int)(sec / 3600);
    int m = (int)((sec % 3600) / 60);
    int s = (int)(sec % 60);
    snprintf(buf, len, "uptime %02d:%02d:%02d", h, m, s);
}

void app_log_printf(const char *fmt, ...)
{
    char ts[40];
    app_log_format_time(ts, sizeof(ts));
    printf("[%s] ", ts);

    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}
