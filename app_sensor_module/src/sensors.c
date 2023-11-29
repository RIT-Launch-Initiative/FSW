#include <zephyr/drivers/sensor.h>
#include <zephyr/zephyr.h>
#include <zephyr/logging/log.h>
#include "sensors.h"


LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

static SENSOR_MODULE_DATA_T readings = {0};


static int check_dev(const struct device *device) {
    int ret = device_is_ready(device);

    if (ret) printk("Error %d: %s is not ready\n", ret, device->name);

    return ret;
}


void update_ms5607_readings(void *unused0, void *unused1, void *unused2) {
    const struct device *const ms5607_dev = DEVICE_DT_GET_ONE(meas_ms5607);
    struct sensor_value press[1];
    struct sensor_value temp[1];

    if (!device_is_ready(ms5607_dev)) {
        LOG_ERR("MS5607 device not ready");
        return;
    }


    while (1) {
        if (sensor_sample_fetch(ms5607_dev)) {
            printk("MS5607 sample update error");
        }

        if (sensor_channel_get(ms5607_dev, SENSOR_CHAN_PRESS, press)) {
            printk("Cannot read MS5607 pressure");
        }

        if (sensor_channel_get(ms5607_dev, SENSOR_CHAN_AMBIENT_TEMP, temp)) {
            printk("Cannot read MS5607 temperature");
        }

        printk("MS5607: Pressure: %f, Temperature: %f",
               sensor_value_to_double(press),
               sensor_value_to_double(temp));

    }

}



void update_tmp117_readings(void *unused0, void *unused1, void *unused2) {
    const struct device *device = DEVICE_DT_GET_ANY(ti_tmp116);

    struct sensor_value temperature = {0};   

    int ret = device_is_ready(device);
    if (ret) printk("Error %d: %s is not ready\n", ret, device->name);

    while (1) {
        if (sensor_sample_fetch(device)) continue;
        
        sensor_channel_get(device, SENSOR_CHAN_AMBIENT_TEMP, &temperature);

        readings.temperature_tmp = sensor_value_to_float(&temperature);

    }
}


void update_lsm6dsl_data(void *unused0, void *unused1, void *unused2) {
    const struct device *const lsm6dsl_dev = DEVICE_DT_GET_ONE(st_lsm6dsl);
    
    struct sensor_value accel[3] = {0};
    struct sensor_value gyro[3] = {0};

    while (1) {
        if (sensor_sample_fetch(lsm6dsl_dev) < 0) {
            printk("LSM6DSL sample update error");
        }

        if (sensor_channel_get(lsm6dsl_dev, SENSOR_CHAN_ACCEL_XYZ, accel) < 0) {
            printk("Cannot read LSM6DSL accel channels");
        }

        if (sensor_channel_get(lsm6dsl_dev, SENSOR_CHAN_GYRO_XYZ, gyro) < 0) {
            printk("Cannot read LSM6DSL accel channels");
        }
       
        readings.accel_x = sensor_value_to_float(&accel[0]);
        readings.accel_y = sensor_value_to_float(&accel[1]);
        readings.accel_z = sensor_value_to_float(&accel[2]);

        readings.gyro_x = sensor_value_to_float(&gyro[0]);
        readings.gyro_y = sensor_value_to_float(&gyro[1]);
        readings.gyro_z = sensor_value_to_float(&gyro[2]);


    }
}

void update_bmp388_data(void *unused0, void *unused1, void *unused2) {}

void update_adxl375_data(void *unused0, void *unused1, void *unused2) {}


void update_lis3mdl_data(void *unused0, void *unused1, void *unused2) {
    const struct device *const lis3mdl_dev = DEVICE_DT_GET_ONE(st_lis3mdl_magn);
    struct sensor_value magn[3];

    if (!device_is_ready(lis3mdl_dev)) {
        LOG_ERR("LIS3MDL device not ready");
        return;
    }

    while (1) {
        if (sensor_sample_fetch(lis3mdl_dev)) {
            printk("LIS3MDL sample update error");
        }

        if (sensor_channel_get(lis3mdl_dev, SENSOR_CHAN_MAGN_XYZ, magn)) {
            printk("Cannot read LIS3MDL magn channels");
        }

        printk("LIS3MDL: X: %f, Y: %f, Z: %f",
               sensor_value_to_double(&magn[0]),
               sensor_value_to_double(&magn[1]),
               sensor_value_to_double(&magn[2]));

        k_msleep(100);
    }
}
