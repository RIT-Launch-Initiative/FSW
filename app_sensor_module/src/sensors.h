#ifndef SENSORS_H_
#define SENSORS_H_


#include "zephyr/drivers/sensor.h"
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

    float temperature_tmp;
} SENSOR_MODULE_DATA_T;

typedef struct {
    int num_readings;
    enum sensor_channel *channels;
    struct sensor_value **values;
    float **float_values;
} SENSOR_READINGS_ARGS_T;

void update_sensor_data(void *dev, void *sensor_args, void *process_float);
void update_adxl375_data(void *unused0, void *unused1, void *unused2);
void update_bmp388_data(void *unused0, void *unused1, void *unused2);
void update_ms5607_data(void *unused0, void *unused1, void *unused2);
void update_lsm6dsl_data(void *unused0, void *unused1, void *unused2);
void update_lis3mdl_data(void *unused0, void *unused1, void *unused2);
void update_tmp117_data(void *unused0, void *unused1, void *unused2);

#endif // !SENSORS_H_


