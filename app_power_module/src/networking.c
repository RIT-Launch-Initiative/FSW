/*
 * Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Self Include
#include "power_module.h"

// Launch Includes
#include <launch_core/backplane_defs.h>
#include <launch_core/dev/dev_common.h>
#include <launch_core/net/net_common.h>
#include <launch_core/net/udp.h>
#include <launch_core/types.h>

// Zephyr Includes
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#define QUEUE_PROCESSING_STACK_SIZE (1024)
#define NUM_SOCKETS                 1

// TODO: Annoying RTOS setup where priority needs to be higher than other tasks and queues need to sleep
// Otherwise, logging task gets starved during flight
K_THREAD_DEFINE(telemetry_broadcast, QUEUE_PROCESSING_STACK_SIZE, telemetry_broadcast_task, NULL, NULL, NULL,
                K_PRIO_PREEMPT(19), 0, 1000);

extern struct k_msgq ina_telemetry_msgq;
extern struct k_msgq adc_telemetry_msgq;

static int udp_sockets[NUM_SOCKETS] = {0};
static int udp_socket_ports[] = {POWER_MODULE_BASE_PORT + POWER_MODULE_INA_DATA_PORT};
static l_udp_socket_list_t udp_socket_list = {.sockets = udp_sockets, .num_sockets = NUM_SOCKETS};

LOG_MODULE_REGISTER(networking);

void init_networking() {
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
    static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
    static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

    timed_power_module_telemetry_t sensor_telemetry = {0};
    timed_adc_data vin_adc_data_v = {0};
    int sock = udp_socket_list.sockets[0];

    while (true) {
        if (!k_msgq_get(&ina_telemetry_msgq, &sensor_telemetry, K_MSEC(10))) {
            gpio_pin_toggle_dt(&led0);
            l_send_udp_broadcast(sock, (uint8_t *) &sensor_telemetry, sizeof(timed_power_module_telemetry_t),
                                 POWER_MODULE_BASE_PORT + POWER_MODULE_INA_DATA_PORT);
        }

        if (!k_msgq_get(&adc_telemetry_msgq, &vin_adc_data_v, K_MSEC(10))) {
            gpio_pin_toggle_dt(&led2);
#ifdef CONFIG_DEBUG
            l_send_udp_broadcast(sock, (uint8_t *) &vin_adc_data_v, sizeof(timed_adc_data),
                                 POWER_MODULE_BASE_PORT + POWER_MODULE_ADC_DATA_PORT);
#endif
        }
    }
}
