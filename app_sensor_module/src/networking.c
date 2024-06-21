// Self Include
#include "sensor_module.h"

// Launch Includes
#include <launch_core/dev/dev_common.h>
#include <launch_core/types.h>

// Zephyr Includes
#include <launch_core/backplane_defs.h>
#include <launch_core/net/udp.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(networking);

static void telemetry_broadcast_task(void*, void*, void*);

K_THREAD_DEFINE(telemetry_broadcast, 1024, telemetry_broadcast_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);

extern struct k_msgq telem_queue;

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

    sensor_module_hundred_hz_telemetry_t telem;

    int hundred_hz_socket =
        l_init_udp_socket(SENSOR_MODULE_IP_ADDR, SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_TEN_HZ_DATA_PORT);

    while (true) {
        if (0 == k_msgq_get(&telem_queue, &telem, K_MSEC(20))) {
            l_send_udp_broadcast(hundred_hz_socket, (uint8_t*) &telem,
                                 sizeof(sensor_module_hundred_hz_telemetry_t),
                                 SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_HUNDRED_HZ_DATA_PORT);
        }
    }
}
