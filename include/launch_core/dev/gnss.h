#ifndef L_GNSS_H
#define L_GNSS_H

#include <zephyr/drivers/gnss.h>

#define L_GNSS_LATITUDE_DIVISION_FACTOR 1000000000.0F
#define L_GNSS_LONGITUDE_DIVISION_FACTOR 1000000000.0F
#define L_GNSS_ALTITUDE_DIVISION_FACTOR 1000.0F

void l_gnss_fix_debug_cb(const struct device *dev, const struct gnss_data *data);
void l_gnss_data_debug_cb(const struct device *dev, const struct gnss_data *data);
void l_gnss_debug_sat_count_cb(const struct device *dev, const struct gnss_satellite *satellites, uint16_t size);

#endif // L_GNSS_H
