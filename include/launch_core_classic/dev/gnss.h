/*
 * Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef L_GNSS_H

#include <zephyr/drivers/gnss.h>

#define L_GNSS_LATITUDE_DIVISION_FACTOR 1000000000.0F
#define L_GNSS_LONGITUDE_DIVISION_FACTOR 1000000000.0F
#define L_GNSS_ALTITUDE_DIVISION_FACTOR 1000.0F

/**
 * Debug callback for logging GNSS fix status
 * @param dev - GNSS device that triggered the callback
 * @param data - GNSS data from the device
 */
void l_gnss_fix_debug_cb(const struct device *dev, const struct gnss_data *data);

/**
 * Debug callback for logging GNSS data
 * @param dev - GNSS device that triggered the callback
 * @param data - GNSS data from the device
 */
void l_gnss_data_debug_cb(const struct device *dev, const struct gnss_data *data);

/**
 * Debug callback for logging GNSS satellite count
 * @param dev - GNSS device that triggered the callback
 * @param satellites - Array of satellite data
 * @param size - Number of satellite data in the array
 */
void l_gnss_debug_sat_count_cb(const struct device *dev, const struct gnss_satellite *satellites, uint16_t size);


