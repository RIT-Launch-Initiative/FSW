/*
 * Copyright (c) 2024 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef DEVICE_UTILS_H
#define DEVICE_UTILS_H

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

typedef struct {
    int num_readings;
    enum sensor_channel *channels;
    struct sensor_value **values;
    float **float_values;
} l_sensor_readings_args_t;

/**
 * Confirm that the device is present and ready to be used.
 * @param dev - Device to check
 * @return Zephyr status code (0 if device is ready, -ENODEV otherwise)
 */
int l_check_device(const struct device *const dev);

/**
 * Command sensor updates and get sensor data from a device.
 * @param dev - Device to read from
 * @param args - Arguments for reading the sensor data
 * @param convert_to_float - Whether to convert the sensor data to floats and store them
 * @return Zephyr status code
 */
int l_update_get_sensor_data(const struct device *const dev, l_sensor_readings_args_t *args, bool convert_to_float);

int l_update_sensors(const struct device *const *devs, int num_devs);

int l_get_sensor_data(const struct device *const dev, int num_channels, enum sensor_channel *channels, struct sensor_value **values);

int l_get_sensor_data_float(const struct device *const dev, int num_channels, enum sensor_channel *channels, float **values);

#endif //DEVICE_UTILS_H
