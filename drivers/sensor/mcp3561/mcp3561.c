/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp3561

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mcp3561, CONFIG_SENSOR_LOG_LEVEL);

struct mcp3561_data {
	int state;
};

struct mcp3561_config {
	struct gpio_dt_spec input;
};

static int mcp3561_sample_fetch(const struct device *dev,
				      enum sensor_channel chan)
{
	const struct mcp3561_config *config = dev->config;
	struct mcp3561_data *data = dev->data;

	data->state = gpio_pin_get_dt(&config->input);

	return 0;
}

static int mcp3561_channel_get(const struct device *dev,
				     enum sensor_channel chan,
				     struct sensor_value *val)
{
	struct mcp3561_data *data = dev->data;

	if (chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	val->val1 = data->state;

	return 0;
}

static const struct sensor_driver_api mcp3561_api = {
	.sample_fetch = &mcp3561_sample_fetch,
	.channel_get = &mcp3561_channel_get,
};

static int mcp3561_init(const struct device *dev)
{
	const struct mcp3561_config *config = dev->config;

	int ret;

	if (!device_is_ready(config->input.port)) {
		LOG_ERR("Input GPIO not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->input, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure input GPIO (%d)", ret);
		return ret;
	}

	return 0;
}

#define MCP3561_INIT(i)						       \
	static struct mcp3561_data mcp3561_data_##i;	       \
									       \
	static const struct mcp3561_config mcp3561_config_##i = {  \
		.input = GPIO_DT_SPEC_INST_GET(i, input_gpios),		       \
	};								       \
									       \
	DEVICE_DT_INST_DEFINE(i, mcp3561_init, NULL,		       \
			      &mcp3561_data_##i,			       \
			      &mcp3561_config_##i, POST_KERNEL,	       \
			      CONFIG_SENSOR_INIT_PRIORITY, &mcp3561_api);

DT_INST_FOREACH_STATUS_OKAY(MCP3561_INIT)
