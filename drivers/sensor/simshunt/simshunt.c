#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT sim_simshunt

LOG_MODULE_REGISTER(SIMSHUNT, CONFIG_SENSOR_LOG_LEVEL);

struct simshunt_config {
    float simulated_voltage;
    float simulated_current;
};

struct simshunt_data {};

static int simshunt_sample_fetch(const struct device *dev, enum sensor_channel chan) {

    if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE && chan != SENSOR_CHAN_POWER &&
        chan != SENSOR_CHAN_CURRENT) {
        return -ENOTSUP;
    }

    return 0;
}

static int simshunt_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val) {
    const struct simshunt_config *config = dev->config;

    switch (chan) {
        case SENSOR_CHAN_VOLTAGE: // Unit: Volts (not milliVolts)
            return sensor_value_from_float(val, config->simulated_voltage);
        case SENSOR_CHAN_CURRENT: // Unit: Amps (not milliAmps)
            return sensor_value_from_float(val, config->simulated_current);
        case SENSOR_CHAN_POWER:
            return sensor_value_from_float(val, config->simulated_voltage * config->simulated_current);

        default:
            LOG_DBG("Channel not supported by device");
            return -ENOTSUP;
    }

    return 0;
}

static int simshunt_init(const struct device *dev) { return 0; }

static const struct sensor_driver_api simshunt_api = {
    .sample_fetch = simshunt_sample_fetch,
    .channel_get = simshunt_channel_get,
};

#define SIMSHUNT_INIT(n)                                                                                               \
    static struct simshunt_data simshunt_data_##n;                                                                     \
                                                                                                                       \
    static const struct simshunt_config simshunt_config_##n = {                                                        \
        .simulated_voltage = DT_INST_PROP(n, simulated_voltage) / 1000.0,                                              \
        .simulated_current = DT_INST_PROP(n, simulated_current) / 1000.0};                                             \
                                                                                                                       \
    SENSOR_DEVICE_DT_INST_DEFINE(n, simshunt_init, NULL, &simshunt_data_##n, &simshunt_config_##n, POST_KERNEL,        \
                                 CONFIG_SENSOR_INIT_PRIORITY, &simshunt_api);

DT_INST_FOREACH_STATUS_OKAY(SIMSHUNT_INIT)
