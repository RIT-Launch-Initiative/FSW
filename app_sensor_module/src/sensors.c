// Self Include
#include "sensors.h"

// stdlib Includes
#include <stdint.h>

// Launch Includes
#include <launch_core/types.h>
#include <launch_core/dev/dev_common.h>
#include <launch_core/dev/sensor.h>


// Zephyr Includes
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

// Constants
#define SENSOR_READING_STACK_SIZE 1024
#define HUNDRED_HZ_TELEM_PRIORITY 10
#define NUM_HUNDRED_HZ_SENSORS 5

// Forward Declarations
static void hundred_hz_sensor_reading_task(void *unused0, void *unused1, void *unused2);

// Threads
K_THREAD_DEFINE(hundred_hz_readings, SENSOR_READING_STACK_SIZE, hundred_hz_sensor_reading_task, NULL, NULL, NULL,
                HUNDRED_HZ_TELEM_PRIORITY, 0, 0);

// Message Queues
K_MSGQ_DEFINE(hundred_hz_telem_queue, sizeof(sensor_module_hundred_hz_telemetry_t), 16, 1);

// Sensor Channels
static const enum sensor_channel barometer_channels[] = {
        SENSOR_CHAN_PRESS,
        SENSOR_CHAN_AMBIENT_TEMP
};

static const enum sensor_channel accelerometer_channels[] = {
        SENSOR_CHAN_ACCEL_XYZ,
};

static const enum sensor_channel imu_channels[] = {
        SENSOR_CHAN_GYRO_XYZ,
        SENSOR_CHAN_ACCEL_XYZ,
};

static const enum sensor_channel magnetometer_channels[] = {
        SENSOR_CHAN_MAGN_XYZ,
};

static const enum sensor_channel temperature_channels[] = {
        SENSOR_CHAN_AMBIENT_TEMP
};

LOG_MODULE_REGISTER(sensing_tasks);

static void check_sensors_ready(const struct device *const *sensors, bool *sensor_ready, uint8_t num_sensors) {
    for (uint8_t i = 0; i < num_sensors; i++) {
        if (l_check_device(sensors[i]) == 0) {
            sensor_ready[i] = true;
        } else {
            LOG_INF("Sensor %d not ready", i);
            sensor_ready[i] = false;
        }
    }
}

static void hundred_hz_sensor_reading_task(void *unused0, void *unused1, void *unused2) {
    ARG_UNUSED(unused0);
    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);

    // Initialize variables for receiving telemetry
    sensor_module_hundred_hz_telemetry_t hundred_hz_telemetry;
    uint32_t timestamp = 0;

    const struct device *sensors[NUM_HUNDRED_HZ_SENSORS] = {
            [SENSOR_MODULE_ADXL375] = DEVICE_DT_GET(DT_INST(0, adi_adxl375)),
            [SENSOR_MODULE_MS5611]  = DEVICE_DT_GET(DT_INST(0, meas_ms5611)),
            [SENSOR_MODULE_BMP388]  = DEVICE_DT_GET(DT_INST(0, bosch_bmp388)),
            [SENSOR_MODULE_LSM6DSL] = DEVICE_DT_GET(DT_INST(0, st_lsm6dsl)),
            [SENSOR_MODULE_LIS3MDL] = DEVICE_DT_GET(DT_INST(0, st_lis3mdl_magn))
    };

    const enum sensor_channel *channels_arr[NUM_HUNDRED_HZ_SENSORS] = {
            accelerometer_channels,
            barometer_channels,
            barometer_channels,
            imu_channels,
            magnetometer_channels
    };

    // Confirm sensors are ready
    bool sensor_ready[NUM_HUNDRED_HZ_SENSORS] = {false};
    check_sensors_ready(sensors, sensor_ready, NUM_HUNDRED_HZ_SENSORS);

    while (true) {
        // Refresh sensor data
        l_update_sensors_safe(sensors, NUM_HUNDRED_HZ_SENSORS, sensor_ready);
        timestamp = k_uptime_get_32();

        // Get sensor data
        struct sensor_value sensor_values[3] = {0};


    }


}
