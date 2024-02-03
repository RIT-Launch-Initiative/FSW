/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file ubxlib_utils.h
 * @authors Nate Aquino naquino14@outlook.com
 * @brief Ublox Library Utilities for Radio Module
 */

#ifndef UBXLIB_UTILS_H_
#define UBXLIB_UTILS_H_

#include <u_cfg_app_platform_specific.h>
#include <ubxlib.h>

#ifndef ZEPHYR_INCLUDE_KERNEL_H_
#include <zephyr/kernel.h>
#endif  // !ZEPHYR_INCLUDE_KERNEL_H_

#ifndef ZEPHYR_INCLUDE_DRIVERS_I2C_H_
#include <zephyr/drivers/i2c.h>
#endif  // !ZEPHYR_INCLUDE_DRIVERS_I2C_H_

/**
 * @brief GNSS device struct
 *
 */
typedef struct gnss_dev {
    uGnssTransportHandle_t transportHandle;
    uDeviceHandle_t gnssHandle;
} gnss_dev_t;

/**
 * @brief Initialize the MAX-M10S GNSS module
 * @param dev GNSS device struct
 *
 * @return int 0 if successful, negative otherwise
 */
int start_maxm10s(gnss_dev_t* dev);

/**
 * @brief Stop the MAX-M10S GNSS module
 * @param dev GNSS device struct
 *
 * @returns int 0 if successful, negative otherwise
 */
int stop_maxm10s(gnss_dev_t* dev);

#endif  // !UBXLIB_UTILS_H_