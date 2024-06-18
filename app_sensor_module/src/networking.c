// Self Include
#include "sensor_module.h"

// Launch Includes
#include <launch_core/dev/dev_common.h>
#include <launch_core/types.h>

// Zephyr Includes
#include <launch_core/backplane_defs.h>
#include <launch_core/net/udp.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(networking);

static void telemetry_broadcast_task(void*, void*, void*);

K_THREAD_DEFINE(telemetry_broadcast, 1024, telemetry_broadcast_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);

extern struct k_msgq hundred_hz_telem_queue;

int init_networking(void) {
    if (l_check_device(DEVICE_DT_GET_ONE(wiznet_w5500)) != 0) {
        LOG_ERR("Wiznet device not found");
        return -ENODEV;
    }

    int ret = l_init_udp_net_stack_default(SENSOR_MODULE_IP_ADDR);
    if (ret != 0) {
        LOG_ERR("Failed to initialize UDP networking stack: %d", ret);
        return -ENXIO;
    }

    return 0;
}

static void telemetry_broadcast_task(void*, void*, void*) {
    LOG_INF("Starting broadcast task");
    static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

    timed_sensor_module_hundred_hz_telemetry_t hundred_hz_telem;

    int hundred_hz_socket =
        l_init_udp_socket(SENSOR_MODULE_IP_ADDR, SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_HUNDRED_HZ_DATA_PORT);

#ifdef CONFIG_IREC_2024_DEMO

    int potato_socket = l_init_udp_socket(SENSOR_MODULE_IP_ADDR, SENSOR_MODULE_BASE_PORT);
#endif

    while (true) {
        gpio_pin_toggle_dt(&led1);
        if (0 == k_msgq_get(&hundred_hz_telem_queue, &hundred_hz_telem, K_MSEC(100))) {
            l_send_udp_broadcast(hundred_hz_socket, (uint8_t*) &hundred_hz_telem,
                                 sizeof(sensor_module_hundred_hz_telemetry_t),
                                 SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_HUNDRED_HZ_DATA_PORT);

#ifdef CONFIG_IREC_2024_DEMO
            float potato_data[3] = {0};
            read_potato_telemetry(&potato_data[0], &potato_data[1], &potato_data[2]);
            l_send_udp_broadcast(potato_socket, (const uint8_t*) potato_data, sizeof(potato_data), SENSOR_MODULE_BASE_PORT);

#endif
        }
    }
}
