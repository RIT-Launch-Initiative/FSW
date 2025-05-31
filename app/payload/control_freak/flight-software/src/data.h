#ifndef FREAK_GORBFS_H
#define FREAK_GORBFS_H
#include "n_autocoder_types.h"

#include <stdbool.h>
#include <stdint.h>

#define IMU_SAMPLES_PER_PACKET                                                                                         \
    (sizeof(((NTypes::SuperFastPacket*) 0)->GyroData) / sizeof(((NTypes::SuperFastPacket*) 0)->GyroData[0]))

#define SLOW_DATA_PER_PACKET 8
using SuperSlowPacket = NTypes::SlowInfo[SLOW_DATA_PER_PACKET];
#endif