/* adxl375_i2c.c - I2C routines for ADXL375 driver
 */

/*
 * Copyright (c) 2022 Analog Devices
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adxl375

#include <string.h>
#include <zephyr/logging/log.h>

#include "adxl375.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

LOG_MODULE_DECLARE(ADXL375, CONFIG_SENSOR_LOG_LEVEL);

static int adxl375_bus_access(const struct device *dev, uint8_t reg, void *data, size_t length)
{
	const struct adxl375_dev_config *config = dev->config;

	if (reg & ADXL375_READ) {
		return i2c_burst_read_dt(&config->i2c, ADXL375_TO_I2C_REG(reg), (uint8_t *)data,
					 length);
	} else {
		if (length != 1) {
			return -EINVAL;
		}

		return i2c_reg_write_byte_dt(&config->i2c, ADXL375_TO_I2C_REG(reg),
					     *(uint8_t *)data);
	}
}

static int adxl375_i2c_reg_read(const struct device *dev, uint8_t reg_addr, uint8_t *reg_data)
{
	return adxl375_bus_access(dev, ADXL375_REG_READ(reg_addr), reg_data, 1);
}

static int adxl375_i2c_reg_read_multiple(const struct device *dev, uint8_t reg_addr,
					 uint8_t *reg_data, uint16_t count)
{
	return adxl375_bus_access(dev, ADXL375_REG_READ(reg_addr), reg_data, count);
}

static int adxl375_i2c_reg_write(const struct device *dev, uint8_t reg_addr, uint8_t reg_data)
{
	return adxl375_bus_access(dev, ADXL375_REG_WRITE(reg_addr), &reg_data, 1);
}

int adxl375_i2c_reg_write_mask(const struct device *dev, uint8_t reg_addr, uint32_t mask,
			       uint8_t data)
{
	int ret;
	uint8_t tmp;

	ret = adxl375_i2c_reg_read(dev, reg_addr, &tmp);
	if (ret) {
		return ret;
	}

	tmp &= ~mask;
	tmp |= data;

	return adxl375_i2c_reg_write(dev, reg_addr, tmp);
}

static const struct adxl375_transfer_function adxl375_i2c_transfer_fn = {
	.read_reg_multiple = adxl375_i2c_reg_read_multiple,
	.write_reg = adxl375_i2c_reg_write,
	.read_reg = adxl375_i2c_reg_read,
	.write_reg_mask = adxl375_i2c_reg_write_mask,
};

int adxl375_i2c_init(const struct device *dev)
{
	struct adxl375_data *data = dev->data;
	const struct adxl375_dev_config *config = dev->config;

	data->hw_tf = &adxl375_i2c_transfer_fn;

	if (!device_is_ready(config->i2c.bus)) {
		return -ENODEV;
	}

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
