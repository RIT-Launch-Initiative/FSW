#ifndef OPENROCKET_IMU_H
#define OPENROCKET_IMU_H

#include "openrocket_sensors.h"

#include <stdbool.h>

struct or_imu_config {
    struct or_common_params sensor_cfg;
    enum axis vertical_axis;
    bool vertical_axis_invert;
    enum axis lateral_axis;

    enum axis roll_axis;
    enum axis pitch_axis;
    enum axis yaw_axis;

    bool roll_axis_invert;
    bool pitch_axis_invert;
    bool yaw_axis_invert;

    or_scalar_t accel_noise;
    or_scalar_t gyro_noise;
};

struct or_imu_data {
    unsigned int last_lower_index;
    uint32_t rand_state;

    or_scalar_t accel_x;
    or_scalar_t accel_y;
    or_scalar_t accel_z;

    or_scalar_t gyro_x;
    or_scalar_t gyro_y;
    or_scalar_t gyro_z;
};

#endif
