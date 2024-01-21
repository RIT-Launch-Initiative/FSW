/*
 * Copyright (c) 2024 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef DEVICE_UTILS_H
#define DEVICE_UTILS_H

#include <zephyr/device.h>

/**
 * Confirm that the device is present and ready to be used.
 * @param dev - Device to check
 * @return Zephyr status code (0 if device is ready, -ENODEV otherwise)
 */
int l_check_device(const struct device *const dev);

#endif //DEVICE_UTILS_H
