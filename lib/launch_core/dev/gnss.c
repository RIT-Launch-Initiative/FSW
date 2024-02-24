#include "gnss.h"

void gnss_debug_fix_cb(const struct device *dev, const struct gnss_data *data) {
    if (data->info.fix_status != GNSS_FIX_STATUS_NO_FIX) {
        LOG_INF("%s has fix!\r\n", dev->name);
    } else {
        LOG_INF("%s has no fix!\r\n", dev->name);
    }
}

static void gnss_debug_satellites_cb(const struct device *dev, const struct gnss_satellite *satellites, uint16_t size) {
    LOG_INF("%s reported %u satellites!\r\n", dev->name, size);
}