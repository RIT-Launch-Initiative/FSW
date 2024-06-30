#include "openrocket_sensors.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(openrocket, CONFIG_OPENROCKET_LOG_LEVEL);

int or_data_interpolator() {
    LOG_INF("getting size");
    return sizeof(struct or_data_t);
}