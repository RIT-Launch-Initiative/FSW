/*
* Copyright (c) 2026 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <n_autocoder_network_defs.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

int main() {
    int i = 0;
    while (true) {
        printk("hello %d\n", i);
        i++;
        k_msleep(1000);
    }
    return 0;
}
