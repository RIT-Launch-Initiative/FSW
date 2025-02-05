#ifdef __cplusplus
extern "C" {
#endif
// https://github.com/mikaelnousiainen/RS41ng/tree/main

#include <stdint.h>
#include <stdbool.h>

// Horus Binary v2 Packet Format
// See: https://github.com/projecthorus/horusdemodlib/wiki/5-Customising-a-Horus-Binary-v2-Packet
// Note that we need to pack this to 1-byte alignment, hence the #pragma flags below
// Refer: https://gcc.gnu.org/onlinedocs/gcc-4.4.4/gcc/Structure_002dPacking-Pragmas.html
#pragma pack(push, 1)
struct horus_packet_v2 {
    uint16_t payload_id; // Payload ID (0-65535)
    uint16_t counter; // Sequence number
    uint8_t hours; // Time of day, Hours
    uint8_t minutes; // Time of day, Minutes
    uint8_t seconds; // Time of day, Seconds
    float latitude; // Latitude in degrees
    float longitude; // Longitude in degrees
    uint16_t altitude; // Altitude in meters
    uint8_t speed; // Speed in km/h
    uint8_t sats; // Number of GPS satellites visible
    int8_t temp; // Temperature in Celsius, as a signed value (-128 to +128, though sensor limited to -64 to +64 deg C)
    uint8_t battery_voltage; // 0 = 0v, 255 = 5.0V, linear steps in-between.
    uint8_t custom_data[9]; // Custom data, see: https://github.com/projecthorus/horusdemodlib/wiki/5-Customising-a-Horus-Binary-v2-Packet#interpreting-the-custom-data-section
    uint16_t checksum; // CRC16-CCITT Checksum.
};
#pragma pack(pop)

#define HORUS_PACKET_SIZE sizeof(struct horus_packet_v2)
// As calculated by horus_l2_get_num_tx_data_bytes(sizeof(horus_packet_v2))
#define HORUS_ENCODED_BUFFER_SIZE 65


typedef uint8_t horus_packet_v2_encoded_buffer_t[HORUS_ENCODED_BUFFER_SIZE];

/**
 * Fills in crc
 */
int horusv2_encode(struct horus_packet_v2 *input_packet, horus_packet_v2_encoded_buffer_t*output_buffer);

#ifdef CONFIG_HORUSV2_RX
int horusv2_decode(horus_packet_v2_encoded_buffer_t*input_buffer, struct horus_packet_v2 *output_packet);
bool horusv2_checksum_verify(const struct horus_packet_v2 *input_packet);
#endif



#ifdef __cplusplus
}
#endif