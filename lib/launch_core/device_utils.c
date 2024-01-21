/*
 * Copyright (c) 2024 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <launch_core/device_utils.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(device_utils);

int l_check_device(const struct device *const dev) {
    if (!device_is_ready(dev)) {
        printk("Device %s is not ready.\n", dev->name);
        return -ENODEV;
    }

    printk("Device %s is ready.\n", dev->name);
    return 0;
}

int l_read_sensor_data(const struct device *const dev, SENSOR_READINGS_ARGS_T *args, bool convert_to_float) {
    int num_readings = args->num_readings;
    enum sensor_channel *channels = args->channels;

    struct sensor_value **values = args->values;
    float **float_values = args->float_values;

    int ret = sensor_sample_fetch(dev);
    if (ret < 0) {
        LOG_ERR("Failed to update %s readings. Errno %d\n", dev->name, ret);
        return ret;
    }

    for (int i = 0; i < num_readings; i++) {
        ret = sensor_channel_get(dev, channels[i], values[i]);
        if (ret < 0) {
            LOG_ERR("Failed to get sensor channel stored at index %d. Errno %d\n", i, ret);
        }

        if (convert_to_float) {
            *float_values[i] = sensor_value_to_float(values[i]);
        }
    }

    return 0;
}