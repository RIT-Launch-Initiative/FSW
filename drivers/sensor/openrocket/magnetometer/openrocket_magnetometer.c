#include "openrocket_magnetometer.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT openrocket_magnetometer

LOG_MODULE_REGISTER(openrocket_magnetometer, CONFIG_SENSOR_LOG_LEVEL);

extern const unsigned int or_packets_size;
extern const struct or_data_t *or_packets;

// Map openrocket values to sensor axes
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEGREES_TO_RADIANS (M_PI / 180.0)
// static or_scalar_t map_ax(or_scalar_t vert, or_scalar_t lat, const struct or_imu_config *cfg) {
//     if (cfg->vertical_axis == OPENROCKET_AXIS_X) {
//         if (cfg->vertical_axis_invert) {
//             return vert;
//         } else {
//             return -vert;
//         }
//     } else if (cfg->lateral_axis == OPENROCKET_AXIS_X) {
//         return lat;
//     } else {
//         return 0;
//     }
// }
// static or_scalar_t map_ay(or_scalar_t vert, or_scalar_t lat, const struct or_imu_config *cfg) {
//     if (cfg->vertical_axis == OPENROCKET_AXIS_Y) {
//         if (cfg->vertical_axis_invert) {
//             return vert;
//         } else {
//             return -vert;
//         }
//     } else if (cfg->lateral_axis == OPENROCKET_AXIS_Y) {
//         return lat;
//     } else {
//         return 0;
//     }
// }
// static or_scalar_t map_az(or_scalar_t vert, or_scalar_t lat, const struct or_imu_config *cfg) {
//     if (cfg->vertical_axis == OPENROCKET_AXIS_Z) {
//         if (cfg->vertical_axis_invert) {
//             return vert;
//         } else {
//             return -vert;
//         }
//     } else if (cfg->lateral_axis == OPENROCKET_AXIS_Z) {
//         return lat;
//     } else {
//         return 0;
//     }
// }

// #define INVERT(val, do_invert) (do_invert ? -val : val)

// static or_scalar_t map_gx(or_scalar_t roll, or_scalar_t pitch, or_scalar_t yaw, const struct or_imu_config *cfg) {
//     if (cfg->roll_axis == OPENROCKET_AXIS_X) {
//         return INVERT(roll, cfg->roll_axis_invert);
//     } else if (cfg->pitch_axis == OPENROCKET_AXIS_X) {
//         return INVERT(pitch, cfg->pitch_axis_invert);
//     } else if (cfg->yaw_axis == OPENROCKET_AXIS_X) {
//         return INVERT(yaw, cfg->pitch_axis_invert);
//     } else {
//         return 0;
//     }
// }
// static or_scalar_t map_gy(or_scalar_t roll, or_scalar_t pitch, or_scalar_t yaw, const struct or_imu_config *cfg) {
//     if (cfg->roll_axis == OPENROCKET_AXIS_Y) {
//         return INVERT(roll, cfg->roll_axis_invert);
//     } else if (cfg->pitch_axis == OPENROCKET_AXIS_Y) {
//         return INVERT(pitch, cfg->pitch_axis_invert);
//     } else if (cfg->yaw_axis == OPENROCKET_AXIS_Y) {
//         return INVERT(yaw, cfg->pitch_axis_invert);
//     } else {
//         return 0;
//     }
// }

// static or_scalar_t map_gz(or_scalar_t roll, or_scalar_t pitch, or_scalar_t yaw, const struct or_imu_config *cfg) {
//     if (cfg->roll_axis == OPENROCKET_AXIS_Z) {
//         return INVERT(roll, cfg->roll_axis_invert);
//     } else if (cfg->pitch_axis == OPENROCKET_AXIS_Z) {
//         return INVERT(pitch, cfg->pitch_axis_invert);
//     } else if (cfg->yaw_axis == OPENROCKET_AXIS_Z) {
//         return INVERT(yaw, cfg->pitch_axis_invert);
//     } else {
//         return 0;
//     }
// }

// static void map_or_to_sensor(struct or_data_t *in, struct or_imu_data *out, const struct or_imu_config *cfg) {
//     out->accel_x = map_ax(in->vert_accel, in->lat_accel, cfg);
//     out->accel_y = map_ay(in->vert_accel, in->lat_accel, cfg);
//     out->accel_z = map_az(in->vert_accel, in->lat_accel, cfg);

//     out->gyro_x = map_gx(in->roll, in->pitch, in->yaw, cfg);
//     out->gyro_y = map_gy(in->roll, in->pitch, in->yaw, cfg);
//     out->gyro_z = map_gz(in->roll, in->pitch, in->yaw, cfg);
// }

static int or_magn_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    const struct or_magnetometer_config *cfg = dev->config;
    k_usleep(cfg->sensor_cfg.measurement_us);
    struct or_magnetometer_data *data = dev->data;
    if (cfg->sensor_cfg.broken) {
        return -ENODEV;
    }
    if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_MAGN_X && chan != SENSOR_CHAN_MAGN_Y &&
        chan != SENSOR_CHAN_MAGN_Z && chan != SENSOR_CHAN_MAGN_XYZ) {
        return -ENOTSUP;
    }
    or_scalar_t time = or_get_time(&cfg->sensor_cfg);
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

        // or_data.vert_accel = or_lerp(lo_data->vert_accel, hi_data->vert_accel, mix);
        // or_data.lat_accel = or_lerp(lo_data->lat_accel, hi_data->lat_accel, mix);

        // or_data.roll = or_lerp(lo_data->roll, hi_data->roll, mix);
        // or_data.pitch = or_lerp(lo_data->pitch, hi_data->pitch, mix);
        // or_data.yaw = or_lerp(lo_data->yaw, hi_data->yaw, mix);
    }
    // or_data.roll *= DEGREES_TO_RADIANS;
    // or_data.pitch *= DEGREES_TO_RADIANS;
    // or_data.yaw *= DEGREES_TO_RADIANS;
    // map_or_to_sensor(&or_data, data, cfg);

    return 0;
}

static int or_magn_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val) {
    const struct or_magnetometer_config *cfg = dev->config;
    struct or_magnetometer_data *data = dev->data;

    if (cfg->sensor_cfg.broken) {
        return -ENODEV;
    }

    switch (chan) {
        case SENSOR_CHAN_MAGN_X:
            // sensor_value_from_or_scalar(val, data->accel_x);
            break;
        case SENSOR_CHAN_MAGN_Y:
            // sensor_value_from_or_scalar(val, data->accel_y);
            break;
        case SENSOR_CHAN_MAGN_Z:
            // sensor_value_from_or_scalar(val, data->accel_z);
            break;
        case SENSOR_CHAN_MAGN_XYZ:
            // sensor_value_from_or_scalar(&val[0], data->accel_x);
            // sensor_value_from_or_scalar(&val[1], data->accel_y);
            // sensor_value_from_or_scalar(&val[2], data->accel_z);
            break;
        default:
            LOG_DBG("Channel not supported by device");
            return -ENOTSUP;
    }
    return 0;
}

static int or_magn_init(const struct device *dev) {
    const struct or_magnetometer_config *cfg = dev->config;
    if (cfg->sensor_cfg.broken) {
        LOG_WRN("Magnetometer device %s is failed to init", dev->name);
        return -ENODEV;
    }
    struct or_magnetometer_data *data = dev->data;
    data->last_lower_index = 0;
    return 0;
}

static const struct sensor_driver_api or_magn_api = {
    .sample_fetch = or_magn_sample_fetch,
    .channel_get = or_magn_channel_get,
};

#define OR_MAGN_INIT(n)                                                                                                \
    static struct or_magnetometer_data or_magn_data_##n;                                                               \
                                                                                                                       \
    static const struct or_magnetometer_config or_magn_config_##n = {                                                  \
        .sensor_cfg =                                                                                                  \
            {                                                                                                          \
                .broken = DT_INST_PROP(n, broken),                                                                     \
                .sampling_period_us = DT_INST_PROP(n, sampling_period_us),                                             \
                .lag_time_ms = DT_INST_PROP(n, lag_time_us),                                                           \
                .measurement_us = DT_INST_PROP(n, measurement_us),                                                     \
            },                                                                                                         \
    };                                                                                                                 \
                                                                                                                       \
    SENSOR_DEVICE_DT_INST_DEFINE(n, or_magn_init, NULL, &or_magn_data_##n, &or_magn_config_##n, POST_KERNEL,           \
                                 CONFIG_SENSOR_INIT_PRIORITY, &or_magn_api);

DT_INST_FOREACH_STATUS_OKAY(OR_MAGN_INIT)