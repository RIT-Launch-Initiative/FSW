/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file main.c
 * @authors Nate Aquino naquino14@outlook.com
 *          Aaron Chan
 * @brief Radio Module Entry Point
 */

#include <launch_core/backplane_defs.h>
#include <launch_core/dev/dev_common.h>
#include <launch_core/net/lora.h>
#include <launch_core/net/net_common.h>
#include <launch_core/net/udp.h>

#include <zephyr/console/console.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/random/random.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>


#include "ubxlib_utils.h"

#define SLEEP_TIME_MS 100

// logging
LOG_MODULE_REGISTER(main);

// LoRa queue
K_QUEUE_DEFINE(lora_tx_queue);
K_QUEUE_DEFINE(net_tx_queue);

// GNSS
gnss_dev_t *gnss_dev;

extern int init_maxm10s(gnss_dev_t *dev);

static struct k_thread gnss_init_thread_data;
static k_tid_t gnss_init_tid;

// device setup
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
//static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_NODELABEL(reset), ublox_reset);
static const struct device *const lora_dev = DEVICE_DT_GET_ONE(semtech_sx1276);
static const struct device *const wiznet = DEVICE_DT_GET_ONE(wiznet_w5500);

static int init() {
    char ip[MAX_IP_ADDRESS_STR_LEN];
    int ret = 0;

    k_queue_init(&net_tx_queue);

    if (!l_check_device(lora_dev)) {
        l_lora_configure(lora_dev, false);
    }

    if (0 > l_create_ip_str_default_net_id(ip, RADIO_MODULE_ID, 1)) {
        LOG_ERR("Failed to create IP address string: %d", ret);
        return -1;
    }

    if (!l_check_device(wiznet)) {
        l_init_udp_net_stack("192.168.1.1");
    }

    if (0 > init_maxm10s(gnss_dev)) {
        LOG_ERR("Failed to initialize GNSS module");
        return -1;
    } else {
        LOG_INF("GNSS module initialized");
    }

    return ret;
}

int main() {
    LOG_DBG("Starting radio module!\n");

    if (init()) {
        return -1;
    }

    while (1) {
        gpio_pin_toggle_dt(&led0);
        gpio_pin_toggle_dt(&led1);
        k_msleep(100);
    }

    return 0;
}

// int main() {
//     init();
//     printk("Receiver started\n");
//     while (1) {
//         int ret = lora_recv_async(lora_dev, lora_debug_recv_cb);
//     }
//
//     return 0;
// }
