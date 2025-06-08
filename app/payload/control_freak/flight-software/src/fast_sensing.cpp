#include "fast_sensing.h"

#include "boost.h"
#include "common.h"
#include "gorbfs.h"
#include "slow_sensing.h"

#include <cmath>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
LOG_MODULE_REGISTER(sensing);

K_TIMER_DEFINE(imutimer, NULL, NULL);
K_TIMER_DEFINE(slowdata_timer, NULL, NULL);

static int set_sampling(const struct device *imu_dev);

bool DONT_STOP = true;

int boost_and_flight_sensing(const struct device *superfast_storage, const struct device *imu_dev,
                             const struct device *barom_dev, const struct device *ina_servo,
                             FreakFlightController *freak_controller) {
    set_sampling(imu_dev);

    int64_t start = k_uptime_get();

    int fast_index = 0;
    NTypes::SuperFastPacket *packet = NULL;

    k_timer_start(&imutimer, K_USEC(920), K_USEC(920));
    k_timer_start(&slowdata_timer, K_SECONDS(1), K_SECONDS(1));
    int packets_sent = 0;

    FlightState flight_state = FlightState::NotSet;
    bool already_imu_boosted = false;

    int ret = 0;

    freak_controller->SubmitEvent(Sources::LSM6DSL, Events::PadReady);
    freak_controller->SubmitEvent(Sources::BMP390, Events::PadReady);

    freak_controller->WaitUntilEvent(Events::PadReady);
    flight_state = FlightState::OnPad;

    while (DONT_STOP && !freak_controller->HasEventOccurred(Events::GroundHit)) {
        if (flight_state == FlightState::OnPad && freak_controller->HasEventOccurred(Events::Boost)) {
            LOG_INF("Fast thread boost");
            flight_state = FlightState::Boost;
        } else if (flight_state == FlightState::Boost && freak_controller->HasEventOccurred(Events::Coast)) {
            LOG_INF("Swapping");
            flight_state = FlightState::Flight;

            gfs_signal_end_of_circle(superfast_storage);
            int64_t end = k_uptime_get();
            int64_t elapsed = end - start;
            float perSec = packets_sent / (elapsed / 1000.0f);
            lock_boostdata();
            LOG_INF("Its half over: %d packets in %lld ms. %f per second", packets_sent, elapsed, (double) perSec);
        }
        k_timer_status_sync(&imutimer);

        if (fast_index == 0) {
            ret = gfs_alloc_slab(superfast_storage, (void **) &packet, K_FOREVER);
            if (ret != 0) {
                LOG_WRN("Couldn alloc fast buffer");
                continue;
            }
            int64_t us = k_ticks_to_us_near64(k_uptime_ticks());
            packet->Timestamp = us;
            float temp = 0;
            float press = 0;
            read_barom(barom_dev, temp, press);
            packet->BaromData.Temperature = temp;
            packet->BaromData.Pressure = press;
        }
        NTypes::AccelerometerData &acc = packet->AccelData[fast_index % IMU_SAMPLES_PER_PACKET];
        NTypes::GyroscopeData &gyro = packet->GyroData[fast_index % IMU_SAMPLES_PER_PACKET];
        ret = read_imu(imu_dev, acc, gyro);
        if (ret < 0) {
            LOG_WRN("Failed to read IMU");
        }

        if (!already_imu_boosted) {
// You just gotta trust me its aligned sorry dude
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
            bool is_imu_boosted = feed_boost_acc(k_uptime_ticks(), packet->AccelData[fast_index]);
#pragma GCC diagnostic pop
            if (is_imu_boosted) {
                freak_controller->SubmitEvent(Sources::LSM6DSL, Events::Boost);
                already_imu_boosted = true;
            }
        }
        if (fast_index == 0) {
            if (k_timer_status_get(&slowdata_timer) > 0) {
                // slow readings timer expired
                NTypes::AccelerometerData normed = normalize(packet->AccelData[0]);
                small_orientation snormed = minify_orientation(normed);
                float temp = packet->BaromData.Temperature;
                float press = packet->BaromData.Pressure;
                float current = 0;
                float volts = 0;
                ret = read_ina(ina_servo, volts, current);
                if (ret != 0) {
                    LOG_WRN("Couldnt read ina: %d", ret);
                }
                ret = submit_slowdata(snormed, temp, current, volts, FLIP_STATE_NOT_TRYING, flight_state);
                if (ret != 0) {
                    LOG_WRN("Couldn submit slowdata: %d", ret);
                }

                ret = submit_horus_data(temp, press, volts, snormed, flight_state);
                if (ret != 0) {
                    LOG_WRN("Couldn submit slowdata: %d", ret);
                }
            }
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

    int64_t end = k_uptime_get();
    int64_t elapsed = end - start;
    float perSec = packets_sent / (elapsed / 1000.0f);

    LOG_INF("Its all over: %d packets in %lld ms. %f per second", packets_sent, elapsed, (double) perSec);

    return 0;
}

static int set_sampling(const struct device *imu_dev) {
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
