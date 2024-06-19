// Self Include
#include "potato.h"

// Launch Includes
#include <launch_core/backplane_defs.h>

// Zephyr Includes
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(sensor_comms, LOG_LEVEL_INF);

#define SMOD_STACK_SIZE 1024
static void smod_read_task(void*);
K_THREAD_DEFINE(smod_read_thread, SMOD_STACK_SIZE, smod_read_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);

extern volatile uint8_t event_byte;

static void smod_read_task(void*) {
    const struct device* from_smod = DEVICE_DT_GET(DT_ALIAS(sensor_mod));
    static const struct gpio_dt_spec de_hack = GPIO_DT_SPEC_GET(DT_NODELABEL(de_hack), gpios);
    static const struct gpio_dt_spec re_hack = GPIO_DT_SPEC_GET(DT_NODELABEL(re_hack), gpios);
    gpio_pin_set_dt(&de_hack, 0);
    gpio_pin_set_dt(&re_hack, 1);

    unsigned char inbyte = 0x0;
    LOG_INF("Listening for sensor mod");

    while (true) {
        // 0 if got char, -1 otherwise
        if (0 == uart_poll_in(from_smod, &inbyte)) {
            event_byte = inbyte;
        }
        k_msleep(200);
    }
}
