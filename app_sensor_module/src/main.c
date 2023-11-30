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

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);


static void udp_broadcast_task(void *, void *, void *) {
    while (1) {
        send_udp_broadcast("Launch!", 7, 10000);
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
                            udp_broadcast_task, NULL, NULL, NULL,
                            K_PRIO_PREEMPT(10), 0, K_NO_WAIT);

            k_thread_start(&threads[0]);

            printk("Ethernet thread started\n");
 
            
        }
    }
}


int main() {
    init();


    return 0;
}




