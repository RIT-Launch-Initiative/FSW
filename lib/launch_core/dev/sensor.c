#include <launch_core/dev/sensor.h>

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(launch_sensor_utils);

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

int l_get_accelerometer_data_float(const struct device *const dev, l_accelerometer_data_t *p_accel_data) {
    int ret = 0;
    struct sensor_value x_sensor_val = {0};
    struct sensor_value y_sensor_val = {0};
    struct sensor_value z_sensor_val = {0};

    if (likely(sensor_channel_get(dev, SENSOR_CHAN_ACCEL_X, &x_sensor_val) == 0)) {
        p_accel_data->accel_x = sensor_value_to_float(&x_sensor_val);
    } else {
        p_accel_data->accel_x = FLOAT_ERROR_VALUE;
        ret |= 0b1;
    }

    if (likely(sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Y, &y_sensor_val) == 0)) {
        p_accel_data->accel_y = sensor_value_to_float(&y_sensor_val);
    } else {
        p_accel_data->accel_y = FLOAT_ERROR_VALUE;
        ret |= 0b10;
    }

    if (likely(sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Z, &z_sensor_val) == 0)) {
        p_accel_data->accel_z = sensor_value_to_float(&z_sensor_val);
    } else {
        p_accel_data->accel_z = FLOAT_ERROR_VALUE;
        ret |= 0b100;
    }

    return -ret;
}

int l_get_barometer_data_float(const struct device *const dev, l_barometer_data_t *p_baro_data) {
    int ret = 0;
    struct sensor_value sensor_val = {0};

    if (likely(sensor_channel_get(dev, SENSOR_CHAN_PRESS, &sensor_val) == 0)) {
        p_baro_data->pressure = sensor_value_to_float(&sensor_val);
    } else {
        p_baro_data->pressure = FLOAT_ERROR_VALUE;
        ret |= 0b1;
    }

    if (likely(sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &sensor_val) == 0)) {
        p_baro_data->temperature = sensor_value_to_float(&sensor_val);
    } else {
        p_baro_data->pressure = FLOAT_ERROR_VALUE;
        ret |= 0b10;
    }

    return -ret;
}

int l_get_temp_sensor_data_float(const struct device *const dev, l_temperature_data_t *p_temp_data) {
    int ret = 0;
    struct sensor_value sensor_val = {0};

    if (likely(sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &sensor_val) == 0)) {
        *p_temp_data = sensor_value_to_float(&sensor_val);
    } else {
        *p_temp_data = FLOAT_ERROR_VALUE;
        ret |= 0b1;
    }

    return -ret;
}

int l_get_magnetometer_data_float(const struct device *const dev, l_magnetometer_data_t *p_magn_data) {
    int ret = 0;
    struct sensor_value sensor_val = {0};

    if (likely(sensor_channel_get(dev, SENSOR_CHAN_MAGN_X, &sensor_val) == 0)) {
        p_magn_data->mag_x = sensor_value_to_float(&sensor_val);
    } else {
        p_magn_data->mag_x = FLOAT_ERROR_VALUE;
        ret |= 0b1;
    }

    if (likely(sensor_channel_get(dev, SENSOR_CHAN_MAGN_Y, &sensor_val) == 0)) {
        p_magn_data->mag_y = sensor_value_to_float(&sensor_val);
    } else {
        p_magn_data->mag_y = FLOAT_ERROR_VALUE;
        ret |= 0b10;
    }

    if (likely(sensor_channel_get(dev, SENSOR_CHAN_MAGN_Z, &sensor_val) == 0)) {
        p_magn_data->mag_z = sensor_value_to_float(&sensor_val);
    } else {
        p_magn_data->mag_z = FLOAT_ERROR_VALUE;
        ret |= 0b100;
    }

    return -ret;
}

int l_get_gyroscope_data_float(const struct device *const dev, l_gyroscope_data_t *p_gyro_data) {
    int ret = 0;
    struct sensor_value sensor_val = {0};

    if (likely(sensor_channel_get(dev, SENSOR_CHAN_GYRO_X, &sensor_val) == 0)) {
        p_gyro_data->gyro_x = sensor_value_to_float(&sensor_val);
    } else {
        p_gyro_data->gyro_x = FLOAT_ERROR_VALUE;
        ret |= 0b1;
    }

    if (likely(sensor_channel_get(dev, SENSOR_CHAN_GYRO_Y, &sensor_val) == 0)) {
        p_gyro_data->gyro_y = sensor_value_to_float(&sensor_val);
    } else {
        p_gyro_data->gyro_y = FLOAT_ERROR_VALUE;
        ret |= 0b10;
    }

    if (likely(sensor_channel_get(dev, SENSOR_CHAN_GYRO_Z, &sensor_val) == 0)) {
        p_gyro_data->gyro_z = sensor_value_to_float(&sensor_val);
    } else {
        p_gyro_data->gyro_z = FLOAT_ERROR_VALUE;
        ret |= 0b100;
    }

    return -ret;
}
