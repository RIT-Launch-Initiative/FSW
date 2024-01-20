/*
 * Copyright (c) 2024 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <launch_core/device_utils.h>
#include <zephyr/kernel.h>

int l_check_device(const struct device *const dev) {
    if (!device_is_ready(dev)) {
        printk("Device %s is not ready.\n", dev->name);
        return -ENODEV;
    }

    printk("Device %s is ready.\n", dev->name);
    return 0;
}
