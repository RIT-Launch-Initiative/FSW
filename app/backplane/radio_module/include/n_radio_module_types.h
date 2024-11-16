#ifndef N_RADIO_MODULE_TYPES_H
#define N_RADIO_MODULE_TYPES_H

#include <stdint.h>
#include <f_core/utils/n_gnss_utils.h>

namespace NRadioModuleTypes {

__attribute__((packed)) struct RadioBroadcastData {
    uint16_t port;
    uint8_t data[256 - sizeof(uint16_t)];
    uint8_t size;
};

__attribute__((packed)) struct GnssBroadcastData {
    NGnssUtils::GnssCoordinates coordinates;
    uint8_t updated;
};

}

#endif