#ifndef FREAK_GORBFS_H
#define FREAK_GORBFS_H
#include "n_autocoder_types.h"

#include <stdbool.h>
#include <stdint.h>

#define IMU_SAMPLES_PER_PACKET                                                                                         \
    (sizeof((NTypes::SuperFastPacket*) 0->gyro_data) / sizeof((NTypes::SuperFastPacket*) 0->gyro_data[0]))

using SuperSlowPacket = NTypes::SlowInfo[8];

#endif