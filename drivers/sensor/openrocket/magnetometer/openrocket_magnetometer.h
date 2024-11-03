#ifndef OPENROCKET_MAGNETOMETER_H
#define OPENROCKET_MAGNETOMETER_H

#include "openrocket_sensors.h"

#include <stdbool.h>

struct or_magnetometer_config {
    struct or_common_params sensor_cfg;

    or_scalar_t noise;
};

struct or_magnetometer_data {
    unsigned int last_lower_index;
    uint32_t rand_state;

    or_scalar_t magn_x;
    or_scalar_t magn_y;
    or_scalar_t magn_z;
};

#endif
