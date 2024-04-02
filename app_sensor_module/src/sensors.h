#ifndef SENSOR_MODULE_SENSORS_H
#define SENSOR_MODULE_SENSORS_H

typedef enum {
    SENSOR_MODULE_ADXL375,
    SENSOR_MODULE_BMP388,
    SENSOR_MODULE_LIS3MDL,
    SENSOR_MODULE_LSM6DSL,
    SENSOR_MODULE_MS5611,
    SENSOR_MODULE_TMP117,
} sensor_module_sensor_t;

/**
 * Start tasks for getting sensor data
 */
void start_sensor_tasks();

#endif //SENSOR_MODULE_SENSORS_H
