#ifndef L_SENSOR_DATA_TYPES_H
#define L_SENSOR_DATA_TYPES_H

typedef struct {
    float accel_x;
    float accel_y;
    float accel_z;
} l_accelerometer_data_t;

typedef struct __attribute__((packed)) {
    float accel_x;
    float accel_y;
    float accel_z;
} l_accelerometer_data_packed_t;

typedef struct {
    float pressure;
    float temperature;
} l_barometer_data_t;

typedef struct __attribute__((packed)) {
    float pressure;
    float temperature;
} l_barometer_data_packed_t;

typedef struct {
    float gyro_x;
    float gyro_y;
    float gyro_z;
} l_gyroscope_data_t;

typedef struct __attribute__((packed)) {
    float gyro_x;
    float gyro_y;
    float gyro_z;
} l_gyroscope_data_packed_t;

typedef struct {
    float mag_x;
    float mag_y;
    float mag_z;
} l_magnetometer_data_t;

typedef struct __attribute__((packed)) {
    float mag_x;
    float mag_y;
    float mag_z;
} l_magnetometer_data_packed_t;

typedef float l_temperature_data_t;
typedef float l_temperature_data_float_t; // Just to keep things consistent

#endif
