#include "openrocket_magnetometer.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT openrocket_magnetometer

LOG_MODULE_REGISTER(openrocket_magnetometer, CONFIG_SENSOR_LOG_LEVEL);

extern const unsigned int or_packets_size;
extern const struct or_data_t *or_packets;

// Map openrocket values to sensor axes
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEGREES_TO_RADIANS (M_PI / 180.0)

static int or_magn_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    const struct or_magnetometer_config *cfg = dev->config;
    k_usleep(cfg->sensor_cfg.measurement_us);

    struct or_magnetometer_data *data = dev->data;
    if (cfg->sensor_cfg.broken) {
        return -ENODEV;
    }
    if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_MAGN_X && chan != SENSOR_CHAN_MAGN_Y &&
        chan != SENSOR_CHAN_MAGN_Z && chan != SENSOR_CHAN_MAGN_XYZ) {
        return -ENOTSUP;
    }
    or_scalar_t time = or_get_time(&cfg->sensor_cfg);
    unsigned int lo, hi = 0;
    or_scalar_t mix = 0;
    or_find_bounding_packets(data->last_lower_index, time, &lo, &hi, &mix);

    struct or_data_t or_data;
    if (hi == 0) {
        // Before sim
        or_get_presim(&or_data);
    } else if (lo == or_packets_size) {
        // After sim
        or_get_postsim(&or_data);
    } else {
        // In the middle
        const struct or_data_t *lo_data = &or_packets[lo];
        const struct or_data_t *hi_data = &or_packets[hi];

        or_data.magn_x = or_lerp(lo_data->magn_x, hi_data->magn_x, mix);
        or_data.magn_y = or_lerp(lo_data->magn_y, hi_data->magn_y, mix);
        or_data.magn_z = or_lerp(lo_data->magn_z, hi_data->magn_z, mix);
    }
    data->magn_x = or_data.magn_x + or_random(&data->rand_state, cfg->noise);
    data->magn_y = or_data.magn_y + or_random(&data->rand_state, cfg->noise);
    data->magn_z = or_data.magn_z + or_random(&data->rand_state, cfg->noise);

    return 0;
}

static int or_magn_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val) {
    const struct or_magnetometer_config *cfg = dev->config;
    struct or_magnetometer_data *data = dev->data;

    if (cfg->sensor_cfg.broken) {
        return -ENODEV;
    }

    switch (chan) {
        case SENSOR_CHAN_MAGN_X:
            sensor_value_from_or_scalar(val, data->magn_x);
            break;
        case SENSOR_CHAN_MAGN_Y:
            sensor_value_from_or_scalar(val, data->magn_y);
            break;
        case SENSOR_CHAN_MAGN_Z:
            sensor_value_from_or_scalar(val, data->magn_z);
            break;
        case SENSOR_CHAN_MAGN_XYZ:
            sensor_value_from_or_scalar(&val[0], data->magn_x);
            sensor_value_from_or_scalar(&val[1], data->magn_y);
            sensor_value_from_or_scalar(&val[2], data->magn_z);
            break;
        default:
            LOG_DBG("Channel not supported by device");
            return -ENOTSUP;
    }
    return 0;
}

static int or_magn_init(const struct device *dev) {
    const struct or_magnetometer_config *cfg = dev->config;
    if (cfg->sensor_cfg.broken) {
        LOG_WRN("Magnetometer device %s is failed to init", dev->name);
        return -ENODEV;
    }
    struct or_magnetometer_data *data = dev->data;
    data->last_lower_index = 0;
    return 0;
}

static const struct sensor_driver_api or_magn_api = {
    .sample_fetch = or_magn_sample_fetch,
    .channel_get = or_magn_channel_get,
};

#define OR_MAGN_INIT(n)                                                                                                \
    static struct or_magnetometer_data or_magn_data_##n = {.rand_state = CONFIG_OPENROCKET_NOISE_SEED};                \
                                                                                                                       \
    static const struct or_magnetometer_config or_magn_config_##n = {                                                  \
        .sensor_cfg =                                                                                                  \
            {                                                                                                          \
                .broken = DT_INST_PROP(n, broken),                                                                     \
                .sampling_period_us = DT_INST_PROP(n, sampling_period_us),                                             \
                .lag_time_ms = DT_INST_PROP(n, lag_time_us),                                                           \
                .measurement_us = DT_INST_PROP(n, measurement_us),                                                     \
            },                                                                                                         \
        .noise = SCALE_OPENROCKET_NOISE(DT_INST_PROP(n, noise)),                                                       \
    };                                                                                                                 \
                                                                                                                       \
    SENSOR_DEVICE_DT_INST_DEFINE(n, or_magn_init, NULL, &or_magn_data_##n, &or_magn_config_##n, POST_KERNEL,           \
                                 CONFIG_SENSOR_INIT_PRIORITY, &or_magn_api);

DT_INST_FOREACH_STATUS_OKAY(OR_MAGN_INIT)
