/*
 * Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "power_module.h"

#include <launch_core/dev/dev_common.h>
#include <launch_core/net/udp.h>
#include <launch_core/net/net_common.h>
#include <launch_core/backplane_defs.h>
#include <launch_core/types.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

#define POWER_MODULE_IP_ADDR BACKPLANE_IP(POWER_MODULE_ID, 2, 1) // TODO: Make this configurable
#define QUEUE_PROCESSING_STACK_SIZE (1024)
#define NUM_SOCKETS 1

K_THREAD_DEFINE(telemetry_broadcast, QUEUE_PROCESSING_STACK_SIZE,
                telemetry_broadcast_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);

extern struct k_msgq ina_telemetry_msgq;
extern struct k_msgq adc_telemetry_msgq;


// TODO: Just use base port for output. Bind to launch events port when we do state machien
static int udp_sockets[NUM_SOCKETS] = {0};
static int udp_socket_ports[] = {POWER_MODULE_BASE_PORT + POWER_MODULE_INA_DATA_PORT};
static l_udp_socket_list_t udp_socket_list = {.sockets = udp_sockets, .num_sockets = NUM_SOCKETS};

LOG_MODULE_REGISTER(networking);

static void init_networking() {
    const struct device *wiznet = DEVICE_DT_GET_ONE(wiznet_w5500);
    if (l_check_device(wiznet) != 0) {
        LOG_ERR("Wiznet device not found");
        return;
    }

    int ret = l_init_udp_net_stack_default(POWER_MODULE_IP_ADDR);
    if (ret != 0) {
        LOG_ERR("Failed to initialize UDP networking stack: %d", ret);
        return;
    }

    for (int i = 0; i < udp_socket_list.num_sockets; i++) {
        udp_socket_list.sockets[i] = l_init_udp_socket(POWER_MODULE_IP_ADDR, udp_socket_ports[i]);
        if (udp_socket_list.sockets[i] < 0) {
            LOG_ERR("Failed to create UDP socket: %d", udp_socket_list.sockets[i]);
            return;
        }
    }
}

void telemetry_broadcast_task(void) {
    static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);
    static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios);
    power_module_telemetry_t sensor_telemetry = {0};
    float vin_adc_data_v = 0.0f;
    int sock = udp_socket_list.sockets[0];

    init_networking();

    // TODO: write to flash when data logging library is ready
    // TODO: See about delegating logging to another task. Would need to profile. Would probably do with zbus
    while (true) {
        if (!k_msgq_get(&ina_telemetry_msgq, &sensor_telemetry, K_MSEC(10))) {
            gpio_pin_toggle_dt(&led2);
            // l_send_udp_broadcast(sock, (uint8_t *) &sensor_telemetry, sizeof(power_module_telemetry_t),
                                 // POWER_MODULE_BASE_PORT + POWER_MODULE_INA_DATA_PORT);
        }

        // TODO: LED doesn't seem to be blinking. Debug shows this gets skipped, but get function seems successful
        if (!k_msgq_get(&adc_telemetry_msgq, &vin_adc_data_v, K_MSEC(10))) {
            gpio_pin_toggle_dt(&led3);
#ifdef CONFIG_DEBUG
            // l_send_udp_broadcast(sock, (uint8_t *) &vin_adc_data_v, sizeof(float),
                                 // POWER_MODULE_BASE_PORT + POWER_MODULE_ADC_DATA_PORT);
#endif
        }
    }
}
