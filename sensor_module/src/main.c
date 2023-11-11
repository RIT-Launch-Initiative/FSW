/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <app_version.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/storage/flash_map.h>

#define SLEEP_TIME_MS   1000

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL
);
K_QUEUE_DEFINE(net_tx_queue);

static void network_init() {

}


static void init(void) {
    // Queues
    k_queue_init(&net_tx_queue);
    network_init();
}

static void lsm6dsl_task() {
//    const struct device *const lsm6dsl_dev = DEVICE_DT_GET_ONE(lsm6dsl);
//    struct sensor_value accel[3];
//
//    if (!device_is_ready(lsm6dsl_dev)) {
//        LOG_ERR("LSM6DSL device not ready");
//        return;
//    }

}

static void lis3mdl_task() {

}

static void ms5607_task() {

}

static void tmp117_task() {

}

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);


int main() {
    init();


    gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);

    while (1) {
        gpio_pin_toggle_dt(&led0);
        gpio_pin_toggle_dt(&led1);
        k_msleep(100);
    }

    return 0;
}




