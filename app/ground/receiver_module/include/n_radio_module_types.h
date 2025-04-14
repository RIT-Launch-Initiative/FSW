#ifndef N_RADIO_MODULE_TYPES_H
#define N_RADIO_MODULE_TYPES_H

#include <stdint.h>
#include <f_core/utils/n_gnss_utils.h>

namespace NTypes {

struct LoRaBroadcastData {
    uint16_t port;
    uint8_t size;
    uint8_t data[256 - sizeof(uint16_t)];
};

struct __attribute__((packed)) GnssData {
    uint32_t systemTime;
    NGnssUtils::GnssData gnssData;
};

struct __attribute__((packed)) GnssBroadcastData {
    NGnssUtils::GnssCoordinates coordinates;
    uint8_t updated;
};

}

#endif