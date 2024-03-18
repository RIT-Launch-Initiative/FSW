#ifndef L_SENSOR_DATA_TYPES_H
#define L_SENSOR_DATA_TYPES_H

#include <stdint.h>

/****** Primitive Data Types ******/
// TODO: See if we can support __fp16 in GSW first
// typedef __fp16 float16_t;

/****** Telemetry Data Types ******/
typedef struct __attribute__((packed)) {
    float accel_x;
    float accel_y;
    float accel_z;
} l_accelerometer_data_t;

typedef struct __attribute__((packed)) {
    float pressure;
    float temperature;
} l_barometer_data_t;

typedef struct __attribute__((packed)) {
    float gyro_x;
    float gyro_y;
    float gyro_z;
} l_gyroscope_data_t;

typedef struct __attribute__((packed)) {
    float mag_x;
    float mag_y;
    float mag_z;
} l_magnetometer_data_t;

typedef struct __attribute__((packed)) {
    float current;
    float voltage;
    float power;
} l_shunt_data_t;

typedef float l_temperature_data_t;

/********** Module Data Types **********/
typedef struct __attribute__((packed)) {
    uint32_t timestamp;
    l_shunt_data_t data_battery;
    l_shunt_data_t data_3v3;
    l_shunt_data_t data_5v0;
    int16_t vin_adc_data_mv;
} power_module_telemetry_t;


#endif
