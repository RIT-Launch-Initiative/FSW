// Self Include
#include "sensing.h"

// stdlib Includes
#include <stdint.h>

// Launch Includes
#include <launch_core/dev/dev_common.h>
#include <launch_core/dev/sensor.h>

// Zephyr Includes
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

// Constants
#define SENSOR_READING_STACK_SIZE 1024
#define HUNDRED_HZ_TELEM_PRIORITY 5
#define SENSOR_MODULE_NUM_HUNDRED_HZ_SENSORS 4
#define HUNDRED_HZ_UPDATE_TIME 10

// Forward Declarations
static void hundred_hz_sensor_reading_task(void *unused0, void *unused1, void *unused2);

// Threads
K_THREAD_DEFINE(hundred_hz_readings, SENSOR_READING_STACK_SIZE, hundred_hz_sensor_reading_task, NULL, NULL, NULL,
                K_PRIO_PREEMPT(HUNDRED_HZ_TELEM_PRIORITY), 0, 1000);

// Message Queues
K_MSGQ_DEFINE(hundred_hz_telem_queue, sizeof(sensor_module_hundred_hz_telemetry_t), 16, 1);

LOG_MODULE_REGISTER(sensing_tasks);

static void check_sensors_ready(const struct device *const *sensors, bool *sensor_ready, uint8_t num_sensors) {
    for (uint8_t i = 0; i < num_sensors; i++) {
        if (l_check_device(sensors[i]) == 0) {
            sensor_ready[i] = true;
        } else {
            sensor_ready[i] = false;
        }
    }
}

static void setup_lsm6dsl() {
    const struct device *lsm6dsl = DEVICE_DT_GET_ONE(st_lsm6dsl);
    const struct sensor_value odr_attr = {
            .val1 = 104,
            .val2 = 0
    };

    if (sensor_attr_set(lsm6dsl, SENSOR_CHAN_ACCEL_XYZ,
                        SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
        LOG_ERR("Cannot set sampling frequency for LSM6DSL accelerometer.\n");
    }

    if (sensor_attr_set(lsm6dsl, SENSOR_CHAN_GYRO_XYZ,
                        SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
        LOG_ERR("Cannot set sampling frequency for LSM6DSL gyroscope.\n");
    }
}

static void hundred_hz_sensor_reading_task(void *unused0, void *unused1, void *unused2) {
    ARG_UNUSED(unused0);
    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);

    // Initialize variables for receiving telemetry
    sensor_module_hundred_hz_telemetry_t hundred_hz_telemetry;
    uint32_t timestamp = 0;

    const struct device *adxl375 = DEVICE_DT_GET_ONE(adi_adxl375);
    //    const struct device *ms5611 = DEVICE_DT_GET_ONE(meas_ms5611);
    const struct device *bmp388 = DEVICE_DT_GET_ONE(bosch_bmp388);
    const struct device *lsm6dsl = DEVICE_DT_GET_ONE(st_lsm6dsl);
    const struct device *lis3mdl = DEVICE_DT_GET_ONE(st_lis3mdl_magn);

    const struct device *sensors[SENSOR_MODULE_NUM_HUNDRED_HZ_SENSORS] = {adxl375,
                                                                          //            ms5611,
                                                                          bmp388, lsm6dsl, lis3mdl};

    setup_lsm6dsl();

    // Confirm sensors are ready
    bool sensor_ready[SENSOR_MODULE_NUM_HUNDRED_HZ_SENSORS] = {false};
    check_sensors_ready(sensors, sensor_ready, SENSOR_MODULE_NUM_HUNDRED_HZ_SENSORS);


    while (true) {
        // Refresh sensor data
        timestamp = k_uptime_get_32();

        int ret = sensor_sample_fetch(adxl375);
        if (ret) {
            LOG_ERR("Failed to fetch adxl375 data %d", ret);
        }

        ret = sensor_sample_fetch(bmp388);
        if (ret) {
            LOG_ERR("Failed to fetch bmp388 data %d", ret);
        }

        sensor_sample_fetch(lsm6dsl);
        if (ret) {
            LOG_ERR("Failed to fetch lsm6dsl data %d", ret);
        }

        sensor_sample_fetch(lis3mdl);
        if (ret) {
            LOG_ERR("Failed to fetch lis3mdl data %d", ret);
        }

        LOG_INF("Took %d to get new data", k_uptime_get_32() - timestamp);

        timestamp = k_uptime_get_32();

        l_get_accelerometer_data_float(adxl375, &hundred_hz_telemetry.adxl375);
        l_get_accelerometer_data_float(lsm6dsl, &hundred_hz_telemetry.lsm6dsl_accel);
        //        l_get_barometer_data_float(ms5611, &hundred_hz_telemetry.ms5611);
        l_get_barometer_data_float(bmp388, &hundred_hz_telemetry.bmp388);
        l_get_gyroscope_data_float(lsm6dsl, &hundred_hz_telemetry.lsm6dsl_gyro);
        l_get_magnetometer_data_float(lis3mdl, &hundred_hz_telemetry.lis3mdl);


        // Put telemetry into queue
        if (k_msgq_put(&hundred_hz_telem_queue, &hundred_hz_telemetry, K_MSEC(10))) {
            LOG_ERR("Failed to put data into sensor processing queue");
        } else {
            LOG_INF("Queued telemetry");
        }

        // Sleep until next update time. TODO: Maybe use timers and signals?
        k_msleep(500);
//        uint32_t time_to_wait = HUNDRED_HZ_UPDATE_TIME - (k_uptime_get_32() - timestamp);
//        if (time_to_wait > 0) {
//            k_sleep(K_MSEC(time_to_wait));
//        }
    }
}