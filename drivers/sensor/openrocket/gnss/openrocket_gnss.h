#ifndef OPENROCKET_GNSS_H
#define OPENROCKET_GNSS_H
#include "openrocket_sensors.h"

struct or_gnss_cfg {
    bool broken;
    unsigned int sampling_period_us;
    unsigned int lag_time_ms;
};
struct or_gnss_data {
    unsigned int last_lower_index;
    or_scalar_t latitude;
    or_scalar_t longitude;

    or_scalar_t velocity;
    or_scalar_t altitude;
    or_scalar_t bearing;
};

#endif
