#pragma once
#include "n_autocoder_types.h"

#include <stdbool.h>
#include <stdint.h>

    (sizeof(((NTypes::SuperFastPacket*) 0)->GyroData) / sizeof(((NTypes::SuperFastPacket*) 0)->GyroData[0]))

using SuperSlowPacket = NTypes::SlowInfo[8];
