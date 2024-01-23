/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp356x

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
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
  uint8_t channels; // 1 2 4
};

static int mcp356x_read_channel(const struct device *dev,
                                const struct adc_sequence *sequence) {

  return -1;
}

int mcp356x_channel_setup(const struct device *dev,
                          const struct adc_channel_cfg *channel_cfg) {
  return -1;
}

static const struct adc_driver_api mcp356x_api = {
    .channel_setup = &mcp356x_channel_setup,
    .read = &mcp356x_read_channel,
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
      .channels = DT_PROP(DT_INST(i, DT_DRV_COMPAT), num_channels)};           \
  DEVICE_DT_INST_DEFINE(i, mcp356x_init, NULL, &mcp356x_data_##i,              \
                        &mcp356x_config_##i, POST_KERNEL,                      \
                        CONFIG_SENSOR_INIT_PRIORITY, &mcp356x_api);

DT_INST_FOREACH_STATUS_OKAY(MCP356x_INIT)
