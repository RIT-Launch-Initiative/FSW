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

int read_barom(const struct device *imu_dev, NTypes::SuperFastPacket *pac);
int read_imu(const struct device *imu_dev, NTypes::SuperFastPacket *packet, int frame);

static const struct device *superfast_storage = DEVICE_DT_GET(DT_NODE_BY_FIXED_PARTITION_LABEL(superfast_storage));

bool DONT_STOP = true;

int boost_and_flight_sensing(const struct device *imu_dev, const struct device *barom_dev,
                             FreakFlightController *freak_controller) {
    set_sampling(imu_dev);

    int64_t start = k_uptime_get();

    int fast_index = 0;
    NTypes::SuperFastPacket *packet = NULL;

    k_timer_start(&imutimer, K_MSEC(1), K_MSEC(1));
    int packets_sent = 0;

    bool has_swapped = false;

    bool already_imu_boosted = false;

    int64_t total_start = k_cycle_get_64();

    int ret = 0;

    freak_controller->SubmitEvent(Sources::LSM6DSL, Events::PadReady);
    freak_controller->SubmitEvent(Sources::BMP390, Events::PadReady);

    freak_controller->WaitUntilEvent(Events::PadReady);

    while (DONT_STOP && !freak_controller->HasEventOccurred(Events::GroundHit)) {
        if (!has_swapped && freak_controller->HasEventOccurred(Events::Coast)) {
            LOG_INF("Swapping");
            has_swapped = true;
            k_timer_stop(&imutimer);
            k_timer_start(&imutimer, K_MSEC(10), K_MSEC(10));
            gfs_signal_end_of_circle(superfast_storage);
            int64_t end = k_uptime_get();
            int64_t elapsed = end - start;
            float perSec = packets_sent / (elapsed / 1000.0f);

            LOG_INF("Its half over: %d packets in %lld ms. %f per second", packets_sent, elapsed, (double) perSec);
        }
        k_timer_status_sync(&imutimer);

        if (fast_index == 0) {
            ret = gfs_alloc_slab(superfast_storage, (void **) &packet, K_FOREVER);
            int64_t us = k_ticks_to_us_near64(k_uptime_ticks());
            packet->Timestamp = us;

            read_barom(barom_dev, packet);
        }
        read_imu(imu_dev, packet, fast_index % IMU_SAMPLES_PER_PACKET);

// You just gotta trust me its aligned sorry dude
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
        bool is_imu_boosted = feed_boost_acc(k_uptime_get(), &packet->AccelData[fast_index].X);
#pragma GCC diagnostic pop
        if (is_imu_boosted && !already_imu_boosted) {
            freak_controller->SubmitEvent(Sources::LSM6DSL, Events::Boost);
            already_imu_boosted = true;
        }

        fast_index++;
        if (fast_index == IMU_SAMPLES_PER_PACKET) {

            // send packet
            packets_sent++;
            ret = gfs_submit_slab(superfast_storage, packet, K_FOREVER);

            if (ret != 0) {
                LOG_WRN("Couldnt submit slab: %d", ret);
            }
            fast_index = 0;
        }
    }
    if (fast_index != 0) {
        // send final half packet
        packets_sent++;
        ret = gfs_submit_slab(superfast_storage, packet, K_FOREVER);
        if (ret != 0) {
            LOG_WRN("Couldnt submit final slab: %d", ret);
        }
    }
    int64_t total_elapsed = k_cycle_get_64() - total_start;

    int64_t end = k_uptime_get();
    int64_t elapsed = end - start;
    float perSec = packets_sent / (elapsed / 1000.0f);

    LOG_INF("Its all over: %d packets in %lld ms. %f per second", packets_sent, elapsed, (double) perSec);

    return 0;
}

int read_imu(const struct device *imu_dev, NTypes::SuperFastPacket *packet, int frame) {
    int ret = sensor_sample_fetch(imu_dev);
    if (ret != 0) {
        LOG_WRN("Couldnt fetch IMU");
        return ret;
    }
    struct sensor_value vals[3];
    ret = sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, vals);
    if (ret != 0) {
        LOG_WRN("Couldnt get axyz");
        return ret;
    }
    packet->AccelData[frame].X = sensor_value_to_float(&vals[0]);
    packet->AccelData[frame].Y = sensor_value_to_float(&vals[1]);
    packet->AccelData[frame].Z = sensor_value_to_float(&vals[2]);

    ret = sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_XYZ, vals);
    if (ret != 0) {
        LOG_WRN("Couldnt get gxyz");
        return ret;
    }
    packet->GyroData[frame].X = sensor_value_to_float(&vals[0]);
    packet->GyroData[frame].Y = sensor_value_to_float(&vals[1]);
    packet->GyroData[frame].Z = sensor_value_to_float(&vals[2]);
    return 0;
}

int read_barom(const struct device *barom_dev, NTypes::SuperFastPacket *packet) {
    int ret = sensor_sample_fetch(barom_dev);
    if (ret != 0) {
        LOG_WRN("Couldnt read barometer: %d", ret);
        return ret;
    }
    struct sensor_value val;
    ret = sensor_channel_get(barom_dev, SENSOR_CHAN_PRESS, &val);
    if (ret != 0) {
        LOG_WRN("Couldnt get pres from barometer: %d", ret);
    }
    packet->BaromData.Pressure = sensor_value_to_float(&val);

    ret = sensor_channel_get(barom_dev, SENSOR_CHAN_AMBIENT_TEMP, &val);
    if (ret != 0) {
        LOG_WRN("Couldnt get temp from barometer: %d", ret);
    }
    packet->BaromData.Temperature = sensor_value_to_float(&val);
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
