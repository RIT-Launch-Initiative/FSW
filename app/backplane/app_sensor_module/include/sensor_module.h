#ifndef SENSOR_MODULE_H
#define SENSOR_MODULE_H

#include <launch_core_classic/net/net_common.h>
#include <launch_core_classic/types.h>
#include <stdbool.h>

#define SENSOR_MODULE_IP_ADDR             BACKPLANE_IP(SENSOR_MODULE_ID, 2, 1)
#define DETECTION_METHOD_PER_SENSOR_COUNT 2

#ifdef CONFIG_DEBUG
#define THREAD_START_TIME 0
#define SAMPLE_COUNT 10
#define SENSOR_READ_THREAD_START_TIME 0
#else
#define THREAD_START_TIME 60000 * 5 // 5 minutes
#define SAMPLE_COUNT 512000000 / sizeof(sensor_module_telemetry_t)
#define SENSOR_READ_THREAD_START_TIME 60000 * 5
#endif

typedef enum { PAD_STATE = 0, PRE_MAIN_STATE, POST_MAIN_STATE, LANDING_STATE } FLIGHT_STATES;

typedef enum {
    SENSOR_MODULE_ADXL375 = 0,
    SENSOR_MODULE_BMP388,
    SENSOR_MODULE_LIS3MDL,
    SENSOR_MODULE_LSM6DSL,
    SENSOR_MODULE_MS5611,
    SENSOR_MODULE_NUM_HUNDRED_HZ_SENSORS
} sensor_module_hundred_hz_sensor_t;

typedef struct __attribute__((packed)) {
    uint64_t timestamp;
    l_accelerometer_data_t adxl375;
    l_accelerometer_data_t lsm6dsl_accel;

    l_barometer_data_t ms5611;
    l_barometer_data_t bmp388;

    l_gyroscope_data_t lsm6dsl_gyro;

    l_magnetometer_data_t lis3mdl;
    l_temperature_data_t tmp117;
} sensor_module_telemetry_t;

typedef struct __attribute__((packed)) {
    uint64_t timestamp;
    sensor_module_ten_hz_telemetry_t data;
} timed_sensor_module_ten_hz_telemetry_t;

void init();

int init_networking(void);

/**
 * Start tasks for getting sensor data
 */
void start_sensor_tasks();


#endif //SENSOR_MODULE_H
