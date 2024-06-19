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
#define SENSOR_MODULE_NUM_HUNDRED_HZ_SENSORS 5
#define HUNDRED_HZ_UPDATE_TIME               100 // TODO: Should be 10, but I2C bus dies

// Forward Declarations
static void hundred_hz_sensor_reading_task(void);

// Threads
K_THREAD_DEFINE(hundred_hz_readings, SENSOR_READING_STACK_SIZE, hundred_hz_sensor_reading_task, NULL, NULL, NULL,
                K_PRIO_PREEMPT(HUNDRED_HZ_TELEM_PRIORITY), 0, 1000);

// Timers
K_TIMER_DEFINE(hundred_hz_timer, NULL, NULL);

// Message Queues
K_MSGQ_DEFINE(hundred_hz_telem_queue, sizeof(timed_sensor_module_hundred_hz_telemetry_t), 16, 1);

// Extern Variables
extern struct k_msgq hun_hz_logging_msgq;
extern struct k_msgq ten_hz_logging_msgq;

bool logging_enabled = false;

// Variables (for boost detection)
float accel_z[DETECTION_METHOD_PER_SENSOR_COUNT] = {0};
float pressure[DETECTION_METHOD_PER_SENSOR_COUNT] = {0};
float temperature[DETECTION_METHOD_PER_SENSOR_COUNT] = {0};

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
    // Initialize timer
    k_timer_start(&hundred_hz_timer, K_MSEC(HUNDRED_HZ_UPDATE_TIME), K_MSEC(HUNDRED_HZ_UPDATE_TIME));

    // Initialize variables for receiving telemetry
    timed_sensor_module_hundred_hz_telemetry_t hundred_hz_telemetry;

    const struct device* adxl375 = DEVICE_DT_GET_ONE(adi_adxl375);
    const struct device* ms5611 = DEVICE_DT_GET_ONE(meas_ms5611);
    const struct device* bmp388 = DEVICE_DT_GET_ONE(bosch_bmp388);
    const struct device* lsm6dsl = DEVICE_DT_GET_ONE(st_lsm6dsl);
    const struct device* lis3mdl = DEVICE_DT_GET_ONE(st_lis3mdl_magn);

    const struct device* sensors[SENSOR_MODULE_NUM_HUNDRED_HZ_SENSORS] = {adxl375,
                                                                          ms5611,
                                                                          // bmp388,
                                                                          lsm6dsl,
                                                                          lis3mdl};

    // Perform any necessary sensor setup
    setup_lsm6dsl();

    // Confirm sensors are ready
    bool sensor_ready[SENSOR_MODULE_NUM_HUNDRED_HZ_SENSORS] = {false};
    check_sensors_ready(sensors, sensor_ready, SENSOR_MODULE_NUM_HUNDRED_HZ_SENSORS);

    while (true) {
        // TODO: Use sensor interrupts instead of timer
        k_timer_status_sync(&hundred_hz_timer);

        // Refresh sensor data
        for (uint8_t i = 0; i < SENSOR_MODULE_NUM_HUNDRED_HZ_SENSORS; i++) {
            if (sensor_sample_fetch(sensors[i])) {
                LOG_ERR("Failed to fetch %s data %d", sensors[i]->name, i);
            }
        }
        hundred_hz_telemetry.timestamp = k_uptime_get();
        l_get_accelerometer_data_float(adxl375, &hundred_hz_telemetry.data.adxl375);
        l_get_accelerometer_data_float(lsm6dsl, &hundred_hz_telemetry.data.lsm6dsl_accel);
        l_get_barometer_data_float(ms5611, &hundred_hz_telemetry.data.ms5611);
        l_get_barometer_data_float(bmp388, &hundred_hz_telemetry.data.bmp388);
        l_get_gyroscope_data_float(lsm6dsl, &hundred_hz_telemetry.data.lsm6dsl_gyro);
        l_get_magnetometer_data_float(lis3mdl, &hundred_hz_telemetry.data.lis3mdl);

        // Put telemetry into queue
        k_msgq_put(&hundred_hz_telem_queue, &hundred_hz_telemetry, K_MSEC(10));

        // Buffer up data for logging before boost. If no space, throw out the oldest entry.
        if (!logging_enabled && k_msgq_num_free_get(&hun_hz_logging_msgq) == 0) {
            timed_sensor_module_hundred_hz_telemetry_t throwaway_data;
            k_msgq_get(&hun_hz_logging_msgq, &throwaway_data, K_NO_WAIT);
        }

        k_msgq_put(&hun_hz_logging_msgq, &hundred_hz_telemetry, K_MSEC(10));

        // Fill data for boost detection
        // TODO: Need to validate on newer hardware. Sus slow trigger time during testing with fake vals.
        // Faulty bus known to affect how fast this loop executes
        accel_z[0] = hundred_hz_telemetry.data.adxl375.accel_z;
        accel_z[1] = hundred_hz_telemetry.data.lsm6dsl_accel.accel_z;

        pressure[0] = hundred_hz_telemetry.data.bmp388.pressure;
        temperature[0] = hundred_hz_telemetry.data.bmp388.temperature;

        pressure[1] = hundred_hz_telemetry.data.ms5611.pressure;
        temperature[1] = hundred_hz_telemetry.data.ms5611.temperature;
    }
}
