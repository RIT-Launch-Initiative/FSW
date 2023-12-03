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

int check_dev(const struct device *device);

void update_sensor_data(void *dev_arg, void *args, void *process_float_arg) {
#endif // !SENSORS_H_


