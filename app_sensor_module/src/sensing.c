// Self Include
#include "sensor_module.h"

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
#define SENSOR_READING_STACK_SIZE            1024
#define HUNDRED_HZ_TELEM_PRIORITY            20
#define SENSOR_MODULE_SENSOR_COUNT 5

// Forward Declarations
static void hundred_hz_sensor_reading_task(void);

// Threads
K_THREAD_DEFINE(hundred_hz_readings, SENSOR_READING_STACK_SIZE, hundred_hz_sensor_reading_task, NULL, NULL, NULL,
                K_PRIO_PREEMPT(HUNDRED_HZ_TELEM_PRIORITY), 0, SENSOR_READ_THREAD_START_TIME);

// Message Queues
K_MSGQ_DEFINE(telem_queue, sizeof(sensor_module_telemetry_t), 16, 1);

// Extern Variables
extern struct k_msgq telem_logging_msgq;

bool logging_enabled = false;

LOG_MODULE_REGISTER(sensing_tasks);

static void check_sensors_ready(const struct device* const* sensors, bool* sensor_ready, uint8_t num_sensors) {
    for (uint8_t i = 0; i < num_sensors; i++) {
        if (l_check_device(sensors[i]) == 0) {
            sensor_ready[i] = true;
        } else {
            sensor_ready[i] = false;
        }
    }
}

static void setup_lsm6dsl() {
    const struct device* lsm6dsl = DEVICE_DT_GET_ONE(st_lsm6dsl);
    const struct sensor_value odr_attr = {.val1 = 104, .val2 = 0};

    if (sensor_attr_set(lsm6dsl, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
        LOG_ERR("Cannot set sampling frequency for LSM6DSL accelerometer.\n");
    }

    if (sensor_attr_set(lsm6dsl, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
        LOG_ERR("Cannot set sampling frequency for LSM6DSL gyroscope.\n");
    }
}

static void hundred_hz_sensor_reading_task(void) {
    LOG_INF("Starting sensor task");
    // Initialize variables for receiving telemetry
    sensor_module_telemetry_t telemetry;


    const struct device* adxl375 = DEVICE_DT_GET_ONE(adi_adxl375);
    const struct device* ms5611 = DEVICE_DT_GET_ONE(meas_ms5611);
    const struct device* bmp388 = DEVICE_DT_GET_ONE(bosch_bmp388);
    const struct device* lsm6dsl = DEVICE_DT_GET_ONE(st_lsm6dsl);
    const struct device* lis3mdl = DEVICE_DT_GET_ONE(st_lis3mdl_magn);

    const struct device* sensors[SENSOR_MODULE_SENSOR_COUNT] = {adxl375,
                                                                  ms5611,
                                                                  bmp388,
                                                                  lsm6dsl,
                                                                  lis3mdl};
    // Perform any necessary sensor setup
    setup_lsm6dsl();

    // Confirm sensors are ready
    bool sensor_ready[SENSOR_MODULE_SENSOR_COUNT] = {false};
    check_sensors_ready(sensors, sensor_ready, SENSOR_MODULE_SENSOR_COUNT);

    while (true) {
        uint32_t start = k_uptime_get();
        // Refresh sensor data
        for (uint8_t i = 0; i < SENSOR_MODULE_SENSOR_COUNT; i++) {
            if (sensor_sample_fetch(sensors[i])) {
                LOG_ERR("Failed to fetch %s data %d", sensors[i]->name, i);
            }
        }
        telemetry.timestamp = k_uptime_get();
        l_get_accelerometer_data_float(adxl375, &telemetry.adxl375);
        l_get_accelerometer_data_float(lsm6dsl, &telemetry.lsm6dsl_accel);
        l_get_gyroscope_data_float(lsm6dsl, &telemetry.lsm6dsl_gyro);
        l_get_barometer_data_float(ms5611, &telemetry.ms5611);
        l_get_barometer_data_float(bmp388, &telemetry.bmp388);
        l_get_magnetometer_data_float(lis3mdl, &telemetry.lis3mdl);
        LOG_INF("Took %d to read", k_uptime_get() - start);

        // Put telemetry into queue
        k_msgq_put(&telem_queue, &telemetry, K_MSEC(10));
        k_msgq_put(&telem_logging_msgq, &telemetry, K_MSEC(10));

        k_msleep(20);
    }
}
