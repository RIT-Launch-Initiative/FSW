/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */
// #define DT_DRV_COMPAT microchip_mcp3561

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
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
// Get the DT node that is the instance identified by inst with n channels
#define INST_DT_MCP356x(inst, n) DT_INST(inst, microchip_mcp356##n)

// Define the init macro for different instances, channel numbers
#define MCP356x_INIT(instance, compat, channel_num)                            \
  static struct mcp356x_data mcp356##channel_num##_data_##instance;            \
                                                                               \
  static const struct mcp356x_config mcp356##channel_num##_config_##instance = \
      {.bus = SPI_DT_SPEC_GET(                                                 \
           INST_DT_MCP356x(instance, channel_num),                             \
           SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8), 0),        \
       .channels = channel_num};                                               \
  DEVICE_DT_DEFINE(INST_DT_MCP356x(instance, channel_num), mcp356x_init, NULL, \
                   &mcp356##channel_num##_data_##instance,                     \
                   &mcp356##channel_num##_config_##instance, POST_KERNEL,      \
                   CONFIG_SENSOR_INIT_PRIORITY, &mcp356x_api);

// 1 Channel ADC
#define MCP3561_INIT(i) MCP356x_INIT(i, microchip_mcp3561, 1)
// 4 Channel ADC
#define MCP3562_INIT(i) MCP356x_INIT(i, microchip_mcp3562, 2)
// 8 Channel ADC
#define MCP3564_INIT(i) MCP356x_INIT(i, microchip_mcp3564, 4)

#define CALL_WITH_ARG(arg, expr) expr(arg)

#define INST_DT_MCP356X_FOREACH(t, inst_expr)                                  \
  LISTIFY(DT_NUM_INST_STATUS_OKAY(microchip_mcp##t), CALL_WITH_ARG, (;),       \
          inst_expr)

INST_DT_MCP356X_FOREACH(3561, MCP3561_INIT);
INST_DT_MCP356X_FOREACH(3562, MCP3562_INIT);
INST_DT_MCP356X_FOREACH(3564, MCP3564_INIT);
