/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <app_version.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

int main(void) {
    int ret;
    const struct device *sensor;

    printk("Zephyr Example Application %s\n", APP_VERSION_STRING);

    while (1) {
        printk("Launch!\n");
    }

    return 0;
}

