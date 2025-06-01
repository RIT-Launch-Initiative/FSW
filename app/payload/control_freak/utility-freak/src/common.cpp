#include "common.h"

#include <cmath>
#include <zephyr/drivers/sensor.h>

int read_ina(const struct device *ina_dev, float &voltage, float &current) {

    struct sensor_value value;
    int ret = sensor_sample_fetch(ina_dev);
    if (ret != 0) {
        return ret;
    }
    sensor_channel_get(ina_dev, SENSOR_CHAN_CURRENT, &value);
    current = sensor_value_to_float(&value);
    if (voltage > 78) {
        // man the EEs gotta be missing something
        current = 0;
    }

    sensor_channel_get(ina_dev, SENSOR_CHAN_VOLTAGE, &value);
    voltage = sensor_value_to_float(&value);
    return 0;
}

int set_lsm_sampling(const struct device *imu_dev, int odr) {
    struct sensor_value sampling = {0};
    sensor_value_from_float(&sampling, odr);
    int ret = sensor_attr_set(imu_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling);
    if (ret < 0) {
        printk("LSM6DSL: Couldnt set accel sampling\n");
        return ret;
    }
    ret = sensor_attr_set(imu_dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling);
    if (ret < 0) {
        printk("LSM6DSL: Couldnt set gyro sampling\n");
        return ret;
    }
    return 0;
}

NTypes::AccelerometerData normalize(NTypes::AccelerometerData acc) {
    float magn = sqrtf(acc.X * acc.X + acc.Y * acc.Y + acc.Z * acc.Z);
    return {acc.X / magn, acc.Y / magn, acc.Z / magn};
}
