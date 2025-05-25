#include "fast_sensing.h"

#include "boost.h"
#include "data.h"
#include "gorbfs.h"

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
LOG_MODULE_REGISTER(sensing);

K_TIMER_DEFINE(imutimer, NULL, NULL);
int set_sampling(const struct device *imu_dev);
static const struct device *superfast_storage = DEVICE_DT_GET(DT_NODE_BY_FIXED_PARTITION_LABEL(superfast_storage));

bool DONT_STOP = true;

int flight_sensing(const struct device *imu_dev, const struct device *barom_dev,
                   FreakFlightController *freak_controller) {
    set_sampling(imu_dev);

    int64_t start = k_uptime_get();

    int frame = 0;
    SuperFastPacket *packet = NULL;

    k_timer_start(&imutimer, K_USEC(100), K_USEC(100));
    int packets_sent = 0;

    bool has_boosted = false;

    int64_t total_start = k_cycle_get_64();

    int ret = 0;

    while (DONT_STOP && !freak_controller->HasEventOccurred(Events::GroundHit)) {
        if (freak_controller->HasEventOccurred(Events::Coast)) {
            // storage target = non-looping
            // timer set to lower value as necessary
        }
        k_timer_status_sync(&imutimer);

        if (frame == 0) {
            ret = gfs_alloc_slab(superfast_storage, (void **) &packet, K_FOREVER);
            int64_t us = k_ticks_to_us_near64(k_uptime_ticks());
            packet->timestamp = us;

            // read barometer
            ret = sensor_sample_fetch(barom_dev);
            if (ret != 0) {
                LOG_WRN("Couldnt read barometer: %d", ret);
            }
            struct sensor_value val;
            ret = sensor_channel_get(barom_dev, SENSOR_CHAN_PRESS, &val);
            if (ret != 0) {
                LOG_WRN("Couldnt get pres from barometer: %d", ret);
            }
            packet->pressure = sensor_value_to_float(&val);

            ret = sensor_channel_get(barom_dev, SENSOR_CHAN_AMBIENT_TEMP, &val);
            if (ret != 0) {
                LOG_WRN("Couldnt get temp from barometer: %d", ret);
            }
            packet->temp = sensor_value_to_float(&val);
        }
        // read imu
        sensor_sample_fetch(imu_dev);
        struct sensor_value vals[3];
        ret = sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, vals);
        if (ret != 0) {
            LOG_WRN("Couldnt get axyz");
        }
        packet->adat[frame].ax = sensor_value_to_float(&vals[0]);
        packet->adat[frame].ay = sensor_value_to_float(&vals[1]);
        packet->adat[frame].az = sensor_value_to_float(&vals[2]);

        ret = sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_XYZ, vals);
        if (ret != 0) {
            LOG_WRN("Couldnt get gxyz");
        }
        packet->gdat[frame].gx = sensor_value_to_float(&vals[0]);
        packet->gdat[frame].gy = sensor_value_to_float(&vals[1]);
        packet->gdat[frame].gz = sensor_value_to_float(&vals[2]);

        bool is_boosted = feed_boost_acc(k_uptime_get(), &packet->adat[frame].ax);
        if (is_boosted) {
        }

        frame++;
        if (frame == IMU_SAMPLES_PER_PACKET) {

            // send packet
            packets_sent++;
            ret = gfs_submit_slab(superfast_storage, packet, K_FOREVER);

            if (ret != 0) {
                LOG_WRN("Couldnt submit slab: %d", ret);
            }
            frame = 0;
        }
    }
    if (frame != 0) {
        // send half packet
        packets_sent++;
        ret = gfs_submit_slab(superfast_storage, packet, K_FOREVER);
        if (ret != 0) {
            LOG_WRN("Couldnt submit final slab: %d", ret);
        }
    }
    lock_boostdata();
    int64_t total_elapsed = k_cycle_get_64() - total_start;

    int64_t end = k_uptime_get();
    int64_t elapsed = end - start;
    float perSec = packets_sent / (elapsed / 1000.0f);

    LOG_INF("Its all over: %d packets in %lld ms. %f per second", packets_sent, elapsed, (double) perSec);

    return 0;
}

int set_sampling(const struct device *imu_dev) {
    struct sensor_value sampling = {0};
    sensor_value_from_float(&sampling, 1666);
    int ret = sensor_attr_set(imu_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling);
    if (ret < 0) {
        LOG_ERR("Couldnt set sampling\n");
    }
    ret = sensor_attr_set(imu_dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling);
    if (ret < 0) {
        LOG_ERR("Couldnt set sampling\n");
    }
    return 0;
}

int cmd_stop(const struct shell *shell, size_t argc, char **argv) {
    DONT_STOP = false;
    shell_print(shell, "Stopping");
    return 0;
}
