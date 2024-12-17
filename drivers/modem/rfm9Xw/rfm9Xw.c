#include "rfm9Xw.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rfm9xw);
#define DT_DRV_COMPAT hoperf_rfm9xw
struct rfm9Xw_config {
    struct spi_dt_spec bus;
    enum RfmModelNumber model_num;
    struct gpio_dt_spec dio0_pin;
    struct gpio_dt_spec dio1_pin;
    struct gpio_dt_spec dio2_pin;
    struct gpio_dt_spec dio3_pin;
    struct gpio_dt_spec dio4_pin;
    struct gpio_dt_spec dio5_pin;
};

struct rfm9Xw_data {
    enum RfmLongRangeModeSetting long_range_mode;
    enum RfmModulationType modulation_type;
    enum RfmLowFrequencyMode low_frequency_mode;
    enum RfmTransceiverMode transceiver_mode;
    enum RfmPacketConfigDataMode data_mode;
    uint32_t carrier_freq;
    uint32_t deviation_freq;
    uint32_t bitrate;
    enum RfmDio0Mapping dio0_mapping;
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

int write_rfm_reg(const struct rfm9Xw_config *config, const uint8_t reg, const uint8_t data) {
    // Write Data
    uint8_t cmd[2] = {0b10000000 | (reg & 0b01111111), data};

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

// Helpers for printing out register information
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)                                                                                           \
    ((byte) & 0x80 ? '1' : '0'), ((byte) & 0x40 ? '1' : '0'), ((byte) & 0x20 ? '1' : '0'),                             \
        ((byte) & 0x10 ? '1' : '0'), ((byte) & 0x08 ? '1' : '0'), ((byte) & 0x04 ? '1' : '0'),                         \
        ((byte) & 0x02 ? '1' : '0'), ((byte) & 0x01 ? '1' : '0')

int32_t bitrate_setting_from_desired_fsk(uint32_t bits_per_sec, uint16_t *bitrate, uint8_t *bitrate_frac) {
    // bits_per_sec = FXOSC/(BitRate(15,0) + BitRateFrac/16)
    // min(bits_per_sec) = FXOSC/(65536)
    // max(bits_per_sec) = FXOSC/(1/16)
    static const uint32_t min_bitrate = RFM_BIT_RATE_FSK_BPS_MIN;
    static const uint32_t max_bitrate = RFM_BIT_RATE_FSK_BPS_MAX;
    if (bits_per_sec > max_bitrate) {
        LOG_ERR("Requested out of range bitrate: %d (too large)", bits_per_sec);
        return -ENOTSUP;
    } else if (bits_per_sec < min_bitrate) {
        LOG_ERR("Requested out of range bitrate: %d (too small)", bits_per_sec);
        return -ENOTSUP;
    }
    // BitRate * (BitRate(15,0) + BitRateFrac/16) = FXOSC
    // BitRate(15,0) + BitRateFrac/16 = FXOSC/BitRate
    double combined = (double) FXOSC_HZ / (double) bits_per_sec;
    LOG_INF("Combined: %.3f", combined);
    *bitrate = (int16_t) combined;
    *bitrate_frac = (uint8_t) ((combined - *bitrate) * 16);

    // Return the actual bitrate achieved on no error
    return (int32_t) ((double) (FXOSC_HZ) / ((double) *bitrate + ((double) *bitrate_frac / 16.0)));
}

/**
 * @param model the model of chip this frequency is asking for
 * @param freq the carrier frequency to calculate registers for
 * @param msb[out] out parameter of the most significant byte for frequency register. Corresponds to REG_FRF_MSB. Not set if out of range
 * @param mid[out] Corresponds to REG_FRF_MID. Not set if out of range
 * @param lsb[out] Corresponds to REG_FRF_LSB. Not set if out of range
 * @return -ERANGE if the requested frequency is out of range for this model
 * 0 if conversion was successful
 */
int32_t frf_reg_from_frequency(enum RfmModelNumber model, uint32_t freq, uint8_t *msb, uint8_t *mid, uint8_t *lsb) {
    switch (model) {
        case RfmModelNumber_95W:
            if (freq < RFM95_SYNTH_MIN_HZ || freq > RFM95_SYNTH_MAX_HZ) {
                return -ERANGE;
            }
            break;
        case RfmModelNumber_96W:
        case RfmModelNumber_98W:
            // 96 and 98 have same range
            if (freq < RFM98_SYNTH_MIN_HZ || freq > RFM98_SYNTH_MAX_HZ) {
                return -ERANGE;
            }
            break;
        case RfmModelNumber_99W:
            if (freq < RFM99_SYNTH_MIN_HZ || freq > RFM99_SYNTH_MAX_HZ) {
                return -ERANGE;
            }
            break;
        default:
            LOG_ERR("Unknown RFM Model");
            return -ENOTSUP;
    }

    ////Frf = Fstep x Frf(23,0)
    // Frf(23,0) = Frf / Fstep
    const double fstep = 61.03515625;
    uint32_t frf = freq / fstep;
    *msb = (frf >> 16) & 0xff;
    *mid = (frf >> 8) & 0xff;
    *lsb = (frf) & 0xff;

    return 0;
}

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
            LOG_INF("      0x%02x:\t" BYTE_TO_BINARY_PATTERN, registers[i], BYTE_TO_BINARY(val));
        } else {
            LOG_INF("%18s:\t" BYTE_TO_BINARY_PATTERN, reg_names[registers[i]], BYTE_TO_BINARY(val));
        }
    }
    return 0;
}

uint8_t make_regopmode_fsk(enum RfmLongRangeModeSetting long_range_mode, enum RfmModulationType mod_type,
                           enum RfmLowFrequencyMode low_freq_mode, enum RfmTransceiverMode trans_mode) {
    return (long_range_mode & REG_OP_MODE_LONG_RANGE_MODE_MASK) | (mod_type & REG_OP_MODE_MODULATION_TYPE_MASK) |
           (low_freq_mode & REG_OP_MODE_LOW_FREQ_MODE_MASK) | (trans_mode & REG_OP_MODE_TRANS_MODE_MASK);
}
//BitRate = FXOSC/(BitRate(15,0)+BitRateFrac/16)

//FXOSC = 32 Mhz
//FSTEP = FXOSC/(2^19)
//Fdev = Fstep x Fdev(15,0)
//Frf = Fstep x Frf(23,0)

void rfm9x_print_data(enum RfmLongRangeModeSetting long_range_mode, enum RfmModulationType modulation_type,
                      enum RfmLowFrequencyMode low_frequency_mode, enum RfmTransceiverMode transceiver_mode,
                      uint8_t frf_msb, uint8_t frf_mid, uint8_t frf_lsb, uint16_t reg_bitrate, uint8_t reg_bitrate_frac,
                      enum RfmPacketConfigDataMode data_mode) {
    if (long_range_mode == RfmLongRangeModeSetting_LoraTmMode) {
        LOG_INF("LongRangeMode: LoRa(TM) Mode (0b1)");
        LOG_WRN("Not showing more data until LoRa(TM) mode is implemented");
        return;
    } else {
        LOG_INF("LongRangeMode: FSK/OOK Mode (0b0) (default)");
    }
    if (modulation_type == RfmModulationType_FSK) {
        LOG_INF("ModulationType: FSK (0b0) (default)");
    } else if (modulation_type == RfmModulationType_OOK) {
        LOG_INF("ModulationType: OOK (0b1)");
    } else {
        LOG_WRN("ModulationType: RESERVED");
    }

    LOG_INF("LowFrequencyMode: 0x%02x", (uint8_t) (low_frequency_mode >> 3));

    switch (transceiver_mode) {
        case RfmTransceiverMode_Sleep:
            LOG_INF("TransceiverMode: Sleep (0b000)");
            break;
        case RfmTransceiverMode_Standby:
            LOG_INF("TransceiverMode: Standby (0b001) (default)");
            break;
        case RfmTransceiverMode_FsModeTx:
            LOG_INF("TransceiverMode: FS mode TX (0b010)");
            break;
        case RfmTransceiverMode_Transmitter:
            LOG_INF("TransceiverMode: Transmitter (0b011)");
            break;
        case RfmTransceiverMode_FsModeRx:
            LOG_INF("TransceiverMode: FS mode TX (0b100)");
            break;
        case RfmTransceiverMode_Receiver:
            LOG_INF("TransceiverMode: Receiver (0b101)");
            break;
        default:
            LOG_WRN("TransceiverMode: RESERVED VALUE");
            break;
    }

    //BitRate = FXOSC/(BitRate(15,0)+BitRateFrac/16)
    double bitrate = FXOSC_HZ / ((double) reg_bitrate + (double) reg_bitrate_frac / 16.0);
    LOG_INF("BitRate: %.3f: (0x%04x)", bitrate, reg_bitrate);

    const double fstep = 61.03515625;
    double frf = fstep * (double) ((frf_msb << 16) | (frf_mid << 8) | frf_lsb);
    LOG_INF("Carrier Frequency = %.4f - (%02x, %02x, %02x)", frf, frf_msb, frf_mid, frf_lsb);

    if (data_mode == RfmPacketConfigDataMode_Continuous) {
        LOG_INF("DataMode: Continuous");
    } else if (data_mode == RfmPacketConfigDataMode_Packet) {
        LOG_INF("DataMode: Packet (Default)");
    } else {
        LOG_WRN("DataMode: UNKNOWN");
    }
}
int32_t rfm9x_print_reg_info(const struct rfm9Xw_config *config) {
    uint8_t reg_op_mode = 0;
    read_rfm_reg(config, REG_OP_MODE, &reg_op_mode);

    enum RfmLongRangeModeSetting long_range_mode = reg_op_mode & REG_OP_MODE_LONG_RANGE_MODE_MASK;
    enum RfmLowFrequencyMode low_freq_mode = reg_op_mode & REG_OP_MODE_LOW_FREQ_MODE_MASK;
    enum RfmModulationType mod_type = reg_op_mode & REG_OP_MODE_MODULATION_TYPE_MASK;
    enum RfmTransceiverMode trans_mode = reg_op_mode & REG_OP_MODE_TRANS_MODE_MASK;

    uint8_t bitrate_parts[2] = {0};
    read_rfm_reg(config, REG_BITRATE_MSB, &bitrate_parts[0]);
    read_rfm_reg(config, REG_BITRATE_LSB, &bitrate_parts[1]);
    uint16_t reg_bitrate = (bitrate_parts[0] << 8) | bitrate_parts[1];

    uint8_t reg_bitrate_frac = 0;
    read_rfm_reg(config, REG_BITRATEFRAC, &reg_bitrate_frac);

    uint8_t frf_msb = 0;
    uint8_t frf_mid = 0;
    uint8_t frf_lsb = 0;
    read_rfm_reg(config, REG_FRF_MSB, &frf_msb);
    read_rfm_reg(config, REG_FRF_MID, &frf_mid);
    read_rfm_reg(config, REG_FRF_LSB, &frf_lsb);

    uint8_t reg_packet_config2 = 0;
    read_rfm_reg(config, REG_PACKET_CONFIG2, &reg_packet_config2);
    enum RfmPacketConfigDataMode data_mode = reg_packet_config2 & REG_PACKET_CONFIG2_DATA_MODE_MASK;

    LOG_INF("Device Information:");
    rfm9x_print_data(long_range_mode, mod_type, low_freq_mode, trans_mode, frf_msb, frf_mid, frf_lsb, reg_bitrate,
                     reg_bitrate_frac, data_mode);
    return 0;
}

int32_t switch_trans_mode(const struct rfm9Xw_config *config, struct rfm9Xw_data *data,
                          enum RfmTransceiverMode trans_mode) {
    data->transceiver_mode = trans_mode;
    uint8_t reg_op_mode =
        make_regopmode_fsk(data->long_range_mode, data->modulation_type, data->low_frequency_mode, trans_mode);
    write_rfm_reg(config, REG_OP_MODE, reg_op_mode);
    return 0;
}

int32_t set_frequency(const struct rfm9Xw_config *config, uint8_t msb, uint8_t mid, uint8_t lsb) {
    int res = 0;
    res = write_rfm_reg(config, REG_FRF_MSB, msb);
    if (res < 0) {
        return res;
    }
    res = write_rfm_reg(config, REG_FRF_MID, mid);
    if (res < 0) {
        return res;
    }
    res = write_rfm_reg(config, REG_FRF_LSB, lsb);
    if (res < 0) {
        return res;
    }

    res = write_rfm_reg(config, REG_OP_MODE,
                        make_regopmode_fsk(RfmLongRangeModeSetting_FskOokMode, RfmModulationType_FSK,
                                           RfmLowFrequencyMode_LowFrequency, RfmTransceiverMode_FsModeTx));
    if (res < 0) {
        return res;
    }
    k_msleep(1);

    res = write_rfm_reg(config, REG_OP_MODE,
                        make_regopmode_fsk(RfmLongRangeModeSetting_FskOokMode, RfmModulationType_FSK,
                                           RfmLowFrequencyMode_LowFrequency, RfmTransceiverMode_Transmitter));
    return 0;
}

int32_t edgy_fsk(const struct rfm9Xw_config *config, struct rfm9Xw_data *data, const char *msg, size_t len,
                 int32_t delay_ms, int32_t deviation_hz) {

    uint8_t freq1_msb = 0;
    uint8_t freq1_mid = 0;
    uint8_t freq1_lsb = 0;
    frf_reg_from_frequency(config->model_num, data->carrier_freq - deviation_hz, &freq1_msb, &freq1_mid, &freq1_lsb);

    int32_t got1 = RFM_FSTEP_HZ * ((freq1_msb << 16) | (freq1_mid << 8) | freq1_lsb);

    uint8_t freq2_msb = 0;
    uint8_t freq2_mid = 0;
    uint8_t freq2_lsb = 0;
    frf_reg_from_frequency(config->model_num, data->carrier_freq + deviation_hz, &freq2_msb, &freq2_mid, &freq2_lsb);
    int32_t got2 = RFM_FSTEP_HZ * ((freq2_msb << 16) | (freq2_mid << 8) | freq2_lsb);

    LOG_INF("Actually using %d and %d", got1, got2);

    struct k_timer edgy_fsk_timer;
    k_timer_init(&edgy_fsk_timer, NULL, NULL);
    k_timer_start(&edgy_fsk_timer, K_MSEC(delay_ms), K_MSEC(delay_ms));

    int32_t res = 0;
    uint8_t last_bit = 1;
    res = set_frequency(config, freq1_msb, freq1_mid, freq1_lsb);

    while (1) {
        int64_t start = k_uptime_get();

        for (int biti = 0; biti < len * 8; biti++) {
            uint8_t byte = msg[biti / 8];
            uint8_t subindex = 7 - (biti % 8);
            uint8_t bit = (byte >> (subindex)) & 0x1;

            k_timer_status_sync(&edgy_fsk_timer);
            if (bit != last_bit) {
                if (bit) {
                    res = set_frequency(config, freq1_msb, freq1_mid, freq1_lsb);
                } else {
                    res = set_frequency(config, freq2_msb, freq2_mid, freq2_lsb);
                }
                switch_trans_mode(config, data, RfmTransceiverMode_Transmitter);

                if (res < 0) {
                    LOG_ERR("Error switching frequency");
                }
            }
            last_bit = bit;
        }
        int64_t end = k_uptime_get();
        int64_t elapsed = end - start;
        double ms_per = (double) elapsed / (double) (len * 8);
        LOG_INF("Transmitted %d bits over %lldms. Time per symbol = %.2f ms", len * 8, elapsed, ms_per);
        switch_trans_mode(config, data, RfmTransceiverMode_FsModeTx);
        k_msleep(5000);
    }
    k_timer_stop(&edgy_fsk_timer);
    return 0;
}

int32_t rfm9x_dostuff(const struct device *dev) {
    LOG_INF("Hello im the driver");
    const struct rfm9Xw_config *config = dev->config;
    struct rfm9Xw_data *data = dev->data;

    LOG_INF("STM Data (may be incomplete)");
    // rfm9x_print_data(data->long_range_mode, data->modulation_type, data->low_frequency_mode, data->transceiver_mode, 0,
    //  0, 0, 0, 0, data->data_mode);
    LOG_INF("Bitrate: %d", data->bitrate);
    LOG_INF("CarrierFreq: %d", data->carrier_freq);
    LOG_INF("FreqDeviation: %d", data->deviation_freq);

    // const char msg[] = "hello you like like ants from up here.";
    // int32_t res = edgy_fsk(config, data, msg, sizeof(msg), 10, 100000);
    // if (res < 0) {
    //     LOG_ERR("Error transmitting");
    // }
    return 0;
}

int32_t switch_freqs(const struct rfm9Xw_config *config) {
    uint8_t freq1_msb = 0;
    uint8_t freq1_mid = 0;
    uint8_t freq1_lsb = 0;
    frf_reg_from_frequency(config->model_num, 434100000, &freq1_msb, &freq1_mid, &freq1_lsb);

    int32_t got1 = RFM_FSTEP_HZ * ((freq1_msb << 16) | (freq1_mid << 8) | freq1_lsb);

    uint8_t freq2_msb = 0;
    uint8_t freq2_mid = 0;
    uint8_t freq2_lsb = 0;
    frf_reg_from_frequency(config->model_num, 433900000, &freq2_msb, &freq2_mid, &freq2_lsb);
    int32_t got2 = RFM_FSTEP_HZ * ((freq2_msb << 16) | (freq2_mid << 8) | freq2_lsb);

    LOG_INF("Actually using %d and %d", got1, got2);

    struct k_timer edgy_fsk_timer;
    k_timer_init(&edgy_fsk_timer, NULL, NULL);
    k_timer_start(&edgy_fsk_timer, K_MSEC(10), K_MSEC(10));

    int res = 0;
    int64_t start = k_uptime_get();
    int iterations = 400;
    for (int i = 0; i < iterations; i++) {
        k_timer_status_sync(&edgy_fsk_timer);
        res = set_frequency(config, freq1_msb, freq1_mid, freq1_lsb);
        if (res < 0) {
            LOG_ERR("Error switching to freq 1");
        }

        k_timer_status_sync(&edgy_fsk_timer);
        res = set_frequency(config, freq2_msb, freq2_mid, freq2_lsb);
        if (res < 0) {
            LOG_ERR("Error switching to freq 2");
        }
    }
    int64_t end = k_uptime_get();
    int64_t elapsed = end - start;
    double ms_per = (double) elapsed / (double) (2 * iterations);
    LOG_INF("Switched frequency %d times over %lldms. switch time: %.2f", (int) (2 * iterations), elapsed, ms_per);
    k_timer_stop(&edgy_fsk_timer);
    // LOG_INF("All done");
    return 0;
}

int32_t startup_config(const struct rfm9Xw_data *data, const struct rfm9Xw_config *config) {
    // Set to sleep
    // Set to FSK/OOK or LoRa (only valid when sleep)
    // Set frequency
    // X Set BitRate
    // X Set Gaussian Filter behavior
    rfm9x_print_reg_info(config);
    int res = 0;
    res = write_rfm_reg(config, REG_OP_MODE,
                        make_regopmode_fsk(RfmLongRangeModeSetting_FskOokMode, RfmModulationType_FSK,
                                           RfmLowFrequencyMode_LowFrequency, RfmTransceiverMode_Standby));

    uint16_t bitrate_reg = 0;
    uint8_t bitrate_frac_reg = 0;
    res = bitrate_setting_from_desired_fsk(300000, &bitrate_reg, &bitrate_frac_reg);
    if (res < 0) {
        LOG_ERR("Error setting bitrate");
    } else {
        LOG_INF("Achieved bitrate: %d", res);
    }
    write_rfm_reg(config, REG_BITRATE_MSB, (bitrate_reg >> 8) & 0xff);
    write_rfm_reg(config, REG_BITRATE_LSB, (bitrate_reg) & 0xff);
    write_rfm_reg(config, REG_BITRATEFRAC, (bitrate_frac_reg) & 0xff);

    uint8_t max_power = 0x0;    // 0-4
    uint8_t output_power = 0x0; // 0-f
    res = write_rfm_reg(config, REG_PA_CONFIG, 0b10000000 | (max_power << 4) | (output_power));
    if (res < 0) {
        LOG_ERR("Error writing reg to set output pin");
    } else {
        LOG_INF("Wrote Reg");
    }

    uint8_t regopmode_val = make_regopmode_fsk(data->long_range_mode, RfmModulationType_FSK, data->low_frequency_mode,
                                               RfmTransceiverMode_Transmitter);

    res = write_rfm_reg(config, REG_OP_MODE, regopmode_val);
    if (res < 0) {
        LOG_ERR("Error writing reg");
    } else {
        LOG_INF("Wrote Reg");
    }

    rfm9x_print_reg_info(config);
    read_registers(config);
    return 0;
}

static int rfm9xw_init(const struct device *dev) {
    const struct rfm9Xw_config *config = dev->config;
    struct rfm9Xw_data *data = dev->data;
    LOG_INF("Initializing rfm9xw");
    if (!device_is_ready(config->bus.bus)) {
        LOG_ERR("SPI bus '%s'not ready", config->bus.bus->name);
        return -ENODEV;
    }

    if (data->modulation_type == RfmModulationType_FSK) {
        // Checks for FSK
        if (data->deviation_freq > RFM_FREQUENCY_DEVIATION_MAX || data->deviation_freq < RFM_FREQUENCY_DEVIATION_MIN) {
            LOG_ERR("Unsupported deviation frequency: %d. Must be between %d and %d", data->deviation_freq,
                    RFM_FREQUENCY_DEVIATION_MIN, RFM_FREQUENCY_DEVIATION_MAX);
            return -ERANGE;
        }
        if (data->bitrate > RFM_BIT_RATE_FSK_BPS_MAX || data->bitrate < RFM_BIT_RATE_FSK_BPS_MIN) {
            LOG_ERR("Unsupported FSK bitrate: %d. Must be between %d and %d", data->bitrate, RFM_BIT_RATE_FSK_BPS_MIN,
                    RFM_BIT_RATE_FSK_BPS_MAX);
            return -ERANGE;
        }
        if (data->deviation_freq + data->bitrate / 2 > 250000) {
            // Datasheet: pg 14: FDA condition
            LOG_ERR("Invalid Frequency deviation or bitrate: frequency_deviation + bitrate/2 =< 250 kHz");
        }
    }

    // rfm9x_print_reg_info(config);
    startup_config(data, config);
    return 0;
}

#define RFM9XW_INIT(n)                                                                                                 \
    static struct rfm9Xw_data rfm9Xw_data_##n = {                                                                      \
        .long_range_mode = DT_STRING_TOKEN(DT_INST(n, DT_DRV_COMPAT), long_range_mode),                                \
        .modulation_type = DT_STRING_TOKEN(DT_INST(n, DT_DRV_COMPAT), modulation_type),                                \
        .low_frequency_mode = DT_STRING_TOKEN(DT_INST(n, DT_DRV_COMPAT), low_frequency_mode),                          \
        .transceiver_mode = RfmTransceiverMode_Standby,                                                                \
        .data_mode = RfmPacketConfigDataMode_Packet,                                                                   \
        .carrier_freq = DT_PROP(DT_INST(n, DT_DRV_COMPAT), carrier_frequency),                                         \
        .deviation_freq = DT_PROP_OR(DT_INST(n, DT_DRV_COMPAT), deviation_frequency, 0),                               \
        .bitrate = DT_PROP(DT_INST(n, DT_DRV_COMPAT), bitrate),                                                        \
        .dio0_mapping = DT_STRING_TOKEN(DT_INST(n, DT_DRV_COMPAT), dio0_mapping),                                      \
    };                                                                                                                 \
                                                                                                                       \
    static const struct rfm9Xw_config rfm9Xw_config_##n = {                                                            \
        .bus = SPI_DT_SPEC_INST_GET(n, SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8), 0),                    \
        .model_num = RfmModelNumber_98W,                                                                               \
        .dio0_pin = GPIO_DT_SPEC_INST_GET_OR(n, dio0_pin, {0}),                                                        \
    };                                                                                                                 \
                                                                                                                       \
    DEVICE_DT_INST_DEFINE(n, rfm9xw_init, NULL, &rfm9Xw_data_##n, &rfm9Xw_config_##n, POST_KERNEL, 90, NULL);

DT_INST_FOREACH_STATUS_OKAY(RFM9XW_INIT)
