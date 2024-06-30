#include "openrocket_imu.h"

#include "openrocket_sensors.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT openrocket_imu

LOG_MODULE_REGISTER(openrocket_imu, CONFIG_SENSOR_LOG_LEVEL);

static int or_imu_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    // read from openrocket
    const struct or_imu_config *cfg = dev->config;
    if (cfg->broken) {
        return -ENODEV;
    }
    if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_ACCEL_X && chan != SENSOR_CHAN_ACCEL_Y &&
        chan != SENSOR_CHAN_ACCEL_Z && chan != SENSOR_CHAN_ACCEL_XYZ && chan != SENSOR_CHAN_GYRO_X &&
        chan != SENSOR_CHAN_GYRO_Y && chan != SENSOR_CHAN_GYRO_Z && chan != SENSOR_CHAN_GYRO_XYZ) {
        return -ENOTSUP;
    }
    or_scalar_t time = 0.0;
    return 0;
}

static int or_imu_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val) {
    const struct or_imu_config *cfg = dev->config;

    if (cfg->broken) {
        return -ENODEV;
    }

    switch (chan) {
        case SENSOR_CHAN_ACCEL_X:
            sensor_value_from_double(val, 0.0);
            break;
        case SENSOR_CHAN_ACCEL_Y:
            sensor_value_from_double(val, 1.0);
            break;
        case SENSOR_CHAN_ACCEL_Z:
            sensor_value_from_double(val, 2.0);
            break;
        case SENSOR_CHAN_ACCEL_XYZ:
            sensor_value_from_double(&val[0], 0.0);
            sensor_value_from_double(&val[1], 1.0);
            sensor_value_from_double(&val[2], 2.0);
            break;
        case SENSOR_CHAN_GYRO_X:
            sensor_value_from_double(val, 0.0);
            break;
        case SENSOR_CHAN_GYRO_Y:
            sensor_value_from_double(val, 1.0);
            break;
        case SENSOR_CHAN_GYRO_Z:
            sensor_value_from_double(val, 2.0);
            break;
        case SENSOR_CHAN_GYRO_XYZ:
            sensor_value_from_double(&val[0], 0.0);
            sensor_value_from_double(&val[1], 1.0);
            sensor_value_from_double(&val[2], 2.0);
            break;
        default:
            LOG_DBG("Channel not supported by device");
            return -ENOTSUP;
    }
    return 0;
}

static int or_imu_init(const struct device *dev) {
    const struct or_imu_config *cfg = dev->config;
    if (cfg->broken) {
        return -ENODEV;
    }
    return 0;
}

static const struct sensor_driver_api or_imu_api = {
    .sample_fetch = or_imu_sample_fetch,
    .channel_get = or_imu_channel_get,
};

#define OR_IMU_INIT(n)                                                                                                 \
    static struct or_imu_data or_imu_data_##n;                                                                         \
                                                                                                                       \
    static const struct or_imu_config or_imu_config_##n = {                                                            \
        .broken = DT_INST_PROP(n, broken),                                                                             \
        .sampling_period_us = DT_INST_PROP(n, sampling_period_us),                                                     \
        .lag_time_ms = DT_INST_PROP(n, lag_time_ms),                                                                   \
    };                                                                                                                 \
                                                                                                                       \
    SENSOR_DEVICE_DT_INST_DEFINE(n, or_imu_init, NULL, &or_imu_data_##n, &or_imu_config_##n, POST_KERNEL,              \
                                 CONFIG_SENSOR_INIT_PRIORITY, &or_imu_api);

DT_INST_FOREACH_STATUS_OKAY(OR_IMU_INIT)
