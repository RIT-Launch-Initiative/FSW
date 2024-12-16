#include "rfm9Xw.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(rfm9xw);
#define DT_DRV_COMPAT hoperf_rfm9xw
struct rfm9Xw_config {
    struct spi_dt_spec bus;
};

int read_rfm_reg(const struct rfm9Xw_config *config, const uint8_t reg, uint8_t *const result) {
    // Write Data
    uint8_t cmd[2] = {reg & (~0x80), 0};
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

#define REG_FIFO            0x0
#define REG_OP_MODE         0x1
#define REG_BITRATE_MSB     0x2
#define REG_BITRATE_LSB     0x3
#define REG_FDEV_MSB        0x4
#define REG_FDEV_LSB        0x5
#define REG_FRF_MSB         0x6
#define REG_FRF_MID         0x7
#define REG_FRF_LSB         0x8
#define REG_PA_CONFIG       0x9
#define REG_PA_RAMP         0xa
#define REG_OCP             0xb
#define REG_LNA             0xc
#define REG_RX_CONFIG       0xd
#define REG_RSSI_CONFIG     0xe
#define REG_RSSI_COLLISION  0xf
#define REG_RSSI_THRESH     0x10
#define REG_RSSI_VALUE      0x11
#define REG_RX_BW           0x12
#define REG_AFC_BW          0x13
#define REG_OOK_PEAK        0x14
#define REG_OOK_FIX         0x15
#define REG_OOK_AVG         0x16
#define REG_AFC_FEI         0x1a
#define REG_AFC_MSB         0x1b
#define REG_AFC_LSB         0x1c
#define REG_FEI_MSB         0x1d
#define REG_FEI_LSB         0x1e
#define REG_PREAMBLE_DETECT 0x1f
#define REG_RX_TIMEOUT1     0x20
#define REG_RX_TIMEOUT2     0x21
#define REG_RX_TIMEOUT3     0x22
#define REG_RX_DELAY        0x23
#define REG_OSC             0x24
#define REG_PREAMBLE_MSB    0x25
#define REG_PREAMBLE_LSB    0x26
#define REG_PACKET_CONFIG1  0x30
#define REG_PACKET_CONFIG2  0x31
#define REG_PAYLOAD_LENGTH  0x32
#define REG_NODE_ADRS       0x33
#define REG_BROADCAST_ADRS  0x34
#define REG_FIFO_THRESH     0x35
#define REG_SEQ_CONFIG1     0x36
#define REG_SEQ_CONFIG2     0x37
#define REG_TIMER_RESOL     0x38
#define REG_TIMER1_COEF     0x39
#define REG_TIMER2_COEF     0x3a
#define REG_IMAGE_CAL       0x3b
#define REG_TEMP            0x3c
#define REG_LOW_BAT         0x3d
#define REG_IRQ_FLAGS1      0x3e
#define REG_IRQ_FLAGS2      0x3f
#define REG_DIO_MAPPING1    0x40
#define REG_DIO_MAPPING2    0x41
#define REG_VERSION         0x42
#define REG_PLL_HOP         0x44
#define REG_TCXO            0x4b
#define REG_PADAC           0x4d
#define REG_FORMERTEMP      0x5b
#define REG_BITRATEFRAC     0x5d
#define REG_BITAGCREF       0x61
#define REG_AGCTHRESH1      0x62
#define REG_AGCTHRESH2      0x63
#define REG_AGCTHRESH3      0x64
#define REG_PLL             0x70

static const char *reg_names[0x71] = {
    [REG_FIFO] = "RegFifo",
    [REG_OP_MODE] = "RegOpMode",
    [REG_BITRATE_MSB] = "RegBitrateMsb",
    [REG_BITRATE_LSB] = "RegBitrateLsb",
    [REG_FDEV_MSB] = "RegFdevMsb",
    [REG_FDEV_LSB] = "RegFdevLsb",
    [REG_FRF_MSB] = "RegFrfMsb",
    [REG_FRF_MID] = "RegFrfMid",
    [REG_FRF_LSB] = "RegFrfLsb",
    [REG_PA_CONFIG] = "RegPaConfig",
    [REG_PA_RAMP] = "RegPaRamp",
    [REG_OCP] = "RegOcp",
    [REG_LNA] = "RegLna",
    [REG_RX_CONFIG] = "RegRxConfig",
    [REG_RSSI_CONFIG] = "RegRssiConfig",
    [REG_RSSI_COLLISION] = "RegRssiCollision",
    [REG_RSSI_THRESH] = "RegRssiThresh",
    [REG_RSSI_VALUE] = "RegRssiValue",
    [REG_RX_BW] = "RegRxBw",
    [REG_AFC_BW] = "RegAfcBw",
    [REG_OOK_PEAK] = "RegOokPeak",
    [REG_OOK_FIX] = "RegOokFix",
    [REG_OOK_AVG] = "RegOokAvg",
    [REG_AFC_FEI] = "RegAfcFei",
    [REG_AFC_MSB] = "RegAfcMsb",
    [REG_AFC_LSB] = "RegAfcLsb",
    [REG_FEI_MSB] = "RegFeiMsb",
    [REG_FEI_LSB] = "RegFeiLsb",
    [REG_PREAMBLE_DETECT] = "RegPreambleDetect",
    [REG_RX_TIMEOUT1] = "RegRxTimeout1",
    [REG_RX_TIMEOUT2] = "RegRxTimeout2",
    [REG_RX_TIMEOUT3] = "RegRxTimeout3",
    [REG_RX_DELAY] = "RegRxDelay",
    [REG_OSC] = "RegOsc",
    [REG_PREAMBLE_MSB] = "RegPreambleMsb",
    [REG_PREAMBLE_LSB] = "RegPreambleLsb",
    [REG_PACKET_CONFIG1] = "RegPacketConfig1",
    [REG_PACKET_CONFIG2] = "RegPacketConfig2",
    [REG_PAYLOAD_LENGTH] = "RegPayloadLength",
    [REG_NODE_ADRS] = "RegNodeAdrs",
    [REG_BROADCAST_ADRS] = "RegBroadcastAdrs",
    [REG_FIFO_THRESH] = "RegFifoThresh",
    [REG_SEQ_CONFIG1] = "RegSeqConfig1",
    [REG_SEQ_CONFIG2] = "RegSeqConfig2",
    [REG_TIMER_RESOL] = "RegTimerResol",
    [REG_TIMER1_COEF] = "RegTimer1Coef",
    [REG_TIMER2_COEF] = "RegTimer2Coef",
    [REG_IMAGE_CAL] = "RegImageCal",
    [REG_TEMP] = "RegTemp",
    [REG_LOW_BAT] = "RegLowBat",
    [REG_IRQ_FLAGS1] = "RegIrqFlags1",
    [REG_IRQ_FLAGS2] = "RegIrqFlags2",
    [REG_DIO_MAPPING1] = "RegDioMapping1",
    [REG_DIO_MAPPING2] = "RegDioMapping2",
    [REG_VERSION] = "RegVersion",
    [REG_PLL_HOP] = "RegPllHop",
    [REG_TCXO] = "RegTcxo",
    [REG_PADAC] = "RegPadac",
    [REG_FORMERTEMP] = "RegFormertemp",
    [REG_BITRATEFRAC] = "RegBitratefrac",
    [REG_BITAGCREF] = "RegBitagcref",
    [REG_AGCTHRESH1] = "RegAgcthresh1",
    [REG_AGCTHRESH2] = "RegAgcthresh2",
    [REG_AGCTHRESH3] = "RegAgcthresh3",
    [REG_PLL] = "RegPll",
};

int32_t read_registers(const struct rfm9Xw_config *config) {
    static uint8_t registers[] = {
        REG_FIFO,
        REG_OP_MODE,
        REG_BITRATE_MSB,
        REG_BITRATE_LSB,
        REG_FDEV_MSB,
        REG_FDEV_LSB,
        REG_FRF_MSB,
        REG_FRF_MID,
        REG_FRF_LSB,
        REG_PA_CONFIG,
        REG_PA_RAMP,
        REG_OCP,
        REG_LNA,
        REG_RX_CONFIG,
        REG_RSSI_CONFIG,
        REG_RSSI_COLLISION,
        REG_RSSI_THRESH,
        REG_RSSI_VALUE,
        REG_RX_BW,
        REG_AFC_BW,
        REG_OOK_PEAK,
        REG_OOK_FIX,
        REG_OOK_AVG,
        REG_AFC_FEI,
        REG_AFC_MSB,
        REG_AFC_LSB,
        REG_FEI_MSB,
        REG_FEI_LSB,
        REG_PREAMBLE_DETECT,
        REG_RX_TIMEOUT1,
        REG_RX_TIMEOUT2,
        REG_RX_TIMEOUT3,
        REG_RX_DELAY,
        REG_OSC,
        REG_PREAMBLE_MSB,
        REG_PREAMBLE_LSB,
        REG_PACKET_CONFIG1,
        REG_PACKET_CONFIG2,
        REG_PAYLOAD_LENGTH,
        REG_NODE_ADRS,
        REG_BROADCAST_ADRS,
        REG_FIFO_THRESH,
        REG_SEQ_CONFIG1,
        REG_SEQ_CONFIG2,
        REG_TIMER_RESOL,
        REG_TIMER1_COEF,
        REG_TIMER2_COEF,
        REG_IMAGE_CAL,
        REG_TEMP,
        REG_LOW_BAT,
        REG_IRQ_FLAGS1,
        REG_IRQ_FLAGS2,
        REG_DIO_MAPPING1,
        REG_DIO_MAPPING2,
        REG_VERSION,
        REG_PLL_HOP,
        REG_TCXO,
        REG_PADAC,
        REG_FORMERTEMP,
        REG_BITRATEFRAC,
        REG_BITAGCREF,
        REG_AGCTHRESH1,
        REG_AGCTHRESH2,
        REG_AGCTHRESH3,
        REG_PLL,
    };

    uint8_t i;
    for (i = 0; i < sizeof(registers); i++) {
        uint8_t val = 0;
        int32_t err = read_rfm_reg(config, registers[i], &val);
        if (err != 0) {
            LOG_ERR("Error reading register: %d", err);
        }
        if (reg_names[registers[i]] == NULL) {
            LOG_INF("      0x%02x:\t%02x", registers[i], val);
        } else {
            LOG_INF("%18s:\t%02x", reg_names[registers[i]], val);
        }
    }
    return 0;
}

enum RfmLongRangeModeSetting {
    RfmLongRangeModeSetting_FSK_OOK_MODE = 0,
    RfmLongRangeModeSetting_LORA_TM_MODE = 1,
};

enum RfmModulationType {
    RfmModulationType_FSK = 0x00,
    RfmModulationType_OOK = 0x01,
};

//BitRate = FXOSC/(BitRate(15,0)+BitRateFrac/16)

//FXOSC = 32 Mhz
//FSTEP = FXOSC/(2^19)
//Fdev = Fstep x Fdev(15,0)
//Frf = Fstep x Frf(23,0)

struct rfm9Xw_data {
    enum RfmLongRangeModeSetting long_range_mode_setting;
    enum RfmModulationType modulation_type;
};

int32_t switch_modes() {
    LOG_INF("SLEEP, SWITCH MODES, UNSLEEP NOT IMPLEMENTED");
    return 0;
}

int32_t rfm9x_print_info(const struct rfm9Xw_config *config) {
    uint8_t frf_msb = 0;
    uint8_t frf_mid = 0;
    uint8_t frf_lsb = 0;
    read_rfm_reg(config, REG_FRF_MSB, &frf_msb);
    read_rfm_reg(config, REG_FRF_MID, &frf_mid);
    read_rfm_reg(config, REG_FRF_LSB, &frf_lsb);

    const double fstep = 61.03515625;
    double frf = fstep * (double) ((frf_msb << 16) | (frf_mid << 8) | frf_lsb);
    LOG_INF("Frf = %.4f", frf);

    return 0;
}
int32_t rfm9x_dostuff(const struct device *dev) {
    LOG_INF("Hello im the driver");
    const struct rfm9Xw_config *config = dev->config;
    read_registers(config);
    return 0;
}

static int rfm9xw_init(const struct device *dev) {
    const struct rfm9Xw_config *config = dev->config;
    // struct rfm9Xw_data *data = dev->data;
    LOG_INF("Initializing rfm9xw");
    if (!device_is_ready(config->bus.bus)) {
        LOG_ERR("SPI bus '%s'not ready", config->bus.bus->name);
        return -ENODEV;
    }
    rfm9x_print_info(config);
    return 0;
}

// static const struct adc_driver_api mcp356x_api = {
// .channel_setup = &mcp356x_channel_setup,
// .read = &mcp356x_read_channel,
// .ref_internal = 2400,
// };

#define RFM9XW_INIT(n)                                                                                                 \
    static struct rfm9Xw_data rfm9Xw_data_##n;                                                                         \
                                                                                                                       \
    static const struct rfm9Xw_config rfm9Xw_config_##n = {                                                            \
        .bus = SPI_DT_SPEC_INST_GET(n, SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8), 0),                    \
    };                                                                                                                 \
                                                                                                                       \
    DEVICE_DT_INST_DEFINE(n, rfm9xw_init, NULL, &rfm9Xw_data_##n, &rfm9Xw_config_##n, POST_KERNEL, 90, NULL);

DT_INST_FOREACH_STATUS_OKAY(RFM9XW_INIT)
