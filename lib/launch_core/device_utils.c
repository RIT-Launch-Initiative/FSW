/*
 * Copyright (c) 2024 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <launch_core/device_utils.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(device_utils);

/********** GENERAL **********/

int l_check_device(const struct device *const dev) {
    if (!device_is_ready(dev)) {
        LOG_ERR("Device %s is not ready.\n", dev->name);
        return -ENODEV;
    }

    LOG_ERR("Device %s is ready.\n", dev->name);
    return 0;
}

/**********   ADC   **********/
int l_init_adc_channels(const struct adc_dt_spec *const channels, const int num_channels) {
    for (int i = 0; i < num_channels; i++) {
        const struct adc_dt_spec *current_channel = &channels[i];
        int ret = adc_is_ready_dt(current_channel);

        if (ret == 0) {
            ret = adc_channel_setup_dt(current_channel);

            if (ret < 0) {
                LOG_ERR("ADC channel %d failed to be setup. Errno %d.", current_channel->channel_id, ret);
            }
        } else {
            LOG_ERR("ADC channel %d is not ready. Errno %d.", current_channel->channel_id, ret);
        }
    }

    return 0;
}

/********** SENSORS **********/
int l_update_get_sensor_data(const struct device *const dev, l_sensor_readings_args_t *args, bool convert_to_float) {
    l_update_sensors(&dev, 1);

    if (convert_to_float) {
        l_get_sensor_data_float(dev, args->num_readings, args->channels, args->float_values);
    } else {
        l_get_sensor_data(dev, args->num_readings, args->channels, args->values);
    }

    return 0;
}

int l_update_sensors(const struct device *const *devs, int num_devs) {
    for (int i = 0; i < num_devs; i++) {
        int ret = sensor_sample_fetch(devs[i]);
        if (ret != 0) {
            LOG_ERR("Failed to fetch sensor data from %s. Errno %d\n", devs[i]->name, ret);
        }
    }

    return 0;
}

int l_update_sensors_safe(const struct device *const *devs, int num_devs, const bool *devs_ready) {
    for (int i = 0; i < num_devs; i++) {
        if (unlikely(!devs_ready[i])) { // Skip if channel is not ready
            continue;
        }

        int ret = sensor_sample_fetch(devs[i]);
        if (ret != 0) {
            LOG_ERR("Failed to fetch sensor data from %s. Errno %d\n", devs[i]->name, ret);
        }
    }

    return 0;
}


int l_get_sensor_data(const struct device *const dev, int num_channels, enum sensor_channel const *channels,
                      struct sensor_value **values) {
    for (int i = 0; i < num_channels; i++) {
        int ret = sensor_channel_get(dev, channels[i], values[i]);
        if (ret != 0) {
            LOG_ERR("Failed to get sensor data from %s. Errno %d\n", dev->name, ret);
        }
    }

    return 0;
}

int l_get_sensor_data_float(const struct device *const dev, int num_channels, enum sensor_channel const *channels,
                            float **values) {
    // TODO: Either get rid of this function or fix it. sensor_values will have null pointers which is causing crashes
//    struct sensor_value *sensor_values[num_channels];

//    l_get_sensor_data(dev, num_channels, channels, sensor_values);

//    for (int i = 0; i < num_channels; i++) {
//        *values[i] = sensor_value_to_float(sensor_values[i]);
//    }

    return -1;
}
