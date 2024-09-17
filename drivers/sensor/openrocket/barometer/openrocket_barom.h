#ifndef OPENROCKET_BAROM_H
#define OPENROCKET_BAROM_H
#include "openrocket_sensors.h"

struct or_barom_config {
    struct or_common_params sensor_cfg;
};

struct or_barom_data {
    unsigned int last_lower_index;

    or_scalar_t pressure;
    or_scalar_t temperature;
};
#endif
