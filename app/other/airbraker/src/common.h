#ifndef FREAK_GORBFS_H
#define FREAK_GORBFS_H

#include "n_autocoder_types.h"

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>

#define FAST_PACKET_ITEMS_PER_PACKET (sizeof(NTypes::FastPacket) / sizeof(NTypes::FastPacketItem))

enum class FlightState : uint8_t {
    NotSet = 0,
    OnPad = 1,
    Boost = 2,
    Flight = 3,
    Ground = 4,

};

enum DataLockMsg {
    Lock,
    Unlock,
};


int set_lsm_sampling(const struct device *imu_dev, int odr);

/**
 * @param ina_dev ina device
 * @param[out] voltage voltage that was read or unchanged on error
 * @param[out] current current that was read or unchanged on error
 */
int read_ina(const struct device *ina_dev, float &voltage, float &current);
int read_barom(const struct device *barom_dev, float &temp, float &press);
int read_imu_up(const struct device *imu_dev, float &vert_axis);



#endif
