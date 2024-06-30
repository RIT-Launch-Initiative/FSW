#include "openrocket_imu.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT openrocket_imu

LOG_MODULE_REGISTER(openrocket_imu, CONFIG_SENSOR_LOG_LEVEL);

extern const unsigned int or_packets_size;
extern const struct or_data_t *or_packets;

// Map openrocket values to sensor axes
static or_scalar_t map_ax(or_scalar_t vert, or_scalar_t lat, const struct or_imu_config *cfg) {
    if (cfg->vertical_axis == X) {
        if (cfg->vertical_axis_invert) {
            return vert;
        } else {
            return -vert;
        }
    } else if (cfg->lateral_axis == X) {
        return lat;
    } else {
        return 0;
    }
}
static or_scalar_t map_ay(or_scalar_t vert, or_scalar_t lat, const struct or_imu_config *cfg) {
    if (cfg->vertical_axis == Y) {
        if (cfg->vertical_axis_invert) {
            return vert;
        } else {
            return -vert;
        }
    } else if (cfg->lateral_axis == Y) {
        return lat;
    } else {
        return 0;
    }
}
static or_scalar_t map_az(or_scalar_t vert, or_scalar_t lat, const struct or_imu_config *cfg) {
    if (cfg->vertical_axis == Z) {
        if (cfg->vertical_axis_invert) {
            return vert;
        } else {
            return -vert;
        }
    } else if (cfg->lateral_axis == Z) {
        return lat;
    } else {
        return 0;
    }
}

#define INVERT(val, do_invert) (do_invert ? -val : val)

static or_scalar_t map_gx(or_scalar_t roll, or_scalar_t pitch, or_scalar_t yaw, const struct or_imu_config *cfg) {
    if (cfg->roll_axis == X) {
        return INVERT(roll, cfg->roll_axis_invert);
    } else if (cfg->pitch_axis == X) {
        return INVERT(pitch, cfg->pitch_axis_invert);
    } else if (cfg->yaw_axis == X) {
        return INVERT(yaw, cfg->pitch_axis_invert);
    } else {
        return 0;
    }
}
static or_scalar_t map_gy(or_scalar_t roll, or_scalar_t pitch, or_scalar_t yaw, const struct or_imu_config *cfg) {
    if (cfg->roll_axis == Y) {
        return INVERT(roll, cfg->roll_axis_invert);
    } else if (cfg->pitch_axis == Y) {
        return INVERT(pitch, cfg->pitch_axis_invert);
    } else if (cfg->yaw_axis == Y) {
        return INVERT(yaw, cfg->pitch_axis_invert);
    } else {
        return 0;
    }
}

static or_scalar_t map_gz(or_scalar_t roll, or_scalar_t pitch, or_scalar_t yaw, const struct or_imu_config *cfg) {
    if (cfg->roll_axis == Z) {
        return INVERT(roll, cfg->roll_axis_invert);
    } else if (cfg->pitch_axis == Z) {
        return INVERT(pitch, cfg->pitch_axis_invert);
    } else if (cfg->yaw_axis == Z) {
        return INVERT(yaw, cfg->pitch_axis_invert);
    } else {
        return 0;
    }
}

static void map_or_to_sensor(struct or_data_t *in, struct or_imu_data *out, const struct or_imu_config *cfg) {
    out->ax = map_ax(in->vert_accel, in->lat_accel, cfg);
    out->ay = map_ay(in->vert_accel, in->lat_accel, cfg);
    out->az = map_az(in->vert_accel, in->lat_accel, cfg);

    out->gx = map_gx(in->roll, in->pitch, in->yaw, cfg);
    out->gy = map_gy(in->roll, in->pitch, in->yaw, cfg);
    out->gz = map_gz(in->roll, in->pitch, in->yaw, cfg);
}

static int or_imu_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    const struct or_imu_config *cfg = dev->config;
    struct or_imu_data *data = dev->data;
    if (cfg->broken) {
        return -ENODEV;
    }
    if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_ACCEL_X && chan != SENSOR_CHAN_ACCEL_Y &&
        chan != SENSOR_CHAN_ACCEL_Z && chan != SENSOR_CHAN_ACCEL_XYZ && chan != SENSOR_CHAN_GYRO_X &&
        chan != SENSOR_CHAN_GYRO_Y && chan != SENSOR_CHAN_GYRO_Z && chan != SENSOR_CHAN_GYRO_XYZ) {
        return -ENOTSUP;
    }
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

        or_data.vert_accel = or_lerp(lo_data->vert_accel, hi_data->vert_accel, mix);
        or_data.lat_accel = or_lerp(lo_data->lat_accel, hi_data->lat_accel, mix);

        or_data.roll = or_lerp(lo_data->roll, hi_data->roll, mix);
        or_data.pitch = or_lerp(lo_data->pitch, hi_data->pitch, mix);
        or_data.yaw = or_lerp(lo_data->yaw, hi_data->yaw, mix);
    }
    map_or_to_sensor(&or_data, data, cfg);

    return 0;
}

static int or_imu_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val) {
    const struct or_imu_config *cfg = dev->config;
    struct or_imu_data *data = dev->data;

    if (cfg->broken) {
        return -ENODEV;
    }

    switch (chan) {
        case SENSOR_CHAN_ACCEL_X:
            sensor_value_from_or_scalar(val, data->ax);
            break;
        case SENSOR_CHAN_ACCEL_Y:
            sensor_value_from_or_scalar(val, data->ay);
            break;
        case SENSOR_CHAN_ACCEL_Z:
            sensor_value_from_or_scalar(val, data->az);
            break;
        case SENSOR_CHAN_ACCEL_XYZ:
            sensor_value_from_or_scalar(&val[0], data->ax);
            sensor_value_from_or_scalar(&val[1], data->ay);
            sensor_value_from_or_scalar(&val[2], data->az);
            break;
        case SENSOR_CHAN_GYRO_X:
            sensor_value_from_or_scalar(val, data->gx);
            break;
        case SENSOR_CHAN_GYRO_Y:
            sensor_value_from_or_scalar(val, data->gy);
            break;
        case SENSOR_CHAN_GYRO_Z:
            sensor_value_from_or_scalar(val, data->gz);
            break;
        case SENSOR_CHAN_GYRO_XYZ:
            sensor_value_from_or_scalar(&val[0], data->gx);
            sensor_value_from_or_scalar(&val[1], data->gy);
            sensor_value_from_or_scalar(&val[2], data->gz);
            break;
        default:
            LOG_DBG("Channel not supported by device");
            return -ENOTSUP;
    }
    return 0;
}

static int or_imu_init(const struct device *dev) {
    const struct or_imu_config *cfg = dev->config;
    if (cfg->broken) {
        return -ENODEV;
    }
    return 0;
}

static const struct sensor_driver_api or_imu_api = {
    .sample_fetch = or_imu_sample_fetch,
    .channel_get = or_imu_channel_get,
};

#define OR_IMU_INIT(n)                                                                                                 \
    static struct or_imu_data or_imu_data_##n;                                                                         \
                                                                                                                       \
    static const struct or_imu_config or_imu_config_##n = {                                                            \
        .broken = DT_INST_PROP(n, broken),                                                                             \
        .sampling_period_us = DT_INST_PROP(n, sampling_period_us),                                                     \
        .lag_time_ms = DT_INST_PROP(n, lag_time_us),                                                                   \
        .vertical_axis = DT_INST_STRING_TOKEN(n, vertical_axis),                                                       \
        .vertical_axis_invert = DT_INST_PROP(n, vertical_axis_invert),                                                 \
        .lateral_axis = DT_INST_STRING_TOKEN(n, lateral_axis),                                                         \
        .roll_axis = DT_INST_STRING_TOKEN(n, roll_axis),                                                               \
        .pitch_axis = DT_INST_STRING_TOKEN(n, pitch_axis),                                                             \
        .yaw_axis = DT_INST_STRING_TOKEN(n, yaw_axis),                                                                 \
        .roll_axis_invert = DT_INST_PROP(n, roll_axis_invert),                                                         \
        .pitch_axis_invert = DT_INST_PROP(n, pitch_axis_invert),                                                       \
        .yaw_axis_invert = DT_INST_PROP(n, yaw_axis_invert),                                                           \
    };                                                                                                                 \
                                                                                                                       \
    SENSOR_DEVICE_DT_INST_DEFINE(n, or_imu_init, NULL, &or_imu_data_##n, &or_imu_config_##n, POST_KERNEL,              \
                                 CONFIG_SENSOR_INIT_PRIORITY, &or_imu_api);

DT_INST_FOREACH_STATUS_OKAY(OR_IMU_INIT)
