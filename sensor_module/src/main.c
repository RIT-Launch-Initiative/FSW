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

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL
);
K_QUEUE_DEFINE(net_tx_queue);

#define STACK_SIZE (2048)
static K_THREAD_STACK_ARRAY_DEFINE(stacks, 2, STACK_SIZE);

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

static void network_init() {

}



static void lsm6dsl_task(void *unused0, void *unused1, void *unused2) {
    const struct device *const lsm6dsl_dev = DEVICE_DT_GET_ONE(st_lsm6dsl);
    struct sensor_value accel[3];

    if (!device_is_ready(lsm6dsl_dev)) {
        LOG_ERR("LSM6DSL device not ready");
        return;
    }

    while (1) {
        if (sensor_sample_fetch(lsm6dsl_dev) < 0) {
            printk("LSM6DSL sample update error");
        }

        if (sensor_channel_get(lsm6dsl_dev, SENSOR_CHAN_ACCEL_XYZ, accel) < 0) {
            printk("Cannot read LSM6DSL accel channels");
        }

        printk("LSM6DSL: X: %f, Y: %f, Z: %f",
               sensor_value_to_double(&accel[0]),
               sensor_value_to_double(&accel[1]),
               sensor_value_to_double(&accel[2]));

        k_msleep(100);
    }
}

static void lis3mdl_task() {

}

static void ms5607_task() {

}

static void tmp117_task() {

}

static void led_toggle(void *unused0, void *unused1, void *unused2) {
    gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);

    while (1) {
        gpio_pin_toggle_dt(&led0);
        gpio_pin_toggle_dt(&led1);
        k_msleep(100);
    }
}


static void init(void) {
    // Queues
    k_queue_init(&net_tx_queue);
    network_init();

    // Threads
    struct k_thread led_toggle_thread;
    struct k_thread lsm6dsl_thread;

    k_thread_create(&led_toggle_thread, stacks[0], STACK_SIZE,
                    led_toggle, NULL, NULL, NULL,
                    0, 0, K_NO_WAIT);

    k_thread_create(&lsm6dsl_thread, stacks[1], STACK_SIZE,
                    lsm6dsl_task, NULL, NULL, NULL,
                    0, 0, K_NO_WAIT);

    k_thread_start(&led_toggle_thread);
    k_thread_start(&lsm6dsl_thread);

}

int main() {
    init();




	k_sleep(K_MSEC(5000));

    return 0;
}




