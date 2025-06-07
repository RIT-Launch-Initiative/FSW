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

// If the file exists, you're good to write
// If the file doesn't exist, consider the flash locked
#define ALLOWFILE_PATH "/lfs/good_to_write"
bool is_data_locked();

// implemented by flash thread
void unlock_boostdata();
void lock_boostdata();

// implemented by horus
struct small_orientation {
    int8_t x;
    int8_t y;
    int8_t z;
};

/**
 * @param normalized orientation vector
 * will be very bad if not orientated
 */
small_orientation minify_orientation(const NTypes::AccelerometerData &normed);

int submit_horus_data(const float &tempC, const float &batteryVoltage, const small_orientation &orientation,
                      const FlightState &fs);

#endif
