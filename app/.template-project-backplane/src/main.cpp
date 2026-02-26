/*
 * Copyright (c) 2025 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

int main() {
    LOG_INF("Application started");
    
    while (true) {
        k_msleep(1000);
    }

    return 0;
}
