/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mcp356x);

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)                                                   \
  ((byte)&0x80 ? '1' : '0'), ((byte)&0x40 ? '1' : '0'),                        \
      ((byte)&0x20 ? '1' : '0'), ((byte)&0x10 ? '1' : '0'),                    \
      ((byte)&0x08 ? '1' : '0'), ((byte)&0x04 ? '1' : '0'),                    \
      ((byte)&0x02 ? '1' : '0'), ((byte)&0x01 ? '1' : '0')

enum MCP_Reg {
  MCP_reg_ADCDATA = 0x0,   // 24 bits
  MCP_reg_CONFIG0 = 0x1,   // 8 bits
  MCP_reg_CONFIG1 = 0x2,   // 8 bits
  MCP_reg_CONFIG2 = 0x3,   // 8 bits
  MCP_reg_CONFIG3 = 0x4,   // 8 bits
  MCP_reg_IRQ = 0x5,       // 8 bits
  MCP_reg_MUX = 0x6,       // 8 bits
  MCP_reg_SCAN = 0x7,      // 24 bits
  MCP_reg_TIMER = 0x8,     // 24 bits
  MCP_reg_OFFSETCAL = 0x9, // 24 bits
  MCP_reg_GAINCAL = 0xA,   // 24 bits
  MCP_reg_LOCK = 0xD,      // 8 bits
  MCP_reg_CRCCFG = 0xF,    // 16 bits
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
  OSR_32 = 0b0000,    // 32  1
  OSR_64 = 0b0001,    // 61  1
  OSR_128 = 0b0010,   // 128 1
  OSR_256 = 0b0011,   // 256 1
  OSR_512 = 0b0100,   // 512 1
  OSR_1024 = 0b0101,  // 512 2
  OSR_2048 = 0b0110,  // 512 4
  OSR_4096 = 0b0111,  // 512 8
  OSR_8192 = 0b1000,  // 512 16
  OSR_16384 = 0b1001, // 512 32
  OSR_20480 = 0b1010, // 512 40
  OSR_24576 = 0b1011, // 512 48
  OSR_40960 = 0b1100, // 512 80
  OSR_49152 = 0b1101, // 512 96
  OSR_81920 = 0b1110, // 512 160
  OSR_98304 = 0b1111  // 512 192
};

struct mcp356x_data {
  uint8_t config0;
  uint8_t config1;
  uint8_t config2;
  uint8_t config3;
  uint8_t mux;
};

struct mcp356x_config {
  struct spi_dt_spec bus;
  uint8_t channels;    // 1
  uint8_t device_addr; // Written on chip packaging

  enum PRE prescale;
  enum CLK_SEL clock;
};

int dump_registers(const struct mcp356x_config *config);

int mcp_read_reg_8(const struct mcp356x_config *config, enum MCP_Reg reg,
                   uint8_t *result) {
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

  uint8_t reg8[2] = {0xbe, 0xad};
  struct spi_buf rxbuf = {
      .buf = &reg8,
      .len = 2,
  };
  struct spi_buf_set rxbufset = {
      .buffers = &rxbuf,
      .count = 1,
  };
  int res = spi_transceive_dt(&config->bus, &txbufset, &rxbufset);
  *result = reg8[1];

  return res;
}

int mcp_read_reg_24(const struct mcp356x_config *config, enum MCP_Reg reg,
                    uint32_t *data) {
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

  int ret = spi_transceive_dt(&config->bus, &txbufset, &rxbufset);
  if (ret < 0) {
    return ret;
  }
  uint32_t res = (reg24[1] << 16) | (reg24[2] << 8) | (reg24[3]);
  *data = res;
  return 0;
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
  // LOG_INF("SPI Writing 8 reg %d  to " BYTE_TO_BINARY_PATTERN, reg,
  // BYTE_TO_BINARY(data));

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
  uint8_t cmds[4];
  cmds[0] = command_specifier;
  cmds[1] = (data >> 16) & 0xFF;
  cmds[2] = (data >> 8) & 0xFF;
  cmds[3] = (data)&0xFF;

  struct spi_buf buf = {
      .buf = &cmds,
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
  struct mcp356x_data *data = dev->data;

  int res = 0;
  if (sequence->options != NULL) {
    LOG_ERR("This driver does not support sequencing");
    return -1;
  }
  if (sequence->buffer_size < 4) {
    LOG_ERR("MCP3561 buffer must be at least 4 bytes long");
    return -1;
  }

  // Wait X pulses of mclk after writing registers for result to come in
  // or, or wait for IRQ to come in and and make sure IRQ reg says its the right
  // one
  // k_usleep(2); // silly hack until then

  // Read data
  uint32_t val = 12345;
  res = mcp_read_reg_24(config, MCP_reg_ADCDATA, &val);
  if (res != 0) {
    return res;
  }
  *(int32_t *)(sequence->buffer) = val;

  return res;
}

int mcp356x_channel_setup(const struct device *dev,
                          const struct adc_channel_cfg *channel_cfg) {
  const struct mcp356x_config *config = dev->config;
  struct mcp356x_data *data = dev->data;

  uint8_t vref_sel_bit = 0b0;
  switch (channel_cfg->reference) {
  case ADC_REF_INTERNAL:
    vref_sel_bit = 0b1;
    break;
  case ADC_REF_EXTERNAL0:
    vref_sel_bit = 0b0;
    break;
  default:
    LOG_ERR("unsupported reference voltage %d", channel_cfg->reference);
    return -ENOTSUP;
  }

  if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
    LOG_ERR("unsupported ADC ACQ Time (Should be ADC_ACQ_TIME_DEFAULT)");
    return -ENOTSUP;
  }

  // Configure Gain
  uint8_t gain_bits = 0;
  switch (channel_cfg->gain) {
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
    LOG_ERR("unsupported channel gain '%d'", channel_cfg->gain);
    return -ENOTSUP;
  }

  // TODO reference voltage, OSR, gain is set to that of which channel has been
  // configured last
  // channel_read doesnt get access to cfg so have to store that internally.
  // solution: driver keeps internal map of channels and stuff so can index with
  // channelid to find params it wants

  // right now just need one channel so im not worrying about this
  data->config0 = (data->config0 & 0b01111111) | (vref_sel_bit << 7);
  mcp_write_reg_8(config, MCP_reg_CONFIG0, data->config0);

  data->config2 = (data->config2 & 0b11000111) | (gain_bits << 3);
  mcp_write_reg_8(config, MCP_reg_CONFIG2, data->config2);

  uint8_t vin_plus = channel_cfg->input_positive;
  uint8_t vin_minus = channel_cfg->input_negative;

  data->mux = (vin_plus << 4) | vin_minus;
  data->mux = 0b00001000;
  mcp_write_reg_8(config, MCP_reg_MUX, data->mux);

  printk("After channel setup\n");
  dump_registers(config);

  return 0;
}

static const struct adc_driver_api mcp356x_api = {
    .channel_setup = &mcp356x_channel_setup,
    .read = &mcp356x_read_channel,
    .ref_internal = 2400,
};

int dump_registers(const struct mcp356x_config *config) {
  int res;
  const int num_8bts = 6;
  char *names[] = {"CONFIG0", "CONFIG1", "CONFIG2", "CONFIG3",
                   "IRQ",     "MUX",     "IRQ"};
  enum MCP_Reg registers_8bit[] = {
      MCP_reg_CONFIG0, MCP_reg_CONFIG1, MCP_reg_CONFIG2, MCP_reg_CONFIG3,
      MCP_reg_IRQ,     MCP_reg_MUX,     MCP_reg_IRQ};

  printk("Registers: ========\n");

  uint32_t adcdata = 0xFF44DE;
  mcp_read_reg_24(config, MCP_reg_ADCDATA, &adcdata);
  printk("ADCDATA: %x\n", adcdata);

  for (int reg = 0; reg < num_8bts; reg++) {
    uint8_t data = 0xcc;
    res = mcp_read_reg_8(config, registers_8bit[reg], &data);
    if (res != 0) {
      return res;
    }
    printk("%s: " BYTE_TO_BINARY_PATTERN "\n", names[reg],
           BYTE_TO_BINARY(data));
  }

  uint32_t timer = 978;
  mcp_read_reg_24(config, MCP_reg_TIMER, &timer);
  printk("TIMER: %d\n", timer);

  return 0;
}

static int mcp356x_init(const struct device *dev) {
  const struct mcp356x_config *config = dev->config;
  struct mcp356x_data *data = dev->data;

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

  // CLK Selection
  uint8_t clk_sel_bits = 0b0;
  switch (config->clock) {
  case CLK_EXTERNAL:
    clk_sel_bits = 0b00;
    break;
  case CLK_INTERNAL:
    clk_sel_bits = 0b11;
    break;
  case CLK_INTERNAL_NO_BROADCAST:
    clk_sel_bits = 0b10;
    break;
  }
  uint8_t adc_mode = 0b11; // page 91 (standby)
  data->config0 = (1 << 6) | (clk_sel_bits << 4) | adc_mode;
  mcp_write_reg_8(config, MCP_reg_CONFIG0, data->config0);

  // Page 92 CONFIG1 ----------------------------------------------------------

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

  data->config1 = (prescale_bits << 6);

  mcp_write_reg_8(config, MCP_reg_CONFIG1, data->config1);

  // Page 93 CONFIG2 ----------------------------------------------------------

  uint8_t boost = 0b10; // 1x boost (bias current) (default)
  uint8_t az_mux = 0b0; // mux auto zero internal (default)
  uint8_t az_ref = 0b1; // auto zero internal voltage ref (default)
  uint8_t reserved_bit = 0b1;

  data->config2 = (boost << 6) | (az_mux << 2) | (az_ref << 1) | reserved_bit;
  mcp_write_reg_8(config, MCP_reg_CONFIG2, data->config2);

  // Page 94 CONFIG3 ----------------------------------------------------------
  uint8_t conv_mode = 0b11;
  uint8_t data_format = 0b00; // 24 bit adc data (default)
  uint8_t crc_format = 0b0;   // 16 bit crc format (default)
  uint8_t en_crccom = 0b0;    // offset calibration (default)
  uint8_t en_offcal = 0b0;    // offset calibration (default)
  uint8_t en_gaincal = 0b0;   // gain calibration (default)
  data->config3 = (conv_mode << 6) | (data_format << 4) | (crc_format << 3) |
                  (en_crccom << 2) | (en_offcal << 1) | en_gaincal;

  int err = mcp_write_reg_8(config, MCP_reg_CONFIG3, data->config3);

  uint8_t irq_reg =
      0b00110111; // set irq to active low so we don't need a pull up resistor
  mcp_write_reg_8(config, MCP_reg_IRQ, irq_reg);

  // Scan register (THIS DRIVER DOES NOT SCAN)
  uint32_t scan_reg = 0;
  mcp_write_reg_24(config, MCP_reg_SCAN, scan_reg);

  uint32_t timer_reg = 0;
  mcp_write_reg_24(config, MCP_reg_TIMER, timer_reg);

  (void)dump_registers(config);

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
