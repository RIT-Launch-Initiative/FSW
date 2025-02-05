#ifndef CONFIG_HORUSV2
#warning "horus encoding disabed. Enable CONFIG_HORUSV2 to use horus featurees"
#else
// https://github.com/mikaelnousiainen/RS41ng/tree/main

#include <stdint.h>

// Horus Binary v2 Packet Format
// See: https://github.com/projecthorus/horusdemodlib/wiki/5-Customising-a-Horus-Binary-v2-Packet
// Note that we need to pack this to 1-byte alignment, hence the #pragma flags below
// Refer: https://gcc.gnu.org/onlinedocs/gcc-4.4.4/gcc/Structure_002dPacking-Pragmas.html
#pragma pack(push, 1)
typedef struct horus_packet_v2 {
    uint16_t PayloadID; // Payload ID (0-65535)
    uint16_t Counter; // Sequence number
    uint8_t Hours; // Time of day, Hours
    uint8_t Minutes; // Time of day, Minutes
    uint8_t Seconds; // Time of day, Seconds
    float Latitude; // Latitude in degrees
    float Longitude; // Longitude in degrees
    uint16_t Altitude; // Altitude in meters
    uint8_t Speed; // Speed in km/h
    uint8_t Sats; // Number of GPS satellites visible
    int8_t Temp; // Temperature in Celsius, as a signed value (-128 to +128, though sensor limited to -64 to +64 deg C)
    uint8_t BattVoltage; // 0 = 0v, 255 = 5.0V, linear steps in-between.
    uint8_t CustomData[9]; // Custom data, see: https://github.com/projecthorus/horusdemodlib/wiki/5-Customising-a-Horus-Binary-v2-Packet#interpreting-the-custom-data-section
    uint16_t Checksum; // CRC16-CCITT Checksum.
} horus_packet_v2;  //  __attribute__ ((packed)); // Doesn't work?
#pragma pack(pop)

#define HORUS_PACKET_SIZE sizeof(struct horus_packet_v2)
// As calculated by horus_l2_get_num_tx_data_bytes(sizeof(horus_packet_v2))
#define HORUS_ENCODED_BUFFER_SIZE 65

#endif