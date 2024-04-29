/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <launch_core/backplane_defs.h>
#include <launch_core/net/net_common.h>
#include <launch_core/net/sntp.h>
#include <launch_core/net/udp.h>
#include <launch_core/os/time.h>
#include <launch_core/types.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/sntp.h>

#define QUEUE_PROCESSING_STACK_SIZE (1024)

#define POWER_MODULE_IP_ADDR BACKPLANE_IP(POWER_MODULE_ID, 2, 1) // TODO: Make this configurable

LOG_MODULE_REGISTER(main, CONFIG_APP_POWER_MODULE_LOG_LEVEL);

struct k_msgq ina_processing_queue;
static uint8_t ina_processing_queue_buffer[CONFIG_INA219_QUEUE_SIZE * sizeof(power_module_telemetry_t)];

static K_THREAD_STACK_DEFINE(ina_processing_stack, QUEUE_PROCESSING_STACK_SIZE);
static struct k_thread ina_processing_thread;

static const struct device *const wiznet = DEVICE_DT_GET_ONE(wiznet_w5500);

//static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
//static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
//static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);
//static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios);
//static const struct gpio_dt_spec led_wiznet = GPIO_DT_SPEC_GET(DT_ALIAS(ledwiz), gpios);

static int udp_sockets[1] = {0};
static int udp_socket_ports[1] = {POWER_MODULE_BASE_PORT + POWER_MODULE_INA_DATA_PORT};
static l_udp_socket_list_t udp_socket_list = {.sockets = udp_sockets, .num_sockets = 1};






static void init_networking() {
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

static void ina_queue_processing_task(void *, void *, void *) {
    power_module_telemetry_t sensor_telemetry = {0};
    int sock = udp_socket_list.sockets[0];

    while (true) {
        if (k_msgq_get(&ina_processing_queue, &sensor_telemetry, K_FOREVER)) {
            LOG_ERR("Failed to get data from INA219 processing queue");
            continue;
        }

        // TODO: write to flash when data logging library is ready
        l_send_udp_broadcast(sock, (uint8_t *) &sensor_telemetry, sizeof(power_module_telemetry_t),
                             POWER_MODULE_BASE_PORT + POWER_MODULE_INA_DATA_PORT);
    }
}

static int init(void) {
    k_msgq_init(&ina_processing_queue, ina_processing_queue_buffer, sizeof(power_module_telemetry_t),
                CONFIG_INA219_QUEUE_SIZE);

    init_networking();

    // TODO: Play with these values on rev 2 where we can do more profiling


    k_thread_create(&ina_processing_thread, &ina_processing_stack[0], QUEUE_PROCESSING_STACK_SIZE,
                    ina_queue_processing_task, NULL, NULL, NULL, K_PRIO_PREEMPT(10), 0, K_NO_WAIT);
    k_thread_start(&ina_processing_thread);

    return 0;
}

int main(void) {
    if (init()) {
        return -1;
    }

    while (true) {
        k_sleep(K_MSEC(100));
    }

    return 0;
}