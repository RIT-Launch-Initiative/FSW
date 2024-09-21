#ifndef OPENROCKET_MAGNETOMETER_H
#define OPENROCKET_MAGNETOMETER_H

#include "openrocket_sensors.h"

#include <stdbool.h>

struct or_magnetometer_config {
    struct or_common_params sensor_cfg;

    or_scalar_t noise;
    // enum axis vertical_axis;
    // bool vertical_axis_invert;
    // enum axis lateral_axis;

    // enum axis roll_axis;
    // enum axis pitch_axis;
    // enum axis yaw_axis;

    // bool roll_axis_invert;
    // bool pitch_axis_invert;
    // bool yaw_axis_invert;
};

struct or_magnetometer_data {
    unsigned int last_lower_index;

    or_scalar_t magn_x;
    or_scalar_t magn_y;
    or_scalar_t magn_z;
};

#endif
