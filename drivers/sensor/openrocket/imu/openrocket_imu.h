#ifndef OPENROCKET_IMU_H
#define OPENROCKET_IMU_H

#include <stdbool.h>

struct or_imu_config {
    bool broken;
    unsigned int sampling_period_us;
    unsigned int lag_time_ms;
};
struct or_imu_data {
    unsigned int last_lower_index;
};

#endif