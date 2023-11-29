/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <app_version.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/fs/fs.h>
//#include <zephyr/fs/littlefs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/storage/flash_map.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL
);
K_QUEUE_DEFINE(net_tx_queue);

#define STACK_SIZE (2048)
static K_THREAD_STACK_ARRAY_DEFINE(stacks, 2, STACK_SIZE);

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

static void init(void) {
    // Queues
    k_queue_init(&net_tx_queue);
    network_init();
}

int main() {
    init();

    return 0;
}




