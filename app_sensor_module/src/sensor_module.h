#ifndef SENSOR_MODULE_H
#define SENSOR_MODULE_H

#include <launch_core/net/net_common.h>
#include <stdbool.h>

#define SENSOR_MODULE_IP_ADDR BACKPLANE_IP(SENSOR_MODULE_ID, 2, 1)

typedef enum {
    PAD_STATE = 0,
    PRE_MAIN_STATE,
    POST_MAIN_STATE,
    LANDING_STATE
} FLIGHT_STATES;

typedef enum {
    SENSOR_MODULE_ADXL375 = 0,
    SENSOR_MODULE_BMP388,
    SENSOR_MODULE_LIS3MDL,
    SENSOR_MODULE_LSM6DSL,
    SENSOR_MODULE_MS5611,
    SENSOR_MODULE_NUM_HUNDRED_HZ_SENSORS
} sensor_module_hundred_hz_sensor_t;


int init_networking(void);

/**
 * Start tasks for getting sensor data
 */
void start_sensor_tasks();

/**
 * Check if boost was detected
 * @return If boost was detected
 */
bool get_boost_detected();

/**
 * Check for a sudden altitude change and set boost_detected if threshold met
 */
void check_altitude_change(float pressure, float temperature);

/**
 * Check for a sudden acceleration change and set boost_detected if threshold met
 */
void check_acceleration_change(float accel_z);

#endif //SENSOR_MODULE_H
