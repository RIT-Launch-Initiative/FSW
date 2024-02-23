#ifndef SENSORS_H_
#define SENSORS_H_

#include <launch_core/dev/sensor_data_types.h>

typedef struct {
    accelerometer_data_t adxl375;
    accelerometer_data_t lsm6dsl_accel;
    
    barometer_data_t ms5611;
    barometer_data_t bmp388;

    gyroscope_data_t lsm6dsl_gyro;

    magnetometer_data_t lis3mdl;

    temperature_data_t LL

} hundred_hz_telemetry_t;

typedef struct {
    float pressure_ms5;
    float temperature_ms5;

    float pressure_bmp3;
    float temperature_bmp3;

    float accel_x;
    float accel_y;
    float accel_z;
   
    float magn_x;
    float magn_y;
    float magn_z;

    float gyro_x;
    float gyro_y;
    float gyro_z;

    float temperature_tmp;
} sensor_module_telemetry_packed_t;



#endif // !SENSORS_H_


