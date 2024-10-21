#ifndef N_RADIO_MODULE_TYPES_H
#define N_RADIO_MODULE_TYPES_H

#include <stdint.h>

namespace NRadioModuleTypes {

struct RadioBroadcastData {
    uint16_t port;
    uint8_t data[256 - sizeof(uint16_t)];
    uint8_t size;
};
}

#endif