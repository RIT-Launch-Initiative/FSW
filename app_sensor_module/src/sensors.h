#ifndef SENSORS_H_
#define SENSORS_H_


typedef struct {
    float accel_x;
    float accel_y;
    float accel_z;
} accelerometer_data_t;

typedef struct __attribute__((packed)) {
    float accel_x;
    float accel_y;
    float accel_z;
} accelerometer_data_packed_t;

typedef struct {
    float pressure;
    float temperature;
} barometer_data_t;

typedef struct __attribute__((packed)) {
    float pressure;
    float temperature;
} barometer_data_packed_t;

typedef struct {
    float gyro_x;
    float gyro_y;
    float gyro_z;
} gyroscope_data_t;

typedef struct __attribute__((packed)) {
    float gyro_x;
    float gyro_y;
    float gyro_z;
} gyroscope_data_packed_t;

typedef struct {
    float mag_x;
    float mag_y;
    float mag_z;
} magnetometer_data_t;

typedef struct __attribute__((packed)) {
    float mag_x;
    float mag_y;
    float mag_z;
} magnetometer_data_packed_t;

typedef float temperature_data_t;
typedef float temperature_data_float_t; // Just to keep things consistent


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
} sensor_module_telemetry_t;

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


