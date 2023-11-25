/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "telem.h"
#include "net_utils.h"

#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

static int init(void) {
    int ret = -1;
    // Guarantee physical layer before initializing upper layers
    if (!init_eth_iface()) {
        ret = init_net_stack();
    } 
    
    init_telem_tasks();
    return 0;
}


int main(void) {
    if (init()) {
        return -1;
    }

    // Won't run if initializing the network stack failed
    while (true) {
        convert_and_send();

        k_sleep(K_MSEC(100));
    }
    return 0;
}

