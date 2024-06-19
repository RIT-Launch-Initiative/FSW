// Self Include
#include "sensor_module.h"

// Launch Include
#include <launch_core/backplane_defs.h>

// Zephyr Includes
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(potato_comms);

K_TIMER_DEFINE(blasting_timer, NULL, NULL);

#define BLASTING_PERIOD K_SECONDS(1)

#define COMMS_STACK_SIZE 512
void potato_comms_handler(void*);
K_THREAD_DEFINE(potato_comms_thread, COMMS_STACK_SIZE, potato_comms_handler, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0,
                1000);

// Globals
// l_event_notification_t
static l_event_notification_t toblast = 0x0; // Won't be blasting until we get boost

void potato_comms_handler(void*) {
    const struct device* topotato = DEVICE_DT_GET(DT_ALIAS(topotato));
    static const struct gpio_dt_spec de_hack = GPIO_DT_SPEC_GET(DT_NODELABEL(de_hack), gpios);
    // get uart, wait for
    gpio_pin_set_dt(&de_hack, 1);

    LOG_INF("Potato comms ready");
    while (1) {
        k_timer_status_sync(&blasting_timer);
        uart_poll_out(topotato, toblast);
        if (toblast == L_LANDING_DETECTED) {
            break;
        }
    }
    // Spam landing for a minute longer
    for (int i = 0; i < 60; i++) {
        uart_poll_out(topotato, L_LANDING_DETECTED);
        k_msleep(1000);
    }
}

int write_boost_detect_byte_modbus(uint8_t event_byte) {
    toblast = event_byte;
    if (event_byte == L_BOOST_DETECTED) {
        k_timer_start(&blasting_timer, BLASTING_PERIOD, BLASTING_PERIOD);
    }
    return 0;
}
