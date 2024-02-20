/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

/*
Registers:
NAME    len addr
ADCDATA   24  0x0
CONFIG0   8   0x1
CONFIG1   8   0x2
CONFIG2   8   0x3
CONFIG3   8   0x4
IRQ       8   0x5
MUX       8   0x6
SCAN      24  0x7
TIMER     24  0x8
OFFSETCAL 24  0x9
GAINCAL   24  0xA
LOCK      8   0xD
CRCCFG    16  0xF
*/

enum MCP_Reg {
  MCP_reg_ADCDATA = 0x0,
  MCP_reg_CONFIG0 = 0x1,
  MCP_reg_CONFIG1 = 0x2,
  MCP_reg_CONFIG2 = 0x3,
  MCP_reg_CONFIG3 = 0x4,
  MCP_reg_IRQ = 0x5,
  MCP_reg_MUX = 0x6,
  MCP_reg_SCAN = 0x7,
  MCP_reg_TIMER = 0x8,
  MCP_reg_OFFSETCAL = 0x9,
  MCP_reg_GAINCAL = 0xA,
  MCP_reg_LOCK = 0xD,
  MCP_reg_CRCCFG = 0xF,
};

enum CLK_SEL {
  CLK_EXTERNAL,
  CLK_INTERNAL,
  CLK_INTERNAL_NO_BROADCAST,
};

enum PRE {
  PRE_8, // AMCLK = MCLK/8
  PRE_4, // AMCLK = MCLK/4
  PRE_2, // AMCLK = MCLK/2
  PRE_1, // AMCLK = MCLK
};

enum OSR {
  // Total OSR3 OSR1 OSR[3:0]
  OSR_32,    // 32  1   0000
  OSR_64,    // 61  1   0001
  OSR_128,   // 128 1   0010
  OSR_256,   // 256 1   0011
  OSR_512,   // 512 1   0100
  OSR_1024,  // 512 2   0101
  OSR_2048,  // 512 4   0110
  OSR_4096,  // 512 8   0111
  OSR_8192,  // 512 16  1000
  OSR_16384, // 512 32  1001
  OSR_20480, // 512 40  1010
  OSR_24576, // 512 48  1011
  OSR_40960, // 512 80  1100
  OSR_49152, // 512 96  1101
  OSR_81920, // 512 160 1110
  OSR_98304  // 512 192 1111
};

LOG_MODULE_REGISTER(mcp356x, CONFIG_SENSOR_LOG_LEVEL);

struct mcp356x_data {
  int state;
};

struct mcp356x_config {
  struct spi_dt_spec bus;
  uint8_t channels;    // 1
  uint8_t device_addr; // Written on chip packaging

  enum adc_reference global_reference;
  enum adc_gain global_gain;
  enum OSR osr;
  enum PRE prescale;
  enum CLK_SEL clock;
};

uint8_t mcp_read_reg_8(const struct mcp356x_config *config, enum MCP_Reg reg) {
  static const uint8_t command_addr_pos = 2;
  static const uint8_t sread_command_mask = 0x01;

  // Device Specific
  const uint8_t device_address_mask = (config->device_addr << 6);
  const uint8_t sread_command = (device_address_mask | sread_command_mask);
  const uint8_t command_specifier = (reg << command_addr_pos) | sread_command;

  // Write Data
  uint8_t cmd[2] = {command_specifier, 0};

  struct spi_buf txbuf = {
      .buf = &cmd,
      .len = 2,
  };

  struct spi_buf_set txbufset = {
      .buffers = &txbuf,
      .count = 1,
  };

  uint8_t reg8[2];
  struct spi_buf rxbuf = {
      .buf = &reg8,
      .len = 2,
  };
  struct spi_buf_set rxbufset = {
      .buffers = &rxbuf,
      .count = 1,
  };

  spi_transceive_dt(&config->bus, &txbufset, &rxbufset);

  return reg8[1];
}

uint32_t mcp_read_reg_24(const struct mcp356x_config *config,
                         enum MCP_Reg reg) {
  static const uint8_t command_addr_pos = 2;
  static const uint8_t sread_command_mask = 0x01;

  // Device Specific
  const uint8_t device_address_mask = (config->device_addr << 6);
  const uint8_t sread_command = (device_address_mask | sread_command_mask);
  const uint8_t command_specifier = (reg << command_addr_pos) | sread_command;

  // Write Data
  uint8_t cmd[2] = {command_specifier, 0};

  struct spi_buf txbuf = {
      .buf = &cmd,
      .len = 2,
  };

  struct spi_buf_set txbufset = {
      .buffers = &txbuf,
      .count = 1,
  };

  uint8_t reg24[5];
  struct spi_buf rxbuf = {
      .buf = &reg24,
      .len = 5,
  };
  struct spi_buf_set rxbufset = {
      .buffers = &rxbuf,
      .count = 1,
  };

  spi_transceive_dt(&config->bus, &txbufset, &rxbufset);

  uint32_t res = (reg24[1] << 16) | (reg24[2] << 8) | (reg24[3]);
  return res;
}

int mcp_write_reg_8(const struct mcp356x_config *config, enum MCP_Reg reg,
                    uint8_t data) {
  // Write Constants
  static const uint8_t command_addr_pos = 2;
  static const uint8_t write_command_mask = 0x02;

  // Device Specific
  const uint8_t device_address_mask = (config->device_addr << 6);
  const uint8_t write_command = (device_address_mask | write_command_mask);
  const uint8_t command_specifier = (reg << command_addr_pos) | write_command;

  // Write Data
  uint8_t cmd[2] = {command_specifier, data};

  struct spi_buf buf = {
      .buf = &cmd,
      .len = 2,
  };

  struct spi_buf_set set = {
      .buffers = &buf,
      .count = 1,
  };
  return spi_write_dt(&config->bus, &set);
}
int mcp_write_reg_24(const struct mcp356x_config *config, enum MCP_Reg reg,
                     uint32_t data) {
  // Write Constants
  static const uint8_t command_addr_pos = 2;
  static const uint8_t write_command_mask = 0x02;

  // Device Specific
  const uint8_t device_address_mask = (config->device_addr << 6);
  const uint8_t write_command = (device_address_mask | write_command_mask);
  const uint8_t command_specifier = (reg << command_addr_pos) | write_command;

  // Write Data
  uint32_t cmd = (command_specifier << 24) | (data & 0x00FFFFFF);

  struct spi_buf buf = {
      .buf = &cmd,
      .len = 4,
  };

  struct spi_buf_set set = {
      .buffers = &buf,
      .count = 1,
  };
  return spi_write_dt(&config->bus, &set);
}

static int mcp356x_read_channel(const struct device *dev,
                                const struct adc_sequence *sequence) {
  const struct mcp356x_config *config = dev->config;

  if (sequence->options != NULL) {
    LOG_ERR("This driver does not support sequencing");
    return -1;
  }
  uint32_t channel = sequence->channels;

  uint8_t vin_plus = 0b1111 & (channel >> 4);
  uint8_t vin_minus = 0b1111 & (channel);
  uint8_t mux_reg = (vin_plus << 4) | vin_minus;

  // Write mux register
  mcp_write_reg_8(config, MCP_reg_MUX, mux_reg);

  // Wait X pulses of mclk for result to come in
  // or, or wait for IRQ to come in and and make sure IRQ reg says its the right
  // one
  k_usleep(10); // silly hack until then

  uint32_t val = mcp_read_reg_24(config, MCP_reg_ADCDATA);

  return val;
}

struct scan_reg {
  uint8_t dly : 3;
  uint8_t reserved : 5;
  uint16_t scan : 16;
};

int mcp356x_channel_setup(const struct device *dev,
                          const struct adc_channel_cfg *channel_cfg) {
  const struct mcp356x_config *config = dev->config;

  if (channel_cfg->reference != config->global_reference) {
    LOG_ERR("Reference for channel %d does not match the global reference. (It "
            "needs to)\n",
            channel_cfg->channel_id);
  }

  if (channel_cfg->gain != config->global_gain) {
    LOG_ERR("Gain for channel %d does not match the global gain. (It "
            "needs to)\n",
            channel_cfg->channel_id);
  }

  uint8_t mux_vin_p = channel_cfg->input_positive;
  uint8_t mux_vin_m = channel_cfg->input_negative;

  if (mux_vin_p > 0b1111) {
    LOG_ERR("Unsupported Vin+ %d", mux_vin_p);
    return -1;
  }
  if (mux_vin_m > 0b1111) {
    LOG_ERR("Unsupported Vin- %d", mux_vin_m);
    return -1;
  }

  uint8_t mux_register = mux_vin_p << 4 | mux_vin_m;

  // ADCDATA
  // CONFIG0 check
  // CONFIG1 check
  // CONFIG2
  // CONFIG3
  // IRQ
  // MUX
  // SCAN
  // TIMER
  // OFFSETCAL
  // GAINCAL
  // LOCK
  // CRCCFG

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
  if (config->channels != 1) {
    LOG_ERR("Only one channel MCP3561 is supported\n");
    return -ENOTSUP;
  }

  // Page 91 CONFIG0 ----------------------------------------------------------
  // VREF Selection
  uint8_t vref_sel_bits = 0b0;
  switch (config->global_reference) {
  case ADC_REF_INTERNAL:
    vref_sel_bits = 0b1;
    break;
  case ADC_REF_EXTERNAL0:
    vref_sel_bits = 0b0;
    break;
  default:
    LOG_ERR("unsupported reference voltage '%d'", config->global_reference);
    return -ENOTSUP;
  }

  // CLK Selection
  uint8_t clk_sel_bits = 0b0;
  switch (config->clock) {
  case CLK_EXTERNAL:
    clk_sel_bits = 0b0;
    break;
  case CLK_INTERNAL:
    clk_sel_bits = 0b0;
    break;
  case CLK_INTERNAL_NO_BROADCAST:
    clk_sel_bits = 0b0;
    break;
  }
  // No Current Source/Sink Selection Bits for Sensor Bias
  // TODO ADC_MODE
  uint8_t adc_mode = 0b10; // page 91 (standby)
  uint8_t CONFIG0 = (vref_sel_bits << 7) | (clk_sel_bits << 4) | adc_mode;
  mcp_write_reg_8(config, MCP_reg_CONFIG0, CONFIG0);

  // Page 92 CONFIG1 ----------------------------------------------------------
  // Configure OSR
  uint8_t osr_bits = 0;
  switch (config->osr) {
  case OSR_32:
    osr_bits = 0b0000;
    break;
  case OSR_64:
    osr_bits = 0b0001;
    break;
  case OSR_128:
    osr_bits = 0b0010;
    break;
  case OSR_256:
    osr_bits = 0b0011;
    break;
  case OSR_512:
    osr_bits = 0b0100;
    break;
  case OSR_1024:
    osr_bits = 0b0101;
    break;
  case OSR_2048:
    osr_bits = 0b0110;
    break;
  case OSR_4096:
    osr_bits = 0b0111;
    break;
  case OSR_8192:
    osr_bits = 0b1000;
    break;
  case OSR_16384:
    osr_bits = 0b1001;
    break;
  case OSR_20480:
    osr_bits = 0b1010;
    break;
  case OSR_24576:
    osr_bits = 0b1011;
    break;
  case OSR_40960:
    osr_bits = 0b1100;
    break;
  case OSR_49152:
    osr_bits = 0b1101;
    break;
  case OSR_81920:
    osr_bits = 0b1110;
    break;
  case OSR_98304:
    osr_bits = 0b1111;
    break;
  default:
    LOG_ERR("unsupported OSR '%d'", config->osr);
    return -ENOTSUP;
  }

  // Configure PRE
  uint8_t prescale_bits = 0;
  switch (config->prescale) {
  case PRE_8:
    prescale_bits = 0b11;
    break;
  case PRE_4:
    prescale_bits = 0b10;
    break;
  case PRE_2:
    prescale_bits = 0b01;
    break;
  case PRE_1:
    prescale_bits = 0b00;
    break;
  }

  uint8_t CONFIG1 = (prescale_bits << 6) | (osr_bits << 2);

  mcp_write_reg_8(config, MCP_reg_CONFIG1, CONFIG1);

  // Page 93 CONFIG2 ----------------------------------------------------------
  // Configure Gain
  uint8_t gain_bits = 0;
  switch (config->global_gain) {
  case ADC_GAIN_1_3:
    gain_bits = 0b000;
    break;
  case ADC_GAIN_1:
    gain_bits = 0b001;
    break;
  case ADC_GAIN_2:
    gain_bits = 0b010;
    break;
  case ADC_GAIN_4:
    gain_bits = 0b011;
    break;
  case ADC_GAIN_8:
    gain_bits = 0b100;
    break;
  case ADC_GAIN_16:
    gain_bits = 0b101;
    break;
  case ADC_GAIN_32:
    gain_bits = 0b110;
    break;
  case ADC_GAIN_64:
    gain_bits = 0b111;
    break;
  default:
    LOG_ERR("unsupported channel gain '%d'", config->global_gain);
    return -ENOTSUP;
  }

  uint8_t boost = 0b10; // 1x boost (bias current) (default)
  uint8_t az_mux = 0b0; // mux auto zero internal (default)
  uint8_t az_ref = 0b1; // auto zero internal voltage ref (default)
  uint8_t reserved_bit = 0b1;

  uint8_t CONFIG2 = (boost << 6) | (gain_bits << 3) | (az_mux << 2) |
                    (az_ref << 1) | reserved_bit;
  mcp_write_reg_8(config, MCP_reg_CONFIG2, CONFIG2);

  // Page 94 CONFIG3 ----------------------------------------------------------
  uint8_t conv_mode = 0b00;
  uint8_t data_format = 0b00; // 24 bit adc data (default)
  uint8_t crc_format = 0b0;   // 16 bit crc format (default)
  uint8_t en_crccom = 0b0;    // offset calibration (default)
  uint8_t en_offcal = 0b0;    // offset calibration (default)
  uint8_t en_gaincal = 0b0;   // gain calibration (default)
  uint8_t CONFIG3 = (conv_mode << 6) | (data_format << 4) | (crc_format << 3) |
                    (en_crccom << 2) | (en_offcal << 1) | en_gaincal;

  mcp_write_reg_8(config, MCP_reg_CONFIG3, CONFIG3);

  // Scan register (THIS DRIVER DOES NOT SCAN)
  uint32_t scan_reg = 0;
  mcp_write_reg_24(config, MCP_reg_SCAN, scan_reg);

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
       .device_addr =                                                          \
           DT_PROP(INST_DT_MCP356x(instance, channel_num), device_address),    \
       .channels = channel_num,                                                \
       .global_reference =                                                     \
           DT_STRING_TOKEN(INST_DT_MCP356x(instance, channel_num), reference), \
       .osr = DT_STRING_TOKEN(INST_DT_MCP356x(instance, channel_num), osr),    \
       .clock = DT_STRING_TOKEN(INST_DT_MCP356x(instance, channel_num),        \
                                clock_selection),                              \
       .prescale =                                                             \
           DT_STRING_TOKEN(INST_DT_MCP356x(instance, channel_num), prescale)}; \
  DEVICE_DT_DEFINE(INST_DT_MCP356x(instance, channel_num), mcp356x_init, NULL, \
                   &mcp356##channel_num##_data_##instance,                     \
                   &mcp356##channel_num##_config_##instance, POST_KERNEL,      \
                   CONFIG_SENSOR_INIT_PRIORITY, &mcp356x_api);

// 1 Channel ADC
// In the future, add MCP356x_INIT for 2, 4 channel and do foreach for those as
// well Only will work in devicetree if you add
// dts/bindings/adc/microchip,mcp356x.yaml
#define MCP3561_INIT(i) MCP356x_INIT(i, microchip_mcp3561, 1)

#define CALL_WITH_ARG(arg, expr) expr(arg)

#define INST_DT_MCP356X_FOREACH(t, inst_expr)                                  \
  LISTIFY(DT_NUM_INST_STATUS_OKAY(microchip_mcp##t), CALL_WITH_ARG, (;),       \
          inst_expr)

INST_DT_MCP356X_FOREACH(3561, MCP3561_INIT);