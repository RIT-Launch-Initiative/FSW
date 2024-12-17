

#ifndef RFM9XW_H
#define RFM9XW_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <zephyr/device.h>

#define REG_OP_MODE_LONG_RANGE_MODE_MASK (0b10000000)
#define REG_OP_MODE_MODULATION_TYPE_MASK (0b01100000)
#define REG_OP_MODE_LOW_FREQ_MODE_MASK   (0b00001000)
#define REG_OP_MODE_TRANS_MODE_MASK      (0b00000111)

#define REG_FDEV_MSB_MAX (0b00111111)

#define REG_PACKET_CONFIG2_DATA_MODE_MASK (0b01000000)

// Frequency synthesizer step FSTEP = FXOSC/(2^19)
#define FXOSC_HZ     32000000
#define RFM_FSTEP_HZ 61.03515625

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
    RfmModulationType_FSK = 0x00000000,
    RfmModulationType_OOK = 0b00100000,
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

int32_t rfm9x_dostuff(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif