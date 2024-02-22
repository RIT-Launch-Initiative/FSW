#include <zephyr/drivers/sensor.h>
#ifndef SENSORS_H_
#define SENSORS_H_


typedef struct __attribute__((__packed__)) {
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

    //float temperature_tmp;
} SENSOR_MODULE_DATA_T;

typedef struct {
    uint32_t timestamp;      // Timestamp to record the time of the measurement
    float temperature_data; // Temperature raw data from TMP116
} tmp116_telemetry_t;

#endif // !SENSORS_H_


