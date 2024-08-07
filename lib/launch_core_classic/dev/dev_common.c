/*
 * Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <launch_core_classic/dev/dev_common.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(launch_dev_utils);

int l_check_device(const struct device *const dev) {
    if (!device_is_ready(dev)) {
        LOG_ERR("Device %s is not ready.\n", dev->name);
        return -ENODEV;
    }

    LOG_INF("Device %s is ready.\n", dev->name);
    return 0;
}


