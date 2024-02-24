#ifndef L_GNSS_H
#define L_GNSS_H

#include <zephyr/drivers/gnss.h>

void gnss_debug_fix_cb(const struct device *dev, const struct gnss_data *data);

static void gnss_debug_satellites_count_cb(const struct device *dev, const struct gnss_satellite *satellites, uint16_t size);

#endif // L_GNSS_H
