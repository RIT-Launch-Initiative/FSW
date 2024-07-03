#include "openrocket_gnss.h"

#include <math.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gnss/gnss_publish.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(openrocket_gnss, CONFIG_GNSS_LOG_LEVEL);

#define DT_DRV_COMPAT openrocket_gnss

extern const unsigned int or_packets_size;
extern const struct or_data_t *or_packets;

static int or_gnss_init(const struct device *dev) {
    const struct or_gnss_cfg *cfg = dev->config;
    if (cfg->broken) {
        LOG_WRN("GNSS device %s is failed to init", dev->name);
        return -ENODEV;
    }
    struct or_gnss_data *data = dev->data;
    data->last_lower_index = 0;

    return 0;
}
static void or_gnss_thread_fn(void *dev_v, void *, void *) {
    const struct device *dev = dev_v;
    const struct or_gnss_cfg *cfg = dev->config;
    struct or_gnss_data *data = dev->data;
    LOG_INF("Starting openrocket GNSS thread");
    while (true) {
        k_usleep(cfg->sampling_period_us);
        or_scalar_t time = or_get_time(cfg->sampling_period_us, cfg->lag_time_ms);
        unsigned int lo, hi = 0;
        or_scalar_t mix = 0;
        or_find_bounding_packets(data->last_lower_index, time, &lo, &hi, &mix);

        struct or_data_t or_data;
        if (hi == 0) {
            // Before sim
            or_get_presim(&or_data);
        } else if (lo == or_packets_size) {
            // After sim
            or_get_postsim(&or_data);
        } else {
            // In the middle
            const struct or_data_t *lo_data = &or_packets[lo];
            const struct or_data_t *hi_data = &or_packets[hi];
            or_data.latitude = or_lerp(lo_data->latitude, hi_data->latitude, mix);
            or_data.longitude = or_lerp(lo_data->longitude, hi_data->longitude, mix);

            or_data.altitude = or_lerp(lo_data->altitude, hi_data->altitude, mix);
            or_data.velocity = or_lerp(lo_data->velocity, hi_data->velocity, mix);
            or_data.bearing = or_lerp(lo_data->bearing, hi_data->bearing, mix);
        }

        // See https://docs.zephyrproject.org/latest/hardware/peripherals/gnss.html#c.navigation_data
        // and openrocket_sensors.h for an explanation of units

        struct gnss_data gnss_data = {
            .nav_data =
                {
                    .latitude = or_data.latitude * 1e9,
                    .longitude = or_data.longitude * 1e9,
                    .bearing = (fmod(or_data.bearing, 360.0)) * 1e3,
                    .speed = or_data.velocity * 1000,
                    .altitude = or_data.altitude * 1000,
                },
            .info =
                {
                    .fix_quality = GNSS_FIX_QUALITY_RTK,
                    .fix_status = GNSS_FIX_STATUS_GNSS_FIX,
                    .satellites_cnt = 6,
                    .hdop = 2, // https://en.wikipedia.org/wiki/Dilution_of_precision_(navigation)
                },
            .utc = 0,
        };
        gnss_publish_data(dev, &gnss_data);
    }
}

static struct gnss_driver_api gnss_api = {};

#define GNSS_NMEA_GENERIC(inst)                                                                                        \
    static struct or_gnss_cfg or_gnss_cfg_##inst = {                                                                   \
        .broken = DT_INST_PROP(inst, broken),                                                                          \
        .sampling_period_us = DT_INST_PROP(inst, sampling_period_us),                                                  \
        .lag_time_ms = DT_INST_PROP(inst, lag_time_us),                                                                \
    };                                                                                                                 \
                                                                                                                       \
    static struct or_gnss_data or_gnss_data_##inst;                                                                    \
                                                                                                                       \
    DEVICE_DT_INST_DEFINE(inst, or_gnss_init, NULL, &or_gnss_data_##inst, &or_gnss_cfg_##inst, POST_KERNEL,            \
                          CONFIG_GNSS_INIT_PRIORITY, &gnss_api);                                                       \
    K_THREAD_DEFINE(or_gnss_thread##inst, 512, or_gnss_thread_fn, DEVICE_DT_INST_GET(inst), NULL, NULL, 0, 0, 0);

DT_INST_FOREACH_STATUS_OKAY(GNSS_NMEA_GENERIC)
