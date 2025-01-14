#include "rfm9Xw.h"

#include <assert.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rfm9xw);

#define HORUS_L2_RX
// #define DEBUG1
// #define DEBUG0
#include "horusl2.h"

#define DT_DRV_COMPAT hoperf_rfm9xw

struct rfm9Xw_config {
    struct spi_dt_spec bus;
    enum RfmModelNumber model_num;
    struct gpio_dt_spec reset_gpios;
    struct gpio_dt_spec dio_gpios[RFM_NUM_DIOS];
    enum RfmPowerAmplifierSelection power_amplifier;
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

    enum RfmMaxPower max_power;
    uint8_t output_power;

    enum RfmPaRamp pa_ramp;
    enum RfmModulationShaping modulation_shaping;

    enum RfmDio0Mapping dio0_mapping;
    enum RfmDio0Mapping dio1_mapping;
    enum RfmDio0Mapping dio2_mapping;

    uint8_t sync_word_len;
    uint64_t sync_word;
};
#define RFM_SPI_WRITE_BIT 0b10000000
#define RFM_SPI_REG_MASK  0b01111111
int read_rfm_reg(const struct rfm9Xw_config *config, const uint8_t reg, uint8_t *const result) {
    // Write Data
    uint8_t cmd[2] = {reg & RFM_SPI_REG_MASK, 0};
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

int write_rfm_reg_burst(const struct rfm9Xw_config *config, uint8_t reg, uint8_t *data, int32_t data_len) {
    // Write Data
    uint8_t write_and_reg = RFM_SPI_WRITE_BIT | (reg & RFM_SPI_REG_MASK);

    struct spi_buf write_and_reg_buf = {
        .buf = &write_and_reg,
        .len = 1,
    };
    struct spi_buf data_buf = {
        .buf = data,
        .len = data_len,
    };

    struct spi_buf bufs[2] = {write_and_reg_buf, data_buf};

    struct spi_buf_set set = {
        .buffers = bufs,
        .count = 2,
    };

    return spi_write_dt(&config->bus, &set);
}

int write_rfm_reg(const struct rfm9Xw_config *config, const uint8_t reg, uint8_t data) {
    return write_rfm_reg_burst(config, reg, &data, 1);
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

/**
 * Calculate the register settings to achieve a specific bitrate
 * @param bits_per_sec the desired bitrate
 * @param[out] bitrate_msb value for RegBitrateMsb register 
 * @param[out] bitrate_lsb value for RegBitrateLsb register 
 * @param[out] bitrate_frac value for RegBitrateFrac register 
 * @return -ERANGE if the desired bitrate is not allowed
 * the actual value of bitrate that will be used.
 */
int32_t bitrate_regs_from_bitrate_fsk(uint32_t bits_per_sec, uint8_t *bitrate_msb, uint8_t *bitrate_lsb,
                                      uint8_t *bitrate_frac) {
    // bits_per_sec = FXOSC/(BitRate(15,0) + BitRateFrac/16)
    // min(bits_per_sec) = FXOSC/(65536)
    // max(bits_per_sec) = FXOSC/(1/16)
    static const uint32_t min_bitrate = RFM_BIT_RATE_FSK_BPS_MIN;
    static const uint32_t max_bitrate = RFM_BIT_RATE_FSK_BPS_MAX;
    if (bits_per_sec > max_bitrate) {
        LOG_ERR("Requested out of range bitrate: %d (too large)", bits_per_sec);
        return -ERANGE;
    } else if (bits_per_sec < min_bitrate) {
        LOG_ERR("Requested out of range bitrate: %d (too small)", bits_per_sec);
        return -ERANGE;
    }
    // BitRate * (BitRate(15,0) + BitRateFrac/16) = FXOSC
    // BitRate(15,0) + BitRateFrac/16 = FXOSC/BitRate
    double combined = (double) FXOSC_HZ / (double) bits_per_sec;
    uint16_t bitrate = (int16_t) combined;
    *bitrate_msb = (bitrate >> 8) & 0xff;
    *bitrate_lsb = (bitrate) & 0xff;
    *bitrate_frac = (uint8_t) ((combined - bitrate) * 16);

    // Return the actual bitrate achieved on no error
    return (int32_t) ((double) (FXOSC_HZ) / ((double) bitrate + ((double) *bitrate_frac / 16.0)));
}

/**
 * Calculates the values of the Frf registers for a desired frequency
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

/**
 * Some names of registers for debugging purposes
 */
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

int32_t dump_registers(const struct rfm9Xw_config *config) {
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
            LOG_INF("      0x%02x:\t0x%02x :" BYTE_TO_BINARY_PATTERN, registers[i], val, BYTE_TO_BINARY(val));
        } else {
            LOG_INF("%18s:\t0x%02x " BYTE_TO_BINARY_PATTERN, reg_names[registers[i]], val, BYTE_TO_BINARY(val));
        }
    }
    return 0;
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

/**
 * Sets the RF carrier frequency registers on the module.
 * This does not immediately change the frequency that the module is transmitting at or receiving from.
 * The in use RF Carrier frequency is only changed when:
 * - Entering FSRX/FSTX modes
 * - Restarting the receiver 
 * Datasheet pg. 89
 */
int32_t set_frequency_by_reg(const struct rfm9Xw_config *config, uint8_t msb, uint8_t mid, uint8_t lsb) {
    uint8_t reg_frf[3] = {msb, mid, lsb};
    return write_rfm_reg_burst(config, REG_FRF_MSB, reg_frf, 3);
}

/**
 * Sets the RF carrier frequency.
 * This does not immediately change the frequency that the module is transmitting at or receiving from.
 * The in use RF Carrier frequency is only changed when:
 * - Entering FSRX/FSTX modes
 * - Restarting the receiver 
 * Datasheet pg. 89
 * @param dev the radio device
 * @param freq the frequency to set to
 * @return -ERANGE if the requested frequency is out of range for this model
 * 0 if the frequency was succesfully set
 * other sub 0 error codes from spi interaction errors.
 */
int32_t set_carrier_frequency(const struct device *dev, uint32_t freq) {
    const struct rfm9Xw_config *config = dev->config;
    struct rfm9Xw_data *data = dev->data;
    uint8_t msb = 0;
    uint8_t mid = 0;
    uint8_t lsb = 0;
    int32_t res = frf_reg_from_frequency(config->model_num, freq, &msb, &mid, &lsb);
    if (res < 0) {
        return res;
    }
    res = set_frequency_by_reg(config, msb, mid, lsb);
    if (res < 0) {
        return res;
    }
    return res;
}

int32_t set_bitrate_by_reg(const struct device *dev, uint8_t msb, uint8_t lsb, uint8_t frac) {
    const struct rfm9Xw_config *config = dev->config;
    int32_t res = write_rfm_reg(config, REG_BITRATE_MSB, msb);
    if (res < 0) {
        return res;
    }
    res = write_rfm_reg(config, REG_BITRATE_LSB, lsb);
    if (res < 0) {
        return res;
    }
    return write_rfm_reg(config, REG_BITRATEFRAC, frac);
}

int32_t set_bitrate(const struct device *dev, uint32_t bitrate) {
    uint8_t bitrate_msb = 0;
    uint8_t bitrate_lsb = 0;
    uint8_t bitrate_frac = 0;
    int32_t res = bitrate_regs_from_bitrate_fsk(bitrate, &bitrate_msb, &bitrate_lsb, &bitrate_frac);
    if (res < 0) {
        return res;
    } else {
        LOG_INF("Achieved bitrate: %d", res);
    }
    return set_bitrate_by_reg(dev, bitrate_msb, bitrate_lsb, bitrate_frac);
}

int32_t set_pramble_len(const struct device *dev, uint16_t preamble_len) {
    const struct rfm9Xw_config *config = dev->config;
    uint8_t msb = (preamble_len >> 8) & 0xff;
    uint8_t lsb = (preamble_len >> 8) & 0xff;
    int32_t res = write_rfm_reg(config, REG_PREAMBLE_MSB, msb);
    if (res < 0) {
        return res;
    }
    return write_rfm_reg(config, REG_PREAMBLE_LSB, lsb);
}

static int32_t set_power_amplifier(const struct device *dev, enum RfmPowerAmplifierSelection pin,
                                   enum RfmMaxPower max_power, uint8_t power_output) {
    if (power_output > RFM_MAX_OUTPUT_POWER) {
        LOG_WRN("Requested higher output power than available. Limitting to max");
        power_output = RFM_MAX_OUTPUT_POWER;
    }
    const struct rfm9Xw_config *config = dev->config;
    uint8_t val = (pin & RFM_PA_CONFIG_MASK_PA_SELECT) | (max_power & RFM_PA_CONFIG_MASK_MAX_POWER) | power_output;
    return write_rfm_reg(config, REG_PA_CONFIG, val);
}

/**
 * Calculate the values needed for Fdev registers from a frequency in Hertz
 * @return -ERANGE if the frequency is out of range. 0 if its in range
 */
static int32_t frequency_dev_regs_from_fdev(uint32_t fdev, uint8_t *msb, uint8_t *lsb) {
    if (fdev > RFM_MAX_FREQUENCY_DEVIATION_HZ) {
        return -ERANGE;
    }
    // Fdev = Fstep * Fdev[15,0]
    // Fdev / Fstep = Fdev[15,0]
    // Fstep = FX_OSC / 2^19
    // (Fdev * 2^19) / FX_OSC = Fdev[15,0]
    uint16_t val = (uint16_t) ((double) fdev / RFM_FSTEP_HZ);
    *msb = (val >> 8) & 0xff;
    *lsb = val & 0xff;
    return 0;
}

static int32_t set_frequency_deviation_by_reg(const struct device *dev, uint8_t msb, uint8_t lsb) {
    const struct rfm9Xw_config *config = dev->config;
    uint8_t data[2] = {msb, lsb};
    return write_rfm_reg_burst(config, REG_FDEV_MSB, data, 2);
}
static int32_t set_frequency_deviation(const struct device *dev, uint32_t freq_dev) {
    uint8_t msb = 0;
    uint8_t lsb = 0;
    if (frequency_dev_regs_from_fdev(freq_dev, &msb, &lsb) < 0) {
        LOG_ERR("Requested frequency deviation of %dHz is larger than possible max of %dHz", freq_dev,
                RFM_MAX_FREQUENCY_DEVIATION_HZ);
        return -ERANGE;
    }

    return set_frequency_deviation_by_reg(dev, msb, lsb);
}

static int32_t set_modulation_shaping(const struct device *dev, enum RfmModulationShaping shaping,
                                      enum RfmPaRamp ramp) {
    const struct rfm9Xw_config *config = dev->config;
    return write_rfm_reg(config, REG_PA_RAMP,
                         (shaping & RFM_REG_PA_RAMP_MASK_MODULATION_SHAPING) | (ramp & RFM_REG_PA_RAMP_MASK_PA_RAMP));
}

int32_t set_operating_mode(const struct device *dev, enum RfmLongRangeModeSetting long_range_mode,
                           enum RfmModulationType mod_type, enum RfmLowFrequencyMode low_freq_mode,
                           enum RfmTransceiverMode trans_mode) {
    const struct rfm9Xw_config *config = dev->config;
    uint8_t val = (long_range_mode & REG_OP_MODE_LONG_RANGE_MODE_MASK) | (mod_type & REG_OP_MODE_MODULATION_TYPE_MASK) |
                  (low_freq_mode & REG_OP_MODE_LOW_FREQ_MODE_MASK) | (trans_mode & REG_OP_MODE_TRANS_MODE_MASK);

    return write_rfm_reg(config, REG_OP_MODE, val);
}

unsigned short crc16(char *ptr, int count) {
    int crc;
    char i;
    crc = 0xffff;
    while (--count >= 0) {
        crc = crc ^ (int) *ptr++ << 8;
        i = 8;
        do {
            if (crc & 0x8000) crc = crc << 1 ^ 0x1021;
            else
                crc = crc << 1;
        } while (--i);
    }
    return (crc);
}

int32_t transmit_4fsk_packet(const struct device *dev, uint8_t preamble_len, uint8_t *buf, uint32_t len) {

    // LOG_HEXDUMP_INF(buf, len, "Packet");
    // LOG_INF("Packet Length: %d", len);
    const struct rfm9Xw_config *config = dev->config;
    struct rfm9Xw_data *data = dev->data;

    const uint32_t carrier = data->carrier_freq;
    const uint32_t deviation = data->deviation_freq;

    const uint32_t bitrate = 100;
    const int usec_per_symbol = 1000000 / bitrate;

    const uint32_t high = carrier + deviation;
    const uint32_t step = deviation * 2 / 3;
    const uint32_t symbols_fdev[4] = {3 * step, 2 * step, step, 0};

    // internal radio preamble, not horus preamble. when len = 0, radio transmits on the low end of its 2FSK. where this falls is controlled by frequency_deviation
    // we achieve fast frequency control by switching the deviation
    // transmitted_freq = high - fdev
    set_pramble_len(dev, 0);
    set_carrier_frequency(dev, high);
    // start transmitting
    set_frequency_deviation(dev, symbols_fdev[0]);
    set_operating_mode(dev, RfmLongRangeModeSetting_FskOokMode, RfmModulationType_FSK, RfmLowFrequencyMode_LowFrequency,
                       RfmTransceiverMode_Transmitter);

    struct k_timer bitrate_timer;
    k_timer_init(&bitrate_timer, NULL, NULL);
    k_timer_start(&bitrate_timer, K_USEC(usec_per_symbol), K_USEC(usec_per_symbol));

    // transmit preamble 0,1,2,3 (low, 2nd lowest, 2nfd highest, highest)
    for (int i = 0; i < preamble_len; i++) {
        for (int j = 0; j < 4; j++) {
            k_timer_status_sync(&bitrate_timer);
            set_frequency_deviation(dev, symbols_fdev[j]);
        }
    }

    for (int byte_index = 0; byte_index < len; byte_index++) {
        const uint8_t byte = buf[byte_index];
        const uint8_t syms[4] = {
            (byte >> 6) & 0b11,
            (byte >> 4) & 0b11,
            (byte >> 2) & 0b11,
            (byte >> 0) & 0b11,
        };
        for (int sym_index = 0; sym_index < 4; sym_index++) {
            uint8_t sym = syms[sym_index];
            uint32_t fdev = symbols_fdev[sym];
            k_timer_status_sync(&bitrate_timer);
            set_frequency_deviation(dev, fdev);
        }
    }
    // Last symbol
    k_timer_status_sync(&bitrate_timer);
    // Then turn off
    set_operating_mode(dev, RfmLongRangeModeSetting_FskOokMode, RfmModulationType_FSK, RfmLowFrequencyMode_LowFrequency,
                       RfmTransceiverMode_Standby);
    k_timer_stop(&bitrate_timer);

    return 0;
}

/**
 * Perform a 'Manual Reset' of the module 
 * (Datasheet pg.111 section 7.2.2)
 * @param config the module config 
 * @return any errors from configuring GPIO
 */
int32_t rfm9xw_software_reset(const struct device *dev) {
    const struct rfm9Xw_config *config = dev->config;

    LOG_DBG("Software resetting radio with %s pin %d", config->reset_gpios.port->name, (int) config->reset_gpios.pin);

    if (gpio_pin_configure_dt(&config->reset_gpios, GPIO_OUTPUT | GPIO_PULL_DOWN) < 0) {
        LOG_ERR("Failed to set pin to 0 to reset chip");
    }

    k_usleep(150); // >100us

    if (gpio_pin_configure_dt(&config->reset_gpios, GPIO_DISCONNECTED) < 0) {
        LOG_ERR("Failed to set pin to 0 to reset chip");
    }
    k_msleep(5);

    return 0;
}

int init_gpios(const struct rfm9Xw_config *config, struct rfm9Xw_data *data) {
    if (config->reset_gpios.port != NULL) {
        // Setup GPIO
        if (!gpio_is_ready_dt(&config->reset_gpios)) {
            LOG_ERR("Reset GPIO is not ready\n");
            return -ENODEV;
        }
        if (gpio_pin_configure_dt(&config->reset_gpios, GPIO_DISCONNECTED) < 0) {
            LOG_ERR("Failed to set pin to High-Z to allow chip to wake");
            return -ENODEV;
        }
    } else {
        LOG_WRN("No reset GPIO supplied for RFM9XW. Strange results may happen");
    }
    for (size_t i = 0; i < RFM_NUM_DIOS; i++) {
        if (config->dio_gpios[i].port != NULL) {
            LOG_INF("Initting DIO%d", i);
            if (!gpio_is_ready_dt(&config->dio_gpios[i])) {
                LOG_ERR("DIO%d GPIO is not ready\n", i);
                return -ENODEV;
            }
            if (gpio_pin_configure_dt(&config->dio_gpios[i], GPIO_OUTPUT_INACTIVE) < 0) {
                return -ENODEV;
            }
        } else {
            LOG_INF("No pin assigned to DIO%d", i);
        }
    }
    return 0;
}

static int rfm9xw_init(const struct device *dev) {
    const struct rfm9Xw_config *config = dev->config;
    struct rfm9Xw_data *data = dev->data;
    golay23_init();

    LOG_INF("Initializing rfm9xw");
    if (!device_is_ready(config->bus.bus)) {
        LOG_ERR("SPI bus '%s'not ready", config->bus.bus->name);
        return -ENODEV;
    }
    int res = init_gpios(config, data);
    if (res < 0) {
        LOG_ERR("Error setting up GPIOs: %d", res);
        return res;
    }

    res = rfm9xw_software_reset(dev);
    if (res < 0) {
        LOG_ERR("Unable to reset: %d", res);
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
            // Datasheet: pg. 14: FDA condition
            LOG_ERR("Invalid Frequency deviation or bitrate: frequency_deviation + bitrate/2 =< 250 kHz");
            return -ERANGE;
        }
    }

    // Set to standby
    set_operating_mode(dev, RfmLongRangeModeSetting_FskOokMode, RfmModulationType_FSK, RfmLowFrequencyMode_LowFrequency,
                       RfmTransceiverMode_Standby);

    set_power_amplifier(dev, config->power_amplifier, data->max_power, data->output_power);
    set_modulation_shaping(dev, data->modulation_shaping, data->pa_ramp);

    dump_registers(config);
    return 0;
}

int32_t rfm9xw_read_temperature(const struct device *dev, int8_t *celsius) {
    struct rfm9Xw_data *data = dev->data;

    set_operating_mode(dev, data->long_range_mode, data->modulation_type, data->low_frequency_mode,
                       RfmTransceiverMode_FsModeTx);
    k_msleep(100);
    uint8_t reg = 0;
    int ret = read_rfm_reg(dev->config, REG_TEMP, &reg);
    if (ret < 0) {
        return ret;
    }

    *celsius = reg & 0x7f;
    if ((reg & 0x80) == 0x80) {
        *celsius *= -1;
    }
    set_operating_mode(dev, data->long_range_mode, data->modulation_type, data->low_frequency_mode,
                       RfmTransceiverMode_Standby);
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
        .output_power = DT_PROP(DT_INST(n, DT_DRV_COMPAT), output_power),                                              \
        .max_power = DT_STRING_TOKEN(DT_INST(n, DT_DRV_COMPAT), max_power),                                            \
        .modulation_shaping = DT_STRING_TOKEN(DT_INST(n, DT_DRV_COMPAT), modulation_shaping),                          \
        .pa_ramp = DT_STRING_TOKEN(DT_INST(n, DT_DRV_COMPAT), pa_ramp),                                                \
        .dio0_mapping = DT_STRING_TOKEN(DT_INST(n, DT_DRV_COMPAT), dio0_mapping),                                      \
    };                                                                                                                 \
                                                                                                                       \
    static const struct rfm9Xw_config rfm9Xw_config_##n = {                                                            \
        .bus = SPI_DT_SPEC_INST_GET(n, SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8), 0),                    \
        .model_num = RfmModelNumber_98W,                                                                               \
        .reset_gpios = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),                                                  \
        .dio_gpios =                                                                                                   \
            {                                                                                                          \
                GPIO_DT_SPEC_INST_GET_OR(n, dio0_gpios, {0}),                                                          \
                GPIO_DT_SPEC_INST_GET_OR(n, dio1_gpios, {0}),                                                          \
                GPIO_DT_SPEC_INST_GET_OR(n, dio2_gpios, {0}),                                                          \
                GPIO_DT_SPEC_INST_GET_OR(n, dio3_gpios, {0}),                                                          \
                GPIO_DT_SPEC_INST_GET_OR(n, dio4_gpios, {0}),                                                          \
                GPIO_DT_SPEC_INST_GET_OR(n, dio5_gpios, {0}),                                                          \
            },                                                                                                         \
        .power_amplifier = DT_STRING_TOKEN(DT_INST(n, DT_DRV_COMPAT), power_amplifier),                                \
    };                                                                                                                 \
                                                                                                                       \
    DEVICE_DT_INST_DEFINE(n, rfm9xw_init, NULL, &rfm9Xw_data_##n, &rfm9Xw_config_##n, POST_KERNEL, 90, NULL);

DT_INST_FOREACH_STATUS_OKAY(RFM9XW_INIT)

int32_t edgy_fsk(const struct device *dev, struct rfm9Xw_data *data, const char *msg, size_t len, int32_t delay_ms,
                 int32_t deviation_hz) {
    const struct rfm9Xw_config *config = dev->config;
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

    struct k_timer bitrate_timer;
    k_timer_init(&bitrate_timer, NULL, NULL);
    k_timer_start(&bitrate_timer, K_MSEC(delay_ms), K_MSEC(delay_ms));

    int32_t res = 0;
    uint8_t last_bit = 1;
    res = set_frequency_by_reg(config, freq1_msb, freq1_mid, freq1_lsb);

    // SET PREAMBLE TO NOTHING SO YOURE NOT GOING 0101010110101010 forever. OR SET THE BITRATE REAL HIGH AND THE DEVIATION REAL LOW
    // set_carrier_frequency(config, 433500000);
    // set_bitrate(config, 1200);
    // set_power_amplifier(config, config->power_amplifier, data->max_power, data->output_power);
    // set_pramble_len(config, 1);
    // set_frequency_deviation(config, 0);

    while (1) {
        int64_t start = k_uptime_get();

        for (int biti = 0; biti < len * 8; biti++) {
            uint8_t byte = msg[biti / 8];
            uint8_t subindex = 7 - (biti % 8);
            uint8_t bit = (byte >> (subindex)) & 0x1;

            k_timer_status_sync(&bitrate_timer);

            if (bit != last_bit) {
                if (bit) {
                    res = set_frequency_by_reg(config, freq1_msb, freq1_mid, freq1_lsb);
                } else {
                    res = set_frequency_by_reg(config, freq2_msb, freq2_mid, freq2_lsb);
                }
                set_operating_mode(dev, RfmLongRangeModeSetting_FskOokMode, RfmModulationType_OOK,
                                   RfmLowFrequencyMode_LowFrequency, RfmTransceiverMode_FsModeTx);

                set_operating_mode(dev, RfmLongRangeModeSetting_FskOokMode, RfmModulationType_OOK,
                                   RfmLowFrequencyMode_LowFrequency, RfmTransceiverMode_Transmitter);

                if (res < 0) {
                    LOG_ERR("Error switching frequency");
                }
            }
            last_bit = bit;
        }
        k_timer_status_sync(&bitrate_timer);

        int64_t end = k_uptime_get();
        int64_t elapsed = end - start;
        double ms_per = (double) elapsed / (double) (len * 8);
        LOG_INF("Transmitted %d bits over %lldms. Time per symbol = %.2f ms", len * 8, elapsed, ms_per);
        k_msleep(5000);
    }
    k_timer_stop(&bitrate_timer);
    return 0;
}
