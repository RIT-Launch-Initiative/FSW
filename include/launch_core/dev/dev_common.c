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

    LOG_INF("Device %s is ready.\n", dev->name);
    return 0;
}

/**********   ADC   **********/
int l_init_adc_channel(const struct adc_dt_spec *const channel, struct adc_sequence *const sequence) {
    int ret = adc_is_ready_dt(channel);

    if (ret >= 0) {
        ret = adc_channel_setup_dt(channel);

        if (ret < 0) {
            LOG_ERR("ADC channel %d failed to be setup. Errno %d.", channel->channel_id, ret);
            return ret;
        }

        ret = adc_sequence_init_dt(channel, sequence);
        if (ret < 0) {
            LOG_ERR("ADC channel %d failed to setup sequence. Errno %d", channel->channel_id, ret);
        }

        LOG_INF("ADC channel %d is ready.", channel->channel_id);
    } else {
        LOG_ERR("ADC channel %d is not ready. Errno %d.", channel->channel_id, ret);
    }

    return ret;
}

int l_init_adc_channels(const struct adc_dt_spec *const channels, struct adc_sequence *const sequence,
                        const int num_channels) {
    for (int i = 0; i < num_channels; i++) {
        l_init_adc_channel(&channels[i], &sequence[i]);
    }

    return 0;
}

int l_read_adc_mv(const struct adc_dt_spec *const channel, struct adc_sequence *const sequence, int32_t *val) {
    int ret = adc_read_dt(channel, sequence);
    int32_t val_mv = 0;

    if (ret == 0) {
        if (channel->channel_cfg.differential) { // Differential channels are 16 bits
            val_mv = (int32_t) *((int16_t*) sequence->buffer);
        } else {
            val_mv = *((int32_t *) (sequence->buffer));
        }

        ret = adc_raw_to_millivolts_dt(channel, &val_mv);
        if (ret < 0) {
            LOG_ERR("Could not convert ADC value from %d to millivolts. Errno %d.", channel->channel_id, ret);
        }

        *val = val_mv;
    } else {
        LOG_ERR("Could not read ADC value from %d. Errno %d.", channel->channel_id, ret);
    }

    return ret;
}

// TODO: Implement
int l_async_read_adc_mv(const struct adc_dt_spec *const channel, struct adc_sequence *const sequence, int32_t *val) {
    return -1;
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
            LOG_ERR("Sensor %s is not ready.\n", devs[i]->name);
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
