#ifndef OPENROCKET_BAROM_H
#define OPENROCKET_BAROM_H
#include "openrocket_sensors.h"

struct or_barom_config {
    struct or_common_params sensor_cfg;
    or_scalar_t temp_noise_scale;
    or_scalar_t press_noise_scale;
};

struct or_barom_data {
    unsigned int last_lower_index;
    uint32_t rand_state;

    or_scalar_t pressure;
    or_scalar_t temperature;
};
#endif
