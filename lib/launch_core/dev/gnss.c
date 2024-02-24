#include <launch_core/dev/gnss.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(launch_gnss);

void l_gnss_fix_debug_cb(const struct device *dev, const struct gnss_data *data) {
    if (data->info.fix_status != GNSS_FIX_STATUS_NO_FIX) {
        LOG_INF("%s has fix!\r\n", dev->name);
    } else {
        LOG_INF("%s has no fix!\r\n", dev->name);
    }
}

void l_gnss_data_debug_cb(const struct device *dev, const struct gnss_data *data) {
    if (data->info.fix_status != GNSS_FIX_STATUS_NO_FIX) {
        LOG_INF("%s has fix!\r\n", dev->name);
        LOG_INF("\tCoordinates: %f, %f, %d\r\n", data->nav_data.latitude, data->nav_data.longitude, data->nav_data.altitude);
        LOG_INF("\tTime: %u-%u-%u %u:%u:%u.%u\r\n",
                data->utc.month, data->utc.month_day, data->utc.century_year,
                data->utc.hour, data->utc.minute, data->utc.millisecond / 1000, data->utc.millisecond % 1000);
        LOG_INF("\tSatellites: %u, Quality: %u\r\n", data->info.satellites_cnt, data->info.fix_quality);

    } else {
        LOG_INF("%s has no fix!\r\n", dev->name);
    }
}

// clang-format off
void l_gnss_debug_sat_count_cb(const struct device *dev, const struct gnss_satellite *satellites, uint16_t size) {
// clang-format on
    LOG_INF("%s reported %u satellites!\r\n", dev->name, size);
}
