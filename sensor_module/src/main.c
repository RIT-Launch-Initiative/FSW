/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <app_version.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

#define SLEEP_TIME_MS   1000

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);
K_QUEUE_DEFINE(net_tx_queue);

static void network_init() {

}


static void init(void) {
    // Queues
    k_queue_init(&net_tx_queue);

    // Devices


    network_init();
}

int main() {
    init();




    return 0;
}



