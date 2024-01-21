/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "telem.h"

#include <launch_core/backplane_defs.h>
#include <launch_core/device_utils.h>
#include <launch_core/net_utils.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

K_QUEUE_DEFINE(net_tx_queue);

static const struct device *const wiznet = DEVICE_DT_GET_ONE(wiznet_w5500);

static int init(void) {
    char ip[MAX_IP_ADDRESS_STR_LEN];
    int ret = -1;

    k_queue_init(&net_tx_queue);
    if (0 > l_create_ip_str_default_net_id(ip, POWER_MODULE_ID, 1)) {
        LOG_ERR("Failed to create IP address string: %d", ret);
        return -1;
    }

    if (!l_check_device(wiznet)) {
        ret = l_init_udp_net_stack(ip);
        if (ret != 0) {
            LOG_ERR("Failed to initialize network stack");
            return ret;
        }
    } else {
        LOG_ERR("Failed to get network device");
        return ret;
    }

    init_telem_tasks();
    return 0;
}


int main(void) {
    if (init()) {
        return -1;
    }

    while (true) {
        convert_and_send();

        k_sleep(K_MSEC(100));
    }
    return 0;
}

