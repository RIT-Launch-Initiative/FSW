#ifndef FREAK_GORBFS_H
#define FREAK_GORBFS_H

#include "n_autocoder_types.h"

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>

typedef uint8_t FlipState;
#define FLIP_STATE_NOT_TRYING 0xff

#define FLIP_STATE(current_face, attempts) (((current_face & 0b111) << 5) | (attempts > 30 ? 30 : attempts))

#define IMU_SAMPLES_PER_PACKET                                                                                         \
    (sizeof(((NTypes::SuperFastPacket *) 0)->GyroData) / sizeof(((NTypes::SuperFastPacket *) 0)->GyroData[0]))

#define SLOW_DATA_PER_PACKET 8
using SuperSlowPacket = NTypes::SlowInfo[SLOW_DATA_PER_PACKET];

/**
 * @param ina_dev ina device
 * @param[out] voltage voltage that was read or unchanged on error
 * @param[out] current current that was read or unchanged on error
 */
int read_ina(const struct device *ina_dev, float &voltage, float &current);

int set_lsm_sampling(const struct device *imu_dev, int odr);

NTypes::AccelerometerData normalize(NTypes::AccelerometerData acc);

#endif
