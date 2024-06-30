#ifndef OPENROCKET_IMU_H
#define OPENROCKET_IMU_H

#include "openrocket_sensors.h"

#include <stdbool.h>
enum axis {
    X,
    Y,
    Z,
};
struct or_imu_config {
    bool broken;
    unsigned int sampling_period_us;
    unsigned int lag_time_ms;
    enum axis vertical_axis;
    bool vertical_axis_invert;
    enum axis lateral_axis;
};
struct or_imu_data {
    unsigned int last_lower_index;

    or_scalar_t ax;
    or_scalar_t ay;
    or_scalar_t az;

    or_scalar_t gx;
    or_scalar_t gy;
    or_scalar_t gz;
};

#endif