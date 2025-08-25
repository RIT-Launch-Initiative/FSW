#include "ina260.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#define DT_DRV_COMPAT ti_ina260

LOG_MODULE_REGISTER(INA260, CONFIG_SENSOR_LOG_LEVEL);

static int ina260_reg_read(const struct device *dev, uint8_t reg_addr, uint16_t *reg_data) {
    const struct ina260_config *cfg = dev->config;
    uint8_t rx_buf[2] = {0};

    int rc = i2c_write_read_dt(&cfg->bus, &reg_addr, sizeof(reg_addr), rx_buf, sizeof(rx_buf));

    if (rc == 0) {
        *reg_data = sys_get_be16(rx_buf);
    }
    return rc;
}

static int ina260_reg_write(const struct device *dev, uint8_t addr, uint16_t reg_data) {
    const struct ina260_config *cfg = dev->config;
    uint8_t tx_buf[3] = {addr, 0, 0};

    sys_put_be16(reg_data, &tx_buf[1]);

    return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

static int ina260_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    const struct ina260_config *cfg = dev->config;
    struct ina260_data *data = dev->data;
    uint16_t tmp;
    int rc;

    if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE && chan != SENSOR_CHAN_POWER &&
        chan != SENSOR_CHAN_CURRENT) {
        return -ENOTSUP;
    }

    if (cfg->mode < CONT_OFF) {
        LOG_ERR("Triggered Mode not supported\n");
        return -ENOTSUP;
    }

    if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_VOLTAGE) {

        rc = ina260_reg_read(dev, INA260_REG_V_BUS, &tmp);
        if (rc) {
            LOG_ERR("Error reading bus voltage.");
            return rc;
        }
        data->v_bus = tmp;
    }

    if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_POWER) {

        rc = ina260_reg_read(dev, INA260_REG_POWER, &tmp);
        if (rc) {
            LOG_ERR("Error reading power register.");
            return rc;
        }
        data->power = tmp;
    }

    if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_CURRENT) {

        rc = ina260_reg_read(dev, INA260_REG_CURRENT, &tmp);
        if (rc) {
            LOG_ERR("Error reading current register.");
            return rc;
        }
        data->current = tmp;
    }

    return 0;
}
static int ina260_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val) {
    struct ina260_data *data = dev->data;

    switch (chan) {
        case SENSOR_CHAN_VOLTAGE: // Want volts
            // Full-scale range = 40.96 V (decimal = 32767); LSB = 1.25 mV.
            return sensor_value_from_float(val, data->v_bus * INA260_VOLTS_PER_LSB);
        case SENSOR_CHAN_CURRENT: // Want Amps
            // The LSB size for the current register is set to 1.25 mA
            return sensor_value_from_float(val, data->current * INA260_AMPS_PER_LSB);
        case SENSOR_CHAN_POWER: // Want power
            // The Power Register LSB is fixed to 10 mW.
            return sensor_value_from_float(val, data->power * INA260_WATTS_PER_LSB);

        default:
            LOG_DBG("Channel not supported by device");
            return -ENOTSUP;
    }

    return 0;
}

static int ina260_init(const struct device *dev) {
    const struct ina260_config *cfg = dev->config;
    int rc;

    if (!device_is_ready(cfg->bus.bus)) {
        LOG_ERR("Device not ready.");
        return -ENODEV;
    }

    rc = ina260_reg_write(dev, INA260_REG_CONF, INA260_RST);
    if (rc) {
        LOG_ERR("Could not reset device.");
        return rc;
    }

    const uint16_t conf_reg = (INA260_CONF_REQUIRED_TOP_BITS << 12) | (cfg->average << 9) |
                              (cfg->voltage_conv_time << 6) | (cfg->current_conv_time << 3) | (cfg->mode);

    rc = ina260_reg_write(dev, INA260_REG_CONF, conf_reg);
    if (rc) {
        LOG_ERR("Could not set configuration data.");
        return rc;
    }
    return 0;
}

static DEVICE_API(sensor, ina260_api) = {
    .sample_fetch = ina260_sample_fetch,
    .channel_get = ina260_channel_get,
};

#define INA260_INIT(n)                                                                                                 \
    static struct ina260_data ina260_data_##n;                                                                         \
                                                                                                                       \
    static const struct ina260_config ina260_config_##n = {                                                            \
        .bus = I2C_DT_SPEC_INST_GET(n),                                                                                \
        .average = DT_STRING_TOKEN(DT_INST(n, DT_DRV_COMPAT), average),                                                \
        .voltage_conv_time = DT_STRING_TOKEN(DT_INST(n, DT_DRV_COMPAT), v_conv_time),                                  \
        .current_conv_time = DT_STRING_TOKEN(DT_INST(n, DT_DRV_COMPAT), i_conv_time),                                  \
        .mode = DT_STRING_TOKEN(DT_INST(n, DT_DRV_COMPAT), mode)};                                                     \
                                                                                                                       \
    SENSOR_DEVICE_DT_INST_DEFINE(n, ina260_init, NULL, &ina260_data_##n, &ina260_config_##n, POST_KERNEL,              \
                                 CONFIG_SENSOR_INIT_PRIORITY, &ina260_api);

DT_INST_FOREACH_STATUS_OKAY(INA260_INIT)
