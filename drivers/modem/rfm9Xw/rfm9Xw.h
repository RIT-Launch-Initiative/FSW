

#ifndef RFM9XW_H
#define RFM9XW_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <zephyr/device.h>

#define RFM_NUM_DIOS 6

#define REG_OP_MODE_LONG_RANGE_MODE_MASK (0b10000000)
#define REG_OP_MODE_MODULATION_TYPE_MASK (0b01100000)
#define REG_OP_MODE_LOW_FREQ_MODE_MASK   (0b00001000)
#define REG_OP_MODE_TRANS_MODE_MASK      (0b00000111)

#define REG_FDEV_MSB_MAX (0b00111111)

#define REG_PACKET_CONFIG2_DATA_MODE_MASK (0b01000000)

// Frequency synthesizer step FSTEP = FXOSC/(2^19)
#define FXOSC_HZ                       32000000
#define RFM_FSTEP_HZ                   61.03515625
#define RFM_MAX_FREQUENCY_DEVIATION_HZ 999879

#define RFM_BIT_RATE_FSK_BPS_MIN 1200
#define RFM_BIT_RATE_FSK_BPS_MAX 300000
#define RFM_BIT_RATE_OOK_BPS_MIN 1200
#define RFM_BIT_RATE_OOK_BPS_MAX 32768

#define RFM_FREQUENCY_DEVIATION_MIN 600
#define RFM_FREQUENCY_DEVIATION_MAX 200000

#define RFM99_SYNTH_MIN_HZ 137000000
#define RFM99_SYNTH_MAX_HZ 175000000

#define RFM98_SYNTH_MIN_HZ 410000000
#define RFM98_SYNTH_MAX_HZ 525000000

// // Same as 98W
#define RFM96_SYNTH_MIN_HZ RFM98_SYNTH_MIN_HZ
#define RFM96_SYNTH_MAX_HZ RFM98_SYNTH_MAX_HZ

#define RFM95_SYNTH_MIN_HZ 862000000
#define RFM95_SYNTH_MAX_HZ 1020000000

enum RfmModelNumber {
    RfmModelNumber_95W,
    RfmModelNumber_96W,
    RfmModelNumber_98W,
    RfmModelNumber_99W,
};

enum RfmLongRangeModeSetting {
    // Bit 7 of RegOpMode
    RfmLongRangeModeSetting_FskOokMode = 0b00000000,
    RfmLongRangeModeSetting_LoraTmMode = 0b10000000,
};

enum RfmModulationType {
    // Bits 5-6 of RegOpMode
    // Built in Modulation Types (register values)
    RfmModulationType_FSK = 0b00000000,
    RfmModulationType_OOK = 0b00100000,
    //
    // Custom Modulation Types (not actual register values)
    // Values assigned to not clash with actual register values
    RfmModulationType_FSK_BitBangFSK = 0x01,
    RfmModulationType_OOK_BitBang4FSK = 0x02,
};

enum RfmLowFrequencyMode {
    // Bit 3 of RegOpMode
    RfmLowFrequencyMode_HighFrequency = 0b0000,
    RfmLowFrequencyMode_LowFrequency = 0b1000,
};
enum RfmTransceiverMode {
    // Bits 0-2 of RegOpMode
    RfmTransceiverMode_Sleep = 0b000,
    RfmTransceiverMode_Standby = 0b001,
    RfmTransceiverMode_FsModeTx = 0b010,    //(FSTx)
    RfmTransceiverMode_Transmitter = 0b011, //(Tx)
    RfmTransceiverMode_FsModeRx = 0b100,    //(FSRx)
    RfmTransceiverMode_Receiver = 0b101,    //(Rx)
};

enum RfmPacketConfigDataMode {
    // Bit 6 of RegPacketConfig2
    RfmPacketConfigDataMode_Continuous = 0b00000000,
    RfmPacketConfigDataMode_Packet = 0b01000000, // Default
};
#define RFM_PA_CONFIG_MASK_PA_SELECT    0x80
#define RFM_PA_CONFIG_MASK_MAX_POWER    0x70
#define RFM_PA_CONFIG_MASK_OUTPUT_POWER 0x0f;
#define RFM_MAX_OUTPUT_POWER            0x0f
enum RfmPowerAmplifierSelection {
    // bit 7 of RegPaConfig
    RfmPowerAmplifierSelection_RFO = 0b00000000,
    RfmPowerAmplifierSelection_PaBoost = 0b10000000,
};
enum RfmMaxPower {
    // bit 5-6 of RegPaConfig
    // Controls Max Power when using the RFO pins for RF output
    RfmMaxPower_10_8_DBM = 0x00,
    RfmMaxPower_11_4_DBM = 0x10,
    RfmMaxPower_12_0_DBM = 0x20,
    RfmMaxPower_12_6_DBM = 0x30,
    RfmMaxPower_13_2_DBM = 0x40, // Default
    RfmMaxPower_13_8_DBM = 0x50,
    RfmMaxPower_14_4_DBM = 0x60,
    RfmMaxPower_15_0_DBM = 0x70,

};

#define RFM_REG_PA_RAMP_MASK_MODULATION_SHAPING 0b01100000
enum RfmModulationShaping {
    RfmModulationShaping_FSK_NoShaping = 0b00000000, // Default
    RfmModulationShaping_FSK_GaussianBT_1_0 = 0b00100000,
    RfmModulationShaping_FSK_GaussianBT_0_5 = 0b01000000,
    RfmModulationShaping_FSK_GaussianBT_0_3 = 0b01100000,

    RfmModulationShaping_OOK_NoShaping = 0b00000000,
    RfmModulationShaping_OOK_FCutoffBitRate = 0b00100000,
    RfmModulationShaping_OOK_FCutoff2xBitRate = 0b01000000,
};

#define RFM_REG_PA_RAMP_MASK_PA_RAMP 0b00001111
enum RfmPaRamp {
    RfmPaRamp_3400us = 0b0000,
    RfmPaRamp_2000us = 0b0001,
    RfmPaRamp_1000us = 0b0010,
    RfmPaRamp_500us = 0b0011,
    RfmPaRamp_250us = 0b0100,
    RfmPaRamp_125us = 0b0101,
    RfmPaRamp_100us = 0b0110,
    RfmPaRamp_62us = 0b0111,
    RfmPaRamp_50us = 0b1000,
    RfmPaRamp_40us = 0b1001, // Default
    RfmPaRamp_31us = 0b1010,
    RfmPaRamp_25us = 0b1011,
    RfmPaRamp_20us = 0b1100,
    RfmPaRamp_15us = 0b1101,
    RfmPaRamp_12us = 0b1110,
    RfmPaRamp_10us = 0b1111,
};

#define RFM_REG_DIO_MAPPING1_MASK_DIO0 0b11000000
enum RfmDio0Mapping {
    //Datasheet: Page 65, Table 28,29
    RfmDio0Mapping_Continuous_SyncAddressTxReady = 0x00,
    RfmDio0Mapping_Continuous_RssiPreambleDetect = 0x1,
    RfmDio0Mapping_Continuous_RxReadyTxReady = 0x2,
    RfmDio0Mapping_Continuous_Nothing = 0x3,

    RfmDio0Mapping_Packet_PayloadReadyPacketSent = 0x00,
    RfmDio0Mapping_Packet_RxCrcOk = 0x1,
    RfmDio0Mapping_Packet_Nothing = 0x2,
    RfmDio0Mapping_Packet_TempChangeLowBat = 0x3,
};

#define RFM_REG_DIO_MAPPING1_MASK_DIO1 0b00110000
enum RfmDio1Mapping {
    //Datasheet: Page 65, Table 28,29
    RfmDio1Mapping_Continuous_Dclk = 0x00,
    RfmDio1Mapping_Continuous_RssiPreambleDetect = 0x1,
    RfmDio1Mapping_Continuous_Nothing = 0x2, // 0x03 is also nothing

    RfmDio1Mapping_Packet_FifoLevel = 0x00,
    RfmDio1Mapping_Packet_FifoEmpty = 0x1,
    RfmDio1Mapping_Packet_Nothing = 0x2,
    RfmDio1Mapping_Packet_TempChangeLowBat = 0x3,
};

#define RFM_REG_DIO_MAPPING1_MASK_DIO2 0b00001100
enum RfmDio2Mapping { RfmDio2Mapping_IDK };

#define RFM_REG_DIO_MAPPING1_MASK_DIO3 0b00000011
enum RfmDio3Mapping { RfmDio3Mapping_IDK };

#define RFM_REG_DIO_MAPPING2_MASK_DIO4 0b11000000
enum RfmDio4Mapping { RfmDio4Mapping_IDK };
#define RFM_REG_DIO_MAPPING2_MASK_DIO5 0b00110000
enum RfmDio5Mapping { RfmDio5Mapping_IDK };

#define RFM_REG_DIO_MAPPING2_MASK_MAP_PREAMBLE_DETECT 0b00000001
enum RfmMapPreambleDetectInterrupt {
    RfmMapPreambleDetectInterrupt_RSSI = 0x0,
    RfmMapPreambleDetectInterrupt_PreambleDetect = 0x1,
};

enum RfmDcFreeEncodingType {
    RfmDcFreeEncodingType_None,
    RfmDcFreeEncodingType_Manchester,
    RfmDcFreeEncodingType_Whitening,
};

/**
 * Read temperature in C
 * <0 return is error
 */
int32_t rfm9xw_read_temperature(const struct device *dev, int8_t *celsius);

int32_t rfm9xw_software_reset(const struct device *dev);

// Honk mimimimi (TODO add detail about power details when in standby
int32_t rfm9xw_sleep(const struct device *dev);
// Stop transmitting (TODO add detail about power details when in standby)
int32_t rfm9xw_standby(const struct device *dev);

// FSK, OOK, BitBangFSK,
struct RfmFSKModulationSettings {
    uint32_t frequency;
    uint32_t frequency_deviation;
    uint32_t bitrate;
    // Power settings
    enum RfmModulationShaping modulation_shaping;
    enum RfmPaRamp ramp;
};

struct RfmOOKModulationSettings {
    // TODO
    uint32_t frequency;
    uint32_t bitrate;
    enum RfmModulationShaping modulation_shaping;
    enum RfmPaRamp ramp;
};
struct RfmBitBangFSKModulationSettings {
    uint32_t frequency;
    uint32_t frequency_deviation;
    uint32_t bitrate;
};
struct RfmBitBang4FSKModulationSettings {
    uint32_t frequency;
    uint32_t frequency_deviation;
    uint32_t bitrate;
};

int32_t rfm9xw_setup_transmission_fsk(const struct device *dev, const struct RfmFSKModulationSettings *settings);
int32_t rfm9xw_setup_transmission_ook(const struct device *dev, const struct RfmOOKModulationSettings *settings);
int32_t rfm9xw_setup_transmission_bitbang_fsk(const struct device *dev,
                                              const struct RfmBitBangFSKModulationSettings *settings);
int32_t rfm9xw_setup_transmission_bitbang_4fsk(const struct device *dev,
                                               const struct RfmBitBang4FSKModulationSettings *settings);
int32_t rfm9xw_set_power_settings();

// maybe don't use these unless you know what you're doing.
int32_t rfm9xw_set_dio0_mapping(const struct device *dev, enum RfmDio0Mapping);
int32_t rfm9xw_set_dio1_mapping(const struct device *dev, enum RfmDio0Mapping);
int32_t rfm9xw_set_dio2_mapping(const struct device *dev, enum RfmDio0Mapping);
int32_t rfm9xw_set_dio3_mapping(const struct device *dev, enum RfmDio0Mapping);
int32_t rfm9xw_set_dio4_mapping(const struct device *dev, enum RfmDio0Mapping);
int32_t rfm9xw_set_dio5_mapping(const struct device *dev, enum RfmDio0Mapping);

/**
 * Send a packet over the radio using the transmission type assigned with rfm9xw_setup_transmission_* types
 * IS BLOCKING!!!!!!
 */
int32_t rfm9xw_send_packet(const struct device *, uint8_t *packet_data, int32_t packet_len);

#ifdef __cplusplus
}
#endif

#endif