/*
 * Copyright (c) 2024 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Utility functions for dealing with the Zephyr sensor API
 */

#ifndef L_SENSOR_UTILS_H
#define L_SENSOR_UTILS_H

#include <launch_core/types.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

typedef struct {
    int num_readings;
    enum sensor_channel *channels;
    struct sensor_value **values;
    float **float_values;
} l_sensor_readings_args_t;

/**
 * Command sensor updates and get sensor data from a device.
 * @param dev - Device to read from
 * @param args - Arguments for reading the sensor data
 * @return Zephyr status code
 */
int l_update_get_sensor_data(const struct device *const dev, l_sensor_readings_args_t *args);

/**
 * Command sensor updates for a list of devices.
 * @param devs - List of devices to update sensor samples
 * @param num_devs - Number of devices in the list
 * @return Status code (Only 0 currently)
 */
int l_update_sensors(const struct device *const *devs, int num_devs);

/**
 * Command sensor updates for a list of devices if they are ready.
 * @param devs - List of devices to update sensor samples
 * @param num_devs - Number of devices in the list
 * @param devs_ready - List of booleans indicating whether the device is ready
 * @return Status code (Only 0 currently)
 */
int l_update_sensors_safe(const struct device *const *devs, int num_devs, const bool *devs_ready);

/**
 * Get sensor data from a device.
 * @param dev - Device to read from
 * @param num_channels - Number of channels to read
 * @param channels - List of channels to read
 * @param values - List of sensor values to store the data in
 * @return Status code (Only 0 currently)
 */
int l_get_sensor_data(const struct device *const dev, int num_channels, enum sensor_channel const *channels,
                      struct sensor_value **values);


#endif // L_SENSOR_UTILS_H
