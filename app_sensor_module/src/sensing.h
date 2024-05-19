#ifndef SENSOR_MODULE_SENSING_H
#define SENSOR_MODULE_SENSING_H

typedef enum {
    SENSOR_MODULE_ADXL375 = 0,
    SENSOR_MODULE_BMP388,
    SENSOR_MODULE_LIS3MDL,
    SENSOR_MODULE_LSM6DSL,
    SENSOR_MODULE_MS5611,
    SENSOR_MODULE_NUM_HUNDRED_HZ_SENSORS
} sensor_module_hundred_hz_sensor_t;

/**
 * Start tasks for getting sensor data
 */
void start_sensor_tasks();

#endif //SENSOR_MODULE_SENSING_H
