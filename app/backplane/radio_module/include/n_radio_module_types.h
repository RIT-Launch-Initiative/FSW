#ifndef N_RADIO_MODULE_TYPES_H
#define N_RADIO_MODULE_TYPES_H

#include <stdint.h>
#include <f_core/utils/n_gnss_utils.h>

namespace NRadioModuleTypes {

struct __attribute__((packed)) RadioBroadcastData {
    uint16_t port;
    uint8_t data[256 - sizeof(uint16_t)];
    uint8_t size;
};

struct __attribute__((packed)) GnssBroadcastData {
    NGnssUtils::GnssCoordinates coordinates;
    uint8_t updated;
};

}

#endif