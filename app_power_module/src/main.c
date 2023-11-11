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
//#include <zephyr/net/socket.h>
#include <zephyr/storage/flash_map.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);
K_QUEUE_DEFINE(net_tx_queue);

#define MY_IP_ADDR      "192.168.1.2"
#define SERVER_IP_ADDR  "192.168.1.100"
#define SERVER_PORT     12345

#define STACK_SIZE (2048)
static K_THREAD_STACK_ARRAY_DEFINE(stacks, 2, STACK_SIZE);


static void net_tx() {
    int sock;
    struct sockaddr_in server_addr;
    struct device *eth_dev;
}

static void init(void) {
    // Queues
    k_queue_init(&net_tx_queue);

    // Threads
//    struct k_thread led_toggle_thread;
//    k_thread_create(&led_toggle_thread, stacks[0], STACK_SIZE,
//                    led_toggle, NULL, NULL, NULL,
//                    0, 0, K_NO_WAIT);
//    k_thread_start(&led_toggle_thread);
}

static void init_network() {
    struct net_if *net_interface = NULL;
    char addr_str[INET_ADDRSTRLEN] = {0};
    enum ethernet_hw_caps eth_caps = 0;
    int ret = -1;
    int if_index = -1;
}

int main(void) {
    const struct device *const ina = DEVICE_DT_GET_ONE(ti_ina219);
    struct sensor_value v_bus;
    struct sensor_value power;
    struct sensor_value current;

    if (!device_is_ready(ina)) {
        printf("Device %s is not ready.\n", ina->name);
        return 0;
    }

    while (true) {
        if (sensor_sample_fetch(ina)) {
            printf("Could not fetch sensor data.\n");
            return 0;
        }

        sensor_channel_get(ina, SENSOR_CHAN_VOLTAGE, &v_bus);
        sensor_channel_get(ina, SENSOR_CHAN_POWER, &power);
        sensor_channel_get(ina, SENSOR_CHAN_CURRENT, &current);

        printf("Bus: %f [V] -- "
               "Power: %f [W] -- "
               "Current: %f [A]\n",
               sensor_value_to_double(&v_bus),
               sensor_value_to_double(&power),
               sensor_value_to_double(&current));
        k_sleep(K_MSEC(2000));
    }

    return 0;
}

