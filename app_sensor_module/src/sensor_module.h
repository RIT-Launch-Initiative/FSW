#ifndef SENSOR_MODULE_H
#define SENSOR_MODULE_H

#include <launch_core/net/net_common.h>
#include <launch_core/types.h>

#include <stdbool.h>

#define SENSOR_MODULE_IP_ADDR BACKPLANE_IP(SENSOR_MODULE_ID, 2, 1)
#define DETECTION_METHOD_PER_SENSOR_COUNT 2

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

void init();

int init_networking(void);

/**
 * Start tasks for getting sensor data
 */
void start_sensor_tasks();

/**
 * Start boost detection timers
 */
void start_boost_detect();

/**
 * Stop boost detection timers
 */
void stop_boost_detect();

/**
 * Check if boost was detected
 * @return If boost was detected
 */
bool get_boost_detected();

int init_modbus_client(void);

int write_boost_detect_byte_modbus(uint8_t event_byte);

int read_potato_telemetry(float *pressure, float *temperature, float *load);

#endif //SENSOR_MODULE_H
