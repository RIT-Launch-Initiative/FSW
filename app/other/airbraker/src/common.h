#ifndef FREAK_GORBFS_H
#define FREAK_GORBFS_H

#include "n_autocoder_types.h"

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>

enum class FlightState : uint8_t {
    NotSet = 0,
    OnPad = 1,
    Boost = 2,
    Flight = 3,
    InitialRoll = 4,
    InitialPump = 5,
    Continuous = 6,

};

enum DataLockMsg {
    Lock,
    Unlock,
};



#define IMU_SAMPLES_PER_PACKET                                                                                         \
    (sizeof(((NTypes::SuperFastPacket *) 0)->GyroData) / sizeof(((NTypes::SuperFastPacket *) 0)->GyroData[0]))

/**
 * @param ina_dev ina device
 * @param[out] voltage voltage that was read or unchanged on error
 * @param[out] current current that was read or unchanged on error
 */
int read_ina(const struct device *ina_dev, float &voltage, float &current);

int set_lsm_sampling(const struct device *imu_dev, int odr);

NTypes::AccelerometerData normalize(NTypes::AccelerometerData acc);

// If the file exists, you're good to write
// If the file doesn't exist, consider the flash locked
#define ALLOWFILE_PATH "/lfs/good_to_write"
bool is_data_locked();

// implemented by flash thread
void unlock_boostdata();
void lock_boostdata();


#endif
