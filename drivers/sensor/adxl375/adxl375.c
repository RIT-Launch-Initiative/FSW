/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <asm-generic/errno-base.h>
#define DT_DRV_COMPAT adi_adxl375

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>

#include "adxl375.h"

LOG_MODULE_REGISTER(ADXL375, CONFIG_SENSOR_LOG_LEVEL);

static int adxl375_check_id(const struct device *dev) {
	struct adxl375_data *data = dev->data;
	uint8_t device_id = 0;

	int ret = data->hw_tf->read_reg(dev, ADXL375_DEVID, &device_id);

	if (ret != 0) {
		return ret;
	}

	if (ADXL375_DEVID_VAL != device_id) {
		return -ENODEV;
	}

	return 0;
}

static int adxl375_set_odr_and_lp(const struct device *dev, const uint32_t data_rate, const bool low_power)
{
	if (low_power) {
		data_rate |= 0x8;
	}

	return dev->data->hw_tf->write_reg(dev, ADXL375_REG_BW_RATE, data_rate);
}

static int adxl375_set_op_mode(const struct device *dev, enum adxl375_op_mode op_mode)
{
}

static int adxl375_wakeup()
{

}

static int adxl375_init()
{

	int ret;
	const struct adxl375_dev_config *cfg = dev->config;

	ret = cfg->bus_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialzie sensor bus.");
		return ret;
	}

	int ret = adxl375_check_id();
	if (!ret) {
		return ret;
	}


	ret = adxl375_set_odr();

	ret = adxl375_set_op_mode();

	ret = adxl375_wakeup();

	return -1;

}

static int adxl375_sample_fetch(const struct device *dev, enum sensor_channel chan)
{

	return -1;
}

static int adxl375_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
	return -1;
}

static const struct sensor_driver_api adxl375_driver_api =
{
	.sample_fetch = adxl375_sample_fetch,
	.channel_get = adxl375_channel_get
};

static struct adxl375_data adxl375_datal

static const struct sensor_driver_api adxl375_api_funcs = {
	.channel_get = adxl375_channel_get,
	.sample_fetch = adxl375_sample_fetch
}

DEVICE_AND_API_INIT(adxl375, CONFIG_ADXL375_DEV_NAME, adxl375_init, &adxl375_data,
		    &adxl375_data, &adxl375_config, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &adxl375_api_funcs);
