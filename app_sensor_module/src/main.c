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

#include "sensors.h"
#include "net_utils.h"

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);
K_QUEUE_DEFINE(net_tx_queue);

#define STACK_SIZE (2048)
static K_THREAD_STACK_ARRAY_DEFINE(stacks, 4, STACK_SIZE);
static struct k_thread threads[4] = {0};

// #define LED0_NODE DT_ALIAS(led0)
// static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
//
// #define LED1_NODE DT_ALIAS(led1)
// static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);


extern SENSOR_MODULE_DATA_T data;

static void broadcast_data_task(void *, void *, void *) {
    while (1) {
        
        send_udp_broadcast((uint8_t *) &data, sizeof(SENSOR_MODULE_DATA_T), 11000);
   }
}

static void randomize_data(void *, void *, void *) {
    while (1) {
        for (int i = 0; i < 256; i++) {
            data.accel_x = i;
            data.accel_y = i;
            data.accel_z = i;
            data.pressure_bmp3 = i;
            data.pressure_ms5 = i;
        }
        
        for (int i = 256; i >= 0; i++) {
            data.accel_x = i;
            data.accel_y = i;
            data.accel_z = i;
            data.pressure_bmp3 = i;
            data.pressure_ms5 = i;
        }

    }
}

static void init(void) {
    // Queues
    k_queue_init(&net_tx_queue);

    if (!init_eth_iface()) {
        printk("Ethernet ready\n");
        if (!init_net_stack()) {
            printk("Network stack initialized\n");
            k_thread_create(&threads[0], &stacks[0][0], STACK_SIZE,
                            broadcast_data_task, NULL, NULL, NULL,
                            K_PRIO_PREEMPT(10), 0, K_NO_WAIT);

            k_thread_start(&threads[0]);

            printk("Ethernet thread started\n");
 
            
        }
    }

    k_thread_create(&threads[1], &stacks[1][0], STACK_SIZE,
                    randomize_data, NULL, NULL, NULL,
                    K_PRIO_PREEMPT(10), 0, K_NO_WAIT);
    k_thread_start(&threads[1]);

    k_thread_create(&threads[2], &stacks[2][0], STACK_SIZE,
                    update_lsm6dsl_data, NULL, NULL, NULL,
                     K_PRIO_PREEMPT(10), 0, K_NO_WAIT);

    k_thread_start(&threads[2]);

    k_thread_create(&threads[3], &stacks[3][0], STACK_SIZE,
                    update_tmp117_data, NULL, NULL, NULL,
                    K_PRIO_PREEMPT(10), 0, K_NO_WAIT);
    k_thread_start(&threads[3]);

}


int main() {
    init();


    return 0;
}




