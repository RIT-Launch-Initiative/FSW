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
#define NUM_SENSORS 5

// Forward Declarations
static void hundred_hz_sensor_reading_task(void *unused0, void *unused1, void *unused2);

// Threads
K_THREAD_DEFINE(hundred_hz_readings, SENSOR_READING_STACK_SIZE, hundred_hz_sensor_reading_task, NULL, NULL, NULL,
                HUNDRED_HZ_TELEM_PRIORITY, 0, 0);

// Message Queues
K_MSGQ_DEFINE(hundred_hz_telem_queue, sizeof(sensor_module_hundred_hz_telemetry_t), 16, 1);

static bool check_and_add_sensor(const struct device *dev, const struct device **dev_arr, uint8_t *device_count) {
    if (l_check_device(dev)) {
        dev_arr[*device_count] = dev;
        (*device_count)++;
        return true;
    }

    return false;
}

static uint8_t populate_sensor_arr(const struct device **dev_arr, enum sensor_channel **channels_arr) {
    uint8_t device_count = 0;

    if (check_and_add_sensor(DEVICE_DT_GET(DT_INST(0, adi_adxl375)), dev_arr, &device_count)) {
        channels_arr[device_count] = (enum sensor_channel[]) {
                SENSOR_CHAN_ACCEL_X,
                SENSOR_CHAN_ACCEL_Y,
                SENSOR_CHAN_ACCEL_Z
        };
    }

    return device_count;
}

static void hundred_hz_sensor_reading_task(void *unused0, void *unused1, void *unused2) {
    ARG_UNUSED(unused0);
    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);

    const struct device *sensor_arr[NUM_SENSORS] = {};
    enum sensor_channel *channels_arr[NUM_SENSORS] = {};
    const uint8_t num_sensors = populate_sensor_arr(sensor_arr, channels_arr);

    sensor_module_hundred_hz_telemetry_t telemetry = {};




}
