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

/**
 * Command sensor updates for a list of devices.
 * @param devs - List of devices to update sensor samples
 * @param num_devs - Number of devices in the list
 * @return Status code (Only 0 currently)
 */
int l_update_sensors(const struct device *const *devs, int num_devs);

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

/**
 * Get sensor data from a device and convert it to floats
 * @param dev - Device to read from
 * @param num_channels - Number of channels to read
 * @param channels - List of channels to read
 * @param values - List of floats to store the data in
 * @return
 */
int l_get_sensor_data_float(const struct device *const dev, int num_channels, enum sensor_channel const *channels,
                            float **values);

#endif //DEVICE_UTILS_H
