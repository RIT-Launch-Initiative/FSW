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

/**
 * Retrieve data from an accelerometer device and assign converted values to a struct
 * @param dev - Device to read from
 * @param p_accel_data - Pointer to an accelerometer data struct
 */
int l_get_accelerometer_data_float(const struct device *const dev, l_accelerometer_data_t *p_accel_data);

/**
 * Retrieve data from an barometer device and assign converted values to a struct
 * @param dev - Device to read from
 * @param p_accel_data - Pointer to an accelerometer data struct
 */
int l_get_barometer_data_float(const struct device *const dev, l_barometer_data_t *p_baro_data);

/**
 * Retrieve data from an temperature sensor device and assign converted values to a struct
 * @param dev - Device to read from
 * @param p_accel_data - Pointer to an accelerometer data struct
 */
int l_get_temp_sensor_data_float(const struct device *const dev, l_temperature_data_t *p_temp_data);

/**
 * Retrieve data from an magnetometer device and assign converted values to a struct
 * @param dev - Device to read from
 * @param p_accel_data - Pointer to an accelerometer data struct
 */
int l_get_magnetometer_data_float(const struct device *const dev, l_magnetometer_data_t *p_magn_data);

/**
 * Retrieve data from an gyroscope device and assign converted values to a struct
 * @param dev - Device to read from
 * @param p_accel_data - Pointer to an accelerometer data struct
 */
int l_get_gyroscope_data_float(const struct device *const dev, l_gyroscope_data_t *p_gyro_data);




#endif // L_SENSOR_UTILS_H
