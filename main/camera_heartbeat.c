#include "camera_heartbeat.h"
#include "camera_build_config.h"

#if ENABLE_HEARTBEAT_LED

#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#ifndef HEARTBEAT_LED_GPIO
#error HEARTBEAT_LED_GPIO must be set in camera_build_config.h
#endif
#ifndef HEARTBEAT_LED_ON
#define HEARTBEAT_LED_ON 0
#endif
#ifndef HEARTBEAT_STABLE_MS
#define HEARTBEAT_STABLE_MS 5000
#endif
#ifndef HEARTBEAT_PULSE_MS
#define HEARTBEAT_PULSE_MS 200
#endif
#ifndef HEARTBEAT_PERIOD_MS
#define HEARTBEAT_PERIOD_MS 2000
#endif

static volatile bool s_link_up;
static volatile bool s_have_ip;
static volatile bool s_services_up;

static void led_set(bool on)
{
    gpio_set_level(HEARTBEAT_LED_GPIO, on ? HEARTBEAT_LED_ON : !HEARTBEAT_LED_ON);
}

static void heartbeat_task(void *arg)
{
    (void)arg;
    int64_t stable_start_ms = 0;
    bool heartbeating = false;
    bool logged_start = false;
    int64_t phase_start_ms = 0;
    bool pulse_on = false;

    for (;;) {
        bool ok = s_link_up && s_have_ip && s_services_up;
        int64_t now_ms = esp_timer_get_time() / 1000;

        if (!ok) {
            stable_start_ms = 0;
            heartbeating = false;
            logged_start = false;
            pulse_on = false;
            led_set(false);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if (stable_start_ms == 0) {
            stable_start_ms = now_ms;
        }

        if (!heartbeating) {
            led_set(false);
            if (now_ms - stable_start_ms >= HEARTBEAT_STABLE_MS) {
                heartbeating = true;
                pulse_on = true;
                phase_start_ms = now_ms;
                led_set(true);
            }
        } else {
            if (!logged_start) {
                logged_start = true;
                printf("Status LED heartbeat active (GPIO%d, active-%s)\n",
                       HEARTBEAT_LED_GPIO,
                       HEARTBEAT_LED_ON ? "high" : "low");
            }
            if (pulse_on) {
                if (now_ms - phase_start_ms >= HEARTBEAT_PULSE_MS) {
                    pulse_on = false;
                    phase_start_ms = now_ms;
                    led_set(false);
                }
            } else if (now_ms - phase_start_ms >= (HEARTBEAT_PERIOD_MS - HEARTBEAT_PULSE_MS)) {
                pulse_on = true;
                phase_start_ms = now_ms;
                led_set(true);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void camera_heartbeat_init(void)
{
    gpio_reset_pin(HEARTBEAT_LED_GPIO);
    const gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << HEARTBEAT_LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    led_set(false);
    xTaskCreate(heartbeat_task, "hb_led", 2048, NULL, 1, NULL);
}

void camera_heartbeat_set_status(bool link_up, bool have_ip, bool services_up)
{
    s_link_up = link_up;
    s_have_ip = have_ip;
    s_services_up = services_up;
}

#else

void camera_heartbeat_init(void) {}

void camera_heartbeat_set_status(bool link_up, bool have_ip, bool services_up)
{
    (void)link_up;
    (void)have_ip;
    (void)services_up;
}

#endif
