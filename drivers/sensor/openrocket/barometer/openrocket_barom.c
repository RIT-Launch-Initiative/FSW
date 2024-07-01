#include "openrocket_barom.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT openrocket_barometer

LOG_MODULE_REGISTER(openrocket_barom, CONFIG_SENSOR_LOG_LEVEL);

extern const unsigned int or_packets_size;
extern const struct or_data_t *or_packets;

static int or_barom_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    const struct or_barom_config *cfg = dev->config;
    struct or_barom_data *data = dev->data;
    if (cfg->broken) {
        return -ENODEV;
    }
    if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP && chan != SENSOR_CHAN_PRESS) {
        return -ENOTSUP;
    }
    return 0;
}

static int or_barom_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val) {
    const struct or_barom_config *cfg = dev->config;
    struct or_barom_data *data = dev->data;

    if (cfg->broken) {
        return -ENODEV;
    }
    return 0;
}

static int or_barom_init(const struct device *dev) {
    const struct or_barom_config *cfg = dev->config;
    if (cfg->broken) {
        return -ENODEV;
    }
    struct or_barom_data *data = dev->data;
    data->last_lower_index = 0;
    return 0;
}

static const struct sensor_driver_api or_barom_api = {
    .sample_fetch = or_barom_sample_fetch,
    .channel_get = or_barom_channel_get,
};

#define OR_BAROM_INIT(n)                                                                                               \
    static struct or_barom_data or_barom_data_##n;                                                                     \
                                                                                                                       \
    static const struct or_barom_config or_barom_config_##n = {                                                        \
        .broken = DT_INST_PROP(n, broken),                                                                             \
        .sampling_period_us = DT_INST_PROP(n, sampling_period_us),                                                     \
        .lag_time_ms = DT_INST_PROP(n, lag_time_us),                                                                   \
    };                                                                                                                 \
                                                                                                                       \
    SENSOR_DEVICE_DT_INST_DEFINE(n, or_barom_init, NULL, &or_barom_data_##n, &or_barom_config_##n, POST_KERNEL,        \
                                 CONFIG_SENSOR_INIT_PRIORITY, &or_barom_api);

DT_INST_FOREACH_STATUS_OKAY(OR_BAROM_INIT)
