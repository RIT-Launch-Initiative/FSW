#pragma once
#include "n_autocoder_types.h"

#include <stdbool.h>
#include <stdint.h>

#define IMU_SAMPLES_PER_PACKET \
    (sizeof(((NTypes::SuperFastPacket*) 0)->GyroData) / sizeof(((NTypes::SuperFastPacket*) 0)->GyroData[0]))

using SuperSlowPacket = NTypes::SlowInfo[8];
