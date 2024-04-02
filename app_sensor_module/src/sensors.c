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

static bool check_and_populate(const struct device *dev_to_place, const struct device *dev_arr_space,
                                             const enum sensor_channel *channels_to_place,
                                             const enum sensor_channel *channel_arr_space) {
    if (l_check_device(dev_to_place)) {
        dev_arr_space = dev_to_place;
        channel_arr_space = channels_to_place;
        return true;
    }

    return false;
}

static uint8_t populate_sensor_arr(const struct device **dev_arr, const enum sensor_channel **channels_arr) {
    uint8_t device_count = 0;

    // TODO: Try and find a way to iterate over this cleanly
    if (check_and_populate(DEVICE_DT_GET(DT_INST(0, adi_adxl375)), dev_arr[device_count],
                                         accelerometer_channels, channels_arr[device_count])) {
        device_count++;
    }
    if (check_and_populate(DEVICE_DT_GET(DT_INST(0, meas_ms5611)), dev_arr[device_count],
                                         barometer_channels, channels_arr[device_count])) {
        device_count++;
    }

    if (check_and_populate(DEVICE_DT_GET(DT_INST(0, bosch_bmp388)), dev_arr[device_count],
                                         barometer_channels, channels_arr[device_count])) {
        device_count++;
    }

    if (check_and_populate(DEVICE_DT_GET(DT_INST(0, st_lsm6dsl)), dev_arr[device_count],
                                         imu_channels, channels_arr[device_count])) {
        device_count++;
    }

    if (check_and_populate(DEVICE_DT_GET(DT_INST(0, st_lis3mdl_magn)), dev_arr[device_count],
                                         magnetometer_channels, channels_arr[device_count])) {
        device_count++;
    }

    return device_count;
}

static void hundred_hz_sensor_reading_task(void *unused0, void *unused1, void *unused2) {
    ARG_UNUSED(unused0);
    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);

    const struct device *sensor_arr[NUM_HUNDRED_HZ_SENSORS] = {};
    const enum sensor_channel *channels_arr[NUM_HUNDRED_HZ_SENSORS] = {};
    const uint8_t num_sensors = populate_sensor_arr(sensor_arr, channels_arr);

    sensor_module_hundred_hz_telemetry_t telemetry = {};


}
