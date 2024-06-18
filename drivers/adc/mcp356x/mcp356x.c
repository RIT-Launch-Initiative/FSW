/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(mcp356x);
#define DT_DRV_COMPAT microchip_mcp356x

#define MAX_CHANNELS      8
#define MAX_INPUT_CHANNEL 15

// Helpers for printing out register information
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)                                                                                           \
    ((byte) & 0x80 ? '1' : '0'), ((byte) & 0x40 ? '1' : '0'), ((byte) & 0x20 ? '1' : '0'),                             \
        ((byte) & 0x10 ? '1' : '0'), ((byte) & 0x08 ? '1' : '0'), ((byte) & 0x04 ? '1' : '0'),                         \
        ((byte) & 0x02 ? '1' : '0'), ((byte) & 0x01 ? '1' : '0')

enum MCP_Reg {
    MCP_Reg_ADCDATA = 0x0,   // len 24/32
    MCP_Reg_CONFIG0 = 0x1,   // len 8
    MCP_Reg_CONFIG1 = 0x2,   // len 8
    MCP_Reg_CONFIG2 = 0x3,   // len 8
    MCP_Reg_CONFIG3 = 0x4,   // len 8
    MCP_Reg_IRQ = 0x5,       // len 8
    MCP_Reg_MUX = 0x6,       // len 8
    MCP_Reg_SCAN = 0x7,      // len 24
    MCP_Reg_TIMER = 0x8,     // len 24
    MCP_Reg_OFFSETCAL = 0x9, // len 24
    MCP_Reg_GAINCAL = 0xA,   // len 24
    MCP_Reg_LOCK = 0xD,      // len 8
    MCP_Reg_CRCCFG = 0xF,    // len 16
};

enum CLK_SEL {
    CLK_EXTERNAL = 0b00,              // Both listen for an external clock on the MCLK Pin
    CLK_EXTERNAL2 = 0b01,             // Both listen for an external clock on the MCLK Pin
    CLK_INTERNAL_NO_BROADCAST = 0b10, // Use the internal clock
    CLK_INTERNAL = 0b11,              // Use the internal clock and follow it with the MCLK Pin
};

enum PRE {
    PRE_1 = 0b00, // AMCLK = MCLK
    PRE_2 = 0b01, // AMCLK = MCLK/2
    PRE_4 = 0b10, // AMCLK = MCLK/4
    PRE_8 = 0b11, // AMCLK = MCLK/8
};

enum OSR {
    // Total,  bits,      OSR3,  OSR1
    OSR_32 = 0b0000,    // 32  x 1
    OSR_64 = 0b0001,    // 64  x 1
    OSR_128 = 0b0010,   // 128 x 1
    OSR_256 = 0b0011,   // 256 x 1
    OSR_512 = 0b0100,   // 512 x 1
    OSR_1024 = 0b0101,  // 512 x 2
    OSR_2048 = 0b0110,  // 512 x 4
    OSR_4096 = 0b0111,  // 512 x 8
    OSR_8192 = 0b1000,  // 512 x 16
    OSR_16384 = 0b1001, // 512 x 32
    OSR_20480 = 0b1010, // 512 x 40
    OSR_24576 = 0b1011, // 512 x 48
    OSR_40960 = 0b1100, // 512 x 80
    OSR_49152 = 0b1101, // 512 x 96
    OSR_81920 = 0b1110, // 512 x 160
    OSR_98304 = 0b1111  // 512 x 192
};

struct channel_map_entry {
    uint8_t mux_reg;
    bool differential;
    uint8_t gain_bits;
    uint8_t reference_bits;
};

struct mcp356x_data {
    struct channel_map_entry channel_map[MAX_CHANNELS];
    uint8_t enabled_channels_bitmap;

    // Configurations. If these registers match we don't need to rewrite them
    uint8_t config0;
    uint8_t config1;
    uint8_t config2;
    uint8_t config3;
    uint8_t mux;
};

struct mcp356x_config {
    struct spi_dt_spec bus;
    uint8_t device_addr; // Specified on chip packaging. Normally 1

    enum OSR osr;
    enum PRE prescale;
    enum CLK_SEL clock;
};
int mcp_read_reg_8(const struct mcp356x_config *config, const enum MCP_Reg reg, uint8_t *const result);

int mcp_read_reg_24(const struct mcp356x_config *config, const enum MCP_Reg reg, uint32_t *const data);
int mcp_write_reg_8(const struct mcp356x_config *config, const enum MCP_Reg reg, const uint8_t data);

int mcp_write_reg_24(const struct mcp356x_config *config, const enum MCP_Reg reg, const uint32_t data);

uint32_t sign_extend_24_32(uint32_t x) {
    const int bits = 24;
    uint32_t m = 1u << (bits - 1);
    return (x ^ m) - m;
}

static int mcp356x_read_channel(const struct device *dev, const struct adc_sequence *sequence) {
    const struct mcp356x_config *config = dev->config;
    struct mcp356x_data *data = dev->data;

    int res = 0;
    if (sequence->options != NULL) {
        LOG_ERR("This driver does not support repeated sequencing");
        return -1;
    }
    int32_t num_channels = __builtin_popcount(sequence->channels);

    if (sequence->buffer_size < 4 * num_channels) {
        LOG_ERR("MCP356x buffer must be at least 4 * # of channels read long. Reading "
                "%d needs %d bytes. was supplied %d",
                num_channels, 4 * num_channels, sequence->buffer_size);
        return -1;
    }

    uint32_t *sample_array = sequence->buffer;
    uint32_t sample_index = 0;

    for (uint8_t i = 0; i < MAX_CHANNELS; i++) {
        uint8_t channel_id = i;
        if (((sequence->channels >> channel_id) & 0b1) == 0) {
            continue;
        }

        struct channel_map_entry cfg = data->channel_map[channel_id];
        uint8_t config0 = (data->config0 & 0b01111111) | (1 << 7);
        if (config0 != data->config0) {
            printk("w\n");
            (void) mcp_write_reg_8(config, MCP_Reg_CONFIG0, config0);
            data->config0 = config0;
        }

        uint8_t config2 = (data->config2 & 0b11000111) | (cfg.gain_bits << 3);
        if (config2 != data->config2) {
            (void) mcp_write_reg_8(config, MCP_Reg_CONFIG2, config2);
            data->config2 = config2;
        }

        uint8_t mux_reg = data->channel_map[i].mux_reg;
        if (mux_reg != data->mux) {
            (void) mcp_write_reg_8(config, MCP_Reg_MUX, mux_reg);
            data->mux = mux_reg;

            // Wait for result to come in after ADC reset because of MUX change
            // right one
            k_usleep(1);
        }

        uint32_t val;
        res = mcp_read_reg_24(config, MCP_Reg_ADCDATA, &val);
        if (res != 0) {
            return res;
        }
        uint32_t signed_value = sign_extend_24_32(val);

        sample_array[sample_index] = signed_value;
        sample_index++;
    }

    return 0;
}

int mcp356x_channel_setup(const struct device *dev, const struct adc_channel_cfg *channel_cfg) {
    if (!device_is_ready(dev)) {
        LOG_ERR("Base mcp356x device is not setup. Can not setup channel");
        return -ENODEV;
    }
    struct mcp356x_data *data = dev->data;

    if (channel_cfg->channel_id > 8) {
        LOG_ERR("register/channel %d of %s invalid. Any mcp356x device supports max 4 "
                "differential channels or 8 single ended channels. You probably "
                "didn't mean to have this many channels",
                channel_cfg->channel_id, dev->name);
        return -ENODEV;
    }

    uint8_t mux_vin_p = channel_cfg->input_positive;
    if (mux_vin_p > MAX_INPUT_CHANNEL) {
        LOG_ERR("zephyr,positive invalid for %s channel %d.", dev->name, channel_cfg->channel_id);
    }
    uint8_t mux_vin_m = channel_cfg->input_negative;
    if (mux_vin_m > MAX_INPUT_CHANNEL) {
        LOG_ERR("zephyr,negative invalid for %s channel %d.", dev->name, channel_cfg->channel_id);
    }

    uint8_t mux_reg = (mux_vin_p << 4) | mux_vin_m;
    bool differential = channel_cfg->differential;
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

    enum adc_reference ref = channel_cfg->reference;

    // VREF Selection
    uint8_t vref_sel_bits = 0b0;
    switch (ref) {
        case ADC_REF_INTERNAL:
            vref_sel_bits = 0b1;
            break;
        case ADC_REF_EXTERNAL0:
            vref_sel_bits = 0b0;
            break;
        default:
            LOG_ERR("unsupported reference voltage '%d'", ref);
            return -ENOTSUP;
    }

    struct channel_map_entry entry = {
        .mux_reg = mux_reg,
        .differential = differential,
        .gain_bits = gain_bits,
        .reference_bits = vref_sel_bits,
    };

    data->channel_map[channel_cfg->channel_id] = entry;

    return 0;
}

static const struct adc_driver_api mcp356x_api = {
    .channel_setup = &mcp356x_channel_setup,
    .read = &mcp356x_read_channel,
    .ref_internal = 2400,
};

int dump_registers(const struct mcp356x_config *config) {
    int res;
    const int num_8bit_reg = 6;
    char *names[] = {"CONFIG0", "CONFIG1", "CONFIG2", "CONFIG3", "IRQ", "MUX", "IRQ"};
    enum MCP_Reg registers_8bit[] = {MCP_Reg_CONFIG0, MCP_Reg_CONFIG1, MCP_Reg_CONFIG2, MCP_Reg_CONFIG3,
                                     MCP_Reg_IRQ,     MCP_Reg_MUX,     MCP_Reg_IRQ};

    printk("Registers: ========\n");

    uint32_t adcdata = 0;
    mcp_read_reg_24(config, MCP_Reg_ADCDATA, &adcdata);
    printk("ADCDATA: %x\n", adcdata);

    for (int reg = 0; reg < num_8bit_reg; reg++) {
        uint8_t data = 0;
        res = mcp_read_reg_8(config, registers_8bit[reg], &data);
        if (res != 0) {
            return res;
        }
        printk("%s: " BYTE_TO_BINARY_PATTERN "\n", names[reg], BYTE_TO_BINARY(data));
    }

    uint32_t timer = 0;
    mcp_read_reg_24(config, MCP_Reg_TIMER, &timer);
    printk("TIMER: %d\n", timer);

    return 0;
}

static int mcp356x_init(const struct device *dev) {
    const struct mcp356x_config *config = dev->config;
    struct mcp356x_data *data = dev->data;
    data->enabled_channels_bitmap = 0;

    if (!device_is_ready(config->bus.bus)) {
        LOG_ERR("SPI bus '%s'not ready", config->bus.bus->name);
        return -ENODEV;
    }

    // Page 91 CONFIG0 ----------------------------------------------------------
    // VREF Selection done by channel but 1 is the default of the adc
    uint8_t vref_sel_bits = 0b1;

    // CLK Selection
    if (config->clock > 3) {
        LOG_ERR("Unknown clock selection bits %d", config->clock);
        return -ENOTSUP;
    }
    uint8_t clk_sel_bits = (uint8_t) config->clock;

    // No Current Source/Sink Selection Bits for Sensor Bias
    uint8_t adc_mode = 0b11; // continous conversion mode
    data->config0 = (vref_sel_bits << 7) | (1 << 6) | (clk_sel_bits << 4) | adc_mode;
    mcp_write_reg_8(config, MCP_Reg_CONFIG0, data->config0);

    // Page 92 CONFIG1 ----------------------------------------------------------
    // Configure OSR
    uint8_t osr_bits = (uint8_t) config->osr;

    // Configure PRESCALE
    if (config->prescale > 3) {
        LOG_ERR("Invalid prescale selection %d", config->prescale);
        return -ENOTSUP;
    }
    uint8_t prescale_bits = (uint8_t) config->prescale;

    data->config1 = (prescale_bits << 6) | (osr_bits << 2);

    mcp_write_reg_8(config, MCP_Reg_CONFIG1, data->config1);

    // Page 93 CONFIG2 ----------------------------------------------------------
    // Configure Gain
    static const uint8_t gain_bits = 0;
    static const uint8_t boost = 0b10; // 1x boost (bias current) (default)
    static const uint8_t az_mux = 0b0; // mux auto zero internal (default)
    static const uint8_t az_ref = 0b1; // auto zero internal voltage ref (default)
    static const uint8_t reserved_bit = 0b1;

    data->config2 = (boost << 6) | (gain_bits << 3) | (az_mux << 2) | (az_ref << 1) | reserved_bit;
    mcp_write_reg_8(config, MCP_Reg_CONFIG2, data->config2);

    // Page 94 CONFIG3 ----------------------------------------------------------
    static const uint8_t conv_mode = 0b11;
    static const uint8_t data_format = 0b00; // 24 bit adc data (default)
    static const uint8_t crc_format = 0b0;   // 16 bit crc format (default)
    static const uint8_t en_crccom = 0b0;    // offset calibration (default)
    static const uint8_t en_offcal = 0b0;    // offset calibration (default)
    static const uint8_t en_gaincal = 0b0;   // gain calibration (default)
    data->config3 =
        (conv_mode << 6) | (data_format << 4) | (crc_format << 3) | (en_crccom << 2) | (en_offcal << 1) | en_gaincal;

    (void) mcp_write_reg_8(config, MCP_Reg_CONFIG3, data->config3);

    static const uint8_t irq_reg = 0b00110111; // set irq to active low so we don't need a pull up resistor
    mcp_write_reg_8(config, MCP_Reg_IRQ, irq_reg);

    // Scan register (THIS DRIVER DOES NOT SCAN)
    uint32_t scan_reg = 0;
    mcp_write_reg_24(config, MCP_Reg_SCAN, scan_reg);

    uint32_t timer_reg = 0;
    mcp_write_reg_24(config, MCP_Reg_TIMER, timer_reg);

    return 0;
}

// Find an instance of an mcp
#define INST_DT_MCP356x(inst) DT_INST(inst, microchip_mcp356x)

// Initialize an adc device
#define MCP356X_INIT(instance)                                                                                         \
    static struct mcp356x_data mcp356x_data_##instance;                                                                \
                                                                                                                       \
    static const struct mcp356x_config mcp356x_config_##instance = {                                                   \
        .bus = SPI_DT_SPEC_GET(INST_DT_MCP356x(instance), SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8), 0), \
        .device_addr = DT_PROP(INST_DT_MCP356x(instance), device_address),                                             \
        .clock = DT_STRING_TOKEN(INST_DT_MCP356x(instance), clock_selection),                                          \
        .osr = 2,                                                                                                      \
        .prescale = DT_STRING_TOKEN(INST_DT_MCP356x(instance), prescale)};                                             \
    DEVICE_DT_DEFINE(INST_DT_MCP356x(instance), mcp356x_init, NULL, &mcp356x_data_##instance,                          \
                     &mcp356x_config_##instance, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &mcp356x_api);

DT_INST_FOREACH_STATUS_OKAY(MCP356X_INIT)

// Register R/W ===============================================================

int mcp_read_reg_8(const struct mcp356x_config *config, const enum MCP_Reg reg, uint8_t *const result) {
    // Constants
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
    *result = reg8[1]; // reg8[0] is just the command we wrote

    return res;
}

int mcp_read_reg_24(const struct mcp356x_config *config, const enum MCP_Reg reg, uint32_t *const data) {
    // Constants
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

int mcp_write_reg_8(const struct mcp356x_config *config, const enum MCP_Reg reg, const uint8_t data) {
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
int mcp_write_reg_24(const struct mcp356x_config *config, const enum MCP_Reg reg, const uint32_t data) {
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
    cmds[3] = (data) & 0xFF;

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
