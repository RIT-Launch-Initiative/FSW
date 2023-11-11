/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <app_version.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

#include <zephyr/drivers/gpio.h>

#define SLEEP_TIME_MS   100
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);
K_QUEUE_DEFINE(lora_tx_queue);
K_QUEUE_DEFINE(net_tx_queue);

static void network_init() {

}


static void init(void) {
    // Queues
    k_queue_init(&lora_tx_queue);
    k_queue_init(&net_tx_queue);

    // Devices
    const struct device *const sx1276 = DEVICE_DT_GET_ONE(semtech_sx1276);
    if (!device_is_ready(sx1276)) {
        printk("Device %s is not ready.\n", sx1276->name);
    } else {
        printk("Device %s is ready.\n", sx1276->name);
    }

    const struct device *const wiznet = DEVICE_DT_GET_ONE(wiznet_w5500);
    if (!device_is_ready(wiznet)) {
        printk("Device %s is not ready.\n", wiznet->name);
    } else {
        printk("Device %s is ready.\n", wiznet->name);
    }

    network_init();
}

static void get_gnss(void) {

}


static void lora_tx() {
    if (!k_queue_is_empty(&lora_tx_queue)) {


    }
}

static void wiznet_tx() {

}

static void toggle_led() {

}

int main() {
    init();

    while (1) {
        gpio_pin_toggle_dt(&led0);
        gpio_pin_toggle_dt(&led1);
        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}