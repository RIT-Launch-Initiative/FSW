#ifndef SENSORS_H_
#define SENSORS_H_

#include <launch_core/types.h>

typedef l_temperature_data_t ten_hz_telemetry_t;

typedef struct __attribute__((packed)) {
    l_accelerometer_data_t adxl375;
    l_accelerometer_data_t lsm6dsl_accel;
    
    l_barometer_data_t ms5611;
    l_barometer_data_t bmp388;

    l_gyroscope_data_t lsm6dsl_gyro;

    l_magnetometer_data_t lis3mdl;
} hundred_hz_telemetry_packed_t;

#endif // !SENSORS_H_


