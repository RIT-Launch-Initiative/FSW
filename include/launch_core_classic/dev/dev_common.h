/*
 * Copyright (c) 2024 Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Shared utility functions for dealing with all Zephyr devices
 */

#ifndef L_DEVICE_UTILS_H
#define L_DEVICE_UTILS_H

#include <zephyr/device.h>

/**
 * Confirm that the device is present and ready to be used.
 * @param dev - Device to check
 * @return Zephyr status code (0 if device is ready, -ENODEV otherwise)
 */
int l_check_device(const struct device *const dev);

#endif // L_DEVICE_UTILS_H
