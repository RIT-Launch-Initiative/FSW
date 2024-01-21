/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp356x

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mcp356x, CONFIG_SENSOR_LOG_LEVEL);

struct mcp356x_data {
  int state;
};

struct mcp356x_config {
  struct spi_dt_spec bus;
};

static int mcp356x_sample_fetch(const struct device *dev,
                                enum sensor_channel chan) {
  const struct mcp356x_config *config = dev->config;
  struct mcp356x_data *data = dev->data;

  return 0;
}

static int mcp356x_channel_get(const struct device *dev,
                               enum sensor_channel chan,
                               struct sensor_value *val) {
  struct mcp356x_data *data = dev->data;

  if (chan != SENSOR_CHAN_VOLTAGE) {
    return -ENOTSUP;
  }

  val->val1 = data->state;

  return 0;
}

static const struct sensor_driver_api mcp356x_api = {
    .sample_fetch = &mcp356x_sample_fetch,
    .channel_get = &mcp356x_channel_get,
};

static int mcp356x_init(const struct device *dev) {

  const struct mcp356x_config *config = dev->config;

  if (!device_is_ready(config->bus.bus)) {
    LOG_ERR("SPI bus '%s'not ready", config->bus.bus->name);
    return -ENODEV;
  }

  return 0;
}

#define INST_DT_MCP356x(inst) DT_INST(inst, DT_DRV_COMPAT)

#define MCP356x_INIT(i)                                                        \
  static struct mcp356x_data mcp356x_data_##i;                                 \
                                                                               \
  static const struct mcp356x_config mcp356x_config_##i = {                    \
      .bus = SPI_DT_SPEC_GET(                                                  \
          DT_INST(i, DT_DRV_COMPAT),                                           \
          SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8), 0),         \
  };                                                                           \
  DEVICE_DT_INST_DEFINE(i, mcp356x_init, NULL, &mcp356x_data_##i,              \
                        &mcp356x_config_##i, POST_KERNEL,                      \
                        CONFIG_SENSOR_INIT_PRIORITY, &mcp356x_api);

DT_INST_FOREACH_STATUS_OKAY(MCP356x_INIT)
