#include "sensors.h"
#include "networking.h"

#include <launch_core/backplane_defs.h>
#include <launch_core/types.h>
#include <launch_core/dev/dev_common.h>
#include <launch_core/dev/uart.h>
#include <launch_core/net/net_common.h>
#include <launch_core/net/udp.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define STACK_SIZE (512)

LOG_MODULE_REGISTER(main, CONFIG_APP_SENSOR_MODULE_LOG_LEVEL);

// Queues
static struct k_msgq ten_hz_telemetry_queue;
static uint8_t ten_hz_telemetry_queue_buffer[CONFIG_TEN_HZ_QUEUE_SIZE * sizeof(sensor_module_ten_hz_telemetry_t)];

// Devices
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

static int init(void) {
//    k_msgq_init(&ten_hz_telemetry_queue, ten_hz_telemetry_queue_buffer, sizeof(sensor_module_ten_hz_telemetry_t),
//                CONFIG_TEN_HZ_QUEUE_SIZE);



    // Tasks
//    k_thread_create(&telemetry_processing_thread, &telemetry_processing_stack[0], STACK_SIZE,
//                    telemetry_processing_task, NULL, NULL, NULL, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
//    k_thread_start(&telemetry_processing_thread);

    return 0;
}


int main() {
    if (init()) {
        return -1;
    }

    while (true) {
        gpio_pin_toggle_dt(&led0);
        gpio_pin_toggle_dt(&led1);
        k_msleep(100);
    }

    return 0;
}


